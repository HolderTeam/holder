#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/AssetImportWrite.h"
#include "api/GeneratedRoutes.h"
#include "api/support/StorageErrors.h"
#include "http_test_helpers.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace {
namespace http = boost::beast::http;

http::request<http::string_body> make_request(
    http::verb method,
    const std::string& target,
    const nlohmann::json& body = nlohmann::json()
) {
  http::request<http::string_body> req{method, target, 11};
  req.set(http::field::host, "127.0.0.1");
  if (!body.is_null() && !body.empty()) {
    req.set(http::field::content_type, "application/json");
    req.body() = body.dump();
    req.prepare_payload();
  }
  return req;
}

} // namespace

#include "git/GitOps.h"
#include "google_storage_test_helpers.h"
#include "privacy/ProjectPrivacy.h"
#include "project/ProjectRepo.h"
#include "resource/AssetEnvelope.h"
#include "resource/LocationBindingStore.h"
#include "resource/LocationRepo.h"
#include "resource/ResourceRepo.h"
#include "storage_http_test_server.h"

namespace {
struct LocationRouteFixture {
  std::filesystem::path root = holder::test::make_temp_dir();
  holder::test::EnvGuard cache{"XDG_CACHE_HOME", (root / "cache").string()};
  holder::platform::Db db = holder::test::open_db_with_schema(root / "holder.db");
  std::unique_ptr<holder::privacy::SecretStore> secrets =
      holder::privacy::make_encrypted_file_secret_store_for_tests(root / "secrets");
  unsigned sequence = 0;
  LocationRouteFixture() {
    holder::test::create_project(db, "project", (root / "project").string());
  }
  http::response<http::string_body> call(
      http::verb method,
      const std::string& path,
      nlohmann::json body = nlohmann::json::object(),
      std::string project = "project",
      bool with_secrets = true,
      const std::map<std::string, std::string>& params = {},
      holder::git::GitOps* git = nullptr
  ) {
    http::response<http::string_body> response;
    // Generated routes come first, as in the daemon; reads take the parameters as their query.
    std::string target = path;
    if (method == http::verb::get) {
      std::map<std::string, std::string> query = params;
      if (path == "/locations" && !project.empty()) query.emplace("project_id", project);
      char separator = '?';
      for (const auto& [key, value] : query) {
        target += separator + key + "=" + value;
        separator = '&';
      }
    }
    auto generated_request = make_request(method, target, body);
    if (holder::api::dispatch_generated_routes(
            generated_request, response, db, nullptr, nullptr,
            with_secrets ? secrets.get() : nullptr, git,
            [&] { return "id-" + std::to_string(++sequence); }
        )) {
      return response;
    }
    FAIL("no generated route for " << target);
    return response;
  }
  http::response<http::string_body> poll_import(const std::string& job_id) {
    auto request = make_request(http::verb::get, "/imports/" + job_id, nlohmann::json::object());
    http::response<http::string_body> response;
    REQUIRE(holder::api::dispatch_generated_routes(request, response, db));
    return response;
  }
  void put(
      const std::string& provider,
      std::map<std::string, std::string> configuration,
      std::map<std::string, std::string> values
  ) {
    holder::resource::LocationRepo(db).put(
        {"location", "project", "Storage", provider, std::move(configuration), 1, 1}
    );
    holder::resource::LocationBindingStore(*secrets)
        .bind("project", "location", {1, provider, std::move(values)}, "Configured", 1);
  }
};
} // namespace

