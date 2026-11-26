package team.holder.android

import androidx.test.platform.app.InstrumentationRegistry
import org.junit.rules.ExternalResource
import team.holder.android.git.backup.SnapshotProtection
import team.holder.android.git.backup.snapshotFile

/** Starts the app the way a test expects: on its start screen, not on Restore from backup.
 *
 * Installing the test APK is a fresh install, and on a phone signed in to Google, Android Auto
 * Backup restores the debug app's last cloud backup into it before the first launch. That
 * includes a backup snapshot, so Holder offers to restore it (see RestoreOffer) and opens on
 * Restore from backup instead, and every test waiting for the start screen times out. Emulators
 * have no Google account, so CI never sees it. This removes a restored snapshot and the offer's
 * marker before the activity launches, so apply it with a lower order than the activity rule. */
class NoRestoredBackupRule : ExternalResource() {
    override fun before() {
        val context = InstrumentationRegistry.getInstrumentation().targetContext
        snapshotFile(context).delete()
        SnapshotProtection.disarm(context.filesDir)
    }
}
