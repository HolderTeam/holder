package team.holder.android.editor

import android.content.Context
import java.io.File
import java.util.UUID
import org.json.JSONObject

/**
 * Device-local copies of editor text that may not have reached the project yet (issue #447).
 *
 * The editor writes one of these on every change ([CardAutosaver.onEdited]) and
 * removes it once the same text has been saved into the card. Anything still here when the app
 * starts is text Android took away before it could be saved -- [RecoveryReplay] saves it then.
 *
 * Mirrors the desktop's editor recovery drafts. Kept in app-private storage for every project,
 * encrypted ones included, by decision: losing a note is worse than a private plaintext copy
 * that only lives until the next successful save.
 *
 * Not project state (see "flat files are the source of truth"): these are transient copies of
 * typing in flight; the card file is still the record once saved.
 */
class EditorRecoveryStore(private val dir: File) {

    data class Draft(
        /** File key: [keyFor] for an existing card, [newCardKey] for one not created yet. */
        val key: String,
        val projectId: String,
        /** Null until the card exists (typing into a new card that hasn't been created yet). */
        val cardId: String?,
        val parentCardId: String?,
        val title: String,
        val content: String,
        /** Cursor offset in the editor's body text, so a restored session lands where it was. */
        val cursor: Int,
        val updatedAt: Long,
    )

    /** Writes atomically (temp file + rename), so a kill mid-write leaves the previous copy. */
    fun write(draft: Draft) {
        dir.mkdirs()
        val json = JSONObject()
            .put("projectId", draft.projectId)
            .put("cardId", draft.cardId ?: JSONObject.NULL)
            .put("parentCardId", draft.parentCardId ?: JSONObject.NULL)
            .put("title", draft.title)
            .put("content", draft.content)
            .put("cursor", draft.cursor)
            .put("updatedAt", draft.updatedAt)
        val target = fileFor(draft.key)
        val temp = File(dir, "${target.name}.tmp")
        temp.writeText(json.toString())
        if (!temp.renameTo(target)) {
            target.delete()
            check(temp.renameTo(target)) { "Couldn't move recovery copy into place: $target" }
        }
    }

    fun read(key: String): Draft? = parse(key, fileFor(key))

    fun remove(key: String) {
        fileFor(key).delete()
    }

    /** Every stored copy, oldest first. Unreadable files are skipped, not deleted. */
    fun all(): List<Draft> =
        (dir.listFiles { f -> f.isFile && f.name.endsWith(SUFFIX) } ?: emptyArray())
            .mapNotNull { parse(it.name.removeSuffix(SUFFIX), it) }
            .sortedBy { it.updatedAt }

    private fun parse(key: String, file: File): Draft? = runCatching {
        if (!file.isFile) return null
        val json = JSONObject(file.readText())
        Draft(
            key = key,
            projectId = json.getString("projectId"),
            cardId = json.optString("cardId").takeUnless { json.isNull("cardId") || it.isEmpty() },
            parentCardId = json.optString("parentCardId").takeUnless { json.isNull("parentCardId") || it.isEmpty() },
            title = json.optString("title"),
            content = json.getString("content"),
            cursor = json.optInt("cursor", 0),
            updatedAt = json.optLong("updatedAt", 0L),
        )
    }.getOrNull()

    private fun fileFor(key: String): File {
        require(SAFE_KEY.matches(key)) { "Unsafe recovery key: $key" }
        return File(dir, key + SUFFIX)
    }

    companion object {
        private const val SUFFIX = ".json"
        private val SAFE_KEY = Regex("[A-Za-z0-9_.-]+")

        fun forContext(context: Context) = EditorRecoveryStore(File(context.filesDir, "editor-recovery-drafts"))

        fun keyFor(projectId: String, cardId: String) = "card_${sanitize(projectId)}_${sanitize(cardId)}"

        fun newCardKey() = "new_${UUID.randomUUID()}"

        private fun sanitize(id: String) = id.replace(Regex("[^A-Za-z0-9.-]"), "-")
    }
}
