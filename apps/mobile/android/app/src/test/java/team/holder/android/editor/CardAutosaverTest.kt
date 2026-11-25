package team.holder.android.editor

import java.io.File
import java.nio.file.Files
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.delay
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test

class CardAutosaverTest {
    private lateinit var dir: File
    private lateinit var store: EditorRecoveryStore
    private lateinit var scope: CoroutineScope
    private val writer = FakeWriter()

    @Before
    fun setUp() {
        dir = Files.createTempDirectory("recovery").toFile()
        store = EditorRecoveryStore(dir)
        scope = CoroutineScope(SupervisorJob() + Dispatchers.Default)
    }

    @After
    fun tearDown() {
        scope.cancel()
        dir.deleteRecursively()
    }

    private fun snap(content: String, blank: Boolean = false) =
        EditorSnapshot(title = content.lineSequence().first(), content = content, blank = blank, cursor = content.length)

    private fun autosaver(
        cardId: String? = "c1",
        autosaveMs: Long = 50,
        recoveryMs: Long = 10,
        retry: List<Long> = listOf(30),
    ) = CardAutosaver(
        scope = scope,
        writer = writer,
        store = store,
        projectId = "p1",
        parentCardId = null,
        initialCardId = cardId,
        initialSaved = null,
        autosaveDelayMs = autosaveMs,
        recoveryDelayMs = recoveryMs,
        retryDelaysMs = retry,
    )

    @Test
    fun editIsSavedAfterThePauseWithoutAnyExplicitSave() = runBlocking {
        val saver = autosaver()
        saver.markSaved(snap("Original"))
        saver.onEdited(snap("Original plus more"))
        delay(300)
        assertEquals(listOf("update c1: Original plus more"), writer.calls)
        assertTrue("recovery copy removed once saved", store.all().isEmpty())
    }

    @Test
    fun recoveryCopyIsWrittenBeforeTheSave() = runBlocking {
        val saver = autosaver(autosaveMs = 10_000)
        saver.markSaved(snap("Original"))
        saver.onEdited(snap("Typed text"))
        delay(150)
        assertTrue(writer.calls.isEmpty())
        val copy = store.read(EditorRecoveryStore.keyFor("p1", "c1"))
        assertEquals("Typed text", copy?.content)
    }

    @Test
    fun writeRecoveryNowIsImmediate() {
        val saver = autosaver(autosaveMs = 10_000, recoveryMs = 10_000)
        saver.markSaved(snap("Original"))
        saver.onEdited(snap("Typed just before the process died"))
        saver.writeRecoveryNow()
        assertEquals("Typed just before the process died", store.all().single().content)
    }

    @Test
    fun unchangedTextIsNotSaved() = runBlocking {
        val saver = autosaver()
        saver.markSaved(snap("Same"))
        saver.onEdited(snap("Same"))
        assertTrue(saver.flush())
        assertTrue(writer.calls.isEmpty())
    }

    @Test
    fun flushSavesImmediatelyAndOnlyOnce() = runBlocking {
        val saver = autosaver(autosaveMs = 10_000)
        saver.markSaved(snap("A"))
        saver.onEdited(snap("AB"))
        assertTrue(saver.flush())
        assertTrue(saver.flush())
        assertEquals(listOf("update c1: AB"), writer.calls)
    }

    @Test
    fun newCardIsCreatedOnceThenUpdated() = runBlocking {
        val saver = autosaver(cardId = null)
        saver.onEdited(snap("First words"))
        assertTrue(saver.flush())
        saver.onEdited(snap("First words and more"))
        assertTrue(saver.flush())
        assertEquals(listOf("create: First words", "update new-1: First words and more"), writer.calls)
        assertEquals("new-1", saver.cardId)
        assertEquals(EditorRecoveryStore.keyFor("p1", "new-1"), saver.recoveryKey)
    }

    @Test
    fun blankNewCardIsNeverCreated() = runBlocking {
        val saver = autosaver(cardId = null)
        saver.onEdited(snap("", blank = true))
        assertTrue(saver.flush())
        assertTrue(writer.calls.isEmpty())
        assertNull(saver.cardId)
    }

    @Test
    fun ensureCreatedCreatesEvenWithNothingTyped() = runBlocking {
        val saver = autosaver(cardId = null)
        assertEquals("new-1", saver.ensureCreated(snap("Untitled", blank = true)))
        assertEquals(listOf("create: Untitled"), writer.calls)
    }

