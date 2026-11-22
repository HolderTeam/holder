#pragma once

#include "card/CardFrontMatter.h"

#include <optional>
#include <string>

namespace holder::sync {

// Merges two edits of a card's body line by line, as git merges a file: the changes each side
// made to its own lines are both kept. Edits to the same lines, or to lines next to each other,
// cannot be combined safely, so the result is empty. Line endings are kept as they are.
std::optional<std::string> merge_body_lines(
    const std::string& base,
    const std::string& local,
    const std::string& remote
);

// Combines two versions of one card that both changed since their common version, field by
// field: the title, the placement (parent and position together), the body, and each link and
// milestone. A field changed on only one side takes that side's value, and a field both sides
// changed to the same value takes it. A field both sides changed differently cannot be combined,
// so the result is empty and the caller keeps both versions as a conflict copy. A change to
// whether the card is in Trash is never combined. A body both sides changed differently is
// merged line by line (see merge_body_lines). updated_at becomes the later of the two versions.
std::optional<holder::core::ParsedCardFile> merge_card_versions(
    const holder::core::ParsedCardFile& base,
    const holder::core::ParsedCardFile& local,
    const holder::core::ParsedCardFile& remote
);

} // namespace holder::sync
