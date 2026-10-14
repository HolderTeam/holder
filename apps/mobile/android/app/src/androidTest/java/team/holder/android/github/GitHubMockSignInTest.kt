package team.holder.android.github

import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.ParcelFileDescriptor
import android.os.SystemClock
import android.util.Log
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.uiautomator.By
import androidx.test.uiautomator.BySelector
import androidx.test.uiautomator.UiDevice
import androidx.test.uiautomator.UiObject2
import androidx.test.uiautomator.Until
import java.io.ByteArrayOutputStream
import java.io.IOException
import java.net.HttpURLConnection
import java.net.URL
import java.util.regex.Pattern
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Assume.assumeTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import team.holder.android.BuildConfig
import team.holder.android.git.github.GitHubEnvironment

/**
 * Connect to GitHub and set up the Home project's repository, end to end: the app, the
 * device's real browser and the OAuth callback App Link, against the local mock GitHub in
 * `mock-github/`. Runs only in a build made with `-Pholder.githubMock`; everywhere else it is
 * skipped. `mock-github/run_device_tests.py` starts the mock and runs it on emulators.
 *
 * Drives everything through UI Automator rather than the Compose test APIs because the flow
 * leaves the app for the browser and comes back through the callback.
 */
@RunWith(AndroidJUnit4::class)
class GitHubMockSignInTest {
    private val instrumentation = InstrumentationRegistry.getInstrumentation()
    private val device = UiDevice.getInstance(instrumentation)
    private val context = instrumentation.targetContext

    // The Android 9 Google image runs a full Play services stack and is far slower to settle.
    private val stepTimeoutMs = if (Build.VERSION.SDK_INT <= 28) 180_000L else 60_000L

    @Before
    fun setUp() {
        assumeTrue("needs a debug build made with -Pholder.githubMock", BuildConfig.GITHUB_MOCK_URL.isNotEmpty())
        awaitMock()
        mock("reset")
        prepareDevice()
    }

    @Test
    fun connectAndSetUpTheHomeRepository() {
        launchApp()
        click(By.desc("Settings"))
        click(By.text("Sync"))
        click(By.text("Connect"))

        authorizeInBrowser()

        // Back in the app: it offers to put the existing Home project on GitHub.
        await(By.text(Pattern.compile("Keep your existing projects safe.*")))
        dialogButton("Sync").click()
        await(By.text("1 of 1 synced."))
        click(By.text("Done"))
        await(By.text("Connected as @mock-user"))

        val state = JSONObject(mock("state"))
        val repos = state.getJSONArray("repos")
        assertEquals("one repository created: $repos", 1, repos.length())
        val repo = repos.getJSONObject(0)
        assertTrue("Home's repository: $repo", repo.getString("name").startsWith("mock-user/holder-home-"))
        assertEquals("the device's deploy key added", 1, repo.getInt("keys"))
        assertEquals("no refresh token sent twice", 0, state.getInt("replays"))
    }

    /** Taps through the browser's first-run screens, if any, then Authorize on the mock's page. */
    private fun authorizeInBrowser() {
        val deadline = SystemClock.uptimeMillis() + stepTimeoutMs
        while (true) {
            val authorize = device.findObject(By.text("Authorize"))
            if (authorize != null) {
                authorize.click()
                return
            }
            CHROME_FIRST_RUN_BUTTONS.firstNotNullOfOrNull { device.findObject(it) }?.click()
            if (SystemClock.uptimeMillis() > deadline) failWithScreen("the mock's Authorize page never appeared")
            SystemClock.sleep(500)
        }
    }

