package xendroid.compose.gamepad

import androidx.compose.runtime.mutableStateListOf

/**
 * #76: a controller takes a player when it first plays (a key, or a deliberate push of a
 * stick, trigger or hat), not when it connects. A device
 * that never sends input (a handheld's raw controls behind its own mapping service, listed
 * next to the device that does send them) then takes no player, and the controls that do play
 * are P1. The game takes players this way ([xendroid.compose.EmulatorHostActivity]); the
 * Controls area and the controller test show the same order from [ControllerUse].
 */
object PadPlay {
    /**
     * A motion sample that takes a player: a stick, trigger or hat pushed past
     * [TouchOverlayPresence.PUSH], the push that also puts the touch controls aside. A worn
     * stick's drift is not one, or a handheld's drifting controls would take P1 from the pad
     * that plays. Sticks -1..1, triggers 0..1.
     */
    fun plays(lx: Float, ly: Float, rx: Float, ry: Float, lt: Float, rt: Float, hatX: Float, hatY: Float): Boolean =
        TouchOverlayPresence.pushed(lx, ly, rx, ry, lt, rt, hatX, hatY)
}

/**
 * The controllers that have played since the app started (by Android descriptor), in the
 * order they first did: who plays as which player in the Controls area and the controller
 * test (#76). The main activity notes every key press and every motion that [PadPlay.plays]
 * from a controller, before any screen sees it. Main thread only.
 */
object ControllerUse {
    private val order = mutableStateListOf<String>()

    /** Descriptors, first player first; a Compose state, so a screen reading it follows it. */
    val descriptors: List<String> get() = order

    fun note(descriptor: String) {
        if (descriptor !in order) order += descriptor
    }
}
