// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

import androidx.annotation.Keep

/** The device's time-zone rules, used where core has no native zone database. */
@Keep
interface TimeZoneProvider {
    /** The zone's seconds east of UTC at an instant, or null when the zone is unknown. */
    @Keep
    fun utcOffset(zone: String, utcSeconds: Long): Long?
    /** The device's current time-zone identifier, such as Europe/London. */
    @Keep
    fun currentZone(): String
}

/** Installs a process-wide host provider. Core releases the previous provider after active calls finish. */
object HostServices {
    @JvmStatic
    external fun registerTimeZoneProvider(provider: TimeZoneProvider)
}
