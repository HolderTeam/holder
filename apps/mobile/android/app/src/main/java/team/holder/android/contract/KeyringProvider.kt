// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

import androidx.annotation.Keep

/** A process-wide secure store for device secrets and project encryption keys. */
@Keep
interface KeyringProvider {
    /** A null result means the secret is absent; an exception means lookup failed. */
    @Keep
    fun lookup(kind: Int, service: String, account: String, projectId: String?): String?

    @Keep
    fun store(kind: Int, service: String, account: String, projectId: String?, label: String, secret: String)

    @Keep
    fun remove(kind: Int, service: String, account: String, projectId: String?)
}

object KeyringHosts {
    @JvmStatic
    external fun registerKeyringProvider(provider: KeyringProvider)
}
