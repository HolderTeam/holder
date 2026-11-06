// Generated from contract/interface by compiler. Do not edit; run make -C contract update.
package team.holder.android.contract

import androidx.annotation.Keep

enum class StorageErrorCode {
    UNAVAILABLE,
    AUTHENTICATION,
    PERMISSION,
    CAPACITY,
    INTEGRITY,
    CONFLICT,
    INVALID_CONFIGURATION,
    TRANSIENT,
}

/** The location provider kinds a host implements; local_directory is built into core. */
enum class RegisterStorageProviderKind(val value: String) {
    S3_COMPATIBLE("s3_compatible"),
    GOOGLE_DRIVE("google-drive"),
}

/** A typed storage failure passed through the generated JNI adapter. */
@Keep
class StorageFailure(val code: Int, override val message: String) : RuntimeException(message)

/** A named host backend for opaque asset objects. */
@Keep
interface StorageProvider {
    @Keep
    fun put(objectKey: String, stagedFilePath: String, storedSize: Long, storedSha256: String)
    @Keep
    fun get(objectKey: String, destinationFilePath: String)
    @Keep
    fun exists(objectKey: String): Boolean
    @Keep
    fun remove(objectKey: String)
}

object StorageHosts {
    @JvmStatic
    external fun registerStorageProvider(kind: RegisterStorageProviderKind, provider: StorageProvider)
}
