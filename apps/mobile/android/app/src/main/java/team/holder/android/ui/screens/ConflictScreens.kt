package team.holder.android.ui.screens

import android.widget.Toast
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.itemsIndexed
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedButton
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SegmentedButton
import androidx.compose.material3.SegmentedButtonDefaults
import androidx.compose.material3.SingleChoiceSegmentedButtonRow
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import team.holder.android.ConflictKeepOutcome
import team.holder.android.HolderConflictCopy
import team.holder.android.HolderNative
import team.holder.android.HolderProject
import team.holder.android.contract.ConflictComparison
import team.holder.android.contract.ConflictComparisonCopyEditedOn
import team.holder.android.contract.ConflictComparisonOriginalEditedOn
import team.holder.android.ui.CenteredMessage
import team.holder.android.ui.LoadState

/** One version of a card that changed on two devices, as the compare screen shows it. */
data class ConflictSide(
    val label: String,
    val title: String,
    val content: String,
    /** Zero-based indexes of the lines in [content] that differ from the other version. */
    val changedLines: Set<Int>,
)

/**
 * The two versions of a conflict copy pair, the card's own version first. Labelled by the
 * device that last changed each one when core knows it ("This device", "Other device"), and
 * otherwise as the card and its copy; when both come out the same, each says which it is.
 */
fun conflictSides(
    comparison: ConflictComparison,
): Pair<ConflictSide, ConflictSide> {
    fun deviceLabel(thisDevice: Boolean, anotherDevice: Boolean, fallback: String) = when {
        thisDevice -> "This device"
        anotherDevice -> "Other device"
        else -> fallback
    }
    var originalLabel = deviceLabel(
        comparison.originalEditedOn == ConflictComparisonOriginalEditedOn.THIS_DEVICE,
        comparison.originalEditedOn == ConflictComparisonOriginalEditedOn.ANOTHER_DEVICE,
        "The card",
    )
    var copyLabel = deviceLabel(
        comparison.copyEditedOn == ConflictComparisonCopyEditedOn.THIS_DEVICE,
        comparison.copyEditedOn == ConflictComparisonCopyEditedOn.ANOTHER_DEVICE,
        "The copy",
    )
    if (originalLabel == copyLabel) {
        originalLabel = "$originalLabel (the card)"
        copyLabel = "$copyLabel (the copy)"
    }
    return ConflictSide(
        label = originalLabel,
        title = comparison.original.title,
        content = comparison.original.content,
        changedLines = comparison.originalChangedLines.map { it.line - 1 }.toSet(),
    ) to ConflictSide(
        label = copyLabel,
        // The copy's own title, without the "(conflicted copy)" Holder added to tell them apart:
        // that's the title it had on its device, and what keeping it should give the card.
        title = separatedCopyTitle(comparison.copyCard.title),
        content = comparison.copyCard.content,
        changedLines = comparison.copyChangedLines.map { it.line - 1 }.toSet(),
    )
}

/** The title a kept-both copy takes: its own, without the "(conflicted copy)" Holder added. */
fun separatedCopyTitle(copyTitle: String): String = team.holder.android.conflictCopyBaseTitle(copyTitle)

/** Every card waiting for review because it changed on two devices, across projects. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ConflictListScreen(
    refreshKey: Any,
    onOpen: (projectId: String, copyId: String) -> Unit,
    onBack: () -> Unit,
) {
    var state by remember { mutableStateOf<LoadState<List<Pair<HolderProject, HolderConflictCopy>>>>(LoadState.Loading) }
    LaunchedEffect(refreshKey) {
        state = runCatching { withContext(Dispatchers.IO) { HolderNative.listAllConflictCopies() } }
            .fold({ LoadState.Success(it) }, { LoadState.Error(it.message ?: it::class.java.simpleName) })
    }
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Changed on two devices") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
            )
        },
    ) { innerPadding ->
        when (val s = state) {
            is LoadState.Loading -> CenteredMessage(Modifier.padding(innerPadding)) { CircularProgressIndicator() }
            is LoadState.Error -> CenteredMessage(Modifier.padding(innerPadding)) { Text("Couldn't load: ${s.message}") }
            is LoadState.Success -> if (s.value.isEmpty()) {
                CenteredMessage(Modifier.padding(innerPadding)) { Text("Nothing to review.") }
            } else {
                LazyColumn(Modifier.fillMaxSize().padding(innerPadding)) {
                    item {
                        Text(
                            "Each of these cards was changed on two devices before they synced. " +
                                "Holder kept both versions; choose what to keep.",
                            style = MaterialTheme.typography.bodyMedium,
                            modifier = Modifier.padding(16.dp),
                        )
                    }
                    itemsIndexed(s.value, key = { _, (_, c) -> c.projectId + "/" + c.copyCardId }) { _, (project, copy) ->
                        ListItem(
                            headlineContent = { Text(copy.originalTitle) },
                            supportingContent = { Text(project.name) },
                            modifier = Modifier.clickable { onOpen(copy.projectId, copy.copyCardId) },
                        )
                        HorizontalDivider()
                    }
                }
            }
        }
    }
}

/** What the compare screen is asking the user to confirm. */
private sealed interface ConflictAction {
    data class Keep(val side: ConflictSide) : ConflictAction
    data object KeepBoth : ConflictAction
    data class Edit(val title: String, val content: String) : ConflictAction
}

