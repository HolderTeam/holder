// Independent stand-in runtime. Its only JSON behavior comes from generated code.
#include "api/generated/DaemonRoutes.h"
#include <cassert>

namespace holder::api {
LocationActionResponse put_locations_location_id_binding(
    holder::platform::Db& db,
    holder::privacy::SecretStore*,
    std::string id,
    LocationBindingRequest request
) {
  ++db.calls;
  if (id == "missing") throw LocationNotFound{};
  if (id == "unavailable") throw SecretStoreUnavailable{};
  if (id == "broken") throw std::runtime_error("runtime failure");
  LocationActionResponse result{
      true,
      {id, false, std::optional<std::string>(request.preview.value_or("Configured")), request.proof}
  };
  if (id == "null") result.data.binding_preview.emplace();
  if (id == "omitted") result.data.binding_preview.reset();
  return result;
}
} // namespace holder::api

int main() {
  struct Context {
    holder::platform::Db db;
    holder::privacy::SecretStore* secret_store = nullptr;
  } context;
  const auto call = [&](std::string id, std::string body) {
    const auto result =
        daemon_http::dispatch("PUT", "/locations/" + id + "/binding", body, context);
    assert(result);
    return *result;
  };
  const auto result = call("location", R"({"values":{},"proof":true})");
  assert(result.status == 200);
  assert(result.body.at("data").at("proof") == true);
  assert(result.body.at("data").at("bound") == false);
  assert(result.body.at("data").at("binding_preview") == "Configured");
  assert(call("location", R"({"values":{}})").body.at("data").at("proof") == true);
  assert(call("location", R"({"values":{},"proof":false})").body.at("data").at("proof") == false);
  assert(!call("location", R"({"values":{},"proof":null})").body.at("data").contains("proof"));
  assert(
      call("location", R"({"values":{},"preview":""})").body.at("data").at("binding_preview") == ""
  );
  assert(call("null", R"({"values":{}})").body.at("data").at("binding_preview").is_null());
  assert(!call("omitted", R"({"values":{}})").body.at("data").contains("binding_preview"));
  assert(call("missing", R"({"values":{}})").status == 404);
  const auto unavailable = call("unavailable", R"({"values":{}})");
  assert(unavailable.status == 503);
  assert(unavailable.body.at("error").at("code") == "secret_store_unavailable");
  const auto broken = call("broken", R"({"values":{}})");
  assert(broken.status == 500 && broken.body.at("error").at("code") == "internal_error");
  const auto previous_calls = context.db.calls;
  for (const auto* body :
       {R"({})",
        R"({"values":[]})",
        R"({"values":{"secret":42}})",
        R"({"values":{},"preview":null})",
        R"({"values":{},"unexpected":true})",
        R"({"values":{"secret":"bad\u0000text"}})"}) {
    const auto bad = call("location", body);
    assert(bad.status == 400 && bad.body.at("error").at("code") == "invalid_argument");
  }
  assert(context.db.calls == previous_calls);
  assert(!daemon_http::dispatch("DELETE", "/locations/location/binding", "{}", context));
  assert(!daemon_http::dispatch("PUT", "/unrelated", "{}", context));
}
