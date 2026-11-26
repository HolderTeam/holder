package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.util.UUID
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.ui.sortKeyOrderedSiblings

/**
 * Issue #106: Android's view of core's hierarchy, move and title behaviour, checked through the
 * real JNI boundary on each device. Core's own suite tests the rules in detail. These check that
 * Android reads back what core decided, in the order Android shows it ([sortKeyOrderedSiblings]),
 * and that it survives reopening and a rebuild of the database from the project files. Projects
 * are encrypted, the default, so rebuilds also go through the Android keyring.
 */
@RunWith(AndroidJUnit4::class)
class CompatibilityBaselineTest {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private lateinit var dataDir: File
    private lateinit var schemaSql: String

    @Before
    fun setUp() {
        dataDir = context.cacheDir.resolve("holder-baseline-${UUID.randomUUID()}")
        check(dataDir.mkdirs()) { "Could not create test data directory: $dataDir" }
        schemaSql = context.assets.open("schema.sql").bufferedReader().use { it.readText() }
        HolderNative.close()
        open()
    }

    @After
    fun tearDown() {
        HolderNative.close()
        dataDir.deleteRecursively()
    }

    @Test
    fun trashingAParent_putsItsChildrenInItsPlace_andRestoringItLeavesThemThere() {
        val project = HolderNative.createProject("Promotion")
        val before = card(project, "Before")
        val parent = card(project, "Parent")
        val after = card(project, "After")
        val child1 = card(project, "Child 1", parent)
        val child2 = card(project, "Child 2", parent)
        val grandchild = card(project, "Grandchild", child1)

        HolderNative.deleteCard(project.projectId, parent.cardId)
        val promoted = listOf(before, child1, child2, after).map { it.cardId }
        assertEquals(promoted, rootOrder(project))
        assertEquals(child1.cardId, parentOf(project, grandchild))

        HolderNative.restoreCard(project.projectId, parent.cardId)
        assertNull(parentOf(project, parent))
        // Restoring Parent doesn't take back the children that took its place.
        assertNull(parentOf(project, child1))
        assertNull(parentOf(project, child2))
        assertEquals(child1.cardId, parentOf(project, grandchild))

        val expected = rootOrder(project)
        assertEquals(5, expected.size)
        assertEquals(promoted, expected.filter { it != parent.cardId })
        reopenAndRebuild {
            assertEquals(expected, rootOrder(project))
            assertEquals(child1.cardId, parentOf(project, grandchild))
        }
    }

    @Test
    fun restoringACardWhoseParentIsInTrash_putsItUnderTheNearestLiveAncestor() {
        val project = HolderNative.createProject("Ancestor restore")
        val ancestor = card(project, "Ancestor")
        val parent = card(project, "Parent", ancestor)
        val child = card(project, "Child", parent)
        HolderNative.deleteCard(project.projectId, child.cardId)
        HolderNative.deleteCard(project.projectId, parent.cardId)

        HolderNative.restoreCard(project.projectId, child.cardId)
        assertEquals(ancestor.cardId, parentOf(project, child))
        // Restoring Parent afterwards doesn't take Child back.
        HolderNative.restoreCard(project.projectId, parent.cardId)
        assertEquals(ancestor.cardId, parentOf(project, child))
        reopenAndRebuild { assertEquals(ancestor.cardId, parentOf(project, child)) }
    }

    @Test
    fun restoringACardWhoseParentWasPurged_putsItAtTheRoot() {
        val project = HolderNative.createProject("Purged parent")
        val ancestor = card(project, "Ancestor")
        val parent = card(project, "Parent", ancestor)
        val child = card(project, "Child", parent)
        HolderNative.deleteCard(project.projectId, child.cardId)
        HolderNative.deleteCard(project.projectId, parent.cardId)
        HolderNative.purgeCard(project.projectId, parent.cardId)

        HolderNative.restoreCard(project.projectId, child.cardId)
        assertNull(parentOf(project, child))
        assertTrue(child.cardId in rootOrder(project))
        reopenAndRebuild { assertNull(parentOf(project, child)) }
    }

