package team.holder.android.ui.screens

/**
 * Both versions of a conflicted card's text in one, for Combine to start from: the lines they
 * share once, and each clash with [a]'s lines first, then [b]'s. With [markers], each clash sits
 * between git's conflict markers, labelled [aLabel] and [bLabel]. The same as the Desktop's
 * ConflictCombiner (git_sync_outcomes.vala), so both apps start Combine from the same text.
 *
 * [aChanged] and [bChanged] are the zero-based lines core marked as differing
 * ([ConflictSide.changedLines]); every other line is shared, in the same order on both sides.
 */
fun combineConflictVersions(
    a: String,
    aChanged: Set<Int>,
    b: String,
    bChanged: Set<Int>,
    markers: Boolean = false,
    aLabel: String = "",
    bLabel: String = "",
): String {
    val aLines = coreLines(a)
    val bLines = coreLines(b)
    val out = mutableListOf<String>()
    var i = 0
    var j = 0
    while (i < aLines.size || j < bLines.size) {
        val aRun = mutableListOf<String>()
        val bRun = mutableListOf<String>()
        while (i < aLines.size && i in aChanged) aRun += aLines[i++]
        while (j < bLines.size && j in bChanged) bRun += bLines[j++]
        if (aRun.isNotEmpty() || bRun.isNotEmpty()) {
            if (markers) out += "<<<<<<< $aLabel"
            out += aRun
            if (markers) out += "======="
            out += bRun
            if (markers) out += ">>>>>>> $bLabel"
        }
        if (i < aLines.size && j < bLines.size) {
            out += aLines[i++]
            j++
        } else {
            // The shared lines ran out on one side only, which a complete comparison never
            // does. Keep whatever is left rather than lose it.
            while (i < aLines.size) out += aLines[i++]
            while (j < bLines.size) out += bLines[j++]
        }
    }
    return out.joinToString("\n")
}

/** As core's lines_of: split at "\n"; a trailing "\n" leaves an empty last line. */
private fun coreLines(text: String): List<String> = if (text.isEmpty()) emptyList() else text.split("\n")
