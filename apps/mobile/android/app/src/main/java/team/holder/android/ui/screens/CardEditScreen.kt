package team.holder.android.ui.screens

import android.net.Uri
import androidx.activity.compose.BackHandler
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.PickVisualMediaRequest
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.ExperimentalFoundationApi
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.text.input.TextFieldLineLimits
import androidx.compose.foundation.text.input.TextFieldState
import androidx.compose.foundation.text.input.rememberTextFieldState
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Check
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.onFocusChanged
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.painterResource
import androidx.compose.ui.text.TextRange
import androidx.compose.ui.unit.dp
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.snapshotFlow
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.SupervisorJob
import team.holder.android.editor.CardAutosaver
import team.holder.android.editor.CardWriter
import team.holder.android.editor.EditorRecoveryStore
import team.holder.android.editor.EditorSnapshot
import team.holder.android.editor.HolderCardWriter
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import team.holder.android.HolderNative
import team.holder.android.HolderSettings
import team.holder.android.R
import team.holder.android.combineTitleAndBody
import team.holder.android.resource.AttachFlowConnectDialog
import team.holder.android.resource.createCameraCaptureUri
import team.holder.android.resource.rememberAttachFlow
import team.holder.android.splitTitleLine
import team.holder.android.titleFromFirstLine
import team.holder.android.ui.markdown.HolderMarkdownEditor
import team.holder.android.ui.markdown.MarkdownFormattingToolbar
import team.holder.android.ui.markdown.trimTrailingWhitespaceForSave

/**
 * Edits a card, or writes a new one, saving as the user types (issue #447): nothing typed here
 * is ever thrown away. There is no Save button and no "discard changes" -- Done (or Back) just
 * leaves once the latest text is in the card. Any earlier version can be restored from the
 * card's history.
 *
 * See [CardAutosaver] for the two layers (a device-local recovery copy on every keypress, a
 * real save 900ms after typing pauses). On top of those, this screen saves immediately when the app
 * goes into the background or the screen goes away, and records itself as the open editor so
 * a cold start can reopen it.
 */