TEST_CASE("Location routes manage bindings preferences and declarations", "[resources][routes]") {
  LocationRouteFixture f;
  auto created = f.call(
      http::verb::post,
      "/locations",
      {{"location_id", "location"},
       {"project_id", "project"},
       {"name", "Storage"},
       {"provider", "local_directory"}}
  );
  REQUIRE(created.result() == http::status::created);
  CHECK(
      f.call(
           http::verb::put,
           "/locations/location/binding",
           {{"values", {{"root_path", (f.root / "storage").string()}}}}
      ).result() == http::status::ok
  );
  CHECK(
      f.call(
           http::verb::put,
           "/locations/preferred",
           {{"project_id", "project"}, {"location_id", "location"}}
      ).result() == http::status::ok
  );
  auto details = f.call(http::verb::get, "/locations/location");
  CHECK(nlohmann::json::parse(details.body())["data"]["bound"] == true);
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::ok);
  CHECK(
      f.call(
           http::verb::patch,
           "/locations/location",
           {{"name", "Renamed"}, {"configuration", {{"prefix", "//archive//"}}}, {"updated_at", 22}}
      ).result() == http::status::ok
  );
  REQUIRE(holder::resource::LocationRepo(f.db).get("location").has_value());
  CHECK(holder::resource::LocationRepo(f.db).get("location")->name == "Renamed");
  CHECK(f.call(http::verb::delete_, "/locations/location/binding").result() == http::status::ok);
  CHECK_FALSE(holder::resource::LocationBindingStore(*f.secrets).preferred("project").has_value());
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::conflict);
  CHECK(
      f.call(
           http::verb::put,
           "/locations/preferred",
           {{"project_id", "project"}, {"location_id", "location"}}
      ).result() == http::status::ok
  );
  CHECK(f.call(http::verb::delete_, "/locations/location").result() == http::status::ok);
  CHECK_FALSE(holder::resource::LocationRepo(f.db).get("location").has_value());
  CHECK_FALSE(holder::resource::LocationBindingStore(*f.secrets).preferred("project").has_value());
}

TEST_CASE("Location routes validate missing services fields and records", "[resources][routes]") {
  LocationRouteFixture f;
  // Bodies are decoded before the location is looked up, so writes send valid ones.
  const std::map<http::verb, nlohmann::json> bodies{
      {http::verb::patch, {{"name", "Renamed"}}},
      {http::verb::put, {{"values", {{"root_path", "/tmp"}}}}},
  };
  const auto body_for = [&](http::verb method) {
    const auto found = bodies.find(method);
    return found == bodies.end() ? nlohmann::json::object() : found->second;
  };
  for (auto method : {http::verb::get, http::verb::patch, http::verb::delete_}) {
    CHECK(f.call(method, "/locations/missing", body_for(method)).result() == http::status::not_found);
  }
  CHECK(f.call(http::verb::patch, "/locations/missing").result() == http::status::not_found);
  for (auto method : {http::verb::put, http::verb::delete_}) {
    CHECK(
        f.call(method, "/locations/missing/binding", body_for(method)).result() ==
        http::status::not_found
    );
    CHECK(
        f.call(method, "/locations/missing/binding", body_for(method), "project", false).result() ==
        http::status::service_unavailable
    );
  }
  CHECK(f.call(http::verb::post, "/locations/missing/test").result() == http::status::not_found);
  CHECK(
      f.call(http::verb::post, "/locations/missing/test", {}, "project", false).result() ==
      http::status::service_unavailable
  );
  CHECK(f.call(http::verb::get, "/locations", {}, "").result() == http::status::bad_request);
  CHECK(
      f.call(http::verb::post, "/locations", {{"name", "Missing fields"}}).result() ==
      http::status::bad_request
  );
  CHECK(
      f.call(
           http::verb::post,
           "/locations",
           {{"name", "Bad"}, {"project_id", "project"}, {"provider", "unsupported"}}
      ).result() == http::status::bad_request
  );
  CHECK(
      f.call(
           http::verb::put,
           "/locations/preferred",
           {{"project_id", "project"}, {"location_id", "missing"}}
      ).result() == http::status::not_found
  );
  CHECK(f.call(http::verb::post, "/imports").result() == http::status::bad_request);
}

