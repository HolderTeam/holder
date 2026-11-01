// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

enum class ListProjectsOrder(val value: String) {
    UPDATED_AT_DESC("updated_at_desc"),
    UPDATED_AT_ASC("updated_at_asc"),
    CREATED_AT_DESC("created_at_desc"),
    CREATED_AT_ASC("created_at_asc"),
    NAME_DESC("name_desc"),
    NAME_ASC("name_asc"),
}

/** How the project's files are stored. */
enum class ProjectPrivacyMode(val value: String) {
    ENCRYPTED_GIT("encrypted_git"),
    PLAIN("plain"),
}

/** How the project's files are stored. */
enum class NewProjectPrivacyMode(val value: String) {
    ENCRYPTED_GIT("encrypted_git"),
    PLAIN("plain"),
}

/** A project in a listing. */
data class ProjectListItem(
    /** The project's ID. */
    val projectId: String,
    /** The project's name. */
    val name: String,
    /** The directory that holds the project's repository on this device. */
    val rootPath: String,
    /** The Git remote the project syncs with; null when it has none. */
    val gitRemoteUrl: String? = null,
    /** Who hosts the Git remote, such as github; null when unknown. */
    val gitProvider: String? = null,
    /** How the project's files are stored. */
    val privacyMode: ProjectPrivacyMode,
    /** The ID of an encrypted project's key; null for a plain project. */
    val projectKeyId: String? = null,
    /** When the project was made. */
    val createdAt: Long,
    /** When the project's settings last changed. */
    val updatedAt: Long,
    /** The project's live cards; present when asked for. */
    val cardCount: Int? = null,
    /** The project's live root cards; present when asked for. */
    val rootCardCount: Int? = null,
)

/** A project on this device. */
data class Project(
    /** The project's ID. */
    val projectId: String,
    /** The project's name. */
    val name: String,
    /** The directory that holds the project's repository on this device. */
    val rootPath: String,
    /** The Git remote the project syncs with; null when it has none. */
    val gitRemoteUrl: String? = null,
    /** Who hosts the Git remote, such as github; null when unknown. */
    val gitProvider: String? = null,
    /** How the project's files are stored. */
    val privacyMode: ProjectPrivacyMode,
    /** The ID of an encrypted project's key; null for a plain project. */
    val projectKeyId: String? = null,
    /** When the project was made. */
    val createdAt: Long,
    /** When the project's settings last changed. */
    val updatedAt: Long,
)

/** A project to make. */
data class NewProject(
    /** The project's ID, for imports; omit for a new one. */
    val projectId: String? = null,
    /** The project's name. */
    val name: String,
    /** The directory for the project's repository, an absolute path; omit for one under this device's projects directory, named for the project. */
    val rootPath: String? = null,
    /** The Git remote to sync with; null or omitted for none. One Git does not accept is an invalid argument. */
    val gitRemoteUrl: String? = null,
    /** Who hosts the Git remote; null or omitted when unknown. */
    val gitProvider: String? = null,
    /** How the project's files are stored. */
    val privacyMode: NewProjectPrivacyMode = NewProjectPrivacyMode.ENCRYPTED_GIT,
    /** When the project was made, for imports, after the epoch; omit for now. */
    val createdAt: Long? = null,
    /** When the project's settings last changed, for imports, after the epoch; omit for when it was made. */
    val updatedAt: Long? = null,
)

/** A change to a project's settings. Each field left out keeps its value. */
data class ProjectUpdate(
    /** The project's name. */
    val name: String? = null,
    /** The Git remote to sync with; null for none. */
    val gitRemoteUrl: Change<String> = Change.Keep,
    /** Who hosts the Git remote; null when unknown. */
    val gitProvider: Change<String> = Change.Keep,
)

/** The projects operations of core's C API. Each fails with a [ContractException]. */
object Projects {
    init {
        System.loadLibrary("holder_jni")
    }

    /**
     * List the projects on this device.
     * Filters combine. Projects that tie in the order keep the order they were stored in.
     */
    external fun listProjects(
        contextHandle: Long,
        name: String? = null,
        updatedAfter: Long? = null,
        updatedBefore: Long? = null,
        order: ListProjectsOrder? = null,
        limit: Int = 100,
        offset: Int = 0,
        includeCardCounts: Boolean = false,
    ): List<ProjectListItem>

    /** Fetch a project. */
    external fun getProject(
        contextHandle: Long,
        projectId: String,
    ): Project

    /**
     * Make a project.
     * Its repository is made, with its settings, and an encrypted project's key. An ID a project on this device has
     * is a Conflict. Making a project that was removed from this device makes it known here again.
     */
    external fun createProject(
        contextHandle: Long,
        project: NewProject,
    ): Project

    /**
     * Change a project's settings.
     * Each field left out keeps its value, and an update that changes nothing writes nothing. The settings are
     * committed to the project's repository; a Git remote is set there before it is stored, and one Git does not
     * accept is an invalid argument.
     */
    external fun updateProject(
        contextHandle: Long,
        projectId: String,
        update: ProjectUpdate,
    ): Project

    /**
     * Remove a project from this device.
     * The project's files and keys are kept, and its removal is remembered, so that looking for projects does not
     * bring it back; making it again does. Returns the project as it was.
     */
    external fun deleteProject(
        contextHandle: Long,
        projectId: String,
    ): Project
}
