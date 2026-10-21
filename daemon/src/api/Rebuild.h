#pragma once

#include "api/HttpError.h"
#include "api/RigAnnotations.h"

#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::index { class FtsIndexer; }

namespace holder::api {

struct RIG_OPEN_RECORD RebuildRequest {
  std::string project_id;
};

struct RIG_OPEN_RECORD RebuildData {
  std::string project_id;
  int cards;
  int ai_messages;
  int ai_threads;
  int links;
};

struct RIG_OPEN_RECORD RebuildResponse {
  bool ok RIG_BOOL_VALUE(true);
  RebuildData data;
};

struct RebuildProjectNotFound : std::runtime_error {
  RebuildProjectNotFound() : std::runtime_error("Project not found.") {}
};

struct RebuildFailed : std::runtime_error {
  using std::runtime_error::runtime_error;
};

RIG_ROUTE(POST, "/rebuild")
RIG_SUMMARY("Rebuild database state from project files")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", RebuildFailed)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Project not found", RebuildProjectNotFound)
RebuildResponse rebuild_project_from_files(holder::platform::Db& db RIG_CONTEXT,
                                           holder::index::FtsIndexer* fts RIG_CONTEXT,
                                           RebuildRequest request);

} // namespace holder::api
