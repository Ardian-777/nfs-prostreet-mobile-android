package xendroid.compose.gamepad

import kotlin.math.abs

/**
 * I01: which physical controller plays as which player. The first controller shares P1
 * with the on-screen pad; the next ones take P2..P4 in the order they first play (#76). A
 * controller that comes back (same device descriptor) gets its previous slot again when it
 * is still free.
 * Thread-safe: controllers come from the main thread, companion phones from their sockets.
 */
class ControllerSlots(private val slotCount: Int = 4) {
    private val bySlot = arrayOfNulls<String>(slotCount)
    private val lastSlot = mutableMapOf<String, Int>()

    @Synchronized
    fun slotOf(key: String): Int? = bySlot.indexOf(key).takeIf { it >= 0 }

    /** Like [connect], but never P1: a remote player only gets P2..P4. */
    @Synchronized
    fun connectRemote(key: String): Int? {
        slotOf(key)?.let { return it }
        val previous = lastSlot[key]?.takeIf { it > 0 && bySlot[it] == null }
        val slot = previous ?: (1 until slotCount).firstOrNull { bySlot[it] == null } ?: return null
        bySlot[slot] = key
        lastSlot[key] = slot
        return slot
    }

    /** Gives [key] a slot (the one it holds, else its previous one, else the lowest free); null when all are taken. */
    @Synchronized
    fun connect(key: String): Int? {
        slotOf(key)?.let { return it }
        val slot = lastSlot[key]?.takeIf { bySlot[it] == null } ?: bySlot.indexOfFirst { it == null }.takeIf { it >= 0 }
            ?: return null
        bySlot[slot] = key
        lastSlot[key] = slot
        return slot
    }

    /** Frees [key]'s slot and returns it. */
    @Synchronized
    fun disconnect(key: String): Int? {
        val slot = slotOf(key) ?: return null
        bySlot[slot] = null
        return slot
    }

    /** Exchanges two players' controllers (either may be empty). */
    @Synchronized
    fun swap(a: Int, b: Int) {
        require(a in 0 until slotCount && b in 0 until slotCount)
        val first = bySlot[a]
        bySlot[a] = bySlot[b]
        bySlot[b] = first
        bySlot[a]?.let { lastSlot[it] = a }
        bySlot[b]?.let { lastSlot[it] = b }
    }

    /** Device key per slot, P1 first; null for a free slot. */
    val players: List<String?> @Synchronized get() = bySlot.toList()
}

/**
 * I03: the input of controllers playing P2..P4, turned into guest key events per slot
 * with the same rules as P1's path (8% stick deadzone, triggers as [AnalogTriggers], hat at
 * half deflection). Each slot keeps its own pressed state, so a disconnect or a menu
 * releases exactly what that controller held.
 */
