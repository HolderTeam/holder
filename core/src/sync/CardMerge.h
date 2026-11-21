#pragma once

#include "card/CardFrontMatter.h"

#include <optional>

namespace holder::sync {

// Combines two versions of one card that both changed since their common version, field by
// field: the title, the placement (parent and position together), the body, and each link and
// milestone. A field changed on only one side takes that side's value, and a field both sides
// changed to the same value takes it. A field both sides changed differently cannot be combined,
// so the result is empty and the caller keeps both versions as a conflict copy. A change to
// whether the card is in Trash is never combined. The body is compared as a whole; there is no
// line-level merge. updated_at becomes the later of the two versions.
std::optional<holder::core::ParsedCardFile> merge_card_versions(
    const holder::core::ParsedCardFile& base,
    const holder::core::ParsedCardFile& local,
    const holder::core::ParsedCardFile& remote
);

} // namespace holder::sync