    @Test
    fun failedSaveKeepsTheCopyAndRetries() = runBlocking {
        writer.failNext = 1
        val saver = autosaver(autosaveMs = 10_000)
        saver.markSaved(snap("A"))
        saver.onEdited(snap("Precious"))
        assertFalse(saver.flush())
        assertEquals("Precious", store.all().single().content)
        delay(300) // the retry
        assertEquals(listOf("update c1: Precious"), writer.calls)
        assertTrue(store.all().isEmpty())
    }

    @Test
    fun discardEmptyTrashesACreatedCardAndStopsFurtherSaves() = runBlocking {
        val saver = autosaver(cardId = null)
        saver.onEdited(snap("x"))
        saver.flush()
        saver.discardEmpty()
        saver.onEdited(snap("after discard"))
        assertTrue(saver.flush())
        assertEquals(listOf("create: x", "trash new-1"), writer.calls)
        assertTrue(store.all().isEmpty())
    }

    @Test
    fun restoredRecoveryKeyIsReused() {
        val saver = CardAutosaver(
            scope = scope, writer = writer, store = store, projectId = "p1", parentCardId = null,
            initialCardId = null, initialRecoveryKey = "new_restored", initialSaved = null,
        )
        assertEquals("new_restored", saver.recoveryKey)
    }

    @Test
    fun storeRoundTripsAndListsOldestFirst() {
        store.write(EditorRecoveryStore.Draft("b", "p", null, "parent", "T2", "two", 3, updatedAt = 2))
        store.write(EditorRecoveryStore.Draft("a", "p", "c", null, "T1", "one", 1, updatedAt = 1))
        val all = store.all()
        assertEquals(listOf("a", "b"), all.map { it.key })
        assertEquals("c", all[0].cardId)
        assertNull(all[1].cardId)
        assertEquals("parent", all[1].parentCardId)
        assertEquals(3, all[1].cursor)
    }

    @Test(expected = IllegalArgumentException::class)
    fun storeRejectsPathLikeKeys() {
        store.remove("../escape")
    }

    @Test
    fun replayUpdatesChangedCardsCreatesOrphansAndClearsCopies() = runBlocking {
        store.write(EditorRecoveryStore.Draft("k1", "p", "c-live", null, "Live", "new text", 0, 1))
        store.write(EditorRecoveryStore.Draft("k2", "p", "c-same", null, "Same", "unchanged", 0, 2))
        store.write(EditorRecoveryStore.Draft("k3", "p", null, null, "Fresh", "never created", 0, 3))
        store.write(EditorRecoveryStore.Draft("k4", "p", "c-trashed", null, "Gone", "still wanted", 0, 4))
        val outcomes = RecoveryReplay.replay(store, writer) { _, cardId ->
            when (cardId) {
                "c-live" -> "old text"
                "c-same" -> "unchanged"
                else -> null
            }
        }
        assertEquals(
            listOf("update c-live: new text", "create: never created", "create: still wanted"),
            writer.calls,
        )
        assertTrue(outcomes.all { it.saved })
        assertEquals("new-1", outcomes.single { it.key == "k3" }.cardId)
        assertTrue(store.all().isEmpty())
    }

    @Test
    fun replayKeepsCopiesItCouldNotSave() = runBlocking {
        writer.failNext = 1
        store.write(EditorRecoveryStore.Draft("k1", "p", "c", null, "T", "text", 0, 1))
        val outcomes = RecoveryReplay.replay(store, writer) { _, _ -> "older" }
        assertFalse(outcomes.single().saved)
        assertNotNull(store.read("k1"))
    }

    private class FakeWriter : CardWriter {
        val calls = mutableListOf<String>()
        var failNext = 0
        private var created = 0

        private fun maybeFail() {
            if (failNext > 0) {
                failNext--
                throw IllegalStateException("backend unavailable")
            }
        }

        override suspend fun create(projectId: String, title: String, content: String, parentCardId: String?): String {
            maybeFail()
            calls += "create: $content"
            return "new-${++created}"
        }

        override suspend fun update(projectId: String, cardId: String, title: String, content: String) {
            maybeFail()
            calls += "update $cardId: $content"
        }

        override suspend fun trash(projectId: String, cardId: String) {
            calls += "trash $cardId"
        }
    }
}
