package team.holder.android.git.github

import android.util.Log
import java.io.IOException
import java.net.ConnectException
import java.net.UnknownHostException
import kotlin.coroutines.resume
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withContext
import okhttp3.Call
import okhttp3.HttpUrl.Companion.toHttpUrl
import okhttp3.MediaType.Companion.toMediaType
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody
import okhttp3.RequestBody.Companion.toRequestBody
import okio.BufferedSink
import org.json.JSONObject

private val JSON_MEDIA_TYPE = "application/json; charset=UTF-8".toMediaType()

// The wire protocol contains only flat objects with scalar fields. Android's JSONTokener
// also accepts non-JSON syntax and duplicate keys; neither can establish a no-spend result.
private const val JSON_STRING = """"(?:[^"\\\u0000-\u001f]|\\(?:["\\/bfnrt]|u[0-9a-fA-F]{4}))*+""""
private const val JSON_NUMBER = """-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?"""
private const val JSON_SPACE = "[ \\t\\r\\n]*"
private val JSON_FIELD = Regex("($JSON_STRING)$JSON_SPACE:$JSON_SPACE(?:$JSON_STRING|$JSON_NUMBER|true|false|null)")
// At most five fields are valid; bound repetition and use possessive string matching so
// long malformed input cannot exhaust the regex engine's stack.
private val JSON_OBJECT = Regex(
    "$JSON_SPACE\\{$JSON_SPACE(?:${JSON_FIELD.pattern}(?:$JSON_SPACE,$JSON_SPACE${JSON_FIELD.pattern}){0,4})?$JSON_SPACE\\}$JSON_SPACE",
)

/** The result of a successful exchange/refresh: an access token to use immediately (cached by
 * [GitHubConnectionCoordinator], never persisted -- see its `accessTokenCache`) and the rotated
 * refresh token/cap pair to store in place of whatever was stored before. */
data class GitHubTokens(
    val accessToken: String,
    val expiresInSeconds: Int,
    val refreshToken: String,
    val refreshCap: String,
    val refreshTokenExpiresInSeconds: Int,
)

/** holder-github-service's own closed error envelope -- see that repo's `src/core/errors.ts`.
 * Deliberately mirrored one-for-one rather than collapsed at this layer; [GitHubConnectionCoordinator]
 * is the one place that decides how each of these maps differently depending on whether it came
 * from exchange or refresh. */
sealed interface RelayError {
    data object InvalidRequest : RelayError
    data class AuthorizationRequired(val reason: AuthorizationReason?) : RelayError
    data class RateLimited(val retryAfterSeconds: Int?) : RelayError
    data object OutcomeUnknown : RelayError
    /** A validated HTTP 500 operational envelope guarantees the credential was not spent. */
    data object Unexpected : RelayError

    /** A response outside the closed protocol cannot establish whether a credential was spent.
     * Keep only status metadata: even a malformed error body can contain live tokens. */
    data class InvalidResponse(val httpStatus: Int) : RelayError

    /** Not part of the relay's own envelope -- this is Android's own transport-layer
     * classification (see "Transport-failure ambiguity" in the plan). A failure that occurred
     * *after* the request may have been transmitted, where whether the relay/GitHub actually
     * received and processed it is genuinely unknown. */
    data object AmbiguousTransportFailure : RelayError

    /** Genuinely never reached the relay at all -- safe to treat as an ordinary, retryable
     * transport failure, since nothing was ever sent anywhere. */
    data class NetworkError(val cause: Throwable) : RelayError
}

sealed interface RelayResult {
    data class Success(val tokens: GitHubTokens) : RelayResult
    data class Failure(val error: RelayError) : RelayResult
}

/**
 * Calls holder-github-service's relay -- never GitHub's token endpoint directly. See
 * GITHUB_SETUP_SERVICE_PLAN.md for why: a public client (this app) can't safely hold the
 * `client_secret` a code/refresh exchange requires, so the relay holds it instead. Internal
 * plumbing behind [GitHubConnectionCoordinator]'s protocol surface -- a future Swift/GTK/WinUI
 * implementation reproduces this wire exchange with its own HTTP stack, not this file.
 */
