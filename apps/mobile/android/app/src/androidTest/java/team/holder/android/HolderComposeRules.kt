package team.holder.android

import androidx.activity.ComponentActivity
import androidx.compose.ui.test.junit4.AndroidComposeTestRule
import androidx.compose.ui.test.junit4.ComposeContentTestRule
import androidx.compose.ui.test.junit4.v2.createAndroidComposeRule
import androidx.compose.ui.test.junit4.v2.createComposeRule
import androidx.test.ext.junit.rules.ActivityScenarioRule

/*
 * Compose test rules whose coroutines resume on the main thread, as the app's do: Compose's v2
 * rules, which run composition coroutines (LaunchedEffect, rememberCoroutineScope) on a standard
 * test dispatcher.
 *
 * The v1 rules used an unconfined one: after `withContext(Dispatchers.IO) { … }` the rest of the
 * coroutine carried on on the IO thread that finished the work. A state write there could make
 * Compose recompose on that thread, which crashes the first time it composes a LazyColumn
 * ("The current thread must have a looper!", from its prefetch scheduler), and a Toast there
 * fails on Android 12 and older. Whether it happened depended on timing, so it showed as a flake
 * (WholeAppSmokeTest on #548's CI). Every test uses these, so the choice stays in one place.
 */

fun holderComposeRule(): ComposeContentTestRule = createComposeRule()

inline fun <reified A : ComponentActivity> holderAndroidComposeRule(): AndroidComposeTestRule<ActivityScenarioRule<A>, A> =
    createAndroidComposeRule<A>()
