// Generated from contract/interface/projects.py, maintenance.py, git.py and app.py by holder_contract.
// Do not edit; run make -C contract update.
package team.holder.android.generated

data class ContractProjectListItem(
    val projectId: String,
    val name: String,
    val rootPath: String,
    val gitRemoteUrl: String?,
    val gitProvider: String?,
    val privacyMode: String,
    val projectKeyId: String?,
    val createdAt: Long,
    val updatedAt: Long,
    val cardCount: Int?,
    val rootCardCount: Int?,
)

data class ContractProject(
    val projectId: String,
    val name: String,
    val rootPath: String,
    val gitRemoteUrl: String?,
    val gitProvider: String?,
    val privacyMode: String,
    val projectKeyId: String?,
    val createdAt: Long,
    val updatedAt: Long,
)

data class ContractNewProject(
    val projectId: String? = null,
    val name: String,
    val rootPath: String? = null,
    val gitRemoteUrl: String? = null,
    val gitProvider: String? = null,
    val privacyMode: String = "encrypted_git",
    val createdAt: Long? = null,
    val updatedAt: Long? = null,
)

data class ContractProjectRecoveryTokenExportResult(
    val projectId: String,
    val keyId: String,
    val recoveryToken: String,
)

data class ContractRecoveryTokenGlobalImportResult(
    val projectId: String,
    val projectCreated: Boolean,
    val remoteHintPresent: Boolean,
    val remoteConfigured: Boolean,
    val remoteError: String?,
    val pullStatus: String,
    val pullError: String?,
)

data class ContractEncryptionCheck(
    val projectId: String,
    val privacyMode: String,
    val ok: Boolean,
    val checkedFiles: Int,
    val unsafeFiles: Int,
    val unsafePaths: List<String>,
    val message: String,
)

data class ContractSyncState(
    val projectId: String,
    val lastCommitAt: Long?,
    val lastPushAt: Long?,
    val lastPullAt: Long?,
    val uncommittedChangesCount: Int,
    val unpushedCommitsCount: Int,
    val lastPushStatus: String?,
    val lastPullStatus: String?,
    val lastSyncError: String?,
    val lastSyncErrorAt: Long?,
    val retryCount: Int,
    val nextRetryAt: Long?,
    val pullRetryCount: Int,
    val nextPullRetryAt: Long?,
    val updatedAt: Long?,
)

data class ContractPullPhase(
    val attempted: Boolean,
    val status: String,
    val conflictsResolved: Int,
    val hiddenCardsRepaired: Int,
    val errorMessage: String?,
)

data class ContractPushPhase(
    val status: String,
    val aheadCount: Int,
    val behindCount: Int,
    val errorMessage: String?,
)

data class ContractRemoteTestResult(
    val status: String,
    val remoteHasHead: Boolean,
    val errorMessage: String?,
)

data class ContractSyncNowResult(
    val pullAttempted: Boolean,
    val pullStatus: String?,
    val pullError: String?,
    val pullConflictsResolved: Int,
    val pushAttempted: Boolean,
    val pushStatus: String?,
    val pushError: String?,
    val pushAheadCount: Int,
    val pushBehindCount: Int,
    val pushLocalHeadCommit: String?,
)

object ContractProjects {
    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>
    external fun nativeGetProject(contextHandle: Long, projectId: String): ContractProject
    external fun nativeCreateProject(contextHandle: Long, project: ContractNewProject): ContractProject
    external fun nativeRenameProject(contextHandle: Long, projectId: String, name: String): ContractProject
    external fun nativeUpdateProjectGitRemote(contextHandle: Long, projectId: String, remoteUrl: String?): ContractProject
    external fun nativeCheckProjectEncryption(contextHandle: Long, projectId: String): ContractEncryptionCheck
    external fun nativeGetProjectSyncState(contextHandle: Long, projectId: String): ContractSyncState
    external fun nativePullProject(contextHandle: Long, projectId: String): ContractPullPhase
    external fun nativePushProject(contextHandle: Long, projectId: String, branch: String?, setUpstream: Boolean): ContractPushPhase
    external fun nativeTestProjectRemote(contextHandle: Long, projectId: String, remoteUrl: String?): ContractRemoteTestResult
    external fun nativeSyncProject(contextHandle: Long, projectId: String, branch: String?, push: Boolean, setUpstream: Boolean): ContractSyncNowResult
    external fun nativeSyncProjectIfDue(contextHandle: Long, projectId: String, pushIntervalSeconds: Int, pullIntervalSeconds: Int): ContractSyncNowResult
    external fun nativeDeleteProject(contextHandle: Long, projectId: String)
    external fun nativeExportRecoveryToken(contextHandle: Long, projectId: String, pin: String): ContractProjectRecoveryTokenExportResult
    external fun nativeImportRecoveryToken(contextHandle: Long, projectId: String, pin: String, recoveryToken: String)
    external fun nativeImportRecoveryTokenGlobal(contextHandle: Long, pin: String, recoveryToken: String): ContractRecoveryTokenGlobalImportResult
}
