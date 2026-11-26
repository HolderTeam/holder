package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import org.json.JSONArray
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.contract.Projects
import team.holder.android.ui.sortKeyOrderedSiblings

/**
 * Issue #106: an operation cut off by the process dying. Driven from the computer by
 * `apps/mobile/android/scripts/interrupt-probe.sh`, one phase per run, chosen by the `interruptPhase`
 * instrumentation argument; skipped in ordinary suite runs.
 *
 * - `prepare`: a fresh encrypted project whose Parent has [CHILDREN] children.
 * - `trash`: trash Parent, which moves every child into its place. The script kills the
 *   process with `kill -9` at a random moment during this.
 * - `verify`: reopen and check that the trash either fully happened or didn't, then rebuild the
 *   database from the project files and check that they agree.
 *
 * The data lives under the app's files directory, so it survives the kill.
 */
@RunWith(AndroidJUnit4::class)
class InterruptedLifecycleProbe {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val phase = InstrumentationRegistry.getArguments().getString("interruptPhase")
    private val dataDir = File(context.filesDir, "holder-interrupt-probe")
    private val stateFile = File(context.filesDir, "holder-interrupt-probe.json")
    private val schemaSql by lazy { context.assets.open("schema.sql").bufferedReader().use { it.readText() } }

    @Test
    fun phase() {
        assumeTrue("Run by scripts/interrupt-probe.sh", phase != null)
        HolderNative.close()
        when (phase) {
            "prepare" -> prepare()
            "trash" -> trash()
            "verify" -> verify()
            else -> error("Unknown interruptPhase $phase")
        }
        HolderNative.close()
    }

    private fun prepare() {
        dataDir.deleteRecursively()
        check(dataDir.mkdirs())
        open()
        val project = HolderNative.createProject("Interrupted")
        val before = HolderNative.createCard(project.projectId, "Before", "# Before")
        val parent = HolderNative.createCard(project.projectId, "Parent", "# Parent")
        val after = HolderNative.createCard(project.projectId, "After", "# After")
        val children = (1..CHILDREN).map {
            HolderNative.createCard(project.projectId, "Child $it", "# Child $it\n\nBody $it", parent.cardId).cardId
        }
        stateFile.writeText(JSONObject()
            .put("project", project.projectId)
            .put("before", before.cardId)
            .put("parent", parent.cardId)
            .put("after", after.cardId)
            .put("children", JSONArray(children))
            .toString())
    }

    private fun trash() {
        val state = JSONObject(stateFile.readText())
        open()
        // The script waits for this line, then kills within the trash itself.
        android.util.Log.i("InterruptProbe", "trash-start")
        val started = System.nanoTime()
        HolderNative.deleteCard(state.getString("project"), state.getString("parent"))
        android.util.Log.i("InterruptProbe", "trash-done ms=${(System.nanoTime() - started) / 1_000_000}")
    }

    private fun verify() {
        val state = JSONObject(stateFile.readText())
        val projectId = state.getString("project")
        val parent = state.getString("parent")
        val children = state.getJSONArray("children").let { a -> List(a.length()) { a.getString(it) } }
        val roots = listOf(state.getString("before"), parent, state.getString("after"))

        open()
        val root = HolderNative.onContract { Projects.getProject(it, projectId) }.rootPath
        // Left behind when the kill landed mid-operation (core's CardMutation journal).
        val journal = if (File(root, ".git/holder-card-mutation").exists()) "pending" else "none"
        // Reading each card's body is what settles an interrupted journal.
        for (id in children) HolderNative.getCardContent(projectId, id)
        val outcome = checkAllOrNothing(projectId, parent, children, roots)
        HolderNative.close()
        assertTrue(dataDir.resolve("server/holder.db").delete())
        HolderNative.rebuildDatabase(context, dataDir, schemaSql)
        assertEquals("the rebuilt database agrees", outcome, checkAllOrNothing(projectId, parent, children, roots))
        // The script greps for this line to count each outcome.
        android.util.Log.i("InterruptProbe", "outcome=$outcome journal=$journal")
    }

    /** "trashed" or "untouched"; fails on anything in between. */
    private fun checkAllOrNothing(projectId: String, parent: String, children: List<String>, roots: List<String>): String {
        val live = HolderNative.listCards(projectId)
        val trashed = HolderNative.listTrashedCards(projectId).map { it.cardId }
        val parentTrashed = parent in trashed
        val rootOrder = sortKeyOrderedSiblings(null, live).map { it.cardId }
        val underParent = sortKeyOrderedSiblings(parent, live).map { it.cardId }
        return if (parentTrashed) {
            assertEquals("children took Parent's place", listOf(roots[0]) + children + listOf(roots[2]), rootOrder)
            assertTrue(underParent.isEmpty())
            "trashed"
        } else {
            assertEquals(roots, rootOrder)
            assertEquals(children, underParent)
            "untouched"
        }
    }

    private fun open() = HolderNative.initialize(context, dataDir, schemaSql)

    private companion object {
        const val CHILDREN = 150
    }
}
