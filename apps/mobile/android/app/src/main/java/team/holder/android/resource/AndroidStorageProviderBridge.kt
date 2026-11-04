package team.holder.android.resource

import team.holder.android.contract.StorageFailure
import team.holder.android.contract.StorageHosts
import team.holder.android.contract.StorageProvider

/**
 * Registers an [AndroidStorageProvider] as holder-core's process-wide storage provider for a
 * given provider name (e.g. "google-drive") -- the storage-seam counterpart to
 * [team.holder.android.keyring.AndroidKeyringStore]'s keyring bridge. Unlike the keyring
 * seam (one process-wide provider, so [AndroidKeyringStore] itself is the callback target),
 * multiple named storage providers can be registered side by side, so the provider instance
 * is passed explicitly here rather than this object being the target itself.
 */
object AndroidStorageProviderBridge {
    /**
     * Registers [provider] for [providerName] for the lifetime of the native process --
     * replacing whatever was previously registered under that name, if anything. Safe to call again to re-register after
     * [team.holder.android.HolderNative] closes and reopens its context, since this seam is
     * process-wide, not scoped to a context.
     */
    fun register(providerName: String, provider: AndroidStorageProvider) {
        val adapter = object : StorageProvider {
            override fun put(objectKey: String, stagedFilePath: String, storedSize: Long, storedSha256: String) {
                provider.put(objectKey, stagedFilePath, storedSize, storedSha256)?.let(::throwFailure)
            }
            override fun get(objectKey: String, destinationFilePath: String) {
                provider.get(objectKey, destinationFilePath)?.let(::throwFailure)
            }
            override fun exists(objectKey: String): Boolean = provider.exists(objectKey)
            override fun remove(objectKey: String) {
                provider.remove(objectKey)?.let(::throwFailure)
            }
        }
        StorageHosts.registerStorageProvider(providerName, adapter)
    }

    private fun throwFailure(encoded: String): Nothing {
        val ordinal = encoded.substringBefore(':').toIntOrNull()
        val code = StorageErrorCode.entries.getOrNull(ordinal ?: -1) ?: StorageErrorCode.UNAVAILABLE
        throw StorageFailure(code.ordinal, encoded.substringAfter(':', encoded))
    }
}
