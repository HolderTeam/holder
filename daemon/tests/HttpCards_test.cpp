#include "http_test_helpers.h"

#include "card/CardRepo.h"

#include <fstream>
#include <optional>

using holder::test::create_project;
using holder::test::ensure_uuid_seeded;
using holder::test::EnvGuard;
using holder::test::http_json_request;
using holder::test::HttpServerThreadGuard;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;
using holder::test::wait_for_http_health_ready;

TEST_CASE("HTTP card create/get/patch", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  const auto project_root = dir / "project_repo";
  create_project(db, "proj-1", project_root.string());
  ensure_uuid_seeded();

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, &card_store, &fts);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  nlohmann::json create_body = {
      {"title", "First"},
      {"content", "hello"},
      {"created_at", 10},
      {"updated_at", 10}
  };

  const auto created = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/projects/proj-1/cards",
      create_body,
      boost::beast::http::status::created
  );
  REQUIRE(created["ok"] == true);
  REQUIRE(created["data"]["card_id"].is_string());
  const std::string first_id = created["data"]["card_id"].get<std::string>();

  nlohmann::json auto_body = {
      {"title", "Auto Card"},
      {"content", "auto"}
  };

  const auto created_auto = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/projects/proj-1/cards",
      auto_body,
      boost::beast::http::status::created
  );
  REQUIRE(created_auto["ok"] == true);
  REQUIRE(created_auto["data"]["card_id"].is_string());
  REQUIRE(created_auto["data"]["card_id"].get<std::string>().size() > 0);
  const std::string auto_id = created_auto["data"]["card_id"].get<std::string>();

  const auto fetched_auto = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/" + auto_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(fetched_auto["data"]["created_at"].get<long long>() > 0);
  REQUIRE(fetched_auto["data"]["updated_at"].get<long long>() > 0);

  const auto fetched = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/" + first_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(fetched["data"]["title"] == "First");
  REQUIRE(fetched["data"]["content"] == "hello");

  const auto listed = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards?parent=roots",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(listed["ok"] == true);
  REQUIRE(listed["data"]["items"].is_array());
  REQUIRE(listed["data"]["items"].size() >= 2);
  bool found_first = false;
  bool found_auto = false;
  for (const auto& item : listed["data"]["items"]) {
    if (item["card_id"] == first_id) {
      found_first = true;
    }
    if (item["card_id"] == auto_id) {
      found_auto = true;
    }
  }
  REQUIRE(found_first);
  REQUIRE(found_auto);

  const auto children_empty = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards?parent=" + first_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(children_empty["ok"] == true);
  REQUIRE(children_empty["data"]["items"].is_array());
  REQUIRE(children_empty["data"]["items"].empty());

  nlohmann::json update_body = {
      {"title", "First Updated"},
      {"content", "hello world"},
      {"updated_at", 20}
  };

  const auto updated = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::patch,
      "/projects/proj-1/cards/" + first_id,
      update_body,
      boost::beast::http::status::ok
  );
  REQUIRE(updated["ok"] == true);

  const auto fetched_after = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/" + first_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(fetched_after["data"]["title"] == "First Updated");
  REQUIRE(fetched_after["data"]["content"] == "hello world");

  nlohmann::json move_body = {{"parent_card_id", auto_id}, {"sort_key", 777.0}, {"updated_at", 30}};

  const auto moved = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::patch,
      "/projects/proj-1/cards/" + first_id,
      move_body,
      boost::beast::http::status::ok
  );
  REQUIRE(moved["ok"] == true);

  const auto fetched_moved = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/" + first_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(fetched_moved["data"]["parent_card_id"] == auto_id);
  REQUIRE(fetched_moved["data"]["sort_key"].get<double>() == 777.0);
  REQUIRE(fetched_moved["data"]["updated_at"] == 30);

  const auto listed_all = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards?order=updated_desc",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  bool found_nested_in_all = false;
  for (const auto& item : listed_all["data"]["items"]) {
    if (item["card_id"] == first_id) {
      found_nested_in_all = true;
      REQUIRE(item["parent_card_id"] == auto_id);
    }
  }
  REQUIRE(found_nested_in_all);

  const auto listed_root = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards?parent=roots",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  bool found_nested_in_root = false;
  for (const auto& item : listed_root["data"]["items"]) {
    if (item["card_id"] == first_id) {
      found_nested_in_root = true;
    }
  }
  REQUIRE(!found_nested_in_root);

  nlohmann::json reparent_root_body = {{"parent_card_id", nullptr}, {"updated_at", 31}};

  const auto reparented = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::patch,
      "/projects/proj-1/cards/" + first_id,
      reparent_root_body,
      boost::beast::http::status::ok
  );
  REQUIRE(reparented["ok"] == true);

  const auto fetched_reparented = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/" + first_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(fetched_reparented["data"]["parent_card_id"].is_null());
  REQUIRE(fetched_reparented["data"]["updated_at"] == 31);

  const auto invalid_order = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards?order=bogus",
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(invalid_order["ok"] == false);

  const auto deleted = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/projects/proj-1/cards/" + first_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(deleted["ok"] == true);

  const auto restored = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/projects/proj-1/cards/" + first_id + "/restore",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(restored["ok"] == true);

  const auto fetched_restored = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/" + first_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(fetched_restored["data"]["title"] == "First Updated");

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP card content stays plaintext over API for encrypted project", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  ensure_uuid_seeded();
  EnvGuard keystore_env("HOLDER_TEST_KEYSTORE_DIR", (dir / "keystore").string());

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, &card_store, &fts);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  HttpServerThreadGuard server_thread(server, signals);
  REQUIRE(wait_for_http_health_ready(bound.bind, bound.port, token));

  const auto project_root = dir / "enc_project_repo";
  const auto created_project = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/projects",
      {
          {"project_id", "proj-enc"},
          {"name", "Encrypted Project"},
          {"root_path", project_root.string()},
      },
      boost::beast::http::status::created
  );
  REQUIRE(created_project["ok"] == true);
  REQUIRE(created_project["data"]["privacy_mode"] == "encrypted_git");
  REQUIRE(created_project["data"]["project_key_id"].is_string());

  const auto created_card = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/projects/proj-enc/cards",
      {
          {"title", "Encrypted Card"},
          {"content", "this is secret plaintext"},
          {"created_at", 10},
          {"updated_at", 10},
      },
      boost::beast::http::status::created
  );
  REQUIRE(created_card["ok"] == true);
  const auto encrypted_card_id = created_card["data"]["card_id"].get<std::string>();

  const auto fetched = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-enc/cards/" + encrypted_card_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(fetched["data"]["content"] == "this is secret plaintext");

  const auto rel_path = std::string(created_card["data"]["rel_path"]);
  const auto full_path = project_root / rel_path;
  std::ifstream in(full_path, std::ios::binary);
  REQUIRE(in.is_open());
  const std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  REQUIRE(raw.rfind("HolderPriv1\n", 0) == 0);
  REQUIRE(raw.find("this is secret plaintext") == std::string::npos);

  server_thread.stop();
}