@OptIn(ExperimentalMaterial3Api::class, ExperimentalFoundationApi::class)
@Composable
fun CardEditScreen(
    screenTitle: String,
    initialTitle: String,
    initialContent: String,
    // Non-null only when this screen was reached by tapping a specific spot in the rendered
    // body (see HolderMarkdownViewer's onRequestEditAt / click_to_edit_position.md) -- null for
    // every other path in (the bottom-bar Edit button, long-press-anywhere, a fresh "new card"),
    // which keep today's behavior of landing at the end of the body.
    initialCursorOffset: Int? = null,
    // What an otherwise-untitled card falls back to: "Untitled" normally, or
    // "Untitled child of {parent title}" when this screen was reached via a "Create child card"
    // action -- MainActivity decides which, this screen just shows/uses whatever it's given.
    // Never blank itself.
    defaultTitle: String = "Untitled",
    projectId: String = "",
    // Only relevant on the "new card" screen (cardId == null): the parent the card is created
    // under once there's something in it.
    parentCardId: String? = null,
    cardId: String? = null,
    // Leaving with everything saved. The id is the card's (a new card's, once created), or null
    // for a new card that ended up with nothing in it and so was never kept.
    onDone: (cardId: String?) -> Unit,
    // Only ever called for an existing card (cardId != null) confirmed-deleted from the
    // "Delete this empty card?" dialog below -- unlike onDone, this needs to pop back past the
    // now-nonexistent card's own view, not just one level.
    onDeleted: () -> Unit = {},
    // Fired the moment a "new card" screen's card is first created (first autosave with text in
    // it, or an attach action).
    onCardCreated: (cardId: String) -> Unit = {},
    // Where cards are written; tests substitute a fake for libholder.
    writer: CardWriter = HolderCardWriter,
    // Null: the app's own store under filesDir.
    recoveryStore: EditorRecoveryStore? = null,
) {
    val context = LocalContext.current
    // Every launch on this scope says Dispatchers.Main.immediate explicitly: under the Compose
    // test harness the scope doesn't bring work back to the main thread after a withContext(IO)
    // save, and what follows (navigation, focus) has to run there.
    val scope = rememberCoroutineScope()
    // Null until DataStore has answered: the editor's text depends on these (title mode, how
    // whitespace is trimmed), so autosave waits for them rather than "saving" a reformatting.
    val separateTitleSetting by HolderSettings.separateTitleEnabled(context).collectAsState(initial = null)
    val preserveTrailingWhitespaceSetting by HolderSettings.preserveTrailingWhitespace(context).collectAsState(initial = null)
    val trimTwoSpaceLineEndingsSetting by HolderSettings.trimTwoSpaceLineEndings(context).collectAsState(initial = null)
    val trimWhitespaceInCodeBlocksSetting by HolderSettings.trimWhitespaceInCodeBlocks(context).collectAsState(initial = null)
    val settingsLoaded = separateTitleSetting != null && preserveTrailingWhitespaceSetting != null &&
        trimTwoSpaceLineEndingsSetting != null && trimWhitespaceInCodeBlocksSetting != null
    val separateTitle = separateTitleSetting ?: true

    // Two independent field states, one per mode -- only one is ever shown, but both are
    // deterministically derived from initialContent regardless of which value `separateTitle`
    // resolves to (it loads asynchronously from DataStore), so whichever ends up displayed is
    // already correctly initialized. See splitTitleLine/combineTitleAndBody: a card's first line
    // is its title, as a `# Title` heading or as plain text, and saving keeps whichever it was,
    // so switching modes is non-destructive.
    // All three TextFieldStates survive process death via their own built-in Saver.
    val titleState = rememberTextFieldState(initialTitle)
    val initialSplit = remember(initialContent, initialTitle) { splitTitleLine(initialContent, initialTitle) }
    val initialSeparateBody = initialSplit?.body ?: initialContent
    // Saved back the way it came: a plain first-line title stays plain. New cards get a heading.
    val titleAsHeading = initialSplit?.asHeading ?: true
    val initialFirstLineBody = remember(initialContent) { initialContent.ifBlank { "# $defaultTitle\n\n" } }

    // initialCursorOffset arrives measured against initialContent itself (see
    // HolderMarkdownViewer's onRequestEditAt / CardViewScreen's translation of a tap into that
    // coordinate space) -- separateBodyState needs it re-based onto initialSeparateBody, which is
    // initialContent with its leading "# Title" heading (and the blank line after it) already
    // stripped off the front. firstLineBodyState shows initialContent verbatim (short of the
    // blank-card fallback below, which a tap could never have produced an offset against in the
    // first place), so it takes the same offset unchanged. Both coerce into range as cheap
    // insurance against a stale offset from a body that's changed length since the tap, not a
    // case expected to actually happen in the normal flow.
    val headingPrefixLength = initialContent.length - initialSeparateBody.length
    val separateBodyState = rememberTextFieldState(
        initialSeparateBody,
        TextRange(
            (initialCursorOffset?.minus(headingPrefixLength) ?: initialSeparateBody.length)
                .coerceIn(0, initialSeparateBody.length),
        ),
    )
    val firstLineBodyState = rememberTextFieldState(
        initialFirstLineBody,
        TextRange((initialCursorOffset ?: initialFirstLineBody.length).coerceIn(0, initialFirstLineBody.length)),
    )

    // Undo/redo acts on whichever field last had focus (defaulting to the body, since that's
    // where most editing happens); each TextFieldState tracks its own history independently
    // via its built-in undoState.
    var titleFocused by remember { mutableStateOf(false) }
    val activeUndo = if (!separateTitle) {
        firstLineBodyState.undoState
    } else if (titleFocused) {
        titleState.undoState
    } else {
        separateBodyState.undoState
    }
    val activeBodyState = if (separateTitle) separateBodyState else firstLineBodyState

    // Starts as the passed-in cardId (null on a fresh "new card" screen) and gains a real value
    // the moment the autosaver creates the card. Saveable, with the recovery key, so an
    // activity recreated by Android carries on with the same card and recovery copy instead of
    // creating a second one.
    var currentCardId by rememberSaveable { mutableStateOf(cardId) }
    var recoveryKey by rememberSaveable { mutableStateOf<String?>(null) }
    // Null until there's something to report (a new card with nothing typed shows no status).
    var status by remember { mutableStateOf<CardAutosaver.Status?>(if (cardId != null) CardAutosaver.Status.Saved else null) }
    val store = recoveryStore ?: remember { EditorRecoveryStore.forContext(context) }
    val autosaver = remember {
        CardAutosaver(
            scope = scope,
            writer = writer,
            store = store,
            projectId = projectId,
            parentCardId = parentCardId,
            initialCardId = currentCardId,
            initialRecoveryKey = recoveryKey,
            initialSaved = null,
            onCardCreated = { id ->
                currentCardId = id
                recoveryKey = EditorRecoveryStore.keyFor(projectId, id)
                onCardCreated(id)
            },
            onStatus = { status = it },
        )
    }
    LaunchedEffect(Unit) { if (recoveryKey == null) recoveryKey = autosaver.recoveryKey }

    // The open editor, for reopening after a cold start; cleared when it's left normally.
    LaunchedEffect(currentCardId, recoveryKey) {
        val key = recoveryKey ?: return@LaunchedEffect
        runCatching { HolderSettings.setEditorSession(context, HolderSettings.EditorSession(projectId, currentCardId, key)) }
    }

    // rememberAttachFlow re-keys on cardId changes (see its own remember(projectId, cardId)),
    // so this picks up currentCardId's real value automatically the moment the card is created.
    val attachFlow = rememberAttachFlow(projectId, currentCardId.orEmpty()) { markdown ->
        insertOwnLine(activeBodyState, markdown)
    }

    val photoPickerLauncher = rememberLauncherForActivityResult(ActivityResultContracts.PickVisualMedia()) { uri ->
        if (uri == null) return@rememberLauncherForActivityResult
        attachFlow.attach(uri)
    }

    var pendingCameraUri by remember { mutableStateOf<Uri?>(null) }
    val cameraLauncher = rememberLauncherForActivityResult(ActivityResultContracts.TakePicture()) { success ->
        if (success) pendingCameraUri?.let { attachFlow.attach(it) }
        pendingCameraUri = null
    }

    // What would be saved for the given field texts. Shared by the live editor and by the text
    // the editor opened with, so the two compare equal unless the user actually changed
    // something (the same trimming and title rules apply to both).
    fun snapshotOf(title: String, separateBody: String, firstLineBody: String, cursor: Int): EditorSnapshot {
        // Falls all the way back to defaultTitle (never blank): a card with real content
        // shouldn't be unsaveable just because its Title field (Separate mode) was left empty.
        val derivedTitle = if (separateTitle) {
            title.ifBlank { titleFromFirstLine(separateBody).ifBlank { defaultTitle } }
        } else {
            titleFromFirstLine(firstLineBody).ifBlank { defaultTitle }
        }
        val raw = if (separateTitle) {
            combineTitleAndBody(derivedTitle, separateBody, titleAsHeading)
        } else {
            firstLineBody.ifBlank { "# $derivedTitle\n\n" }
        }
        val content = trimTrailingWhitespaceForSave(
            raw,
            preserve = preserveTrailingWhitespaceSetting ?: false,
            trimTwoSpaceLineEndings = trimTwoSpaceLineEndingsSetting ?: false,
            trimWhitespaceInCodeBlocks = trimWhitespaceInCodeBlocksSetting ?: false,
        )
        val blank = if (separateTitle) title.isBlank() && separateBody.isBlank() else firstLineBody.isBlank()
        return EditorSnapshot(derivedTitle, content, blank, cursor)
    }

    val opening = remember(settingsLoaded, separateTitle) {
        snapshotOf(initialTitle, initialSeparateBody, initialFirstLineBody, 0)
    }

    fun liveSnapshot(): EditorSnapshot {
        val live = snapshotOf(
            titleState.text.toString(),
            separateBodyState.text.toString(),
            firstLineBodyState.text.toString(),
            activeBodyState.selection.start,
        )
        // A new card still showing exactly what it opened with (e.g. First line mode's
        // "# Untitled" placeholder) counts as nothing typed: opening the screen alone
        // shouldn't create a card.
        val untouched = cardId == null && live.title == opening.title && live.content == opening.content
        return if (untouched) live.copy(blank = true) else live
    }

    // Feed every change to the autosaver, once settings are known.
    LaunchedEffect(settingsLoaded) {
        if (!settingsLoaded) return@LaunchedEffect
        if (cardId != null) autosaver.markSaved(opening)
        snapshotFlow { liveSnapshot() }.collect { autosaver.onEdited(it) }
    }

    // App going into the background: the recovery copy now (synchronously; the process may be
    // killed any moment after this), and the real save straight away.
    val lifecycleOwner = LocalLifecycleOwner.current
    DisposableEffect(lifecycleOwner) {
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_STOP) {
                autosaver.writeRecoveryNow()
                scope.launch(Dispatchers.Main.immediate) { autosaver.flush() }
            }
        }
        lifecycleOwner.lifecycle.addObserver(observer)
        onDispose {
            lifecycleOwner.lifecycle.removeObserver(observer)
            // Leaving by any route other than Done (e.g. a notification navigating elsewhere):
            // this screen's scope is about to be cancelled, so finish the save on the app's.
            autosaver.writeRecoveryNow()
            EditorSaveScope.launch { autosaver.flush() }
        }
    }

    // Attaching needs a real card, even on a new card with nothing typed yet.
    suspend fun ensureCardCreated(): String? = autosaver.ensureCreated(liveSnapshot())

    var leaving by remember { mutableStateOf(false) }
    var showDeleteEmptyDialog by remember { mutableStateOf(false) }
    var showLeaveAnywayDialog by remember { mutableStateOf(false) }
    var isDeleting by remember { mutableStateOf(false) }

    fun finish(savedCardId: String?) {
        scope.launch(Dispatchers.Main.immediate) { runCatching { HolderSettings.setEditorSession(context, null) } }
        onDone(savedCardId)
    }

    // Done and Back: make sure the latest text is in the card, then leave. An existing card
    // emptied out asks whether to delete it; a new card with nothing in it just goes.
    fun done() {
        if (leaving) return
        leaving = true
        scope.launch(Dispatchers.Main.immediate) {
            val snap = liveSnapshot()
            autosaver.onEdited(snap)
            if (snap.blank && cardId == null) {
                autosaver.discardEmpty()
                finish(null)
                return@launch
            }
            val saved = autosaver.flush()
            leaving = false
            when {
                !saved -> showLeaveAnywayDialog = true
                snap.blank -> showDeleteEmptyDialog = true
                else -> finish(autosaver.cardId)
            }
        }
    }

    BackHandler(onBack = ::done)

    Scaffold(
        topBar = {
            TopAppBar(
                title = {
                    Column {
                        Text(screenTitle)
                        status?.let { SaveStatusLine(it) }
                    }
                },
                navigationIcon = {
                    IconButton(onClick = ::done) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                actions = {
                    IconButton(onClick = { activeUndo.undo() }, enabled = activeUndo.canUndo) {
                        Icon(painterResource(R.drawable.ic_undo), contentDescription = "Undo")
                    }
                    IconButton(onClick = { activeUndo.redo() }, enabled = activeUndo.canRedo) {
                        Icon(painterResource(R.drawable.ic_redo), contentDescription = "Redo")
                    }
                    if (leaving) {
                        CircularProgressIndicator(modifier = Modifier.padding(12.dp))
                    } else {
                        IconButton(onClick = ::done) {
                            Icon(Icons.Filled.Check, contentDescription = "Done")
                        }
                    }
                },
            )
        },
        bottomBar = {
            // Formatting only makes sense for the body -- hidden while the plain single-line
            // Title field has focus in Separate mode; always shown in First line mode, since
            // there's only the one field.
            if (!separateTitle || !titleFocused) {
                MarkdownFormattingToolbar(
                    state = activeBodyState,
                    modifier = Modifier.fillMaxWidth().imePadding(),
                    // Always offered, even on a still-uncreated "new card" screen: attaching
                    // itself creates the card if there isn't one yet.
                    onAttachPhoto = if (!attachFlow.attaching) {
                        {
                            scope.launch(Dispatchers.Main.immediate) {
                                if (ensureCardCreated() != null) {
                                    photoPickerLauncher.launch(PickVisualMediaRequest(ActivityResultContracts.PickVisualMedia.ImageOnly))
                                }
                            }
                        }
                    } else {
                        null
                    },
                    onAttachCamera = if (!attachFlow.attaching) {
                        {
                            scope.launch(Dispatchers.Main.immediate) {
                                if (ensureCardCreated() != null) {
                                    val uri = createCameraCaptureUri(context)
                                    pendingCameraUri = uri
                                    cameraLauncher.launch(uri)
                                }
                            }
                        }
                    } else {
                        null
                    },
                    attaching = attachFlow.attaching,
                )
            }
        },
    ) { innerPadding ->
        Column(modifier = Modifier.fillMaxSize().padding(innerPadding).padding(16.dp)) {
            if (attachFlow.attachError != null) {
                Text(
                    text = "Couldn't attach photo: ${attachFlow.attachError}",
                    color = MaterialTheme.colorScheme.error,
                    modifier = Modifier.padding(bottom = 8.dp),
                )
            }
            if (separateTitle) {
                OutlinedTextField(
                    state = titleState,
                    label = { Text("Title") },
                    // Previews exactly what leaving this field blank will actually produce --
                    // the same fallback chain the saved title uses -- rather than a generic
                    // hint, so there's never a surprise between what's shown here and what gets
                    // saved.
                    placeholder = {
                        Text(titleFromFirstLine(separateBodyState.text.toString()).ifBlank { defaultTitle })
                    },
                    lineLimits = TextFieldLineLimits.SingleLine,
                    modifier = Modifier.fillMaxWidth().onFocusChanged { titleFocused = it.isFocused },
                )
                HolderMarkdownEditor(
                    state = separateBodyState,
                    modifier = Modifier.fillMaxWidth().weight(1f).padding(top = 12.dp),
                    autoFocus = true,
                )
            } else {
                HolderMarkdownEditor(
                    state = firstLineBodyState,
                    modifier = Modifier.fillMaxWidth().weight(1f),
                    autoFocus = true,
                )
            }
        }
    }

    // The save didn't go through (it keeps retrying in the background meanwhile). Leaving is
    // still safe: the text stays in the recovery copy and is saved when Holder next starts.
    if (showLeaveAnywayDialog) {
        AlertDialog(
            onDismissRequest = { showLeaveAnywayDialog = false },
            title = { Text("Not saved yet") },
            text = {
                val reason = (status as? CardAutosaver.Status.Failed)?.message
                Text(
                    (if (reason != null) "Holder couldn't save this card ($reason). " else "Holder couldn't save this card yet. ") +
                        "Your text is kept on this device and will be saved when Holder can.",
                )
            },
            confirmButton = {
                TextButton(onClick = { showLeaveAnywayDialog = false }) { Text("Keep editing") }
            },
            dismissButton = {
                TextButton(onClick = {
                    showLeaveAnywayDialog = false
                    onDone(currentCardId)
                }) { Text("Leave") }
            },
        )
    }

    // An existing card wiped down to nothing -- almost always means "get rid of this", so this
    // offers the delete (to the trash) instead of quietly keeping an empty placeholder.
    // The emptied text is already saved either way, and the card's history has what it was.
    if (showDeleteEmptyDialog) {
        AlertDialog(
            onDismissRequest = { if (!isDeleting) showDeleteEmptyDialog = false },
            title = { Text("Delete this empty card?") },
            text = { Text("You have cleared the title and content.") },
            confirmButton = {
                TextButton(
                    enabled = !isDeleting,
                    onClick = {
                        if (!isDeleting) {
                            isDeleting = true
                            scope.launch(Dispatchers.Main.immediate) {
                                runCatching { autosaver.discardEmpty() }
                                isDeleting = false
                                showDeleteEmptyDialog = false
                                runCatching { HolderSettings.setEditorSession(context, null) }
                                onDeleted()
                            }
                        }
                    },
                ) { Text("Delete") }
            },
            dismissButton = {
                TextButton(
                    enabled = !isDeleting,
                    onClick = {
                        showDeleteEmptyDialog = false
                        finish(currentCardId)
                    },
                ) { Text("Keep") }
            },
        )
    }

    AttachFlowConnectDialog(attachFlow)
}