    @Test
    fun restoringACardWithNoLiveAncestor_putsItAtTheRoot() {
        val project = HolderNative.createProject("No live ancestor")
        val ancestor = card(project, "Ancestor")
        val parent = card(project, "Parent", ancestor)
        val child = card(project, "Child", parent)
        HolderNative.deleteCard(project.projectId, child.cardId)
        HolderNative.deleteCard(project.projectId, parent.cardId)
        HolderNative.deleteCard(project.projectId, ancestor.cardId)

        HolderNative.restoreCard(project.projectId, child.cardId)
        assertNull(parentOf(project, child))
    }

    @Test
    fun repeatedMovesIntoOneGap_keepTheRequestedOrderThroughReSpacing() {
        val project = HolderNative.createProject("Dense gap")
        // First takes sort key 0, so Before sits at 1. A gap beside 0 never runs out: doubles
        // keep their precision all the way down, so core would never need to re-space.
        val first = card(project, "First")
        val before = card(project, "Before")
        val x = card(project, "X")
        val y = card(project, "Y")
        val after = card(project, "After")
        // Each move halves the gap after Before, until a double can't split it and core re-spaces
        // the siblings. After every move, Android's own order must be the one asked for.
        repeat(60) { i ->
            val (moving, other) = if (i % 2 == 0) x to y else y to x
            HolderNative.moveCard(project.projectId, moving.cardId, CardPlacementIntent.AFTER, targetCardId = before.cardId)
            assertEquals("move $i", listOf(first, before, moving, other, after).map { it.cardId }, rootOrder(project))
        }
        // 60 halvings would leave a gap no double can hold: a wider one means core re-spaced.
        val keys = HolderNative.listCards(project.projectId).associate { it.cardId to it.sortKey }
        assertTrue("sort keys $keys", keys.getValue(y.cardId) - keys.getValue(before.cardId) > 1e-9)
        val expected = listOf(first, before, y, x, after).map { it.cardId }
        reopenAndRebuild { assertEquals(expected, rootOrder(project)) }
    }

    @Test
    fun aTitleOnlyRename_survivesReopeningAndARebuild() {
        val project = HolderNative.createProject("Rename")
        // A body with no heading (as the Desktop and holderctl write), and one whose heading
        // the rename leaves alone.
        val plain = HolderNative.createCard(project.projectId, "Old plain", "Old plain\n\nBody")
        val headed = HolderNative.createCard(project.projectId, "Old headed", "# Old headed\n\nBody")
        HolderNative.updateCard(project.projectId, plain.cardId, "New plain", "Old plain\n\nBody")
        HolderNative.updateCard(project.projectId, headed.cardId, "New headed", "# Old headed\n\nBody")

        fun titles() = HolderNative.listCards(project.projectId).associate { it.cardId to it.title }
        val expected = mapOf(plain.cardId to "New plain", headed.cardId to "New headed")
        assertEquals(expected, titles())
        reopenAndRebuild {
            assertEquals(expected, titles())
            assertEquals("New plain", HolderNative.getCard(project.projectId, plain.cardId).title)
        }
    }

    private fun open() = HolderNative.initialize(context, dataDir, schemaSql)

    private fun card(project: HolderProject, title: String, parent: HolderCard? = null) =
        HolderNative.createCard(project.projectId, title, "# $title", parent?.cardId)

    private fun rootOrder(project: HolderProject) =
        sortKeyOrderedSiblings(null, HolderNative.listCards(project.projectId)).map { it.cardId }

    private fun parentOf(project: HolderProject, card: HolderCard) =
        HolderNative.listCards(project.projectId).single { it.cardId == card.cardId }.parentCardId

    /** Runs [check] after a close and reopen, then again after rebuilding the database from the
     * project files (and the keyring), as a reinstall or a damaged database would. */
    private fun reopenAndRebuild(check: () -> Unit) {
        HolderNative.close()
        open()
        check()
        HolderNative.close()
        assertTrue(dataDir.resolve("server/holder.db").delete())
        HolderNative.rebuildDatabase(context, dataDir, schemaSql)
        check()
    }
}
