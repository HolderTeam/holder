package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.security.MessageDigest
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.ui.screens.loadStartScreenData

/**
 * Issue #106: after installing this build over an older one, the older build's data opens.
 * Run by `scripts/upgrade-probe.sh` with the `upgradeVerify` instrumentation argument, after
 * the older build's own test (scripts/upgrade-probe/OldProfileSeed.kt) filled the app's real store
 * and wrote what it made to upgrade-seed.json. Skipped in ordinary suite runs.
 *
 * Checks every card's title, body and parent, the trashed card, a link, a tag, the attachment's
 * bytes and the encrypted project's encryption, then the start screen, then all of it again
 * after rebuilding the database from the project files and the Android keyring.
 */
@RunWith(AndroidJUnit4::class)
class UpgradedProfileProbe {
    private val context = InstrumentationRegistry.getInstrumentation().targetContext
    private val dataDir = File(context.filesDir, "holder")
    private val schemaSql by lazy { context.assets.open("schema.sql").bufferedReader().use { it.readText() } }

    @Test
    fun theOlderBuildsDataOpens() {
        assumeTrue("Run by scripts/upgrade-probe.sh",
            InstrumentationRegistry.getArguments().getString("upgradeVerify") != null)
        val seed = JSONObject(File(context.filesDir, "upgrade-seed.json").readText())
        HolderNative.close()
        HolderNative.initialize(context, dataDir, schemaSql)
        check(seed)
        val recent = loadStartScreenData().recentCards.map { it.cardId }
        assertTrue("the start screen lists the upgraded cards", seed.getString("tagged") in recent)

        HolderNative.close()
        assertTrue(dataDir.resolve("server/holder.db").delete())
        HolderNative.rebuildDatabase(context, dataDir, schemaSql)
        check(seed)
        HolderNative.close()
        android.util.Log.i("UpgradeProbe", "upgraded profile verified")
    }

    private fun check(seed: JSONObject) {
        val encrypted = seed.getString("encrypted")
        val plain = seed.getString("plain")
        val projects = HolderNative.listProjects().associateBy { it.projectId }
        assertEquals("encrypted_git", projects.getValue(encrypted).privacyMode)
        assertEquals("plain", projects.getValue(plain).privacyMode)
        assertTrue("the encrypted project's files are encrypted", HolderNative.encryptionCheck(encrypted).ok)

        val cards = seed.getJSONArray("cards")
        for (i in 0 until cards.length()) {
            val expected = cards.getJSONObject(i)
            val projectId = expected.getString("project")
            val id = expected.getString("id")
            if (id == seed.getString("trashed")) continue
            val card = HolderNative.listCards(projectId).single { it.cardId == id }
            assertEquals(expected.getString("title"), card.title)
            assertEquals(if (expected.isNull("parent")) null else expected.getString("parent"), card.parentCardId)
            assertEquals(expected.getString("content"), HolderNative.getCardContent(projectId, id))
        }
        assertTrue(HolderNative.listTrashedCards(encrypted).any { it.cardId == seed.getString("trashed") })

        val link = seed.getJSONObject("link")
        val outgoing = HolderNative.listCardLinks(encrypted, link.getString("from")).outgoing
        assertTrue(outgoing.any { it.toCardId == link.getString("to") && it.kind == "blocked_by" && it.label == "costs" })
        assertTrue(HolderNative.cardsWithTag(encrypted, "travel").any { it.cardId == seed.getString("tagged") })

        val resource = HolderNative.listResources(encrypted).single { it.resourceId == seed.getString("resource") }
        val asset = resource.assets.single { it.assetId == seed.getString("asset") }
        val out = File(context.cacheDir, "upgrade-asset").apply { delete() }
        HolderNative.retrieveAsset(resource.resourceId, asset.assetId, asset.placements.single().placementId, out.absolutePath)
        val sha = MessageDigest.getInstance("SHA-256").digest(out.readBytes()).joinToString("") { "%02x".format(it) }
        out.delete()
        assertEquals(seed.getString("assetSha256"), sha)
    }
}
