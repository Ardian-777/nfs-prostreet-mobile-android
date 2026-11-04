package xendroid.compose.ui.keymap

import android.view.InputDevice
import android.view.MotionEvent
import android.view.Window
import android.view.KeyEvent as AndroidKeyEvent
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.focusable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.compositeOver
import androidx.compose.ui.input.key.KeyEventType
import androidx.compose.ui.input.key.onKeyEvent
import androidx.compose.ui.input.key.type
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalView
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.DialogWindowProvider
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import xendroid.compose.R
import xendroid.compose.data.GameButton
import xendroid.compose.data.GameButtons
import xendroid.compose.gamepad.AnalogTriggers
import xendroid.compose.ui.design.Gamepads
import xendroid.compose.ui.design.LocalSwapConfirm
import xendroid.compose.ui.design.LocalXdToast
import xendroid.compose.ui.design.NoteTone
import xendroid.compose.ui.design.PadButton
import xendroid.compose.ui.design.Xd
import xendroid.compose.ui.design.XdArea
import xendroid.compose.ui.design.XdButton
import xendroid.compose.ui.design.XdButtonKind
import xendroid.compose.ui.design.XdButtonSize
import xendroid.compose.ui.design.XdCard
import xendroid.compose.ui.design.XdHint
import xendroid.compose.ui.design.XdIcons
import xendroid.compose.ui.design.XdNote
import xendroid.compose.ui.design.XdSheet
import xendroid.compose.ui.design.XdSingleScreen
import xendroid.compose.ui.design.XdText
import xendroid.compose.ui.design.focusRing

private val Warn = Color(0xFFE9A23B)

/**
 * 15o: the key of each of the 16 buttons, on the drawn controller and in a list beside it
 * (under it in portrait). Changed keys, keys on two buttons and buttons without a key have
 * colours of their own; tapping a button waits for the key, a key another button had trades
 * places (and says so). A controller works it too: A picks the key, Y clears, X swaps A/B and X/Y.
 */
