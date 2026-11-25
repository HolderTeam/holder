package team.holder.android.ui.screens

import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.consumeWindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.imePadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.filled.Clear
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.ListItem
import androidx.compose.material3.ListItemDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TextField
import androidx.compose.material3.TextFieldDefaults
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.focus.FocusRequester
import androidx.compose.ui.focus.focusRequester
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.withContext
import team.holder.android.HolderNative
import team.holder.android.ui.LoadState

private const val SEARCH_DEBOUNCE_MS = 300L
private const val PER_PROJECT_LIMIT = 50

private data class GlobalSearchHit(
    val projectId: String,
    val projectName: String,
    val cardId: String,
    val title: String,
    val snippet: String,
)

/** Search across every project, reached from the start screen. Each project's results keep the
 * order its own search ranks them in; projects are listed in [HolderNative.listProjects] order. */
@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun SearchScreen(
    onOpenCard: (projectId: String, cardId: String, title: String) -> Unit,
    onBack: () -> Unit,
) {
    var query by rememberSaveable { mutableStateOf("") }
    var state by remember { mutableStateOf<LoadState<List<GlobalSearchHit>>?>(null) }
    var multipleProjects by remember { mutableStateOf(false) }
    val focusRequester = remember { FocusRequester() }

    LaunchedEffect(Unit) { focusRequester.requestFocus() }

    LaunchedEffect(query) {
        val trimmed = query.trim()
        if (trimmed.isEmpty()) {
            state = null
            return@LaunchedEffect
        }
        delay(SEARCH_DEBOUNCE_MS)
        state = LoadState.Loading
        state = runCatching {
            withContext(Dispatchers.IO) {
                val projects = HolderNative.listProjects()
                multipleProjects = projects.size > 1
                projects.flatMap { project ->
                    HolderNative.searchCards(project.projectId, trimmed, PER_PROJECT_LIMIT).map {
                        GlobalSearchHit(project.projectId, project.name, it.cardId, it.title, it.snippet)
                    }
                }
            }
        }.fold(
            onSuccess = { LoadState.Success(it) },
            onFailure = { LoadState.Error(it.message ?: it::class.java.simpleName) },
        )
    }

    Scaffold(
        topBar = {
            TopAppBar(
                navigationIcon = {
                    IconButton(onClick = onBack) {
                        Icon(Icons.AutoMirrored.Filled.ArrowBack, contentDescription = "Back")
                    }
                },
                title = {
                    TextField(
                        value = query,
                        onValueChange = { query = it },
                        placeholder = { Text("Search all cards") },
                        singleLine = true,
                        trailingIcon = {
                            if (query.isNotEmpty()) {
                                IconButton(onClick = { query = "" }) {
                                    Icon(Icons.Filled.Clear, contentDescription = "Clear search")
                                }
                            }
                        },
                        colors = TextFieldDefaults.colors(
                            focusedContainerColor = Color.Transparent,
                            unfocusedContainerColor = Color.Transparent,
                            focusedIndicatorColor = Color.Transparent,
                            unfocusedIndicatorColor = Color.Transparent,
                        ),
                        modifier = Modifier
                            .fillMaxWidth()
                            .focusRequester(focusRequester),
                    )
                },
            )
        },
    ) { padding ->
        Column(
            Modifier
                .padding(padding)
                .consumeWindowInsets(padding)
                .imePadding()
                .fillMaxSize(),
        ) {
            when (val current = state) {
                null -> Unit
                LoadState.Loading -> Message("Searching…")
                is LoadState.Error -> Message("Search failed: ${current.message}")
                is LoadState.Success -> if (current.value.isEmpty()) {
                    Message("No cards match.")
                } else {
                    LazyColumn {
                        items(current.value, key = { it.projectId + "/" + it.cardId }) { hit ->
                            ListItem(
                                headlineContent = { Text(hit.title, maxLines = 1, overflow = TextOverflow.Ellipsis) },
                                supportingContent = hit.snippet.takeIf { it.isNotBlank() }?.let { snippet ->
                                    { Text(snippet, maxLines = 2, overflow = TextOverflow.Ellipsis) }
                                },
                                trailingContent = if (multipleProjects) {
                                    {
                                        Text(
                                            hit.projectName,
                                            style = MaterialTheme.typography.labelSmall,
                                            color = MaterialTheme.colorScheme.primary,
                                        )
                                    }
                                } else {
                                    null
                                },
                                colors = ListItemDefaults.colors(containerColor = Color.Transparent),
                                modifier = Modifier.clickable { onOpenCard(hit.projectId, hit.cardId, hit.title) },
                            )
                            HorizontalDivider()
                        }
                    }
                }
            }
        }
    }
}

@Composable
private fun Message(text: String) {
    Text(
        text,
        style = MaterialTheme.typography.bodyMedium,
        color = MaterialTheme.colorScheme.onSurfaceVariant,
        modifier = Modifier.padding(16.dp),
    )
}
