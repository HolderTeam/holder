package team.holder.android

import android.content.Context
import team.holder.android.contract.Card
import team.holder.android.contract.CardListItem
import team.holder.android.contract.CardMetadata
import team.holder.android.contract.CardMove
import team.holder.android.contract.CardMoveIntent
import team.holder.android.contract.CardReferenceResolveDataStatus
import team.holder.android.contract.CardReferenceResolveRequest
import team.holder.android.contract.CardReferenceResolveRequestScope
import team.holder.android.contract.CardUpdate
import team.holder.android.contract.Cards
import team.holder.android.contract.Change
import team.holder.android.contract.ConflictException
import team.holder.android.contract.DeleteProjectTrashItemType
import team.holder.android.contract.EmptyTrashType
import team.holder.android.contract.ListCardsOrder
import team.holder.android.contract.ListCardsParent
import team.holder.android.contract.NewCard
import team.holder.android.contract.Page
import team.holder.android.contract.RecoverySnapshotPage
import team.holder.android.contract.RecoverySnapshotRestore
import team.holder.android.contract.RecoverySnapshotRestorePrivacyMode
import team.holder.android.contract.Search
import team.holder.android.contract.Trash
import team.holder.android.contract.CardHistoryRestoreResult
import team.holder.android.contract.CardHistoryVersion
import team.holder.android.contract.CompareCardHistoryMode
import team.holder.android.contract.Git
import team.holder.android.contract.LinkSourceType
import team.holder.android.contract.LinkTargetType
import team.holder.android.contract.Links
import team.holder.android.contract.NewLink
import team.holder.android.contract.NewLinkTargetType
import team.holder.android.contract.NotFoundException
import team.holder.android.contract.RemoveCardLinkTargetType
import team.holder.android.contract.History
import team.holder.android.contract.ListProjectsOrder
import team.holder.android.contract.Locations
import team.holder.android.contract.Maintenance
import team.holder.android.contract.Milestone
import team.holder.android.contract.MilestoneDate
import team.holder.android.contract.MilestoneUpdate
import team.holder.android.contract.Milestones
import team.holder.android.contract.NewMilestone
import team.holder.android.contract.NewProject
import team.holder.android.contract.NewStorageLocation
import team.holder.android.contract.NewStorageLocationProvider
import team.holder.android.contract.NewProjectPrivacyMode
import team.holder.android.contract.Project
import team.holder.android.contract.ProjectUpdate
import team.holder.android.contract.Projects
import team.holder.android.contract.PullPhase
import team.holder.android.contract.PushOptions
import team.holder.android.contract.PushPhase
import team.holder.android.contract.Recovery
import team.holder.android.contract.RecoveryTokenExportRequest
import team.holder.android.contract.RecoveryTokenImportRequest
import team.holder.android.contract.RemoteTest
import team.holder.android.contract.RemoteTestResultStatus
import team.holder.android.contract.Resource
import team.holder.android.contract.Resources
import team.holder.android.contract.AssetImportRequest
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.time.Instant
import java.time.LocalDate
import java.time.OffsetDateTime
import java.time.ZoneId
import java.time.ZoneOffset
import java.time.format.DateTimeFormatter

data class HolderProject(
    val projectId: String,
    val name: String,
    val gitRemoteUrl: String?,
    val privacyMode: String,
)

data class HolderCard(
    val cardId: String,
    val projectId: String,
    val title: String,
    val parentCardId: String?,
    val createdAt: Long,
    val updatedAt: Long,
    /** Manual sibling order set by Flowboard's drag-to-reorder on desktop; defaults to
     * creation order (each new card gets a higher sort_key than its existing siblings)
     * until someone explicitly rearranges them. Scoped to cards sharing the same
     * parentCardId -- not comparable across siblings of different parents. */
    val sortKey: Double,
    val deletedAt: Long? = null,
    /** Number of non-deleted direct children, set only when the query that produced this
     * card was asked for it (see [HolderNative.queryCards]'s includeChildCounts); every other
     * card-returning call leaves this null rather than paying for a count nobody asked for. */
    val childCount: Int? = null,
)

data class HolderTrashedCard(val cardId: String, val title: String, val deletedAt: Long)

data class HolderOutgoingLink(
    val toCardId: String,
    val toType: String,
    val kind: String,
    val label: String?,
    val toTitle: String?,
)

data class HolderBacklink(
    val fromCardId: String,
    val kind: String,
    val label: String?,
    val fromTitle: String?,
)

data class HolderCardRef(
    val cardId: String,
    val title: String,
)

/** Result of [HolderNative.resolveCardReference] -- mirrors holder-core's
 * CardReferenceResolver::resolve (full-id, then id-prefix, then case-sensitive exact-title). */
sealed interface CardReferenceResolution {
    data class Resolved(val matchKind: String, val card: HolderCardReferenceCandidate) : CardReferenceResolution
    data class Ambiguous(val matchKind: String, val candidates: List<HolderCardReferenceCandidate>) : CardReferenceResolution
    object NotFound : CardReferenceResolution
}

data class HolderCardReferenceCandidate(val cardId: String, val title: String, val deletedAt: Long?)

/** Matches holder_card_move_json's "intent" values (lowercased, underscore-separated, on the
 * wire) -- see holder-core's CardPlacementResolver for the full algorithm each one runs. */
enum class CardPlacementIntent { INTO, BEFORE, AFTER, TO_START, TO_END, LEFT, RIGHT, UP_LEVEL }

/** Result of [HolderNative.moveCard] -- the card's new placement, same shape holder-daemon's
 * /move route already returns. [revision] is the card's updated_at after the move. */
data class HolderCardMoveResult(
    val cardId: String,
    val parentCardId: String?,
    val sortKey: Double,
)

data class HolderCardLinks(
    val outgoing: List<HolderOutgoingLink>,
    val backlinks: List<HolderBacklink>,
    /** Hierarchy (parent_card_id), shown automatically -- not editable through
     * addCardLink/removeCardLink. Null at the root of a project. */
    val parent: HolderCardRef?,
    val children: List<HolderCardRef>,
)

/** A card's tags, and those of them [HolderNative.removeCardTag] would remove. */
data class HolderCardTags(
    val tags: List<String>,
    val removable: List<String>,
)

data class HolderTagCount(
    val tag: String,
    val count: Int,
)

data class HolderLinkKind(
    val id: String,
    val forward: String,
    val reverse: String,
)

data class HolderSearchResult(
    val cardId: String,
    val title: String,
    val snippet: String,
)

data class HolderMilestone(
    val milestoneId: String,
    val cardId: String,
    val startAt: Long,
    val endAt: Long?,
    val allDay: Boolean,
    val kind: String?,
    val description: String?,
    val createdAt: Long,
    val updatedAt: Long,
    /** Only populated by listMilestonesInRange (the Calendar query) -- null from
     * listCardMilestones, where the caller already knows which card this is. */
    val cardTitle: String? = null,
)

/** A tri-state value for [HolderNative.updateCardMilestone]'s clearable fields (endAt/kind/
 * description): [Unset] leaves the field unchanged, [Clear] explicitly nulls it, [Value] sets
 * it. Mirrors CardStore::MilestoneUpdate's has_*-flag semantics on the native side. */
sealed class OptionalLong {
    object Unset : OptionalLong()
    object Clear : OptionalLong()
    data class Value(val value: Long) : OptionalLong()
}

/** String counterpart of [OptionalLong], for endAt/kind/description-shaped string fields. */
sealed class OptionalString {
    object Unset : OptionalString()
    object Clear : OptionalString()
    data class Value(val value: String) : OptionalString()
}