@Composable
fun KeymapScreen(vm: KeymapViewModel, onBack: () -> Unit) {
    val state by vm.state.collectAsStateWithLifecycle()
    val swap by vm.swap.collectAsStateWithLifecycle()
    val toast = LocalXdToast.current
    val context = LocalContext.current
    var capturing by remember { mutableStateOf<KeymapRow?>(null) }
    var focused by remember { mutableStateOf<Int?>(null) }
    val unbound = stringResource(R.string.km_unbound)
    fun bindingText(code: Int) = if (code == 0) unbound else keyLabel(code)
    fun rowOf(index: Int) = state.rows.firstOrNull { it.button.index == index }
    // A/B and X/Y count as swapped while A sends what B usually does.
    val faceSwapped = rowOf(4)?.boundKey == AndroidKeyEvent.KEYCODE_BUTTON_B && rowOf(5)?.boundKey == AndroidKeyEvent.KEYCODE_BUTTON_A
    fun swapFaces() {
        vm.onSwapFaceButtons()
        toast.show(context.getString(if (faceSwapped) R.string.xd_km_swapped_off else R.string.xd_km_swapped_on))
    }
    fun clear(index: Int) {
        vm.onClear(index)
        toast.show(context.getString(R.string.xd_km_cleared, buttonName(context, GameButtons.ALL[index])))
    }

    // 15o: say when a key was taken from another button, which got this one's old key.
    swap?.let { traded ->
        val message = stringResource(R.string.km_swapped, buttonLabel(GameButtons.ALL[traded.from]), buttonLabel(GameButtons.ALL[traded.index]))
        LaunchedEffect(traded) {
            toast.show(message)
            vm.onSwapShown()
        }
    }

    val swapConfirm = LocalSwapConfirm.current
    XdSingleScreen(
        title = stringResource(R.string.lib_menu_keymap),
        area = XdArea.CONTROLS,
        subtitle = stringResource(R.string.xd_km_sub),
        onBack = onBack,
        headIcon = XdIcons.keyboard,
        actions = {
            XdButton(stringResource(R.string.km_reset), {
                vm.onResetDefaults()
                toast.show(context.getString(R.string.xd_km_restored))
            }, kind = XdButtonKind.GHOST, size = XdButtonSize.SM, icon = XdIcons.reset)
        },
        hints = listOf(
            XdHint(if (swapConfirm) "B" else "A", stringResource(R.string.xd_km_hint_pick)),
            XdHint("Y", stringResource(R.string.km_clear)),
            XdHint("X", stringResource(R.string.km_swap_face)),
            XdHint(if (swapConfirm) "A" else "B", stringResource(R.string.xd_back), onBack),
        ),
        onPad = { b ->
            when (b) {
                PadButton.Y -> { focused?.let { if ((rowOf(it)?.boundKey ?: 0) != 0) clear(it) }; true }
                PadButton.X -> { swapFaces(); true }
                else -> false
            }
        },
    ) {
        BoxWithConstraints(Modifier.fillMaxWidth()) {
            val wide = maxWidth >= 720.dp
            val drawing: @Composable ColumnScope.() -> Unit = {
                XdCard(Modifier.fillMaxWidth()) {
                    ControllerDrawing(
                        label = { buttonLabel(GameButtons.ALL[it]) },
                        binding = { index -> bindingText(rowOf(index)?.boundKey ?: 0) },
                        state = { index -> stateOf(state, index) },
                        onPick = { index -> rowOf(index)?.let { capturing = it } },
                        caption = { index ->
                            val key = rowOf(index)?.boundKey ?: 0
                            if (stateOf(state, index) == ButtonState.USUAL) null else if (key == 0) "—" else keyLabel(key).removePrefix("BUTTON_")
                        },
                        onFocus = { focused = it },
                        modifier = Modifier.widthIn(max = 560.dp).padding(top = 4.dp, bottom = 14.dp),
                    )
                    Legend()
                }
                Text(stringResource(R.string.xd_km_hint), style = XdText.note, color = Xd.colors.fg3)
                if (state.shared.isNotEmpty()) {
                    val names = GameButtons.ALL.filter { it.index in state.shared }.map { buttonLabel(it) }
                    XdNote(stringResource(R.string.km_shared_warning, names.joinToString(", ")), tone = NoteTone.WARN, icon = XdIcons.warn)
                }
                Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                    XdButton(stringResource(R.string.km_swap_face), ::swapFaces, size = XdButtonSize.SM, icon = XdIcons.refresh,
                        kind = if (faceSwapped) XdButtonKind.PRIMARY else XdButtonKind.SECONDARY)
                    Text(stringResource(R.string.xd_km_swap_note), style = XdText.small, color = Xd.colors.fg3, modifier = Modifier.weight(1f))
                }
            }
            val list: @Composable ColumnScope.() -> Unit = {
                state.rows.forEach { row ->
                    KeyRow(row, stateOf(state, row.button.index), bindingText(row.boundKey),
                        onPick = { capturing = row }, onClear = { clear(row.button.index) }, onFocus = { focused = row.button.index })
                }
            }
            if (wide) Row(horizontalArrangement = Arrangement.spacedBy(16.dp)) {
                Column(Modifier.weight(1.1f), verticalArrangement = Arrangement.spacedBy(10.dp), content = drawing)
                Column(Modifier.weight(1f), verticalArrangement = Arrangement.spacedBy(6.dp), content = list)
            } else Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                drawing()
                Column(Modifier.padding(top = 4.dp), verticalArrangement = Arrangement.spacedBy(6.dp), content = list)
            }
        }
    }

    capturing?.let { row ->
        val label = buttonLabel(row.button)
        KeyCaptureSheet(
            label = label,
            current = bindingText(row.boundKey),
            onKey = { code ->
                vm.onKeyCaptured(row.button.index, code)
                toast.show(if (code == 0) context.getString(R.string.xd_km_cleared, label) else "$label: ${keyLabel(code)}")
                capturing = null
            },
            onDismiss = { capturing = null },
        )
    }
}

private fun stateOf(state: KeymapUiState, index: Int): ButtonState {
    val key = state.rows.firstOrNull { it.button.index == index }?.boundKey ?: 0
    return when {
        state.rows.isEmpty() -> ButtonState.USUAL
        key == 0 -> ButtonState.UNBOUND
        index in state.shared -> ButtonState.SHARED
        index in state.changed -> ButtonState.CHANGED
        else -> ButtonState.USUAL
    }
}

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun Legend() {
    val c = Xd.colors
    FlowRow(horizontalArrangement = Arrangement.spacedBy(16.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
        listOf(
            Triple(c.acc, true, stringResource(R.string.xd_km_legend_changed)),
            Triple(Warn, true, stringResource(R.string.xd_km_legend_shared)),
            Triple(Color.White.copy(alpha = 0.35f), false, stringResource(R.string.xd_km_legend_unbound)),
        ).forEach { (color, solid, text) ->
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                Box(Modifier.size(10.dp).clip(CircleShape).then(if (solid) Modifier.background(color) else Modifier.border(1.5.dp, color, CircleShape)))
                Text(text, style = XdText.small, color = c.fg2)
            }
        }
    }
}

