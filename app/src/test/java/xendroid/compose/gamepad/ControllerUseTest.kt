package xendroid.compose.gamepad

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class ControllerUseTest {
    @Test fun onlyADeliberatePushPlays() {
        assertFalse(PadPlay.plays(0.05f, -0.07f, 0f, 0f, 0.01f, 0f, 0f, 0f))    // rest noise
        assertFalse(PadPlay.plays(0.3f, 0f, 0f, -0.2f, 0f, 0.4f, 0f, 0f))       // drift, a light touch
        assertFalse(PadPlay.plays(Float.NaN, 0f, 0f, 0f, Float.NaN, 0f, Float.NaN, 0f))
        assertFalse(PadPlay.plays(0f, 0f, 0f, 0f, 0f, 0f, 0.4f, 0f))          // a hat not past half
        assertTrue(PadPlay.plays(0f, 0f, 0f, -0.6f, 0f, 0f, 0f, 0f))          // a stick pushed
        assertTrue(PadPlay.plays(0f, 0f, 0f, 0f, 0f, 0.6f, 0f, 0f))           // a trigger pulled
        assertTrue(PadPlay.plays(0f, 0f, 0f, 0f, 0f, 0f, 0f, -1f))            // hat up
    }

    @Test fun controllersAreKeptInTheOrderTheyFirstPlayed() {
        val before = ControllerUse.descriptors.size
        ControllerUse.note("use-test-b")
        ControllerUse.note("use-test-a")
        ControllerUse.note("use-test-b")
        assertEquals(listOf("use-test-b", "use-test-a"), ControllerUse.descriptors.drop(before))
    }
}
