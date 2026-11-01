#pragma once

// The generated asset file operations for the C API's tests, with an import's result as JSON.

#include <holder/holder.h>
#include <nlohmann/json.hpp>

namespace holder::test {

inline int import_asset_file(holder_context* context, const char* project_id, const char* card_id,
                             const char* location_id, const char* source_path, nlohmann::json* out,
                             holder_error** error) {
  holder_arena* arena = holder_arena_new();
  const holder_asset_import_request request{project_id, card_id, location_id, source_path};
  const holder_asset_import* result = nullptr;
  const int rc = holder_import_asset_file(context, &request, arena, &result, error);
  if (rc == HOLDER_OK && out != nullptr) {
    *out = {{"resource_id", result->resource_id},
            {"asset_id", result->asset_id},
            {"duplicate_reused", result->duplicate_reused},
            {"link_created", result->link_created}};
  }
  holder_arena_free(arena);
  return rc;
}

inline int retrieve_asset_file(holder_context* context, const char* resource_id, const char* asset_id,
                               const char* placement_id, const char* destination_path, holder_error** error) {
  holder_arena* arena = holder_arena_new();
  const holder_asset_file* result = nullptr;
  const int rc =
      holder_retrieve_asset_file(context, resource_id, asset_id, placement_id, destination_path, arena, &result, error);
  holder_arena_free(arena);
  return rc;
}

} // namespace holder::test
