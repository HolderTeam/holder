package team.holder.android

import androidx.test.core.app.ActivityScenario
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import androidx.work.WorkInfo
import androidx.work.WorkManager
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith

/** Background sync is on until the user turns it off: launching Holder with no saved choice
 * schedules it, so another device's changes arrive without "Sync now". */
@RunWith(AndroidJUnit4::class)
class BackgroundSyncDefaultTest {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext

    @Test
    fun withNoSavedChoice_launchSchedulesBackgroundSync() = runBlocking {
        HolderSettings.clearGitBackgroundSyncEnabled(context)
        WorkManager.getInstance(context).cancelUniqueWork("git-sync").result.get()
        assertTrue(HolderSettings.gitBackgroundSyncEnabled(context).first())

        ActivityScenario.launch(MainActivity::class.java).use {
            val deadline = System.currentTimeMillis() + 30_000
            var scheduled = false
            while (!scheduled && System.currentTimeMillis() < deadline) {
                scheduled = WorkManager.getInstance(context).getWorkInfosForUniqueWork("git-sync").get()
                    .any { it.state == WorkInfo.State.ENQUEUED || it.state == WorkInfo.State.RUNNING }
                if (!scheduled) Thread.sleep(200)
            }
            assertTrue("background sync should be scheduled on launch by default", scheduled)
        }
    }
}
