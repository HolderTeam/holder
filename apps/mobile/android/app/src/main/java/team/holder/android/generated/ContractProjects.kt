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

object ContractProjects {
    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>
    external fun nativeCreateProject(contextHandle: Long, name: String, privacyMode: String?): ContractProject
    external fun nativeRenameProject(contextHandle: Long, projectId: String, name: String): ContractProject
    external fun nativeUpdateProjectGitRemote(contextHandle: Long, projectId: String, remoteUrl: String?): ContractProject
    external fun nativeCheckProjectEncryption(contextHandle: Long, projectId: String): ContractEncryptionCheck
    external fun nativeGetProjectSyncState(contextHandle: Long, projectId: String): ContractSyncState
    external fun nativePullProject(contextHandle: Long, projectId: String): ContractPullPhase
    external fun nativeDeleteProject(contextHandle: Long, projectId: String)
}
