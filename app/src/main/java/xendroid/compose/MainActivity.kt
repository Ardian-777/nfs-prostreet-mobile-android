package xendroid.compose

import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.os.Bundle
import android.util.Log
import android.view.KeyEvent
import android.annotation.SuppressLint
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import xendroid.compose.ui.design.InputModePref
import xendroid.compose.ui.design.InputModeStore
import xendroid.compose.ui.design.LocalInputMode
import xendroid.compose.ui.design.LocalXdToast
import xendroid.compose.ui.design.XdToastHost
import xendroid.compose.ui.design.XdToastState
import xendroid.compose.ui.design.rememberControllerConnected
import androidx.compose.runtime.getValue
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch
import xendroid.compose.core.EmulatorRuntime
import xendroid.compose.core.FrontendLaunch
import xendroid.compose.ui.library.ACTION_LAUNCH_GAME
import xendroid.compose.ui.library.EXTRA_GAME_URI
import xendroid.compose.core.SessionLogs
import xendroid.compose.saves.BackupSync
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import xendroid.compose.ui.AppNavHost
import xendroid.compose.ui.theme.UiScale
import xendroid.compose.ui.theme.UiScaleStore
import xendroid.compose.ui.theme.xendroidTheme
import xendroid.compose.settings.AppLanguageStore
import xendroid.compose.settings.ConfigStore
import xendroid.compose.settings.seedTouchOverlayDefault
import xendroid.compose.updater.UpdateResult
import xendroid.compose.updater.UpdateSheet
import xendroid.compose.updater.checkForUpdates
import xendroid.compose.updater.shouldCheckForUpdates
import xendroid.compose.updater.saveLastCheck
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.concurrent.thread


class MainActivity : ComponentActivity() {

    companion object {
        // One rotation per main-process lifetime = the app-session boundary.
        private val sessionRotated = AtomicBoolean(false)
    }

    private var updateResult by mutableStateOf<UpdateResult?>(null)
    private var backupSyncJob: Job? = null

    // 15l: the UI scale chosen in Settings, followed as it changes (the listener is held here
    // because SharedPreferences keeps only a weak reference to it).
    private var uiScale by mutableStateOf(UiScale.DEFAULT)
    // Redesign: touch (B) or controller (C) mode, kept in the same preferences file as the scale.
    private var inputPref by mutableStateOf(InputModePref.AUTO)
    private val uiScaleListener = SharedPreferences.OnSharedPreferenceChangeListener { prefs, _ ->
        uiScale = UiScaleStore.read(prefs)
        inputPref = InputModeStore.read(prefs)
    }

    // The app's toasts, held here so the exit guard can show one too.
    private val toast = XdToastState()
    private val exitGuard = ExitGuard(toast, { getString(R.string.xd_exit_again, it) }) { onBackPressedDispatcher.onBackPressed() }

    /** 15l: before Android 13 the language chosen in the app is applied here. */
    override fun attachBaseContext(newBase: Context) {
        super.attachBaseContext(newBase)
        AppLanguageStore.overrideFor(newBase)?.let { applyOverrideConfiguration(it) }
    }

    override fun onDestroy() {
        UiScaleStore.prefs(this).unregisterOnSharedPreferenceChangeListener(uiScaleListener)
        super.onDestroy()
    }
    override fun onStart() {
        super.onStart()
        if (backupSyncJob?.isActive != true) backupSyncJob = lifecycleScope.launch {
            BackupSync.publishAutomatic(applicationContext)
        }
        // A game process that died while this one lived (killed, crashed) left its run open:
        // close it whenever the library comes back, not only when the app process starts.
        lifecycleScope.launch(kotlinx.coroutines.Dispatchers.IO) {
            runCatching {
                xendroid.compose.sessions.SessionRuns.store()
                    .reconcile(xendroid.compose.sessions.SessionRuns.fates(applicationContext))
            }.onFailure { Log.w("MainActivity", "Closing the runs of ended game processes failed", it) }
        }
    }

