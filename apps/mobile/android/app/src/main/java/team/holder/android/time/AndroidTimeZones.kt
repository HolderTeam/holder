package team.holder.android.time

import team.holder.android.contract.HostServices
import team.holder.android.contract.TimeZoneProvider
import java.time.DateTimeException
import java.time.Instant
import java.time.ZoneId

/**
 * Core's time-zone provider on Android. Android's
 * C++ library has no time-zone database, so core asks this for a zone's UTC offset at an instant and
 * for the device's zone, answered from java.time with Android's own, OS-updated zone data. Core
 * turns days into instants itself, so the calendar behaves the same as on every other platform.
 */
object AndroidTimeZones : TimeZoneProvider {

    /** Installs this as core's provider for the process. Returns a HOLDER_* status code. */
    fun register() = HostServices.registerTimeZoneProvider(this)

    override fun utcOffset(zone: String, utcSeconds: Long): Long? = try {
        ZoneId.of(zone).rules.getOffset(Instant.ofEpochSecond(utcSeconds)).totalSeconds.toLong()
    } catch (_: DateTimeException) {
        null
    }

    override fun currentZone(): String = ZoneId.systemDefault().id
}
