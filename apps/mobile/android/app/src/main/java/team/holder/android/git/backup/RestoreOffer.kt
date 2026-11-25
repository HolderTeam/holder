package team.holder.android.git.backup

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * The "we found a backup snapshot, restore it?" offer on a new device -- the automatic half of
 * BACKUP_RESTORE_IMPLEMENTATION_PLAN.md step 9's detection (the "post-reinstall" case: Auto
 * Backup restored `files/backup/snapshot.jsonl.gz` before this install's first launch, while
 * `files/holder/` -- excluded from backup -- starts out empty). The "deliberate" case (a
 * manual "Restore from backup" entry point in Settings) is separate and unconditional.
 *
 * Gated only by [SnapshotProtection.isArmed]: armed solely on a fresh install that found a
 * restored snapshot, never backed up, and disarmed by RestoreBackupScreen once it has read the
 * snapshot -- which is where this offer navigates, so it fires once per restore. It used to be
 * gated by a one-time "offer shown" DataStore flag as well, but that flag was itself backed up
 * and set on every device's first launch, so a replacement phone always restored it as "already
 * shown" and the offer never appeared exactly when it mattered (found while checking #355).
 *
 * `HolderNative.initialize()` always creates a default "Home" welcome project on a project-
 * less device (see `ensure_home_project`), so this can't gate on "zero local projects".
 *
 * Checks the marker rather than re-reading [snapshotFile] itself: this runs from a
 * Composable's LaunchedEffect, after MainActivity.onCreate has already called
 * [SnapshotScheduler.reconcile], which can start [SnapshotWorker] regenerating that exact file
 * within milliseconds -- a live re-check here would race it and could see a snapshot
 * [SnapshotWorker] itself just wrote, not a genuine Auto Backup restore.
 * [SnapshotProtection.armIfFreshInstallHasAnUnseenSnapshot] answers the right question (was a
 * snapshot already there before this install's first [HolderNative.initialize] call) at the
 * right moment (synchronously in onCreate, before [SnapshotScheduler.reconcile] runs).
 */
object RestoreOffer {
    /** Call once at startup (see MainActivity). True if Auto Backup restored a snapshot before
     * this install's first launch and nobody has looked at it yet. */
    suspend fun shouldOffer(context: Context): Boolean {
        val appContext = context.applicationContext
        return withContext(Dispatchers.IO) { SnapshotProtection.isArmed(appContext.filesDir) }
    }
}
