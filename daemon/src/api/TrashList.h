#pragma once

#include "api/RigAnnotations.h"
#include "api/Schemas.h"

#include <optional>
#include <string>
#include <vector>

namespace holder::index { class FtsIndexer; }
namespace holder::platform { class Db; }

namespace holder::api {

struct RIG_OPEN_RECORD TrashItem {
  std::string type;
  std::optional<std::optional<std::string>> card_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> message_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> thread_id RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::string> project_id RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> title RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<std::optional<std::string>> role RIG_OMIT_EMPTY RIG_NULLABLE;
  std::optional<long long> deleted_at RIG_OMIT_EMPTY;
  std::optional<long long> created_at RIG_OMIT_EMPTY;
  std::optional<std::optional<std::string>> rel_path RIG_OMIT_EMPTY RIG_NULLABLE;
};

struct RIG_OPEN_RECORD TrashListResponse {
  bool ok RIG_BOOL_VALUE(true);
  std::vector<TrashItem> data;
};

RIG_ROUTE(GET, "/trash")
RIG_SUMMARY("List trashed items by project")
RIG_RESPONSE_DESCRIPTION("OK")
TrashListResponse get_trash(
    holder::platform::Db& db RIG_CONTEXT,
    holder::index::FtsIndexer* fts RIG_CONTEXT,
    std::string project_id RIG_QUERY RIG_MIN_LENGTH(1),
    std::optional<std::string> type RIG_QUERY RIG_PARAMETER_DESCRIPTION("card | ai_message | all"));

} // namespace holder::api
