package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import android.os.Debug
import android.util.Log
import java.io.File
import java.util.UUID
import org.json.JSONArray
import org.json.JSONObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertThrows
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.generated.ContractNewProject

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
        assertEquals(HolderNative.getCardContentJsonForBenchmark(card.cardId),
            HolderNative.getCardContent(project.projectId, card.cardId))
        assertEquals(card, HolderNative.listCards(project.projectId).single { it.cardId == card.cardId })
        assertEquals(card, HolderNative.getCard(project.projectId, card.cardId))

        val results = HolderNative.searchCards(project.projectId, "résumé")
        assertEquals(1, results.size)
        assertEquals(card.cardId, results.single().cardId)
        assertEquals(title, results.single().title)
        assertNotNull(results.single().snippet)
    }

    @Test
    fun generatedProjectListing_matchesJsonWithNullableFieldsAndUnicode() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Café 日本語 📝", "plain")

        val generated = HolderNative.listProjects()
        assertEquals(HolderNative.listProjectsJsonForBenchmark(), generated)
        assertEquals("Café 日本語 📝", generated.single { it.projectId == project.projectId }.name)
        assertNull(generated.single { it.projectId == project.projectId }.gitRemoteUrl)
        assertEquals(generated, HolderNative.listProjects())

        val generatedTimes = ArrayList<Long>()
        val jsonTimes = ArrayList<Long>()
        fun sample(call: () -> Unit): Long {
            val start = System.nanoTime()
            call()
            return System.nanoTime() - start
        }
        repeat(9) { round ->
            if (round % 2 == 0) {
                generatedTimes.add(sample { HolderNative.listProjects() })
                jsonTimes.add(sample { HolderNative.listProjectsJsonForBenchmark() })
            } else {
                jsonTimes.add(sample { HolderNative.listProjectsJsonForBenchmark() })
                generatedTimes.add(sample { HolderNative.listProjects() })
            }
        }
        val generatedMedian = generatedTimes.sorted()[generatedTimes.size / 2]
        val jsonMedian = jsonTimes.sorted()[jsonTimes.size / 2]
        Log.i("HolderProjectListTiming", "2 projects: generated=${generatedMedian / 1_000_000.0}ms " +
            "json=${jsonMedian / 1_000_000.0}ms")
        assertTrue("Generated project list took over twice the JSON path plus 10ms: $generatedMedian vs $jsonMedian ns",
            generatedMedian <= jsonMedian * 2 + 10_000_000)
    }

    @Test
    fun generatedProjectWrites_matchJsonAndPreservePlainDefault() {
        initialize("# Welcome\n\nWelcome")
        val timings = listOf("create", "rename", "delete").associateWith {
            Pair(ArrayList<Long>(), ArrayList<Long>())
        }
        fun <T> timed(samples: MutableList<Long>, record: Boolean, call: () -> T): T {
            val start = System.nanoTime()
            val result = call()
            if (record) samples.add(System.nanoTime() - start)
            return result
        }
        fun <T> pair(operation: String, round: Int, generated: () -> T, json: () -> T): Pair<T, T> {
            val (generatedTimes, jsonTimes) = timings.getValue(operation)
            val record = round >= 2
            return if (round % 2 == 0) {
                val first = timed(generatedTimes, record, generated)
                val second = timed(jsonTimes, record, json)
                Pair(first, second)
            } else {
                val second = timed(jsonTimes, record, json)
                val first = timed(generatedTimes, record, generated)
                Pair(first, second)
            }
        }
        repeat(9) { round ->
            val (generated, json) = pair("create", round,
                { HolderNative.createProject("Generated $round 🚀") },
                { HolderNative.createProjectJsonForBenchmark("JSON $round 🚀") })
            assertEquals("plain", generated.privacyMode)
            assertEquals(json.privacyMode, generated.privacyMode)
            assertNull(generated.gitRemoteUrl)
            assertEquals(generated, HolderNative.listProjects().single { it.projectId == generated.projectId })

            val (renamed, jsonRenamed) = pair("rename", round,
                { HolderNative.renameProject(generated.projectId, "Renamed $round 日本語") },
                { HolderNative.renameProjectJsonForBenchmark(json.projectId, "Renamed $round 日本語") })
            assertEquals(jsonRenamed.name, renamed.name)
            assertEquals(jsonRenamed.privacyMode, renamed.privacyMode)
            assertEquals(renamed, HolderNative.listProjects().single { it.projectId == generated.projectId })

            pair("delete", round,
                { HolderNative.deleteProject(generated.projectId) },
                { HolderNative.deleteProjectJsonForBenchmark(json.projectId) })
            assertTrue(HolderNative.listProjects().none {
                it.projectId == generated.projectId || it.projectId == json.projectId
            })
        }
        fun median(samples: List<Long>) = samples.sorted()[samples.size / 2]
        timings.forEach { (operation, samples) ->
            Log.i("HolderProjectWriteTiming", "$operation generated=${median(samples.first) / 1_000_000.0}ms " +
                "JSON=${median(samples.second) / 1_000_000.0}ms")
        }

        val encrypted = HolderNative.createProject("Encrypted 🚀", "encrypted_git")
        assertEquals("encrypted_git", encrypted.privacyMode)
    }

    @Test
    fun generatedNewProjectRequest_preservesImportedFields() {
        initialize("# Welcome\n\nWelcome")
        val id = UUID.randomUUID().toString()
        val root = dataDir.resolve("imported-project").absolutePath
        val createdAt = 1_700_000_000L
        val project = HolderNative.createProjectContract(ContractNewProject(
            projectId = id,
            name = "Imported project",
            rootPath = root,
            gitProvider = "example",
            privacyMode = "plain",
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
    fun generatedProjectRemoteUpdate_matchesJsonForSetAndClear() {
        initialize("# Welcome\n\nWelcome")
        val generatedProject = HolderNative.createProject("Remote generated")
        val jsonProject = HolderNative.createProjectJsonForBenchmark("Remote JSON")
        val generatedSet = ArrayList<Long>()
        val jsonSet = ArrayList<Long>()
        val generatedClear = ArrayList<Long>()
        val jsonClear = ArrayList<Long>()
        fun timed(samples: MutableList<Long>, record: Boolean, call: () -> HolderProject): HolderProject {
            val start = System.nanoTime()
            val result = call()
            if (record) samples.add(System.nanoTime() - start)
            return result
        }
        fun pair(round: Int, generatedSamples: MutableList<Long>, jsonSamples: MutableList<Long>,
                 generated: () -> HolderProject, json: () -> HolderProject): Pair<HolderProject, HolderProject> {
            val record = round >= 2
            return if (round % 2 == 0) {
                Pair(timed(generatedSamples, record, generated), timed(jsonSamples, record, json))
            } else {
                val old = timed(jsonSamples, record, json)
                Pair(timed(generatedSamples, record, generated), old)
            }
        }
        repeat(9) { round ->
            val url = "https://example.com/holder-remote-$round.git"
            val (set, jsonSetResult) = pair(round, generatedSet, jsonSet,
                { HolderNative.updateProjectGitRemote(generatedProject.projectId, url) },
                { HolderNative.updateProjectGitRemoteJsonForBenchmark(jsonProject.projectId, url) })
            assertEquals(url, set.gitRemoteUrl)
            assertEquals(jsonSetResult.gitRemoteUrl, set.gitRemoteUrl)
            assertEquals(set, HolderNative.listProjects().single { it.projectId == set.projectId })

            val (cleared, jsonCleared) = pair(round, generatedClear, jsonClear,
                { HolderNative.updateProjectGitRemote(generatedProject.projectId, null) },
                { HolderNative.updateProjectGitRemoteJsonForBenchmark(jsonProject.projectId, null) })
            assertNull(cleared.gitRemoteUrl)
            assertEquals(jsonCleared.gitRemoteUrl, cleared.gitRemoteUrl)
            assertEquals(cleared, HolderNative.listProjects().single { it.projectId == cleared.projectId })
        }
        val url = "https://example.com/holder-empty-clear.git"
        HolderNative.updateProjectGitRemote(generatedProject.projectId, url)
        assertNull(HolderNative.updateProjectGitRemote(generatedProject.projectId, "").gitRemoteUrl)
        assertThrows(RuntimeException::class.java) {
            HolderNative.updateProjectGitRemote("missing-project", url)
        }
        assertNull(HolderNative.updateProjectGitRemote(generatedProject.projectId, null).gitRemoteUrl)

        fun median(samples: List<Long>) = samples.sorted()[samples.size / 2]
        Log.i("HolderProjectRemoteTiming", "set generated=${median(generatedSet) / 1_000_000.0}ms " +
            "JSON=${median(jsonSet) / 1_000_000.0}ms")
        Log.i("HolderProjectRemoteTiming", "clear generated=${median(generatedClear) / 1_000_000.0}ms " +
            "JSON=${median(jsonClear) / 1_000_000.0}ms")
    }

    @Test
    fun generatedEncryptionCheck_matchesJsonForPlainAndEncryptedProjects() {
        initialize("# Welcome\n\nWelcome")
        val plain = HolderNative.createProject("Plain check")
        val encrypted = HolderNative.createProject("Encrypted check", "encrypted_git")
        HolderNative.createCard(encrypted.projectId, "Private card", "# Private\n\nCafé 日本語 📝")

        for ((name, projectId) in listOf("plain" to plain.projectId, "encrypted_git" to encrypted.projectId)) {
            val expected = HolderNative.encryptionCheckJsonForBenchmark(projectId)
            val actual = HolderNative.encryptionCheck(projectId)
            assertEquals(expected, actual)
            assertEquals(name, actual.privacyMode)
            assertTrue(actual.ok)
            if (name == "plain") {
                assertEquals(0, actual.checkedFiles)
                assertTrue(actual.unsafePaths.isEmpty())
            }

            val generatedTimes = ArrayList<Long>()
            val jsonTimes = ArrayList<Long>()
            fun sample(call: () -> EncryptionCheckResult): Long {
                val start = System.nanoTime()
                val result = call()
                val elapsed = System.nanoTime() - start
                assertEquals(expected, result)
                return elapsed
            }
            repeat(9) { round ->
                if (round % 2 == 0) {
                    val generated = sample { HolderNative.encryptionCheck(projectId) }
                    val json = sample { HolderNative.encryptionCheckJsonForBenchmark(projectId) }
                    if (round >= 2) { generatedTimes.add(generated); jsonTimes.add(json) }
                } else {
                    val json = sample { HolderNative.encryptionCheckJsonForBenchmark(projectId) }
                    val generated = sample { HolderNative.encryptionCheck(projectId) }
                    if (round >= 2) { generatedTimes.add(generated); jsonTimes.add(json) }
                }
            }
            fun median(samples: List<Long>) = samples.sorted()[samples.size / 2]
            Log.i("HolderEncryptionTiming", "$name generated=${median(generatedTimes) / 1_000_000.0}ms " +
                "JSON=${median(jsonTimes) / 1_000_000.0}ms")
        }

        assertThrows(RuntimeException::class.java) {
            HolderNative.encryptionCheck("missing-project")
        }
        assertEquals(HolderNative.encryptionCheckJsonForBenchmark(plain.projectId),
            HolderNative.encryptionCheck(plain.projectId))
    }

    @Test
    fun generatedGitSyncState_matchesJsonBeforeAndAfterFailedPush() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Sync state project")
        val fresh = HolderNative.gitSyncStatus(project.projectId)
        assertEquals(HolderNative.gitSyncStatusJsonForBenchmark(project.projectId), fresh)
        assertNull(fresh.lastPushStatus)

        assertEquals("remote_unset", HolderNative.pushGit(project.projectId).status)
        val expected = HolderNative.gitSyncStatusJsonForBenchmark(project.projectId)
        assertEquals("remote_unset", expected.lastPushStatus)
        assertEquals(expected, HolderNative.gitSyncStatus(project.projectId))

        val generatedTimes = ArrayList<Long>()
        val jsonTimes = ArrayList<Long>()
        fun sample(call: () -> GitSyncStatus): Long {
            val start = System.nanoTime()
            val result = call()
            val elapsed = System.nanoTime() - start
            assertEquals(expected, result)
            return elapsed
        }
        repeat(9) { round ->
            if (round % 2 == 0) {
                val generated = sample { HolderNative.gitSyncStatus(project.projectId) }
                val json = sample { HolderNative.gitSyncStatusJsonForBenchmark(project.projectId) }
                if (round >= 2) { generatedTimes.add(generated); jsonTimes.add(json) }
            } else {
                val json = sample { HolderNative.gitSyncStatusJsonForBenchmark(project.projectId) }
                val generated = sample { HolderNative.gitSyncStatus(project.projectId) }
                if (round >= 2) { generatedTimes.add(generated); jsonTimes.add(json) }
            }
        }
        fun median(samples: List<Long>) = samples.sorted()[samples.size / 2]
        Log.i("HolderSyncStateTiming", "generated=${median(generatedTimes) / 1_000_000.0}ms " +
            "JSON=${median(jsonTimes) / 1_000_000.0}ms")

        assertThrows(RuntimeException::class.java) { HolderNative.gitSyncStatus("missing-project") }
        assertEquals(expected, HolderNative.gitSyncStatus(project.projectId))
    }

    @Test
    fun generatedGitPull_matchesJsonWhenRemoteIsUnset() {
        initialize("# Welcome\n\nWelcome")
        val generatedProject = HolderNative.createProject("Generated pull")
        val jsonProject = HolderNative.createProjectJsonForBenchmark("JSON pull")
        val generatedTimes = ArrayList<Long>()
        val jsonTimes = ArrayList<Long>()
        fun sample(call: () -> GitPullResult): Pair<GitPullResult, Long> {
            val start = System.nanoTime()
            val result = call()
            return Pair(result, System.nanoTime() - start)
        }
        repeat(9) { round ->
            val generated: Pair<GitPullResult, Long>
            val json: Pair<GitPullResult, Long>
            if (round % 2 == 0) {
                generated = sample { HolderNative.pullGit(generatedProject.projectId) }
                json = sample { HolderNative.pullGitJsonForBenchmark(jsonProject.projectId) }
            } else {
                json = sample { HolderNative.pullGitJsonForBenchmark(jsonProject.projectId) }
                generated = sample { HolderNative.pullGit(generatedProject.projectId) }
            }
            assertEquals(json.first, generated.first)
            assertEquals("failed", generated.first.status)
            assertEquals(0, generated.first.conflictsResolved)
            if (round >= 2) {
                generatedTimes.add(generated.second)
                jsonTimes.add(json.second)
            }
        }
        fun median(samples: List<Long>) = samples.sorted()[samples.size / 2]
        Log.i("HolderGitPullTiming", "generated=${median(generatedTimes) / 1_000_000.0}ms " +
            "JSON=${median(jsonTimes) / 1_000_000.0}ms")

        assertThrows(RuntimeException::class.java) { HolderNative.pullGit("missing-project") }
        assertEquals(HolderNative.pullGitJsonForBenchmark(jsonProject.projectId),
            HolderNative.pullGit(generatedProject.projectId))

        val generatedRepo = dataDir.resolve("projects/generated-pull")
        val jsonRepo = dataDir.resolve("projects/json-pull")
        assertTrue(generatedRepo.isDirectory)
        assertTrue(jsonRepo.isDirectory)
        HolderNative.updateProjectGitRemote(generatedProject.projectId, generatedRepo.absolutePath)
        HolderNative.updateProjectGitRemote(jsonProject.projectId, jsonRepo.absolutePath)
        val successfulGenerated = HolderNative.pullGit(generatedProject.projectId)
        val successfulJson = HolderNative.pullGitJsonForBenchmark(jsonProject.projectId)
        assertEquals(successfulJson, successfulGenerated)
        assertEquals("succeeded", successfulGenerated.status)
    }

    @Test
    fun generatedGitPushAndRemoteProbe_matchJson() {
        initialize("# Welcome\n\nWelcome")
        val generatedProject = HolderNative.createProject("Generated Git operations")
        val jsonProject = HolderNative.createProjectJsonForBenchmark("JSON Git operations")

        val generatedPush = HolderNative.pushGit(generatedProject.projectId)
        val jsonPush = HolderNative.pushGitJsonForBenchmark(jsonProject.projectId)
        assertEquals(jsonPush, generatedPush)
        assertEquals("remote_unset", generatedPush.status)

        val missingRemote = dataDir.resolve("missing-remote").absolutePath
        val generatedProbe = HolderNative.probeRemoteUrl(generatedProject.projectId, missingRemote)
        val jsonProbe = HolderNative.probeRemoteUrlJsonForBenchmark(jsonProject.projectId, missingRemote)
        assertEquals(jsonProbe, generatedProbe)
        assertFalse(generatedProbe.remoteHasHead)

        assertEquals(HolderNative.testGitRemoteJsonForBenchmark(jsonProject.projectId, "main"),
            HolderNative.testGitRemote(generatedProject.projectId, "main"))
    }

    @Test
    fun generatedGitSyncNow_matchesJsonWithAndWithoutRemote() {
        initialize("# Welcome\n\nWelcome")
        val generatedProject = HolderNative.createProject("Generated sync now")
        val jsonProject = HolderNative.createProjectJsonForBenchmark("JSON sync now")
        assertEquals(HolderNative.gitSyncNowJsonForBenchmark(jsonProject.projectId),
            HolderNative.gitSyncNow(generatedProject.projectId))

        val generatedRepo = dataDir.resolve("projects/generated-sync-now")
        val jsonRepo = dataDir.resolve("projects/json-sync-now")
        HolderNative.updateProjectGitRemote(generatedProject.projectId, generatedRepo.absolutePath)
        HolderNative.updateProjectGitRemote(jsonProject.projectId, jsonRepo.absolutePath)
        for (push in listOf(false, true)) {
            val json = HolderNative.gitSyncNowJsonForBenchmark(jsonProject.projectId, push)
            val generated = HolderNative.gitSyncNow(generatedProject.projectId, push = push)
            assertEquals(json.copy(pushLocalHeadCommit = null),
                generated.copy(pushLocalHeadCommit = null))
            assertEquals(json.pushLocalHeadCommit != null, generated.pushLocalHeadCommit != null)
        }
    }

    @Test
    fun generatedGitSyncIfDue_matchesJsonCadenceAndNoRemote() {
        initialize("# Welcome\n\nWelcome")
        val generatedProject = HolderNative.createProject("Generated sync if due")
        val jsonProject = HolderNative.createProjectJsonForBenchmark("JSON sync if due")
        assertEquals(HolderNative.gitSyncIfDueJsonForBenchmark(jsonProject.projectId),
            HolderNative.gitSyncIfDue(generatedProject.projectId))

        val generatedRepo = dataDir.resolve("projects/generated-sync-if-due")
        val jsonRepo = dataDir.resolve("projects/json-sync-if-due")
        HolderNative.updateProjectGitRemote(generatedProject.projectId, generatedRepo.absolutePath)
        HolderNative.updateProjectGitRemote(jsonProject.projectId, jsonRepo.absolutePath)
        repeat(2) {
            val json = HolderNative.gitSyncIfDueJsonForBenchmark(jsonProject.projectId, 3600, 3600)
            val generated = HolderNative.gitSyncIfDue(generatedProject.projectId, 3600, 3600)
            assertEquals(json, generated)
        }
        assertThrows(RuntimeException::class.java) {
            HolderNative.gitSyncIfDue("no-such-project")
        }
    }

    @Test
    fun generatedCardPage_matchesJsonAndStaysWithinCallBudget() {
        initialize("# Welcome\n\nWelcome")
        val projectId = HolderNative.createProject("Card page timing").projectId
        repeat(300) { index ->
            HolderNative.createCard(projectId, "Card $index 📝", "Body $index")
        }

        assertEquals(HolderNative.listCardsJsonForBenchmark(projectId), HolderNative.listCards(projectId))
        repeat(3) {
            HolderNative.listCards(projectId)
            HolderNative.listCardsJsonForBenchmark(projectId)
        }
        val generated = ArrayList<Long>()
        val json = ArrayList<Long>()
        repeat(9) { round ->
            fun sample(call: () -> Unit): Long {
                val start = System.nanoTime()
                call()
                return System.nanoTime() - start
            }
            if (round % 2 == 0) {
                generated.add(sample { HolderNative.listCards(projectId) })
                json.add(sample { HolderNative.listCardsJsonForBenchmark(projectId) })
            } else {
                json.add(sample { HolderNative.listCardsJsonForBenchmark(projectId) })
                generated.add(sample { HolderNative.listCards(projectId) })
            }
        }
        val generatedMedian = generated.sorted()[generated.size / 2]
        val jsonMedian = json.sorted()[json.size / 2]
        Log.i("HolderCardPageTiming", "300 cards: generated=${generatedMedian / 1_000_000.0}ms " +
            "json=${jsonMedian / 1_000_000.0}ms")
        assertTrue("Generated page took over twice the JSON path plus 10ms: $generatedMedian vs $jsonMedian ns",
            generatedMedian <= jsonMedian * 2 + 10_000_000)
    }

    @Test
    fun generatedCardMetadata_matchesJsonAndRespectsProjectScope() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Metadata project")
        val other = HolderNative.createProject("Other project")
        val card = HolderNative.createCard(project.projectId, "Café 日本語 📝",
            "# Café 日本語 📝\n\n" + "body text ".repeat(4_096))

        assertEquals(HolderNative.getCardJsonForBenchmark(card.cardId),
            HolderNative.getCard(project.projectId, card.cardId))
        assertThrows(RuntimeException::class.java) { HolderNative.getCard(other.projectId, card.cardId) }
        assertEquals(card, HolderNative.getCard(project.projectId, card.cardId))

        val generatedTimes = ArrayList<Long>()
        val jsonTimes = ArrayList<Long>()
        fun sample(call: () -> Unit): Long {
            val start = System.nanoTime()
            call()
            return System.nanoTime() - start
        }
        repeat(3) {
            HolderNative.getCard(project.projectId, card.cardId)
            HolderNative.getCardJsonForBenchmark(card.cardId)
        }
        repeat(9) { round ->
            if (round % 2 == 0) {
                generatedTimes.add(sample { HolderNative.getCard(project.projectId, card.cardId) })
                jsonTimes.add(sample { HolderNative.getCardJsonForBenchmark(card.cardId) })
            } else {
                jsonTimes.add(sample { HolderNative.getCardJsonForBenchmark(card.cardId) })
                generatedTimes.add(sample { HolderNative.getCard(project.projectId, card.cardId) })
            }
        }
        val generatedMedian = generatedTimes.sorted()[generatedTimes.size / 2]
        val jsonMedian = jsonTimes.sorted()[jsonTimes.size / 2]
        Log.i("HolderCardMetadataTiming", "generated=${generatedMedian / 1_000_000.0}ms " +
            "json=${jsonMedian / 1_000_000.0}ms")
        assertTrue("Generated metadata took over twice JSON plus 10ms: $generatedMedian vs $jsonMedian ns",
            generatedMedian <= jsonMedian * 2 + 10_000_000)
    }

    @Test
    fun generatedCardCreation_preservesUnicodeAndStaysWithinCallBudget() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Generated creation")
        val parent = HolderNative.createCardJsonForBenchmark(project.projectId, "Parent", "Parent body")
        val content = "# Café 日本語 📝\n\nПривет 🌍 #tag"

        val created = HolderNative.createCard(project.projectId, "Café 日本語 📝", content, parent.cardId)
        assertEquals(parent.cardId, created.parentCardId)
        assertEquals(HolderNative.getCardJsonForBenchmark(created.cardId), created)
        assertEquals(content, HolderNative.getCardContent(project.projectId, created.cardId))
        assertThrows(RuntimeException::class.java) {
            HolderNative.createCard(project.projectId, "Missing parent", "body", "no-such-parent")
        }
        assertEquals("After error", HolderNative.createCard(project.projectId, "After error", "body").title)

        val generatedTimes = ArrayList<Long>()
        val jsonTimes = ArrayList<Long>()
        fun sample(call: () -> Unit): Long {
            val start = System.nanoTime()
            call()
            return System.nanoTime() - start
        }
        repeat(2) { index ->
            HolderNative.createCard(project.projectId, "Generated warmup $index", "Body $index")
            HolderNative.createCardJsonForBenchmark(project.projectId, "JSON warmup $index", "Body $index")
        }
        repeat(7) { round ->
            if (round % 2 == 0) {
                generatedTimes.add(sample { HolderNative.createCard(project.projectId, "Generated $round", "Body $round") })
                jsonTimes.add(sample { HolderNative.createCardJsonForBenchmark(project.projectId, "JSON $round", "Body $round") })
            } else {
                jsonTimes.add(sample { HolderNative.createCardJsonForBenchmark(project.projectId, "JSON $round", "Body $round") })
                generatedTimes.add(sample { HolderNative.createCard(project.projectId, "Generated $round", "Body $round") })
            }
        }
        val generatedMedian = generatedTimes.sorted()[generatedTimes.size / 2]
        val jsonMedian = jsonTimes.sorted()[jsonTimes.size / 2]
        Log.i("HolderCardCreateTiming", "generated=${generatedMedian / 1_000_000.0}ms " +
            "json=${jsonMedian / 1_000_000.0}ms")
        assertTrue("Generated creation took over twice JSON plus 10ms: $generatedMedian vs $jsonMedian ns",
            generatedMedian <= jsonMedian * 2 + 10_000_000)
    }

    @Test
    fun generatedCardWrites_preserveEditorSemanticsAndStayWithinCallBudget() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Generated writes")
        val other = HolderNative.createProject("Other project")
        val generated = HolderNative.createCard(project.projectId, "Café 📝", "First body")
        val json = HolderNative.createCard(project.projectId, "Café 📝", "First body")
        assertThrows(RuntimeException::class.java) {
            HolderNative.updateCard(other.projectId, generated.cardId, "Wrong", "Wrong")
        }
        assertThrows(RuntimeException::class.java) {
            HolderNative.deleteCard(other.projectId, generated.cardId)
        }
        assertEquals("Café 📝", HolderNative.updateCard(project.projectId, generated.cardId,
            "", "Привет 🌍").title)
        assertEquals("Привет 🌍", HolderNative.getCardContent(project.projectId, generated.cardId))

        val generatedUpdate = ArrayList<Long>()
        val jsonUpdate = ArrayList<Long>()
        val generatedTrash = ArrayList<Long>()
        val jsonTrash = ArrayList<Long>()
        val generatedRestore = ArrayList<Long>()
        val jsonRestore = ArrayList<Long>()
        fun sample(call: () -> Unit): Long {
            val start = System.nanoTime()
            call()
            return System.nanoTime() - start
        }
        repeat(2) { round ->
            HolderNative.updateCard(project.projectId, generated.cardId, "Café 📝", "warmup $round")
            HolderNative.updateCardJsonForBenchmark(json.cardId, "Café 📝", "warmup $round")
            HolderNative.deleteCard(project.projectId, generated.cardId)
            HolderNative.deleteCardJsonForBenchmark(json.cardId)
            HolderNative.restoreCard(project.projectId, generated.cardId)
            HolderNative.restoreCardJsonForBenchmark(json.cardId)
        }
        repeat(5) { round ->
            val body = "Body $round #tag"
            if (round % 2 == 0) {
                generatedUpdate.add(sample { HolderNative.updateCard(project.projectId, generated.cardId, "Café 📝", body) })
                jsonUpdate.add(sample { HolderNative.updateCardJsonForBenchmark(json.cardId, "Café 📝", body) })
                generatedTrash.add(sample { HolderNative.deleteCard(project.projectId, generated.cardId) })
                jsonTrash.add(sample { HolderNative.deleteCardJsonForBenchmark(json.cardId) })
                generatedRestore.add(sample { HolderNative.restoreCard(project.projectId, generated.cardId) })
                jsonRestore.add(sample { HolderNative.restoreCardJsonForBenchmark(json.cardId) })
            } else {
                jsonUpdate.add(sample { HolderNative.updateCardJsonForBenchmark(json.cardId, "Café 📝", body) })
                generatedUpdate.add(sample { HolderNative.updateCard(project.projectId, generated.cardId, "Café 📝", body) })
                jsonTrash.add(sample { HolderNative.deleteCardJsonForBenchmark(json.cardId) })
                generatedTrash.add(sample { HolderNative.deleteCard(project.projectId, generated.cardId) })
                jsonRestore.add(sample { HolderNative.restoreCardJsonForBenchmark(json.cardId) })
                generatedRestore.add(sample { HolderNative.restoreCard(project.projectId, generated.cardId) })
            }
            assertEquals(body, HolderNative.getCardContent(project.projectId, generated.cardId))
            assertEquals(HolderNative.getCard(project.projectId, json.cardId).title,
                HolderNative.getCard(project.projectId, generated.cardId).title)
        }
        fun median(samples: List<Long>) = samples.sorted()[samples.size / 2]
        fun report(name: String, generatedTimes: List<Long>, jsonTimes: List<Long>) {
            val newMedian = median(generatedTimes)
            val oldMedian = median(jsonTimes)
            Log.i("HolderCardWriteTiming", "$name generated=${newMedian / 1_000_000.0}ms " +
                "json=${oldMedian / 1_000_000.0}ms")
            assertTrue("$name generated took over twice JSON plus 10ms: $newMedian vs $oldMedian ns",
                newMedian <= oldMedian * 2 + 10_000_000)
        }
        report("update", generatedUpdate, jsonUpdate)
        report("trash", generatedTrash, jsonTrash)
        report("restore", generatedRestore, jsonRestore)
    }

    @Test
    fun generatedTagAndSearchLists_matchJsonAndStayWithinCallBudget() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Generated record lists")
        repeat(8) { index ->
            HolderNative.createCard(project.projectId, "Résumé $index 📝", "A résumé about #urgent and #android $index")
        }
        assertEquals(HolderNative.listProjectTagsJsonForBenchmark(project.projectId),
            HolderNative.listProjectTags(project.projectId))
        assertEquals(HolderNative.searchCardsJsonForBenchmark(project.projectId, "résumé"),
            HolderNative.searchCards(project.projectId, "résumé"))

        val tagsGenerated = ArrayList<Long>()
        val tagsJson = ArrayList<Long>()
        val searchGenerated = ArrayList<Long>()
        val searchJson = ArrayList<Long>()
        fun sample(call: () -> Unit): Long {
            val start = System.nanoTime()
            call()
            return System.nanoTime() - start
        }
        repeat(2) {
            HolderNative.listProjectTags(project.projectId)
            HolderNative.listProjectTagsJsonForBenchmark(project.projectId)
            HolderNative.searchCards(project.projectId, "résumé")
            HolderNative.searchCardsJsonForBenchmark(project.projectId, "résumé")
        }
        repeat(7) { round ->
            if (round % 2 == 0) {
                tagsGenerated.add(sample { HolderNative.listProjectTags(project.projectId) })
                tagsJson.add(sample { HolderNative.listProjectTagsJsonForBenchmark(project.projectId) })
                searchGenerated.add(sample { HolderNative.searchCards(project.projectId, "résumé") })
                searchJson.add(sample { HolderNative.searchCardsJsonForBenchmark(project.projectId, "résumé") })
            } else {
                tagsJson.add(sample { HolderNative.listProjectTagsJsonForBenchmark(project.projectId) })
                tagsGenerated.add(sample { HolderNative.listProjectTags(project.projectId) })
                searchJson.add(sample { HolderNative.searchCardsJsonForBenchmark(project.projectId, "résumé") })
                searchGenerated.add(sample { HolderNative.searchCards(project.projectId, "résumé") })
            }
        }
        fun median(samples: List<Long>) = samples.sorted()[samples.size / 2]
        fun report(name: String, generated: List<Long>, json: List<Long>) {
            val newMedian = median(generated)
            val oldMedian = median(json)
            Log.i("HolderRecordListTiming", "$name generated=${newMedian / 1_000_000.0}ms " +
                "json=${oldMedian / 1_000_000.0}ms")
            assertTrue("$name generated took over twice JSON plus 10ms: $newMedian vs $oldMedian ns",
                newMedian <= oldMedian * 2 + 10_000_000)
        }
        report("projectTags", tagsGenerated, tagsJson)
        report("searchCards", searchGenerated, searchJson)
    }

    @Test
    fun generatedCardListing_followsCursorPastTheFirstPage() {
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

        val jsonCards = HolderNative.listCardsJsonForBenchmark(project.projectId)
        val generatedCards = HolderNative.listCards(project.projectId)
        assertEquals(5_001, generatedCards.size)
        assertEquals(jsonCards, generatedCards)
        assertEquals(1, generatedCards.count { it.title == "Café 日本語 📝" })
        assertEquals(generatedCards, HolderNative.listCards(project.projectId))

        // ART's process-wide allocation counter includes the read lane; retain each result so
        // native-heap deltas describe memory still held after the JNI arena has been freed.
        val retained = ArrayList<List<HolderCard>>(8)
        fun sample(call: () -> List<HolderCard>): Pair<Long, Long> {
            val javaBefore = Debug.getRuntimeStat("art.gc.bytes-allocated")!!.toLong()
            val nativeBefore = Debug.getNativeHeapAllocatedSize()
            retained.add(call())
            return Pair(
                Debug.getRuntimeStat("art.gc.bytes-allocated")!!.toLong() - javaBefore,
                Debug.getNativeHeapAllocatedSize() - nativeBefore,
            )
        }
        val generatedMemory = ArrayList<Pair<Long, Long>>()
        val jsonMemory = ArrayList<Pair<Long, Long>>()
        repeat(4) { round ->
            if (round % 2 == 0) {
                generatedMemory.add(sample { HolderNative.listCards(project.projectId) })
                jsonMemory.add(sample { HolderNative.listCardsJsonForBenchmark(project.projectId) })
            } else {
                jsonMemory.add(sample { HolderNative.listCardsJsonForBenchmark(project.projectId) })
                generatedMemory.add(sample { HolderNative.listCards(project.projectId) })
            }
        }
        fun median(values: List<Long>) = values.sorted()[values.size / 2]
        Log.i("HolderCardPageMemory", "5001 cards: generated Java allocated=" +
            "${median(generatedMemory.map { it.first })}B, JSON=${median(jsonMemory.map { it.first })}B; " +
            "generated native heap delta=${median(generatedMemory.map { it.second })}B, " +
            "JSON=${median(jsonMemory.map { it.second })}B")
    }

    @Test
    fun generatedCardListing_errorLeavesReadLaneUsable() {
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
        val legacy = HolderNative.getTrashedCard(card.cardId)
        assertEquals(legacy.cardId, trashed.cardId)
        assertEquals(legacy.title, trashed.title)
        assertEquals(legacy.deletedAt, trashed.deletedAt)
        assertEquals(HolderNative.listTrashedCardsJsonForBenchmark(project.projectId).map { it.cardId },
            HolderNative.listTrashedCards(project.projectId).map { it.cardId })

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
    fun generatedTrashPurge_respectsProjectAndEmptiesOnlyItsCards() {
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

        val empty = HolderNative.listCardLinks(blocked.cardId)
        assertTrue(empty.outgoing.isEmpty())
        assertTrue(empty.backlinks.isEmpty())

        val outgoing = HolderNative.addCardLink(blocked.cardId, blocking.cardId, "blocked_by", "waiting")
        assertEquals(1, outgoing.size)
        assertEquals(blocking.cardId, outgoing.single().toCardId)
        assertEquals("Blocking task", outgoing.single().toTitle)

        val blockingLinks = HolderNative.listCardLinks(blocking.cardId)
        assertEquals(1, blockingLinks.backlinks.size)
        assertEquals(blocked.cardId, blockingLinks.backlinks.single().fromCardId)
        assertEquals("Blocked task", blockingLinks.backlinks.single().fromTitle)

        HolderNative.removeCardLink(blocked.cardId, blocking.cardId, "blocked_by")
        assertTrue(HolderNative.listCardLinks(blocked.cardId).outgoing.isEmpty())
        assertTrue(HolderNative.listCardLinks(blocking.cardId).backlinks.isEmpty())
    }

    @Test
    fun cardLinks_reflectParentChildHierarchyThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Hierarchy test project")
        val parent = HolderNative.createCard(project.projectId, "Parent card", "content")
        val child = HolderNative.createCard(project.projectId, "Child card", "content", parent.cardId)

        val parentLinks = HolderNative.listCardLinks(parent.cardId)
        assertEquals(null, parentLinks.parent)
        assertEquals(1, parentLinks.children.size)
        assertEquals(child.cardId, parentLinks.children.single().cardId)

        val childLinks = HolderNative.listCardLinks(child.cardId)
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
        assertEquals(HolderNative.listCardTagsJsonForBenchmark(card.cardId),
            HolderNative.listCardTags(project.projectId, card.cardId))

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
    fun milestones_roundTripThroughTheFullJniBoundary() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Milestones test project")
        val card = HolderNative.createCard(project.projectId, "Renewal", "content")

        assertTrue(HolderNative.listCardMilestones(card.cardId).isEmpty())

        // An all-day milestone is a calendar date, stored as its midnight UTC.
        val start = 1_799_971_200L
        val afterAdd = HolderNative.addCardMilestone(
            cardId = card.cardId,
            startAt = start,
            endAt = null,
            allDay = true,
            kind = "Renewal",
            description = "Car insurance",
        )
        assertEquals(1, afterAdd.size)
        val added = afterAdd.single()
        assertEquals(card.cardId, added.cardId)
        assertEquals(start, added.startAt)
        assertNull(added.endAt)
        assertTrue(added.allDay)
        assertEquals("Renewal", added.kind)
        assertEquals("Car insurance", added.description)

        assertEquals(added, HolderNative.listCardMilestones(card.cardId).single())

        val inRange = HolderNative.listMilestonesInRange(project.projectId, start - 3600, start + 3600)
        assertEquals(1, inRange.size)
        assertEquals(added.milestoneId, inRange.single().milestoneId)
        assertEquals("Renewal", inRange.single().cardTitle)

        assertTrue(HolderNative.listMilestonesInRange(project.projectId, start + 7200, start + 10_800).isEmpty())

        HolderNative.removeCardMilestone(card.cardId, added.milestoneId)
        assertTrue(HolderNative.listCardMilestones(card.cardId).isEmpty())
        assertTrue(HolderNative.listMilestonesInRange(project.projectId, start - 3600, start + 3600).isEmpty())
    }

    @Test
    fun milestones_supportASpanWithAnEndAndSurviveCardTrash() {
        initialize("# Welcome\n\nWelcome")
        val project = HolderNative.createProject("Milestone span test project")
        val card = HolderNative.createCard(project.projectId, "Conference", "content")

        val start = 1_800_000_000L
        val end = start + 2 * 24 * 60 * 60
        val added = HolderNative.addCardMilestone(cardId = card.cardId, startAt = start, endAt = end)
            .single()
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
        val restored = HolderNative.restoreCardHistory(card.cardId, creationOid)
        assertEquals(card.cardId, restored.cardId)
        assertEquals("Sharpen at 15 degrees", HolderNative.getCardContent(project.projectId, card.cardId))

        val afterRestore = HolderNative.listCardHistory(project.projectId, card.cardId)
        assertEquals(3, afterRestore.entries.size)
        assertEquals("restored", afterRestore.entries.first().kind)
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
