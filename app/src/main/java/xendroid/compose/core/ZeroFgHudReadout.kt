package xendroid.compose.core

/** The requested tier and native output authority are separate during a mode change.
 * Output rate includes real output submissions; it does not prove that a synthetic frame was shown. */
data class ZeroFgHudReadout(
    val requested: ZeroFgMode = ZeroFgMode.OFF,
    val outputActive: Boolean = false,
    private val observedOutputFps: Double? = null,
) {
    val visible: Boolean get() = requested != ZeroFgMode.OFF || outputActive
    val measuredOutputFps: Double? get() = observedOutputFps?.takeIf {
        outputActive && it.isFinite() && it > 0.0
    }
}
