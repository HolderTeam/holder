package team.holder.android

import androidx.activity.ComponentActivity
import androidx.compose.ui.test.junit4.AndroidComposeTestRule
import androidx.compose.ui.test.junit4.ComposeContentTestRule
import androidx.compose.ui.test.junit4.createAndroidComposeRule
import androidx.compose.ui.test.junit4.createComposeRule
import androidx.test.ext.junit.rules.ActivityScenarioRule
import kotlinx.coroutines.test.StandardTestDispatcher

/*
 * Compose test rules whose coroutines resume on the main thread, as the app's do.
 *
 * By default the rules run composition coroutines (LaunchedEffect, rememberCoroutineScope) on
 * an unconfined test dispatcher: after `withContext(Dispatchers.IO) { … }` the rest of the
 * coroutine carries on on the IO thread that finished the work. A state write there can make
 * Compose recompose on that thread, which crashes the first time it composes a LazyColumn
 * ("The current thread must have a looper!", from its prefetch scheduler), and a Toast there
 * fails on Android 12 and older. Whether it happens depends on timing, so it showed as a flake
 * (WholeAppSmokeTest on #548's CI). A standard test dispatcher queues those continuations for
 * the test clock instead, which runs them on the main thread.
 */

fun holderComposeRule(): ComposeContentTestRule = createComposeRule(StandardTestDispatcher())

inline fun <reified A : ComponentActivity> holderAndroidComposeRule(): AndroidComposeTestRule<ActivityScenarioRule<A>, A> =
    createAndroidComposeRule<A>(StandardTestDispatcher())
