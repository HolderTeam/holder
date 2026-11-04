package team.holder.android

/** Debug-only JNI probe for callback attachment from a native-created thread. */
internal object HostAdapterNativeProbe {
    @JvmStatic
    external fun calendarFromNativeThread(contextHandle: Long, projectId: String): Int
}