/** One editing-session-grouped history entry (one saved card revision, e.g. one autosave). */
data class HolderCardHistorySave(
    val oid: String,
    val parentOids: List<String>,
    val authoredAt: Long,
    val committedAt: Long,
    val message: String,
)

/** One row of card_id's history timeline: either a single structural event (creation, move,
 * trash, restore, merge...) or a burst of ordinary same-author saves grouped into one editing
 * session -- see HISTORY_TOOL_PLAN.md's grouping rules. [saves] is chronological, first grouped
 * save to last; a single-commit entry still carries exactly one. */
data class HolderCardHistoryEntry(
    val firstOid: String,
    val lastOid: String,
    val parentOids: List<String>,
    /** Direct parents also present as another entry in this same page -- a missing link may
     * cross filtered history or a page boundary and must never be drawn as a direct connection. */
    val visibleParentOids: List<String>,
    val authorName: String,
    val authorEmail: String,
    val startedAt: Long,
    val endedAt: Long,
    val kind: String,
    val summary: String,
    val commitCount: Int,
    val isMerge: Boolean,
    val saves: List<HolderCardHistorySave>,
)

/** One page of [HolderNative.listCardHistory], most-recent entry first. */
data class HolderCardHistoryPage(
    /** The commit OID HEAD pointed at when this page was loaded -- callers must keep using this
     * exact OID for later "Since this version" comparisons rather than a live/moving HEAD, so an
     * autosave landing after this call cannot silently change what a caller believes it sees. */
    val headOid: String?,
    val entries: List<HolderCardHistoryEntry>,
    val nextCursor: String?,
    /** True if the underlying scan hit its bound before finishing this page -- nextCursor still
     * lets the caller continue rather than presenting an incomplete timeline as complete. */
    val scanLimited: Boolean,
)

/** A card's read-only state at one specific commit, as returned by [HolderNative.compareCardHistory]. */
data class HolderCardHistoryVersion(
    /** False for the state before a card's creation event -- there is no earlier version. */
    val exists: Boolean,
    val oid: String,
    val title: String,
    val body: String,
)

data class HolderCardHistoryDiffLine(
    /** One of " " (unchanged), "+" (added), or "-" (removed). */
    val origin: String,
    val text: String,
    val oldLine: Long?,
    val newLine: Long?,
)

data class HolderCardHistoryComparison(
    val from: HolderCardHistoryVersion,
    val to: HolderCardHistoryVersion,
    val summary: String,
    val lines: List<HolderCardHistoryDiffLine>,
    /** True if the diff hit its bounded line count/length and was shortened. */
    val truncated: Boolean,
)

/** A storage location a project's Resources/Assets can be placed in -- e.g. a Google Drive
 * folder. `configuration` is provider-specific and portable (git-synced, per-project); it
 * holds public locators like a Drive folder id, never secrets -- see AndroidStorageProvider
 * for where the actual OAuth/API credentials live instead. */
data class HolderLocation(
    val locationId: String,
    val projectId: String,
    val name: String,
    val provider: String,
    val configuration: Map<String, String>,
    val createdAt: Long,
    val updatedAt: Long,
)

data class HolderPlacement(
    val placementId: String,
    val locationId: String,
    val encoding: String,
    val storedByteSize: Long,
    val createdAt: Long,
)

data class HolderAsset(
    val assetId: String,
    val resourceId: String,
    val originalFilename: String,
    val mediaType: String,
    val byteSize: Long,
    val plaintextSha256: String,
    val createdAt: Long,
    val updatedAt: Long,
    val placements: List<HolderPlacement>,
)

data class HolderResource(
    val resourceId: String,
    val projectId: String,
    val type: String,
    val label: String,
    val createdAt: Long,
    val updatedAt: Long,
    val assets: List<HolderAsset>,
)

data class HolderAssetImportResult(
    val resourceId: String,
    val assetId: String,
    val duplicateReused: Boolean,
    val linkCreated: Boolean,
)

data class GitTestRemoteResult(
    val status: String,
    val remoteHasHead: Boolean,
    val errorMessage: String?,
)

data class GitPushResult(
    val status: String,
    val aheadCount: Int,
    val behindCount: Int,
    val errorMessage: String?,
)

data class GitPullResult(
    val status: String,
    val errorMessage: String?,
    val conflictsResolved: Int,
)

data class GitSyncStatus(
    val lastPushAt: Long?,
    val lastPullAt: Long?,
    val uncommittedChangesCount: Int,
    val unpushedCommitsCount: Int,
    val lastPushStatus: String?,
    val lastPullStatus: String?,
    val lastSyncError: String?,
)

data class GitSyncIfDueResult(
    val pullAttempted: Boolean,
    val pullStatus: String?,
    val pullError: String?,
    /** Non-zero only when pullStatus is "succeeded" after a divergent remote -- Holder's
     * remote-wins conflict merge, not a user-facing merge tool. */
    val pullConflictsResolved: Int,
    val pushAttempted: Boolean,
    val pushStatus: String?,
    val pushError: String?,
)

data class GitSyncNowResult(
    val pullAttempted: Boolean,
    val pullStatus: String?,
    val pullError: String?,
    /** Non-zero only when pullStatus is "succeeded" after a divergent remote -- Holder's
     * remote-wins conflict merge, not a user-facing merge tool. */
    val pullConflictsResolved: Int,
    val pushAttempted: Boolean,
    val pushStatus: String?,
    val pushError: String?,
    val pushAheadCount: Int,
    val pushBehindCount: Int,
    val pushLocalHeadCommit: String?,
)

data class EncryptionCheckResult(
    val privacyMode: String,
    val ok: Boolean,
    val checkedFiles: Int,
    val unsafeFiles: Int,
    val unsafePaths: List<String>,
    val message: String,
)

data class RecoveryTokenExportResult(
    val projectId: String,
    val keyId: String,
    val recoveryToken: String,
)

data class RecoveryTokenImportGlobalResult(
    val projectId: String,
    val projectCreated: Boolean,
    val remoteHintPresent: Boolean,
    val remoteConfigured: Boolean,
    val remoteError: String?,
    val pullStatus: String,
    val pullError: String?,
)

/**
 * Kotlin -> JNI -> C ABI -> libholder boundary. Opens a single native
 * holder_context on first use (see initialize) and keeps it open for the
 * app's lifetime instead of reopening the SQLite store on every call.
 */
object HolderNative {
    /** The project `ensure_default_project` seeds on first run -- see RestoreOffer/
     * SnapshotProtection's doc comments. Not private: the share-in project picker (see
     * MainActivity's ProjectPickerDialog usage) sorts this project first since it's the one
     * most people land in most often. */
    const val DEFAULT_PROJECT_NAME = "Home"
    private const val WELCOME_CARD_TITLE_FALLBACK = "Welcome"

    private val loadError: Throwable? = runCatching {
        System.loadLibrary("holder_jni")
    }.exceptionOrNull()

    /** Every native call runs on one of three lanes, each a thread with its own holder-core
     * context (see [NativeLane]), split the way holder-daemon splits its request lanes:
     * - [saveLane]: every write, in the order the user made them, so a slow write (each one is
     *   a Git commit) never holds up what's on screen;
     * - [readLane]: what the screen needs now; it sees only committed data, never a write
     *   that's still in progress on the save lane;
     * - [backgroundLane]: Git network work, history, backup snapshots, encryption and assets
     *   (whose storage provider may be on the network), so none of it blocks the other two.
     * Writes on different lanes are ordered by SQLite, and Git work on one project by core's
     * per-repository lock. */
    // Projects are listed in pages of the contract's largest page.
    private const val PROJECT_PAGE = 1000

    private val saveLane = NativeLane("holder-save")
    private val readLane = NativeLane("holder-read")
    private val backgroundLane = NativeLane("holder-background")

