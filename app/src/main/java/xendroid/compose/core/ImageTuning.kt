package xendroid.compose.core

/**
 * The in-game menu's Image options, tried live (the core applies them from the next frame) and
 * kept for the game on request. -1 in each: the game's Settings value.
 *  - [antialiasing]: 0 none, 1 FXAA, 2 FXAA extreme;
 *  - [sharpness]: 0 soft .. 4 maximum, for CAS (extra sharpness) and FSR (less sharpness
 *    reduction) alike, on the values the Settings lists offer;
 *  - [dither]: 0 off, 1 on.
 */
data class ImageTuning(val antialiasing: Int = -1, val sharpness: Int = -1, val dither: Int = -1) {
    /** CAS additional sharpness for [sharpness]; negative: the Settings value. */
    val casSharpness: Float get() = if (sharpness in CAS.indices) CAS[sharpness].toFloat() else -1f

    /** FSR sharpness reduction (stops) for [sharpness]; negative: the Settings value. */
    val fsrSharpnessReduction: Float get() = if (sharpness in FSR.indices) FSR[sharpness].toFloat() else -1f

    fun nextAntialiasing() = copy(antialiasing = if (antialiasing >= 2) -1 else antialiasing + 1)
    fun nextSharpness() = copy(sharpness = if (sharpness >= 4) -1 else sharpness + 1)
    fun nextDither() = copy(dither = when (dither) { -1 -> 1; 1 -> 0; else -> -1 })

    /**
     * What to keep for the game, as (Settings key, raw value) the way the Settings screens write
     * them: the scaling effect picked in the menu ([scalingEffect], -1: none picked) and each
     * option set here. Empty when nothing differs from the Settings.
     */
    fun cvars(scalingEffect: Int): List<Pair<String, String>> = buildList {
        if (scalingEffect in EFFECTS.indices) add("Display|postprocess_scaling_and_sharpening" to EFFECTS[scalingEffect])
        if (antialiasing in AA.indices) add("Display|postprocess_antialiasing" to AA[antialiasing])
        if (sharpness in CAS.indices) {
            add("Display|postprocess_ffx_cas_additional_sharpness" to CAS[sharpness])
            add("Display|postprocess_ffx_fsr_sharpness_reduction" to FSR[sharpness])
        }
        if (dither >= 0) add("Display|postprocess_dither" to (dither == 1).toString())
    }

    /**
     * Round 2: every key the Image options write, each with its value or null where the option
     * follows the Settings (the game's own value goes), as the menu keeps them for the game.
     */
    fun cvarValues(): Map<String, String?> = mapOf(
        "Display|postprocess_antialiasing" to AA.getOrNull(antialiasing),
        "Display|postprocess_ffx_cas_additional_sharpness" to CAS.getOrNull(sharpness),
        "Display|postprocess_ffx_fsr_sharpness_reduction" to FSR.getOrNull(sharpness),
        "Display|postprocess_dither" to if (dither >= 0) (dither == 1).toString() else null,
    )

    companion object {
        /** The scaling effect [effect] (-1: the Settings one) as the config writes it; null = the Settings'. */
        fun scalingValue(effect: Int): String? = EFFECTS.getOrNull(effect)

        /** The core's scaling effects in the menu's order (Presenter's Effect). */
        val EFFECTS = listOf("bilinear", "cas", "fsr", "sgsr", "lanczos", "crt", "sgsr_edge")
        private val AA = listOf("none", "fxaa", "fxaa_extreme")
        private val CAS = listOf("0.0", "0.25", "0.5", "0.75", "1.0")
        private val FSR = listOf("2.0", "1.0", "0.5", "0.2", "0.0")
    }
}