TEST_CASE(
    "Location probes exercise S3 credentials and clean up after provider failures",
    "[resources][routes]"
) {
  using Server = holder::test::StorageHttpTestServer;
  unsigned response_status = 200;
  http::status expected = http::status::ok;
  bool missing_after_put = false;
  SECTION("success") {}
  SECTION("authentication") {
    response_status = 401;
    expected = http::status::bad_gateway;
  }
  SECTION("permission") {
    response_status = 403;
    expected = http::status::bad_gateway;
  }
  SECTION("integrity") {
    response_status = 404;
    expected = http::status::service_unavailable;
  }
  SECTION("transient") {
    response_status = 503;
    expected = http::status::service_unavailable;
  }
  SECTION("the provider accepts the write but does not have the object") {
    missing_after_put = true;
    expected = http::status::unprocessable_entity;
  }
  LocationRouteFixture f;
  Server server([&](const Server::Request& request) {
    if (missing_after_put && request.method() == http::verb::head) return Server::response(404);
    return Server::response(response_status);
  });
  f.put(
      "s3_compatible",
      {{"endpoint", server.endpoint()},
       {"region", "region"},
       {"bucket", "bucket"},
       {"allow_insecure_localhost", "true"},
       {"addressing_style", "path"},
       {"prefix", "//archive//"}},
      {{"access_key_id", "access"}, {"secret_access_key", "secret"}, {"session_token", "session"}}
  );
  const auto response = f.call(http::verb::post, "/locations/location/test");
  INFO(response.body());
  CHECK(response.result() == expected);
  server.stop();
  server.rethrow_error();
  REQUIRE_FALSE(server.requests().empty());
  CHECK(std::string(server.requests()[0].target()).starts_with("/bucket/archive/.holder-probes/"));
  CHECK(server.requests()[0]["x-amz-security-token"] == "session");
  if (missing_after_put) {
    REQUIRE(server.requests().size() == 3);
    CHECK(server.requests()[1].method() == http::verb::head);
    CHECK(server.requests()[2].method() == http::verb::delete_);
    CHECK(nlohmann::json::parse(response.body())["error"]["code"] == "storage_integrity_failed");
  }
  CHECK(std::filesystem::is_empty(f.root / "cache/holder/asset-probes"));
}

TEST_CASE(
    "Location probes report invalid provider configuration before transferring",
    "[resources][routes]"
) {
  LocationRouteFixture f;
  f.put("s3_compatible", {}, {});
  for (const auto& [key, value] : std::map<std::string, std::string>{
           {"endpoint", "https://storage.invalid"},
           {"region", "region"},
           {"bucket", "bucket"}
       }) {
    CHECK(
        f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request
    );
    auto location = *holder::resource::LocationRepo(f.db).get("location");
    location.configuration[key] = value;
    holder::resource::LocationRepo(f.db).put(location);
  }
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request);
  f.put(
      "s3_compatible",
      {{"region", "region"}, {"bucket", "bucket"}},
      {{"endpoint", "https://storage.invalid"}, {"access_key_id", "access"}}
  );
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request);
  f.put("google-drive", {}, {});
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request);
  f.put("google-drive", {{"folder_id", "folder"}}, {});
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request);
  f.put("local_directory", {}, {});
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request);
  f.put("unsupported", {}, {});
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request);
  f.put("local_directory", {}, {{"root_path", f.root.string()}});
  holder::resource::LocationBindingStore(*f.secrets)
      .bind("project", "location", {1, "google-drive", {}}, "Mismatch", 1);
  CHECK(f.call(http::verb::post, "/locations/location/test").result() == http::status::bad_request);
}

TEST_CASE("Location probes use Google OAuth and remove the probe object", "[resources][routes]") {
  using Server = holder::test::StorageHttpTestServer;
  LocationRouteFixture f;
  holder::test::EnvGuard client("HOLDER_GOOGLE_OAUTH_CLIENT_ID", "client");
  holder::test::EnvGuard secret("HOLDER_GOOGLE_OAUTH_CLIENT_SECRET", "secret");
  bool present = false;
  holder::test::GoogleStorageTestServer google([&](const Server::Request& request) {
    if (request.target() == "/token") return Server::response(200, R"({"access_token":"access"})");
    if (request.method() == http::verb::get)
      return Server::response(200, present ? R"({"files":[{"id":"probe"}]})" : R"({"files":[]})");
    if (request.method() == http::verb::post) {
      present = true;
      return Server::response(200, R"({"id":"probe"})");
    }
    present = false;
    return Server::response(204);
  });
  f.put("google-drive", {{"folder_id", "folder"}}, {{"refresh_token", "refresh"}});
  const auto response = f.call(http::verb::post, "/locations/location/test");
  INFO(response.body());
  CHECK(response.result() == http::status::ok);
  google.finish();
  CHECK_FALSE(present);
  CHECK(std::filesystem::is_empty(f.root / "cache/holder/asset-probes"));
}

#include "resource/StorageProvider.h"

