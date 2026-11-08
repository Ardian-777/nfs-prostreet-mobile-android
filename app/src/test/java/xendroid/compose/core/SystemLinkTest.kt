package xendroid.compose.core

import org.junit.Assert.*
import org.junit.Test

class SystemLinkTest {
    private fun ip(vararg b: Int) = ByteArray(4) { b[it].toByte() }

    @Test fun broadcastIsTheSubnetWithTheHostBitsSet() {
        assertEquals("192.168.1.255", SystemLink.dotted(SystemLink.broadcast(ip(192, 168, 1, 37), 24)))
        assertEquals("10.147.31.255", SystemLink.dotted(SystemLink.broadcast(ip(10, 147, 17, 5), 20)))
        assertEquals("100.127.255.255", SystemLink.dotted(SystemLink.broadcast(ip(100, 64, 0, 2), 10)))  // Tailscale's CGNAT range
        assertEquals("255.255.255.255", SystemLink.dotted(SystemLink.broadcast(ip(192, 168, 1, 37), 0)))
        assertEquals("192.168.1.37", SystemLink.dotted(SystemLink.broadcast(ip(192, 168, 1, 37), 32)))
    }
}
