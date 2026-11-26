package team.holder.android

import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import java.io.File
import java.security.MessageDigest
import org.json.JSONArray
import org.json.JSONObject
import org.junit.Test
import org.junit.runner.RunWith

/** Issue #106, run only on this old (schema 6) build: fills the app's real store with data for
 * the current build to open after an upgrade, and writes what it made to upgrade-seed.json. */
@RunWith(AndroidJUnit4::class)
class OldProfileSeed {
    @Test
    fun seed() {
        val context = InstrumentationRegistry.getInstrumentation().targetContext
        HolderNative.initialize(
            context,
            File(context.filesDir, "holder"),
            context.assets.open("schema.sql").bufferedReader().use { it.readText() },
            context.assets.open("WELCOME.md").bufferedReader().use { it.readText() },
        )
        val cards = JSONArray()
        fun card(projectId: String, title: String, content: String, parent: HolderCard? = null): HolderCard {
            val made = HolderNative.createCard(projectId, title, content, parent?.cardId)
            cards.put(JSONObject().put("project", projectId).put("id", made.cardId).put("title", title)
                .put("content", content).put("parent", parent?.cardId ?: JSONObject.NULL))
            return made
        }

        val secret = HolderNative.createProject("Upgrade encrypted", "encrypted_git")
        val plain = HolderNative.createProject("Upgrade plain", "plain")
        val trip = card(secret.projectId, "Trip ✈", "# Trip ✈\n\nPack light. #travel")
        val days = card(secret.projectId, "Days", "# Days", trip)
        val monday = card(secret.projectId, "Monday", "# Monday\n\nMuseum, then café.", days)
        card(secret.projectId, "Tuesday", "# Tuesday\n\nHike.", days)
        val budget = card(secret.projectId, "Budget", "# Budget\n\n- Train £120")
        val doomed = card(secret.projectId, "Old idea", "# Old idea\n\nDropped.")
        card(plain.projectId, "Shopping", "Shopping\n\nMilk, eggs")
        HolderNative.addCardLink(monday.cardId, budget.cardId, "blocked_by", "costs")
        HolderNative.deleteCard(doomed.cardId)

        val locationId = "upgrade-local"
        HolderNative.putLocation(locationId, secret.projectId, "Local", "local_directory", emptyMap(),
            System.currentTimeMillis() / 1000)
        val photo = ByteArray(120_000) { (it * 31 + 7).toByte() }
        val source = File(context.cacheDir, "ticket.jpg").apply { writeBytes(photo) }
        val asset = HolderNative.importAsset(secret.projectId, trip.cardId, locationId, source.absolutePath)
        source.delete()

        File(context.filesDir, "upgrade-seed.json").writeText(JSONObject()
            .put("encrypted", secret.projectId)
            .put("plain", plain.projectId)
            .put("cards", cards)
            .put("trashed", doomed.cardId)
            .put("link", JSONObject().put("from", monday.cardId).put("to", budget.cardId))
            .put("tagged", trip.cardId)
            .put("resource", asset.resourceId)
            .put("asset", asset.assetId)
            .put("assetSha256", MessageDigest.getInstance("SHA-256").digest(photo).joinToString("") { "%02x".format(it) })
            .toString(2))
        HolderNative.close()
        // The keyring saves project keys with SharedPreferences.apply(), which writes later. A
        // process that ends at once can lose them, so give the write time to land.
        Thread.sleep(3000)
    }
}