    /** Compose's standard click/back keys for controller-driven frontend screens.
     * The emulator Activity keeps its separate guest/menu input routing. */
    // This is the public Activity/Window callback. AndroidX annotates its internal
    // implementation class RestrictTo, but overriding the platform callback is valid.
    @SuppressLint("RestrictedApi")
    override fun dispatchKeyEvent(event: KeyEvent): Boolean {
        // #76: who has played, before any screen takes the event (the controller test does).
        if (event.action == KeyEvent.ACTION_DOWN && event.repeatCount == 0 &&
            event.keyCode != KeyEvent.KEYCODE_BACK) noteControllerUse(event)
        // U05: the controller test sees controllers raw and alone.
        if (xendroid.compose.gamepad.GamepadCapture.listener?.invoke(event) == true) return true
        // U04: A clicks and B goes back, or the other way round when the user swapped them.
        val code = xendroid.compose.gamepad.MenuButtons.frontendKey(event.keyCode,
            xendroid.compose.gamepad.MenuButtonPrefs.swapConfirm(this))
        val translated = if (code == null) event else KeyEvent(
            event.downTime, event.eventTime, event.action, code, event.repeatCount,
            event.metaState, event.deviceId, event.scanCode, event.flags, event.source,
        )
        // A controller's Back: its B (A when swapped) turned into one, or a pad's own Back key.
        val padBack = translated.keyCode == KeyEvent.KEYCODE_BACK &&
            (code != null || event.isFromSource(android.view.InputDevice.SOURCE_GAMEPAD))
        if (!padBack) return super.dispatchKeyEvent(translated)
        exitGuard.arm(if (event.keyCode == KeyEvent.KEYCODE_BUTTON_A) "A" else "B")
        try {
            return super.dispatchKeyEvent(translated)
        } finally {
            exitGuard.disarm()
        }
    }

    override fun dispatchGenericMotionEvent(event: android.view.MotionEvent): Boolean {
        if (event.source and android.view.InputDevice.SOURCE_JOYSTICK == android.view.InputDevice.SOURCE_JOYSTICK) {
            fun axis(a: Int) = event.getAxisValue(a)
            val plays = xendroid.compose.gamepad.PadPlay.plays(
                axis(android.view.MotionEvent.AXIS_X), axis(android.view.MotionEvent.AXIS_Y),
                axis(android.view.MotionEvent.AXIS_Z), axis(android.view.MotionEvent.AXIS_RZ),
                maxOf(axis(android.view.MotionEvent.AXIS_LTRIGGER), axis(android.view.MotionEvent.AXIS_BRAKE)),
                maxOf(axis(android.view.MotionEvent.AXIS_RTRIGGER), axis(android.view.MotionEvent.AXIS_GAS)),
                axis(android.view.MotionEvent.AXIS_HAT_X), axis(android.view.MotionEvent.AXIS_HAT_Y))
            if (plays) noteControllerUse(event)
        }
        if (xendroid.compose.gamepad.GamepadCapture.listener?.invoke(event) == true) return true
        return super.dispatchGenericMotionEvent(event)
    }

    /** #76: a controller that plays, in [xendroid.compose.gamepad.ControllerUse]'s order. */
    private fun noteControllerUse(event: android.view.InputEvent) {
        val device = event.device ?: return
        if (xendroid.compose.ui.design.Gamepads.isController(device)) {
            xendroid.compose.gamepad.ControllerUse.note(device.descriptor)
        }
    }

    override fun onNewIntent(intent: Intent) {
        super.onNewIntent(intent)
        relaunchIfAsked(intent)
    }

