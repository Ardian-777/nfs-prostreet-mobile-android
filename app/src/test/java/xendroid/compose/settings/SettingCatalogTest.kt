package xendroid.compose.settings

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Test

/** The redesign's groups and levels (docs/ui-redesign/bc): every setting placed once, sensibly. */
class SettingCatalogTest {
    @Test fun every_schema_setting_is_placed_exactly_once() {
        val placed = SettingCatalog.placedKeys
        assertEquals("placed twice: ${placed.groupBy { it }.filterValues { it.size > 1 }.keys}", placed.size, placed.toSet().size)
        assertEquals(SettingsSchema.allSettings.map { it.key }.toSet(), placed.toSet())
    }

    @Test fun essential_keeps_player_settings_plus_volume_vibration_and_zerofg() {
        val essential = SettingCatalog.settings(SettingLevel.ESSENTIAL).map { it.key }.toSet()
        assertEquals(SettingsSchema.playerKeys.toSet() + setOf("APU|volume", "HID|vibration", "Vulkan|zerofg_mode"), essential)
    }

    @Test fun levels_nest() {
        val e = SettingCatalog.settings(SettingLevel.ESSENTIAL).toSet()
        val a = SettingCatalog.settings(SettingLevel.ADVANCED).toSet()
        val x = SettingCatalog.settings(SettingLevel.ALL).toSet()
        assertTrue(a.containsAll(e))
        assertTrue(x.containsAll(a))
        assertEquals(SettingsSchema.allSettings.size, x.size)
    }

    @Test fun twenty_nine_new_cvars_and_two_new_values() {
        assertEquals(29, SettingCatalog.newCount)
        assertEquals(2, SettingCatalog.newOptionCount)
    }

    @Test fun dependencies_name_real_settings_and_values() {
        for (key in SettingCatalog.placedKeys) {
            val dep = SettingCatalog.meta(key).dependsOn ?: continue
            val s = SettingsSchema.byKey[dep.key]
            assertNotNull("$key depends on a missing setting ${dep.key}", s)
            when (s) {
                is Setting.Bool -> assertTrue(dep.value == "true" || dep.value == "false")
                is Setting.ListChoice -> assertTrue("$key: ${dep.value} not an option of ${dep.key}", s.options.any { it.value == dep.value })
                else -> error("$key depends on ${dep.key}, which is not a switch or a list")
            }
        }
    }

    @Test fun a_dependent_setting_applies_only_with_the_needed_value() {
        val fsr = SettingsSchema.byKey["Display|postprocess_ffx_fsr_sharpness_reduction"]!!
        assertTrue(SettingCatalog.applies(fsr) { if (it == "Display|postprocess_scaling_and_sharpening") "fsr" else null })
        assertFalse(SettingCatalog.applies(fsr) { if (it == "Display|postprocess_scaling_and_sharpening") "cas" else null })
        val interp = SettingsSchema.byKey["GPU|async_shader_vs_interpreter"]!!
        assertTrue(SettingCatalog.applies(interp) { "true" })
        assertFalse(SettingCatalog.applies(interp) { "false" })
        // Unknown value of the other setting: shown as applying, never hidden by a guess.
        assertTrue(SettingCatalog.applies(interp) { null })
    }

    @Test fun live_settings_are_the_ones_the_in_game_menu_changes() {
        val live = SettingCatalog.placedKeys.filter { SettingCatalog.meta(it).live }.toSet()
        assertEquals(setOf("GPU|framerate_limit", "Display|postprocess_scaling_and_sharpening", "HID|show_touch_overlay", "APU|volume", "Vulkan|zerofg_mode"), live)
    }

    @Test fun pins_toggle() {
        val start = PinnedSettings.DEFAULT
        val without = PinnedSettings.toggled(start, "GPU|framerate_limit")
        assertFalse("GPU|framerate_limit" in without)
        assertEquals(start.toSet(), PinnedSettings.toggled(without, "GPU|framerate_limit").toSet())
    }
}