TEST_CASE("Storage failures keep their categories as HTTP statuses", "[resources][routes]") {
  using Code = holder::resource::StorageErrorCode;
  struct Case {
    Code code;
    unsigned status;
    const char* name;
  };
  for (const auto& item : std::vector<Case>{
           {Code::Unavailable, 503, "storage_unavailable"},
           {Code::Authentication, 502, "storage_authentication_failed"},
           {Code::Permission, 502, "storage_permission_denied"},
           {Code::Capacity, 507, "storage_capacity_exceeded"},
           {Code::Integrity, 422, "storage_integrity_failed"},
           {Code::Conflict, 409, "storage_conflict"},
           {Code::InvalidConfiguration, 400, "storage_configuration_invalid"},
           {Code::Transient, 503, "storage_transient_failure"}
       }) {
    CAPTURE(item.name);
    const auto [status, code] = holder::api::support::storage_failure_status(
        holder::resource::StorageError(item.code, "storage failure")
    );
    CHECK(status == item.status);
    CHECK(std::string(code) == item.name);
  }
}

TEST_CASE("Asset and location routes report bad requests and database failures", "[resources][routes]") {
  LocationRouteFixture f;
  CHECK(
      f.call(http::verb::get, "/resources/resource/assets/asset/content").result() ==
      http::status::bad_request
  );
  f.db.exec("DROP TABLE storage_locations");
  CHECK(f.call(http::verb::get, "/locations").result() == http::status::internal_server_error);
}

TEST_CASE(
    "Resource imports validate inputs and publish background failures",
    "[resources][routes]"
) {
  LocationRouteFixture f;
  holder::git::RealGitOps git;
  const auto source = f.root / "source.txt";
  std::ofstream(source) << "asset";

  f.put("local_directory", {}, {{"root_path", (f.root / "objects").string()}});
  CHECK(
      f.call(
           http::verb::post,
           "/imports",
           {{"project_id", "project"},
            {"card_id", "card"},
            {"location_id", "location"},
            {"source_path", (f.root / "missing").string()}},
           "project",
           true,
           {},
           &git
      )
          .result() == http::status::bad_request
  );
  CHECK(
      f.call(
           http::verb::post,
           "/imports",
           {{"project_id", "other"},
            {"card_id", "card"},
            {"location_id", "location"},
            {"source_path", source.string()}},
           "other",
           true,
           {},
           &git
      )
          .result() == http::status::not_found
  );

  const nlohmann::json valid = {{"project_id", "project"},
                                {"card_id", "card"},
                                {"location_id", "location"},
                                {"source_path", source.string()}};
  CHECK(f.call(http::verb::post, "/imports", valid, "project", false, {}, &git).result() ==
        http::status::service_unavailable);
  CHECK(f.call(http::verb::post, "/imports", valid).result() == http::status::not_implemented);
  holder::resource::LocationRepo(f.db).put(
      {"unbound", "project", "Unbound", "local_directory", {}, 1, 1});
  auto unbound = valid;
  unbound["location_id"] = "unbound";
  const auto needs_binding =
      f.call(http::verb::post, "/imports", unbound, "project", true, {}, &git);
  CHECK(needs_binding.result() == http::status::conflict);
  CHECK(nlohmann::json::parse(needs_binding.body())["error"]["code"] ==
        "storage_binding_required");

  f.put("unsupported", {}, {});
  const auto started = f.call(
      http::verb::post,
      "/imports",
      {{"project_id", "project"},
       {"card_id", "card"},
       {"location_id", "location"},
       {"source_path", source.string()}},
      "project",
      true,
      {},
      &git
  );
  REQUIRE(started.result() == http::status::accepted);
  const auto job_id = nlohmann::json::parse(started.body())["data"]["job_id"].get<std::string>();
  holder::api::wait_for_asset_import_jobs();
  const auto job = nlohmann::json::parse(f.poll_import(job_id).body());
  CHECK(job["data"]["status"] == "failed");
  CHECK_FALSE(job["data"]["error"].get<std::string>().empty());
  CHECK(job["data"]["resource_id"].is_null());
  CHECK(job["data"]["link_created"] == false);
}

TEST_CASE("Polling an unknown asset import job is not found", "[resources][routes]") {
  LocationRouteFixture f;
  const auto missing = f.poll_import("missing");
  REQUIRE(missing.result() == http::status::not_found);
  REQUIRE(nlohmann::json::parse(missing.body())["error"]["message"] == "Import job not found.");
}

