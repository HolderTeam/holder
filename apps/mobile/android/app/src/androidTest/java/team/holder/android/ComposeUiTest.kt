package team.holder.android

import androidx.compose.ui.test.assertIsDisplayed
import androidx.compose.ui.test.hasSetTextAction
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onNodeWithContentDescription
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.compose.ui.test.performScrollTo
import androidx.compose.ui.test.performTextReplacement
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.util.UUID
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.git.github.GitHubConnectionCoordinator
import team.holder.android.ui.markdown.HolderMarkdownViewer
import team.holder.android.ui.screens.CardEditScreen
import team.holder.android.ui.screens.DiagnosticsSettingsScreen
import team.holder.android.ui.screens.SettingsScreen
import team.holder.android.ui.screens.StorageSettingsScreen
import team.holder.android.ui.screens.SyncSettingsScreen

@RunWith(AndroidJUnit4::class)
class ComposeUiTest {
    @get:Rule
    val composeRule = holderComposeRule()

    // For the couple of waits here that depend on background work (a file read, a failing
    // resource lookup) landing and recomposing, not just on composition. 5s was enough on
    // fast emulators but flaked on the slower API 36 image under CI's software renderer.
    private val backgroundWorkTimeoutMs = 15_000L

    // Never actually invoked by these render-only tests (nothing here taps "Connect GitHub") --
    // just enough to satisfy SyncSettingsScreen/RecoverProjectScreen's real signature.
    private val noOpGitHubBrowserLauncher = object : GitHubConnectionCoordinator.GitHubBrowserLauncher {
        override fun resolveBrowser(
            context: android.content.Context,
            authorizationUrl: String,
        ): GitHubConnectionCoordinator.BrowserLaunch? = error("not used by this test")
        override fun launch(
            context: android.content.Context,
            browser: GitHubConnectionCoordinator.BrowserLaunch,
            uri: android.net.Uri,
            attemptId: java.util.UUID,
        ) = error("not used by this test")
    }

    @Test
    fun markdownViewer_rendersHeadingsListsAndWikilinkLabels() {
        composeRule.setContent {
            HolderMarkdownViewer(
                markdown = "# Heading\n\nA paragraph.\n\n- First item\n- Second item\n\n[[Another card]]",
                projectId = "project-1",
                cardId = null,
                onNavigateToCard = { _, _ -> },
                onNavigateToTag = {},
                onCardCreated = { _, _, _ -> },
            )
        }

        composeRule.onNodeWithText("Heading").assertIsDisplayed()
        composeRule.onNodeWithText("A paragraph.").assertIsDisplayed()
        composeRule.onNodeWithText("First item").assertIsDisplayed()
        composeRule.onNodeWithText("Second item").assertIsDisplayed()
        composeRule.onNodeWithText("Another card").assertIsDisplayed()
    }

    @Test
    fun markdownViewer_showsASingleLineBreakAsTyped() {
        // As in the sync run's Charlie card (#535): one line break, no blank line between.
        composeRule.setContent {
            HolderMarkdownViewer(
                markdown = "Sheen\nEdited on the Pixel",
                projectId = "project-1",
                cardId = null,
                onNavigateToCard = { _, _ -> },
                onNavigateToTag = {},
                onCardCreated = { _, _, _ -> },
            )
        }

        composeRule.onNodeWithText("Sheen\nEdited on the Pixel").assertIsDisplayed()
    }

    @Test
    fun markdownViewer_resourceImageReference_fallsBackToAnErrorRatherThanCrashing() {
        // No HolderNative.initialize() in this isolated test -- HolderNative.getResource
        // throws deterministically, exercising rememberResourceAttachmentKind's failure path
        // (and proving a resource-image paragraph doesn't take down the rest of the document
        // with it). The failure happens before the dispatch even knows whether this would
        // have been an image or not, hence "attachment", not "image", in the message.
        composeRule.setContent {
            HolderMarkdownViewer(
                markdown = "Before.\n\n![Holiday photo](holder://resource/some-id)\n\nAfter.",
                projectId = "project-1",
                cardId = null,
                onNavigateToCard = { _, _ -> },
                onNavigateToTag = {},
                onCardCreated = { _, _, _ -> },
            )
        }

        composeRule.onNodeWithText("Before.").assertIsDisplayed()
        composeRule.waitUntil(timeoutMillis = backgroundWorkTimeoutMs) {
            composeRule.onAllNodesWithText("Couldn't load attachment", substring = true).fetchSemanticsNodes().isNotEmpty()
        }
        composeRule.onNodeWithText("After.").assertIsDisplayed()
    }

    @Test
    fun cardEdit_doneWithoutChanges_leavesWithoutWriting() {
        val writer = RecordingWriter()
        var doneWith: String? = "not called"

        composeRule.setContent {
            CardEditScreen(
                screenTitle = "Edit card",
                initialTitle = "Original",
                initialContent = "# Original\n\nBody",
                projectId = "p1",
                cardId = "c1",
                onDone = { doneWith = it },
                writer = writer,
                recoveryStore = tempRecoveryStore(),
            )
        }

        composeRule.onNodeWithContentDescription("Done").performClick()
        composeRule.waitUntil(timeoutMillis = backgroundWorkTimeoutMs) { doneWith != "not called" }

        assertEquals("c1", doneWith)
        assertEquals(emptyList<String>(), writer.calls)
    }

