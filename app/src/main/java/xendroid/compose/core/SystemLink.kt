package xendroid.compose.core

import android.content.Context
import android.net.ConnectivityManager
import android.net.LinkProperties
import android.net.NetworkCapabilities
import android.net.wifi.WifiManager
import java.net.Inet4Address
import java.security.SecureRandom

/**
 * What the core's system link (xenia/kernel/systemlink.cc) needs from Android: this device's IPv4
 * address and its network's broadcast address, an Ethernet address of its own, and the Wi-Fi
 * multicast lock, without which many phones drop the broadcasts the other consoles send.
 *
 * They reach the core as launch cvars, never written to a config file: the address changes with
 * the network, and the Ethernet address must not travel with a copied config.
 */
object SystemLink {
    const val SECTION = "Network"
    const val ENABLED = "systemlink"
    const val PEERS = "systemlink_peers"

    private const val PREFS = "system_link"
    private const val KEY_MAC = "mac"

    /**
     * The launch cvars. [overVpn]: other consoles are named (systemlink_peers), so a VPN's address
     * is the one to give them; otherwise the Wi-Fi's, where the broadcasts go. Without an IPv4
     * network only the Ethernet address goes (the core finds an interface itself).
     */
    fun launchArgs(context: Context, overVpn: Boolean): List<String> = buildList {
        add("--systemlink_mac=${macAddress(context)}")
        val link = activeLink(context, overVpn) ?: return@buildList
        val v4 = link.linkAddresses.firstOrNull { it.address is Inet4Address } ?: return@buildList
        val address = v4.address.address
        val bits = v4.prefixLength.coerceIn(0, 32)
        add("--systemlink_address=${dotted(address)}")
        add("--systemlink_broadcast=${dotted(broadcast(address, bits))}/$bits")
    }

    /** The Wi-Fi multicast lock, held while a game runs with system link; null without Wi-Fi. */
    fun multicastLock(context: Context): WifiManager.MulticastLock? =
        context.applicationContext.getSystemService(WifiManager::class.java)
            ?.createMulticastLock("Xendroid+ system link")
            ?.apply { setReferenceCounted(false) }

    /**
     * This device's Ethernet address for its XNADDR, made once and kept: two devices with one
     * address confuse the titles' peer lists. Microsoft's prefix marked locally administered, so no
     * real console has it.
     */
    fun macAddress(context: Context): String {
        val prefs = context.applicationContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE)
        prefs.getString(KEY_MAC, null)?.let { if (MAC.matches(it)) return it }
        val tail = ByteArray(3).also { SecureRandom().nextBytes(it) }
        val mac = (byteArrayOf(0x02, 0x22, 0x48) + tail).joinToString(":") { "%02X".format(it.toInt() and 0xFF) }
        prefs.edit().putString(KEY_MAC, mac).apply()
        return mac
    }

    /** A new Ethernet address, for a device whose data was copied from another one. */
    fun resetMacAddress(context: Context) {
        context.applicationContext.getSharedPreferences(PREFS, Context.MODE_PRIVATE).edit().remove(KEY_MAC).apply()
    }

    /** The network to play on: a VPN with IPv4 when asked for and up, else the Wi-Fi (or the active one). */
    private fun activeLink(context: Context, overVpn: Boolean): LinkProperties? {
        val cm = context.applicationContext.getSystemService(ConnectivityManager::class.java) ?: return null
        @Suppress("DEPRECATION")
        val networks = cm.allNetworks
        val vpn = if (!overVpn) null else networks.firstOrNull {
            cm.getNetworkCapabilities(it)?.hasTransport(NetworkCapabilities.TRANSPORT_VPN) == true &&
                cm.getLinkProperties(it)?.linkAddresses?.any { a -> a.address is Inet4Address } == true
        }
        val wifi = networks.firstOrNull {
            cm.getNetworkCapabilities(it)?.hasTransport(NetworkCapabilities.TRANSPORT_WIFI) == true
        }
        val network = vpn ?: wifi ?: cm.activeNetwork ?: return null
        return cm.getLinkProperties(network)
    }

    internal fun broadcast(address: ByteArray, bits: Int): ByteArray {
        val ip = address.fold(0L) { acc, b -> (acc shl 8) or (b.toLong() and 0xFF) }
        val mask = if (bits == 0) 0L else (0xFFFFFFFFL shl (32 - bits)) and 0xFFFFFFFFL
        val b = (ip and mask) or (mask.inv() and 0xFFFFFFFFL)
        return ByteArray(4) { i -> (b shr (24 - 8 * i)).toByte() }
    }

    internal fun dotted(address: ByteArray): String = address.joinToString(".") { (it.toInt() and 0xFF).toString() }

    private val MAC = Regex("([0-9A-F]{2}:){5}[0-9A-F]{2}")
}
