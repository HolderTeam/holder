#pragma once

#include "model/CardLink.h"

namespace holder::platform {
class Db;
}

namespace holder::card {

// Whether a link stays listed when deleted items are hidden. A link to a card in another project
// is always listed, since its source owns it even when the target cannot be loaded; a link to a
// missing or trashed local card or AI message is not. Other target types are always listed.
bool link_target_visible(holder::platform::Db& db, const holder::model::CardLink& link);

// Whether a backlink's source, a card or an AI message, exists and is not trashed.
bool backlink_source_visible(holder::platform::Db& db, const holder::model::CardLink& link);

} // namespace holder::card
