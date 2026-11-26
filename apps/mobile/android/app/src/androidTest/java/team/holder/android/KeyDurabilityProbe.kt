package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import org.junit.Assert.assertEquals
import org.junit.Assume.assumeTrue
import org.junit.Test
import org.junit.runner.RunWith

/**
 * Issue #106: a new encrypted project's key must be on disk by the time creating the project
 * returns, so a crash straight afterwards doesn't leave its cards unreadable. Two runs, chosen
 * by the `keyPhase` instrumentation argument; skipped in ordinary suite runs.
 *
 * - `create`: a fresh store (so also a fresh encrypted Home), an encrypted project and a card,
 *   then the process kills itself at once, as a crash would.
 * - `open`: a new process opens the store and reads the card.
 */
@RunWith(AndroidJUnit4::class)
class KeyDurabilityProbe {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val phase = InstrumentationRegistry.getArguments().getString("keyPhase")
    private val dataDir = File(context.filesDir, "holder-key-probe")
    private val stateFile = File(context.filesDir, "holder-key-probe.txt")
    private val schemaSql by lazy { context.assets.open("schema.sql").bufferedReader().use { it.readText() } }

    @Test
    fun phase() {
        assumeTrue("Run with -e keyPhase create, then -e keyPhase open", phase != null)
        HolderNative.close()
        when (phase) {
            "create" -> {
                dataDir.deleteRecursively()
                HolderNative.initialize(context, dataDir, schemaSql)
                val project = HolderNative.createProject("Key probe", "encrypted_git")
                val card = HolderNative.createCard(project.projectId, "Secret", "# Secret\n\nBody")
                stateFile.writeText("${project.projectId}\n${card.cardId}")
                android.os.Process.killProcess(android.os.Process.myPid())
            }
            "open" -> {
                val (projectId, cardId) = stateFile.readLines()
                HolderNative.initialize(context, dataDir, schemaSql)
                assertEquals("# Secret\n\nBody", HolderNative.getCardContent(projectId, cardId))
                HolderNative.close()
                dataDir.deleteRecursively()
                stateFile.delete()
            }
            else -> error("Unknown keyPhase $phase")
        }
    }
}
