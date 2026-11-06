// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

/** Storage provider kind. */
enum class ProjectStorageLocationProvider(val value: String) {
    LOCAL_DIRECTORY("local_directory"),
    S3_COMPATIBLE("s3_compatible"),
    GOOGLE_DRIVE("google-drive"),
}

/** Storage provider kind. */
enum class NewStorageLocationProvider(val value: String) {
    LOCAL_DIRECTORY("local_directory"),
    S3_COMPATIBLE("s3_compatible"),
    GOOGLE_DRIVE("google-drive"),
}

/** A portable declaration with this device's binding status. */
data class ProjectStorageLocation(
    /** The location's ID. */
    val locationId: String,
    /** The owning project. */
    val projectId: String,
    /** Display name. */
    val name: String,
    /** Storage provider kind. */
    val provider: ProjectStorageLocationProvider,
    /** Portable non-secret provider configuration. */
    val configuration: Map<String, String>,
    /** Whether this device has a private binding. */
    val bound: Boolean,
    /** Safe binding display text, or null; credentials are never returned. */
    val bindingPreview: String? = null,
    /** When the declaration was created. */
    val createdAt: Long,
    /** When the declaration was last updated. */
    val updatedAt: Long,
)

/** A project's storage locations, with this device's binding status, and this device's preferred one. */
data class StorageLocationList(
    /** The project's locations, most recently updated first; none for a project with none, or one this device does not have. */
    val data: List<ProjectStorageLocation>,
    /** This device's preferred location for the project; null when it has none. */
    val preferredLocationId: String? = null,
)

/** The location affected by an action. */
data class LocationDeleteResult(
    /** The affected location ID. */
    val locationId: String,
)

/** A portable location declaration to create. */
data class NewStorageLocation(
    /** Optional imported ID; Holder makes one when omitted or empty. */
    val locationId: String? = null,
    /** The owning project. */
    val projectId: String,
    /** Display name. */
    val name: String,
    /** Storage provider kind. */
    val provider: NewStorageLocationProvider,
    /** Portable non-secret provider configuration; empty when omitted. */
    val configuration: Map<String, String>? = null,
    /** Imported creation time; current time when omitted. */
    val createdAt: Long? = null,
    /** Imported update time; creation time when omitted. */
    val updatedAt: Long? = null,
)

/** Changes to a portable location declaration. */
data class StorageLocationUpdate(
    /** Replacement display name. */
    val name: String? = null,
    /** Replacement configuration map. */
    val configuration: Map<String, String>? = null,
    /** Update time; current time when omitted. */
    val updatedAt: Long? = null,
)

/** The locations operations of core's C API. Each fails with a [ContractException]. */
object Locations {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * List a project's storage declarations, with this device's binding status and preferred location.
     * Locations are most recently updated first. A project with none, or one this device does not have, lists none.
     * The daemon serves this as GET /locations.
     */
    external fun listStorageLocations(
        contextHandle: Long,
        projectId: String,
    ): StorageLocationList

    /** Get a storage declaration and this device's binding status. */
    external fun getStorageLocation(
        contextHandle: Long,
        locationId: String,
    ): ProjectStorageLocation

    /** Create a project's portable storage declaration. */
    external fun createStorageLocation(
        contextHandle: Long,
        request: NewStorageLocation,
    ): ProjectStorageLocation

    /** Change a storage declaration; configuration replaces the whole map. */
    external fun updateStorageLocation(
        contextHandle: Long,
        locationId: String,
        request: StorageLocationUpdate,
    ): ProjectStorageLocation

    /** Delete an unused declaration, its binding and its preferred setting on this device. */
    external fun deleteStorageLocation(
        contextHandle: Long,
        locationId: String,
    ): LocationDeleteResult
}
