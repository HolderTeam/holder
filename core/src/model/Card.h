#pragma once

#include "api/Contract.h"

namespace holder::model {

enum class CardScope {
  Live,
  Trashed,
  Either,
};

// A card's metadata, as stored: the contract's CardMetadata (contract/interface/cards.py), so the
// record core stores and the record the contract describes are one type.
using Card = holder::contract::CardMetadata;

} // namespace holder::model