    /** 15e: "Try again" from the game process: the same game, discs and loading picture, in a new
     *  :emu once the old one is gone. Only this app's own game process asks. */
    private fun relaunchIfAsked(request: Intent?) {
        if (request?.action != EmulatorHostActivity.ACTION_RELAUNCH_GAME || referrer?.host != packageName) return
        val game = request.getStringExtra(EXTRA_GAME_URI)?.takeIf { it.isNotBlank() } ?: return
        val launch = Intent(ACTION_LAUNCH_GAME).apply {
            setPackage(packageName)
            EmulatorHostActivity.RELAUNCH_EXTRAS.forEach { key ->
                request.getStringExtra(key)?.let { putExtra(key, it) }
                request.getStringArrayExtra(key)?.let { putExtra(key, it) }
            }
            if (game.startsWith("content://")) {
                clipData = android.content.ClipData.newRawUri("game", android.net.Uri.parse(game))
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
        }
        lifecycleScope.launch {
            xendroid.compose.core.GameRelaunch.awaitEmuGone(applicationContext)
            runCatching { startActivity(launch) }.onFailure { Log.w("MainActivity", "Starting the game again failed", it) }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // First, under every Back the screens add; without an owner, so it is never re-added
        // above them when the activity starts again.
        onBackPressedDispatcher.addCallback(exitGuard)

        // Same intent shape as the in-app launch, which routes through the
        // manifest filter into the host's own task. Finishing here instead
        // collapses that task and the emulator is paused before it draws.
        // Resolved for the :emu process: a real path, or the URI itself (with the read
        // grant) - never this process's /proc/self/fd/<n>, which :emu cannot use.
        // 15e: a "Try again" from the game process carries the game too, but waits for the old
        // :emu to end (relaunchIfAsked below), so it is not a hand-off.
        val relaunch = intent?.action == EmulatorHostActivity.ACTION_RELAUNCH_GAME
        val frontendGame = if (relaunch) null else FrontendLaunch.resolveForHandOff(this, intent)
        if (frontendGame != null) {
            startActivity(
                Intent(ACTION_LAUNCH_GAME).apply {
                    setPackage(packageName)
                    putExtra(EXTRA_GAME_URI, frontendGame)
                    if (frontendGame.startsWith("content://")) {
                        clipData = android.content.ClipData.newRawUri("game", android.net.Uri.parse(frontendGame))
                        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
                    }
                }
            )
        }

        if (!sessionRotated.getAndSet(true)) {
            val appContext = applicationContext

            thread(name = "SessionLogs") {
                runCatching { SessionLogs.startAppSession(appContext) }
                // Close game runs whose :emu process died without finishing them.
                runCatching {
                    xendroid.compose.sessions.SessionRuns.store()
                        .reconcile(xendroid.compose.sessions.SessionRuns.fates(appContext))
                }

                // Pre-warm so settings doesn't pay the delay-load System.loadLibrary.
                runCatching { EmulatorRuntime.ensureLoaded() }
                // Needs the native config, so it follows ensureLoaded on this same thread.
                runCatching { seedTouchOverlayDefault(appContext, ConfigStore(appContext)) }
            }
        }

        if (savedInstanceState == null) relaunchIfAsked(intent)

        AppLanguageStore.moveToSystem(this)
        uiScale = UiScaleStore.read(this)
        inputPref = InputModeStore.read(this)
        UiScaleStore.prefs(this).registerOnSharedPreferenceChangeListener(uiScaleListener)

        val container = AppContainer(applicationContext)

        enableEdgeToEdge()

       setContent {
            // Controller mode follows a controller connecting and disconnecting (Automatic).
            val mode = inputPref.resolve(rememberControllerConnected())
            CompositionLocalProvider(LocalInputMode provides mode, LocalXdToast provides toast) {
            xendroidTheme(scale = uiScale, mode = mode) {

                Box(Modifier.fillMaxSize()) {
                    AppNavHost(container)
                    XdToastHost(toast, Modifier.align(Alignment.BottomCenter))
                }

                LaunchedEffect(Unit) {
                    if (frontendGame != null) return@LaunchedEffect
                    // A debug build's -debug versionName never matches a release tag, so the
                    // check always reports an update - to an APK whose .debug-suffixed package
                    // could not replace this install anyway.
                    if (BuildConfig.DEBUG) return@LaunchedEffect
                    if (!shouldCheckForUpdates(applicationContext)) {
                        Log.d("Updater", "Skipping update check (less than 5 minutes)")
                        return@LaunchedEffect
                    }

                    try {
                        val result = checkForUpdates(applicationContext)

                        // An automatic check only interrupts the user for a real update;
                        // "you are up to date" belongs to an explicit check.
                        updateResult = result.takeIf { it is UpdateResult.Available }

                        // Export check result only if github replied with a valid response
                        saveLastCheck(applicationContext)
                    } catch (e: Exception) {
                        Log.e(
                            "Updater",
                            "Failed to check updates",
                            e
                        )
                    }
                }

                // Lote 6: the offered update as a sheet with its steps (download, check, install).
                (updateResult as? UpdateResult.Available)?.let { result ->
                    UpdateSheet(release = result.release, onDismiss = { updateResult = null })
                }
            }
            }
        }
    }
}
