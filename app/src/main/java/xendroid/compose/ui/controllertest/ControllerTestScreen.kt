package xendroid.compose.ui.controllertest

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.input.InputManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.view.InputDevice
import android.view.InputEvent
import android.view.KeyEvent
import android.view.MotionEvent
import androidx.activity.compose.BackHandler
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.FlowRow
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Icon
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.pluralStringResource
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import kotlin.math.abs
import kotlin.math.hypot
import xendroid.compose.R
import xendroid.compose.gamepad.ControlOptions
import xendroid.compose.gamepad.ControlOptionsStore
import xendroid.compose.gamepad.ControllerTestModel
import xendroid.compose.gamepad.ControllerUse
import xendroid.compose.gamepad.GamepadCapture
import xendroid.compose.gamepad.PadAxes
import xendroid.compose.gamepad.RumbleIntensity
import xendroid.compose.gamepad.TestedDevice
import xendroid.compose.settings.Setting
import xendroid.compose.settings.SettingsSchema
import xendroid.compose.settings.SettingsViewModel
import xendroid.compose.ui.controls.InputDevices
import xendroid.compose.ui.controls.PadRumbleState
import xendroid.compose.ui.controls.rememberPadRumble
import xendroid.compose.ui.design.BadgeTone
import xendroid.compose.ui.design.LocalSwapConfirm
import xendroid.compose.ui.design.NoteTone
import xendroid.compose.ui.design.Xd
import xendroid.compose.ui.design.XdArea
import xendroid.compose.ui.design.XdBadge
import xendroid.compose.ui.design.XdButton
import xendroid.compose.ui.design.XdButtonKind
import xendroid.compose.ui.design.XdButtonSize
import xendroid.compose.ui.design.XdCard
import xendroid.compose.ui.design.XdHint
import xendroid.compose.ui.design.XdIcons
import xendroid.compose.ui.design.XdNote
import xendroid.compose.ui.design.XdSingleScreen
import xendroid.compose.ui.design.XdStepper
import xendroid.compose.ui.design.XdText
import xendroid.compose.ui.keymap.ButtonState
import xendroid.compose.ui.keymap.ControllerDrawing
import xendroid.compose.ui.rumbleLabel
import xendroid.compose.ui.settings.GlobalSettingsEditing
import xendroid.compose.ui.settings.stepped
import xendroid.compose.ui.settings.valueLabel

/** How long B must be held to leave with a controller only. */
private const val HOLD_TO_LEAVE_MS = 1000L

/** The host's own cut on every stick axis (the core's dead zone comes on top). */
private const val APP_DEADZONE = 0.08f

/**
 * U05: every connected controller, live: the drawn controller lights what is pressed and the
 * checklist marks what already worked, the sticks show the dead zone and what the game gets
 * (the core's dead zone can be changed right there), triggers have bars, a gyroscope its
 * reading, and vibration plays only when asked. Inputs here go to no game and no other screen.
 * [model] and [listen] let the screenshots feed the test themselves.
 */
