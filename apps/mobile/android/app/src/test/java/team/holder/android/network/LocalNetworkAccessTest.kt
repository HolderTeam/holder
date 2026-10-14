package team.holder.android.network

import java.net.InetAddress
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class LocalNetworkAccessTest {
    private fun resolvesTo(vararg addresses: String): (String) -> List<InetAddress> =
        { addresses.map { InetAddress.getByName(it) } }

    private val noLookup: (String) -> List<InetAddress> = { error("unexpected lookup of $it") }

    @Test
    fun gitRemoteHosts() {
        assertEquals("192.168.1.10", LocalNetworkAccess.hostOfGitRemote("ssh://git@192.168.1.10:2222/srv/notes.git"))
        assertEquals("nas.local", LocalNetworkAccess.hostOfGitRemote("git@nas.local:notes.git"))
        assertEquals("nas", LocalNetworkAccess.hostOfGitRemote("nas:repos/notes.git"))
        assertEquals("github.com", LocalNetworkAccess.hostOfGitRemote("git@github.com:owner/repo.git"))
        assertEquals("gitea.example.org", LocalNetworkAccess.hostOfGitRemote("https://gitea.example.org/owner/repo.git"))
        assertEquals("fd00::5", LocalNetworkAccess.hostOfGitRemote("git@[fd00::5]:notes.git"))
        assertEquals("[fd00::5]", LocalNetworkAccess.hostOfGitRemote("ssh://git@[fd00::5]/notes.git"))
        assertNull(LocalNetworkAccess.hostOfGitRemote("/sdcard/notes.git"))
        assertNull(LocalNetworkAccess.hostOfGitRemote("./notes:old.git"))
        assertNull(LocalNetworkAccess.hostOfGitRemote("file:///sdcard/notes.git"))
        assertNull(LocalNetworkAccess.hostOfGitRemote(""))
    }

    @Test
    fun s3EndpointHosts() {
        assertEquals("192.168.0.20", LocalNetworkAccess.hostOfUrl("http://192.168.0.20:9000"))
        assertEquals("minio.lan", LocalNetworkAccess.hostOfUrl("https://minio.lan/base"))
        assertEquals("s3.amazonaws.com", LocalNetworkAccess.hostOfUrl("https://s3.amazonaws.com"))
        assertNull(LocalNetworkAccess.hostOfUrl("not a url"))
    }

    @Test
    fun localAddressLiterals() {
        for (host in listOf("10.0.2.2", "172.16.5.4", "172.31.255.1", "192.168.0.24", "169.254.1.1", "fd00::5", "[fe80::1]")) {
            assertTrue(host, LocalNetworkAccess.mayBeLocal(host, noLookup))
        }
        for (host in listOf("127.0.0.1", "::1", "localhost", "8.8.8.8", "172.32.0.1", "100.96.202.74", "2001:db8::1")) {
            assertFalse(host, LocalNetworkAccess.mayBeLocal(host, noLookup))
        }
    }

    @Test
    fun localNamesNeedNoLookup() {
        for (host in listOf("nas", "NAS.local", "git.lan", "router.home.arpa", "box.internal", "printer.localdomain.")) {
            assertTrue(host, LocalNetworkAccess.mayBeLocal(host, noLookup))
        }
    }

    @Test
    fun otherNamesAreLocalOnlyIfTheyResolveLocally() {
        assertTrue(LocalNetworkAccess.mayBeLocal("nas.example.org", resolvesTo("192.168.1.10")))
        assertTrue(LocalNetworkAccess.mayBeLocal("git.example.org", resolvesTo("203.0.113.5", "10.1.2.3")))
        assertFalse(LocalNetworkAccess.mayBeLocal("github.com", resolvesTo("140.82.121.3")))
        assertFalse(LocalNetworkAccess.mayBeLocal("offline.example.org") { throw java.net.UnknownHostException(it) })
    }
}
