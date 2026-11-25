package team.holder.android.editor

import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withContext
import team.holder.android.HolderNative

/** Where the autosaver writes cards. An interface so tests can stand in for libholder. */
interface CardWriter {
    /** Creates the card and returns its id. */
    suspend fun create(projectId: String, title: String, content: String, parentCardId: String?): String
    suspend fun update(projectId: String, cardId: String, title: String, content: String)
    /** Moves the card to the trash (recoverable), as Delete elsewhere in the app does. */
    suspend fun trash(projectId: String, cardId: String)
}

object HolderCardWriter : CardWriter {
    override suspend fun create(projectId: String, title: String, content: String, parentCardId: String?): String =
        withContext(Dispatchers.IO) { HolderNative.createCard(projectId, title, content, parentCardId).cardId }

    override suspend fun update(projectId: String, cardId: String, title: String, content: String) {
        withContext(Dispatchers.IO) { HolderNative.updateCard(projectId, cardId, title, content) }
    }

    override suspend fun trash(projectId: String, cardId: String) {
        withContext(Dispatchers.IO) { HolderNative.deleteCard(projectId, cardId) }
    }
}

/** One moment of the editor: what would be saved if the card were saved now. */
data class EditorSnapshot(
    val title: String,
    val content: String,
    /** True when the user has typed nothing at all (no title, no body). */
    val blank: Boolean,
    val cursor: Int,
)

/**
 * Saves the editor's text without the user asking (issue #447), the same two layers as the
 * desktop's EditorSaveController:
 *
 *  1. A recovery copy in [store], [recoveryDelayMs] after each change. Cheap, local, survives
 *     the process being killed; removed once the same text is in the card.
 *  2. A real save into the card, [autosaveDelayMs] after typing pauses. That's a normal card
 *     update, so card history groups the run of autosaves into one entry and any earlier
 *     version can be restored from there.
 *
 * A new card (null [initialCardId]) is created by its first save that has any text in it.
 * Saves never overlap; a change during a save is picked up by a follow-up save. A failed save
 * keeps the recovery copy and retries with backoff.
 */
