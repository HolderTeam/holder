package team.holder.android.ui.screens

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class SyncErrorTextTest {
    @Test
    fun noNetworkIsOffline_andDoesNotOfferRegisteringTheDevice() {
        // As the Moto showed it with Wi-Fi and data off (#535).
        val raw = "git_remote_fetch failed (rc=-1): failed to resolve address for github.com: " +
            "No address associated with hostname"
        assertEquals(SyncErrorKind.OFFLINE, syncErrorKind(raw))
        assertFalse(syncErrorKind(raw).offersRegisterDevice)
        for (other in listOf("connect: Network is unreachable", "failed to connect to github.com: Connection timed out",
                "Software caused connection abort", "Could not resolve host: github.com")) {
            assertEquals(other, SyncErrorKind.OFFLINE, syncErrorKind(other))
        }
    }

    @Test
    fun aRefusedKeyOrMissingRepository_offersRegisteringTheDevice() {
        val refused = "git_remote_fetch failed (rc=-16): ERROR: Permission denied (publickey)"
        assertEquals(SyncErrorKind.AUTH, syncErrorKind(refused))
        assertTrue(syncErrorKind(refused).offersRegisterDevice)
        val missing = "ERROR: Repository not found."
        assertEquals(SyncErrorKind.NOT_FOUND, syncErrorKind(missing))
        assertTrue(syncErrorKind(missing).offersRegisterDevice)
        assertEquals(SyncErrorKind.OTHER, syncErrorKind("Pull blocked by staged or unresolved Git changes"))
        assertFalse(SyncErrorKind.OTHER.offersRegisterDevice)
    }

    @Test
    fun syncNowOutcome_saysWhatHappened() {
        assertEquals("Couldn't sync.", syncNowOutcome(true, "failed to resolve address for github.com", false, null, ""))
        assertEquals("Brought in changes (1 conflict copy preserved), but couldn't send this phone's.",
            syncNowOutcome(true, null, true, "Permission denied (publickey)", " (1 conflict copy preserved)"))
        assertEquals("Synced.", syncNowOutcome(true, null, true, null, ""))
        assertEquals("Nothing to sync.", syncNowOutcome(false, null, false, null, ""))
    }
}
