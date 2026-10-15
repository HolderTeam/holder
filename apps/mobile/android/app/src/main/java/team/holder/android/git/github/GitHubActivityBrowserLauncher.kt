package team.holder.android.git.github

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.util.Log
import androidx.activity.result.ActivityResultLauncher
import androidx.browser.auth.AuthTabIntent
import androidx.browser.customtabs.CustomTabsClient
import androidx.browser.customtabs.CustomTabsIntent
import androidx.browser.customtabs.CustomTabsService
import java.util.UUID

/**
 * Implements [GitHubConnectionCoordinator.GitHubBrowserLauncher] for a real Activity --
 * everything Activity/AuthTabIntent-specific that the coordinator itself must stay agnostic
 * of. [authTabLauncher] must be registered unconditionally, as an Activity field initializer
 * (see `MainActivity`) -- never created conditionally inside a Composable, matching the "at
 * most one unresolved AuthTabIntent launch per registration, ever" policy the coordinator's
 * own design relies on. [setOutstandingAttemptId] persists only a non-secret mirror onto the
 * Activity's SavedState (see `MainActivity.onSaveInstanceState`); the coordinator owns the
 * authoritative attempt marker and restores this mirror solely to discard stale OS results.
 */
internal class GitHubActivityBrowserLauncher(
    private val authTabLauncher: ActivityResultLauncher<Intent>,
    private val setOutstandingAttemptId: (UUID?) -> Unit,
) : GitHubConnectionCoordinator.GitHubBrowserLauncher {

    override fun resolveBrowser(
        context: Context,
        authorizationUrl: String,
    ): GitHubConnectionCoordinator.BrowserLaunch? {
        val packageManager = context.packageManager
        val launch = selectBrowser(
            authorizationUrl = authorizationUrl,
            defaultBrowser = defaultBrowserPackage(context),
            handlerPackages = { url -> handlerPackages(context, url) },
            offersCustomTabs = { packageName ->
                packageManager.resolveService(
                    Intent(CustomTabsService.ACTION_CUSTOM_TABS_CONNECTION).setPackage(packageName),
                    0,
                ) != null
            },
            // Checked for the selected package itself: capability and launch must agree.
            authTabSupported = { packageName -> CustomTabsClient.isAuthTabSupported(context, packageName) },
        )
        Log.d("GitHubConnection", "resolveBrowser: provider=${launch?.packageName} requestedLaunch=${launch?.kind}")
        return launch
    }

    override fun launch(
        context: Context,
        browser: GitHubConnectionCoordinator.BrowserLaunch,
        uri: Uri,
        attemptId: UUID,
    ) {
        when (browser.kind) {
            GitHubConnectionCoordinator.LaunchKind.AuthTab -> {
                // Persist BEFORE actually launching -- a result (even a fast one) must never
                // be able to arrive before this is recorded.
                setOutstandingAttemptId(attemptId)
                val redirect = Uri.parse(GitHubEnvironment.OAUTH_CALLBACK_URL)
                AuthTabIntent.Builder().build().also { authTabIntent ->
                    authTabIntent.intent.setPackage(browser.packageName)
                    authTabIntent.launch(authTabLauncher, uri, redirect.host.orEmpty(), redirect.path.orEmpty())
                }
            }
            GitHubConnectionCoordinator.LaunchKind.CustomTab -> {
                // GitHubConnectionCoordinator deliberately passes its own applicationContext
                // here (its operations can outlive any single screen), but starting an
                // Activity from a non-Activity Context requires this flag or it throws --
                // confirmed live: this exact call crashed on a real device without it.
                val customTabsIntent = CustomTabsIntent.Builder().build()
                customTabsIntent.intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                customTabsIntent.intent.setPackage(browser.packageName)
                customTabsIntent.launchUrl(context, uri)
            }
            GitHubConnectionCoordinator.LaunchKind.ExternalBrowser ->
                context.startActivity(
                    Intent(Intent.ACTION_VIEW, uri)
                        .addCategory(Intent.CATEGORY_BROWSABLE)
                        .setPackage(browser.packageName)
                        .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK),
                )
        }
    }

    /** The browser Android opens web links with, or null when there is none: with several
     * browsers and no default chosen (possible before Android 10's browser role), this
     * resolves to the system chooser, which is not a browser and so never selected. */
    private fun defaultBrowserPackage(context: Context): String? =
        context.packageManager.resolveActivity(
            Intent(Intent.ACTION_VIEW, Uri.parse(GENERIC_BROWSER_URI)).addCategory(Intent.CATEGORY_BROWSABLE),
            PackageManager.MATCH_DEFAULT_ONLY,
        )?.activityInfo?.packageName

    private fun handlerPackages(context: Context, url: String): List<String> =
        context.packageManager.queryIntentActivities(
            Intent(Intent.ACTION_VIEW, Uri.parse(url)).addCategory(Intent.CATEGORY_BROWSABLE),
            PackageManager.MATCH_DEFAULT_ONLY,
        ).mapNotNull { it.activityInfo?.packageName }

    internal companion object {
        /** Never launch a second Auth Tab through the same result registration. Its old result
         * could otherwise consume the newer attempt's marker. */
        fun customTabsLaunchKind(authTabSupported: Boolean): GitHubConnectionCoordinator.LaunchKind =
            if (authTabSupported) {
                GitHubConnectionCoordinator.LaunchKind.AuthTab
            } else {
                GitHubConnectionCoordinator.LaunchKind.CustomTab
            }

        /** A scheme-only URI exercises Android's generic browser intent semantics. It has no
         * arbitrary probe host for a link-handling app to claim. This works back to API 1. */
        internal const val GENERIC_BROWSER_URI = "http:"

        /**
         * The browser to open the authorization page in. Candidates are only generic browsers
         * that also resolve the exact authorization URI ([eligibleBrowsers]), the default
         * browser first. The first that offers Custom Tabs gets an Auth Tab or Custom Tab;
         * only if none does is the first opened as an external browser. Asking the default
         * browser alone would skip Custom Tabs whenever no default is chosen, even with
         * Custom Tabs browsers installed (#201).
         */
        fun selectBrowser(
            authorizationUrl: String,
            defaultBrowser: String?,
            handlerPackages: (String) -> List<String>,
            offersCustomTabs: (String) -> Boolean,
            authTabSupported: (String) -> Boolean,
        ): GitHubConnectionCoordinator.BrowserLaunch? {
            val eligible = eligibleBrowsers(authorizationUrl, defaultBrowser, handlerPackages)
            eligible.firstOrNull(offersCustomTabs)?.let { packageName ->
                return GitHubConnectionCoordinator.BrowserLaunch(
                    customTabsLaunchKind(authTabSupported(packageName)),
                    packageName,
                )
            }
            return eligible.firstOrNull()?.let {
                GitHubConnectionCoordinator.BrowserLaunch(GitHubConnectionCoordinator.LaunchKind.ExternalBrowser, it)
            }
        }

        /** Generic browsers that also resolve the exact authorization URI, [defaultBrowser]
         * first when it is one of them, otherwise in PackageManager's order. */
        fun eligibleBrowsers(
            authorizationUrl: String,
            defaultBrowser: String?,
            handlerPackages: (String) -> List<String>,
        ): List<String> {
            val genericBrowserHandlers = handlerPackages(GENERIC_BROWSER_URI).toSet()
            if (genericBrowserHandlers.isEmpty()) return emptyList()
            val eligible = handlerPackages(authorizationUrl).filter { it in genericBrowserHandlers }.distinct()
            return if (defaultBrowser in eligible) listOf(defaultBrowser!!) + (eligible - defaultBrowser) else eligible
        }

        /** [selectBrowser] when no candidate offers Custom Tabs and there is no default. */
        fun resolveExternalBrowser(
            authorizationUrl: String,
            handlerPackages: (String) -> List<String>,
        ): GitHubConnectionCoordinator.BrowserLaunch? = selectBrowser(
            authorizationUrl = authorizationUrl,
            defaultBrowser = null,
            handlerPackages = handlerPackages,
            offersCustomTabs = { false },
            authTabSupported = { false },
        )
    }
}
