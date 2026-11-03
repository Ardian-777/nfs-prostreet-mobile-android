package xendroid.compose.gamepad

import org.junit.Assert.*
import org.junit.Test

class ControllerSlotsTest {
    @Test fun controllersTakePlayersInArrivalOrderAndGetTheirSlotBack() {
        val slots = ControllerSlots()
        assertEquals(0, slots.connect("pad-a"))      // P1, shared with the on-screen pad
        assertEquals(1, slots.connect("pad-b"))
        assertEquals(2, slots.connect("pad-c"))
        assertEquals(1, slots.connect("pad-b"))      // already holds one
        assertEquals(1, slots.disconnect("pad-b"))
        assertEquals(1, slots.connect("pad-d"))      // a newcomer takes the lowest free slot: P2
        assertEquals(3, slots.connect("pad-b"))      // its old slot is taken now: the next free one
    }

    @Test fun aReturningControllerPrefersItsPreviousSlot() {
        val slots = ControllerSlots()
        slots.connect("a"); slots.connect("b"); slots.connect("c")
        assertEquals(1, slots.disconnect("b"))
        assertEquals(1, slots.connect("b"))          // same descriptor, same player
        assertEquals(0, slots.disconnect("a"))
        assertEquals(0, slots.connect("new"))        // lowest free slot for a newcomer
        assertEquals(3, slots.connect("a"))          // its slot is taken: the next free one
        assertNull(slots.connect("fifth"))           // four players at most
        assertNull(slots.disconnect("unknown"))
        assertEquals(listOf("new", "b", "c", "a"), slots.players)
    }

    @Test fun playersCanBeSwapped() {
        val slots = ControllerSlots()
        slots.connect("a"); slots.connect("b")
        slots.swap(0, 1)
        assertEquals(listOf("b", "a", null, null), slots.players)
        assertEquals(1, slots.slotOf("a"))
        slots.disconnect("a")
        assertEquals(1, slots.connect("a"))          // the swapped slot is now its own
        assertThrows(IllegalArgumentException::class.java) { slots.swap(0, 4) }
    }
}

class SlotInputRouterTest {
    private val sent = mutableListOf<String>()
    private val router = SlotInputRouter { slot, key, pressed, value -> sent += "P${slot + 1}:$key:${if (pressed) "down" else "up"}:$value" }

    @Test fun buttonsAreSentOncePerPressAndReleasedOnce() {
        router.keyDown(1, identity = 7, key = 4)
        router.keyDown(1, identity = 7, key = 4)     // key repeat: nothing new
        assertTrue(router.keyUp(1, 7))
        assertFalse(router.keyUp(1, 7))
        assertFalse(router.keyUp(2, 7))              // another player's controller held nothing
        assertEquals(listOf("P2:4:down:-1", "P2:4:up:-1"), sent)
    }

    @Test fun sticksTriggersAndHatFollowP1sRules() {
        router.motion(1, lx = 0.05f, ly = 0f, rx = 0f, ry = 0f, lt = 0f, rt = 0f, hatX = 0f, hatY = 0f)
        assertTrue(sent.none { it.contains("down") })                         // inside the deadzone
        sent.clear()
        router.motion(1, lx = -1f, ly = -0.5f, rx = 0f, ry = 0f, lt = 0.9f, rt = 0f, hatX = 1f, hatY = 0f)
        assertTrue(sent.contains("P2:16:down:-32768"))                       // left stick left, full
        // Pushed up (Android -Y): P1's rule sends the "down" key with a positive value; the
        // value carries the sign, and XInput's Y up is positive.
        assertTrue(sent.contains("P2:19:down:16383"))
        assertTrue(sent.contains("P2:14:down:230"))                          // left trigger, 0.9 of its travel (#97)
        assertTrue(sent.contains("P2:2:down:-1"))                            // hat right = D-pad right
        sent.clear()
        router.motion(1, lx = -1f, ly = -0.5f, rx = 0f, ry = 0f, lt = 0.9f, rt = 0f, hatX = 1f, hatY = 0f)
        assertEquals(emptyList<String>(), sent)                              // nothing changed, nothing sent
        router.motion(1, lx = Float.NaN, ly = 0f, rx = 0f, ry = 0f, lt = 0f, rt = 0f, hatX = 0f, hatY = 0f)
        assertTrue(sent.contains("P2:16:up:0"))                              // a broken sample reads as centred
    }

    @Test fun triggersFollowTheMappingOfL2AndR2() {
        // L2 on the guest's A (4): past half way it presses A, and LT stays alone.
        router.motion(1, lx = 0f, ly = 0f, rx = 0f, ry = 0f, lt = 0.6f, rt = 0.3f, hatX = 0f, hatY = 0f, ltKey = 4, rtKey = 15)
        fun triggerKeys() = sent.filter { it.startsWith("P2:4:") || it.startsWith("P2:15:") }.toSet()
        assertEquals(setOf("P2:4:down:-1", "P2:15:down:77"), triggerKeys())
        sent.clear()
        router.motion(1, lx = 0f, ly = 0f, rx = 0f, ry = 0f, lt = 0.4f, rt = 0f, hatX = 0f, hatY = 0f, ltKey = 4, rtKey = 15)
        assertEquals(setOf("P2:4:up:-1", "P2:15:up:0"), triggerKeys())
    }

    @Test fun aTriggerAtRestLeavesATriggerKeyHeldAlone() {
        router.keyDown(1, identity = 7, key = 14)                            // a Switch Pro's digital ZL
        router.motion(1, lx = 0.6f, ly = 0f, rx = 0f, ry = 0f, lt = 0f, rt = 0f, hatX = 0f, hatY = 0f)
        assertTrue(sent.none { it.startsWith("P2:14:up") })
    }

    @Test fun releasingAPlayerReleasesOnlyWhatItsControllerHeld() {
        router.keyDown(1, 7, 4)
        router.motion(1, lx = 1f, ly = 0f, rx = 0f, ry = 0f, lt = 0f, rt = 1f, hatX = 0f, hatY = -1f)
        router.keyDown(2, 9, 5)
        sent.clear()
        router.release(1)
        assertEquals(setOf("P2:4:up:-1", "P2:18:up:0", "P2:15:up:0", "P2:1:up:-1"), sent.toSet())
        sent.clear()
        router.release(1)                                                    // already released
        assertEquals(emptyList<String>(), sent)
        router.releaseAll()
        assertEquals(listOf("P3:5:up:-1"), sent)
    }
}
