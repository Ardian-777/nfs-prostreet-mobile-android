package xendroid.compose.gamepad

import android.view.KeyEvent
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ControllerTestModelTest {
    private fun pad(id: Int, descriptor: String = "desc-$id") =
        TestedDevice(id, "Pad $id", descriptor, 0x045E, 0x028E, listOf("gamepad", "joystick"), canVibrate = true, hasGyro = false)

    @Test fun aGameStartedNowTakesControllersInTheOrderTheyFirstPlayed() {
        val model = ControllerTestModel()
        listOf(9, 5, 12, 3, 7, 20).forEach { model.connected(pad(it)) }
        model.disconnected(20)
        // 20 played first but is gone; 5 and 7 haven't played (a silent twin of a handheld's controls, #76).
        val used = listOf("desc-20", "desc-12", "desc-3", "desc-9")
        assertEquals(mapOf(12 to 0, 3 to 1, 9 to 2, 5 to null, 7 to null), ControllerTestModel.playerSlots(model.all, used))
        assertEquals(mapOf(12 to 0, 3 to null, 9 to null, 5 to null, 7 to null),
            ControllerTestModel.playerSlots(model.all, used, slots = 1))
        assertEquals(mapOf(3 to null, 5 to null, 7 to null, 9 to null, 12 to null),
            ControllerTestModel.playerSlots(model.all, emptyList()))
    }

    @Test fun buttonsLightWhileHeldAndStayCheckedOff() {
        val model = ControllerTestModel()
        model.connected(pad(5))
        model.key(5, KeyEvent.KEYCODE_BUTTON_A, down = true)
        assertEquals(setOf("A"), model.all.single().pressed)
        model.key(5, KeyEvent.KEYCODE_BUTTON_A, down = false)
        model.key(5, KeyEvent.KEYCODE_BUTTON_THUMBR, down = true)
        val device = model.all.single()
        assertEquals(setOf("R3"), device.pressed)
        assertEquals(setOf("A", "R3"), device.seen)
        model.key(99, KeyEvent.KEYCODE_BUTTON_B, down = true)             // unknown device: ignored
        assertEquals("Key 999", ControllerTestModel.buttonName(999))
        assertEquals("Back", ControllerTestModel.buttonName(KeyEvent.KEYCODE_BUTTON_SELECT))
    }

    @Test fun theGameSeesWhatTheRulesLetThrough() {
        val model = ControllerTestModel(deadzone = 0.08f)
        val view = model.gameView(PadAxes(lx = 0.05f, ly = -0.5f, rx = 1.4f, ry = Float.NaN, lt = 0.4f, rt = 0.6f, hatX = -1f, hatY = 0.7f))
        assertEquals(0f, view.lx)                                            // inside the dead zone
        assertEquals(-0.5f, view.ly)
        assertEquals(1f, view.rx)                                            // clamped
        assertEquals(0f, view.ry)                                            // not a number: nothing
        assertFalse(view.ltPressed)
        assertTrue(view.rtPressed)
        assertEquals(setOf(ControllerTestModel.DPAD_LEFT, ControllerTestModel.DPAD_DOWN), view.dpad)
        // A hat and a pulled trigger tick the checklist like buttons.
        model.connected(pad(1))
        model.motion(1, PadAxes(rt = 1f, hatY = -1f))
        assertEquals(setOf("RT", ControllerTestModel.DPAD_UP), model.all.single().seen)
    }

    @Test fun aControllerThatComesBackIsRecognized() {
        val model = ControllerTestModel()
        model.connected(pad(5, "xbox-1"))
        model.key(5, KeyEvent.KEYCODE_BUTTON_X, down = true)
        model.disconnected(5)
        assertFalse(model.all.single().connected)
        assertEquals(emptySet<String>(), model.all.single().pressed)        // nothing stays held
        model.connected(pad(9, "xbox-1"))                                    // same controller, new id
        val back = model.all.single()
        assertEquals(9, back.id)
        assertEquals(setOf("X"), back.seen)
        assertEquals(listOf("Pad 9: reconnected", "Pad 5: disconnected", "Pad 5: connected"), model.events.map { it.english })
        model.connected(pad(9, "xbox-1"))                                    // a "changed" callback: no new line
        assertEquals(3, model.events.size)
    }
}
