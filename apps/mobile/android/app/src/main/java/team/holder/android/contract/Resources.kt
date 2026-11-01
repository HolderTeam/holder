// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

/** How the copy is stored: as the file is, or encrypted for the project. */
enum class AssetPlacementEncoding(val value: String) {
    PLAIN("plain"),
    HOLDER_ASSET_V1("holder_asset_v1"),
}

/** One stored copy of an asset. */
data class AssetPlacement(
    /** The copy's ID. */
    val placementId: String,
    /** The storage location that holds the copy. */
    val locationId: String,
    /** How the copy is stored: as the file is, or encrypted for the project. */
    val encoding: AssetPlacementEncoding,
    /** The size of the stored copy, in bytes. */
    val storedByteSize: Long,
    /** When the copy was stored. */
    val createdAt: Long,
)

/** A file that belongs to a resource. */
data class ResourceAsset(
    /** The asset's ID. */
    val assetId: String,
    /** The file's name when it was imported. */
    val originalFilename: String,
    /** The file's media type, such as application/pdf. */
    val mediaType: String,
    /** The file's size, in bytes. */
    val byteSize: Long,
    /** The SHA-256 of the file's content, in lower-case hex. */
    val plaintextSha256: String,
    /** The stored copies of the file. */
    val placements: List<AssetPlacement>,
    /** When the asset was imported. */
    val createdAt: Long,
    /** When the asset was last changed. */
    val updatedAt: Long,
)

/** A resource in a project, with its files. */
data class Resource(
    /** The resource's ID. */
    val resourceId: String,
    /** The project the resource belongs to. */
    val projectId: String,
    /** What kind of thing the resource is. */
    val type: String,
    /** The resource's name. */
    val label: String,
    /** What is known about the resource, by name. */
    val metadata: Map<String, List<String>>,
    /** The resource's files. */
    val assets: List<ResourceAsset>,
    /** When the resource was made. */
    val createdAt: Long,
    /** When the resource was last changed. */
    val updatedAt: Long,
)

/** A resource to make. */
data class NewResource(
    /** The resource's ID, for imports; omit for a new one. */
    val resourceId: String? = null,
    /** What kind of thing the resource is. */
    val type: String,
    /** The resource's name. */
    val label: String,
    /** What is known about the resource; omit for nothing. A name without values is left out. */
    val metadata: Map<String, List<String>>? = null,
    /** When the resource was made, for imports; omit for now. */
    val createdAt: Long? = null,
    /** When the resource was last changed, for imports; omit for when it was made. */
    val updatedAt: Long? = null,
)

/** A change to a resource. Each field left out keeps its value. */
data class ResourceUpdate(
    /** What kind of thing the resource is. */
    val type: String? = null,
    /** The resource's name. */
    val label: String? = null,
    /** Names whose values change: each name's values replace its old ones, an empty list removes the name, and names left out are kept. */
    val metadata: Map<String, List<String>>? = null,
    /** When the resource was changed, for imports; omit for now. */
    val updatedAt: Long? = null,
)

/** The resources operations of core's C API. Each fails with a [ContractException]. */
object Resources {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * List a project's resources.
     * By when they last changed, latest first, then by ID.
     */
    external fun listResources(
        contextHandle: Long,
        projectId: String,
    ): List<Resource>

    /**
     * Fetch a resource.
     * A resource in another project is not found.
     */
    external fun getResource(
        contextHandle: Long,
        projectId: String,
        resourceId: String,
    ): Resource

    /**
     * Make a resource.
     * A project that does not exist is not found. Resource IDs are unique on this device, so an ID already in use,
     * in this project or another, is a Conflict.
     */
    external fun createResource(
        contextHandle: Long,
        projectId: String,
        resource: NewResource,
    ): Resource

    /**
     * Change a resource.
     * Each field left out keeps its value. A resource in another project is not found.
     */
    external fun updateResource(
        contextHandle: Long,
        projectId: String,
        resourceId: String,
        update: ResourceUpdate,
    ): Resource

    /**
     * Delete a resource.
     * The resource goes, and so do the links to it from cards. Its files' stored copies are left where they are.
     * Returns the resource as it was. A resource in another project is not found.
     */
    external fun deleteResource(
        contextHandle: Long,
        projectId: String,
        resourceId: String,
    ): Resource

    /**
     * List the links to a resource.
     * Links of every kind, attachments among them, from cards and AI messages in the resource's project, newest
     * first. A resource in another project is not found.
     */
    external fun listResourceBacklinks(
        contextHandle: Long,
        projectId: String,
        resourceId: String,
        includeDeleted: Boolean = false,
    ): List<Link>

    /**
     * List the resources attached to a card.
     * By when they last changed, latest first, then by ID. A card in the trash or in another project is not found.
     */
    external fun listCardResources(
        contextHandle: Long,
        projectId: String,
        cardId: String,
    ): List<Resource>

    /**
     * Attach a resource to a card.
     * An attachment is the card's link of kind attachment to the resource. A card that has it already is left as it
     * is. Returns the card's attached resources, as list_card_resources lists them. A card in the trash or in another
     * project is not found, and neither is a resource that is not in the card's project.
     */
    external fun attachCardResource(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        resourceId: String,
    ): List<Resource>

    /**
     * Detach a resource from a card.
     * The resource itself is kept. A card without the attachment is left as it is, and a resource that no longer
     * exists can still be detached. Returns the card's attached resources, as list_card_resources lists them. A card
     * in the trash or in another project is not found.
     */
    external fun detachCardResource(
        contextHandle: Long,
        projectId: String,
        cardId: String,
        resourceId: String,
    ): List<Resource>
}
