package team.holder.android

import android.os.Build
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.onAllNodesWithContentDescription
import androidx.compose.ui.test.onAllNodesWithText
import androidx.compose.ui.test.onFirst
import androidx.compose.ui.test.performClick
import androidx.test.ext.junit.runners.AndroidJUnit4
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith

/** A pull that kept both devices' versions of a card says so while Holder is open, and offers
 * the review list. The pull is stood in for by the signal it publishes. */
@RunWith(AndroidJUnit4::class)
class ConflictNoticeTest {
    // Before the activity launches: see NoRestoredBackupRule.
    @get:Rule(order = 0)
    val noRestoredBackup = NoRestoredBackupRule()

    @get:Rule(order = 1)
    val composeRule = createAndroidComposeRule<MainActivity>()

    private val timeoutMs = if (Build.VERSION.SDK_INT <= 28) 300_000L else 60_000L

    @Test
    fun conflictCopiesFromAPull_offerReview() {
        composeRule.waitUntil(timeoutMs) {
            composeRule.onAllNodesWithContentDescription("Projects").fetchSemanticsNodes().isNotEmpty()
        }

        HolderNative.notifyConflictCopiesArrived("any-project", 2)

        awaitText("2 cards were changed on two devices")
        composeRule.onAllNodesWithText("Review").onFirst().performClick()
        awaitText("Changed on two devices")
    }

    private fun awaitText(text: String) = composeRule.waitUntil(timeoutMs) {
        composeRule.onAllNodesWithText(text, substring = true).fetchSemanticsNodes().isNotEmpty()
    }
}