@Composable
fun ControllerTestScreen(
    onBack: () -> Unit,
    global: SettingsViewModel? = null,
    model: ControllerTestModel = remember { ControllerTestModel() },
    listen: Boolean = true,
) {
    val context = LocalContext.current
    var devices by remember { mutableStateOf(model.all) }
    var events by remember { mutableStateOf(model.events) }
    val gyro = remember { mutableStateMapOf<Int, FloatArray>() }
    val rumble = rememberPadRumble()
    val optionsStore = remember { ControlOptionsStore(context.applicationContext) }
    val options by optionsStore.options.collectAsStateWithLifecycle(initialValue = ControlOptions())
    fun publish() { devices = model.all; events = model.events }

    if (listen) DisposableEffect(Unit) {
        val input = context.getSystemService(Context.INPUT_SERVICE) as InputManager
        val handler = Handler(Looper.getMainLooper())
        val gyroListeners = HashMap<Int, Pair<android.hardware.SensorManager, SensorEventListener>>()
        fun add(id: Int) {
            val device = InputDevice.getDevice(id)?.takeIf(xendroid.compose.ui.design.Gamepads::isController) ?: return
            model.connected(InputDevices.describe(context, device))
            if (Build.VERSION.SDK_INT >= 31 && id !in gyroListeners) {
                val sensors = device.sensorManager
                sensors.getDefaultSensor(Sensor.TYPE_GYROSCOPE)?.let { sensor ->
                    val listener = object : SensorEventListener {
                        override fun onSensorChanged(event: SensorEvent) { gyro[id] = event.values.copyOf(3) }
                        override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) = Unit
                    }
                    if (sensors.registerListener(listener, sensor, android.hardware.SensorManager.SENSOR_DELAY_UI)) {
                        gyroListeners[id] = sensors to listener
                    }
                }
            }
            publish()
        }
        val listener = object : InputManager.InputDeviceListener {
            override fun onInputDeviceAdded(deviceId: Int) = add(deviceId)
            override fun onInputDeviceChanged(deviceId: Int) = add(deviceId)
            override fun onInputDeviceRemoved(deviceId: Int) {
                gyroListeners.remove(deviceId)?.let { (sensors, l) -> sensors.unregisterListener(l) }
                gyro.remove(deviceId)
                model.disconnected(deviceId)
                publish()
            }
        }
        input.registerInputDeviceListener(listener, handler)
        input.inputDeviceIds.forEach(::add)
        GamepadCapture.listener = { event -> handle(event, model, onBack).also { if (it) publish() } }
        onDispose {
            GamepadCapture.listener = null
            input.unregisterInputDeviceListener(listener)
            gyroListeners.values.forEach { (sensors, l) -> sensors.unregisterListener(l) }
        }
    }
    BackHandler(onBack = onBack)

    // The core's dead zones are changed here too: a durable write when leaving, read again on return.
    val owner = androidx.lifecycle.compose.LocalLifecycleOwner.current
    if (global != null) DisposableEffect(owner, global) {
        val observer = androidx.lifecycle.LifecycleEventObserver { _, e ->
            when (e) {
                androidx.lifecycle.Lifecycle.Event.ON_PAUSE -> global.flush()
                androidx.lifecycle.Lifecycle.Event.ON_RESUME -> global.onResume()
                else -> {}
            }
        }
        owner.lifecycle.addObserver(observer)
        onDispose { owner.lifecycle.removeObserver(observer); global.flush() }
    }
    val values = global?.values?.collectAsStateWithLifecycle()?.value
    val editing = remember(global, values) { if (global != null && values != null) GlobalSettingsEditing(global, values) else null }
    val connected = devices.count { it.connected }
    val swap = LocalSwapConfirm.current
    XdSingleScreen(
        title = stringResource(R.string.lib_menu_test_controllers),
        area = XdArea.CONTROLS,
        subtitle = if (connected > 0) pluralStringResource(R.plurals.xd_ct_count, connected, connected) else null,
        onBack = onBack,
        headIcon = XdIcons.gamepad,
        hints = listOf(XdHint(if (swap) "A" else "B", stringResource(R.string.xd_ct_hold_b), onBack)),
    ) {
        Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
            XdNote(stringResource(R.string.ct_intro), tone = NoteTone.INFO, icon = XdIcons.info)
            if (devices.isEmpty()) XdCard(Modifier.fillMaxWidth()) { Text(stringResource(R.string.ct_none), style = XdText.body, color = Xd.colors.fg) }
            val slots = ControllerTestModel.playerSlots(devices, ControllerUse.descriptors)
            devices.forEach { device ->
                DeviceCard(device, slots[device.id], gyro[device.id], rumble, options.rumble, editing)
            }
            if (events.isNotEmpty()) XdCard(Modifier.fillMaxWidth(), title = stringResource(R.string.ct_connections), icon = XdIcons.timeline) {
                events.take(10).forEach { Text(eventText(it), style = XdText.bodySm, color = Xd.colors.fg2) }
            }
        }
    }
}

