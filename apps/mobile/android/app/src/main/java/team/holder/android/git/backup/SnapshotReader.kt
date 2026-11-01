package team.holder.android.git.backup

import org.json.JSONObject
import java.io.File
import java.util.zip.GZIPInputStream

/** One source project's worth of cards read back out of a snapshot file, ready to pass to
 * `HolderNative.restoreRecoverySnapshot` -- see [team.holder.android.ui.screens.RestoreBackupScreen]. */
data class SnapshotGroup(
    val projectName: String,
    val privacyMode: String,
    val cardCount: Int,
    /** The group's cards in core's recovery snapshot encoding, one per line, as
     * [SnapshotWriter] got them from core: what core restores. */
    val snapshot: String,
)

/** The inverse of [SnapshotWriter]: reads a gzip-JSONL snapshot file back into one
 * [SnapshotGroup] per original project. */
object SnapshotReader {
    /** True if [destination] (see [snapshotFile]) exists and looks non-empty -- the cheap
     * check to run before doing the real work of reading it. */
    fun hasSnapshot(destination: File): Boolean = destination.isFile && destination.length() > 0

    /**
     * Reads [source] (gzip JSONL -- the format [SnapshotWriter.writeLines] produces) and
     * groups its lines back into one [SnapshotGroup] per original `project_id`, in first-seen
     * order. Grouping works with no separate per-project header record because every line
     * carries its card's `project_id`/`project_name`/`privacy_mode` -- see [snapshotLine]. A line's
     * `project_name`/`privacy_mode` are taken from the first card seen for that `project_id`;
     * later cards in the same group only contribute their own content, not a second identity
     * for the same project.
     *
     * A malformed or incomplete line (valid JSON but missing `project_id` or `line`, or not valid JSON
     * at all -- e.g. the very last line of a snapshot Auto Backup captured mid-write) is
     * skipped, not fatal: a snapshot is a best-effort recovery aid, and one bad line
     * shouldn't sink everything else in it.
     *
     * Returns an empty list, rather than throwing, when [source] doesn't exist at all -- by far
     * the most common way this actually gets called: the manual "Restore from backup" button is
     * an always-available, unconditional entry point, so most real taps on it happen on a device
     * that has never had anything to restore. Without this, that ordinary case surfaced as a
     * raw `open failed: ENOENT (No such file or directory)` -- the internal file path and all --
     * instead of [RestoreBackupScreen]'s own friendly "No backup snapshot found on this device."
     * empty-groups message, which is exactly what this should look like instead.
     */
    fun readGroups(source: File): List<SnapshotGroup> {
        if (!hasSnapshot(source)) return emptyList()

        val order = mutableListOf<String>()
        val cardsByProject = mutableMapOf<String, MutableList<String>>()
        val nameByProject = mutableMapOf<String, String>()
        val privacyByProject = mutableMapOf<String, String>()

        GZIPInputStream(source.inputStream()).bufferedReader(Charsets.UTF_8).useLines { lines ->
            for (line in lines) {
                if (line.isBlank()) continue
                val card = runCatching { JSONObject(line) }.getOrNull() ?: continue
                val projectId = card.optString("project_id").takeIf { it.isNotBlank() } ?: continue
                val coreLine = card.optString("line").takeIf { it.isNotBlank() } ?: continue

                val cards = cardsByProject.getOrPut(projectId) {
                    order.add(projectId)
                    nameByProject[projectId] = card.optString("project_name").ifBlank { "Restored project" }
                    privacyByProject[projectId] = card.optString("privacy_mode").ifBlank { "plain" }
                    mutableListOf()
                }
                cards.add(coreLine)
            }
        }

        return order.map { projectId ->
            val cards = cardsByProject.getValue(projectId)
            SnapshotGroup(
                projectName = nameByProject.getValue(projectId),
                privacyMode = privacyByProject.getValue(projectId),
                cardCount = cards.size,
                snapshot = cards.joinToString("\n"),
            )
        }
    }
}