class CardAutosaver(
    private val scope: CoroutineScope,
    private val writer: CardWriter,
    private val store: EditorRecoveryStore,
    private val projectId: String,
    private val parentCardId: String?,
    initialCardId: String?,
    /** A key restored from saved state, so a recreated editor keeps using the same copy. */
    initialRecoveryKey: String? = null,
    /** The text as it was when the editor opened; equal snapshots aren't saved. */
    initialSaved: EditorSnapshot?,
    private val onCardCreated: (String) -> Unit = {},
    private val onStatus: (Status) -> Unit = {},
    private val autosaveDelayMs: Long = AUTOSAVE_DELAY_MS,
    private val recoveryDelayMs: Long = RECOVERY_DELAY_MS,
    private val retryDelaysMs: List<Long> = RETRY_DELAYS_MS,
    private val now: () -> Long = System::currentTimeMillis,
) {
    sealed interface Status {
        data object Saved : Status
        data object Pending : Status
        data class Failed(val message: String) : Status
    }

    var cardId: String? = initialCardId
        private set

    private val mutex = Mutex()
    private var latest: EditorSnapshot? = null
    private var lastSaved: EditorSnapshot? = initialSaved
    /** Where this editor's recovery copy lives; changes once a new card is created. */
    var recoveryKey: String = initialRecoveryKey
        ?: initialCardId?.let { EditorRecoveryStore.keyFor(projectId, it) }
        ?: EditorRecoveryStore.newCardKey()
        private set
    private var recoveryJob: Job? = null
    private var saveJob: Job? = null
    private var retryAttempt = 0
    // Set by discardEmpty: the card is gone, so nothing after that may write it again.
    private var discarded = false

    /** Records [snapshot] as what the card already holds, e.g. the text the editor opened with. */
    fun markSaved(snapshot: EditorSnapshot) {
        lastSaved = snapshot
        if (latest == null) latest = snapshot
    }

    /** Call on every change. Schedules the recovery copy and the save. */
    fun onEdited(snapshot: EditorSnapshot) {
        if (discarded || latest == snapshot) return
        latest = snapshot
        if (sameText(snapshot, lastSaved)) return
        if (cardId == null && snapshot.blank) {
            // A new card with nothing typed (or typed and erased again): nothing to keep.
            recoveryJob?.cancel()
            saveJob?.cancel()
            runCatching { store.remove(recoveryKey) }
            return
        }
        onStatus(Status.Pending)
        recoveryJob?.cancel()
        recoveryJob = scope.launch {
            delay(recoveryDelayMs)
            writeRecovery()
        }
        scheduleSave(autosaveDelayMs)
    }

    /**
     * Writes the recovery copy now and saves now, returning once the latest text is in the card
     * (or the save failed). For leaving the editor, and for the app going into the background.
     */
    suspend fun flush(): Boolean {
        recoveryJob?.cancel()
        saveJob?.cancel()
        if (latest?.let { !sameText(it, lastSaved) } == true) writeRecovery()
        return saveNow()
    }

    /**
     * Makes sure the card exists even with nothing typed yet (attaching a photo to a new card
     * needs a card to attach to). Returns its id, or null if creating it failed.
     */
    suspend fun ensureCreated(snapshot: EditorSnapshot): String? {
        cardId?.let { return it }
        latest = snapshot.copy(blank = false)
        flush()
        return cardId
    }

    /** The recovery copy only, synchronously -- for when the process may die any moment. */
    fun writeRecoveryNow() {
        recoveryJob?.cancel()
        if (latest?.let { !sameText(it, lastSaved) } == true) writeRecovery()
    }

    /**
     * The card should go: it was emptied, or it's a new card that ended up with nothing in it.
     * Trashes it if it was ever created, and drops the recovery copy either way.
     */
    suspend fun discardEmpty() {
        recoveryJob?.cancel()
        saveJob?.cancel()
        discarded = true
        withContext(NonCancellable) { mutex.withLock {
            cardId?.let { writer.trash(projectId, it) }
            runCatching { store.remove(recoveryKey) }
        } }
    }

    private fun scheduleSave(delayMs: Long) {
        saveJob?.cancel()
        saveJob = scope.launch {
            delay(delayMs)
            saveNow()
        }
    }

    private fun writeRecovery() {
        if (discarded) return
        val snap = latest ?: return
        runCatching {
            store.write(
                EditorRecoveryStore.Draft(
                    key = recoveryKey,
                    projectId = projectId,
                    cardId = cardId,
                    parentCardId = parentCardId,
                    title = snap.title,
                    content = snap.content,
                    cursor = snap.cursor,
                    updatedAt = now(),
                ),
            )
        }
    }

    // NonCancellable: a pending save is cancelled freely while it's only waiting, but once it
    // has started writing it must finish -- cancelling after createCard returned but before
    // cardId was recorded would make the next save create a second card.
    private suspend fun saveNow(): Boolean = withContext(NonCancellable) { mutex.withLock {
        if (discarded) return@withLock true
        val snap = latest ?: return@withLock true
        if (sameText(snap, lastSaved)) {
            runCatching { store.remove(recoveryKey) }
            onStatus(Status.Saved)
            return@withLock true
        }
        val existing = cardId
        // Nothing typed into a card that doesn't exist yet: nothing to create.
        if (existing == null && snap.blank) return@withLock true
        val result = runCatching {
            if (existing == null) {
                val created = writer.create(projectId, snap.title, snap.content, parentCardId)
                cardId = created
                // The copy so far was keyed as a new card; from now on it's keyed by the card.
                val newKey = EditorRecoveryStore.keyFor(projectId, created)
                runCatching { store.remove(recoveryKey) }
                recoveryKey = newKey
                onCardCreated(created)
            } else {
                writer.update(projectId, existing, snap.title, snap.content)
            }
        }
        if (result.isSuccess) {
            lastSaved = snap
            retryAttempt = 0
            if (latest == snap) {
                runCatching { store.remove(recoveryKey) }
                onStatus(Status.Saved)
            } else {
                // Typing continued during the save: keep the copy current, save again shortly.
                writeRecovery()
                scheduleSave(autosaveDelayMs)
            }
            true
        } else {
            val error = result.exceptionOrNull()
            writeRecovery()
            onStatus(Status.Failed(error?.message ?: error?.let { it::class.java.simpleName } ?: "unknown error"))
            val delayMs = retryDelaysMs[retryAttempt.coerceAtMost(retryDelaysMs.lastIndex)]
            retryAttempt++
            scheduleSave(delayMs)
            false
        }
    } }

    private fun sameText(a: EditorSnapshot, b: EditorSnapshot?) =
        b != null && a.title == b.title && a.content == b.content

    companion object {
        /** Same as the desktop editor. */
        const val AUTOSAVE_DELAY_MS = 900L
        const val RECOVERY_DELAY_MS = 250L
        val RETRY_DELAYS_MS = listOf(2_000L, 5_000L, 15_000L, 30_000L)
    }
}
