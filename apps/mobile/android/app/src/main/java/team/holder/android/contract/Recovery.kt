// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

/** What the link points at. Restore keeps links to cards and resources and drops the rest. */
enum class RecoverySnapshotLinkTargetType(val value: String) {
    CARD("card"),
    AI_MESSAGE("ai_message"),
    AI_THREAD("ai_thread"),
    RESOURCE("resource"),
}

/** How the copy's bytes are stored. */
enum class RecoverySnapshotPlacementEncoding(val value: String) {
    PLAIN("plain"),
    HOLDER_ASSET_V1("holder_asset_v1"),
}

/** Storage provider kind. */
enum class RecoverySnapshotLocationProvider(val value: String) {
    LOCAL_DIRECTORY("local_directory"),
    S3_COMPATIBLE("s3_compatible"),
    GOOGLE_DRIVE("google-drive"),
}

/** That project's privacy mode. */
enum class RecoverySnapshotCardPrivacyMode(val value: String) {
    ENCRYPTED_GIT("encrypted_git"),
    PLAIN("plain"),
}

/** Outcome of the attempted pull. */
enum class RecoveryTokenGlobalImportResultPullStatus(val value: String) {
    NOT_ATTEMPTED("not_attempted"),
    SUCCEEDED("succeeded"),
    FAILED("failed"),
}

/** The new project's privacy mode. */
enum class RecoverySnapshotRestorePrivacyMode(val value: String) {
    ENCRYPTED_GIT("encrypted_git"),
    PLAIN("plain"),
}

/** A card's link, as a snapshot holds it. */
data class RecoverySnapshotLink(
    /** What the link points at. Restore keeps links to cards and resources and drops the rest. */
    val targetType: RecoverySnapshotLinkTargetType,
    /** What the link points at, by its ID when the snapshot was read. */
    val targetId: String,
    /** For a card in another project, that project; restore keeps such a link as it is. */
    val targetProjectId: String? = null,
    /** What the link means. */
    val kind: String,
    /** The link's label; null when it has none. */
    val label: String? = null,
)

/** A card's milestone, as a snapshot holds it. */
data class RecoverySnapshotMilestone(
    /** The milestone's dates. */
    val dates: List<MilestoneDate>,
    /** What the milestone is; null when unset. */
    val kind: String? = null,
    /** More about the milestone; null when unset. */
    val description: String? = null,
)

/** Where a stored copy of an asset's bytes is, so a restored resource can still be read. */
data class RecoverySnapshotPlacement(
    /** The placement's ID when the snapshot was read; restore gives it a new one. */
    val placementId: String,
    /** The storage location holding the copy, one of the snapshot card's locations. */
    val locationId: String,
    /** The copy's key within its storage location. */
    val objectKey: String,
    /** How the copy's bytes are stored. */
    val encoding: RecoverySnapshotPlacementEncoding,
    /** The stored copy's size in bytes. */
    val storedByteSize: Long,
    /** The SHA-256 of the stored copy's bytes, as hex. */
    val storedSha256: String,
    /** When the copy was stored. */
    val createdAt: Long,
)

/** A file kept for a resource, as a snapshot holds it. */
data class RecoverySnapshotAsset(
    /** The asset's ID when the snapshot was read; restore gives it a new one. */
    val assetId: String,
    /** The file's name when it was imported. */
    val originalFilename: String,
    /** The file's media type. */
    val mediaType: String,
    /** The file's size in bytes. */
    val byteSize: Long,
    /** The SHA-256 of the file's bytes, as hex. */
    val plaintextSha256: String,
    /** Where stored copies of the file are. */
    val placements: List<RecoverySnapshotPlacement>,
    /** When the asset was imported. */
    val createdAt: Long,
    /** When the asset last changed. */
    val updatedAt: Long,
)

