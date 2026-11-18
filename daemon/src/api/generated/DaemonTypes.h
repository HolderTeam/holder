// Generated from contract/interface (ai, cards, daemon, documents, errors.py, events, git, history,
// links, locations, maintenance, milestones, pagination, projects, recovery, resources, runtime,
// search, trash) by compiler. Do not edit; change the contract, then run make -C contract update.
//
// Complete daemon boundary records and handler signatures.
#pragma once
#include <map>
#include <optional>
#include <string>

namespace holder::platform {
class Db;
}
namespace holder::privacy {
class SecretStore;
}

namespace holder::api {
struct LocationBindingRequest {
  std::map<std::string, std::string> values{};
  std::optional<std::string> preview{};
};

struct LocationAction {
  std::string location_id{};
  std::optional<bool> bound{};
  std::optional<std::optional<std::string>> binding_preview{};
};

struct LocationActionResponse {
  bool ok{};
  holder::api::LocationAction data{};
};

holder::api::LocationActionResponse put_locations_location_id_binding(
    holder::platform::Db& db,
    holder::privacy::SecretStore* secret_store,
    std::string location_id,
    holder::api::LocationBindingRequest request
);

} // namespace holder::api
