package xendroid.compose.core

/** Stable JNI values and configuration keys, independent of the engine's internal names. */
enum class ZeroFgMode(val value: Int, val key: String) {
    OFF(0, "off"), REALLY_ZERO(1, "reallyzero"), ZERO(2, "zero");

    companion object {
        fun fromValue(value: Int): ZeroFgMode = entries.firstOrNull { it.value == value } ?: OFF
    }
}