    /** Runs [block] with the open context's handle on the save lane, for calls to the generated
     * contract client (team.holder.android.contract), which take the handle first. */
    internal fun <T> onContract(block: (Long) -> T): T = saveLane.call(block)

    @Volatile
    private var isOpen = false

    /** Set once by [initialize], reused by [selectGitSignerForProject] -- the native signer
     * can be re-registered any number of times after the context is first opened (each project
     * gets its own alias), and every one of those re-registrations needs the same homedir
     * [team.holder.android.git.GitIdentity.registerWithNative] was originally given. */
    @Volatile
    private var dataDir: File? = null

    private external fun nativeVersion(): String
    private external fun nativeContextOpen(dataDir: String, schemaSql: String): Long
    private external fun nativeContextClose(contextHandle: Long)
    private external fun nativeDatabaseRebuild(dataDir: String, schemaSql: String, dryRun: Boolean): String
    private external fun nativeEnsureDefaultProject(
        contextHandle: Long,
        name: String,
        welcomeTitle: String,
        welcomeContent: String?,
    ): String

    fun version(): String {
        loadError?.let {
            return "native load failed: ${it.message ?: it::class.java.simpleName}"
        }

        return runCatching {
            nativeVersion()
        }.getOrElse {
            "native call failed: ${it.message ?: it::class.java.simpleName}"
        }
    }

    /**
     * Opens the native store (once; safe to call again) and, on first launch,
     * bootstraps a default Home project with a welcome card.
     */
    @Synchronized
    fun initialize(context: Context, dataDir: File, schemaSql: String, welcomeContent: String) {
        loadError?.let { throw it }

        if (!isOpen) {
            // Recovery of encrypted projects needs the platform keyring before
            // holder_context_open can reconstruct the SQLite projection. Registration
            // remains best-effort here: a plain profile does not need it, while core
            // will fail closed if reconstruction actually needs an unavailable key.
            runCatching { team.holder.android.keyring.AndroidKeyringStore.registerWithNative(context) }
            // One context per lane, opened one after another: the first open may rebuild the
            // database, and the others then find it healthy.
            try {
                for (lane in listOf(saveLane, readLane, backgroundLane)) {
                    lane.attach(nativeContextOpen(dataDir.absolutePath, schemaSql))
                }
            } catch (failure: Throwable) {
                closeLanes()
                throw failure
            }
            isOpen = true
            this.dataDir = dataDir
            // Best-effort, and deliberately not project-scoped here: this just points libgit2
            // at dataDir for ~/.ssh/known_hosts and installs *some* signer so the context has
            // one at all; selectGitSignerForProject re-registers with the right per-project
            // alias immediately before any actual git network operation, which is what
            // matters. Only the background lane does Git network work, so only it needs one.
            backgroundLane.call {
                runCatching { team.holder.android.git.GitIdentity.registerWithNative(it, dataDir) }
            }
        }
        saveLane.call {
            nativeEnsureDefaultProject(
                it,
                DEFAULT_PROJECT_NAME,
                deriveWelcomeTitle(welcomeContent, WELCOME_CARD_TITLE_FALLBACK),
                welcomeContent,
            )
        }
    }

    /**
     * Reconstructs Holder's disposable SQLite projection from the Git-backed
     * project files. The native context is closed for the swap and reopened
     * before this method returns, including after a dry run.
     */
    @Synchronized
    fun rebuildDatabase(
        context: Context,
        dataDir: File,
        schemaSql: String,
        welcomeContent: String,
        dryRun: Boolean = false,
    ): JSONObject {
        loadError?.let { throw it }
        var primaryFailure: Throwable? = null
        return try {
            close()
            // As in initialize(), let holder-core decide whether a missing keyring
            // is material for the projects being reconstructed.
            runCatching { team.holder.android.keyring.AndroidKeyringStore.registerWithNative(context) }
            JSONObject(nativeDatabaseRebuild(dataDir.absolutePath, schemaSql, dryRun))
        } catch (failure: Throwable) {
            primaryFailure = failure
            throw failure
        } finally {
            try {
                initialize(context, dataDir, schemaSql, welcomeContent)
            } catch (reopenFailure: Throwable) {
                if (primaryFailure != null) {
                    primaryFailure.addSuppressed(reopenFailure)
                } else {
                    throw reopenFailure
                }
            }
        }
    }

    /** Closes the native store. Safe to call even if it was never opened. Waits for calls
     * already queued on each lane to finish first, so a context is never closed under a call
     * that's still using it; calls queued after this fail as not initialized. */
    @Synchronized
    fun close() {
        isOpen = false
        closeLanes()
    }

    private fun closeLanes() {
        for (lane in listOf(saveLane, readLane, backgroundLane)) {
            lane.detach { nativeContextClose(it) }
        }
    }

    fun listProjects(): List<HolderProject> {
        val result = ArrayList<HolderProject>()
        readLane.call { context ->
            var count: Int
            do {
                val page = Projects.listProjects(context, order = ListProjectsOrder.UPDATED_AT_DESC,
                                                 limit = PROJECT_PAGE, offset = result.size)
                count = page.size
                page.forEach { project ->
                    result.add(HolderProject(project.projectId, project.name,
                        project.gitRemoteUrl, project.privacyMode.value))
                }
            } while (count == PROJECT_PAGE)
        }
        return result
    }

    internal fun listCardsPage(
        projectId: String,
        parent: ListCardsParent? = null,
        tag: String? = null,
        order: ListCardsOrder = ListCardsOrder.CARD_ID_ASC,
        limit: Int = 200,
        cursor: String? = null,
        includeContent: Boolean = false,
        includeChildCount: Boolean = false,
    ): Page<CardListItem> = readLane.call { context ->
        Cards.listCards(context, projectId, parent, tag, order, limit, cursor, includeContent, includeChildCount)
    }

    fun listCards(projectId: String): List<HolderCard> {
        val result = ArrayList<HolderCard>()
        readLane.call { context ->
            var cursor: String? = null
            do {
                val page = Cards.listCards(context, projectId, order = ListCardsOrder.SORT_KEY_ASC, limit = 5000, cursor = cursor)
                result.ensureCapacity(result.size + page.items.size)
                page.items.forEach { card ->
                    result.add(HolderCard(
                        cardId = card.cardId,
                        projectId = card.projectId,
                        title = card.title,
                        parentCardId = card.parentCardId,
                        createdAt = card.createdAt,
                        updatedAt = card.updatedAt,
                        sortKey = card.sortKey,
                        deletedAt = card.deletedAt,
                        childCount = card.childCount,
                    ))
                }
                cursor = page.nextCursor
            } while (cursor != null)
        }
        return result
    }

    /** Fetches one live card's metadata in its project. */
    fun getCard(projectId: String, cardId: String): HolderCard {
        val card = readLane.call { Cards.getCardMetadata(it, projectId, cardId) }
        return fromContractMetadata(card)
    }

    fun getCardContent(projectId: String, cardId: String): String =
        readLane.call { Cards.getCard(it, projectId, cardId) }.content

