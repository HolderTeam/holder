package team.holder.android.ui

import android.content.Context
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.platform.LocalContext
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import team.holder.android.network.LocalNetworkAccess

/**
 * Asks for [LocalNetworkAccess.PERMISSION] just before a screen connects to a host that may be
 * on the local network, so the connection works instead of timing out. Android files the
 * permission under "Nearby devices" and its prompt speaks of finding devices and their
 * position, so Holder first explains what it needs it for ([LocalNetworkPermissionDialog]).
 * Create with [rememberLocalNetworkPermission].
 */
class LocalNetworkPermission internal constructor(private val context: Context) {
    internal var launcher: ActivityResultLauncher<String>? = null
    private var pending: CompletableDeferred<Boolean>? = null

    /** The host the explanation dialog is showing for, while it is showing. */
    internal var explainingHost by mutableStateOf<String?>(null)
        private set
    private var explanationAnswer: CompletableDeferred<Boolean>? = null

    /** Null when [host] can be reached: no host, not local, already allowed, or the user
     * allows it now. Otherwise the message to show, and the caller should not connect. */
    suspend fun ensureAllowed(host: String?): String? {
        if (host == null || LocalNetworkAccess.isGranted(context)) return null
        if (!withContext(Dispatchers.IO) { LocalNetworkAccess.mayBeLocal(host) }) return null
        val launcher = launcher ?: return LocalNetworkAccess.BLOCKED_MESSAGE
        if (!explain(host)) return LocalNetworkAccess.BLOCKED_MESSAGE
        // One request at a time: a newer one supersedes a request whose result never came
        // back (the Activity was recreated while the system dialog was showing). Completing
        // rather than cancelling lets the superseded caller finish its own cleanup.
        pending?.complete(false)
        val result = CompletableDeferred<Boolean>().also { pending = it }
        launcher.launch(LocalNetworkAccess.PERMISSION)
        return if (result.await()) null else LocalNetworkAccess.BLOCKED_MESSAGE
    }

    private suspend fun explain(host: String): Boolean {
        explanationAnswer?.complete(false)
        val answer = CompletableDeferred<Boolean>().also { explanationAnswer = it }
        explainingHost = host
        return answer.await()
    }

    internal fun answerExplanation(proceed: Boolean) {
        explainingHost = null
        explanationAnswer?.complete(proceed)
        explanationAnswer = null
    }

    internal fun onResult(granted: Boolean) {
        pending?.complete(granted)
        pending = null
    }
}

/** A [LocalNetworkPermission] bound to this screen's permission request registration. The
 * screen must also show [LocalNetworkPermissionDialog] for it. */
@Composable
fun rememberLocalNetworkPermission(): LocalNetworkPermission {
    val context = LocalContext.current
    val permission = remember(context) { LocalNetworkPermission(context) }
    permission.launcher = rememberLauncherForActivityResult(ActivityResultContracts.RequestPermission()) {
        permission.onResult(it)
    }
    return permission
}

/** Explains the permission before Android's own prompt; shown only while [permission] asks. */
@Composable
fun LocalNetworkPermissionDialog(permission: LocalNetworkPermission) {
    val host = permission.explainingHost ?: return
    AlertDialog(
        onDismissRequest = { permission.answerExplanation(false) },
        title = { Text("Allow access to your local network?") },
        text = {
            Text(
                "$host looks like a device on your local network. Android only lets apps connect " +
                    "to those with the \"Nearby devices\" permission. Android's prompt also mentions " +
                    "finding devices and their position; Holder only uses it to connect to the " +
                    "addresses you enter.",
            )
        },
        confirmButton = { TextButton(onClick = { permission.answerExplanation(true) }) { Text("Continue") } },
        dismissButton = { TextButton(onClick = { permission.answerExplanation(false) }) { Text("Not now") } },
    )
}
