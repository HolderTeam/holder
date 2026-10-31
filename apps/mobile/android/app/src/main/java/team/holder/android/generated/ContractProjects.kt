// Generated from contract/interface/projects.py and contract/interface/app.py by holder_contract.
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

object ContractProjects {
    external fun nativeListProjectsPage(contextHandle: Long, offset: Int): List<ContractProjectListItem>
}