internal object GitHubOAuth {
    fun buildAuthorizationUrl(state: String, codeChallenge: String): String =
        "https://github.com/login/oauth/authorize".toHttpUrl()
            .newBuilder()
            .addQueryParameter("client_id", GitHubEnvironment.CLIENT_ID)
            .addQueryParameter("redirect_uri", GitHubEnvironment.OAUTH_CALLBACK_URL)
            .addQueryParameter("state", state)
            .addQueryParameter("code_challenge", codeChallenge)
            .addQueryParameter("code_challenge_method", "S256")
            .build()
            .toString()

    suspend fun exchangeCode(client: OkHttpClient, code: String, codeVerifier: String): RelayResult =
        postRelay(client, "${GitHubEnvironment.RELAY_BASE_URL}/v1/github/exchange") {
            put("code", code)
            put("code_verifier", codeVerifier)
        }

    suspend fun refresh(client: OkHttpClient, refreshToken: String, refreshCap: String): RelayResult =
        postRelay(client, "${GitHubEnvironment.RELAY_BASE_URL}/v1/github/refresh") {
            put("refresh_token", refreshToken)
            put("refresh_cap", refreshCap)
        }

    private suspend fun postRelay(client: OkHttpClient, url: String, body: JSONObject.() -> Unit): RelayResult =
        withContext(Dispatchers.IO) {
            val payload = JSONObject().apply(body).toString()
            val request = Request.Builder()
                .url(url)
                .header("Content-Type", "application/json")
                // Both authorization-code exchange and refresh pass through this one path.
                // They are credential-spending operations, so make the body explicitly non-
                // replayable as a second line of defence alongside the dedicated client's no-
                // retry/no-redirect settings.
                .post(oneShotJsonBody(payload))
                .build()
            val call = client.newCall(request)
            // A continuation's cancellation handler runs at cancellation initiation, even
            // while this IO thread is inside execute() or reading the response body. An
            // ordinary Job completion handler cannot run until that blocking work returns.
            // Keep execution inline: worker completion must also mean transport/body cleanup
            // has finished, which is the connect admission barrier's shutdown guarantee.
            suspendCancellableCoroutine<Result<RelayResult>> { continuation ->
                continuation.invokeOnCancellation { call.cancel() }
                if (continuation.isActive) {
                    // Carry exceptions as values through the cancellable boundary so a late
                    // I/O exception after cancellation cannot escape as an unhandled resume.
                    continuation.resume(runCatching { postRelayBlocking(call, url) })
                }
            }.getOrThrow()
        }

    /** Genuinely blocking (OkHttp's synchronous `execute()`) -- [postRelay] installs a
     * cancellation hook on [call] before entering it, so detaching a connect operation also
     * interrupts the socket operation rather than leaving it alive behind a cancelled Job. */
    private fun postRelayBlocking(call: Call, url: String): RelayResult {
        val response = try {
            call.execute()
        } catch (e: UnknownHostException) {
            Log.w("GitHubOAuth", "postRelay: $url never reached (UnknownHostException)")
            return RelayResult.Failure(RelayError.NetworkError(e))
        } catch (e: ConnectException) {
            Log.w("GitHubOAuth", "postRelay: $url never reached (ConnectException)")
            return RelayResult.Failure(RelayError.NetworkError(e))
        } catch (e: IOException) {
            // Anything past connection establishment (a timeout awaiting the response, a
            // connection reset mid-read, ...) -- the request may already have been received
            // and processed. See "Transport-failure ambiguity": never assume this is safely
            // retryable.
            Log.w("GitHubOAuth", "postRelay: $url ambiguous transport failure")
            return RelayResult.Failure(RelayError.AmbiguousTransportFailure)
        }

        response.use { resp ->
            val responseBody = try {
                resp.body?.string().orEmpty()
            } catch (_: IOException) {
                Log.w("GitHubOAuth", "postRelay: $url response read failed (HTTP ${resp.code})")
                return RelayResult.Failure(RelayError.AmbiguousTransportFailure)
            }
            if (resp.code != 200) {
                val error = mapRelayErrorBody(resp.code, responseBody)
                Log.w("GitHubOAuth", "postRelay: $url HTTP ${resp.code} (${error::class.simpleName})")
                return RelayResult.Failure(error)
            }
            val json = parseObject(responseBody)
            val accessToken = json?.nonBlankString("access_token")
            val refreshToken = json?.nonBlankString("refresh_token")
            val refreshCap = json?.nonBlankString("refresh_cap")
            val expiresIn = json?.positiveInt("expires_in")
            val refreshExpiresIn = json?.positiveInt("refresh_token_expires_in")
            if (json == null || !json.hasOnlyKeys(
                    "access_token", "expires_in", "refresh_token", "refresh_cap", "refresh_token_expires_in",
                ) || accessToken == null || refreshToken == null || refreshCap == null ||
                expiresIn == null || refreshExpiresIn == null
            ) {
                Log.w("GitHubOAuth", "postRelay: $url invalid token response (HTTP ${resp.code})")
                return RelayResult.Failure(RelayError.InvalidResponse(resp.code))
            }
            Log.d("GitHubOAuth", "postRelay: $url succeeded")
            return RelayResult.Success(GitHubTokens(accessToken, expiresIn, refreshToken, refreshCap, refreshExpiresIn))
        }
    }

