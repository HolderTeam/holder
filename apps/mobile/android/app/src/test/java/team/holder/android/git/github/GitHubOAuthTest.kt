package team.holder.android.git.github

import java.util.concurrent.TimeUnit
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.async
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import okhttp3.HttpUrl.Companion.toHttpUrl
import okhttp3.OkHttpClient
import okhttp3.Protocol
import okhttp3.Response
import okhttp3.ResponseBody
import okhttp3.ResponseBody.Companion.toResponseBody
import okio.Buffer
import okio.BufferedSource
import okio.ForwardingSource
import okio.Source
import okio.Timeout
import okio.buffer
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

class GitHubOAuthTest {
    @Test
    fun transportRetryabilityDependsOnCredentialWriteNotExceptionClass() = runBlocking {
        for (failure in listOf(
            javax.net.ssl.SSLHandshakeException("sensitive handshake diagnostic"),
            java.net.SocketTimeoutException("timeout"),
            java.net.UnknownHostException("dns"),
            java.net.ConnectException("connect"),
        )) {
            for (writeBody in listOf(false, true)) {
                val client = OkHttpClient.Builder().addInterceptor { chain ->
                    if (writeBody) chain.request().body!!.writeTo(Buffer())
                    throw failure
                }.build()
                val result = GitHubOAuth.refresh(client, "refresh", "cap")
                if (writeBody) {
                    assertEquals(RelayResult.Failure(RelayError.AmbiguousTransportFailure), result)
                } else {
                    assertTrue((result as RelayResult.Failure).error is RelayError.NetworkError)
                }
            }
        }
    }

    @Test
    fun failedTlsConnectionBeforeHttpIsRetryable() = runBlocking {
        val server = java.net.ServerSocket(0, 1, java.net.InetAddress.getByName("127.0.0.1"))
        server.soTimeout = 5000
        val peer = async(Dispatchers.IO) {
            server.accept().use { socket ->
                socket.getOutputStream().write("HTTP/1.1 502 Bad Gateway\r\n\r\n".toByteArray())
            }
        }
        try {
            val client = GitHubConnectionCoordinator.newCredentialRelayHttpClient().newBuilder()
                .proxy(java.net.Proxy.NO_PROXY)
                .callTimeout(5, TimeUnit.SECONDS)
                .addInterceptor { chain ->
                    chain.proceed(chain.request().newBuilder().url("https://127.0.0.1:${server.localPort}/refresh").build())
                }.build()
            val result = GitHubOAuth.refresh(client, "refresh", "cap") as RelayResult.Failure
            assertTrue(result.error is RelayError.NetworkError)
            assertTrue((result.error as RelayError.NetworkError).cause is javax.net.ssl.SSLException)
        } finally {
            server.close()
            peer.await()
        }
    }

    private fun tokenBody() = JSONObject()
        .put("access_token", "access_secret")
        .put("expires_in", 28800)
        .put("refresh_token", "refresh_secret")
        .put("refresh_cap", "cap_secret")
        .put("refresh_token_expires_in", 15811200)

    private suspend fun refreshResponse(status: Int, body: String): RelayResult {
        val client = OkHttpClient.Builder().addInterceptor { chain ->
            Response.Builder().request(chain.request()).protocol(Protocol.HTTP_1_1)
                .code(status).message("test response").body(body.toResponseBody()).build()
        }.build()
        return GitHubOAuth.refresh(client, "submitted_refresh", "submitted_cap")
    }

    @Test
    fun refreshAcceptsOnlyCompleteTypedSuccessAtHttp200() = runBlocking {
        assertEquals(
            RelayResult.Success(GitHubTokens("access_secret", 28800, "refresh_secret", "cap_secret", 15811200)),
            refreshResponse(200, tokenBody().toString()),
        )
        for (status in listOf(201, 204, 302, 400, 429, 500, 503)) {
            assertEquals(RelayResult.Failure(RelayError.InvalidResponse(status)), refreshResponse(status, tokenBody().toString()))
        }
    }

