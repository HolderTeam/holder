package team.holder.android.editor

/**
 * Saves every recovery copy left behind by a previous process (issue #447): text typed into the
 * editor that Android took away before the autosave could put it in the card.
 *
 * Run once at a cold start, before any editor is open. Not on an activity recreated from saved
 * state: there the restored editor still holds the same text and saves it itself, and replaying
 * a not-yet-created card's copy as well would create it twice.
 *
 * The replayed text wins over whatever the card holds now, because it's the user's latest
 * typing. The card's previous version stays in its history.
 */
object RecoveryReplay {

    /** What happened to one copy. [cardId] is where the text now lives, if anywhere. */
    data class Outcome(val key: String, val projectId: String, val cardId: String?, val saved: Boolean)

    /**
     * @param currentContent the card's stored content, or null if it no longer exists (deleted
     *   or trashed), in which case the text is saved as a new card rather than dropped.
     */
    suspend fun replay(
        store: EditorRecoveryStore,
        writer: CardWriter,
        currentContent: suspend (projectId: String, cardId: String) -> String?,
    ): List<Outcome> = store.all().map { draft ->
        val result = runCatching {
            val existing = draft.cardId
            val stored = existing?.let { currentContent(draft.projectId, it) }
            when {
                existing != null && stored != null -> {
                    if (stored != draft.content) writer.update(draft.projectId, existing, draft.title, draft.content)
                    existing
                }
                draft.content.isBlank() && draft.title.isBlank() -> null
                else -> writer.create(draft.projectId, draft.title, draft.content, draft.parentCardId)
            }
        }
        if (result.isSuccess) {
            runCatching { store.remove(draft.key) }
            Outcome(draft.key, draft.projectId, result.getOrNull(), saved = true)
        } else {
            // Left in place for the next start; never dropped on a failure.
            Outcome(draft.key, draft.projectId, draft.cardId, saved = false)
        }
    }
}
