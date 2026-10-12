package team.holder.android

import java.util.concurrent.Callable
import java.util.concurrent.ExecutionException
import java.util.concurrent.Executors

/** One thread that owns one native holder context and runs every call on it in order.
 * holder-core's contexts are not safe to use from two threads at once (see holder.h): a read
 * on another thread would share the connection and see a write's uncommitted rows. So
 * [HolderNative] opens one context per lane, the same way holder-daemon gives each worker its
 * own connection, and each lane's calls run one at a time on that lane's thread.
 *
 * [call] blocks the calling thread (normally Dispatchers.IO) until the call is done, so
 * HolderNative's functions keep their blocking signatures. A call made from the lane's own
 * thread runs inline instead of queueing behind itself. */
internal class NativeLane(name: String) {
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
        if (Thread.currentThread() === thread) return block()
        val future = executor.submit(Callable { block() })
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
}
