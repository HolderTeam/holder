package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.util.UUID
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.contract.CardMove
import team.holder.android.contract.CardMoveIntent
import team.holder.android.contract.CardUpdate
import team.holder.android.contract.Cards
import team.holder.android.contract.Change
import team.holder.android.contract.InvalidArgumentException
import team.holder.android.contract.InvalidMoveException
import team.holder.android.contract.ListCardsOrder
import team.holder.android.contract.ListCardsParent
import team.holder.android.contract.Locations
import team.holder.android.contract.MilestoneDate
import team.holder.android.contract.Milestones
import team.holder.android.contract.NewCard
import team.holder.android.contract.NewMilestone
import team.holder.android.contract.NewProject
import team.holder.android.contract.NewProjectPrivacyMode
import team.holder.android.contract.NewResource
import team.holder.android.contract.NewStorageLocation
import team.holder.android.contract.NewStorageLocationProvider
import team.holder.android.contract.NotFoundException
import team.holder.android.contract.Projects
import team.holder.android.contract.Resources

/** The generated contract client, through JNI and core's C API, for each kind of value it converts. */
@RunWith(AndroidJUnit4::class)
class ContractClientIntegrationTest {
    private lateinit var dataDir: File

    @Before
    fun setUp() {
        val context = InstrumentationRegistry.getInstrumentation().targetContext
        dataDir = context.cacheDir.resolve("holder-contract-test-${UUID.randomUUID()}")
        check(dataDir.mkdirs()) { "Could not create test data directory: $dataDir" }
        HolderNative.close()
        HolderNative.initialize(
            context = context,
            dataDir = dataDir,
            schemaSql = context.assets.open("schema.sql").bufferedReader().use { it.readText() },
        )
    }

    @After
    fun tearDown() {
        HolderNative.close()
        dataDir.deleteRecursively()
    }

    private fun project(): String = HolderNative.onContract {
        Projects.createProject(it, NewProject(name = "Client ${UUID.randomUUID()}", privacyMode = NewProjectPrivacyMode.PLAIN))
    }.projectId

    @Test
    fun recordsRoundTripWithDefaultsUnicodeAndTags() {
        val projectId = project()
        val created = HolderNative.onContract {
            Cards.createCard(it, projectId, NewCard(title = "Café 🌍", content = "A #idea in 日本語"))
        }
        assertEquals("Café 🌍", created.title)
        assertEquals("A #idea in 日本語", created.content)
        assertEquals(listOf("idea"), created.tags)
        assertEquals("idea", created.tagOccurrences.single().tag)
        assertNull(created.parentCardId)
        assertEquals(created, HolderNative.onContract { Cards.getCard(it, projectId, created.cardId) })
    }

    @Test
    fun partialUpdatesKeepSetAndClear() {
        val projectId = project()
        val parent = HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Parent")) }
        val child = HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Child")) }

        val moved = HolderNative.onContract {
            Cards.updateCard(it, projectId, child.cardId, CardUpdate(parentCardId = Change.Set(parent.cardId)))
        }
        assertEquals(parent.cardId, moved.parentCardId)
        assertEquals("Child", moved.title)

        val renamed = HolderNative.onContract { Cards.updateCard(it, projectId, child.cardId, CardUpdate(title = "Renamed")) }
        assertEquals(parent.cardId, renamed.parentCardId)

        val root = HolderNative.onContract {
            Cards.updateCard(it, projectId, child.cardId, CardUpdate(parentCardId = Change.Clear))
        }
        assertNull(root.parentCardId)
    }

    @Test
    fun pagesTakeEnumAndUnionArgumentsAndFollowCursors() {
        val projectId = project()
        val parent = HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Parent")) }
        repeat(3) { index ->
            HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Child $index", parentCardId = parent.cardId)) }
        }
        val first = HolderNative.onContract {
            Cards.listCards(it, projectId, parent = ListCardsParent.CardId(parent.cardId), order = ListCardsOrder.UPDATED_DESC, limit = 2,
                            includeContent = true)
        }
        assertEquals(2, first.items.size)
        assertTrue(first.items.all { item -> item.parentCardId == parent.cardId && item.content != null })
        val second = HolderNative.onContract {
            Cards.listCards(it, projectId, parent = ListCardsParent.CardId(parent.cardId), order = ListCardsOrder.UPDATED_DESC, limit = 2,
                            cursor = first.nextCursor)
        }
        assertEquals(1, second.items.size)
        assertNull(second.nextCursor)
        val roots = HolderNative.onContract { Cards.listCards(it, projectId, parent = ListCardsParent.Roots) }
        assertEquals(listOf(parent.cardId), roots.items.map { item -> item.cardId })
    }

    @Test
    fun recordEnumsCrossTheBoundary() {
        val projectId = project()
        val target = HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Target")) }
        val card = HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Card")) }
        val moved = HolderNative.onContract {
            Cards.moveCard(it, projectId, card.cardId, CardMove(intent = CardMoveIntent.INTO, targetCardId = target.cardId))
        }
        assertEquals(target.cardId, moved.parentCardId)
    }

    @Test
    fun errorKindsBecomeTheirExceptions() {
        val projectId = project()
        val missing = assertThrows(NotFoundException::class.java) {
            HolderNative.onContract { Cards.getCard(it, projectId, "no-such-card") }
        }
        assertEquals(4040, missing.code)
        val invalid = assertThrows(InvalidArgumentException::class.java) {
            HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "")) }
        }
        assertEquals(4000, invalid.code)
        val card = HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Card")) }
        val move = assertThrows(InvalidMoveException::class.java) {
            HolderNative.onContract {
                Cards.moveCard(it, projectId, card.cardId, CardMove(intent = CardMoveIntent.INTO, targetCardId = card.cardId))
            }
        }
        assertEquals(4220, move.code)
    }

    @Test
    fun mapsOfTextAndOfListsRoundTrip() {
        val projectId = project()
        val metadata = mapOf("author" to listOf("Ada", "Lin"), "year" to listOf("1843"))
        val resource = HolderNative.onContract {
            Resources.createResource(it, projectId, NewResource(type = "book", label = "Notes 📘", metadata = metadata))
        }
        assertEquals(metadata, resource.metadata)
        assertEquals(metadata, HolderNative.onContract { Resources.getResource(it, projectId, resource.resourceId) }.metadata)

        val configuration = mapOf("bucket" to "notes", "region" to "eu")
        val location = HolderNative.onContract {
            Locations.createStorageLocation(it, NewStorageLocation(projectId = projectId, name = "S3",
                                                                   provider = NewStorageLocationProvider.S3_COMPATIBLE,
                                                                   configuration = configuration))
        }
        assertEquals(configuration, location.configuration)
    }

    @Test
    fun listsOfInputRecordsRoundTrip() {
        val projectId = project()
        val card = HolderNative.onContract { Cards.createCard(it, projectId, NewCard(title = "Card")) }
        val dates = listOf(MilestoneDate(role = "start", date = "2026-11-01"),
                           MilestoneDate(role = "due", datetime = "2026-11-02T09:00:00Z"))
        val milestone = HolderNative.onContract {
            Milestones.addCardMilestone(it, projectId, card.cardId, NewMilestone(dates = dates, kind = "deadline"))
        }
        assertEquals(dates.map { date -> date.role }, milestone.dates.map { date -> date.role })
        assertEquals("deadline", milestone.kind)
    }
}
