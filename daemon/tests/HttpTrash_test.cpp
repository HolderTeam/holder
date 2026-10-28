#include "http_test_helpers.h"

#include "ai/AiMessageRepo.h"
#include "ai/AiThreadRepo.h"
#include "card/CardRepo.h"
#include "model/AiThread.h"

using holder::test::create_project;
using holder::test::http_json_request;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;

TEST_CASE("HTTP trash list/empty/hard delete", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  const auto project_root = dir / "project_repo";
  create_project(db, "proj-1", project_root.string());

  holder::model::AiThread thread;
  thread.thread_id = "thread-1";
  thread.project_id = "proj-1";
  thread.title = "Thread";
  thread.created_at = 1;
  thread.updated_at = 1;
  holder::ai::AiThreadRepo thread_repo(db);
  thread_repo.create(thread);

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

  holder::test::create_card_fixture(card_store, "card-1", "proj-1", "Card", "Hello", 10);

  nlohmann::json msg_body = {
      {"message_id", "msg-1"},
      {"thread_id", "thread-1"},
      {"role", "user"},
      {"source", "manual"},
      {"content", "Hi"},
      {"created_at", 11}
  };
  http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/ai/messages",
      msg_body,
      boost::beast::http::status::created
  );

  http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/projects/proj-1/cards/card-1",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/ai/messages/msg-1",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );

  const auto trash_list = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/trash?project_id=proj-1",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(trash_list["ok"] == true);
  REQUIRE(trash_list["data"].is_array());
  REQUIRE(trash_list["data"].size() == 2);

  const auto trash_cards = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/trash?project_id=proj-1&type=card",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(trash_cards["ok"] == true);
  REQUIRE(trash_cards["data"].is_array());
  REQUIRE(trash_cards["data"].size() == 1);
  REQUIRE(trash_cards["data"][0]["type"] == "card");

  const auto trash_msgs = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/trash?project_id=proj-1&type=ai_message",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(trash_msgs["ok"] == true);
  REQUIRE(trash_msgs["data"].is_array());
  REQUIRE(trash_msgs["data"].size() == 1);
  REQUIRE(trash_msgs["data"][0]["type"] == "ai_message");

  // The contract gives every item the same nullable fields.
  auto keys = [](const nlohmann::json& item) {
    std::vector<std::string> names;
    for (const auto& [name, value] : item.items()) names.push_back(name);
    return names;
  };
  const auto& trashed_card = trash_cards["data"][0];
  REQUIRE(keys(trashed_card) ==
          std::vector<std::string>{
              "card_id", "created_at", "deleted_at", "message_id", "project_id", "rel_path", "role",
              "thread_id", "title", "type"});
  REQUIRE(trashed_card["card_id"] == "card-1");
  REQUIRE(trashed_card["project_id"] == "proj-1");
  REQUIRE(trashed_card["title"] == "Card");
  REQUIRE(trashed_card["deleted_at"].is_number_integer());
  REQUIRE(trashed_card["rel_path"].is_string());
  const auto& trashed_message = trash_msgs["data"][0];
  REQUIRE(keys(trashed_message) ==
          std::vector<std::string>{
              "card_id", "created_at", "deleted_at", "message_id", "project_id", "rel_path", "role",
              "thread_id", "title", "type"});
  REQUIRE(trashed_message["message_id"] == "msg-1");
  REQUIRE(trashed_message["thread_id"] == "thread-1");
  REQUIRE(trashed_message["project_id"] == "proj-1");
  REQUIRE(trashed_message["role"] == "user");
  REQUIRE(trashed_message["created_at"] == 11);

  for (const auto& [query, count] :
       std::vector<std::pair<std::string, std::size_t>>{
           {"&type=all", 2}, {"&type=", 2}, {"&type=unknown", 0}}) {
    CAPTURE(query);
    const auto listed = http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::get,
        "/trash?project_id=proj-1" + query,
        nlohmann::json::object(),
        boost::beast::http::status::ok
    );
    REQUIRE(listed["data"].size() == count);
  }
  REQUIRE(
      http_json_request(
          bound.bind,
          bound.port,
          token,
          boost::beast::http::verb::get,
          "/trash?project_id=",
          nlohmann::json::object(),
          boost::beast::http::status::bad_request
      )["error"]["code"] == "invalid_argument"
  );

  const auto hard_deleted = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/card/card-1",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(hard_deleted["ok"] == true);

  const auto emptied = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash?project_id=proj-1",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(emptied["ok"] == true);

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP trash routes validate parameters and hard-delete variants", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  const auto project_root = dir / "project_repo";
  create_project(db, "proj-1", project_root.string());

  holder::model::AiThread thread;
  thread.thread_id = "thread-1";
  thread.project_id = "proj-1";
  thread.title = "Thread";
  thread.created_at = 1;
  thread.updated_at = 1;
  holder::ai::AiThreadRepo thread_repo(db);
  thread_repo.create(thread);

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

  auto mk_card = [&](const std::string& id) {
    holder::test::create_card_fixture(card_store, id, "proj-1", id, "Hello", 10);
    http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::delete_,
        "/projects/proj-1/cards/" + id,
        nlohmann::json::object(),
        boost::beast::http::status::ok
    );
  };

  mk_card("card-a");
  mk_card("card-b");

  nlohmann::json msg_body = {
      {"message_id", "msg-a"},
      {"thread_id", "thread-1"},
      {"role", "user"},
      {"source", "manual"},
      {"content", "Hi"},
      {"created_at", 11}
  };
  http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::post,
      "/ai/messages",
      msg_body,
      boost::beast::http::status::created
  );
  http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/ai/messages/msg-a",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );

  auto bad_get = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/trash",
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(bad_get["error"]["code"] == "invalid_argument");

  auto bad_delete = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash",
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(bad_delete["error"]["code"] == "invalid_argument");

  auto not_found = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/card",
      nlohmann::json::object(),
      boost::beast::http::status::not_found
  );
  REQUIRE(not_found["error"]["code"] == "not_found");

  auto missing_id = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/card/",
      nlohmann::json::object(),
      boost::beast::http::status::not_found
  );
  REQUIRE(missing_id["error"]["code"] == "not_found");

  auto unknown_type = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/unknown/abc",
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(unknown_type["error"]["code"] == "invalid_argument");

  auto hard_delete_msg = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/ai_message/msg-a",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(hard_delete_msg["ok"] == true);

  auto card_only = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash?project_id=proj-1&type=card",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(card_only["ok"] == true);

  auto bad_card_delete = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/card/no-such-card",
      nlohmann::json::object(),
      boost::beast::http::status::not_found
  );
  REQUIRE(bad_card_delete["error"]["code"] == "not_found");

  create_project(db, "proj-2", (dir / "other_repo").string());
  holder::model::AiThread other_thread = thread;
  other_thread.thread_id = "thread-2";
  other_thread.project_id = "proj-2";
  thread_repo.create(other_thread);

  holder::test::create_card_fixture(card_store, "live-card", "proj-1", "Live", "Body", 12);
  holder::test::create_card_fixture(card_store, "other-card", "proj-2", "Other", "Body", 12);
  http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/projects/proj-2/cards/other-card",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );

  auto add_message = [&](const std::string& id, const std::string& thread_id) {
    nlohmann::json body = {
        {"message_id", id},
        {"thread_id", thread_id},
        {"role", "user"},
        {"source", "manual"},
        {"content", "Body"},
        {"created_at", 12}
    };
    http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::post,
        "/ai/messages",
        body,
        boost::beast::http::status::created
    );
  };
  add_message("live-message", "thread-1");
  add_message("other-message", "thread-2");
  http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/ai/messages/other-message",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );

  for (const auto& [type, id] :
       {std::pair<std::string, std::string>{"card", "other-card"},
        {"ai_message", "other-message"}}) {
    auto foreign = http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::delete_,
        "/projects/proj-1/trash/" + type + "/" + id,
        nlohmann::json::object(),
        boost::beast::http::status::not_found
    );
    REQUIRE(foreign["error"]["code"] == "not_found");
  }
  REQUIRE(holder::card::CardRepo(db).get("other-card")->deleted_at.has_value());
  REQUIRE(holder::ai::AiMessageRepo(db, &fts).get("other-message")->deleted_at.has_value());

  for (const auto& [type, id] :
       {std::pair<std::string, std::string>{"card", "live-card"}, {"ai_message", "live-message"}}) {
    auto live = http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::delete_,
        "/projects/proj-1/trash/" + type + "/" + id,
        nlohmann::json::object(),
        boost::beast::http::status::conflict
    );
    REQUIRE(live["error"]["code"] == "not_trashed");
  }
  auto legacy_live = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/ai_message/live-message",
      nlohmann::json::object(),
      boost::beast::http::status::conflict
  );
  REQUIRE(legacy_live["error"]["code"] == "not_trashed");
  REQUIRE(holder::card::CardRepo(db).get("live-card")->deleted_at == std::nullopt);
  REQUIRE(holder::ai::AiMessageRepo(db, &fts).get("live-message")->deleted_at == std::nullopt);

  for (const auto& [type, id] :
       {std::pair<std::string, std::string>{"card", "other-card"},
        {"ai_message", "other-message"}}) {
    auto removed = http_json_request(
        bound.bind,
        bound.port,
        token,
        boost::beast::http::verb::delete_,
        "/projects/proj-2/trash/" + type + "/" + id,
        nlohmann::json::object(),
        boost::beast::http::status::ok
    );
    REQUIRE(removed["ok"] == true);
  }
  REQUIRE_FALSE(holder::card::CardRepo(db).get("other-card").has_value());
  REQUIRE_FALSE(holder::ai::AiMessageRepo(db, &fts).get("other-message").has_value());

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP trash card hard-delete works without a daemon CardStore", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  holder::index::FtsIndexer fts(db);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, &fts);
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

  auto res = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash/card/abc",
      nlohmann::json::object(),
      boost::beast::http::status::not_found
  );
  REQUIRE(res["error"]["code"] == "not_found");

  std::raise(SIGTERM);
  server_thread.join();
}

TEST_CASE("HTTP trash top-level routes catch repository exceptions", "[http]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";

  auto db = open_db_with_schema(db_path);
  create_project(db, "proj-1", (dir / "project_repo").string());
  db.exec("DROP TABLE ai_messages;");

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

  auto list_fail = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::get,
      "/trash?project_id=proj-1&type=ai_message",
      nlohmann::json::object(),
      boost::beast::http::status::internal_server_error
  );
  REQUIRE(list_fail["ok"] == false);

  auto delete_fail = http_json_request(
      bound.bind,
      bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/trash?project_id=proj-1&type=ai_message",
      nlohmann::json::object(),
      boost::beast::http::status::internal_server_error
  );
  REQUIRE(delete_fail["ok"] == false);

  std::raise(SIGTERM);
  server_thread.join();
}