    /** Resolves a [[wikilink]]-style reference (a card id, an unambiguous id prefix, or a
     * case-sensitive exact title) the same way holder-daemon/holderctl do. scope defaults
     * to 0 (live cards only), matching holder::model::CardScope::Live. */
    fun resolveCardReference(projectId: String, reference: String, scope: Int = 0): CardReferenceResolution {
        val request = CardReferenceResolveRequest(projectId, reference, CardReferenceResolveRequestScope.entries[scope])
        val result = readLane.call { Cards.resolveCardReference(it, request) }
        return when (result.status) {
            CardReferenceResolveDataStatus.RESOLVED -> CardReferenceResolution.Resolved(
                matchKind = requireNotNull(result.matchKind),
                card = requireNotNull(result.card).let { HolderCardReferenceCandidate(it.cardId, it.title, it.deletedAt) },
            )
            CardReferenceResolveDataStatus.AMBIGUOUS -> CardReferenceResolution.Ambiguous(
                matchKind = requireNotNull(result.matchKind),
                candidates = requireNotNull(result.candidates).map {
                    HolderCardReferenceCandidate(it.cardId, it.title, it.deletedAt)
                },
            )
            CardReferenceResolveDataStatus.NOT_FOUND -> CardReferenceResolution.NotFound
        }
    }

    /** One page of projectId's recovery snapshot, most recently updated first, for
     * [team.holder.android.git.backup.SnapshotWriter]. Pass cursor = null for the first page,
     * then the previous page's [RecoverySnapshotPage.nextCursor]. */
    fun recoverySnapshotPage(projectId: String, cursor: String?, limit: Int): RecoverySnapshotPage =
        backgroundLane.call { Recovery.readRecoverySnapshot(it, projectId, cursor, limit) }

    /** Restores [snapshot], recovery snapshot lines one per line, into a new project with one commit.
     * Core gives every card, resource and storage location a new ID and keeps the card tree and
     * links between them; a snapshot core cannot read fails and leaves no project behind. */
    fun restoreRecoverySnapshot(
        projectName: String,
        privacyMode: String,
        snapshot: String,
        commitMessage: String,
    ): HolderProject {
        val mode = RecoverySnapshotRestorePrivacyMode.entries.singleOrNull { it.value == privacyMode }
            ?: RecoverySnapshotRestorePrivacyMode.PLAIN
        val request = RecoverySnapshotRestore(projectName, mode, snapshot, commitMessage)
        return fromContractProject(saveLane.call { Recovery.restoreRecoverySnapshot(it, request) })
    }

    /** Android's absent privacy mode means plain; the contract request defaults to encrypted_git. */
    fun createProject(name: String, privacyMode: String? = null): HolderProject {
        val mode = privacyMode?.let { value -> NewProjectPrivacyMode.entries.single { it.value == value } }
            ?: NewProjectPrivacyMode.PLAIN
        return fromContractProject(createProjectContract(NewProject(name = name, privacyMode = mode)))
    }

    internal fun createProjectContract(project: NewProject): Project =
        saveLane.call { Projects.createProject(it, project) }

    fun renameProject(projectId: String, name: String): HolderProject =
        fromContractProject(saveLane.call { Projects.updateProject(it, projectId, ProjectUpdate(name = name)) })

    fun deleteProject(projectId: String) {
        saveLane.call { Projects.deleteProject(it, projectId) }
    }

    private fun fromContractProject(project: Project) = HolderProject(
        project.projectId, project.name, project.gitRemoteUrl, project.privacyMode.value,
    )

    fun createCard(projectId: String, title: String, content: String, parentCardId: String? = null): HolderCard =
        fromContractCard(saveLane.call {
            Cards.createCard(it, projectId, NewCard(title = title, content = content, parentCardId = parentCardId))
        })

    /** Saves the editor's title and content. An empty title keeps the card's current one, as the
     * editor sends an empty title when the user has not given one. */
    fun updateCard(projectId: String, cardId: String, title: String, content: String): HolderCard =
        fromContractCard(saveLane.call {
            Cards.updateCard(it, projectId, cardId, CardUpdate(title = title.ifEmpty { null }, content = content))
        })

    /** Resolves and applies one of eight relative-placement intents to cardId within
     * projectId, via holder-core's CardPlacementResolver (see that class for the full
     * algorithm/error vocabulary this shares with holder-daemon's /move route). [targetCardId]
     * is required for [CardPlacementIntent.INTO]/[CardPlacementIntent.BEFORE]/
     * [CardPlacementIntent.AFTER] and ignored otherwise. [parentCardId] is an optional override
     * for [CardPlacementIntent.TO_START]/[CardPlacementIntent.TO_END]/[CardPlacementIntent.LEFT]/
     * [CardPlacementIntent.RIGHT]; when omitted, cardId's current parent is used. Driven by
     * Board mode's per-card move menu and its drag-to-reorder handle -- see CardListScreen. */
    fun moveCard(
        projectId: String,
        cardId: String,
        intent: CardPlacementIntent,
        targetCardId: String? = null,
        parentCardId: String? = null,
    ): HolderCardMoveResult {
        val move = CardMove(
            intent = CardMoveIntent.valueOf(intent.name),
            targetCardId = targetCardId,
            parentCardId = parentCardId?.let { Change.Set(it) } ?: Change.Keep,
        )
        val card = saveLane.call { Cards.moveCard(it, projectId, cardId, move) }
        return HolderCardMoveResult(cardId = card.cardId, parentCardId = card.parentCardId, sortKey = card.sortKey)
    }

    /** Soft-deletes (trashes) a card; it stops appearing in listCards until restored or purged. */
    fun deleteCard(projectId: String, cardId: String) {
        saveLane.call { Cards.trashCard(it, projectId, cardId) }
    }

    fun listTrashedCards(projectId: String): List<HolderTrashedCard> =
        readLane.call { Trash.listTrash(it, projectId, type = "card") }.map { item ->
            HolderTrashedCard(
                cardId = requireNotNull(item.cardId),
                title = requireNotNull(item.title),
                deletedAt = item.deletedAt,
            )
        }

    /** Restores a trashed card so it reappears in listCards. Fails if cardId isn't trashed. */
    fun restoreCard(projectId: String, cardId: String): HolderCard =
        fromContractCard(saveLane.call { Cards.restoreCard(it, projectId, cardId) })

    /** Permanently deletes a trashed card. Irreversible. Fails if cardId isn't trashed. */
    fun purgeCard(projectId: String, cardId: String) {
        saveLane.call { Trash.deleteProjectTrashItem(it, projectId, DeleteProjectTrashItemType.CARD, cardId) }
    }

    fun emptyCardTrash(projectId: String) {
        saveLane.call { Trash.emptyTrash(it, projectId, EmptyTrashType.CARD) }
    }

    /** cardId's editing-session-grouped history within projectId's repository, most-recent
     * entry first, cursor-paginated (cursor null for the first page; for a later page, pass
     * back the previous response's nextCursor). Follows both cardId's live and Trash paths
     * automatically. Never mutates the repository, index, working tree, or database. */
    fun listCardHistory(
        projectId: String,
        cardId: String,
        cursor: String? = null,
        limit: Int = 50,
    ): HolderCardHistoryPage {
        val page = backgroundLane.call { History.listCardHistory(it, projectId, cardId, limit, cursor) }
        return HolderCardHistoryPage(
            headOid = page.headOid,
            entries = page.entries.map { entry ->
                HolderCardHistoryEntry(
                    firstOid = entry.firstOid,
                    lastOid = entry.lastOid,
                    parentOids = entry.parentOids,
                    visibleParentOids = entry.visibleParentOids,
                    authorName = entry.author.name,
                    authorEmail = entry.author.email,
                    startedAt = entry.startedAt,
                    endedAt = entry.endedAt,
                    kind = entry.kind,
                    summary = entry.summary,
                    commitCount = entry.commitCount,
                    isMerge = entry.isMerge,
                    saves = entry.saves.map { save -> HolderCardHistorySave(
                        oid = save.oid,
                        parentOids = save.parentOids,
                        authoredAt = save.authoredAt,
                        committedAt = save.committedAt,
                        message = save.message,
                    ) },
                )
            },
            nextCursor = page.nextCursor,
            scanLimited = page.scanLimited,
        )
    }

