package team.holder.android

import android.os.Build
import androidx.compose.ui.test.SemanticsMatcher
import androidx.compose.ui.test.hasContentDescription
import androidx.compose.ui.test.hasSetTextAction
import androidx.compose.ui.test.hasText
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithContentDescription
import androidx.compose.ui.test.onFirst
import androidx.compose.ui.test.onNodeWithContentDescription
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performTextInput
import androidx.test.ext.junit.runners.AndroidJUnit4
import java.util.UUID
import org.junit.After
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/** Critical user journey through the real Activity, Compose navigation, JNI, and libholder. */
@RunWith(AndroidJUnit4::class)
class WholeAppSmokeTest {
    // Before the activity launches: see NoRestoredBackupRule.
    @get:Rule(order = 0)
    val noRestoredBackup = NoRestoredBackupRule()

    @get:Rule(order = 1)
    val composeRule = createAndroidComposeRule<MainActivity>()

    // Deliberately generous: this test's slow steps each wait on a real libholder round trip
    // (git-backed create/save) rendered by a software-GPU emulator under CI contention. The old
    // 20s budget was where this test's flakiness lived -- it would time out mid-flow on a loaded
    // runner and pass on a rerun; 60s covers that on pixel6Api34/pixel6Api36 (still the aosp
    // image). pixel2Api28 is a separate story: aosp no longer ships an x86_64 image for API 28 at
    // all, so it has to run on the "google" image instead, whose full GMS background stack
    // (Phenotype sync, Play services module scans, a real keyboard, even a live font download)
    // was enough to blow past even 120s, each run stalling at a different point in the flow
    // rather than hanging at the same spot twice -- general resource contention, not a hang. Try
    // brute-force patience there before reaching for anything more drastic (a different image
    // doesn't exist below API 30; see build.gradle.kts's pixel2Api28 comment).
    private val settleTimeoutMs = if (Build.VERSION.SDK_INT <= 28) 300_000L else 60_000L

    private var smokeTitle: String? = null

    @After
    fun tearDown() {
        runCatching {
            val title = smokeTitle ?: return@runCatching
            // Every project, not just Home: quick entry saves to whichever project its chip
            // last picked, which on a device with several projects needn't be Home.
            HolderNative.listProjects().forEach { project ->
                HolderNative.listCards(project.projectId)
                    .filter { it.title == title }
                    .forEach { HolderNative.deleteCard(project.projectId, it.cardId) }
            }
        }
        HolderNative.close()
    }

    @Test
    fun launchHome_createEditSaveAndReopenCard() {
        val title = "Smoke card ${UUID.randomUUID()}"
        smokeTitle = title
        val initialBody = "Created by whole-app smoke test."

        // The app opens on the start screen; the project list is behind its Projects icon.
        awaitContentDescription("Projects")
        composeRule.onNodeWithContentDescription("Projects").performClick()
        awaitText("Home")
        composeRule.onNodeWithText("Home").performClick()

        awaitContentDescription("New card")
        composeRule.onNodeWithContentDescription("New card").performClick()
        awaitTextFields()

        val fields = composeRule.onAllNodes(hasSetTextAction())
        fields[0].performTextInput(title)
        fields[1].performTextInput(initialBody)
        awaitContentDescription("Done")
        composeRule.onNodeWithContentDescription("Done").performClick()

        awaitCardListRow(title)
        composeRule.onNodeWithText(title).performClick()
        awaitText(initialBody)
        awaitContentDescription("Edit")
        composeRule.onNodeWithContentDescription("Edit").performClick()
        awaitTextFields()

        composeRule
            .onNode(hasSetTextAction() and hasText(initialBody))
            .performTextInput("\nEdited and persisted.")
        awaitContentDescription("Done")
        composeRule.onNodeWithContentDescription("Done").performClick()

        // The edited text is in the editor's own field too, so wait for the editor to close:
        // on a slow phone the save keeps it open, and its Back arrow would be tapped below.
        awaitEditorClosed()
        awaitText("Edited and persisted.", substring = true)

        // Click the Back arrow that is on screen, not "the" one or the first: the card sits in a
        // HorizontalPager of its siblings (see CardViewPagerScreen), and a neighbouring page --
        // its own CardViewScreen with its own TopAppBar Back arrow -- can be composed just off
        // screen. Tapping that one does nothing; on the moto g6 plus it was the first match.
        val backOnScreen = hasContentDescription("Back") and isOnScreen()
        composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
            composeRule.onAllNodes(backOnScreen).fetchSemanticsNodes().isNotEmpty()
        }
        composeRule.onAllNodes(backOnScreen).onFirst().performClick()

