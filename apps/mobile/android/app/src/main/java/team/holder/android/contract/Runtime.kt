// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

/** The version of the core library loaded. */
data class CoreVersion(
    /** The whole version, such as 1.4.2. */
    val version: String,
    /** Its major number. */
    val major: Int,
    /** Its minor number. */
    val minor: Int,
    /** Its patch number. */
    val patch: Int,
)

/** What rebuilding a data directory's database from its project files found, or would find. */
data class DatabaseRebuildReport(
    /** Whether the database was left as it was and only counted. */
    val dryRun: Boolean,
    /** The old database's state: healthy, missing, corrupt or stale. */
    val previousHealth: String,
    /** Projects read. */
    val projects: Int,
    /** Cards read. */
    val cards: Int,
    /** AI threads read. */
    val aiThreads: Int,
    /** AI messages read. */
    val aiMessages: Int,
    /** Resources read. */
    val resources: Int,
    /** Assets read. */
    val assets: Int,
    /** Asset placements read. */
    val placements: Int,
    /** Storage locations read. */
    val locations: Int,
    /** Cards left out because their file is missing; each is logged beside the database. */
    val quarantinedCards: Int,
    /** Where the old database was kept; null when there was none to keep, or on a dry run. */
    val backupPath: String? = null,
)

/** The runtime operations of core's C API. Each fails with a [ContractException]. */
object Runtime {
    init {
        System.loadLibrary("holder_jni")
    }

    /** The version of the core library loaded. */
    external fun version(): CoreVersion

    /**
     * Open a context on a data directory: the handle every other operation works through.
     * Opening makes the directory's database, or rebuilds it from the project files when it is
     * missing, damaged or from an older schema; that needs schema_sql. A context holds one database
     * connection and must not run two calls at once: open one per thread or serial worker, the first
     * before the others since it may rebuild. The platform keyring must be registered first when
     * encrypted projects may need rebuilding. Release the context with context_destroy.
     * @return The new context's handle, which the caller releases with the closing operation.
     */
    external fun contextOpen(
        dataDir: String,
        schemaSql: String? = null,
    ): Long

    /** Release a context. No call on it may still be running, and it must not be used again. */
    external fun contextDestroy(
        contextHandle: Long,
    )

    /**
     * Rebuild a data directory's database from its project files, keeping the old one aside.
     * No context on the directory may be open while this runs: close them all first and open them
     * again after. The platform keyring must be registered first when encrypted projects are present.
     * A dry run only counts what a rebuild would read.
     */
    external fun databaseRebuild(
        dataDir: String,
        schemaSql: String,
        dryRun: Boolean = false,
    ): DatabaseRebuildReport
}