    /** Compares cardId's state at fromOid (null for a card's creation event, which has no
     * earlier version) against its state at toOid, both read directly from their commit trees.
     * toOid must be a real, previously-observed commit OID -- e.g. [HolderCardHistoryPage.headOid]
     * or an entry's lastOid -- never a live/moving "current" concept of the caller's own, so an
     * autosave landing between calls cannot silently change what is being compared. */
    fun compareCardHistory(
        projectId: String,
        cardId: String,
        fromOid: String?,
        toOid: String,
    ): HolderCardHistoryComparison {
        // Android compares from a given version, or shows the change a creation made.
        val mode = if (fromOid != null) CompareCardHistoryMode.SINCE else CompareCardHistoryMode.CHANGE
        val comparison = backgroundLane.call {
            History.compareCardHistory(it, projectId, cardId, toOid, fromOid, mode)
        }
        fun version(value: CardHistoryVersion) = HolderCardHistoryVersion(
            exists = value.exists, oid = value.oid, title = value.title, body = value.body,
        )
        return HolderCardHistoryComparison(
            // Android has always represented the absent state before creation with an empty OID.
            from = version(comparison.before).let { before ->
                if (fromOid == null && !before.exists) before.copy(oid = "") else before
            },
            to = version(comparison.after),
            summary = comparison.summary,
            lines = comparison.lines.map { line -> HolderCardHistoryDiffLine(
                origin = line.origin, text = line.text, oldLine = line.oldLine, newLine = line.newLine,
            ) },
            truncated = comparison.truncated,
        )
    }

    /** Restores cardId to the whole card snapshot recorded at historicalOid -- title, body,
     * links, milestones, hierarchy/location, tags, Resource references, and live/Trash state
     * together -- through the ordinary write path (a new "Restore card ..." commit). Never
     * checks out or rewrites Git history, so the pre-restore state remains its own, still-
     * reachable history entry. Returns what was restored: the card's ID, title and whether it is in the Trash. */
    fun restoreCardHistory(projectId: String, cardId: String, historicalOid: String): CardHistoryRestoreResult =
        saveLane.call { History.restoreCardHistorySnapshot(it, projectId, cardId, historicalOid) }

    /** cardId's explicit connections -- not inline [[wikilinks]] -- with its parent and live children,
     * which hierarchy shows automatically. Titles come from the linked cards and resources, as on
     * Desktop; a link whose card or resource cannot be read has no title. */
    fun listCardLinks(projectId: String, cardId: String): HolderCardLinks = readLane.call { context ->
        val titles = HashMap<String, String?>()
        fun cardTitle(id: String): String? = titles.getOrPut(id) {
            try {
                Cards.getCardMetadata(context, projectId, id).title
            } catch (_: NotFoundException) {
                null
            }
        }
        val resourceLabels by lazy { Resources.listResources(context, projectId).associate { it.resourceId to it.label } }
        val outgoing = Links.listCardLinks(context, projectId, cardId).map { link ->
            HolderOutgoingLink(
                toCardId = link.targetId,
                toType = link.targetType.value,
                kind = link.kind,
                label = link.label,
                toTitle = when (link.targetType) {
                    LinkTargetType.CARD -> cardTitle(link.targetId)
                    LinkTargetType.RESOURCE -> resourceLabels[link.targetId]
                    else -> null
                },
            )
        }
        val backlinks = Links.listCardBacklinks(context, projectId, cardId)
            .filter { it.sourceType == LinkSourceType.CARD }
            .map { link -> HolderBacklink(link.sourceId, link.kind, link.label, cardTitle(link.sourceId)) }
        val parent = Cards.getCardMetadata(context, projectId, cardId).parentCardId?.let { parentId ->
            cardTitle(parentId)?.let { title -> HolderCardRef(parentId, title) }
        }
        val children = Cards.listCards(context, projectId, parent = ListCardsParent.CardId(cardId),
                                       order = ListCardsOrder.SORT_KEY_ASC, limit = 5000)
            .items.map { child -> HolderCardRef(child.cardId, child.title) }
        HolderCardLinks(outgoing, backlinks, parent, children)
    }

    /** Adds (or updates, if fromCardId/toCardId/kind already matches) an explicit connection between
     * two cards in projectId. */
    fun addCardLink(projectId: String, fromCardId: String, toCardId: String, kind: String, label: String? = null) {
        saveLane.call {
            Links.addCardLink(it, projectId, fromCardId,
                NewLink(targetType = NewLinkTargetType.CARD, targetId = toCardId, kind = kind, label = label))
        }
    }

    fun removeCardLink(projectId: String, fromCardId: String, toCardId: String, kind: String) {
        saveLane.call { Links.removeCardLink(it, projectId, fromCardId, RemoveCardLinkTargetType.CARD, toCardId, kind) }
    }

    /** Attach an existing resource to a live card in the same project. */
    fun attachResource(projectId: String, cardId: String, resourceId: String, label: String?) {
        saveLane.call {
            Links.addCardLink(it, projectId, cardId,
                NewLink(targetType = NewLinkTargetType.RESOURCE, targetId = resourceId, kind = ATTACHMENT, label = label))
        }
    }

    /** Remove only the attachment link; the resource and stored file remain available. */
    fun detachResource(projectId: String, cardId: String, resourceId: String) {
        saveLane.call {
            Links.removeCardLink(it, projectId, cardId, RemoveCardLinkTargetType.RESOURCE, resourceId, ATTACHMENT)
        }
    }

    private const val ATTACHMENT = "attachment"

    @Volatile
    private var linkKindCatalog: List<HolderLinkKind>? = null

    /** Holder's built-in vocabulary of link kinds with curated English labels, from the contract's
     * list_link_kinds. Fetched once and cached -- the same for every project and device. */
    fun listLinkKinds(): List<HolderLinkKind> {
        linkKindCatalog?.let { return it }
        val parsed = readLane.call { Links.listLinkKinds(it) }
            .map { HolderLinkKind(id = it.kind, forward = it.forwardLabel, reverse = it.reverseLabel) }
        linkKindCatalog = parsed
        return parsed
    }

    /** Resolves kind to a display label: the catalog's forward/reverse label if it's one of
     * Holder's built-in link kinds, otherwise a humanized (snake_case -> "Snake case") fallback
     * shown the same regardless of direction, since an arbitrary custom kind has no known
     * reverse. */
    fun linkKindLabel(kind: String, forward: Boolean): String {
        val entry = listLinkKinds().firstOrNull { it.id == kind }
        if (entry != null) return if (forward) entry.forward else entry.reverse
        return kind.replace('_', ' ').replaceFirstChar { it.uppercase() }
    }

    /** cardId's #tags, as extracted from its body (including any added via addCardTag). */
    fun listCardTags(projectId: String, cardId: String): List<String> =
        readLane.call { Cards.getCard(it, projectId, cardId) }.tags

    /** Holder's semantic tag operation -- callers should use this instead of editing #tag text
     * into a card's body themselves, so core stays free to change how tags are represented
     * later without every caller changing too.
     *
     * Adds tag to cardId, normalized to lowercase. Writes it to the card's trailing tag line
     * (its last non-blank line, when that line is nothing but #tags), creating one if none
     * exists yet. A card that already carries the tag, anywhere in it, is left as it is -- a
     * card is either tagged or it isn't, regardless of where the tag text lives. */
    fun addCardTag(projectId: String, cardId: String, tag: String) {
        saveLane.call { Cards.addCardTag(it, projectId, cardId, tag) }
    }

    enum class RemoveTagResult { REMOVED, PRESENT_OUTSIDE_EDITABLE_TAG_LINE }