TEST_CASE("Resource asset retrieval validates placement ownership", "[resources][routes]") {
  LocationRouteFixture f;
  holder::git::RealGitOps git;
  f.put("local_directory", {}, {{"root_path", (f.root / "objects").string()}});
  holder::model::ResourceBundle bundle{
      .resource = {"resource", "project", "file", "Asset", {}, 1, 1},
      .assets =
          {{"asset",
            "resource",
            "asset.txt",
            "text/plain",
            5,
            "hash",
            1,
            1,
            {{"placement", "asset", "location", "object", "identity", 5, "hash", 1}}}}
  };
  holder::resource::ResourceRepo(f.db).put_bundle(bundle);
  boost::asio::io_context io;
  boost::asio::ip::tcp::socket socket(io);

  auto retrieve = [&](const std::map<std::string, std::string>& params, bool& streamed) {
    std::string target = "/resources/resource/assets/asset/content";
    if (const auto found = params.find("placement_id"); found != params.end())
      target += "?placement_id=" + found->second;
    auto request = make_request(http::verb::get, target);
    http::response<http::string_body> response;
    const auto served = holder::test::serve_generated(
        request,
        response,
        socket,
        f.db,
        nullptr,
        nullptr,
        f.secrets.get(),
        &git,
        [] {
          return std::string("recovered");
        }
    );
    REQUIRE(served.handled);
    streamed = served.streamed;
    return response;
  };

  bool streamed = false;
  CHECK(retrieve({{"placement_id", "other"}}, streamed).result() == http::status::not_found);
  // A location that a placement still uses cannot be deleted, with or without Git.
  for (holder::git::GitOps* ops : {static_cast<holder::git::GitOps*>(nullptr),
                                   static_cast<holder::git::GitOps*>(&git)}) {
    const auto in_use = f.call(http::verb::delete_, "/locations/location", {}, "project", true, {}, ops);
    CHECK(in_use.result() == http::status::conflict);
    CHECK(nlohmann::json::parse(in_use.body())["error"]["code"] == "conflict");
  }
  holder::resource::LocationBindingStore(*f.secrets).unbind("project", "location");
  const auto unbound = retrieve({}, streamed);
  REQUIRE(unbound.result() == http::status::conflict);
  CHECK(nlohmann::json::parse(unbound.body())["error"]["code"] == "storage_binding_required");
  CHECK_FALSE(streamed);

  f.db.exec("PRAGMA foreign_keys=OFF; DELETE FROM storage_locations; PRAGMA foreign_keys=ON;");
  CHECK(retrieve({}, streamed).result() == http::status::not_found);
  CHECK_FALSE(streamed);
}

TEST_CASE("Location updates and removals persist their Git manifests", "[resources][routes]") {
  LocationRouteFixture f;
  holder::git::RealGitOps git;
  f.put("local_directory", {}, {{"root_path", f.root.string()}});
  REQUIRE(
      f.call(
           http::verb::patch,
           "/locations/location",
           {{"name", "Changed"}},
           "project",
           true,
           {},
           &git
      )
          .result() == http::status::ok
  );
  CHECK(holder::resource::LocationRepo(f.db).get("location")->name == "Changed");
  REQUIRE(
      f.call(http::verb::delete_, "/locations/location", {}, "project", true, {}, &git).result() ==
      http::status::ok
  );
  CHECK_FALSE(holder::resource::LocationRepo(f.db).get("location").has_value());
}

TEST_CASE("Resource routes reject malformed bindings and import requests", "[resources][routes]") {
  LocationRouteFixture f;
  struct Case {
    http::verb method;
    const char* path;
    nlohmann::json body;
    const char* message;
  };
  for (const auto& item : std::vector<Case>{
           {http::verb::put, "/locations/location/binding", {{"values", "invalid"}},
            "values must be an object"},
           {http::verb::put, "/locations/location/binding", {{"values", {{"root_path", 42}}}},
            "binding values must be strings"},
           {http::verb::put, "/locations/preferred", {{"project_id", "project"}, {"location_id", 42}},
            "location_id must be a string"},
           {http::verb::post, "/imports", {{"unexpected", true}}, "there is no field unexpected"},
           {http::verb::get, "/imports/missing?unexpected=true", nlohmann::json::object(),
            "unexpected"},
       }) {
    CAPTURE(item.path, item.body);
    const auto res = f.call(item.method, item.path, item.body);
    REQUIRE(res.result() == http::status::bad_request);
    const auto error = nlohmann::json::parse(res.body()).at("error");
    CHECK(error.at("code") == "invalid_argument");
    CHECK(error.at("message").get<std::string>().find(item.message) != std::string::npos);
  }
}

