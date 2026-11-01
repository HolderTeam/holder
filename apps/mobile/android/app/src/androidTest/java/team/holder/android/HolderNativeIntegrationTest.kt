package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.util.UUID
import org.json.JSONArray
import org.json.JSONObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.contract.NewProject
import team.holder.android.contract.NewProjectPrivacyMode

/**
 * Representative Kotlin -> JNI -> C API -> libholder tests.
 *
 * These deliberately cover a small number of end-to-end round trips rather than duplicating the
 * exhaustive holder-core C API suite. Every test uses a unique cache directory and closes the
 * process-wide HolderNative context after each case.
 */
@RunWith(AndroidJUnit4::class)
class HolderNativeIntegrationTest {
    private lateinit var context: android.content.Context
    private lateinit var dataDir: File
    private lateinit var schemaSql: String

    @Before
    fun setUp() {
        context = InstrumentationRegistry.getInstrumentation().targetContext
        dataDir = context.cacheDir.resolve("holder-jni-test-${UUID.randomUUID()}")
        check(dataDir.mkdirs()) { "Could not create test data directory: $dataDir" }
        schemaSql = context.assets.open("schema.sql").bufferedReader().use { it.readText() }
        HolderNative.close()
    }

    @After
    fun tearDown() {
        HolderNative.close()
        dataDir.deleteRecursively()
    }

    @Test
    fun nativeVersion_isAvailableThroughJni() {
        val version = HolderNative.version()

        assertFalse(version.startsWith("native load failed:"))
        assertFalse(version.startsWith("native call failed:"))
        assertTrue(version.isNotBlank())
    }

    @Test
    fun initialize_bootstrapsHomeProjectAndWelcomeCard() {
        val welcome = "# Welcome 🌍\n\nHolder is ready."

        initialize(welcome)

        val projects = HolderNative.listProjects()
        assertEquals(1, projects.size)
        assertEquals("Home", projects.single().name)

        val cards = HolderNative.listCards(projects.single().projectId)
        assertEquals(1, cards.size)
        assertEquals("Welcome 🌍", cards.single().title)
        assertEquals(welcome, HolderNative.getCardContent(projects.single().projectId, cards.single().cardId))
    }

    @Test
    fun projectAndCardRoundTrip_preservesUtf8AndSearchResults() {
        initialize("# Welcome\n\nWelcome")

        val project = HolderNative.createProject("Проект 🚀")
        val title = "Café 日本語 📝"
        val content = "# $title\n\nПривет, мир — résumé."
        val card = HolderNative.createCard(project.projectId, title, content)

        assertEquals(project.projectId, card.projectId)
        assertEquals(title, card.title)
        assertEquals(content, HolderNative.getCardContent(project.projectId, card.cardId))
        assertEquals(card, HolderNative.listCards(project.projectId).single { it.cardId == card.cardId })
        assertEquals(card, HolderNative.getCard(project.projectId, card.cardId))

        val results = HolderNative.searchCards(project.projectId, "résumé")
        assertEquals(1, results.size)
        assertEquals(card.cardId, results.single().cardId)
        assertEquals(title, results.single().title)
        assertNotNull(results.single().snippet)
    }

    @Test
    fun contractCardPage_exposesResponseOptionsAndFilters() {
        initialize("# Welcome")
        val project = HolderNative.createProject("Page proof")
        val parent = HolderNative.createCard(project.projectId, "Parent 🌍", "Parent body #proof")
        val child = HolderNative.createCard(project.projectId, "Child", "Child body", parent.cardId)

        val items = ArrayList<team.holder.android.contract.CardListItem>()
        var cursor: String? = null
        do {
            val page = HolderNative.listCardsPage(
                project.projectId, order = team.holder.android.contract.ListCardsOrder.SORT_KEY_ASC, limit = 1, cursor = cursor,
                includeContent = true, includeChildCount = true,
            )
            items.addAll(page.items)
            cursor = page.nextCursor
        } while (cursor != null)
        assertEquals(setOf(parent.cardId, child.cardId), items.map { it.cardId }.toSet())
        assertEquals("Parent body #proof", items.single { it.cardId == parent.cardId }.content)
        assertEquals(1, items.single { it.cardId == parent.cardId }.childCount)
        assertEquals("Child body", items.single { it.cardId == child.cardId }.content)

        val roots = HolderNative.listCardsPage(project.projectId, parent = team.holder.android.contract.ListCardsParent.Roots)
        assertEquals(listOf(parent.cardId), roots.items.map { it.cardId })
        assertNull(roots.items.single().content)
        assertNull(roots.items.single().childCount)
        val children = HolderNative.listCardsPage(project.projectId, parent = team.holder.android.contract.ListCardsParent.CardId(parent.cardId))
        assertEquals(listOf(child.cardId), children.items.map { it.cardId })
        val tagged = HolderNative.listCardsPage(project.projectId, tag = "proof")
        assertEquals(listOf(parent.cardId), tagged.items.map { it.cardId })
    }