    /** Removes tag from cardId, normalized to lowercase. Only ever removes text from the card's
     * trailing tag line (see addCardTag) -- this never rewrites prose; a card that does not
     * carry the tag is left as it is. If tag occurs only in prose, returns
     * PRESENT_OUTSIDE_EDITABLE_TAG_LINE rather than silently failing: the card remains tagged
     * since nothing was removed, and the caller should point the user at the text itself. */
    fun removeCardTag(projectId: String, cardId: String, tag: String): RemoveTagResult = try {
        saveLane.call { Cards.removeCardTag(it, projectId, cardId, tag) }
        RemoveTagResult.REMOVED
    } catch (_: ConflictException) {
        RemoveTagResult.PRESENT_OUTSIDE_EDITABLE_TAG_LINE
    }

    /** cardId's tags, and those of them removeCardTag would take off: each is on the card's
     * trailing tag line and nowhere else in it. A tag also written in prose stays until the
     * content is edited, so callers should offer removal only for [HolderCardTags.removable]. */
    fun cardTags(projectId: String, cardId: String): HolderCardTags {
        val card = readLane.call { Cards.getCard(it, projectId, cardId) }
        return HolderCardTags(card.tags, card.removableTags)
    }

    /** Cards in projectId carrying tag (case-insensitive). */
    fun cardsWithTag(projectId: String, tag: String): List<HolderCardRef> = cardsWithTagPaged(projectId, tag, 5000)

    internal fun cardsWithTagPaged(projectId: String, tag: String, pageSize: Int): List<HolderCardRef> {
        val result = ArrayList<HolderCardRef>()
        readLane.call { context ->
            var cursor: String? = null
            do {
                val page = Cards.listCards(context, projectId, tag = tag, order = ListCardsOrder.UPDATED_DESC,
                                           limit = pageSize, cursor = cursor)
                result.ensureCapacity(result.size + page.items.size)
                page.items.forEach { card -> result.add(HolderCardRef(cardId = card.cardId, title = card.title)) }
                cursor = page.nextCursor
            } while (cursor != null)
        }
        return result
    }

    /** Every distinct tag in projectId with how many cards carry it, most-used first. */
    fun listProjectTags(projectId: String): List<HolderTagCount> {
        return readLane.call { Cards.listProjectTags(it, projectId) }
            .map { HolderTagCount(tag = it.tag, count = it.cardCount) }
    }

    // A milestone's dates are roles with a calendar day or an instant. Android, like Desktop, shows
    // one with a single start and at most one end: an all-day milestone when its start is a day.
    // Android's screens hold a day as the epoch of its midnight UTC, and an instant as an epoch
    // written with this device's offset.
    private const val START = "start"
    private const val END = "end"
    private val INSTANT_FORMAT: DateTimeFormatter = DateTimeFormatter.ofPattern("yyyy-MM-dd'T'HH:mm:ssXXX")

    private fun dayOf(epoch: Long): String = Instant.ofEpochSecond(epoch).atOffset(ZoneOffset.UTC).toLocalDate().toString()

    private fun milestoneDate(role: String, epoch: Long, allDay: Boolean) = if (allDay) {
        MilestoneDate(role = role, date = dayOf(epoch))
    } else {
        MilestoneDate(role = role, datetime = Instant.ofEpochSecond(epoch).atZone(ZoneId.systemDefault()).format(INSTANT_FORMAT))
    }

    private fun epochOf(date: MilestoneDate): Long = date.date?.let { LocalDate.parse(it).atStartOfDay(ZoneOffset.UTC).toEpochSecond() }
        ?: OffsetDateTime.parse(requireNotNull(date.datetime)).toEpochSecond()

    /** A milestone as Android shows it, or null when it has no single start or more than one end. */
    private fun holderMilestone(
        milestoneId: String, cardId: String, dates: List<MilestoneDate>, kind: String?, description: String?,
        createdAt: Long, updatedAt: Long, cardTitle: String? = null,
    ): HolderMilestone? {
        val start = dates.singleOrNull { it.role == START } ?: return null
        val ends = dates.filter { it.role == END }
        if (ends.size > 1) return null
        return HolderMilestone(milestoneId, cardId, epochOf(start), ends.singleOrNull()?.let(::epochOf),
                               start.date != null, kind, description, createdAt, updatedAt, cardTitle)
    }

    private fun holderMilestone(m: Milestone) =
        holderMilestone(m.milestoneId, m.cardId, m.dates, m.kind, m.description, m.createdAt, m.updatedAt)

    /** The dates for a start and an optional end, with the milestone's other dates kept in place. */
    private fun withStartAndEnd(kept: List<MilestoneDate>, startAt: Long, endAt: Long?, allDay: Boolean) =
        listOfNotNull(milestoneDate(START, startAt, allDay), endAt?.let { milestoneDate(END, it, allDay) }) +
            kept.filter { it.role != START && it.role != END }

    /** cardId's milestones, ordered by start (soonest first). */
    fun listCardMilestones(projectId: String, cardId: String): List<HolderMilestone> =
        readLane.call { Milestones.listCardMilestones(it, projectId, cardId) }
            .mapNotNull(::holderMilestone).sortedBy { it.startAt }

    /** Adds a milestone to cardId: a point in time when endAt is null, otherwise a span. */
    fun addCardMilestone(
        projectId: String,
        cardId: String,
        startAt: Long,
        endAt: Long? = null,
        allDay: Boolean = false,
        kind: String? = null,
        description: String? = null,
    ): HolderMilestone {
        val milestone = NewMilestone(dates = withStartAndEnd(emptyList(), startAt, endAt, allDay),
                                     kind = kind, description = description)
        return requireNotNull(holderMilestone(saveLane.call { Milestones.addCardMilestone(it, projectId, cardId, milestone) }))
    }

    /** Partially updates cardId's milestoneId. Each parameter left at its default (Unset) means
     * "leave unchanged"; pass Clear to null a clearable field (endAt/kind/description), or a real
     * value to set it. Changing its start, end or all-day keeps the milestone's other dates. */
    fun updateCardMilestone(
        projectId: String,
        cardId: String,
        milestoneId: String,
        startAt: Long? = null,
        endAt: OptionalLong = OptionalLong.Unset,
        allDay: Boolean? = null,
        kind: OptionalString = OptionalString.Unset,
        description: OptionalString = OptionalString.Unset,
    ): HolderMilestone = saveLane.call { context ->
        val dates = if (startAt == null && endAt is OptionalLong.Unset && allDay == null) null else {
            val current = Milestones.listCardMilestones(context, projectId, cardId).single { it.milestoneId == milestoneId }
            val shown = requireNotNull(holderMilestone(current)) { "the milestone has no single start to change" }
            val end = when (endAt) {
                is OptionalLong.Unset -> shown.endAt
                is OptionalLong.Clear -> null
                is OptionalLong.Value -> endAt.value
            }
            withStartAndEnd(current.dates, startAt ?: shown.startAt, end, allDay ?: shown.allDay)
        }
        fun change(value: OptionalString): Change<String> = when (value) {
            is OptionalString.Unset -> Change.Keep
            is OptionalString.Clear -> Change.Clear
            is OptionalString.Value -> Change.Set(value.value)
        }
        val update = MilestoneUpdate(dates = dates, kind = change(kind), description = change(description))
        requireNotNull(holderMilestone(Milestones.updateCardMilestone(context, projectId, cardId, milestoneId, update)))
    }

    /** A no-op, not an error, if milestoneId doesn't exist or belongs to a different card. */
    fun removeCardMilestone(projectId: String, cardId: String, milestoneId: String) {
        try {
            saveLane.call { Milestones.removeCardMilestone(it, projectId, cardId, milestoneId) }
        } catch (_: NotFoundException) {
        }
    }

