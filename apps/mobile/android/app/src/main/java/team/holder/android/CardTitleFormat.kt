package team.holder.android

private val ATX_HEADING = Regex("^#{1,6}[ \t].*")

// A line that's nothing but an attached resource's own Markdown reference (see
// AssetAttachment.kt's `![label](holder://resource/<id>)`) -- attaching a photo as the very
// first thing typed into a fresh card would otherwise hand that raw syntax back as the "title".
private val RESOURCE_REFERENCE_LINE = Regex("^!\\[[^\\]]*]\\(holder://resource/[^)]*\\)$")

/**
 * Extracts the title implied by a card's body under "first line is the title" mode: the first
 * non-blank, non-resource-reference line, verbatim, with a leading Markdown heading marker
 * stripped if present. Doesn't require valid Markdown -- any such line is accepted as the title
 * as-is.
 */
fun titleFromFirstLine(content: String): String {
    val line = content.lineSequence()
        .map { it.trim() }
        .firstOrNull { it.isNotBlank() && !RESOURCE_REFERENCE_LINE.matches(it) }
        ?: return ""
    return if (ATX_HEADING.matches(line)) line.trimStart('#').trim() else line
}

/**
 * Splits a card body that starts with a `# Title` heading (as written by [combineTitleAndBody])
 * into the remaining body, for "Separate title" editing where that heading line is redundant
 * with a distinct Title field. Returns null if the body doesn't start with a heading -- there's
 * nothing to hide, so the caller should show the body unchanged.
 */
fun splitLeadingHeading(content: String): String? {
    val firstNewline = content.indexOf('\n')
    val firstLine = if (firstNewline == -1) content else content.substring(0, firstNewline)
    if (!ATX_HEADING.matches(firstLine.trim())) return null
    if (firstNewline == -1) return ""
    var rest = content.substring(firstNewline + 1)
    if (rest.startsWith("\n")) rest = rest.substring(1)
    return rest
}

/** A card body with its title line taken off: what "Separate title" editing shows. */
data class TitleSplit(
    val body: String,
    /** Whether the title line was a `# Title` heading (else the plain title text), so saving
     * writes it back the way it was. */
    val asHeading: Boolean,
)

/**
 * Takes the title line off the front of [content] for "Separate title" editing and viewing,
 * where a distinct Title field (or the app bar) already shows it: a leading `# Title` heading,
 * or a first line that is the card's [title] as plain text -- the "first line is the title"
 * convention cards written elsewhere (Desktop, holderctl) follow. Without the second case the
 * title appeared twice, and saving added a heading above it ("# Ideas", then "Ideas").
 * Null when there's no title line to take off.
 */
fun splitTitleLine(content: String, title: String): TitleSplit? {
    splitLeadingHeading(content)?.let { return TitleSplit(it, asHeading = true) }
    if (title.isBlank()) return null
    val firstNewline = content.indexOf('\n')
    val firstLine = if (firstNewline == -1) content else content.substring(0, firstNewline)
    if (firstLine.trim() != title.trim()) return null
    if (firstNewline == -1) return TitleSplit("", asHeading = false)
    var rest = content.substring(firstNewline + 1)
    if (rest.startsWith("\n")) rest = rest.substring(1)
    return TitleSplit(rest, asHeading = false)
}

/**
 * Reconstructs a card body with its title as the first line: a `# Title` heading, the
 * convention every Holder client expects, or, for a card that had its title as a plain first
 * line ([splitTitleLine]'s asHeading false), the plain title again.
 */
fun combineTitleAndBody(title: String, body: String, asHeading: Boolean = true): String {
    val line = if (asHeading) "# $title" else title
    return if (body.isEmpty()) "$line\n\n" else "$line\n\n$body"
}
