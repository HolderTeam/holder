package team.holder.android.network

import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import java.net.Inet6Address
import java.net.InetAddress
import java.net.URI

/**
 * Android 17's local network protection. From API 37, an app targeting API 37 needs the
 * [PERMISSION] runtime permission to open connections to devices on the network the phone is
 * attached to, such as a NAS or Git server on the same Wi-Fi. Without it a connection is
 * silently dropped and only fails when it times out (two minutes for a Git operation), so
 * Holder checks before connecting: it asks for the permission in the foreground, and fails fast
 * with [BLOCKED_MESSAGE] in the background. Loopback and internet hosts never need it.
 *
 * Which hosts count is decided by the device's routing (its directly attached network), which
 * Holder cannot see in advance for a host name. [mayBeLocal] errs towards asking: private and
 * link-local addresses, names that resolve to them, single-label names and local-only domains.
 */
object LocalNetworkAccess {
    const val PERMISSION = "android.permission.ACCESS_LOCAL_NETWORK"

    // Android lists the permission under "Nearby devices" in an app's permission settings.
    const val BLOCKED_MESSAGE =
        "Holder needs permission to reach devices on your local network. " +
            "Allow \"Nearby devices\" in Settings > Apps > Holder > Permissions."

    private val LOCAL_DOMAIN_SUFFIXES = listOf(".local", ".lan", ".home", ".home.arpa", ".internal", ".localdomain")

    /** True on Android 17 and later, where the permission is enforced for this app. */
    val isEnforced: Boolean get() = Build.VERSION.SDK_INT >= 37

    fun isGranted(context: Context): Boolean =
        !isEnforced || context.checkSelfPermission(PERMISSION) == PackageManager.PERMISSION_GRANTED

    /** Whether connecting to [host] now would be blocked. May resolve [host]; call off the
     * main thread. */
    fun isBlocked(context: Context, host: String?): Boolean =
        host != null && !isGranted(context) && mayBeLocal(host)

    /** Whether [host] is, or resolves to, an address that may be on the local network.
     * May resolve [host]; call off the main thread. A failed lookup counts as not local. */
    fun mayBeLocal(host: String, resolve: (String) -> List<InetAddress> = ::resolveAll): Boolean {
        val name = host.trim().trimEnd('.').removePrefix("[").removeSuffix("]").lowercase()
        if (name.isEmpty() || name == "localhost") return false
        parseIpLiteral(name)?.let { return isLocalAddress(it) }
        if ('.' !in name || LOCAL_DOMAIN_SUFFIXES.any { name.endsWith(it) }) return true
        return runCatching { resolve(name) }.getOrDefault(emptyList()).any(::isLocalAddress)
    }

    /** Private (RFC 1918, IPv6 unique local) or link-local, and not loopback. */
    fun isLocalAddress(address: InetAddress): Boolean {
        if (address.isLoopbackAddress || address.isAnyLocalAddress) return false
        if (address.isSiteLocalAddress || address.isLinkLocalAddress) return true
        return address is Inet6Address && (address.address[0].toInt() and 0xFE) == 0xFC
    }

    /** The host of an S3 endpoint such as `https://nas.local:9000/bucket`, or null. */
    fun hostOfUrl(url: String): String? =
        runCatching { URI(url.trim()).host }.getOrNull()?.takeIf { it.isNotEmpty() }

    /** The host of a Git remote in any form Git accepts: `ssh://user@host:port/path`,
     * `https://host/path`, `git://host/path`, or scp-like `user@host:path`. Null for a local
     * path or `file://` URL, which needs no network. */
    fun hostOfGitRemote(remote: String): String? {
        val value = remote.trim()
        if (value.isEmpty()) return null
        if ("://" in value) {
            return if (value.startsWith("file://", ignoreCase = true)) null else hostOfUrl(value)
        }
        // scp-like syntax: a colon before any slash, as Git itself decides.
        val colon = value.indexOf(':')
        val slash = value.indexOf('/')
        if (colon <= 0 || (slash in 0 until colon)) return null
        val authority = value.substring(0, colon).substringAfterLast('@')
        if (authority.startsWith("[")) {
            return value.substringAfter('[').substringBefore(']').takeIf { it.isNotEmpty() }
        }
        return authority.takeIf { it.isNotEmpty() }
    }

    private fun resolveAll(host: String): List<InetAddress> = InetAddress.getAllByName(host).toList()

    /** Parses only numeric IPv4/IPv6 literals, never doing a lookup. */
    private fun parseIpLiteral(value: String): InetAddress? {
        val looksNumeric = value.all { it.isDigit() || it == '.' } && value.count { it == '.' } == 3 ||
            (':' in value && value.all { it.isLetterOrDigit() || it == ':' || it == '.' || it == '%' })
        if (!looksNumeric) return null
        return runCatching { InetAddress.getByName(value) }.getOrNull()
    }
}
