package team.holder.android.sync

import android.content.Context
import androidx.work.Constraints
import androidx.work.ExistingPeriodicWorkPolicy
import androidx.work.ExistingWorkPolicy
import androidx.work.NetworkType
import androidx.work.OneTimeWorkRequestBuilder
import androidx.work.PeriodicWorkRequestBuilder
import androidx.work.WorkManager
import androidx.work.workDataOf
import java.util.concurrent.TimeUnit
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

private const val UNIQUE_WORK_NAME = "git-sync"
private const val SOON_WORK_NAME = "git-sync-soon"

/** Input to [GitSyncWorker]: sync now, not when the user's interval says it's due. */
internal const val SYNC_SOON_KEY = "soon"

/** How long after the last change a sync starts: typing a card makes one sync, not one per
 * autosave. */
internal const val SYNC_AFTER_CHANGE_SECONDS = 30L

/** Opening Holder syncs at most this often, so returning to it again and again doesn't. */
private const val SYNC_ON_OPEN_MIN_GAP_MS = 60_000L

/** Reconciles WorkManager's scheduled periodic sync with HolderSettings, matching an
 * enable/disable toggle and an interval the user can change from SettingsScreen. Call
 * whenever either setting changes, and once at app startup to restore the schedule (a fresh
 * process doesn't automatically know a periodic work request was enqueued in a past one). */
object GitSyncScheduler {
    fun reconcile(context: Context, enabled: Boolean, intervalMinutes: Int) {
        val workManager = WorkManager.getInstance(context)
        if (!enabled) {
            workManager.cancelUniqueWork(UNIQUE_WORK_NAME)
            return
        }

        val request = PeriodicWorkRequestBuilder<GitSyncWorker>(intervalMinutes.toLong(), TimeUnit.MINUTES)
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .build()
        workManager.enqueueUniquePeriodicWork(UNIQUE_WORK_NAME, ExistingPeriodicWorkPolicy.UPDATE, request)
    }

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
    private var pending: Job? = null
    private var lastOpenSyncAt = 0L

    /** Syncs [SYNC_AFTER_CHANGE_SECONDS] after the last of a run of local changes. Each change
     * restarts the wait. The sync waits for a network, so changes made offline go out once the
     * phone is back online. GitSyncWorker skips it when background sync is off. */
    @Synchronized
    fun syncAfterChange(context: Context) {
        val appContext = context.applicationContext
        pending?.cancel()
        pending = scope.launch {
            delay(SYNC_AFTER_CHANGE_SECONDS * 1000)
            enqueueSoon(appContext)
        }
    }

    /** Syncs when Holder is opened, to bring in what other devices changed; at most once a
     * minute. */
    @Synchronized
    fun syncOnOpen(context: Context, nowMs: Long = System.currentTimeMillis()) {
        if (nowMs - lastOpenSyncAt < SYNC_ON_OPEN_MIN_GAP_MS) return
        lastOpenSyncAt = nowMs
        enqueueSoon(context.applicationContext)
    }

    /** One sync as soon as there's a network. One already running isn't cancelled: this one runs
     * after it, so a change made during that sync is still sent. */
    private fun enqueueSoon(context: Context) {
        val request = OneTimeWorkRequestBuilder<GitSyncWorker>()
            .setConstraints(Constraints.Builder().setRequiredNetworkType(NetworkType.CONNECTED).build())
            .setInputData(workDataOf(SYNC_SOON_KEY to true))
            .build()
        WorkManager.getInstance(context).enqueueUniqueWork(SOON_WORK_NAME, ExistingWorkPolicy.APPEND_OR_REPLACE, request)
    }
}