    @Test
    fun malformedSuccessFieldsCannotBecomeTokensOrAnOperationalFailure() = runBlocking {
        val invalidBodies = mutableListOf("", "{broken", "[]", "null", "{}", tokenBody().toString() + " trailing")
        for (field in listOf("access_token", "refresh_token", "refresh_cap", "expires_in", "refresh_token_expires_in")) {
            invalidBodies += tokenBody().apply { remove(field) }.toString()
            invalidBodies += tokenBody().put(field, JSONObject.NULL).toString()
            invalidBodies += tokenBody().put(field, true).toString()
            invalidBodies += tokenBody().put(field, JSONObject()).toString()
        }
        for (field in listOf("access_token", "refresh_token", "refresh_cap")) {
            for (value in listOf("", "  ", 17)) invalidBodies += tokenBody().put(field, value).toString()
        }
        for (field in listOf("expires_in", "refresh_token_expires_in")) {
            for (value in listOf(0, -1, 1.5, "28800", 2147483648L, Long.MAX_VALUE)) {
                invalidBodies += tokenBody().put(field, value).toString()
            }
        }
        invalidBodies += tokenBody().put("error", "unexpected").toString()
        for (body in invalidBodies) {
            assertEquals(body, RelayResult.Failure(RelayError.InvalidResponse(200)), refreshResponse(200, body))
        }
    }

    @Test
    fun operationalEnvelopesRequireExactFieldsTypesAndStatus() = runBlocking {
        val envelopes = listOf(
            Triple(400, "{\"error\":\"invalid_request\"}", RelayError.InvalidRequest),
            Triple(400, "{\"error\":\"authorization_required\",\"reason\":\"verify_email\"}", RelayError.AuthorizationRequired(AuthorizationReason.VerifyEmail)),
            Triple(429, "{\"error\":\"rate_limited\",\"retry_after_seconds\":30}", RelayError.RateLimited(30)),
            Triple(500, "{\"error\":\"unexpected\"}", RelayError.Unexpected),
            Triple(503, "{\"error\":\"outcome_unknown\"}", RelayError.OutcomeUnknown),
        )
        for ((expectedStatus, body, expectedError) in envelopes) {
            for (status in listOf(200, 201, 400, 401, 429, 500, 503)) {
                assertEquals(
                    "$status $body",
                    RelayResult.Failure(if (status == expectedStatus) expectedError else RelayError.InvalidResponse(status)),
                    refreshResponse(status, body),
                )
            }
            for (field in listOf("access_token", "refresh_token", "refresh_cap", "unknown_field")) {
                val mixedBody = JSONObject(body).put(field, "secret").toString()
                assertEquals(RelayError.InvalidResponse(expectedStatus), GitHubOAuth.mapRelayErrorBody(expectedStatus, mixedBody))
            }
        }
        for (value in listOf(JSONObject.NULL, "30", -1, 2.5, 2147483648L, true)) {
            val body = JSONObject().put("error", "rate_limited").put("retry_after_seconds", value).toString()
            assertEquals(RelayError.InvalidResponse(429), GitHubOAuth.mapRelayErrorBody(429, body))
        }
        for (body in listOf("{broken", "{}", "[]", "{\"error\":17}", "{\"error\":\"unknown\"}")) {
            assertEquals(RelayError.InvalidResponse(500), GitHubOAuth.mapRelayErrorBody(500, body))
        }
    }

    @Test
    fun longResponseStringsDoNotExhaustValidationStack() = runBlocking {
        val longCap = "a".repeat(MAX_RELAY_RESPONSE_BYTES / 2)
        val success = refreshResponse(200, tokenBody().put("refresh_cap", longCap).toString())
        assertTrue(success is RelayResult.Success)
        assertEquals(longCap, (success as RelayResult.Success).tokens.refreshCap)
        assertEquals(
            RelayError.InvalidResponse(500),
            GitHubOAuth.mapRelayErrorBody(500, "{\"error\":\"$longCap"),
        )
    }

    @Test
    fun responseLimitAcceptsExactBoundaryAndRejectsOversizedBodiesBeforeParsing() = runBlocking {
        val base = tokenBody().put("refresh_cap", "").toString()
        val exact = tokenBody().put("refresh_cap", "a".repeat(MAX_RELAY_RESPONSE_BYTES - base.length)).toString()
        assertEquals(MAX_RELAY_RESPONSE_BYTES, exact.toByteArray().size)
        assertTrue(refreshResponse(200, exact) is RelayResult.Success)
        for (status in listOf(200, 500)) {
            assertEquals(
                RelayResult.Failure(RelayError.InvalidResponse(status)),
                refreshResponse(status, exact + " "),
            )
        }
        assertEquals(
            RelayResult.Failure(RelayError.InvalidResponse(200)),
            refreshResponse(200, tokenBody().put("refresh_cap", "é".repeat(MAX_RELAY_RESPONSE_BYTES / 2)).toString()),
        )
    }

