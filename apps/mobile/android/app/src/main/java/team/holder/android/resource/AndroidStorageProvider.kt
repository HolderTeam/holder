package team.holder.android.resource

typealias StorageErrorCode = team.holder.android.contract.StorageErrorCode

/** Encodes a failure for [AndroidStorageProvider]'s put/get/remove return convention. */
fun StorageErrorCode.encode(message: String): String = "${ordinal}:$message"

/**
 * Implemented by a Kotlin storage backend (e.g. [team.holder.android.resource.drive.GoogleDriveStorageProvider])
 * registered with holder-core's process-wide storage provider registry via
 * [AndroidStorageProviderBridge.register] through the generated host adapter. Mirrors
 * [team.holder.android.HolderNative]'s object_key-addressed model exactly: an object_key is
 * an opaque, already-unique string (see holder-core's `object_key_for`), never something a
 * provider chooses or interprets itself.
 *
 * put/get/remove return `null` on success, or a [StorageErrorCode]-[encode]d string on
 * failure. The registration adapter converts that string into a generated [team.holder.android.contract.StorageFailure]
 * so JNI can report a typed failure back across the C ABI. Any *other* exception is
 * still treated as a failure (a generic, unspecific one), so implementations
 * are free to let an unexpected exception propagate rather than catching everything --
 * [encode] is only needed to report a specific, actionable code.
 *
 * [exists] is the one exception to that convention: a genuine inability to determine
 * whether an object exists is rare and not worth a typed code, so it simply throws; the
 * adapter reports that as a generic failure. Returning `false` because the object legitimately
 * isn't there is not a failure at all -- see
 * `StorageProvider::exists` in holder-core.
 */
interface AndroidStorageProvider {
    fun put(objectKey: String, stagedFilePath: String, storedSize: Long, storedSha256: String): String?
    fun get(objectKey: String, destinationFilePath: String): String?
    fun exists(objectKey: String): Boolean
    fun remove(objectKey: String): String?
}