class SlotInputRouter(
    private val deadzone: Float = 0.08f,
    private val send: (slot: Int, key: Int, pressed: Boolean, value: Int) -> Unit,
) {
    private class Pad {
        val keys = HashMap<Long, Int>()                 // key identity -> guest key
        val axisPressed = BooleanArray(24)
        val axisValue = IntArray(24) { Int.MIN_VALUE }
        val held = BooleanArray(24)                     // hat directions
        val triggers = AnalogTriggers.Sent()
    }

    private val pads = HashMap<Int, Pad>()
    private fun pad(slot: Int) = pads.getOrPut(slot) { Pad() }

    fun keyDown(slot: Int, identity: Long, key: Int) {
        val pad = pad(slot)
        if (pad.keys.put(identity, key) == null) send(slot, key, true, KEY_VALUE_UNUSED)
    }

    /** Forgets a held key without releasing it: what it pressed is the trigger axis's now (#97). */
    fun forget(slot: Int, identity: Long) {
        pads[slot]?.keys?.remove(identity)
    }

    /** True when this slot held the key (the event is consumed). */
    fun keyUp(slot: Int, identity: Long): Boolean {
        val key = pads[slot]?.keys?.remove(identity) ?: return false
        send(slot, key, false, KEY_VALUE_UNUSED)
        return true
    }

    /** One controller motion sample: sticks -1..1 (Y down positive, Android's convention), triggers 0..1,
     *  hat -1..1; [ltKey] and [rtKey] are the guest keys L2 and R2 are mapped to (null: unbound). */
    fun motion(slot: Int, lx: Float, ly: Float, rx: Float, ry: Float, lt: Float, rt: Float, hatX: Float, hatY: Float,
               ltKey: Int? = KC_TRIGGER_L, rtKey: Int? = KC_TRIGGER_R) {
        val pad = pad(slot)
        axisPair(slot, pad, lx, KC_LTHUMB_LEFT, KC_LTHUMB_RIGHT, invert = false)
        axisPair(slot, pad, ly, KC_LTHUMB_UP, KC_LTHUMB_DOWN, invert = true)
        axisPair(slot, pad, rx, KC_RTHUMB_LEFT, KC_RTHUMB_RIGHT, invert = false)
        axisPair(slot, pad, ry, KC_RTHUMB_UP, KC_RTHUMB_DOWN, invert = true)
        for (out in pad.triggers.changes(AnalogTriggers.outputs(lt, rt, ltKey, rtKey))) send(slot, out.key, out.pressed, out.value)
        held(slot, pad, KC_DPAD_LEFT, hatX < -0.5f)
        held(slot, pad, KC_DPAD_RIGHT, hatX > 0.5f)
        held(slot, pad, KC_DPAD_UP, hatY < -0.5f)
        held(slot, pad, KC_DPAD_DOWN, hatY > 0.5f)
    }

    /** Releases everything [slot] holds (disconnect, menu, focus loss). */
    fun release(slot: Int) {
        val pad = pads.remove(slot) ?: return
        pad.keys.values.forEach { send(slot, it, false, KEY_VALUE_UNUSED) }
        for (code in pad.axisPressed.indices) if (pad.axisPressed[code]) send(slot, code, false, 0)
        for (code in pad.held.indices) if (pad.held[code]) send(slot, code, false, KEY_VALUE_UNUSED)
        for (out in pad.triggers.releaseAll()) send(slot, out.key, false, out.value)
    }

    fun releaseAll() = pads.keys.toList().forEach(::release)

    private fun axisPair(slot: Int, pad: Pad, axis: Float, negKey: Int, posKey: Int, invert: Boolean) {
        val raw = if (invert) -axis else axis
        val v = if (abs(raw) < deadzone || !raw.isFinite()) 0f else raw.coerceIn(-1f, 1f)
        when {
            v < 0f -> { axis(slot, pad, posKey, false, 0); axis(slot, pad, negKey, true, (v * 32768f).toInt()) }
            v > 0f -> { axis(slot, pad, negKey, false, 0); axis(slot, pad, posKey, true, (v * 32767f).toInt()) }
            else -> { axis(slot, pad, negKey, false, 0); axis(slot, pad, posKey, false, 0) }
        }
    }

    private fun axis(slot: Int, pad: Pad, code: Int, pressed: Boolean, value: Int) {
        if (pad.axisPressed[code] == pressed && pad.axisValue[code] == value) return
        pad.axisPressed[code] = pressed
        pad.axisValue[code] = value
        send(slot, code, pressed, value)
    }

    private fun held(slot: Int, pad: Pad, code: Int, down: Boolean) {
        if (pad.held[code] == down) return
        pad.held[code] = down
        send(slot, code, down, KEY_VALUE_UNUSED)
    }

    companion object {
        const val KEY_VALUE_UNUSED = -1
        // Guest key indices (xendroid_emu.cpp key_maps order).
        const val KC_DPAD_LEFT = 0
        const val KC_DPAD_UP = 1
        const val KC_DPAD_RIGHT = 2
        const val KC_DPAD_DOWN = 3
        const val KC_TRIGGER_L = 14
        const val KC_TRIGGER_R = 15
        const val KC_LTHUMB_LEFT = 16
        const val KC_LTHUMB_UP = 17
        const val KC_LTHUMB_RIGHT = 18
        const val KC_LTHUMB_DOWN = 19
        const val KC_RTHUMB_LEFT = 20
        const val KC_RTHUMB_UP = 21
        const val KC_RTHUMB_RIGHT = 22
        const val KC_RTHUMB_DOWN = 23
    }
}
