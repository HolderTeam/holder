// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

/** Outcome of the attempted pull. */
enum class RecoveryTokenGlobalImportResultPullStatus(val value: String) {
    NOT_ATTEMPTED("not_attempted"),
    SUCCEEDED("succeeded"),
    FAILED("failed"),
}

/** A portable encrypted project key. */
data class ProjectRecoveryTokenExportResult(
    /** The project whose key was exported. */
    val projectId: String,
    /** The encrypted project's key ID. */
    val keyId: String,
    /** Serialized .hrk token JSON. */
    val recoveryToken: String,
)

/** The project whose key was restored. */
data class ProjectRecoveryTokenImportResult(
    /** The project whose key was restored. */
    val projectId: String,
)

/** The result of restoring a token to this device. */
data class RecoveryTokenGlobalImportResult(
    /** The project whose key was restored. */
    val projectId: String,
    /** Whether this device made the project from the token. */
    val projectCreated: Boolean,
    /** Whether the token names a Git remote. */
    val remoteHintPresent: Boolean,
    /** Whether that remote was configured. */
    val remoteConfigured: Boolean,
    /** Why remote configuration failed, or null. */
    val remoteError: String? = null,
    /** Outcome of the attempted pull. */
    val pullStatus: RecoveryTokenGlobalImportResultPullStatus,
    /** Why pulling failed, or null. */
    val pullError: String? = null,
)

/** PIN used to wrap the project key. */
data class RecoveryTokenExportRequest(
    /** User-provided PIN used to wrap or unwrap the key. */
    val pin: String,
)

/** PIN and serialized .hrk token to restore. */
data class RecoveryTokenImportRequest(
    /** User-provided PIN used to wrap or unwrap the key. */
    val pin: String,
    /** Serialized .hrk token JSON. */
    val recoveryToken: String,
)

/** The recovery token or its PIN is invalid, or the token belongs to another project. */
class RecoveryTokenInvalidException(message: String) : ContractException(4001, message)

/** The recovery operations of core's C API. Each fails with a [ContractException]. */
object Recovery {
    init {
        System.loadLibrary("holder_jni")
    }

    /** Wrap an encrypted project's key in a portable token protected by a PIN. */
    external fun exportProjectRecoveryToken(
        contextHandle: Long,
        projectId: String,
        request: RecoveryTokenExportRequest,
    ): ProjectRecoveryTokenExportResult

    /** Restore an encrypted project's key from a token for that project. */
    external fun importProjectRecoveryToken(
        contextHandle: Long,
        projectId: String,
        request: RecoveryTokenImportRequest,
    ): ProjectRecoveryTokenImportResult

    /**
     * Restore a token, finding its project here or making one from the token's metadata.
     * If the token names a Git remote, configure it and attempt a pull. Remote and pull failures
     * are reported in the result. Returns 201 when the project was created, otherwise 200.
     */
    external fun importRecoveryTokenGlobal(
        contextHandle: Long,
        request: RecoveryTokenImportRequest,
    ): RecoveryTokenGlobalImportResult
}