    /** Only a validated envelope with its pinned status can establish a reusable credential.
     * Unknown fields (including tokens mixed into an error) invalidate that guarantee. */
    internal fun mapRelayErrorBody(httpStatus: Int, body: String): RelayError {
        val invalid = RelayError.InvalidResponse(httpStatus)
        val json = parseObject(body) ?: return invalid
        return when (json.opt("error")) {
            "invalid_request" ->
                if (httpStatus == 400 && json.hasOnlyKeys("error")) RelayError.InvalidRequest else invalid
            "authorization_required" -> {
                if (httpStatus != 400 || !json.hasOnlyKeys("error", "reason")) return invalid
                val reason = when {
                    !json.has("reason") -> null
                    json.opt("reason") == "reconnect" -> null
                    json.opt("reason") == "verify_email" -> AuthorizationReason.VerifyEmail
                    else -> return invalid
                }
                RelayError.AuthorizationRequired(reason)
            }
            "rate_limited" -> {
                if (httpStatus != 429 || !json.hasOnlyKeys("error", "retry_after_seconds")) return invalid
                val retryAfter = if (json.has("retry_after_seconds")) {
                    json.nonNegativeInt("retry_after_seconds") ?: return invalid
                } else null
                RelayError.RateLimited(retryAfter)
            }
            "outcome_unknown" ->
                if (httpStatus == 503 && json.hasOnlyKeys("error")) RelayError.OutcomeUnknown else invalid
            "unexpected" ->
                if (httpStatus == 500 && json.hasOnlyKeys("error")) RelayError.Unexpected else invalid
            else -> invalid
        }
    }

    private fun parseObject(body: String): JSONObject? = try {
        if (!JSON_OBJECT.matches(body)) {
            null
        } else {
            val keys = JSON_FIELD.findAll(body).map {
                org.json.JSONTokener(it.groupValues[1]).nextValue() as String
            }.toList()
            if (keys.size != keys.toSet().size) null else JSONObject(body)
        }
    } catch (_: org.json.JSONException) {
        // Parser exception messages can contain the original token-bearing input.
        null
    }

    private fun JSONObject.hasOnlyKeys(vararg allowed: String): Boolean =
        keys().asSequence().all { it in allowed }

    private fun JSONObject.nonBlankString(key: String): String? =
        (opt(key) as? String)?.takeIf { it.isNotBlank() }

    private fun JSONObject.nonNegativeInt(key: String): Int? = when (val value = opt(key)) {
        is Int -> value.takeIf { it >= 0 }
        is Long -> value.takeIf { it in 0..Int.MAX_VALUE.toLong() }?.toInt()
        else -> null
    }

    private fun JSONObject.positiveInt(key: String): Int? = nonNegativeInt(key)?.takeIf { it > 0 }

    /**
     * OkHttp consults [RequestBody.isOneShot] before connection recovery and HTTP follow-up
     * handling.  Wrapping its ordinary byte-string body retains the exact JSON bytes and
     * content metadata while preventing a code or rotating refresh credential being sent twice.
     * Internal visibility keeps the transport guarantee directly regression-testable.
     */
    internal fun oneShotJsonBody(payload: String): RequestBody =
        object : RequestBody() {
            private val delegate = payload.toRequestBody(JSON_MEDIA_TYPE)

            override fun contentType() = delegate.contentType()

            override fun contentLength() = delegate.contentLength()

            override fun writeTo(sink: BufferedSink) = delegate.writeTo(sink)

            override fun isOneShot() = true
        }
}
