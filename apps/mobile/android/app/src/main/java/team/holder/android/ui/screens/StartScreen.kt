package team.holder.android.ui.screens

import android.content.res.Configuration
import android.text.format.DateUtils
import androidx.compose.animation.animateContentSize
import androidx.compose.animation.core.Animatable
import androidx.compose.animation.core.tween
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ExperimentalLayoutApi
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.WindowInsetsSides
import androidx.compose.foundation.layout.consumeWindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.ime
import androidx.compose.foundation.layout.imeAnimationSource
import androidx.compose.foundation.layout.imeAnimationTarget
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.only
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.List
import androidx.compose.material.icons.filled.ArrowDropDown
import androidx.compose.material.icons.filled.Check
import androidx.compose.material.icons.filled.DateRange
import androidx.compose.material.icons.filled.Search
import androidx.compose.material.icons.filled.Warning
import androidx.compose.material3.AssistChip
import androidx.compose.material3.DropdownMenu
import androidx.compose.material3.DropdownMenuItem
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.ListItem
import androidx.compose.material3.ListItemDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedCard
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarDuration
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.SnackbarResult
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TextField
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.material3.TopAppBar
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.rememberUpdatedState
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.LifecycleEventObserver
import androidx.lifecycle.compose.LocalLifecycleOwner
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.clipToBounds
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.graphicsLayer
import androidx.compose.ui.graphics.lerp
import androidx.compose.ui.layout.layout
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.platform.LocalSoftwareKeyboardController
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.input.KeyboardCapitalization
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import kotlin.math.roundToInt
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import team.holder.android.HolderNative
import team.holder.android.HolderProject
import team.holder.android.HolderSettings
import team.holder.android.titleFromFirstLine

/**
 * Holder's start screen: what needs attention (a milestone strip, only when something is due),
 * a quick-entry editor for capturing a new card without navigating anywhere, and the most
 * recently edited cards across every project. See issue #449.
 *
 * Layout and motion were settled with a mockup on a phone; the parts worth knowing:
 *  - While the keyboard is up, the strip and Recent collapse (and, in landscape, the app bar) so
 *    the editor gets the whole screen. All of it is driven by the keyboard's own inset progress,
 *    not by separate timed animations, which made the editor visibly bounce.
 *  - Recent gets at most half the height in portrait and 30% in landscape, and only as much as
 *    its rows need; the editor takes everything else.
 */
private val STRIP_MAX_HEIGHT = 112.dp