    /** Every milestone in projectId whose start falls within [from, to] (inclusive), ordered by
     * start -- the Calendar's primary query. Never includes a trashed card's milestones. */
    fun listMilestonesInRange(projectId: String, from: Long, to: Long): List<HolderMilestone> =
        readLane.call { Milestones.getCalendar(it, projectId, from, to, dayOf(from), dayOf(to)) }.milestones
            .mapNotNull { m ->
                holderMilestone(m.milestoneId, m.cardId, m.dates, m.kind, m.description, m.createdAt, m.updatedAt, m.cardTitle)
            }
            .filter { it.startAt in from..to }
            .sortedBy { it.startAt }

    fun searchCards(projectId: String, query: String, limit: Int = 50): List<HolderSearchResult> {
        return readLane.call { Search.searchCards(it, projectId, query, limit) }
            .map { HolderSearchResult(cardId = it.cardId, title = it.title, snippet = it.snippet) }
    }

    /** remoteUrl null or empty, as from an emptied remote field, clears the configured remote. */
    fun updateProjectGitRemote(projectId: String, remoteUrl: String?): HolderProject =
        fromContractProject(saveLane.call {
            val change = if (remoteUrl.isNullOrEmpty()) Change.Clear else Change.Set(remoteUrl)
            Projects.updateProject(it, projectId, ProjectUpdate(gitRemoteUrl = change))
        })

    /** Re-registers the background lane's native git signer with [projectId]'s own key --
     * must only ever be called on the background lane, immediately before the one native call
     * it's preparing for, in the same lane call. The context holds one signer, not one per
     * project, so selecting and using it in one lane call is what stops another project's
     * sync from switching the key in between. Best-effort, matching [initialize]'s own policy: a
     * Keystore hiccup here shouldn't crash a sync, it should just fail that one push/pull with
     * whatever auth error the native side reports -- same failure shape as a real key mismatch
     * would produce. */
    private fun selectGitSignerForProject(contextHandle: Long, projectId: String) {
        val dir = dataDir ?: return
        runCatching {
            team.holder.android.git.GitIdentity.registerWithNative(
                contextHandle,
                dir,
                alias = team.holder.android.git.GitIdentity.aliasForProject(projectId),
            )
        }
    }

    fun testGitRemote(projectId: String, branch: String? = null): GitTestRemoteResult {
        val result = backgroundLane.call {
            selectGitSignerForProject(it, projectId)
            Git.testProjectRemote(it, projectId, RemoteTest())
        }
        return GitTestRemoteResult(
            status = result.status.value,
            remoteHasHead = result.remoteHasHead,
            errorMessage = if (result.status == RemoteTestResultStatus.REMOTE_UNSET) "Remote URL is not configured."
                else result.errorMessage,
        )
    }

    fun probeRemoteUrl(projectId: String, url: String): GitTestRemoteResult {
        val result = backgroundLane.call {
            selectGitSignerForProject(it, projectId)
            Git.testProjectRemote(it, projectId, RemoteTest(remoteUrl = url))
        }
        return GitTestRemoteResult(
            status = result.status.value,
            remoteHasHead = result.remoteHasHead,
            errorMessage = result.errorMessage,
        )
    }

    fun pushGit(projectId: String, branch: String? = null, setUpstream: Boolean = true): GitPushResult {
        val result = backgroundLane.call {
            selectGitSignerForProject(it, projectId)
            Git.pushProject(it, projectId, PushOptions(branch = branch, setUpstream = setUpstream)).push
        }
        return GitPushResult(
            status = result.status.value,
            aheadCount = result.aheadCount,
            behindCount = result.behindCount,
            errorMessage = result.errorMessage,
        )
    }

    fun pullGit(projectId: String): GitPullResult {
        val pull = backgroundLane.call {
            selectGitSignerForProject(it, projectId)
            Git.pullProject(it, projectId).pull
        }
        return GitPullResult(
            // The old Android result reports an unset remote as a failed pull.
            status = if (pull.status == team.holder.android.contract.PullPhaseStatus.REMOTE_UNSET) "failed"
                else pull.status.value,
            errorMessage = pull.errorMessage,
            conflictsResolved = pull.conflictsResolved,
        )
    }

    fun gitSyncStatus(projectId: String): GitSyncStatus {
        val sync = readLane.call { Git.getProjectSyncState(it, projectId) }
        return GitSyncStatus(
            lastPushAt = sync.lastPushAt,
            lastPullAt = sync.lastPullAt,
            uncommittedChangesCount = sync.uncommittedChangesCount,
            unpushedCommitsCount = sync.unpushedCommitsCount,
            lastPushStatus = sync.lastPushStatus,
            lastPullStatus = sync.lastPullStatus,
            lastSyncError = sync.lastSyncError,
        )
    }

    /**
     * Pulls and/or pushes only if enough time has passed since the last attempt of each,
     * per the same cadence policy holder-daemon's background worker uses -- intended for a
     * periodic background job (see GitSyncWorker) to call on a tighter schedule than the
     * desired sync interval, without over-syncing. A no-op if the project has no remote
     * configured, or if neither is due yet. intervalSeconds <= 0 uses the built-in default
     * (1200s push / 300s pull).
     */
    fun gitSyncIfDue(
        projectId: String,
        pushIntervalSeconds: Int = 0,
        pullIntervalSeconds: Int = 0,
    ): GitSyncIfDueResult {
        val result = backgroundLane.call {
            selectGitSignerForProject(it, projectId)
            Git.syncProjectIfDue(it, projectId, maxOf(pushIntervalSeconds, 0), maxOf(pullIntervalSeconds, 0))
        }
        return GitSyncIfDueResult(
            pullAttempted = result.pull.attempted,
            pullStatus = attemptedStatus(result.pull),
            pullError = result.pull.errorMessage,
            pullConflictsResolved = result.pull.conflictsResolved,
            pushAttempted = result.push.attempted,
            pushStatus = attemptedStatus(result.push),
            pushError = result.push.errorMessage,
        )
    }

    /**
     * Runs pull-then-push as one serialized, forced operation -- unlike [gitSyncIfDue], not
     * gated by cadence. A failed pull always skips the push phase, so this is safe to call
     * as a single "Sync now" action rather than composing separate [pullGit]/[pushGit] calls
     * (which have no such ordering guarantee between them). A no-op if the project has no
     * remote configured.
     */
    fun gitSyncNow(
        projectId: String,
        branch: String? = null,
        setUpstream: Boolean = true,
    ): GitSyncNowResult {
        return backgroundLane.call {
            selectGitSignerForProject(it, projectId)
            // The existing Android action leaves a project without a remote untouched.
            if (Projects.getProject(it, projectId).gitRemoteUrl.isNullOrEmpty()) {
                GitSyncNowResult(false, null, null, 0, false, null, null, 0, 0, null)
            } else {
                val options = PushOptions(branch = branch?.takeIf { value -> value.isNotEmpty() }, setUpstream = setUpstream)
                val result = Git.syncProject(it, projectId, options)
                GitSyncNowResult(
                    pullAttempted = result.pull.attempted,
                    pullStatus = attemptedStatus(result.pull),
                    pullError = result.pull.errorMessage,
                    pullConflictsResolved = result.pull.conflictsResolved,
                    pushAttempted = result.push.attempted,
                    pushStatus = attemptedStatus(result.push),
                    pushError = result.push.errorMessage,
                    pushAheadCount = result.push.aheadCount,
                    pushBehindCount = result.push.behindCount,
                    pushLocalHeadCommit = result.push.localHeadCommit,
                )
            }
        }
    }