    @Test
    fun streamingResponseReadIsBoundedAndClosedForMissingOrMisleadingContentLength() = runBlocking {
        for (declaredLength in listOf(-1L, 1L, MAX_RELAY_RESPONSE_BYTES + 1L)) {
            var bytesRead = 0L
            var closed = false
            val stream = object : Source {
                override fun read(sink: Buffer, byteCount: Long): Long {
                    // An unending server response: reaching EOF is not needed to reject it.
                    val count = minOf(byteCount, 8192L).toInt()
                    sink.write(ByteArray(count) { 'a'.code.toByte() })
                    bytesRead += count
                    return count.toLong()
                }
                override fun timeout() = Timeout.NONE
                override fun close() { closed = true }
            }.buffer()
            val body = object : ResponseBody() {
                override fun contentType() = null
                override fun contentLength() = declaredLength
                override fun source(): BufferedSource = stream
            }
            val client = OkHttpClient.Builder().addInterceptor { chain ->
                Response.Builder().request(chain.request()).protocol(Protocol.HTTP_1_1)
                    .code(200).message("OK").body(body).build()
            }.build()
            assertEquals(RelayResult.Failure(RelayError.InvalidResponse(200)), GitHubOAuth.refresh(client, "refresh", "cap"))
            assertTrue(closed)
            if (declaredLength > MAX_RELAY_RESPONSE_BYTES) {
                assertEquals(0L, bytesRead)
            } else {
                assertTrue(bytesRead in (MAX_RELAY_RESPONSE_BYTES + 1L)..(MAX_RELAY_RESPONSE_BYTES + 8192L))
            }
        }
    }

    @Test
    fun invalidUtf8IsRejectedRatherThanReplacedInsideTokens() = runBlocking {
        val parts = tokenBody().put("refresh_cap", "ENCODING_MARKER").toString().split("ENCODING_MARKER")
        val body = parts[0].toByteArray() + byteArrayOf(0xc3.toByte(), 0x28) + parts[1].toByteArray()
        val client = OkHttpClient.Builder().addInterceptor { chain ->
            Response.Builder().request(chain.request()).protocol(Protocol.HTTP_1_1)
                .code(200).message("OK").body(body.toResponseBody()).build()
        }.build()
        assertEquals(RelayResult.Failure(RelayError.InvalidResponse(200)), GitHubOAuth.refresh(client, "refresh", "cap"))
    }

    @Test
    fun lenientJsonExtensionsCannotEstablishAnOperationalEnvelope() {
        for (body in listOf(
            "{error:unexpected}",
            "{'error':'unexpected'}",
            "{\"error\":\"unexpected\",}",
            "{\"error\":\"unexpected\"} trailing",
            "{\"error\":\"outcome_unknown\",\"error\":\"unexpected\"}",
            "{\"error\":\"unexpected\",\"err\\u006fr\":\"unexpected\"}",
            "{\"error\":/*comment*/\"unexpected\"}",
        )) {
            assertEquals(body, RelayError.InvalidResponse(500), GitHubOAuth.mapRelayErrorBody(500, body))
        }
        assertEquals(RelayError.Unexpected, GitHubOAuth.mapRelayErrorBody(500, " \n{ \"err\\u006fr\" : \"unexpected\" }\t"))
    }

    @Test
    fun responseBodyReadFailureIsAnAmbiguousOutcome() = runBlocking {
        val client = OkHttpClient.Builder().addInterceptor { chain ->
            val source = object : ForwardingSource(Buffer().writeUtf8("partial_secret")) {
                override fun read(sink: Buffer, byteCount: Long): Long = throw java.io.IOException("read failed")
            }.buffer()
            val body = object : ResponseBody() {
                override fun contentType() = null
                override fun contentLength() = -1L
                override fun source(): BufferedSource = source
            }
            Response.Builder().request(chain.request()).protocol(Protocol.HTTP_1_1)
                .code(200).message("OK").body(body).build()
        }.build()
        assertEquals(RelayResult.Failure(RelayError.AmbiguousTransportFailure), GitHubOAuth.refresh(client, "refresh", "cap"))
    }

    @Test
    fun authorizationUrlRetainsTheExactOAuthParametersUsedForBrowserResolutionAndLaunch() {
        val url = GitHubOAuth.buildAuthorizationUrl(
            state = "state with reserved + characters",
            codeChallenge = "challenge/value",
        ).toHttpUrl()

        assertEquals("https", url.scheme)
        assertEquals("github.com", url.host)
        assertEquals("/login/oauth/authorize", url.encodedPath)
        assertEquals(GitHubEnvironment.CLIENT_ID, url.queryParameter("client_id"))
        assertEquals(GitHubEnvironment.OAUTH_CALLBACK_URL, url.queryParameter("redirect_uri"))
        assertEquals("state with reserved + characters", url.queryParameter("state"))
        assertEquals("challenge/value", url.queryParameter("code_challenge"))
        assertEquals("S256", url.queryParameter("code_challenge_method"))
    }

