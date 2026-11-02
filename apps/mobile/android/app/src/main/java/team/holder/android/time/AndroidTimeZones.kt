package team.holder.android.time

import androidx.annotation.Keep
import java.time.DateTimeException
import java.time.Instant
import java.time.ZoneId

/**
 * Core's time-zone provider on Android (holder_time_zone_set_provider in holder/holder.h). Android's
 * C++ library has no time-zone database, so core asks this for a zone's UTC offset at an instant and
 * for the device's zone, answered from java.time with Android's own, OS-updated zone data. Core
 * turns days into instants itself, so the calendar behaves the same as on every other platform.
 */
object AndroidTimeZones {
    /** Returned by [offsetSeconds] for a zone java.time does not know. */
    private const val UNKNOWN_ZONE = Long.MIN_VALUE

    /** Installs this as core's provider for the process. Returns a HOLDER_* status code. */
    fun register(): Int = nativeRegisterProvider()

    @Keep
    @JvmStatic
    @Suppress("unused") // invoked via JNI
    fun offsetSeconds(zone: String, utcSeconds: Long): Long = try {
        ZoneId.of(zone).rules.getOffset(Instant.ofEpochSecond(utcSeconds)).totalSeconds.toLong()
    } catch (_: DateTimeException) {
        UNKNOWN_ZONE
    }

    @Keep
    @JvmStatic
    @Suppress("unused") // invoked via JNI
    fun currentZone(): String = ZoneId.systemDefault().id

    @JvmStatic
    private external fun nativeRegisterProvider(): Int
}