/** "Saved" / "Saving…" / why it isn't, under the editor's title. */
@Composable
private fun SaveStatusLine(status: CardAutosaver.Status) {
    val (text, color) = when (status) {
        CardAutosaver.Status.Saved -> "Saved" to MaterialTheme.colorScheme.onSurfaceVariant
        CardAutosaver.Status.Pending -> "Saving…" to MaterialTheme.colorScheme.onSurfaceVariant
        is CardAutosaver.Status.Failed -> "Not saved yet, retrying: ${status.message}" to MaterialTheme.colorScheme.error
    }
    Text(text, style = MaterialTheme.typography.labelMedium, color = color, maxLines = 1)
}

/** Finishes saves that outlive the editor screen's own scope (see its onDispose). */
internal object EditorSaveScope : CoroutineScope by CoroutineScope(SupervisorJob() + Dispatchers.Main.immediate)

/** Inserts [text] as its own CommonMark paragraph -- a full blank line before it (unless the
 * cursor is at the very start of the document) and after it, mirroring holder-desktop's
 * MarkdownResourceImageController.block_insertion for the same
 * `![label](holder://resource/<id>)` references, so a card edited on either platform ends up
 * with the same shape around an attached image. A single newline is only a *soft line break*
 * within the same paragraph as whatever precedes it -- inserting with just one, as this used
 * to, merges the reference into an adjacent line of text, which silently drops image
 * rendering entirely: HolderMarkdownViewer only renders `![...](holder://resource/...)` as an
 * image when it is the sole child of its own paragraph, so a merged paragraph falls back to
 * showing just the image's alt text as plain words. */
@OptIn(ExperimentalFoundationApi::class)
private fun insertOwnLine(state: TextFieldState, text: String) {
    state.edit {
        val cursor = selection.start
        val content = asCharSequence()
        val before = content.subSequence(0, cursor).toString()
        val leading = when {
            before.isEmpty() -> ""
            before.endsWith("\n\n") -> ""
            before.endsWith("\n") -> "\n"
            else -> "\n\n"
        }
        val insertion = "$leading$text\n\n"
        replace(cursor, cursor, insertion)
        placeCursorBeforeCharAt(cursor + insertion.length)
    }
}
