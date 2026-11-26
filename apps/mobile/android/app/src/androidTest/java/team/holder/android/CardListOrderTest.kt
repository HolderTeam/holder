package team.holder.android

import android.os.Build
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithContentDescription
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onFirst
import androidx.compose.ui.test.onNodeWithText
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import java.io.File
import java.util.UUID
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.runBlocking
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.contract.Projects
import team.holder.android.ui.CardListOrder

/** The plain card list's order menu: picking an order reorders the list and is remembered. */
@RunWith(AndroidJUnit4::class)
class CardListOrderTest {
    // Before the activity launches: see NoRestoredBackupRule.
    @get:Rule(order = 0)
    val noRestoredBackup = NoRestoredBackupRule()

    @get:Rule(order = 1)
    val composeRule = createAndroidComposeRule<MainActivity>()

    private val timeoutMs = if (Build.VERSION.SDK_INT <= 28) 300_000L else 60_000L
    private val run = UUID.randomUUID().toString().take(8)
    private lateinit var project: HolderProject

    @Before
    fun setUp() {
        awaitContentDescription("Projects")
        runBlocking {
            HolderSettings.setBoardViewEnabled(composeRule.activity, false)
            HolderSettings.setCardListOrder(composeRule.activity, CardListOrder.LAST_MODIFIED)
        }
        project = HolderNative.createProject("Order $run", "plain")
        // Made in this order, so last modified first is Apple, Cherry, Banana, or Banana, Cherry,
        // Apple if they were all made in the same second (ties fall back to card ID). Neither is A to Z.
        for (title in listOf("Banana $run", "Cherry $run", "Apple $run")) {
            HolderNative.createCard(project.projectId, title, "# $title")
        }
    }

    @After
    fun tearDown() {
        runCatching { runBlocking { HolderSettings.setCardListOrder(composeRule.activity, CardListOrder.LAST_MODIFIED) } }
        // Removing a project keeps its folder, and this test runs in the app's real store.
        runCatching {
            val root = HolderNative.onContract { Projects.getProject(it, project.projectId) }.rootPath
            HolderNative.deleteProject(project.projectId)
            File(root).deleteRecursively()
        }
    }

    @Test
    fun choosingTitleOrdersTheListAToZAndIsRemembered() {
        awaitContentDescription("Projects")
        composeRule.onAllNodesWithContentDescription("Projects").onFirst().performClick()
        awaitText("Order $run")
        composeRule.onAllNodesWithText("Order $run").onFirst().performClick()
        awaitText("Cherry $run")
        assertNotEquals(listOf("Apple", "Banana", "Cherry"), shownOrder())

        composeRule.onAllNodesWithContentDescription("Order: Last modified").onFirst().performClick()
        awaitText("Title")
        composeRule.onNodeWithText("Title").performClick()

        awaitContentDescription("Order: Title")
        composeRule.waitUntil(timeoutMs) { shownOrder() == listOf("Apple", "Banana", "Cherry") }
        assertEquals(CardListOrder.TITLE, runBlocking { HolderSettings.cardListOrder(composeRule.activity).first() })
    }

    /** The test's cards, top to bottom as shown. */
    private fun shownOrder(): List<String> =
        listOf("Apple", "Banana", "Cherry")
            .map { it to composeRule.onAllNodesWithText("$it $run").fetchSemanticsNodes().first().boundsInRoot.top }
            .sortedBy { it.second }
            .map { it.first }

    private fun awaitText(text: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithText(text).fetchSemanticsNodes().isNotEmpty()
    }

    private fun awaitContentDescription(description: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithContentDescription(description).fetchSemanticsNodes().isNotEmpty()
    }
}
