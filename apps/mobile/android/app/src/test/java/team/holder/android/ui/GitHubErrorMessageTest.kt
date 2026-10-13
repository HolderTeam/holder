package team.holder.android.ui

import java.io.IOException
import javax.net.ssl.SSLHandshakeException
import org.junit.Assert.assertEquals
import org.junit.Test
import team.holder.android.git.github.GitHubError

class GitHubErrorMessageTest {
    @Test
    fun relayAndTransientConnectFailuresHaveAccurateActions() {
        assertEquals(
            "GitHub sign-in service is unavailable -- try again",
            githubErrorMessage(GitHubError.RelayUnavailable),
        )
        assertEquals(
            "GitHub sign-in is busy -- try again",
            githubErrorMessage(GitHubError.TryAgain),
        )
    }

    @Test
    fun networkErrorsDoNotExposeTransportDiagnostics() {
        for (cause in listOf(
            IOException("request failed: refresh_token=secret"),
            SSLHandshakeException("certificate diagnostic: private-host.example"),
            IOException(),
        )) {
            assertEquals(
                "Could not reach GitHub -- check your connection and try again",
                githubErrorMessage(GitHubError.NetworkError(cause)),
            )
        }
    }
}