TEST_CASE("Storage failure exceptions preserve the typed response and its error", "[resources][routes]") {
  using Code = holder::resource::StorageErrorCode;
  using namespace holder::api::support;
  const auto check = []<typename Failure>(Code code, const char* name) {
    CAPTURE(name);
    try {
      throw_storage_failure(holder::resource::StorageError(code, "provider detail"));
    } catch (const Failure& failure) {
      CHECK(failure.code == name);
      CHECK(std::string(failure.what()) == "provider detail");
    }
  };
  check.operator()<StorageBadRequest>(Code::InvalidConfiguration, "storage_configuration_invalid");
  check.operator()<StorageConflict>(Code::Conflict, "storage_conflict");
  check.operator()<StorageIntegrityFailed>(Code::Integrity, "storage_integrity_failed");
  check.operator()<StorageGatewayFailure>(Code::Permission, "storage_permission_denied");
  check.operator()<StorageCapacityExceeded>(Code::Capacity, "storage_capacity_exceeded");
  check.operator()<StorageUnavailable>(Code::Transient, "storage_transient_failure");
}

TEST_CASE("Asset downloads report storage failures and clean up after a write failure",
          "[resources][routes]") {
  LocationRouteFixture f;
  holder::git::RealGitOps git;
  const auto objects = f.root / "objects";
  std::filesystem::create_directories(objects);
  const auto object = objects / "object";
  std::ofstream(object, std::ios::binary) << "asset";
  const auto digest = holder::resource::digest_file(object);
  f.put("local_directory", {}, {{"root_path", objects.string()}});

  holder::model::ResourceBundle bundle{
      .resource = {"resource", "project", "file", "Asset", {}, 1, 1},
      .assets = {{"asset", "resource", "asset.txt", "text/plain", digest.byte_size,
                  digest.sha256, 1, 1,
                  {{"placement", "asset", "location", "object", "plain",
                    digest.byte_size, digest.sha256, 1}}}},
  };
  const auto cache = f.root / "cache/holder/asset-cache";

  std::unique_ptr<holder::test::StorageHttpTestServer> server;
  std::optional<http::status> expected_error;
  std::string expected_code;
  SECTION("missing object") {
    expected_error = http::status::service_unavailable;
    expected_code = "storage_unavailable";
    std::filesystem::remove(object);
  }
  SECTION("object bytes fail integrity verification") {
    expected_error = http::status::unprocessable_entity;
    expected_code = "storage_integrity_failed";
    std::ofstream(object, std::ios::binary) << "wrong";
  }
  SECTION("stored encoding is unsupported") {
    expected_error = http::status::bad_request;
    expected_code = "bad_request";
    bundle.assets.front().placements.front().encoding = "unsupported";
  }
  SECTION("object key escapes the storage root") {
    expected_error = http::status::bad_request;
    expected_code = "storage_configuration_invalid";
    bundle.assets.front().placements.front().object_key = "../object";
  }
  SECTION("remote provider denies access") {
    using Server = holder::test::StorageHttpTestServer;
    server = std::make_unique<Server>([](const Server::Request&) {
      return Server::response(403);
    });
    f.put("s3_compatible",
          {{"endpoint", server->endpoint()}, {"region", "region"}, {"bucket", "bucket"},
           {"allow_insecure_localhost", "true"}, {"addressing_style", "path"}},
          {{"access_key_id", "access"}, {"secret_access_key", "secret"}});
    expected_error = http::status::bad_gateway;
    expected_code = "storage_permission_denied";
  }
  SECTION("socket cannot write the recovered file") {}

  holder::resource::ResourceRepo(f.db).put_bundle(bundle);
  boost::asio::io_context io;
  boost::asio::ip::tcp::socket closed_socket(io);
  auto req = make_request(http::verb::get, "/resources/resource/assets/asset/content");
  http::response<http::string_body> res;
  const auto served = holder::test::serve_generated(
      req, res, closed_socket, f.db, nullptr, nullptr, f.secrets.get(), &git,
      [] { return std::string("download-test"); });
  INFO(res.body());
  REQUIRE(served.handled);
  CHECK(std::filesystem::is_empty(cache));

  REQUIRE(served.streamed == !expected_error.has_value());
  if (expected_error) {
    CHECK(res.result() == *expected_error);
    const auto error = nlohmann::json::parse(res.body()).at("error");
    CHECK(error.at("code") == expected_code);
  }
}