/** True when the event belonged to a controller (it is then the test's alone). */
private fun handle(event: InputEvent, model: ControllerTestModel, onBack: () -> Unit): Boolean {
    val controller = event.source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
        event.source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK
    if (!controller) return false   // the phone's own Back and touch keep working
    when (event) {
        is KeyEvent -> {
            if (event.action == KeyEvent.ACTION_DOWN && event.keyCode == KeyEvent.KEYCODE_BUTTON_B &&
                event.eventTime - event.downTime >= HOLD_TO_LEAVE_MS) {
                onBack()
                return true
            }
            if (event.repeatCount == 0) model.key(event.deviceId, event.keyCode, event.action == KeyEvent.ACTION_DOWN)
        }
        is MotionEvent -> if (event.action == MotionEvent.ACTION_MOVE) model.motion(event.deviceId, PadAxes(
            event.getAxisValue(MotionEvent.AXIS_X), event.getAxisValue(MotionEvent.AXIS_Y),
            event.getAxisValue(MotionEvent.AXIS_Z), event.getAxisValue(MotionEvent.AXIS_RZ),
            maxOf(event.getAxisValue(MotionEvent.AXIS_LTRIGGER), event.getAxisValue(MotionEvent.AXIS_BRAKE)),
            maxOf(event.getAxisValue(MotionEvent.AXIS_RTRIGGER), event.getAxisValue(MotionEvent.AXIS_GAS)),
            event.getAxisValue(MotionEvent.AXIS_HAT_X), event.getAxisValue(MotionEvent.AXIS_HAT_Y)))
    }
    return true
}

/** The checklist name of each drawn button (the drawing's indices are the game's buttons). */
private val CHECK_OF = mapOf(
    0 to ControllerTestModel.DPAD_LEFT, 1 to ControllerTestModel.DPAD_UP, 2 to ControllerTestModel.DPAD_RIGHT, 3 to ControllerTestModel.DPAD_DOWN,
    4 to "A", 5 to "B", 6 to "X", 7 to "Y", 8 to "Back", 9 to "Start", 10 to "LB", 11 to "RB", 12 to "L3", 13 to "R3", 14 to "LT", 15 to "RT",
)

/** The core's dead zone of one stick (0..0.3), with the host's own cut as its floor. */
private fun deadZone(editing: GlobalSettingsEditing?, s: Setting?): Float =
    maxOf(APP_DEADZONE, s?.let { editing?.raw(it)?.toFloatOrNull() } ?: 0f)

private fun gets(v: Float, dz: Float) = if (!v.isFinite() || abs(v) < dz) 0f else v.coerceIn(-1f, 1f)

private fun n2(v: Float): String = "%.2f".format(v)

