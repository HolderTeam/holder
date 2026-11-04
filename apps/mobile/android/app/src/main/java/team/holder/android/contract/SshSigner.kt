// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

import androidx.annotation.Keep

/** Signs SSH authentication challenges with a host-owned private key. */
@Keep
interface SshSigner {
    /** Returns a DER-encoded P-256 ECDSA signature. */
    @Keep
    fun sign(data: ByteArray): ByteArray
}

object SignerHosts {
    @JvmStatic
    external fun registerSshSigner(contextHandle: Long, username: String, publicKeyBlob: ByteArray, signer: SshSigner)
}
