// Generated from contract/interface by holder_contract. Do not edit; run make -C contract update.
package team.holder.android.contract

/** One page of a listing. */
data class Page<T>(
    /** The page's items. */
    val items: List<T>,
    /** Pass back to get the next page; null on the last page. */
    val nextCursor: String? = null,
)