TEST_CASE("HTTP card create rejects caller-supplied card_id", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  const auto project_root = dir / "project_repo";
  create_project(db, "proj-1", project_root.string());

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, &card_store, &fts);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const std::string supplied_id = "11111111-1111-4111-8111-111111111111";
  nlohmann::json create_body = {
      {"card_id", supplied_id},
      {"title", "First"},
      {"content", "hello"},
      {"created_at", 10},
      {"updated_at", 10}
  };

  const auto rejected = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/projects/proj-1/cards",
      create_body,
      boost::beast::http::status::bad_request
  );
  REQUIRE(rejected["ok"] == false);
  REQUIRE(rejected["error"]["code"] == "invalid_argument");
  REQUIRE(rejected["error"]["message"] == "NewCard has no field card_id");
  holder::card::CardRepo card_repo(db);
  REQUIRE_FALSE(card_repo.get(supplied_id).has_value());

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP card endpoints reject missing fields", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  const auto project_root = dir / "project_repo";
  create_project(db, "proj-1", project_root.string());

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, &card_store, &fts);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto bad_create = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/projects/proj-1/cards",
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(bad_create["ok"] == false);
  // The contract's routes name an invalid argument as the contract does.
  REQUIRE(bad_create["error"]["code"] == "invalid_argument");

  const auto bad_patch = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::patch,
      "/projects/proj-1/cards/abcd1234",
      nlohmann::json{{"title", ""}},
      boost::beast::http::status::bad_request
  );
  REQUIRE(bad_patch["ok"] == false);
  REQUIRE(bad_patch["error"]["code"] == "invalid_argument");

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP card endpoints reject invalid token", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  const auto project_root = dir / "project_repo";
  create_project(db, "proj-1", project_root.string());

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, &card_store, &fts);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  const auto bad = http_json_request(
      bound.bind,
      bound.port,
      "badtoken",
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/abcd1234",
      nlohmann::json::object(),
      boost::beast::http::status::unauthorized
  );
  REQUIRE(bad["ok"] == false);
  REQUIRE(bad["error"]["code"] == "unauthorized");

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP card endpoints handle bad JSON and missing cards", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  const auto project_root = dir / "project_repo";
  create_project(db, "proj-1", project_root.string());

  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, &card_store, &fts);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread server_thread([&server, &signals]() {
    server.run(signals);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;

  boost::asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));

  tcp::socket socket(ioc);
  boost::asio::connect(socket, endpoints);

  http::request<http::string_body> req{http::verb::post, "/projects/proj-1/cards", 11};
  req.set(http::field::host, bound.bind);
  req.set(http::field::user_agent, "holder-tests");
  req.set(http::field::authorization, "Bearer " + token);
  req.set(http::field::content_type, "application/json");
  req.body() = "{ invalid json";
  req.prepare_payload();

  http::write(socket, req);

  boost::beast::flat_buffer buffer;
  http::response<http::string_body> res;
  http::read(socket, buffer, res);

  holder::test::close_http_socket(socket);

  REQUIRE(res.result() == http::status::bad_request);
  const auto error = nlohmann::json::parse(res.body());
  REQUIRE(error["ok"] == false);
  REQUIRE(error["error"]["code"] == "invalid_argument");
  REQUIRE(error["error"]["message"].is_string());

  const auto missing = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/cards/missing",
      nlohmann::json::object(),
      boost::beast::http::status::not_found
  );
  REQUIRE(missing["ok"] == false);
  REQUIRE(missing["error"]["code"] == "not_found");

  std::raise(SIGTERM);
  server_thread.join();
}