    private fun prepareDevice() {
        val packageName = context.packageName
        val callbackHost = Uri.parse(GitHubEnvironment.OAUTH_CALLBACK_URL).host!!
        // Approve the callback App Link for this package directly, so the test does not depend
        // on the device having verified the domain over the network.
        if (Build.VERSION.SDK_INT >= 31) {
            shell("pm set-app-links-user-selection --user 0 --package $packageName true $callbackHost")
        } else {
            shell("pm set-app-link $packageName always")
        }
        // Google emulator images skip the setup wizard, which leaves its web dialog enabled with
        // a catch-all, higher-priority web link filter, so it outranks Chrome as the browser.
        // Shell may not disable another package's component, but the activity disables itself
        // the first time it starts after setup. Real devices finished setup and never have it.
        val webHandlers = context.packageManager.queryIntentActivities(
            Intent(Intent.ACTION_VIEW, Uri.parse("http://example.com/")).addCategory(Intent.CATEGORY_BROWSABLE),
            0,
        ).map { it.activityInfo.packageName }
        if (SETUP_WIZARD in webHandlers) {
            shell("am start -W -a android.intent.action.VIEW -d http://example.com/ -n $SETUP_WIZARD/.util.WebDialogActivity")
            device.pressHome()
        }
    }

    private fun launchApp() {
        val intent = context.packageManager.getLaunchIntentForPackage(context.packageName)!!
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK)
        context.startActivity(intent)
        await(By.desc("Settings"))
    }

    /** The button with [text] in the dialog's button row, not the screen title of the same name. */
    private fun dialogButton(text: String): UiObject2 {
        val notNow = await(By.text("Not now"))
        return device.findObjects(By.text(text))
            .minByOrNull { Math.abs(it.visibleBounds.centerY() - notNow.visibleBounds.centerY()) }
            ?: failWithScreen("no '$text' button next to 'Not now'")
    }

    private fun click(selector: BySelector) {
        await(selector).click()
    }

    private fun await(selector: BySelector): UiObject2 =
        device.wait(Until.findObject(selector), stepTimeoutMs) ?: failWithScreen("timed out waiting for $selector")

    private fun failWithScreen(message: String): Nothing {
        val dump = ByteArrayOutputStream().also { device.dumpWindowHierarchy(it) }.toString()
        Log.e(TAG, "$message. Screen:\n$dump")
        throw AssertionError("$message (screen hierarchy in logcat under $TAG)")
    }

    private fun shell(command: String): String {
        val output = instrumentation.uiAutomation.executeShellCommand(command)
        return ParcelFileDescriptor.AutoCloseInputStream(output).bufferedReader().use { it.readText() }
            .also { Log.i(TAG, "$ command\n$it") }
    }

    /** A freshly booted emulator can report boot complete before its network is up. */
    private fun awaitMock() {
        val deadline = SystemClock.uptimeMillis() + 90_000L
        while (true) {
            try {
                mock("state")
                return
            } catch (e: IOException) {
                if (SystemClock.uptimeMillis() > deadline) {
                    throw AssertionError("the mock at ${BuildConfig.GITHUB_MOCK_URL} is not reachable; is it running?", e)
                }
                SystemClock.sleep(1_000)
            }
        }
    }

    /** GET /_mock/state, or POST /_mock/<action>. */
    private fun mock(action: String): String {
        val connection = URL("${BuildConfig.GITHUB_MOCK_URL}/_mock/$action").openConnection() as HttpURLConnection
        return try {
            if (action != "state") {
                connection.requestMethod = "POST"
                connection.doOutput = true
                connection.setRequestProperty("Content-Type", "application/json")
                connection.outputStream.use { it.write("{}".toByteArray()) }
            }
            check(connection.responseCode == 200) { "mock $action: HTTP ${connection.responseCode}" }
            connection.inputStream.bufferedReader().use { it.readText() }
        } finally {
            connection.disconnect()
        }
    }

    private companion object {
        const val TAG = "GitHubMockSignInTest"
        const val SETUP_WIZARD = "com.google.android.setupwizard"

        // Chrome's first-run and sign-in prompts across versions; any of them may appear before
        // the page loads on a freshly created emulator. Recent versions expose only the IDs.
        val CHROME_FIRST_RUN_BUTTONS: List<BySelector> = listOf(
            "Accept & continue",
            "No thanks",
            "No, thanks",
            "Use without an account",
            "Got it",
        ).map { By.text(Pattern.compile(Pattern.quote(it), Pattern.CASE_INSENSITIVE)) } + listOf(
            "terms_accept",
            "signin_fre_dismiss_button",
            "negative_button",
            "ack_button",
        ).map { By.res("com.android.chrome", it) }
    }
}
