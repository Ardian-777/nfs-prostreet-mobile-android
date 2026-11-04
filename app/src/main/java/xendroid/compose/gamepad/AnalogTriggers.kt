package xendroid.compose.gamepad

import kotlin.math.roundToInt

/**
 * #97, #104: a controller's trigger axes as the guest gets them (the triggers acted as
 * buttons, so a racing game had no throttle or brake control). The axes follow the key
 * mapping of L2 and R2, which a pad in analog mode doesn't send as keys: left on the guest's
 * LT or RT, a trigger keeps its whole travel, 0-255; on another button it presses that button
 * past half way. Shared by P1's path and [SlotInputRouter].
 */
object AnalogTriggers {
    /** Rest noise under this reads as released. */
    const val DEADZONE = 0.02f
    /** Past this, a trigger mapped to a button presses it (P1's old rule for both triggers). */
    const val BUTTON_THRESHOLD = 0.5f
    const val KC_TRIGGER_L = 14
    const val KC_TRIGGER_R = 15
    private const val KEY_VALUE_UNUSED = -1

    /** A trigger's travel, 0..1, as the guest's trigger byte. */
    fun byteOf(value: Float): Int =
        if (!value.isFinite() || value < DEADZONE) 0 else (value * 255f).roundToInt().coerceIn(0, 255)

    /** One guest key as a motion sample leaves it: [value] is the trigger byte, or unused for a button. */
    data class Output(val key: Int, val pressed: Boolean, val value: Int)

    /**
     * The guest keys one sample of the left and right trigger ([lt], [rt], 0..1) drives, given
     * the guest keys L2 and R2 are mapped to ([ltKey], [rtKey]; null = left unbound, so that
     * trigger drives nothing). Both triggers on one key give it the deeper one.
     */
    fun outputs(lt: Float, rt: Float, ltKey: Int?, rtKey: Int?): List<Output> {
        val travel = linkedMapOf<Int, Float>()
        fun add(key: Int?, value: Float) {
            if (key == null) return
            val v = if (value.isFinite()) value else 0f
            travel[key] = maxOf(travel[key] ?: 0f, v)
        }
        add(ltKey, lt)
        add(rtKey, rt)
        return travel.map { (key, value) ->
            if (key == KC_TRIGGER_L || key == KC_TRIGGER_R) {
                val byte = byteOf(value)
                Output(key, byte > 0, byte)
            } else {
                Output(key, value > BUTTON_THRESHOLD, KEY_VALUE_UNUSED)
            }
        }
    }

    /**
     * What the trigger axes last sent per guest key, so only a change goes out. A key the axes
     * never pressed is never released by them: a trigger at rest must not release LT held by a
     * pad's digital ZL key, nor the button a mapping puts a trigger on held by its own key.
     */
    class Sent {
        private val last = HashMap<Int, Output>()

        /** The outputs of one sample that must be sent. */
        fun changes(outputs: List<Output>): List<Output> = outputs.filter { out ->
            val previous = last[out.key]
            if (previous == out || (previous == null && !out.pressed)) return@filter false
            last[out.key] = out
            true
        }

        /** The releases of what the axes hold; then nothing is held. */
        fun releaseAll(): List<Output> {
            val held = last.values.filter { it.pressed }.map { Output(it.key, false, if (it.value < 0) KEY_VALUE_UNUSED else 0) }
            last.clear()
            return held
        }
    }
}