@OptIn(ExperimentalMaterial3Api::class, ExperimentalLayoutApi::class)
@Composable
fun StartScreen(
    refreshKey: Int,
    onOpenCard: (projectId: String, cardId: String, title: String) -> Unit,
    onProjectsClick: () -> Unit,
    onSearchClick: () -> Unit,
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    val focusManager = LocalFocusManager.current
    val keyboard = LocalSoftwareKeyboardController.current
    val snackbar = remember { SnackbarHostState() }
    val listState = rememberLazyListState()
    val landscape = LocalConfiguration.current.orientation == Configuration.ORIENTATION_LANDSCAPE

    var data by remember { mutableStateOf<StartScreenData?>(null) }
    var loadError by remember { mutableStateOf<String?>(null) }
    var reloadKey by remember { mutableStateOf(0) }
    var highlightedCardId by remember { mutableStateOf<String?>(null) }

    // Null until the persisted draft has been read, so an empty initial value never overwrites it.
    var draft by remember { mutableStateOf<String?>(null) }
    var draftProjectId by remember { mutableStateOf<String?>(null) }
    var saving by remember { mutableStateOf(false) }

    LaunchedEffect(Unit) {
        draftProjectId = HolderSettings.startDraftProjectId(context).first()
        val stored = HolderSettings.startDraft(context).first()
        if (draft == null) draft = stored
    }
    LaunchedEffect(draft) {
        val text = draft ?: return@LaunchedEffect
        delay(300)
        HolderSettings.setStartDraft(context, text)
    }
    // Going into the background: write the draft now rather than after the debounce, in case
    // Android ends the process before it fires (issue #447: every keypress is kept).
    val lifecycleOwner = LocalLifecycleOwner.current
    val latestDraft by rememberUpdatedState(draft)
    DisposableEffect(lifecycleOwner) {
        val observer = LifecycleEventObserver { _, event ->
            if (event == Lifecycle.Event.ON_STOP) {
                latestDraft?.let { text -> EditorSaveScope.launch { HolderSettings.setStartDraft(context, text) } }
            }
        }
        lifecycleOwner.lifecycle.addObserver(observer)
        onDispose { lifecycleOwner.lifecycle.removeObserver(observer) }
    }
    LaunchedEffect(refreshKey, reloadKey) {
        runCatching { withContext(Dispatchers.IO) { loadStartScreenData() } }
            .onSuccess { data = it; loadError = null }
            .onFailure { loadError = it.message ?: it::class.java.simpleName }
    }

    // Bring a just-saved card into view once the reload has actually delivered it. Scrolling
    // any earlier does nothing useful: when the new row then arrives at index 0, the lazy list
    // keeps the previous first row anchored in place, leaving the new card just off-screen above.
    LaunchedEffect(data) {
        val id = highlightedCardId ?: return@LaunchedEffect
        if (data?.recentCards?.firstOrNull()?.cardId == id) listState.animateScrollToItem(0)
    }

    val projects = data?.projects.orEmpty()
    val multipleProjects = projects.size > 1
    val homeProject = projects.firstOrNull { it.name == HolderNative.DEFAULT_PROJECT_NAME } ?: projects.firstOrNull()
    val selectedProject = projects.firstOrNull { it.projectId == draftProjectId } ?: homeProject

    // Keyboard progress, 0 (down) to 1 (up), read from the IME insets every frame. Everything
    // that makes room for the editor is driven from this one value so it moves with the keyboard.
    val density = LocalDensity.current
    val imeNow = WindowInsets.ime.getBottom(density)
    val imeFull = maxOf(
        WindowInsets.imeAnimationSource.getBottom(density),
        WindowInsets.imeAnimationTarget.getBottom(density),
        imeNow,
    )
    val imeProgress = if (imeFull > 0) imeNow.toFloat() / imeFull else 0f
    val visibleFraction = 1f - imeProgress

    fun save() {
        val text = draft?.takeIf { it.isNotBlank() } ?: return
        val project = selectedProject ?: return
        if (saving) return
        saving = true
        // Explicitly Main: under the Compose test harness the scope's dispatcher doesn't bring
        // us back to the main thread after withContext(IO), and clearFocus/hide touch Views.
        // Same reason the new-card route wraps its post-save work in Main.immediate.
        scope.launch(Dispatchers.Main.immediate) {
            val result = runCatching {
                withContext(Dispatchers.IO) {
                    HolderNative.createCard(project.projectId, titleFromFirstLine(text).ifBlank { "Untitled" }, text)
                }
            }
            saving = false
            result.onSuccess { card ->
                draft = ""
                focusManager.clearFocus()
                keyboard?.hide()
                highlightedCardId = card.cardId
                reloadKey++
                val choice = snackbar.showSnackbar(
                    message = if (multipleProjects) "Saved to ${project.name}" else "Card saved",
                    actionLabel = "Undo",
                    // Long: Undo is the only way back from an accidental save here.
                    duration = SnackbarDuration.Long,
                )
                if (choice == SnackbarResult.ActionPerformed) {
                    val undone = runCatching {
                        withContext(Dispatchers.IO) { HolderNative.deleteCard(project.projectId, card.cardId) }
                    }
                    if (undone.isSuccess) {
                        if (draft.isNullOrEmpty()) draft = text
                        reloadKey++
                    } else {
                        snackbar.showSnackbar("Couldn't undo: ${undone.exceptionOrNull()?.message.orEmpty()}")
                    }
                }
            }.onFailure {
                snackbar.showSnackbar("Couldn't save card: ${it.message ?: it::class.java.simpleName}")
            }
        }
    }

    Scaffold(
        topBar = {
            // Status-bar padding sits outside the collapsing part, so the editor never slides
            // under the status bar once the app bar has collapsed (landscape, keyboard up).
            Box(Modifier.windowInsetsPadding(TopAppBarDefaults.windowInsets.only(WindowInsetsSides.Top))) {
                Box(Modifier.collapseVertically(if (landscape) visibleFraction else 1f)) {
                    TopAppBar(
                        windowInsets = TopAppBarDefaults.windowInsets.only(WindowInsetsSides.Horizontal),
                        title = { Text("Holder") },
                        actions = {
                            IconButton(onClick = onSearchClick) {
                                Icon(Icons.Filled.Search, contentDescription = "Search")
                            }
                            IconButton(onClick = onProjectsClick) {
                                Icon(Icons.AutoMirrored.Filled.List, contentDescription = "Projects")
                            }
                        },
                    )
                }
            }
        },
        snackbarHost = { SnackbarHost(snackbar) },
    ) { padding ->
        BoxWithConstraints(
            modifier = Modifier
                .padding(padding)
                // Without this, imePadding re-adds the nav-bar inset Scaffold already applied,
                // leaving a gap above the keyboard.
                .consumeWindowInsets(padding)
                .imePadding()
                .fillMaxSize(),
        ) {
            val recentCap = maxHeight * (if (landscape) 0.3f else 0.5f) * visibleFraction
            // Expanded in landscape, a fixed cap left the editor only a sliver (seen on a Pixel 6
            // emulator with 3-button navigation), so there it's a quarter of the height instead.
            val stripCap = if (landscape) minOf(STRIP_MAX_HEIGHT, maxHeight * 0.25f) else STRIP_MAX_HEIGHT
            Column(Modifier.fillMaxSize()) {
                val milestones = data?.milestones.orEmpty()
                if (milestones.isNotEmpty()) {
                    Box(Modifier.collapseVertically(visibleFraction)) {
                        MilestoneStrip(
                            milestones = milestones,
                            compact = landscape,
                            showProject = multipleProjects,
                            hiddenByKeyboard = imeProgress >= 1f,
                            maxHeight = stripCap,
                            onOpen = { onOpenCard(it.projectId, it.cardId, it.cardTitle) },
                        )
                    }
                }

                QuickEntry(
                    draft = draft.orEmpty(),
                    onDraftChange = { draft = it },
                    onSave = ::save,
                    saving = saving,
                    showDraftHint = imeProgress == 0f && !draft.isNullOrBlank(),
                    selectedProject = selectedProject,
                    projects = projects,
                    showProjectPicker = multipleProjects,
                    onProjectChange = { project ->
                        draftProjectId = project.projectId
                        scope.launch { HolderSettings.setStartDraftProjectId(context, project.projectId) }
                    },
                    modifier = Modifier
                        .fillMaxWidth()
                        .weight(1f)
                        .padding(horizontal = 12.dp, vertical = 8.dp),
                )

                Column(
                    Modifier
                        .fillMaxWidth()
                        // A cap, not a fixed height: with only a few recent cards the section
                        // shrinks to fit them and the editor (weight 1f) takes the spare space.
                        .heightIn(max = recentCap)
                        .graphicsLayer { alpha = visibleFraction }
                        .clipToBounds(),
                ) {
                    Text(
                        "Recent",
                        style = MaterialTheme.typography.titleSmall,
                        color = MaterialTheme.colorScheme.onSurfaceVariant,
                        modifier = Modifier.padding(start = 16.dp, top = 4.dp, bottom = 4.dp),
                    )
                    val recent = data?.recentCards
                    when {
                        loadError != null && recent == null -> RecentMessage("Couldn't load recent cards: $loadError")
                        recent == null -> Unit
                        recent.isEmpty() -> RecentMessage("Cards you write or edit will show up here.")
                        else -> LazyColumn(state = listState, modifier = Modifier.fillMaxWidth()) {
                            items(recent, key = { it.projectId + "/" + it.cardId }) { card ->
                                Column(Modifier.animateItem()) {
                                    RecentCardRow(
                                        card = card,
                                        showProject = multipleProjects,
                                        highlighted = card.cardId == highlightedCardId,
                                        onHighlightDone = { if (highlightedCardId == card.cardId) highlightedCardId = null },
                                        onClick = { onOpenCard(card.projectId, card.cardId, card.title) },
                                    )
                                    HorizontalDivider()
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun RecentMessage(text: String) {
    Text(
        text,
        style = MaterialTheme.typography.bodyMedium,
        color = MaterialTheme.colorScheme.onSurfaceVariant,
        modifier = Modifier.padding(horizontal = 16.dp, vertical = 12.dp),
    )
}

@Composable
private fun MilestoneStrip(
    milestones: List<StartMilestone>,
    compact: Boolean,
    showProject: Boolean,
    hiddenByKeyboard: Boolean,
    maxHeight: Dp,
    onOpen: (StartMilestone) -> Unit,
) {
    // In landscape there's little height, so only the most urgent one shows, plus "+N more".
    var expanded by remember { mutableStateOf(false) }
    // Comes back collapsed after typing.
    LaunchedEffect(hiddenByKeyboard) { if (hiddenByKeyboard) expanded = false }
    val shown = if (compact && !expanded) milestones.take(1) else milestones
    val hidden = milestones.size - shown.size
    // Collapsed, the whole strip is one tap target that expands it; individual milestones only
    // open their card once they're all visible.
    val collapsed = hidden > 0
    Surface(
        color = MaterialTheme.colorScheme.secondaryContainer,
        modifier = Modifier
            .fillMaxWidth()
            // Capped so it never crowds out the editor; scrolls if there are many.
            .heightIn(max = maxHeight)
            .then(if (collapsed) Modifier.clickable(onClickLabel = "Show all milestones") { expanded = true } else Modifier),
    ) {
        Column(
            modifier = Modifier
                .animateContentSize(tween(200))
                .verticalScroll(rememberScrollState())
                .padding(horizontal = 12.dp, vertical = 6.dp),
            verticalArrangement = Arrangement.spacedBy(2.dp),
        ) {
            shown.forEachIndexed { index, m ->
                val accent = if (m.overdue) MaterialTheme.colorScheme.error else MaterialTheme.colorScheme.onSecondaryContainer
                Row(
                    verticalAlignment = Alignment.CenterVertically,
                    modifier = Modifier
                        .fillMaxWidth()
                        .then(if (collapsed) Modifier else Modifier.clickable { onOpen(m) })
                        .padding(vertical = 4.dp),
                ) {
                    Icon(
                        if (m.overdue) Icons.Filled.Warning else Icons.Filled.DateRange,
                        contentDescription = if (m.overdue) "Overdue" else null,
                        tint = accent,
                        modifier = Modifier.size(18.dp),
                    )
                    Spacer(Modifier.width(8.dp))
                    Text(
                        m.label,
                        style = MaterialTheme.typography.bodyMedium,
                        fontWeight = FontWeight.Medium,
                        maxLines = 1,
                        overflow = TextOverflow.Ellipsis,
                        modifier = Modifier.weight(1f),
                    )
                    Text(
                        if (showProject) "${m.dueLabel} · ${m.projectName}" else m.dueLabel,
                        style = MaterialTheme.typography.labelMedium,
                        color = accent,
                        modifier = Modifier.padding(start = 8.dp),
                    )
                    if (index == 0 && collapsed) {
                        Text(
                            "+$hidden more",
                            style = MaterialTheme.typography.labelLarge,
                            color = MaterialTheme.colorScheme.primary,
                            modifier = Modifier.padding(start = 16.dp),
                        )
                    } else if (index == 0 && compact && expanded) {
                        Text(
                            "Less",
                            style = MaterialTheme.typography.labelLarge,
                            color = MaterialTheme.colorScheme.primary,
                            modifier = Modifier
                                .padding(start = 8.dp)
                                .clip(MaterialTheme.shapes.small)
                                .clickable { expanded = false }
                                .padding(horizontal = 8.dp, vertical = 2.dp),
                        )
                    }
                }
            }
        }
    }
}

@Composable
private fun QuickEntry(
    draft: String,
    onDraftChange: (String) -> Unit,
    onSave: () -> Unit,
    saving: Boolean,
    showDraftHint: Boolean,
    selectedProject: HolderProject?,
    projects: List<HolderProject>,
    showProjectPicker: Boolean,
    onProjectChange: (HolderProject) -> Unit,
    modifier: Modifier = Modifier,
) {
    OutlinedCard(modifier = modifier) {
        Column(Modifier.fillMaxSize()) {
            TextField(
                value = draft,
                onValueChange = onDraftChange,
                placeholder = { Text("What are you thinking about?") },
                keyboardOptions = KeyboardOptions(capitalization = KeyboardCapitalization.Sentences),
                colors = TextFieldDefaults.colors(
                    focusedContainerColor = Color.Transparent,
                    unfocusedContainerColor = Color.Transparent,
                    focusedIndicatorColor = Color.Transparent,
                    unfocusedIndicatorColor = Color.Transparent,
                ),
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
            )
            Row(
                verticalAlignment = Alignment.CenterVertically,
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(horizontal = 8.dp, vertical = 4.dp),
            ) {
                if (showProjectPicker && selectedProject != null) {
                    var open by remember { mutableStateOf(false) }
                    Box {
                        AssistChip(
                            onClick = { open = true },
                            label = { Text(selectedProject.name, maxLines = 1, overflow = TextOverflow.Ellipsis) },
                            trailingIcon = { Icon(Icons.Filled.ArrowDropDown, contentDescription = null) },
                        )
                        DropdownMenu(expanded = open, onDismissRequest = { open = false }) {
                            // Home first, then the rest in the order listProjects gives them.
                            val ordered = projects.sortedByDescending { it.name == HolderNative.DEFAULT_PROJECT_NAME }
                            ordered.forEach { project ->
                                DropdownMenuItem(
                                    text = { Text(project.name) },
                                    onClick = {
                                        onProjectChange(project)
                                        open = false
                                    },
                                )
                            }
                        }
                    }
                }
                if (showDraftHint) {
                    Text(
                        "Unsaved draft",
                        style = MaterialTheme.typography.labelMedium,
                        color = MaterialTheme.colorScheme.tertiary,
                        modifier = Modifier.padding(start = 8.dp),
                    )
                }
                Spacer(Modifier.weight(1f))
                TextButton(onClick = onSave, enabled = draft.isNotBlank() && !saving && selectedProject != null) {
                    Icon(Icons.Filled.Check, contentDescription = null, modifier = Modifier.size(18.dp))
                    Spacer(Modifier.width(4.dp))
                    Text("Save card")
                }
            }
        }
    }
}

@Composable
private fun RecentCardRow(
    card: StartRecentCard,
    showProject: Boolean,
    highlighted: Boolean,
    onHighlightDone: () -> Unit,
    onClick: () -> Unit,
) {
    // A just-saved card flashes in primaryContainer, then fades back to normal.
    val flash = remember { Animatable(0f) }
    LaunchedEffect(highlighted) {
        if (highlighted) {
            flash.snapTo(1f)
            delay(600)
            flash.animateTo(0f, tween(1400))
            onHighlightDone()
        }
    }
    val now = System.currentTimeMillis()
    val edited = if (now - card.updatedAt * 1000 < DateUtils.MINUTE_IN_MILLIS) {
        "Just now"
    } else {
        DateUtils.getRelativeTimeSpanString(
            card.updatedAt * 1000,
            now,
            DateUtils.MINUTE_IN_MILLIS,
            DateUtils.FORMAT_ABBREV_RELATIVE,
        ).toString()
    }
    ListItem(
        headlineContent = { Text(card.title, maxLines = 1, overflow = TextOverflow.Ellipsis) },
        supportingContent = card.snippet.takeIf { it.isNotEmpty() }?.let { snippet ->
            { Text(snippet, maxLines = 1, overflow = TextOverflow.Ellipsis) }
        },
        trailingContent = {
            Column(horizontalAlignment = Alignment.End) {
                Text(edited, style = MaterialTheme.typography.labelSmall)
                if (showProject) {
                    Text(
                        card.projectName,
                        style = MaterialTheme.typography.labelSmall,
                        color = MaterialTheme.colorScheme.primary,
                    )
                }
            }
        },
        colors = ListItemDefaults.colors(
            // Transparent at rest so a themed background (Walnut) shows through, as elsewhere.
            containerColor = lerp(
                Color.Transparent,
                MaterialTheme.colorScheme.primaryContainer,
                flash.value,
            ),
        ),
        modifier = Modifier.clickable(onClick = onClick),
    )
}

/**
 * Shows [fraction] (0..1) of the content's height, sliding it up under whatever is above it and
 * fading it as it goes. Driven per frame by the keyboard's progress, not by its own timer.
 */
private fun Modifier.collapseVertically(fraction: Float): Modifier = this
    .clipToBounds()
    .graphicsLayer { alpha = fraction }
    .layout { measurable, constraints ->
        val placeable = measurable.measure(constraints.copy(minHeight = 0))
        val height = (placeable.height * fraction).roundToInt()
        layout(placeable.width, height) { placeable.place(0, height - placeable.height) }
    }