/** A resource a card links to, as a snapshot holds it. */
data class RecoverySnapshotResource(
    /** The resource's ID when the snapshot was read; restore gives it a new one and updates links and holder://resource/ references in content to it. */
    val resourceId: String,
    /** What kind of thing the resource is. */
    val type: String,
    /** The resource's label. */
    val label: String,
    /** The resource's metadata. */
    val metadata: Map<String, List<String>>,
    /** The files kept for the resource. */
    val assets: List<RecoverySnapshotAsset>,
    /** When the resource was made. */
    val createdAt: Long,
    /** When the resource last changed. */
    val updatedAt: Long,
)

/** A storage location a card's resources use, as a snapshot holds it: its portable declaration only. */
data class RecoverySnapshotLocation(
    /** The location's ID when the snapshot was read; restore gives it a new one. */
    val locationId: String,
    /** Display name. */
    val name: String,
    /** Storage provider kind. */
    val provider: RecoverySnapshotLocationProvider,
    /** Portable non-secret provider configuration. */
    val configuration: Map<String, String>,
    /** When the declaration was made. */
    val createdAt: Long,
    /** When the declaration last changed. */
    val updatedAt: Long,
)

/** A card, as a snapshot holds it. */
data class RecoverySnapshotCard(
    /** The card's ID when the snapshot was read. Restore gives every card a new ID and uses this one only to keep links and the card tree between the cards it restores. */
    val cardId: String,
    /** The project the card was read from. */
    val projectId: String,
    /** That project's name. */
    val projectName: String,
    /** That project's privacy mode. */
    val privacyMode: RecoverySnapshotCardPrivacyMode,
    /** The card's title. */
    val title: String,
    /** The card's body. */
    val content: String,
    /** The card's parent; null for a root card. Restore keeps a parent it also restores, and otherwise makes the card a root. */
    val parentCardId: String? = null,
    /** The card's place among its siblings; lower comes first. */
    val sortKey: Double,
    /** When the card was created. */
    val createdAt: Long,
    /** When the card was last changed. */
    val updatedAt: Long,
    /** The card's outgoing links. */
    val links: List<RecoverySnapshotLink>,
    /** The card's milestones. */
    val milestones: List<RecoverySnapshotMilestone>,
    /** The resources the card links to. */
    val resources: List<RecoverySnapshotResource>,
    /** The storage locations those resources' assets are placed in. */
    val locations: List<RecoverySnapshotLocation>,
)

/** One page of a project's recovery snapshot. */
data class RecoverySnapshotPage(
    /** The page's cards, most recently updated first. */
    val cards: List<RecoverySnapshotCard>,
    /** Each card in core's encoding, in the same order: what a platform stores and later restores. */
    val lines: List<String>,
    /** Pass back to read the next page; null on the last page. */
    val nextCursor: String? = null,
)

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

/** A snapshot's cards to restore into a new project. */
data class RecoverySnapshotRestore(
    /** The new project's name. */
    val projectName: String,
    /** The new project's privacy mode. */
    val privacyMode: RecoverySnapshotRestorePrivacyMode,
    /** The cards' lines from read_recovery_snapshot, one per line. */
    val snapshot: String,
    /** The message of the commit that adds the restored cards. */
    val commitMessage: String,
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

    /**
     * Read one page of a project's recovery snapshot: its live cards, most recently updated first.
     * Each card comes as a record and as a line in core's encoding; a platform keeps the lines and
     * later hands them to restore_recovery_snapshot. The daemon does not serve this.
     */
    external fun readRecoverySnapshot(
        contextHandle: Long,
        projectId: String,
        cursor: String? = null,
        limit: Int = 500,
    ): RecoverySnapshotPage

    /**
     * Make a new project from a recovery snapshot's lines and commit the restored cards to it.
     * Cards, resources, assets, placements and storage locations get new IDs; links between the restored
     * cards and resources, holder://resource/ references in content and the card tree follow them. Lines
     * are restored in any order. A line core cannot read fails the restore and leaves no project behind.
     * The daemon does not serve this.
     */
    external fun restoreRecoverySnapshot(
        contextHandle: Long,
        request: RecoverySnapshotRestore,
    ): Project

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