@OptIn(ExperimentalLayoutApi::class)
@Composable
private fun DeviceCard(device: TestedDevice, slot: Int?, gyro: FloatArray?, rumble: PadRumbleState, default: RumbleIntensity,
                       editing: GlobalSettingsEditing?) {
    val c = Xd.colors
    val left = SettingsSchema.byKey["HID|left_stick_deadzone_percentage"]
    val right = SettingsSchema.byKey["HID|right_stick_deadzone_percentage"]
    val dzLeft = deadZone(editing, left)
    val dzRight = deadZone(editing, right)
    XdCard(Modifier.fillMaxWidth()) {
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            Column(Modifier.weight(1f)) {
                Text(device.name + if (device.connected) "" else " · " + stringResource(R.string.ct_disconnected), style = XdText.label, color = c.fg,
                    maxLines = 1, overflow = TextOverflow.Ellipsis)
                Text("%04X:%04X · %s".format(device.vendor, device.product, device.sources.joinToString(", ")), style = XdText.mono.copy(fontSize = 11.sp), color = c.fg3)
            }
            if (device.connected) XdBadge(slot?.let { stringResource(R.string.ct_plays_as, it + 1) }
                ?: stringResource(if (device.descriptor in ControllerUse.descriptors) R.string.ct_no_slot else R.string.ct_plays_on_press),
                tone = if (slot != null) BadgeTone.ACCENT else BadgeTone.WARN)
        }
        BoxWithConstraints {
            val wide = maxWidth >= 640.dp
            val drawing: @Composable () -> Unit = {
                Column(verticalArrangement = Arrangement.spacedBy(10.dp)) {
                    ControllerDrawing(
                        label = { CHECK_OF.getValue(it) },
                        binding = { CHECK_OF.getValue(it) },
                        state = { index ->
                            val name = CHECK_OF.getValue(index)
                            when (name) {
                                in device.pressed -> ButtonState.PRESSED
                                in device.seen -> ButtonState.SEEN
                                else -> ButtonState.USUAL
                            }
                        },
                        onPick = null,
                        stick = { index -> if (index == 12) Offset(device.axes.lx, device.axes.ly) else Offset(device.axes.rx, device.axes.ry) },
                        trigger = { index -> if (index == 14) device.axes.lt else device.axes.rt },
                        modifier = Modifier.widthIn(max = 480.dp),
                    )
                    FlowRow(horizontalArrangement = Arrangement.spacedBy(6.dp), verticalArrangement = Arrangement.spacedBy(6.dp)) {
                        ControllerTestModel.CHECKLIST.forEach { button -> CheckChip(button, button in device.pressed, button in device.seen) }
                    }
                }
            }
            val readings: @Composable () -> Unit = {
                Column(verticalArrangement = Arrangement.spacedBy(12.dp)) {
                    Row(horizontalArrangement = Arrangement.spacedBy(14.dp), verticalAlignment = Alignment.CenterVertically) {
                        Stick(device.axes.lx, device.axes.ly, dzLeft, stringResource(R.string.xd_ct_left))
                        Stick(device.axes.rx, device.axes.ry, dzRight, stringResource(R.string.xd_ct_right))
                        Column(Modifier.weight(1f)) {
                            Text(stringResource(R.string.xd_ct_game_gets), style = XdText.small, color = c.fg3)
                            Text("E ${n2(gets(device.axes.lx, dzLeft))}; ${n2(gets(device.axes.ly, dzLeft))}", style = XdText.mono.copy(fontSize = 12.sp), color = c.fg)
                            Text("D ${n2(gets(device.axes.rx, dzRight))}; ${n2(gets(device.axes.ry, dzRight))}", style = XdText.mono.copy(fontSize = 12.sp), color = c.fg)
                        }
                    }
                    Trigger("LT", device.axes.lt)
                    Trigger("RT", device.axes.rt)
                    if (editing != null && left != null && right != null) {
                        Column(Modifier.fillMaxWidth().clip(RoundedCornerShape(12.dp)).background(c.s2).padding(horizontal = 12.dp, vertical = 10.dp),
                            verticalArrangement = Arrangement.spacedBy(8.dp)) {
                            Text(stringResource(R.string.xd_ct_dz_core), style = XdText.labelSm, color = c.fg)
                            DeadZoneRow(stringResource(R.string.xd_ct_left), left, editing)
                            DeadZoneRow(stringResource(R.string.xd_ct_right), right, editing)
                        }
                    }
                    if (device.hasGyro) Text(gyro?.let { stringResource(R.string.ct_gyro, it[0], it[1], it[2]) } ?: stringResource(R.string.ct_gyro_wait),
                        style = XdText.mono.copy(fontSize = 12.sp), color = c.fg2)
                    if (device.canVibrate) {
                        val own = rumble.own(device.descriptor)
                        val effective = own ?: default
                        FlowRow(horizontalArrangement = Arrangement.spacedBy(8.dp), verticalArrangement = Arrangement.spacedBy(8.dp)) {
                            XdButton(stringResource(R.string.ct_game_rumble, own?.let { rumbleLabel(it) } ?: stringResource(R.string.ct_rumble_default)), {
                                // Default → Off → Low → Medium → High → default again.
                                rumble.set(device.descriptor, when (own) { null -> RumbleIntensity.OFF; RumbleIntensity.HIGH -> null; else -> own.next() })
                            }, size = XdButtonSize.SM)
                            if (device.connected) XdButton(stringResource(R.string.ct_vibrate), { InputDevices.vibrate(device.id, effective) },
                                kind = XdButtonKind.GHOST, size = XdButtonSize.SM, icon = XdIcons.vibrate, enabled = effective != RumbleIntensity.OFF)
                        }
                    } else XdNote(stringResource(R.string.ct_no_motor))
                }
            }
            if (wide) Row(horizontalArrangement = Arrangement.spacedBy(18.dp)) {
                Box(Modifier.weight(1.1f)) { drawing() }
                Box(Modifier.weight(1f)) { readings() }
            } else Column(verticalArrangement = Arrangement.spacedBy(14.dp)) { drawing(); readings() }
        }
    }
}