/**
 * Compares the two versions of a card that changed on two devices and settles it: keep one,
 * keep both as separate cards, or edit a combined version (issue: Android conflict review).
 * [onDone] runs once the pair is settled.
 */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun ConflictCompareScreen(
    projectId: String,
    copyId: String,
    onDone: () -> Unit,
    onBack: () -> Unit,
) {
    val context = LocalContext.current
    val scope = rememberCoroutineScope()
    var state by remember { mutableStateOf<LoadState<ConflictComparison>>(LoadState.Loading) }
    var reloadKey by remember { mutableIntStateOf(0) }
    var selected by rememberSaveable { mutableIntStateOf(0) }
    var pending by remember { mutableStateOf<ConflictAction?>(null) }
    var working by remember { mutableStateOf(false) }
    var error by remember { mutableStateOf<String?>(null) }

    LaunchedEffect(projectId, copyId, reloadKey) {
        state = runCatching { withContext(Dispatchers.IO) { HolderNative.compareConflictCopy(projectId, copyId) } }
            .fold({ LoadState.Success(it) }, { LoadState.Error(it.message ?: it::class.java.simpleName) })
    }

    fun run(action: ConflictAction, comparison: ConflictComparison) {
        working = true
        error = null
        scope.launch(Dispatchers.Main.immediate) {
            val outcome = runCatching {
                withContext(Dispatchers.IO) {
                    when (action) {
                        is ConflictAction.Keep -> HolderNative.keepConflictResult(
                            projectId, copyId, comparison.reviewToken, action.side.title, action.side.content,
                        )
                        is ConflictAction.Edit -> HolderNative.keepConflictResult(
                            projectId, copyId, comparison.reviewToken, action.title, action.content,
                        )
                        ConflictAction.KeepBoth -> {
                            HolderNative.keepBothConflictVersions(
                                projectId, copyId, comparison.reviewToken, separatedCopyTitle(comparison.copyCard.title),
                            )
                            ConflictKeepOutcome.COPY_KEPT_SEPARATELY
                        }
                    }
                }
            }
            working = false
            outcome.onSuccess { result ->
                HolderNative.notifyProjectDataChanged(projectId)
                val message = when {
                    action == ConflictAction.KeepBoth -> "Kept both versions as separate cards."
                    result == ConflictKeepOutcome.COPY_IN_TRASH -> "Conflict resolved. The other version is in Trash."
                    result == ConflictKeepOutcome.COPY_KEPT_SEPARATELY ->
                        "Saved. The other version couldn't go to Trash without moving this card, so it's kept as a separate card."
                    else -> "Saved. The other version is still there to review."
                }
                Toast.makeText(context, message, Toast.LENGTH_SHORT).show()
                onDone()
            }.onFailure {
                // Most likely the pair changed since it was loaded (a stale review): show it again.
                error = "Couldn't save: ${it.message ?: it::class.java.simpleName}"
                reloadKey++
            }
        }
    }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("Changed on two devices") },
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
            )
        },
        bottomBar = {
            val comparison = (state as? LoadState.Success)?.value
            if (comparison != null) {
                val side = conflictSides(comparison).toList()[selected]
                Surface(tonalElevation = 3.dp) {
                    Column(Modifier.fillMaxWidth().navigationBarsPadding().padding(12.dp)) {
                        error?.let { Text(it, color = MaterialTheme.colorScheme.error, modifier = Modifier.padding(bottom = 8.dp)) }
                        Button(
                            onClick = { pending = ConflictAction.Keep(side) },
                            enabled = !working,
                            modifier = Modifier.fillMaxWidth(),
                        ) { Text("Keep this version") }
                        Row(horizontalArrangement = Arrangement.spacedBy(8.dp), modifier = Modifier.padding(top = 4.dp)) {
                            OutlinedButton(onClick = { pending = ConflictAction.KeepBoth }, enabled = !working, modifier = Modifier.weight(1f)) {
                                Text("Keep both")
                            }
                            OutlinedButton(
                                onClick = { pending = ConflictAction.Edit(side.title, side.content) },
                                enabled = !working,
                                modifier = Modifier.weight(1f),
                            ) { Text("Edit…") }
                        }
                    }
                }
            }
        },
    ) { innerPadding ->
        when (val s = state) {
            is LoadState.Loading -> CenteredMessage(Modifier.padding(innerPadding)) { CircularProgressIndicator() }
            is LoadState.Error -> CenteredMessage(Modifier.padding(innerPadding).padding(24.dp)) {
                Text("This card can't be compared now; it may already have been settled on another device.\n\n${s.message}")
            }
            is LoadState.Success -> {
                val sides = conflictSides(s.value).toList()
                val side = sides[selected]
                Column(Modifier.fillMaxSize().padding(innerPadding)) {
                    SingleChoiceSegmentedButtonRow(Modifier.fillMaxWidth().padding(horizontal = 16.dp, vertical = 8.dp)) {
                        sides.forEachIndexed { index, it ->
                            SegmentedButton(
                                selected = selected == index,
                                onClick = { selected = index },
                                shape = SegmentedButtonDefaults.itemShape(index, sides.size),
                            ) { Text(it.label) }
                        }
                    }
                    val titlesDiffer = sides[0].title != sides[1].title
                    Text(
                        side.title,
                        style = MaterialTheme.typography.titleLarge,
                        // The highlight's own text colour: the screen's default is light in the
                        // dark theme, and unreadable on the light highlight.
                        color = if (titlesDiffer) MaterialTheme.colorScheme.onTertiaryContainer else Color.Unspecified,
                        modifier = Modifier
                            .fillMaxWidth()
                            .then(if (titlesDiffer) Modifier.background(MaterialTheme.colorScheme.tertiaryContainer) else Modifier)
                            .padding(horizontal = 16.dp, vertical = 8.dp),
                    )
                    if (s.value.truncated) {
                        Text(
                            "These versions differ too much to highlight every change.",
                            style = MaterialTheme.typography.bodySmall,
                            modifier = Modifier.padding(horizontal = 16.dp),
                        )
                    }
                    val lines = side.content.split("\n")
                    LazyColumn(Modifier.fillMaxWidth().weight(1f)) {
                        itemsIndexed(lines) { index, line ->
                            val changed = index in side.changedLines
                            Text(
                                line.ifEmpty { " " },
                                fontFamily = FontFamily.Monospace,
                                style = MaterialTheme.typography.bodyMedium,
                                color = if (changed) MaterialTheme.colorScheme.onTertiaryContainer else Color.Unspecified,
                                modifier = Modifier
                                    .fillMaxWidth()
                                    .heightIn(min = 20.dp)
                                    .then(if (changed) Modifier.background(MaterialTheme.colorScheme.tertiaryContainer) else Modifier)
                                    .padding(horizontal = 16.dp, vertical = 1.dp),
                            )
                        }
                    }
                }
            }
        }
    }

    val comparison = (state as? LoadState.Success)?.value
    when (val action = pending) {
        null -> Unit
        is ConflictAction.Keep -> AlertDialog(
            onDismissRequest = { pending = null },
            title = { Text(keepQuestion(action.side.label)) },
            text = {
                Text(
                    "The other version moves to Trash, where you can restore it. Anything only it had " +
                        "(links, milestones, tags, child cards) moves to the card you keep.",
                )
            },
            confirmButton = { TextButton(onClick = { pending = null; comparison?.let { run(action, it) } }) { Text("Keep") } },
            dismissButton = { TextButton(onClick = { pending = null }) { Text("Cancel") } },
        )
        ConflictAction.KeepBoth -> AlertDialog(
            onDismissRequest = { pending = null },
            title = { Text("Keep both versions?") },
            text = {
                Text(
                    "Both stay as separate cards. The copy is renamed " +
                        "“${comparison?.let { separatedCopyTitle(it.copyCard.title) }.orEmpty()}”.",
                )
            },
            confirmButton = { TextButton(onClick = { pending = null; comparison?.let { run(action, it) } }) { Text("Keep both") } },
            dismissButton = { TextButton(onClick = { pending = null }) { Text("Cancel") } },
        )
        is ConflictAction.Edit -> {
            var title by remember(action) { mutableStateOf(action.title) }
            var content by remember(action) { mutableStateOf(action.content) }
            AlertDialog(
                onDismissRequest = { pending = null },
                title = { Text("Edit the version to keep") },
                text = {
                    Column {
                        OutlinedTextField(value = title, onValueChange = { title = it }, label = { Text("Title") }, singleLine = true)
                        OutlinedTextField(
                            value = content,
                            onValueChange = { content = it },
                            label = { Text("Text") },
                            modifier = Modifier.padding(top = 8.dp).heightIn(min = 160.dp, max = 360.dp),
                        )
                        Text(
                            "Saving keeps this as the card; the other version moves to Trash.",
                            style = MaterialTheme.typography.bodySmall,
                            modifier = Modifier.padding(top = 8.dp),
                        )
                    }
                },
                confirmButton = {
                    TextButton(onClick = {
                        pending = null
                        comparison?.let { run(ConflictAction.Edit(title, content), it) }
                    }) { Text("Save") }
                },
                dismissButton = { TextButton(onClick = { pending = null }) { Text("Cancel") } },
            )
        }
    }
}

/** "Keep this device's version?", "Keep the other device's version?", or plainly "Keep this version?". */
fun keepQuestion(label: String): String = when {
    label == "This device" -> "Keep this device's version?"
    label == "Other device" -> "Keep the other device's version?"
    else -> "Keep this version?"
}