    @Test
    fun cardEdit_changesAreSavedWithoutPressingAnything() {
        val writer = RecordingWriter()

        composeRule.setContent {
            CardEditScreen(
                screenTitle = "Edit card",
                initialTitle = "Original",
                initialContent = "# Original\n\nBody",
                projectId = "p1",
                cardId = "c1",
                onDone = {},
                writer = writer,
                recoveryStore = tempRecoveryStore(),
            )
        }

        composeRule
            .onNode(hasSetTextAction() and androidx.compose.ui.test.hasText("Original"))
            .performTextReplacement("Changed")

        // Autosave fires ~900ms after typing stops; no Done, no Back.
        composeRule.waitUntil(timeoutMillis = backgroundWorkTimeoutMs) {
            writer.calls.any { it.startsWith("update c1") && it.contains("Changed") }
        }
        composeRule.onNodeWithText("Saved").assertIsDisplayed()
    }

    @Test
    fun cardEdit_newCardLeftUntouched_createsNothing() {
        val writer = RecordingWriter()
        var doneWith: String? = "not called"

        composeRule.setContent {
            CardEditScreen(
                screenTitle = "New card",
                initialTitle = "",
                initialContent = "",
                projectId = "p1",
                cardId = null,
                onDone = { doneWith = it },
                writer = writer,
                recoveryStore = tempRecoveryStore(),
            )
        }

        composeRule.onNodeWithContentDescription("Done").performClick()
        composeRule.waitUntil(timeoutMillis = backgroundWorkTimeoutMs) { doneWith != "not called" }

        assertEquals(null, doneWith)
        assertEquals(emptyList<String>(), writer.calls)
    }

    private fun tempRecoveryStore(): team.holder.android.editor.EditorRecoveryStore {
        val dir = java.io.File(
            androidx.test.platform.app.InstrumentationRegistry.getInstrumentation().targetContext.cacheDir,
            "recovery-test-${java.util.UUID.randomUUID()}",
        )
        return team.holder.android.editor.EditorRecoveryStore(dir)
    }

    private class RecordingWriter : team.holder.android.editor.CardWriter {
        val calls = java.util.Collections.synchronizedList(mutableListOf<String>())
        override suspend fun create(projectId: String, title: String, content: String, parentCardId: String?): String {
            calls += "create: $content"
            return "created-1"
        }
        override suspend fun update(projectId: String, cardId: String, title: String, content: String) {
            calls += "update $cardId: $content"
        }
        override suspend fun trash(projectId: String, cardId: String) {
            calls += "trash $cardId"
        }
    }

    @Test
    fun settingsScreen_rendersEveryCategoryRow() {
        composeRule.setContent {
            SettingsScreen(
                onBack = {},
                onAppearanceClick = {},
                onEditorClick = {},
                onBackupClick = {},
                onSyncClick = {},
                onStorageClick = {},
                onDiagnosticsClick = {},
                onAboutClick = {},
                onRecoverProjectClick = {},
            )
        }

        composeRule.onNodeWithText("Appearance").assertIsDisplayed()
        composeRule.onNodeWithText("Editor").assertIsDisplayed()
        composeRule.onNodeWithText("Backup").assertIsDisplayed()
        composeRule.onNodeWithText("Sync").assertIsDisplayed()
        composeRule.onNodeWithText("Storage").assertIsDisplayed()
        composeRule.onNodeWithText("Diagnostics").assertIsDisplayed()
        composeRule.onNodeWithText("About").assertIsDisplayed()
    }

    @Test
    fun storageSettingsScreen_rendersWithoutCrashing() {
        composeRule.setContent {
            StorageSettingsScreen(onBack = {})
        }

        // Not asserting Connect/Disconnect specifically -- Google Drive's connected state is
        // real local DataStore state that can carry over between test runs on the same
        // device. The point of this test is that the whole screen, including the S3 row below
        // it, composes and renders without throwing.
        composeRule.onNodeWithText("Google Drive").assertIsDisplayed()
        composeRule.onNodeWithText("S3-compatible storage").performScrollTo().assertIsDisplayed()
    }

    @Test
    fun syncSettingsScreen_rendersWithoutCrashing() {
        composeRule.setContent {
            SyncSettingsScreen(onBack = {}, browserLauncher = noOpGitHubBrowserLauncher)
        }

        composeRule.onNodeWithText("Automatic background sync").assertIsDisplayed()
        composeRule.onNodeWithText("GitHub").performScrollTo().assertIsDisplayed()
    }

    @Test
    fun diagnosticsSettingsScreen_showsEmptyState() {
        // Its own missing file, not the app's real log: a device that has run the app keeps
        // entries there across test runs, and the screen would rightly list them instead.
        val context = InstrumentationRegistry.getInstrumentation().targetContext
        val logFile = context.cacheDir.resolve("diagnostics-empty-${UUID.randomUUID()}/log.json")
        composeRule.setContent {
            DiagnosticsSettingsScreen(onBack = {}, logFile = logFile)
        }

        // The log file read happens on a background dispatcher the compose test clock doesn't
        // track, unlike the composition itself -- wait for it explicitly rather than racing it.
        composeRule.waitUntil(timeoutMillis = backgroundWorkTimeoutMs) {
            composeRule.onAllNodesWithText("No activity recorded yet.").fetchSemanticsNodes().isNotEmpty()
        }
    }
}