@Composable
private fun KeyRow(row: KeymapRow, look: ButtonState, key: String, onPick: () -> Unit, onClear: () -> Unit, onFocus: () -> Unit) {
    val c = Xd.colors
    val shape = RoundedCornerShape(12.dp)
    val tint = when (look) {
        ButtonState.CHANGED -> c.acc
        ButtonState.SHARED -> Warn
        else -> null
    }
    Row(
        Modifier.fillMaxWidth().clip(shape)
            .background(tint?.copy(alpha = 0.1f)?.compositeOver(c.solid(c.s1)) ?: c.s1)
            .then(if (tint != null) Modifier.border(1.dp, tint.copy(alpha = 0.35f), shape) else Modifier)
            .padding(end = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        Column(
            Modifier.weight(1f).focusRing(shape).clip(shape).clickable(role = Role.Button, onClick = onPick)
                .onFocusChanged { if (it.isFocused) onFocus() }.padding(horizontal = 12.dp, vertical = 9.dp),
        ) {
            Text(buttonLabel(row.button), style = XdText.label, color = c.fg, maxLines = 1, overflow = TextOverflow.Ellipsis)
            Text(if (look == ButtonState.SHARED) "$key · ${stringResource(R.string.xd_km_also)}" else key,
                style = XdText.mono.copy(fontSize = 11.5.sp), color = when (look) {
                    ButtonState.UNBOUND -> c.fg3
                    ButtonState.SHARED -> Warn
                    ButtonState.CHANGED -> c.acc
                    else -> c.fg2
                }, maxLines = 1, overflow = TextOverflow.Ellipsis)
        }
        XdButton(stringResource(R.string.km_clear), onClear, kind = XdButtonKind.GHOST, size = XdButtonSize.SM, enabled = row.boundKey != 0)
    }
}

/** Waits for one key: a controller button or a keyboard key; the phone's own Back closes it. */
@Composable
private fun KeyCaptureSheet(label: String, current: String, onKey: (Int) -> Unit, onDismiss: () -> Unit) {
    val c = Xd.colors
    val focus = remember { FocusRequester() }
    LaunchedEffect(Unit) { runCatching { focus.requestFocus() } }
    // One capture per sheet: a pad may send a trigger both as its L2/R2 key and as an axis.
    val taken = remember { booleanArrayOf(false) }
    val currentOnKey by rememberUpdatedState(onKey)
    fun take(key: Int) {
        if (taken[0]) return
        taken[0] = true
        currentOnKey(key)
    }
    XdSheet(onDismiss = onDismiss, title = stringResource(R.string.km_press, label), actions = {
        XdButton(stringResource(R.string.xd_km_leave_unbound), { onKey(0) }, kind = XdButtonKind.GHOST)
        XdButton(stringResource(R.string.common_cancel), onDismiss)
    }) {
        // #97: a pad in analog mode sends its triggers only as axes. Past half way they stand
        // for its L2 and R2 keys, whose mapping the game's trigger axes follow. The sheet is a
        // dialog: the controller's motion goes to its own window, not to the activity's.
        val view = LocalView.current
        DisposableEffect(view) {
            val window = (view.parent as? DialogWindowProvider)?.window
            val original = window?.callback
            if (window != null && original != null) {
                window.callback = object : Window.Callback by original {
                    override fun dispatchGenericMotionEvent(event: MotionEvent): Boolean {
                        val key = triggerKeyOf(event) ?: return original.dispatchGenericMotionEvent(event)
                        take(key)
                        return true
                    }
                }
            }
            onDispose { if (window != null && original != null) window.callback = original }
        }
        Column(
            Modifier.fillMaxWidth().focusRequester(focus).focusable()
                .onKeyEvent { ev ->
                    if (ev.type != KeyEventType.KeyDown) return@onKeyEvent false
                    val native = ev.nativeKeyEvent
                    // Back from the phone itself (not a controller's Select) leaves without binding.
                    if (native.keyCode == AndroidKeyEvent.KEYCODE_BACK && !Gamepads.isController(InputDevice.getDevice(native.deviceId))) {
                        onDismiss()
                    } else if (native.repeatCount == 0) take(native.keyCode)
                    true
                }
                .padding(vertical = 10.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(8.dp),
        ) {
            Box(Modifier.size(64.dp).clip(CircleShape).background(c.acc.copy(alpha = 0.16f)), contentAlignment = Alignment.Center) {
                Icon(XdIcons.gamepad, null, Modifier.size(30.dp), tint = c.acc)
            }
            Text(stringResource(R.string.km_waiting), style = XdText.label, color = c.fg)
            Text(stringResource(R.string.km_now, current), style = XdText.small, color = c.fg3)
        }
    }
}

/** L2 or R2 for a trigger axis pushed past half way, as the game's triggers read them (#97). */
private fun triggerKeyOf(event: MotionEvent): Int? {
    if (event.source and InputDevice.SOURCE_JOYSTICK != InputDevice.SOURCE_JOYSTICK) return null
    val half = AnalogTriggers.BUTTON_THRESHOLD
    return when {
        maxOf(event.getAxisValue(MotionEvent.AXIS_LTRIGGER), event.getAxisValue(MotionEvent.AXIS_BRAKE)) > half ->
            AndroidKeyEvent.KEYCODE_BUTTON_L2
        maxOf(event.getAxisValue(MotionEvent.AXIS_RTRIGGER), event.getAxisValue(MotionEvent.AXIS_GAS)) > half ->
            AndroidKeyEvent.KEYCODE_BUTTON_R2
        else -> null
    }
}

/** The usual names of the keys a controller sends (and a few keyboard ones), as Android names them. */
private val KEY_NAMES = mapOf(
    AndroidKeyEvent.KEYCODE_DPAD_UP to "DPAD_UP", AndroidKeyEvent.KEYCODE_DPAD_DOWN to "DPAD_DOWN",
    AndroidKeyEvent.KEYCODE_DPAD_LEFT to "DPAD_LEFT", AndroidKeyEvent.KEYCODE_DPAD_RIGHT to "DPAD_RIGHT",
    AndroidKeyEvent.KEYCODE_DPAD_CENTER to "DPAD_CENTER", AndroidKeyEvent.KEYCODE_BUTTON_A to "BUTTON_A",
    AndroidKeyEvent.KEYCODE_BUTTON_B to "BUTTON_B", AndroidKeyEvent.KEYCODE_BUTTON_X to "BUTTON_X",
    AndroidKeyEvent.KEYCODE_BUTTON_Y to "BUTTON_Y", AndroidKeyEvent.KEYCODE_BUTTON_L1 to "BUTTON_L1",
    AndroidKeyEvent.KEYCODE_BUTTON_R1 to "BUTTON_R1", AndroidKeyEvent.KEYCODE_BUTTON_L2 to "BUTTON_L2",
    AndroidKeyEvent.KEYCODE_BUTTON_R2 to "BUTTON_R2", AndroidKeyEvent.KEYCODE_BUTTON_THUMBL to "BUTTON_THUMBL",
    AndroidKeyEvent.KEYCODE_BUTTON_THUMBR to "BUTTON_THUMBR", AndroidKeyEvent.KEYCODE_BUTTON_START to "BUTTON_START",
    AndroidKeyEvent.KEYCODE_BUTTON_SELECT to "BUTTON_SELECT", AndroidKeyEvent.KEYCODE_BUTTON_MODE to "BUTTON_MODE",
    AndroidKeyEvent.KEYCODE_BACK to "BACK", AndroidKeyEvent.KEYCODE_ENTER to "ENTER", AndroidKeyEvent.KEYCODE_SPACE to "SPACE",
    AndroidKeyEvent.KEYCODE_ESCAPE to "ESCAPE",
)

/** Human-readable name for a bound Android keycode. */
internal fun keyLabel(code: Int): String = KEY_NAMES[code] ?: AndroidKeyEvent.keyCodeToString(code).removePrefix("KEYCODE_")

/** U02: the button's shown name; letters, Back and Start stay as printed on the controller. */
@Composable
internal fun buttonLabel(button: GameButton): String = when (button.index) {
    0 -> stringResource(R.string.km_btn_dpad_left)
    1 -> stringResource(R.string.km_btn_dpad_up)
    2 -> stringResource(R.string.km_btn_dpad_right)
    3 -> stringResource(R.string.km_btn_dpad_down)
    10 -> stringResource(R.string.km_btn_lb)
    11 -> stringResource(R.string.km_btn_rb)
    12 -> stringResource(R.string.km_btn_l3)
    13 -> stringResource(R.string.km_btn_r3)
    14 -> stringResource(R.string.km_btn_lt)
    15 -> stringResource(R.string.km_btn_rt)
    else -> button.label
}

private fun buttonName(context: android.content.Context, button: GameButton): String = when (button.index) {
    0 -> context.getString(R.string.km_btn_dpad_left)
    1 -> context.getString(R.string.km_btn_dpad_up)
    2 -> context.getString(R.string.km_btn_dpad_right)
    3 -> context.getString(R.string.km_btn_dpad_down)
    10 -> context.getString(R.string.km_btn_lb)
    11 -> context.getString(R.string.km_btn_rb)
    12 -> context.getString(R.string.km_btn_l3)
    13 -> context.getString(R.string.km_btn_r3)
    14 -> context.getString(R.string.km_btn_lt)
    15 -> context.getString(R.string.km_btn_rt)
    else -> button.label
}