@Composable
private fun CheckChip(name: String, pressed: Boolean, seen: Boolean) {
    val c = Xd.colors
    val shape = RoundedCornerShape(50)
    Row(
        Modifier.height(28.dp).clip(shape)
            .background(when { pressed -> c.acc; seen -> c.acc.copy(alpha = 0.16f); else -> c.s2 })
            .then(if (seen && !pressed) Modifier.border(1.dp, c.acc.copy(alpha = 0.5f), shape) else Modifier)
            .padding(horizontal = 10.dp),
        verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(4.dp),
    ) {
        Text(name, style = XdText.chip.copy(fontSize = 12.sp), color = if (pressed) c.onAcc else if (seen) c.fg else c.fg2, maxLines = 1)
        if (seen && !pressed) Icon(XdIcons.check, null, Modifier.size(13.dp), tint = c.acc)
    }
}

@Composable
private fun Trigger(name: String, value: Float) {
    val c = Xd.colors
    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        Text(name, style = XdText.labelSm, color = c.fg2, modifier = Modifier.width(26.dp))
        Box(Modifier.weight(1f).height(8.dp).clip(RoundedCornerShape(50)).background(c.s3)) {
            Box(Modifier.fillMaxWidth(value.coerceIn(0f, 1f)).height(8.dp).clip(RoundedCornerShape(50)).background(c.acc))
        }
        Text(n2(value) + if (value > 0.5f) " " + stringResource(R.string.ct_pressed) else "", style = XdText.mono.copy(fontSize = 11.5.sp),
            color = c.fg, modifier = Modifier.widthIn(min = 44.dp))
    }
}

@Composable
private fun DeadZoneRow(side: String, s: Setting, editing: GlobalSettingsEditing) {
    val raw = editing.raw(s)
    Row(verticalAlignment = Alignment.CenterVertically) {
        Text(side, style = XdText.bodySm, color = Xd.colors.fg2, modifier = Modifier.weight(1f))
        XdStepper(valueLabel(s, raw),
            { stepped(s, raw, -1, wrap = false)?.let { editing.set(s, it) } },
            { stepped(s, raw, 1, wrap = false)?.let { editing.set(s, it) } },
            canPrevious = (raw.toFloatOrNull() ?: 0f) > 0f, canNext = (raw.toFloatOrNull() ?: 0f) < 0.3f, minLabelWidth = 56.dp)
    }
}

@Composable
private fun eventText(event: ControllerTestModel.Event): String = stringResource(when (event.kind) {
    ControllerTestModel.Event.Kind.CONNECTED -> R.string.ct_event_connected
    ControllerTestModel.Event.Kind.RECONNECTED -> R.string.ct_event_reconnected
    ControllerTestModel.Event.Kind.DISCONNECTED -> R.string.ct_event_disconnected
}, event.name)

/** The stick's raw position over its dead zone: inside the inner circle the game gets nothing. */
@Composable
private fun Stick(x: Float, y: Float, deadZone: Float, label: String) {
    val c = Xd.colors
    val dead = hypot(x, y) < deadZone
    Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Canvas(Modifier.size(78.dp)) {
            val radius = size.minDimension / 2f
            drawCircle(Color(0xFF1B211F), radius)
            drawCircle(Color.White.copy(alpha = 0.14f), radius, style = Stroke(2f))
            drawCircle(Color(0xFFE9A23B).copy(alpha = 0.25f), radius * deadZone.coerceIn(0f, 1f))
            drawCircle(Color(0xFFE9A23B).copy(alpha = 0.6f), radius * deadZone.coerceIn(0f, 1f), style = Stroke(1.5f))
            drawCircle(if (dead) c.fg3 else c.acc, 7.dp.toPx(), center + Offset(x.coerceIn(-1f, 1f) * radius, y.coerceIn(-1f, 1f) * radius))
        }
        Text(label, style = XdText.small, color = c.fg3)
    }
}
