// Generated from contract/interface/resources.py and contract/interface/operations/resources.py by holder_contract.
// Do not edit; run make -C contract update.
package team.holder.android.generated

data class ContractAssetPlacement(
    val placementId: String,
    val locationId: String,
    val encoding: String,
    val storedByteSize: Long,
    val createdAt: Long,
)

data class ContractResourceAsset(
    val assetId: String,
    val originalFilename: String,
    val mediaType: String,
    val byteSize: Long,
    val plaintextSha256: String,
    val placements: List<ContractAssetPlacement>,
    val createdAt: Long,
    val updatedAt: Long,
)

data class ContractResource(
    val resourceId: String,
    val projectId: String,
    val type: String,
    val label: String,
    val assets: List<ContractResourceAsset>,
    val createdAt: Long,
    val updatedAt: Long,
)

object ContractResources {
    external fun nativeListResources(contextHandle: Long, projectId: String): List<ContractResource>
}
