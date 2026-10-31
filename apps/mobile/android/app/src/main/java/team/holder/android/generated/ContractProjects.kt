// Generated from contract/interface/projects.py, maintenance.py and app.py by holder_contract.
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

object ContractProjects {
    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>
    external fun nativeCreateProject(contextHandle: Long, name: String, privacyMode: String?): ContractProject
    external fun nativeRenameProject(contextHandle: Long, projectId: String, name: String): ContractProject
    external fun nativeUpdateProjectGitRemote(contextHandle: Long, projectId: String, remoteUrl: String?): ContractProject
    external fun nativeCheckProjectEncryption(contextHandle: Long, projectId: String): ContractEncryptionCheck
    external fun nativeDeleteProject(contextHandle: Long, projectId: String)
}