    /** A sync phase's status, or null when the phase was not attempted, as Android reports it. */
    private fun attemptedStatus(phase: PullPhase): String? = if (phase.attempted) phase.status.value else null

    private fun attemptedStatus(phase: PushPhase): String? = if (phase.attempted) phase.status.value else null

    fun encryptionCheck(projectId: String): EncryptionCheckResult {
        val result = backgroundLane.call { Maintenance.checkProjectEncryption(it, projectId) }
        val check = result.check
        return EncryptionCheckResult(
            privacyMode = result.privacyMode.value,
            ok = check.ok,
            checkedFiles = check.checkedFiles,
            unsafeFiles = check.unsafeFiles,
            unsafePaths = check.unsafePaths,
            message = check.message,
        )
    }

    fun exportRecoveryToken(projectId: String, pin: String): RecoveryTokenExportResult {
        val result = backgroundLane.call {
            Recovery.exportProjectRecoveryToken(it, projectId, RecoveryTokenExportRequest(pin = pin))
        }
        return RecoveryTokenExportResult(
            projectId = result.projectId,
            keyId = result.keyId,
            recoveryToken = result.recoveryToken,
        )
    }

    /** Re-establishes projectId's key material from a recovery token; projectId must already
     * exist and match the token's own project id. See importRecoveryTokenGlobal for recovering
     * onto a device that doesn't have the project yet. */
    fun importRecoveryToken(projectId: String, pin: String, recoveryToken: String) {
        saveLane.call {
            Recovery.importProjectRecoveryToken(it, projectId, RecoveryTokenImportRequest(pin = pin, recoveryToken = recoveryToken))
        }
    }

    /** The device-setup path: recovers a project from just a PIN and a recovery token, creating
     * it first if this device has never seen it before, and pulling its remote if the token
     * includes one. */
    fun importRecoveryTokenGlobal(pin: String, recoveryToken: String): RecoveryTokenImportGlobalResult {
        val result = saveLane.call {
            Recovery.importRecoveryTokenGlobal(it, RecoveryTokenImportRequest(pin = pin, recoveryToken = recoveryToken))
        }
        return RecoveryTokenImportGlobalResult(
            projectId = result.projectId,
            projectCreated = result.projectCreated,
            remoteHintPresent = result.remoteHintPresent,
            remoteConfigured = result.remoteConfigured,
            remoteError = result.remoteError,
            pullStatus = result.pullStatus.value,
            pullError = result.pullError,
        )
    }

    /** Storage locations (e.g. a Google Drive folder) available to attach Resources into,
     * within projectId. */
    /** projectId's storage locations, most recently updated first. */
    fun listLocations(projectId: String): List<HolderLocation> =
        readLane.call { Locations.listStorageLocations(it, projectId) }.data.map { location ->
            HolderLocation(location.locationId, location.projectId, location.name, location.provider.value,
                           location.configuration, location.createdAt, location.updatedAt)
        }

    /** Creates or updates a storage location. locationId identifies which -- pass a fresh
     * uuid to create one. */
    fun createLocation(
        locationId: String,
        projectId: String,
        name: String,
        provider: String,
        configuration: Map<String, String>,
        now: Long,
    ): HolderLocation {
        val location = NewStorageLocation(
            locationId = locationId, projectId = projectId, name = name,
            provider = NewStorageLocationProvider.entries.single { it.value == provider },
            configuration = configuration, createdAt = now, updatedAt = now,
        )
        val created = saveLane.call { Locations.createStorageLocation(it, location) }
        return HolderLocation(created.locationId, created.projectId, created.name, created.provider.value,
                              created.configuration, created.createdAt, created.updatedAt)
    }

    fun getResource(projectId: String, resourceId: String): HolderResource =
        fromContractResource(readLane.call { Resources.getResource(it, projectId, resourceId) })

    fun listResources(projectId: String): List<HolderResource> =
        readLane.call { Resources.listResources(it, projectId) }.map(::fromContractResource)

    private fun fromContractResource(resource: Resource) =
            HolderResource(
                resourceId = resource.resourceId,
                projectId = resource.projectId,
                type = resource.type,
                label = resource.label,
                createdAt = resource.createdAt,
                updatedAt = resource.updatedAt,
                assets = resource.assets.map { asset -> HolderAsset(
                    assetId = asset.assetId,
                    resourceId = resource.resourceId,
                    originalFilename = asset.originalFilename,
                    mediaType = asset.mediaType,
                    byteSize = asset.byteSize,
                    plaintextSha256 = asset.plaintextSha256,
                    createdAt = asset.createdAt,
                    updatedAt = asset.updatedAt,
                    placements = asset.placements.map { placement -> HolderPlacement(
                        placementId = placement.placementId,
                        locationId = placement.locationId,
                        encoding = placement.encoding.value,
                        storedByteSize = placement.storedByteSize,
                        createdAt = placement.createdAt,
                    ) },
                ) },
            )

    /** Imports sourceFilePath into projectId's Resource/Asset model, stores it via
     * locationId's storage provider (see [team.holder.android.resource.AndroidStorageProvider]),
     * and attaches it to cardId -- both as a structural link and, separately, by inserting a
     * `![label](holder://resource/<id>)` reference into the card's body, the caller's job just
     * like it is for holder-desktop (see HolderMarkdownViewer). Deduplicates by content hash
     * within the project -- a second import of identical bytes reuses the existing Resource. */
    fun importAsset(projectId: String, cardId: String, locationId: String, sourceFilePath: String): HolderAssetImportResult {
        val result = backgroundLane.call {
            Resources.importAssetFile(it, AssetImportRequest(projectId, cardId, locationId, sourceFilePath))
        }
        return HolderAssetImportResult(result.resourceId, result.assetId, result.duplicateReused, result.linkCreated)
    }

    /** Downloads assetId's placementId back down to destinationFilePath via that placement's
     * Location's storage provider, verified against the Asset's recorded size/hash; bytes that fail
     * the check throw DataCorruptedException and leave no file. */
    fun retrieveAsset(resourceId: String, assetId: String, placementId: String, destinationFilePath: String) {
        backgroundLane.call { Resources.retrieveAssetFile(it, resourceId, assetId, placementId, destinationFilePath) }
    }

    private fun fromContractMetadata(card: CardMetadata) = HolderCard(
        card.cardId, card.projectId, card.title, card.parentCardId,
        card.createdAt, card.updatedAt, card.sortKey, card.deletedAt,
    )

    private fun fromContractCard(card: Card) = HolderCard(
        card.cardId, card.projectId, card.title, card.parentCardId,
        card.createdAt, card.updatedAt, card.sortKey, card.deletedAt,
    )

    private fun JSONArray.toStringList(): List<String> = List(length()) { index -> getString(index) }

    private fun JSONObject.optStringOrNull(name: String): String? =
        if (isNull(name)) null else getString(name)

    private fun JSONObject.optLongOrNull(name: String): Long? =
        if (isNull(name)) null else getLong(name)

    private fun JSONObject.optIntOrNull(name: String): Int? =
        if (isNull(name)) null else getInt(name)

    private fun JSONObject.toStringMap(): Map<String, String> {
        val map = mutableMapOf<String, String>()
        keys().forEach { key -> map[key] = getString(key) }
        return map
    }

    /** Mirrors holder-daemon's Bootstrap.cpp: first line, only if it's a markdown heading. */
    private fun deriveWelcomeTitle(content: String, fallback: String): String {
        val firstLine = content.substringBefore('\n').trimStart(' ', '\t', '\r')
        if (firstLine.isEmpty() || firstLine[0] != '#') return fallback
        val title = firstLine.trimStart('#', ' ', '\t')
        return title.ifEmpty { fallback }
    }
}
