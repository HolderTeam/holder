#pragma once

#include <stdexcept>
#include <string>

namespace holder::card {

// Why a card cannot be placed where a move or create asked. what() is the code's name (for
// example "move_would_create_cycle"), which the C API passes on unchanged.
enum class CardPlacementErrorCode {
  CardNotFound,
  CrossProjectMoveForbidden,
  TargetNotFound,
  MoveWouldCreateCycle,
  InvalidTarget,
  InvalidMoveIntent,
  AlreadyAtProjectRoot,
  MissingTargetCardId,
};

inline const char* card_placement_error_name(CardPlacementErrorCode code) {
  switch (code) {
  case CardPlacementErrorCode::CardNotFound: return "card_not_found";
  case CardPlacementErrorCode::CrossProjectMoveForbidden: return "cross_project_move_forbidden";
  case CardPlacementErrorCode::TargetNotFound: return "target_not_found";
  case CardPlacementErrorCode::MoveWouldCreateCycle: return "move_would_create_cycle";
  case CardPlacementErrorCode::InvalidTarget: return "invalid_target";
  case CardPlacementErrorCode::InvalidMoveIntent: return "invalid_move_intent";
  case CardPlacementErrorCode::AlreadyAtProjectRoot: return "already_at_project_root";
  case CardPlacementErrorCode::MissingTargetCardId: return "missing_target_card_id";
  }
  return "invalid_move_intent"; // LCOV_EXCL_LINE
}

class CardPlacementError : public std::runtime_error {
 public:
  explicit CardPlacementError(CardPlacementErrorCode code)
      : std::runtime_error(card_placement_error_name(code)), code_(code) {}
  CardPlacementErrorCode code() const { return code_; }

 private:
  CardPlacementErrorCode code_;
};

} // namespace holder::card