TEST_CASE("Resource writes preserve an attached resource when its project key is missing",
          "[resources][routes]") {
  LocationRouteFixture f;
  holder::test::EnvGuard keys("HOLDER_TEST_KEYSTORE_DIR", (f.root / "empty-keystore").string());
  holder::git::RealGitOps git;
  holder::project::ProjectRepo projects(f.db);
  projects.update_privacy_mode("project", "encrypted_git", 1);
  const auto key_id = holder::privacy::ensure_project_key_material(
      projects, "project", std::nullopt, 1, [] { return std::string("fixture-key"); });
  holder::card::CardStore cards(f.db, nullptr);
  holder::test::create_card_fixture(cards, "card", "project", "Card", "Body", 10);

  const std::string list_path = "/projects/project/resources";
  const auto resource_path = list_path + "/resource";
  const nlohmann::json body{{"resource_id", "resource"}, {"type", "file"}, {"label", "Original"}};
  const auto call = [&](http::verb method, const std::string& path,
                        const nlohmann::json& payload = nlohmann::json::object()) {
    return f.call(method, path, payload, "project", true, {}, &git);
  };
  REQUIRE(call(http::verb::post, list_path, body).result() == http::status::created);
  REQUIRE(call(http::verb::put, "/projects/project/cards/card/resources/resource").result() ==
          http::status::ok);

  const auto check_error = [](const auto& res, http::status status, const char* code) {
    INFO(res.body());
    REQUIRE(res.result() == status);
    CHECK(nlohmann::json::parse(res.body()).at("error").at("code") == code);
  };
  check_error(call(http::verb::post, list_path, body), http::status::conflict, "conflict");
  check_error(call(http::verb::post, "/projects/missing/resources", body),
              http::status::not_found, "not_found");
  check_error(call(http::verb::patch, list_path + "/missing", {{"label", "Missing"}}),
              http::status::not_found, "not_found");

  holder::git::GitRepo repo;
  repo.open_or_init(f.root / "project");
  const auto head = repo.head_oid();
  REQUIRE(head);
  REQUIRE(std::filesystem::remove(f.root / "empty-keystore" / (key_id + ".key")));

  auto new_resource = body;
  new_resource["resource_id"] = "new-resource";
  check_error(call(http::verb::post, list_path, new_resource), http::status::conflict, "key_missing");
  check_error(call(http::verb::patch, resource_path, {{"label", "Changed"}}),
              http::status::conflict, "key_missing");
  check_error(call(http::verb::delete_, resource_path), http::status::conflict, "key_missing");
  CHECK(repo.head_oid() == head);
  holder::resource::ResourceRepo resources(f.db);
  const auto original = resources.get("resource");
  REQUIRE(original);
  CHECK(original->label == "Original");
  CHECK_FALSE(resources.get("new-resource"));
  const auto attached = call(http::verb::get, "/projects/project/cards/card/resources");
  REQUIRE(attached.result() == http::status::ok);
  CHECK(nlohmann::json::parse(attached.body()).at("data").size() == 1);
}

TEST_CASE("Asset import reports a failed location lookup without accepting a job",
          "[resources][routes]") {
  LocationRouteFixture f;
  holder::git::RealGitOps git;
  const auto source = f.root / "source.txt";
  std::ofstream(source) << "asset";
  f.db.exec("DROP TABLE storage_locations");
  const auto res = f.call(
      http::verb::post, "/imports",
      {{"project_id", "project"}, {"card_id", "card"}, {"location_id", "location"},
       {"source_path", source.string()}},
      "project", true, {}, &git);
  REQUIRE(res.result() == http::status::internal_server_error);
  CHECK(nlohmann::json::parse(res.body()).at("error").at("code") == "internal_error");
  CHECK(f.sequence == 0);
}
