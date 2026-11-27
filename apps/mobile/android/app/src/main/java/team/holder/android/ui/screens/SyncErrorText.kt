package team.holder.android.ui.screens

/** What kind of failure a sync error message is, for telling the user what it means and what
 * (if anything) to do. Core passes pull failures through as libgit2's own text, so this reads
 * the text, with the same patterns as core's classify_push_error (GitRepo.cpp) plus the wording
 * Android's resolver and sockets use when there's no network. Checked in core's order: a
 * rejected key mentions "denied" even when it also mentions the host. */
enum class SyncErrorKind {
    /** No network, or the host can't be reached: nothing for the user to fix. */
    OFFLINE,

    /** GitHub (or the remote) refused this device's key. */
    AUTH,

    /** The repository isn't there, or this device's key can't see it. */
    NOT_FOUND,

    OTHER,
}

private val AUTH_TEXT = listOf("authentication", "permission denied", "publickey", "access denied")
private val NOT_FOUND_TEXT = listOf("repository not found", "not found")
private val OFFLINE_TEXT = listOf(
    "could not resolve host",
    "failed to resolve address",
    "no address associated",
    "network is unreachable",
    "connection refused",
    "connection reset",
    "failed to connect",
    "software caused connection abort",
    "timed out",
    "timeout",
)

fun syncErrorKind(message: String): SyncErrorKind {
    val text = message.lowercase()
    return when {
        AUTH_TEXT.any { it in text } -> SyncErrorKind.AUTH
        NOT_FOUND_TEXT.any { it in text } -> SyncErrorKind.NOT_FOUND
        OFFLINE_TEXT.any { it in text } -> SyncErrorKind.OFFLINE
        else -> SyncErrorKind.OTHER
    }
}

/** One plain sentence for [kind]; the raw message is shown separately, as detail. */
fun syncErrorSummary(kind: SyncErrorKind): String = when (kind) {
    SyncErrorKind.OFFLINE -> "No connection. Holder will sync when this phone is back online."
    SyncErrorKind.AUTH -> "GitHub didn't accept this device. Register it with GitHub again."
    SyncErrorKind.NOT_FOUND -> "The repository wasn't found, or this device can't see it."
    SyncErrorKind.OTHER -> "Sync didn't finish."
}

/** Registering the device's key again helps only when the remote refused it or can't see the
 * repository: never when the phone is offline. */
val SyncErrorKind.offersRegisterDevice: Boolean
    get() = this == SyncErrorKind.AUTH || this == SyncErrorKind.NOT_FOUND

/** The line Sync now leaves on the Sync screen: what happened. Why it failed is the screen's
 * last-error line just above ([syncErrorSummary]), so it isn't repeated here. A pull that fails
 * skips the push. */
fun syncNowOutcome(
    pullAttempted: Boolean,
    pullError: String?,
    pushAttempted: Boolean,
    pushError: String?,
    conflictsSuffix: String,
): String = when {
    pullAttempted && pullError != null -> "Couldn't sync."
    pushAttempted && pushError != null -> "Brought in changes$conflictsSuffix, but couldn't send this phone's."
    !pullAttempted && !pushAttempted -> "Nothing to sync."
    else -> "Synced$conflictsSuffix."
}
