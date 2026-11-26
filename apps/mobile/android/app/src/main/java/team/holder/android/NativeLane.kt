package team.holder.android

import android.os.Looper
import android.util.Log
import java.util.concurrent.Callable
import java.util.concurrent.ExecutionException
import java.util.concurrent.Executors

/** One thread that owns one native holder context and runs every call on it in order.
 * holder-core's contexts are not safe to use from two threads at once (see holder_api.h): a read
 * on another thread would share the connection and see a write's uncommitted rows. So
 * [HolderNative] opens one context per lane, the same way holder-daemon gives each worker its
 * own connection, and each lane's calls run one at a time on that lane's thread.
 *
 * [call] blocks the calling thread (normally Dispatchers.IO) until the call is done, so
 * HolderNative's functions keep their blocking signatures. A callback on this lane cannot
 * start another call on its busy context. */
internal class NativeLane(private val name: String) {
    @Volatile
    private var thread: Thread? = null

    private val executor = Executors.newSingleThreadExecutor { runnable ->
        Thread(runnable, name).also {
            it.isDaemon = true
            thread = it
        }
    }

    /** Only touched on the lane's own thread, by [attach], [detach] and the calls in between. */
    private var handle = 0L
    private var executing = false

    fun <T> call(block: (Long) -> T): T = onLane {
        check(handle != 0L) { "HolderNative.initialize() must be called first" }
        block(handle)
    }

    /** Hands an opened context to this lane. Runs after any calls already queued here. */
    fun attach(contextHandle: Long) = onLane {
        check(handle == 0L) { "lane already has a context" }
        handle = contextHandle
    }

    /** Takes the context back, once every call queued before this one has finished, and
     * passes it to [close]. A call queued after this fails as not initialized. */
    fun detach(close: (Long) -> Unit) = onLane {
        val current = handle
        handle = 0L
        if (current != 0L) close(current)
    }

    private fun <T> onLane(block: () -> T): T {
        if (Thread.currentThread() === thread) return runOnLane(block)
        reportMainThreadCall()
        val queuedAt = System.nanoTime()
        val caller = if (BuildConfig.DEBUG) callerName() else null
        val future = executor.submit(Callable {
            val startedAt = System.nanoTime()
            try {
                runOnLane(block)
            } finally {
                caller?.let { reportSlowCall(it, queuedAt, startedAt) }
            }
        })
        try {
            return future.get()
        } catch (failure: ExecutionException) {
            throw failure.cause ?: failure
        } catch (interrupted: InterruptedException) {
            // Drop the call if it hasn't started; one already running finishes on the lane.
            future.cancel(false)
            Thread.currentThread().interrupt()
            throw interrupted
        }
    }

    /** Debug builds only: logs (never throws) when a call would block the main thread, with
     * the caller's stack, so a new one shows up in logcat under "HolderNative" (issue #105). */
    private fun reportMainThreadCall() {
        if (!BuildConfig.DEBUG) return
        val onMain = runCatching { Looper.getMainLooper()?.isCurrentThread == true }.getOrDefault(false)
        if (onMain) Log.w("HolderNative", "Native call on the main thread; run it on Dispatchers.IO", Throwable())
    }

    /** Debug builds only: logs a call that took [SLOW_CALL_MS] or more, split into the time it
     * waited for this lane (an earlier call still running here) and the time it ran, under
     * "HolderNative". A slow wait means another call on this lane held it up; a slow run means
     * the call itself, or core waiting on another lane's lock, took the time. */
    private fun reportSlowCall(caller: String, queuedAt: Long, startedAt: Long) {
        val waitedMs = (startedAt - queuedAt) / 1_000_000
        val ranMs = (System.nanoTime() - startedAt) / 1_000_000
        if (waitedMs + ranMs >= SLOW_CALL_MS) {
            Log.w("HolderNative", "Slow native call $caller on $name: waited ${waitedMs}ms, ran ${ranMs}ms")
        }
    }

    /** The HolderNative function that made this call, for [reportSlowCall]. */
    private fun callerName(): String =
        Throwable().stackTrace.firstOrNull { frame ->
            frame.className.startsWith("team.holder.android.HolderNative")
        }?.let { "${it.className.substringAfterLast('.')}.${it.methodName}" } ?: "unknown"

    private fun <T> runOnLane(block: () -> T): T {
        check(!executing) { "NativeLane is already executing a native call" }
        executing = true
        try {
            return block()
        } finally {
            executing = false
        }
    }
}

/** See [NativeLane.reportSlowCall]. */
private const val SLOW_CALL_MS = 500L