    @Test
    fun cardReferenceResolution_acrossScopes() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("References")
        val first = HolderNative.createCard(project.projectId, "Shared title", "# Shared title")
        val second = HolderNative.createCard(project.projectId, "Shared title", "# Shared title")

        val byId = HolderNative.resolveCardReference(project.projectId, first.cardId) as CardReferenceResolution.Resolved
        assertEquals(first.cardId, byId.card.cardId)
        val byTitle = HolderNative.resolveCardReference(project.projectId, "Shared title") as CardReferenceResolution.Ambiguous
        assertEquals(setOf(first.cardId, second.cardId), byTitle.candidates.map { it.cardId }.toSet())
        assertEquals(CardReferenceResolution.NotFound,
            HolderNative.resolveCardReference(project.projectId, "missing-reference"))

        HolderNative.deleteCard(project.projectId, second.cardId)
        val trashed = HolderNative.resolveCardReference(project.projectId, second.cardId, 1) as CardReferenceResolution.Resolved
        assertEquals(second.cardId, trashed.card.cardId)
        assertNotNull(trashed.card.deletedAt)
    }

    @Test
    fun projectListing_preservesNullableFieldsAndUnicode() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Café 日本語 📝", "plain")

        val projects = HolderNative.listProjects()
        assertEquals(2, projects.size)
        val listed = projects.single { it.projectId == project.projectId }
        assertEquals(project, listed)
        assertEquals("Café 日本語 📝", listed.name)
        assertNull(listed.gitRemoteUrl)
        assertEquals("plain", listed.privacyMode)
    }

    @Test
    fun projectWrites_preservePlainDefault() {
        initialize("# Welcome\n\nWelcome")
        repeat(3) { round ->
            val created = HolderNative.createProject("Project $round 🚀")
            assertEquals("plain", created.privacyMode)
            assertNull(created.gitRemoteUrl)
            assertEquals(created, HolderNative.listProjects().single { it.projectId == created.projectId })

            val renamed = HolderNative.renameProject(created.projectId, "Renamed $round 日本語")
            assertEquals("Renamed $round 日本語", renamed.name)
            assertEquals("plain", renamed.privacyMode)
            assertEquals(renamed, HolderNative.listProjects().single { it.projectId == created.projectId })

            HolderNative.deleteProject(created.projectId)
            assertTrue(HolderNative.listProjects().none { it.projectId == created.projectId })
        }

        val encrypted = HolderNative.createProject("Encrypted 🚀", "encrypted_git")
        assertEquals("encrypted_git", encrypted.privacyMode)
    }

    @Test
    fun newProjectRequest_preservesImportedFields() {
        initialize("# Welcome\n\nWelcome")
        val id = UUID.randomUUID().toString()
        val root = dataDir.resolve("imported-project").absolutePath
        val createdAt = 1_700_000_000L
        val project = HolderNative.createProjectContract(NewProject(
            projectId = id,
            name = "Imported project",
            rootPath = root,
            gitProvider = "example",
            privacyMode = NewProjectPrivacyMode.PLAIN,
            createdAt = createdAt,
            updatedAt = createdAt + 10,
        ))
        assertEquals(id, project.projectId)
        assertEquals(root, project.rootPath)
        assertEquals("example", project.gitProvider)
        assertEquals(createdAt, project.createdAt)
        assertEquals(createdAt + 10, project.updatedAt)
    }

    @Test
    fun recoveryTokenExportAndImport_roundTripThroughJni() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Recovery 🚀", "encrypted_git")
        val pin = "1234-秘密"
        val exported = HolderNative.exportRecoveryToken(project.projectId, pin)
        assertEquals(project.projectId, exported.projectId)
        assertTrue(exported.keyId.isNotEmpty())

        HolderNative.importRecoveryToken(project.projectId, pin, exported.recoveryToken)
        val global = HolderNative.importRecoveryTokenGlobal(pin, exported.recoveryToken)
        assertEquals(project.projectId, global.projectId)
        assertFalse(global.projectCreated)
        assertFalse(global.remoteHintPresent)
        assertEquals("not_attempted", global.pullStatus)
        assertTrue(HolderNative.encryptionCheck(project.projectId).ok)
    }

    @Test
    fun globalRecoveryImport_createsProjectOnFreshDevice() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("New device recovery", "encrypted_git")
        val pin = "9876"
        val token = HolderNative.exportRecoveryToken(project.projectId, pin).recoveryToken
        val originalDataDir = dataDir
        HolderNative.close()
        dataDir = context.cacheDir.resolve("holder-jni-recovery-${UUID.randomUUID()}")
        check(dataDir.mkdirs())
        originalDataDir.deleteRecursively()
        initialize("# Welcome\n\nWelcome")

        val imported = HolderNative.importRecoveryTokenGlobal(pin, token)
        assertEquals(project.projectId, imported.projectId)
        assertTrue(imported.projectCreated)
        assertFalse(imported.remoteHintPresent)
        assertEquals("not_attempted", imported.pullStatus)
        assertTrue(HolderNative.encryptionCheck(project.projectId).ok)
    }

    @Test
    fun projectRemoteUpdate_setsAndClears() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Remote project")
        val url = "https://example.com/holder-remote.git"

        val set = HolderNative.updateProjectGitRemote(project.projectId, url)
        assertEquals(url, set.gitRemoteUrl)
        assertEquals(set, HolderNative.listProjects().single { it.projectId == project.projectId })

        val cleared = HolderNative.updateProjectGitRemote(project.projectId, null)
        assertNull(cleared.gitRemoteUrl)
        assertEquals(cleared, HolderNative.listProjects().single { it.projectId == project.projectId })

        HolderNative.updateProjectGitRemote(project.projectId, url)
        assertNull(HolderNative.updateProjectGitRemote(project.projectId, "").gitRemoteUrl)
        assertThrows(RuntimeException::class.java) {
            HolderNative.updateProjectGitRemote("missing-project", url)
        }
    }

    @Test
    fun encryptionCheck_forPlainAndEncryptedProjects() {
        initialize("# Welcome\n\nWelcome")
        val plain = HolderNative.createProject("Plain check")
        val encrypted = HolderNative.createProject("Encrypted check", "encrypted_git")
        HolderNative.createCard(encrypted.projectId, "Private card", "# Private\n\nCafé 日本語 📝")

        val plainCheck = HolderNative.encryptionCheck(plain.projectId)
        assertEquals("plain", plainCheck.privacyMode)
        assertTrue(plainCheck.ok)
        assertEquals(0, plainCheck.checkedFiles)
        assertTrue(plainCheck.unsafePaths.isEmpty())

        val encryptedCheck = HolderNative.encryptionCheck(encrypted.projectId)
        assertEquals("encrypted_git", encryptedCheck.privacyMode)
        assertTrue(encryptedCheck.ok)
        assertTrue(encryptedCheck.checkedFiles > 0)
        assertEquals(0, encryptedCheck.unsafeFiles)
        assertTrue(encryptedCheck.unsafePaths.isEmpty())

        assertThrows(RuntimeException::class.java) {
            HolderNative.encryptionCheck("missing-project")
        }
    }

    @Test
    fun gitSyncState_beforeAndAfterFailedPush() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Sync state project")
        val fresh = HolderNative.gitSyncStatus(project.projectId)
        assertNull(fresh.lastPushStatus)
        assertNull(fresh.lastPushAt)

        assertEquals("remote_unset", HolderNative.pushGit(project.projectId).status)
        assertEquals("remote_unset", HolderNative.gitSyncStatus(project.projectId).lastPushStatus)

        assertThrows(RuntimeException::class.java) { HolderNative.gitSyncStatus("missing-project") }
    }

    @Test
    fun gitPull_failsWithoutRemoteAndSucceedsWithOne() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Pull project")

        val unset = HolderNative.pullGit(project.projectId)
        assertEquals("failed", unset.status)
        assertEquals(0, unset.conflictsResolved)
        assertThrows(RuntimeException::class.java) { HolderNative.pullGit("missing-project") }

        val repo = dataDir.resolve("projects/pull-project")
        assertTrue(repo.isDirectory)
        HolderNative.updateProjectGitRemote(project.projectId, repo.absolutePath)
        assertEquals("succeeded", HolderNative.pullGit(project.projectId).status)
    }

    @Test
    fun gitPushAndRemoteProbe_reportUnsetAndMissingRemotes() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Git operations")

        val push = HolderNative.pushGit(project.projectId)
        assertEquals("remote_unset", push.status)
        assertEquals(0, push.aheadCount)

        val probe = HolderNative.probeRemoteUrl(project.projectId, dataDir.resolve("missing-remote").absolutePath)
        assertFalse(probe.remoteHasHead)
        assertNotEquals("reachable", probe.status)

        val test = HolderNative.testGitRemote(project.projectId, "main")
        assertEquals("remote_unset", test.status)
        assertEquals("Remote URL is not configured.", test.errorMessage)
    }

    @Test
    fun gitSyncNow_withAndWithoutRemote() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Sync now")
        val untouched = HolderNative.gitSyncNow(project.projectId)
        assertFalse(untouched.pullAttempted)
        assertFalse(untouched.pushAttempted)

        HolderNative.updateProjectGitRemote(project.projectId, dataDir.resolve("projects/sync-now").absolutePath)
        val synced = HolderNative.gitSyncNow(project.projectId)
        assertTrue(synced.pullAttempted)
        assertEquals("succeeded", synced.pullStatus)
        assertTrue(synced.pushAttempted)
        assertNotNull(synced.pushStatus)
    }

    @Test
    fun gitSyncIfDue_followsCadenceAndSkipsProjectsWithoutRemote() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Sync if due")
        val idle = HolderNative.gitSyncIfDue(project.projectId)
        assertFalse(idle.pullAttempted)
        assertFalse(idle.pushAttempted)

        HolderNative.updateProjectGitRemote(project.projectId, dataDir.resolve("projects/sync-if-due").absolutePath)
        assertTrue(HolderNative.gitSyncIfDue(project.projectId, 3600, 3600).pullAttempted)
        val again = HolderNative.gitSyncIfDue(project.projectId, 3600, 3600)
        assertFalse(again.pullAttempted)
        assertFalse(again.pushAttempted)

        assertThrows(RuntimeException::class.java) {
            HolderNative.gitSyncIfDue("no-such-project")
        }
    }

    @Test
    fun cardListing_returnsEveryCardInPlaceOrder() {
        initialize("# Welcome\n\nWelcome")
        val projectId = HolderNative.createProject("Card page").projectId
        val created = List(300) { index -> HolderNative.createCard(projectId, "Card $index 📝", "Body $index") }

        val listed = HolderNative.listCards(projectId)
        assertEquals(created.map { it.cardId }.toSet(), listed.map { it.cardId }.toSet())
        assertEquals(listed.sortedBy { it.sortKey }, listed)
        assertEquals("Card 7 📝", listed.single { it.cardId == created[7].cardId }.title)
    }

    @Test
    fun cardMetadata_respectsProjectScope() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Metadata project")
        val other = HolderNative.createProject("Other project")
        val card = HolderNative.createCard(project.projectId, "Café 日本語 📝",
            "# Café 日本語 📝\n\n" + "body text ".repeat(4_096))

        assertEquals(card, HolderNative.getCard(project.projectId, card.cardId))
        assertThrows(RuntimeException::class.java) { HolderNative.getCard(other.projectId, card.cardId) }
    }

    @Test
    fun cardCreation_preservesUnicodeAndParent() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Card creation")
        val parent = HolderNative.createCard(project.projectId, "Parent", "Parent body")
        val content = "# Café 日本語 📝\n\nПривет 🌍 #tag"

        val created = HolderNative.createCard(project.projectId, "Café 日本語 📝", content, parent.cardId)
        assertEquals(parent.cardId, created.parentCardId)
        assertEquals(created, HolderNative.getCard(project.projectId, created.cardId))
        assertEquals(content, HolderNative.getCardContent(project.projectId, created.cardId))
        assertThrows(RuntimeException::class.java) {
            HolderNative.createCard(project.projectId, "Missing parent", "body", "no-such-parent")
        }
        assertEquals("After error", HolderNative.createCard(project.projectId, "After error", "body").title)
    }

    @Test
    fun cardWrites_preserveEditorSemantics() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Card writes")
        val other = HolderNative.createProject("Other project")
        val card = HolderNative.createCard(project.projectId, "Café 📝", "First body")
        assertThrows(RuntimeException::class.java) {
            HolderNative.updateCard(other.projectId, card.cardId, "Wrong", "Wrong")
        }
        assertThrows(RuntimeException::class.java) {
            HolderNative.deleteCard(other.projectId, card.cardId)
        }
        // An empty title from the editor keeps the card's title.
        assertEquals("Café 📝", HolderNative.updateCard(project.projectId, card.cardId, "", "Привет 🌍").title)
        assertEquals("Привет 🌍", HolderNative.getCardContent(project.projectId, card.cardId))

        repeat(3) { round ->
            val body = "Body $round #tag"
            HolderNative.updateCard(project.projectId, card.cardId, "Café 📝", body)
            assertEquals(body, HolderNative.getCardContent(project.projectId, card.cardId))
            HolderNative.deleteCard(project.projectId, card.cardId)
            assertThrows(RuntimeException::class.java) { HolderNative.getCard(project.projectId, card.cardId) }
            assertEquals("Café 📝", HolderNative.restoreCard(project.projectId, card.cardId).title)
        }
    }

    @Test
    fun tagAndSearchLists_coverEveryCard() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Record lists")
        repeat(8) { index ->
            HolderNative.createCard(project.projectId, "Résumé $index 📝", "A résumé about #urgent and #android $index")
        }
        assertEquals(setOf(HolderTagCount("android", 8), HolderTagCount("urgent", 8)),
            HolderNative.listProjectTags(project.projectId).toSet())
        val results = HolderNative.searchCards(project.projectId, "résumé")
        assertEquals(8, results.size)
        assertTrue(results.all { it.title.startsWith("Résumé ") })
    }

    @Test
    fun cardListing_followsCursorPastTheFirstPage() {
        initialize("# Welcome\n\nWelcome")
        val cards = JSONArray()
        repeat(5_001) { index ->
            cards.put(JSONObject()
                .put("card_id", "seed-$index")
                .put("title", if (index == 5_000) "Café 日本語 📝" else "Card $index")
                .put("body", "Body $index")
                .put("created_at", index)
                .put("updated_at", index))
        }
        val project = HolderNative.backupRestore("Paged cards", "plain", cards.toString(), "Seed card page")

        val listed = HolderNative.listCards(project.projectId)
        assertEquals(5_001, listed.size)
        assertEquals(5_001, listed.map { it.cardId }.toSet().size)
        assertEquals(List(5_000) { "Card $it" }.toSet() + "Café 日本語 📝", listed.map { it.title }.toSet())
        assertEquals(1, listed.count { it.title == "Café 日本語 📝" })
        assertEquals(listed, HolderNative.listCards(project.projectId))
    }

    @Test
    fun cardListing_errorLeavesReadLaneUsable() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.listProjects().single()

        assertThrows(RuntimeException::class.java) { HolderNative.listCards("missing-project") }
        assertEquals(1, HolderNative.listCards(project.projectId).size)
    }

    @Test
    fun closeAndReopen_preservesProjectAndCardData() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Persistent project")
        val card = HolderNative.createCard(project.projectId, "Persistent card", "# Persistent card\n\nBody")

        HolderNative.close()
        initialize("# Welcome\n\nWelcome")

        assertEquals(project, HolderNative.listProjects().single { it.projectId == project.projectId })
        assertEquals("# Persistent card\n\nBody", HolderNative.getCardContent(project.projectId, card.cardId))
    }

    @Test
    fun databaseRebuild_restoresProjectsAndCardsFromDurableFiles() {
        val welcome = "# Welcome\n\nWelcome"
        initialize(welcome)
        val project = HolderNative.createProject("Rebuild project")
        val card = HolderNative.createCard(
            project.projectId,
            "Rebuilt card",
            "# Rebuilt card\n\nThis body is durable.",
        )

        val dryRun = HolderNative.rebuildDatabase(
            context = context,
            dataDir = dataDir,
            schemaSql = schemaSql,
            welcomeContent = welcome,
            dryRun = true,
        )
        assertTrue(dryRun.getBoolean("dry_run"))
        assertEquals(2, dryRun.getInt("projects"))

        HolderNative.close()
        assertTrue(dataDir.resolve("server/holder.db").delete())

        val rebuilt = HolderNative.rebuildDatabase(
            context = context,
            dataDir = dataDir,
            schemaSql = schemaSql,
            welcomeContent = welcome,
        )
        assertEquals("missing", rebuilt.getString("previous_health"))
        assertEquals(2, rebuilt.getInt("projects"))
        assertEquals(
            "# Rebuilt card\n\nThis body is durable.",
            HolderNative.getCardContent(project.projectId, card.cardId),
        )
    }

    @Test
    fun initialize_rebuildsEncryptedProjectUsingAndroidKeyring() {
        val welcome = "# Welcome\n\nWelcome"
        initialize(welcome)
        val project = HolderNative.createProject(
            name = "Encrypted rebuild project",
            privacyMode = "encrypted_git",
        )
        val content = "# Private card\n\nThis encrypted body survives reconstruction."
        val card = HolderNative.createCard(project.projectId, "Private card", content)
        assertTrue(HolderNative.encryptionCheck(project.projectId).ok)

        HolderNative.close()
        assertTrue(dataDir.resolve("server/holder.db").delete())

        // initialize() must register the Android keyring before holder_context_open()
        // notices the missing projection and reconstructs the encrypted project.
        initialize(welcome)

        assertEquals("encrypted_git", HolderNative.listProjects()
            .single { it.projectId == project.projectId }
            .privacyMode)
        assertEquals(content, HolderNative.getCardContent(project.projectId, card.cardId))
        assertTrue(HolderNative.encryptionCheck(project.projectId).ok)
    }

    @Test
    fun deletingMissingCard_returnsNativeErrorThroughKotlinBoundary() {
        initialize("# Welcome\n\nWelcome")

        val error = assertThrows(RuntimeException::class.java) {
            HolderNative.deleteCard("project-that-does-not-exist", "card-that-does-not-exist")
        }

        assertTrue(error.message.orEmpty().isNotBlank())
    }

    @Test
    fun trashRestorePurge_roundTripsThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Trash test project")
        val card = HolderNative.createCard(project.projectId, "Doomed", "original content")

        HolderNative.deleteCard(project.projectId, card.cardId)

        assertTrue(HolderNative.listCards(project.projectId).none { it.cardId == card.cardId })
        val trashed = HolderNative.listTrashedCards(project.projectId).single { it.cardId == card.cardId }
        assertNotNull(trashed.deletedAt)
        assertThrows(RuntimeException::class.java) { HolderNative.getCard(project.projectId, card.cardId) }

        val restored = HolderNative.restoreCard(project.projectId, card.cardId)
        assertEquals(card.cardId, restored.cardId)
        assertEquals("original content", HolderNative.getCardContent(project.projectId, card.cardId))
        assertEquals(1, HolderNative.listCards(project.projectId).count { it.cardId == card.cardId })
        assertTrue(HolderNative.listTrashedCards(project.projectId).none { it.cardId == card.cardId })

        HolderNative.deleteCard(project.projectId, card.cardId)
        HolderNative.purgeCard(project.projectId, card.cardId)

        assertTrue(HolderNative.listTrashedCards(project.projectId).none { it.cardId == card.cardId })
        assertThrows(RuntimeException::class.java) { HolderNative.getCardContent(project.projectId, card.cardId) }
    }

    @Test
    fun trashPurge_respectsProjectAndEmptiesOnlyItsCards() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Trash purge")
        val other = HolderNative.createProject("Other trash")
        val first = HolderNative.createCard(project.projectId, "First", "body")
        val second = HolderNative.createCard(project.projectId, "Second", "body")
        val otherCard = HolderNative.createCard(other.projectId, "Other", "body")
        for ((projectId, cardId) in listOf(
            project.projectId to first.cardId,
            project.projectId to second.cardId,
            other.projectId to otherCard.cardId,
        )) HolderNative.deleteCard(projectId, cardId)

        assertThrows(RuntimeException::class.java) {
            HolderNative.purgeCard(other.projectId, first.cardId)
        }
        HolderNative.purgeCard(project.projectId, first.cardId)
        assertEquals(listOf(second.cardId), HolderNative.listTrashedCards(project.projectId).map { it.cardId })
        HolderNative.emptyCardTrash(project.projectId)
        assertTrue(HolderNative.listTrashedCards(project.projectId).isEmpty())
        assertEquals(otherCard.cardId, HolderNative.listTrashedCards(other.projectId).single().cardId)
    }

    @Test
    fun cardLinks_roundTripThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Connections test project")
        val blocked = HolderNative.createCard(project.projectId, "Blocked task", "content")
        val blocking = HolderNative.createCard(project.projectId, "Blocking task", "content")

        val empty = HolderNative.listCardLinks(project.projectId, blocked.cardId)
        assertTrue(empty.outgoing.isEmpty())
        assertTrue(empty.backlinks.isEmpty())

        HolderNative.addCardLink(project.projectId, blocked.cardId, blocking.cardId, "blocked_by", "waiting")
        val outgoing = HolderNative.listCardLinks(project.projectId, blocked.cardId).outgoing
        assertEquals(1, outgoing.size)
        assertEquals("waiting", outgoing.single().label)
        assertEquals(blocking.cardId, outgoing.single().toCardId)
        assertEquals("Blocking task", outgoing.single().toTitle)

        val blockingLinks = HolderNative.listCardLinks(project.projectId, blocking.cardId)
        assertEquals(1, blockingLinks.backlinks.size)
        assertEquals(blocked.cardId, blockingLinks.backlinks.single().fromCardId)
        assertEquals("Blocked task", blockingLinks.backlinks.single().fromTitle)

        HolderNative.removeCardLink(project.projectId, blocked.cardId, blocking.cardId, "blocked_by")
        assertTrue(HolderNative.listCardLinks(project.projectId, blocked.cardId).outgoing.isEmpty())
        assertTrue(HolderNative.listCardLinks(project.projectId, blocking.cardId).backlinks.isEmpty())
    }

    @Test
    fun linkKinds_labelBothEndsAndHumanizeUnknownKinds() {
        initialize("# Welcome\n\nWelcome")
        val kinds = HolderNative.listLinkKinds()
        assertEquals(100, kinds.size)
        assertEquals(HolderLinkKind("ref", "References", "Referenced by"), kinds.single { it.id == "ref" })
        assertEquals("Depends on", HolderNative.linkKindLabel("depends_on", forward = true))
        assertEquals("Required by", HolderNative.linkKindLabel("depends_on", forward = false))
        assertEquals("My custom kind", HolderNative.linkKindLabel("my_custom_kind", forward = false))
    }

    @Test
    fun cardLinks_reflectParentChildHierarchyThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Hierarchy test project")
        val parent = HolderNative.createCard(project.projectId, "Parent card", "content")
        val child = HolderNative.createCard(project.projectId, "Child card", "content", parent.cardId)

        val parentLinks = HolderNative.listCardLinks(project.projectId, parent.cardId)
        assertEquals(null, parentLinks.parent)
        assertEquals(1, parentLinks.children.size)
        assertEquals(child.cardId, parentLinks.children.single().cardId)

        val childLinks = HolderNative.listCardLinks(project.projectId, child.cardId)
        assertEquals(parent.cardId, childLinks.parent?.cardId)
        assertEquals("Parent card", childLinks.parent?.title)
        assertTrue(childLinks.children.isEmpty())
    }

    @Test
    fun tags_roundTripThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Tags test project")
        val card = HolderNative.createCard(project.projectId, "Tagged", "Body mentioning #TODO and #android.")

        assertEquals(listOf("android", "todo"), HolderNative.listCardTags(project.projectId, card.cardId))

        val withTodo = HolderNative.cardsWithTag(project.projectId, "TODO")
        assertEquals(1, withTodo.size)
        assertEquals(card.cardId, withTodo.single().cardId)

        val projectTags = HolderNative.listProjectTags(project.projectId)
        assertEquals(2, projectTags.size)
        assertTrue(projectTags.all { it.count == 1 })

        HolderNative.updateCard(project.projectId, card.cardId, "Tagged", "Only #urgent now.")
        assertEquals(listOf("urgent"), HolderNative.listCardTags(project.projectId, card.cardId))
        assertTrue(HolderNative.cardsWithTag(project.projectId, "todo").isEmpty())
    }

    @Test
    fun tagFilteredCards_pageWithinTheirProject() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Tag pages")
        val other = HolderNative.createProject("Other tags")
        val cards = (1..3).map { index ->
            HolderNative.createCard(project.projectId, "Tagged $index", "Body #TODO")
        }
        HolderNative.createCard(project.projectId, "Untagged", "Body")
        HolderNative.createCard(other.projectId, "Other project tag", "Body #TODO")

        val tagged = HolderNative.cardsWithTag(project.projectId, "todo")
        assertEquals(cards.map { it.cardId }.toSet(), tagged.map { it.cardId }.toSet())
        assertEquals(tagged, HolderNative.cardsWithTagPaged(project.projectId, "TODO", 1))
        assertTrue(HolderNative.cardsWithTag(project.projectId, "missing").isEmpty())
    }

    @Test
    fun milestones_roundTripThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Milestones test project")
        val card = HolderNative.createCard(project.projectId, "Renewal", "content")

        assertTrue(HolderNative.listCardMilestones(project.projectId, card.cardId).isEmpty())

        // An all-day milestone is a calendar date, stored as its midnight UTC.
        val start = 1_799_971_200L
        val added = HolderNative.addCardMilestone(
            projectId = project.projectId,
            cardId = card.cardId,
            startAt = start,
            endAt = null,
            allDay = true,
            kind = "Renewal",
            description = "Car insurance",
        )
        assertEquals(card.cardId, added.cardId)
        assertEquals(start, added.startAt)
        assertNull(added.endAt)
        assertTrue(added.allDay)
        assertEquals("Renewal", added.kind)
        assertEquals("Car insurance", added.description)

        assertEquals(added, HolderNative.listCardMilestones(project.projectId, card.cardId).single())

        val moved = HolderNative.updateCardMilestone(project.projectId, card.cardId, added.milestoneId,
            startAt = start + 86_400, kind = OptionalString.Clear)
        assertEquals(start + 86_400, moved.startAt)
        assertTrue(moved.allDay)
        assertNull(moved.kind)
        assertEquals("Car insurance", moved.description)
        HolderNative.updateCardMilestone(project.projectId, card.cardId, added.milestoneId, startAt = start)

        val inRange = HolderNative.listMilestonesInRange(project.projectId, start - 3600, start + 3600)
        assertEquals(1, inRange.size)
        assertEquals(added.milestoneId, inRange.single().milestoneId)
        assertEquals("Renewal", inRange.single().cardTitle)

        assertTrue(HolderNative.listMilestonesInRange(project.projectId, start + 7200, start + 10_800).isEmpty())

        HolderNative.removeCardMilestone(project.projectId, card.cardId, added.milestoneId)
        assertTrue(HolderNative.listCardMilestones(project.projectId, card.cardId).isEmpty())
        assertTrue(HolderNative.listMilestonesInRange(project.projectId, start - 3600, start + 3600).isEmpty())
    }

    @Test
    fun milestones_supportASpanWithAnEndAndSurviveCardTrash() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Milestone span test project")
        val card = HolderNative.createCard(project.projectId, "Conference", "content")

        val start = 1_800_000_000L
        val end = start + 2 * 24 * 60 * 60
        val added = HolderNative.addCardMilestone(project.projectId, card.cardId, startAt = start, endAt = end)
        assertEquals(end, added.endAt)

        val inRange = HolderNative.listMilestonesInRange(project.projectId, start - 3600, start + 3600)
        assertEquals(1, inRange.size)

        HolderNative.deleteCard(project.projectId, card.cardId)
        assertTrue(HolderNative.listMilestonesInRange(project.projectId, start - 3600, end + 3600).isEmpty())
    }

    @Test
    fun history_roundTripsListCompareAndRestoreThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("History test project")
        val card = HolderNative.createCard(project.projectId, "Knife care", "Sharpen at 15 degrees")

        val firstPage = HolderNative.listCardHistory(project.projectId, card.cardId)
        assertEquals(1, firstPage.entries.size)
        assertEquals("created", firstPage.entries.single().kind)
        assertNotNull(firstPage.headOid)
        val creationOid = firstPage.entries.single().lastOid

        HolderNative.updateCard(project.projectId, card.cardId, "Knife care", "Sharpen at 15-20 degrees")

        val page = HolderNative.listCardHistory(project.projectId, card.cardId)
        assertEquals(2, page.entries.size)
        assertEquals(page.headOid, page.entries.first().lastOid)

        // "Since this version": the creation event's own state through to the current saved
        // version -- from is the state AT creationOid, which exists, not the absent state
        // before it (that's the separate creationComparison case below).
        val sinceComparison = HolderNative.compareCardHistory(
            project.projectId, card.cardId, creationOid, page.headOid!!,
        )
        assertTrue(sinceComparison.from.exists)
        assertEquals("Sharpen at 15 degrees", sinceComparison.from.body)
        assertTrue(sinceComparison.to.exists)
        assertEquals("Sharpen at 15-20 degrees", sinceComparison.to.body)

        // The card's creation event has no earlier version: fromOid omitted entirely.
        val creationComparison = HolderNative.compareCardHistory(
            project.projectId, card.cardId, null, creationOid,
        )
        assertFalse(creationComparison.from.exists)
        assertEquals("", creationComparison.from.oid)
        assertEquals("Sharpen at 15 degrees", creationComparison.to.body)

        // Restoration is a new forward commit -- never a rewrite -- so the pre-restore edit
        // remains its own reachable history entry afterwards.
        val restored = HolderNative.restoreCardHistory(project.projectId, card.cardId, creationOid)
        assertEquals(card.cardId, restored.cardId)
        assertEquals("Sharpen at 15 degrees", HolderNative.getCardContent(project.projectId, card.cardId))

        val afterRestore = HolderNative.listCardHistory(project.projectId, card.cardId)
        assertEquals(3, afterRestore.entries.size)
        assertEquals("restored", afterRestore.entries.first().kind)
    }

    @Test
    fun cardHistory_pagesByCursor() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("History pages")
        val card = HolderNative.createCard(project.projectId, "Résumé", "First version")
        HolderNative.updateCard(project.projectId, card.cardId, "Résumé", "Second version")

        val history = HolderNative.listCardHistory(project.projectId, card.cardId)
        assertEquals(2, history.entries.size)
        val first = HolderNative.listCardHistory(project.projectId, card.cardId, limit = 1)
        assertEquals(history.entries.first().lastOid, first.entries.single().lastOid)
        assertNotNull(first.nextCursor)
        val second = HolderNative.listCardHistory(project.projectId, card.cardId, first.nextCursor, 1)
        assertEquals(history.entries.last().lastOid, second.entries.single().lastOid)
    }

    @Test
    fun cardHistoryComparison_sinceAVersionAndFromCreation() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("History comparisons")
        val card = HolderNative.createCard(project.projectId, "Résumé", "First line\nOld line")
        val createdOid = HolderNative.listCardHistory(project.projectId, card.cardId).entries.single().lastOid
        HolderNative.updateCard(project.projectId, card.cardId, "Résumé", "First line\nNew café line")
        val updatedOid = HolderNative.listCardHistory(project.projectId, card.cardId).entries.first().lastOid

        val since = HolderNative.compareCardHistory(project.projectId, card.cardId, createdOid, updatedOid)
        assertTrue(since.lines.any { it.origin == "+" && it.oldLine == null && it.text.contains("café") })
        assertTrue(since.lines.any { it.origin == "-" && it.newLine == null && it.text.contains("Old line") })

        val creation = HolderNative.compareCardHistory(project.projectId, card.cardId, null, createdOid)
        assertFalse(creation.from.exists)
        assertEquals("", creation.from.oid)
        assertTrue(creation.to.exists)
    }

    @Test
    fun cardHistoryRestore_returnsTheRestoredCard() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("History restore")
        repeat(2) { index ->
            val card = HolderNative.createCard(project.projectId, "Restore $index", "Earlier version")
            val oid = HolderNative.listCardHistory(project.projectId, card.cardId).entries.single().lastOid
            HolderNative.updateCard(project.projectId, card.cardId, "Restore $index", "Later version")

            val restored = HolderNative.restoreCardHistory(project.projectId, card.cardId, oid)
            assertEquals(card.cardId, restored.cardId)
            assertEquals("Restore $index", restored.title)
            assertNull(restored.deletedAt)
            assertEquals("Earlier version", HolderNative.getCardContent(project.projectId, card.cardId))
        }

        val trashed = HolderNative.createCard(project.projectId, "Trashed history", "Body")
        HolderNative.deleteCard(project.projectId, trashed.cardId)
        val trashOid = HolderNative.listCardHistory(project.projectId, trashed.cardId).entries.first().lastOid
        val restoredTrash = HolderNative.restoreCardHistory(project.projectId, trashed.cardId, trashOid)
        assertNotNull(restoredTrash.deletedAt)
        assertEquals(trashed.cardId, restoredTrash.cardId)
    }

    @Test
    fun resourceListing_respectsProjectScope() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Resources")
        val other = HolderNative.createProject("Other resources")
        fun addResource(projectId: String, name: String, body: String): String {
            val card = HolderNative.createCard(projectId, "Attachment host", "Body")
            val locationId = UUID.randomUUID().toString()
            HolderNative.createLocation(locationId, projectId, "Local", "local_directory", emptyMap(),
                System.currentTimeMillis() / 1000)
            val source = File(dataDir, name).apply { writeText(body) }
            return HolderNative.importAsset(projectId, card.cardId, locationId, source.absolutePath).resourceId
        }
        val firstId = addResource(project.projectId, "café.txt", "first attachment")
        val secondId = addResource(project.projectId, "second.txt", "second attachment")
        addResource(other.projectId, "other.txt", "other attachment")

        val listed = HolderNative.listResources(project.projectId)
        assertEquals(setOf(firstId, secondId), listed.map { it.resourceId }.toSet())
        assertTrue(listed.all { it.projectId == project.projectId && it.assets.single().placements.size == 1 })
        assertEquals(listed, HolderNative.listResources(project.projectId))
        assertEquals(other.projectId, HolderNative.listResources(other.projectId).single().projectId)
    }

    private fun initialize(welcomeContent: String) {
        HolderNative.initialize(
            context = context,
            dataDir = dataDir,
            schemaSql = schemaSql,
            welcomeContent = welcomeContent,
        )
    }
}
