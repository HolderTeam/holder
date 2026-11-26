package team.holder.android

import android.os.Build
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithContentDescription
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onFirst
import androidx.compose.ui.test.onRoot
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import java.util.UUID
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Screens that stay open follow changes another device makes, once a sync brings them in
 * (issue #104). "Another device" here is a direct HolderNative write followed by the same
 * [HolderNative.notifyProjectDataChanged] a pull that moved the project's commit publishes.
 */
@RunWith(AndroidJUnit4::class)
class RefreshAfterSyncTest {
    // Before the activity launches: see NoRestoredBackupRule.
    @get:Rule(order = 0)
    val noRestoredBackup = NoRestoredBackupRule()

    @get:Rule(order = 1)
    val composeRule = createAndroidComposeRule<MainActivity>()

    private val timeoutMs = if (Build.VERSION.SDK_INT <= 28) 300_000L else 60_000L
    private val run = UUID.randomUUID().toString().take(8)
    private lateinit var project: HolderProject
    private lateinit var parent: HolderCard
    private lateinit var a: HolderCard
    private lateinit var b: HolderCard
    private lateinit var c: HolderCard

    @Before
    fun setUp() {
        awaitContentDescription("Projects")
        project = HolderNative.createProject("Sync refresh $run", "plain")
        parent = HolderNative.createCard(project.projectId, "Folder $run", "# Folder $run")
        a = HolderNative.createCard(project.projectId, "Alpha $run", "# Alpha $run\n\nBody of alpha $run", parent.cardId)
        b = HolderNative.createCard(project.projectId, "Bravo $run", "# Bravo $run\n\nBody of bravo $run", parent.cardId)
        c = HolderNative.createCard(project.projectId, "Charlie $run", "# Charlie $run\n\nBody of charlie $run", parent.cardId)
    }

    @After
    fun tearDown() {
        runCatching { setBoardView(false) }
        runCatching { HolderNative.deleteProject(project.projectId) }
    }

    @Test
    fun swipeDeck_staysOnTheSameCard_whenAnEarlierSiblingIsRemovedElsewhere() {
        setBoardView(false)
        openProject()
        tapText("Bravo $run")
        awaitOnScreen("Body of bravo $run")

        // Alpha sat before Bravo in the deck; a deck that kept its page number would now show Charlie.
        HolderNative.deleteCard(project.projectId, a.cardId)
        HolderNative.notifyProjectDataChanged(project.projectId)

        // Alpha was composed as Bravo's neighbour; once it's gone, the deck has been rebuilt.
        awaitGone("Body of alpha $run")
        awaitOnScreen("Body of bravo $run")
    }

    @Test
    fun swipeDeck_movesToTheNextSibling_whenTheShownCardIsTrashedElsewhere() {
        setBoardView(false)
        openProject()
        tapText("Bravo $run")
        awaitOnScreen("Body of bravo $run")

        HolderNative.deleteCard(project.projectId, b.cardId)
        HolderNative.notifyProjectDataChanged(project.projectId)

        awaitOnScreen("Body of charlie $run")
    }

    @Test
    fun swipeDeck_followsTheShownCard_whenItIsMovedUnderAnotherParentElsewhere() {
        setBoardView(false)
        val elsewhere = HolderNative.createCard(project.projectId, "Elsewhere $run", "# Elsewhere $run")
        openProject()
        tapText("Bravo $run")
        awaitOnScreen("Body of bravo $run")

        HolderNative.moveCard(project.projectId, b.cardId, CardPlacementIntent.INTO, targetCardId = elsewhere.cardId)
        HolderNative.notifyProjectDataChanged(project.projectId)

        // Still Bravo, now the only card in its new deck: no sibling either side.
        awaitGone("Body of alpha $run")
        awaitOnScreen("Body of bravo $run")
    }

    @Test
    fun board_leavesAFolderDeletedElsewhere_forTheRoot() {
        setBoardView(true)
        openProject()
        tapText("Folder $run")
        awaitText("Alpha $run")

        // Trashing a parent moves its live children up into its place, as a pulled remote
        // deletion followed by core's hierarchy repair also does.
        HolderNative.deleteCard(project.projectId, parent.cardId)
        HolderNative.notifyProjectDataChanged(project.projectId)

        awaitGone("Folder $run")
        awaitText("Alpha $run")
        awaitText("Charlie $run")
    }

    private fun setBoardView(enabled: Boolean) = runBlocking {
        HolderSettings.setBoardViewEnabled(composeRule.activity, enabled)
    }

    private fun openProject() {
        awaitContentDescription("Projects")
        composeRule.onAllNodesWithContentDescription("Projects").onFirst().performClick()
        tapText("Sync refresh $run")
    }

    private fun tapText(text: String) {
        awaitText(text)
        composeRule.onAllNodesWithText(text).onFirst().performClick()
    }

    private fun awaitText(text: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithText(text, substring = true).fetchSemanticsNodes().isNotEmpty()
    }

    /** On screen, not just composed: the pager also composes the cards either side. */
    private fun awaitOnScreen(text: String) = composeRule.waitUntil(timeoutMs) {
        val width = composeRule.onRoot().fetchSemanticsNode().size.width
        composeRule.onAllNodesWithText(text, substring = true).fetchSemanticsNodes().any { node ->
            val bounds = node.boundsInRoot
            bounds.width > 0f && bounds.left >= 0f && bounds.right <= width + 1f
        }
    }

    private fun awaitGone(text: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithText(text, substring = true).fetchSemanticsNodes().isEmpty()
    }

    private fun awaitContentDescription(description: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithContentDescription(description).fetchSemanticsNodes().isNotEmpty()
    }
}
