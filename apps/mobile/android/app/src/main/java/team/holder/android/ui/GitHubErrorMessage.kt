package team.holder.android.ui

import team.holder.android.git.github.GitHubError

/** A short, human-readable summary of [error] -- shared by every screen that surfaces a
 * `GitHubResult.Failure` directly (New Project, `RecoverProjectScreen`) rather than each
 * writing its own copy for the same handful of cases. Deliberately generic; a caller that
 * wants a specific call-to-action (e.g. "tap to grant access", using
 * [team.holder.android.git.github.GitHubError.RepositoryNotAccessible]'s own
 * `installationSettingsUrl`) still branches on [error]'s subtype itself for that -- this is
 * just the fallback description. */
fun githubErrorMessage(error: GitHubError): String = when (error) {
    GitHubError.AuthorizationRequired -> "GitHub needs to be reconnected"
    is GitHubError.InstallationRequired -> "Holder Project Setup isn't installed on your GitHub account yet"
    is GitHubError.RepositoryNotAccessible -> "GitHub needs one more permission for this repository"
    is GitHubError.RateLimited -> "GitHub request was rate limited -- try again shortly"
    is GitHubError.NetworkError -> "Could not reach GitHub -- check your connection and try again"
    GitHubError.RelayUnavailable -> "GitHub sign-in service is unavailable -- try again"
    is GitHubError.ConnectInterrupted -> when (error.reason) {
        GitHubError.ConnectInterrupted.Reason.RequestsBusy -> "GitHub is busy with another request -- try again"
        GitHubError.ConnectInterrupted.Reason.CredentialChanging ->
            "GitHub sign-in changed while it was being checked -- try again"
    }
    is GitHubError.Unexpected -> if (error.httpStatus != null) {
        "GitHub request failed (HTTP ${error.httpStatus})"
    } else {
        "GitHub operation failed -- try again"
    }
    GitHubError.AuthorizationVerificationFailed -> "GitHub sign-in verification failed -- try again"
    GitHubError.BrowserUnavailable -> "No browser is available to sign in with GitHub"
}
