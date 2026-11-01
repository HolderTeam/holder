package team.holder.android.git.backup

import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Rule
import org.junit.Test
import org.junit.rules.TemporaryFolder
import java.io.File
import java.util.zip.GZIPOutputStream

class SnapshotReaderTest {
    @get:Rule
    val tempFolder = TemporaryFolder()

    private fun gzipJsonl(lines: List<String>): File {
        val file = File(tempFolder.newFolder("snapshot"), "snapshot.jsonl.gz")
        GZIPOutputStream(file.outputStream()).use { gzip ->
            lines.forEach { gzip.write((it + "\n").toByteArray(Charsets.UTF_8)) }
        }
        return file
    }

    // The reader never reads core's line, so any text stands in for it.
    private fun card(projectId: String, projectName: String, privacyMode: String, cardId: String) =
        snapshotLine(projectId, projectName, privacyMode, "core line $cardId")

    @Test
    fun hasSnapshot_isFalse_whenNoFileExists() {
        val file = File(tempFolder.newFolder("nope"), "snapshot.jsonl.gz")
        assertFalse(SnapshotReader.hasSnapshot(file))
    }

    @Test
    fun hasSnapshot_isFalse_forAnEmptyFile() {
        val file = File(tempFolder.newFolder("empty"), "snapshot.jsonl.gz")
        file.createNewFile()
        assertFalse(SnapshotReader.hasSnapshot(file))
    }

    @Test
    fun hasSnapshot_isTrue_forANonEmptyFile() {
        val file = gzipJsonl(listOf(card("p1", "House", "plain", "c1")))
        assertTrue(SnapshotReader.hasSnapshot(file))
    }

    @Test
    fun readGroups_groupsCardsByProjectId_inFirstSeenOrder() {
        val file = gzipJsonl(
            listOf(
                card("p2", "Car", "plain", "c1"),
                card("p1", "House", "encrypted_git", "c2"),
                card("p2", "Car", "plain", "c3"),
                card("p1", "House", "encrypted_git", "c4"),
            ),
        )

        val groups = SnapshotReader.readGroups(file)

        assertEquals(listOf("Car", "House"), groups.map { it.projectName })
        assertEquals(listOf("plain", "encrypted_git"), groups.map { it.privacyMode })
        assertEquals(listOf(2, 2), groups.map { it.cardCount })
    }

    @Test
    fun readGroups_snapshot_isCoresLinesUnchanged_inOrder() {
        val file = gzipJsonl(
            listOf(
                card("p1", "House", "plain", "c1"),
                card("p1", "House", "plain", "c2"),
            ),
        )

        val group = SnapshotReader.readGroups(file).single()

        assertEquals("core line c1\ncore line c2", group.snapshot)
    }

    @Test
    fun readGroups_skipsBlankAndMalformedLines_withoutFailingTheWholeRead() {
        val file = File(tempFolder.newFolder("snapshot"), "snapshot.jsonl.gz")
        GZIPOutputStream(file.outputStream()).use { gzip ->
            gzip.write((card("p1", "House", "plain", "c1") + "\n").toByteArray(Charsets.UTF_8))
            gzip.write("\n".toByteArray(Charsets.UTF_8))
            gzip.write("not json at all, e.g. a truncated last line\n".toByteArray(Charsets.UTF_8))
            gzip.write("""{"line":"core line c2"}""".plus("\n").toByteArray(Charsets.UTF_8))
            gzip.write("""{"project_id":"p1","project_name":"House"}""".plus("\n").toByteArray(Charsets.UTF_8))
            gzip.write((card("p1", "House", "plain", "c3") + "\n").toByteArray(Charsets.UTF_8))
        }

        val group = SnapshotReader.readGroups(file).single()

        assertEquals(2, group.cardCount)
        assertEquals("core line c1\ncore line c3", group.snapshot)
    }

    @Test
    fun readGroups_returnsEmptyList_forASnapshotWithNoCards() {
        val file = gzipJsonl(emptyList())
        assertTrue(SnapshotReader.readGroups(file).isEmpty())
    }

    @Test
    fun readGroups_returnsEmptyList_ratherThanThrowing_whenNoFileExistsAtAll() {
        // The common real case: the manual "Restore from backup" button has no guard of its
        // own (see RestoreBackupScreen), so most taps on it happen on a device that's never had
        // anything to restore. This used to surface as a raw FileNotFoundException instead of
        // the same "nothing here" result an empty snapshot already produces above.
        val file = File(tempFolder.newFolder("nope"), "snapshot.jsonl.gz")
        assertTrue(SnapshotReader.readGroups(file).isEmpty())
    }

    @Test
    fun readGroups_defaultsProjectNameAndPrivacyMode_whenMissing() {
        val file = File(tempFolder.newFolder("snapshot"), "snapshot.jsonl.gz")
        val bareCard = JSONObject().apply {
            put("project_id", "p1")
            put("line", "core line c1")
        }.toString()
        GZIPOutputStream(file.outputStream()).use { gzip ->
            gzip.write((bareCard + "\n").toByteArray(Charsets.UTF_8))
        }

        val group = SnapshotReader.readGroups(file).single()

        assertEquals("Restored project", group.projectName)
        assertEquals("plain", group.privacyMode)
    }
}
