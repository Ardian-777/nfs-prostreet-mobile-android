package xendroid.compose

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test
import xendroid.compose.core.ZeroFgHudReadout
import xendroid.compose.core.ZeroFgMode

class ZeroFgHudReadoutTest {
    @Test fun aRequestDoesNotEstablishOutputAuthority() {
        val sample = ZeroFgHudReadout(ZeroFgMode.ZERO, false, 60.0)
        assertTrue(sample.visible)
        assertFalse(sample.outputActive)
        assertNull(sample.measuredOutputFps)
    }

    @Test fun outputAuthorityDoesNotInventAnOutputRate() {
        val sample = ZeroFgHudReadout(ZeroFgMode.REALLY_ZERO, true)
        assertTrue(sample.outputActive)
        assertNull(sample.measuredOutputFps)
        assertNull(ZeroFgHudReadout(ZeroFgMode.ZERO, true, 0.0).measuredOutputFps)
        assertNull(ZeroFgHudReadout(ZeroFgMode.ZERO, true, Double.NaN).measuredOutputFps)
    }

    @Test fun outputDrainingAfterOffRemainsVisibleWithoutClaimingAnActiveTier() {
        val sample = ZeroFgHudReadout(ZeroFgMode.OFF, true, 59.5)
        assertTrue(sample.visible)
        assertEquals(ZeroFgMode.OFF, sample.requested)
        assertTrue(sample.outputActive)
        assertEquals(59.5, sample.measuredOutputFps!!, 0.0)
        assertFalse(ZeroFgHudReadout().visible)
    }

}