    @Test
    fun cancellationReachesAnExecutingExchangeBeforeTheWorkerCanComplete() = assertActiveCallCancellation()

    @Test
    fun cancellationReachesAnExecutingRefreshBeforeTheWorkerCanComplete() = assertActiveCallCancellation(refresh = true)

    @Test
    fun cancellationRemainsConnectedDuringResponseBodyRead() = assertActiveCallCancellation(holdResponseBody = true)

    private fun assertActiveCallCancellation(refresh: Boolean = false, holdResponseBody: Boolean = false) = runBlocking {
        val transport = BlockedRelayCall(holdResponseBody)
        val worker = launch(Dispatchers.IO) {
            try {
                if (refresh) GitHubOAuth.refresh(transport.client, "refresh", "cap")
                else GitHubOAuth.exchangeCode(transport.client, "code", "verifier")
            } catch (_: java.io.IOException) {
                // The held response source throws when released, including during cleanup
                // of a failed cancellation assertion. Do not mask that assertion with I/O.
            }
        }
        try {
            assertTrue("worker never entered synchronous HTTP I/O", transport.entered.await(5, TimeUnit.SECONDS))
            assertTrue(transport.call!!.isExecuted())
            worker.cancel()

            assertFalse("worker must still be executing the held call", worker.isCompleted)
            assertTrue("cancellation must reach the real Call before execute/body read returns", transport.call!!.isCanceled())
            assertEquals(1, transport.cancellations.get())
            assertEquals(1, transport.requests.get())
        } finally {
            transport.release.countDown()
            worker.cancel()
            worker.join()
        }
    }

    @Test
    fun relayCredentialRequestBody_isOneShot() {
        // exchangeCode() and refresh() both reach postRelayBlocking(), which creates its body
        // through this shared helper.  A one-shot body prevents OkHttp from retransmitting
        // either an authorization code or a rotating refresh credential on connection recovery
        // or an HTTP follow-up.
        assertTrue(GitHubOAuth.oneShotJsonBody("{\"credential\":\"secret\"}").isOneShot())
    }

    @Test
    fun relayCredentialRequestBody_preservesTheJsonPayload() {
        val payload = "{\"code\":\"one-time-code\"}"
        val sink = Buffer()

        GitHubOAuth.oneShotJsonBody(payload).writeTo(sink)

        assertEquals(payload, sink.readUtf8())
    }

    @Test
    fun credentialRelayClient_outlivesTheWorkersUpstreamDeadline() {
        val client = GitHubConnectionCoordinator.newCredentialRelayHttpClient()

        // The Worker has a 15-second complete upstream deadline.  Both bounds matter: a read
        // limit must not cut off a slow but live response, and the overall call must still
        // leave time for Android↔relay transport around that upstream operation.
        assertEquals(20_000, client.readTimeoutMillis)
        assertEquals(20_000, client.callTimeoutMillis)
    }

    @Test
    fun relayErrorEnvelope_requiresItsPinnedHttpStatusBeforeItCanDriveAnAuthorizationAction() {
        val body = "{\"error\":\"authorization_required\",\"reason\":\"reconnect\"}"

        assertEquals(RelayError.AuthorizationRequired(null), GitHubOAuth.mapRelayErrorBody(400, body))
        assertEquals(RelayError.InvalidResponse(503), GitHubOAuth.mapRelayErrorBody(503, body))
        assertEquals(RelayError.InvalidResponse(400), GitHubOAuth.mapRelayErrorBody(400, "{\"error\":\"authorization_required\"}"))
    }

    @Test
    fun relayRateLimitAndAmbiguityEnvelopes_requireTheirOwnPinnedStatuses() {
        val rateBody = "{\"error\":\"rate_limited\",\"retry_after_seconds\":30}"
        val unknownBody = "{\"error\":\"outcome_unknown\"}"

        assertEquals(RelayError.RateLimited(30), GitHubOAuth.mapRelayErrorBody(429, rateBody))
        assertEquals(RelayError.InvalidResponse(429), GitHubOAuth.mapRelayErrorBody(429, "{\"error\":\"rate_limited\"}"))
        assertEquals(RelayError.InvalidResponse(503), GitHubOAuth.mapRelayErrorBody(503, rateBody))
        assertEquals(RelayError.OutcomeUnknown, GitHubOAuth.mapRelayErrorBody(503, unknownBody))
        assertEquals(RelayError.InvalidResponse(400), GitHubOAuth.mapRelayErrorBody(400, unknownBody))
    }
}