        awaitCardListRow(title)
        composeRule.onNodeWithText(title).performClick()

        awaitText("Edited and persisted.", substring = true)
        assertTrue(
            composeRule.onAllNodesWithText("Edited and persisted.", substring = true)
                .fetchSemanticsNodes()
                .isNotEmpty(),
        )
    }

    @Test
    fun editedTextReachesTheCardWithoutDone() {
        val title = "Autosave ${UUID.randomUUID()}"
        smokeTitle = title

        awaitContentDescription("Projects")
        composeRule.onNodeWithContentDescription("Projects").performClick()
        awaitText("Home")
        composeRule.onNodeWithText("Home").performClick()
        awaitContentDescription("New card")
        composeRule.onNodeWithContentDescription("New card").performClick()
        awaitTextFields()

        val fields = composeRule.onAllNodes(hasSetTextAction())
        fields[0].performTextInput(title)
        fields[1].performTextInput("Typed and never explicitly saved.")

        // No Done, no Back: the card must exist with the text once autosave has run.
        val home = HolderNative.listProjects().first { it.name == "Home" }
        composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
            HolderNative.listCards(home.projectId).any { card ->
                card.title == title &&
                    HolderNative.getCardContent(home.projectId, card.cardId).contains("Typed and never explicitly saved.")
            }
        }
    }

    @Test
    fun startScreen_quickEntrySavesCardIntoRecent() {
        val title = "Quick entry ${UUID.randomUUID()}"
        smokeTitle = title

        awaitText("Recent")
        composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
            composeRule.onAllNodes(hasSetTextAction()).fetchSemanticsNodes().isNotEmpty()
        }
        composeRule.onAllNodes(hasSetTextAction())[0].performTextInput("$title\nCaptured from the start screen.")
        composeRule.onNodeWithText("Save card").performClick()

        // The new card lands at the top of Recent, and opens in the ordinary card view.
        awaitText(title)
        composeRule.onNodeWithText(title).performClick()
        awaitText("Captured from the start screen.", substring = true)
    }

    private fun awaitText(text: String, substring: Boolean = false) =
        composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
            composeRule.onAllNodesWithText(text, substring = substring).fetchSemanticsNodes().isNotEmpty()
        }

    /** Waits for [title]'s row in the card list, not just the title: after Done (or Back) the
     * editor (or card view) stays on screen until the save and the navigation finish, showing
     * the same title. On a slow phone (Android 9, moto g6 plus) the save outlasts Compose's idle
     * wait, and a tap on "the title" landed on the editor's title field instead of the row.
     * "New card" is only on the list; one match for the title means the other screen is gone. */
    private fun awaitCardListRow(title: String) = composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
        composeRule.onAllNodesWithContentDescription("New card").fetchSemanticsNodes().isNotEmpty() &&
            composeRule.onAllNodesWithText(title).fetchSemanticsNodes().size == 1
    }

    private fun awaitEditorClosed() = composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
        composeRule.onAllNodes(hasSetTextAction()).fetchSemanticsNodes().isEmpty()
    }

    /** Has a visible area inside the window. A pager's neighbouring page is laid out off
     * screen, and its nodes can also report empty (0, 0, 0, 0) bounds once clipped. */
    private fun isOnScreen() = SemanticsMatcher("is on screen") { node ->
        val bounds = node.boundsInWindow
        val width = composeRule.activity.window.decorView.width
        bounds.width > 0f && bounds.height > 0f && bounds.left >= 0f && bounds.right <= width
    }

    private fun awaitContentDescription(description: String) =
        composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
            composeRule.onAllNodesWithContentDescription(description).fetchSemanticsNodes().isNotEmpty()
        }

    private fun awaitTextFields() = composeRule.waitUntil(timeoutMillis = settleTimeoutMs) {
        composeRule.onAllNodes(hasSetTextAction()).fetchSemanticsNodes().size >= 2
    }
}
