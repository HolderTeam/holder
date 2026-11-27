package team.holder.android

import android.os.Build
import androidx.compose.ui.test.hasClickAction
import androidx.compose.ui.test.hasText
import androidx.compose.ui.test.onAllNodesWithContentDescription
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onFirst
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import java.io.File
import java.util.UUID
import kotlin.math.abs
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.contract.Projects

/** Issue #106: restoring a card from Trash when its parent is in Trash too, and finding it on the
 * Board where core put it, under the nearest live ancestor. */
@RunWith(AndroidJUnit4::class)
class TrashRestoreBoardTest {
    // Before the activity launches: see NoRestoredBackupRule.
    @get:Rule(order = 0)
    val noRestoredBackup = NoRestoredBackupRule()

    @get:Rule(order = 1)
    val composeRule = holderAndroidComposeRule<MainActivity>()

    private val timeoutMs = if (Build.VERSION.SDK_INT <= 28) 300_000L else 60_000L
    private val run = UUID.randomUUID().toString().take(8)
    private lateinit var project: HolderProject
    private lateinit var ancestor: HolderCard
    private lateinit var child: HolderCard

    @Before
    fun setUp() {
        awaitContentDescription("Projects")
        runBlocking { HolderSettings.setBoardViewEnabled(composeRule.activity, true) }
        project = HolderNative.createProject("Restore $run")
        ancestor = HolderNative.createCard(project.projectId, "Ancestor $run", "# Ancestor $run")
        val parent = HolderNative.createCard(project.projectId, "Parent $run", "# Parent $run", ancestor.cardId)
        child = HolderNative.createCard(project.projectId, "Child $run", "# Child $run", parent.cardId)
        HolderNative.deleteCard(project.projectId, child.cardId)
        HolderNative.deleteCard(project.projectId, parent.cardId)
    }

    @After
    fun tearDown() {
        runCatching { runBlocking { HolderSettings.setBoardViewEnabled(composeRule.activity, false) } }
        // Removing a project keeps its folder, and this test runs in the app's real store.
        runCatching {
            val root = HolderNative.onContract { Projects.getProject(it, project.projectId) }.rootPath
            HolderNative.deleteProject(project.projectId)
            File(root).deleteRecursively()
        }
    }

    @Test
    fun restoringFromTrash_showsTheCardOnTheBoardUnderItsNearestLiveAncestor() {
        composeRule.onAllNodesWithContentDescription("Projects").onFirst().performClick()
        tapText("Restore $run")
        awaitText("Ancestor $run")
        composeRule.onAllNodesWithContentDescription("Trash").onFirst().performClick()
        awaitText("Child $run")
        tapRestoreBeside("Child $run")
        awaitGone("Child $run")
        // Parent is still in Trash.
        awaitText("Parent $run")

        composeRule.onAllNodesWithContentDescription("Back").onFirst().performClick()
        tapText("Ancestor $run")
        awaitText("Child $run")
        assertEquals(ancestor.cardId,
            HolderNative.listCards(project.projectId).single { it.cardId == child.cardId }.parentCardId)
    }

    /** Each Trash row has its own Restore button; tap the one level with [title]. */
    private fun tapRestoreBeside(title: String) {
        val row = composeRule.onAllNodesWithText(title).fetchSemanticsNodes().first().boundsInRoot
        val buttons = composeRule.onAllNodes(hasText("Restore") and hasClickAction())
        val index = buttons.fetchSemanticsNodes().withIndex()
            .minBy { abs(it.value.boundsInRoot.center.y - row.center.y) }.index
        buttons[index].performClick()
    }

    private fun tapText(text: String) {
        awaitText(text)
        composeRule.onAllNodesWithText(text).onFirst().performClick()
    }

    private fun awaitText(text: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithText(text).fetchSemanticsNodes().isNotEmpty()
    }

    private fun awaitGone(text: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithText(text).fetchSemanticsNodes().isEmpty()
    }

    private fun awaitContentDescription(description: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithContentDescription(description).fetchSemanticsNodes().isNotEmpty()
    }
}
