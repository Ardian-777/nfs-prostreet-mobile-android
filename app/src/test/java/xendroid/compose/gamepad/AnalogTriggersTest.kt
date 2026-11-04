package xendroid.compose.gamepad

import org.junit.Assert.assertEquals
import org.junit.Test
import xendroid.compose.gamepad.AnalogTriggers.Output

class AnalogTriggersTest {
    @Test fun travelBecomesTheGuestsTriggerByte() {
        assertEquals(0, AnalogTriggers.byteOf(0f))
        assertEquals(0, AnalogTriggers.byteOf(0.015f))        // rest noise
        assertEquals(0, AnalogTriggers.byteOf(Float.NaN))
        assertEquals(0, AnalogTriggers.byteOf(-0.5f))
        assertEquals(128, AnalogTriggers.byteOf(0.5f))
        assertEquals(255, AnalogTriggers.byteOf(1f))
        assertEquals(255, AnalogTriggers.byteOf(1.2f))         // a pad reporting past its range
    }

    @Test fun triggersOnLtAndRtKeepTheirTravelAndAnUnboundOneDrivesNothing() {
        assertEquals(listOf(Output(14, true, 64), Output(15, false, 0)),
            AnalogTriggers.outputs(0.25f, 0f, 14, 15))
        assertEquals(listOf(Output(15, false, 0)), AnalogTriggers.outputs(0.9f, 0f, null, 15))
    }

    @Test fun theAxesSendChangesAndNeverReleaseWhatTheyDidNotPress() {
        val sent = AnalogTriggers.Sent()
        assertEquals(emptyList<Output>(), sent.changes(AnalogTriggers.outputs(0f, 0f, 14, 4)))  // at rest
        assertEquals(listOf(Output(14, true, 128)), sent.changes(AnalogTriggers.outputs(0.5f, 0f, 14, 4)))
        assertEquals(emptyList<Output>(), sent.changes(AnalogTriggers.outputs(0.5f, 0f, 14, 4)))  // no change
        assertEquals(listOf(Output(14, false, 0)), sent.changes(AnalogTriggers.outputs(0f, 0f, 14, 4)))
        assertEquals(listOf(Output(4, true, -1)), sent.changes(AnalogTriggers.outputs(0f, 0.9f, 14, 4)))
        assertEquals(listOf(Output(4, false, -1)), sent.releaseAll())
        assertEquals(emptyList<Output>(), sent.releaseAll())
    }

    @Test fun aTriggerOnAButtonPressesItPastHalfWay() {
        assertEquals(listOf(Output(4, false, -1), Output(15, true, 255)),
            AnalogTriggers.outputs(0.5f, 1f, 4, 15))
        assertEquals(listOf(Output(4, true, -1), Output(15, false, 0)),
            AnalogTriggers.outputs(0.51f, 0f, 4, 15))
    }

    @Test fun bothTriggersOnOneKeyGiveTheDeeperOne() {
        assertEquals(listOf(Output(14, true, 204)), AnalogTriggers.outputs(0.3f, 0.8f, 14, 14))
    }
}
