#pragma once

#include "api/HttpError.h"
#include "api/ResourceRead.h"
#include "api/RigAnnotations.h"
#include "api/declared/Schemas.h"
#include "model/ConflictError.h"

#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

namespace holder::platform { class Db; }
namespace holder::git { class GitOps; }

namespace holder::api {

struct RIG_OPEN_RECORD ResourceCreateRequest {
  std::optional<std::string> resource_id RIG_FIELD_DESCRIPTION("Optional; server generates if omitted.") RIG_OMIT_EMPTY;
  std::string project_id;
  std::string type;
  std::string label;
  std::optional<ResourceMetadata> metadata RIG_OMIT_EMPTY;
  std::optional<long long> created_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses created_at if omitted.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD ResourceUpdateRequest {
  std::optional<std::string> type RIG_OMIT_EMPTY;
  std::optional<std::string> label RIG_OMIT_EMPTY;
  std::optional<ResourceMetadata> metadata RIG_FIELD_DESCRIPTION("Partial metadata update. Empty arrays remove a property; omitted properties are preserved.") RIG_OMIT_EMPTY;
  std::optional<long long> updated_at RIG_FIELD_DESCRIPTION("Optional; server uses current time if omitted.") RIG_OMIT_EMPTY;
};

struct RIG_OPEN_RECORD RIG_INLINE_RECORD ResourceDeleteResponseData {
  std::string resource_id;
};

struct RIG_OPEN_RECORD ResourceDeleteResponse {
  bool ok RIG_BOOL_VALUE(true);
  ResourceDeleteResponseData data;
};

RIG_ROUTE(POST, "/resources")
RIG_SUMMARY("Create a resource")
RIG_RESPONSE_DESCRIPTION("Created")
RIG_SUCCESS_STATUS(201)
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request, including a project that does not exist", std::invalid_argument)
RIG_ERROR_RESPONSE(409, ErrorResponse, "A resource with this ID already exists", holder::model::ConflictError)
ResourceResponse post_resources(holder::platform::Db& db RIG_CONTEXT,
                                holder::git::GitOps* git_ops RIG_CONTEXT,
                                const std::function<std::string()>& uuid_v4 RIG_CONTEXT,
                                ResourceCreateRequest request);

RIG_ROUTE(PATCH, "/resources/{resource_id}")
RIG_SUMMARY("Update a resource")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(400, ErrorResponse, "Bad request", std::invalid_argument)
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", ResourceNotFound)
ResourceResponse patch_resources_resource_id(holder::platform::Db& db RIG_CONTEXT,
                                             holder::git::GitOps* git_ops RIG_CONTEXT,
                                             std::string resource_id,
                                             ResourceUpdateRequest request);

RIG_ROUTE(DELETE, "/resources/{resource_id}")
RIG_SUMMARY("Delete a resource and remove its Card links")
RIG_RESPONSE_DESCRIPTION("OK")
RIG_ERROR_RESPONSE(404, ErrorResponse, "Not found", ResourceNotFound)
ResourceDeleteResponse delete_resources_resource_id(holder::platform::Db& db RIG_CONTEXT,
                                                    holder::git::GitOps* git_ops RIG_CONTEXT,
                                                    std::string resource_id);

} // namespace holder::api
