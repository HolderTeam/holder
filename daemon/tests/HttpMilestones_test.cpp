#include "http_test_helpers.h"

#include "api/GeneratedRoutes.h"
#include "model/Card.h"
#include "model/Milestone.h"

using holder::test::create_project;
using holder::test::http_json_request;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;

namespace {

struct RunningServer {
  holder::index::FtsIndexer fts;
  holder::card::CardStore cards;
  holder::api::HttpServer server;
  holder::core::SignalHandler signals;
  holder::api::HttpServer::BoundInfo bound;
  std::unique_ptr<holder::test::HttpServerThreadGuard> thread;

  RunningServer(holder::platform::Db& db, const std::string& token)
      : fts(db),
        cards(db, &fts),
        server("127.0.0.1", 0, db, token, &cards, &fts) {
    try {
      bound = server.start();
    } catch (const std::exception& ex) {
      SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
    }
    thread = std::make_unique<holder::test::HttpServerThreadGuard>(server, signals);
    REQUIRE(holder::test::wait_for_http_listener(bound.bind, bound.port));
  }
};

holder::model::Card card(
    const std::string& card_id,
    const std::string& title,
    long long created_at,
    long long updated_at
) {
  holder::model::Card value;
  value.card_id = card_id;
  value.project_id = "proj-1";
  value.title = title;
  value.created_at = created_at;
  value.updated_at = updated_at;
  return value;
}

} // namespace

TEST_CASE(
    "HTTP milestones persist through cards and feed the project calendar",
    "[http][milestones]"
) {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  create_project(db, "proj-1", (dir / "project_repo").string());
  const std::string token = "testtoken";
  RunningServer running(db, token);

  running.cards.create(card("card-a", "Car records", 100, 100), "Car notes\n");
  running.cards.create(card("card-b", "House records", 150, 250), "House notes\n");

  const auto created = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::post,
      "/cards/card-a/milestones",
      {{"start_at", 200},
       {"end_at", 220},
       {"all_day", false},
       {"kind", "Service"},
       {"description", "Annual service"}},
      boost::beast::http::status::created
  );
  REQUIRE(created["data"]["card_id"] == "card-a");
  REQUIRE(created["data"]["start_at"] == 200);
  REQUIRE(created["data"]["end_at"] == 220);
  REQUIRE(created["data"]["kind"] == "Service");
  const auto milestone_id = created["data"]["milestone_id"].get<std::string>();
  const auto milestone_created_at = created["data"]["created_at"].get<long long>();

  const auto updated = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::patch,
      "/cards/card-a/milestones/" + milestone_id,
      {{"kind", "Renewal"}},
      boost::beast::http::status::ok
  );
  REQUIRE(updated["data"]["milestone_id"] == milestone_id);
  REQUIRE(updated["data"]["card_id"] == "card-a");
  REQUIRE(updated["data"]["start_at"] == 200);
  REQUIRE(updated["data"]["end_at"] == 220);
  REQUIRE(updated["data"]["all_day"] == false);
  REQUIRE(updated["data"]["kind"] == "Renewal");
  REQUIRE(updated["data"]["description"] == "Annual service");
  REQUIRE(updated["data"]["created_at"] == milestone_created_at);
  REQUIRE(updated["data"]["updated_at"].get<long long>() >= milestone_created_at);

  const auto listed = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/cards/card-a/milestones",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(listed["data"].size() == 1);
  REQUIRE(listed["data"][0]["milestone_id"] == milestone_id);
  REQUIRE(listed["data"][0]["kind"] == "Renewal");

  const auto calendar = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/calendar?project_id=proj-1&from=50&to=300",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(calendar["data"]["milestones"].size() == 1);
  REQUIRE(calendar["data"]["milestones"][0]["card_title"] == "Car records");
  REQUIRE(calendar["data"]["created_cards"].size() == 2);
  REQUIRE(calendar["data"]["updated_cards"].size() == 1);
  REQUIRE(calendar["data"]["updated_cards"][0]["card_id"] == "card-b");

  const auto removed = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::delete_,
      "/cards/card-a/milestones/" + milestone_id,
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(removed["data"]["removed"] == true);

  const auto after_delete = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/cards/card-a/milestones",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(after_delete["data"].empty());
}

TEST_CASE("HTTP labelled milestones preserve undated and repeated dates", "[http][milestones]") {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  create_project(db, "proj-1", (dir / "project_repo").string());
  const std::string token = "testtoken";
  RunningServer running(db, token);
  running.cards.create(card("card-a", "Tasks", 100, 100), "Notes\n");

  const auto undated = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::post,
      "/cards/card-a/milestones-with-dates",
      {{"dates", nlohmann::json::array()}, {"kind", "task"}},
      boost::beast::http::status::created
  );
  REQUIRE(undated["data"]["dates"].empty());
  const auto undated_id = undated["data"]["milestone_id"].get<std::string>();
  const auto incompatible_update = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::patch,
      "/cards/card-a/milestones/" + undated_id, {{"kind", "changed"}},
      boost::beast::http::status::bad_request
  );
  REQUIRE(incompatible_update["ok"] == false);

  const nlohmann::json dates = nlohmann::json::array({
      {{"role", "deadline"}, {"date", "2026-06-01"}},
      {{"role", "deadline"}, {"date", "2026-06-03"}},
      {{"role", "occurred"}, {"datetime", "2026-06-02T12:30:00+02:30"}},
  });
  const auto created = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::post,
      "/cards/card-a/milestones-with-dates",
      {{"dates", dates}, {"description", "Track work"}},
      boost::beast::http::status::created
  );
  REQUIRE(created["data"]["dates"] == dates);
  const auto milestone_id = created["data"]["milestone_id"].get<std::string>();

  const auto full = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::get,
      "/cards/card-a/milestones-with-dates", nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(full["data"].size() == 2);
  bool saw_undated = false;
  bool saw_dated = false;
  for (const auto& milestone : full["data"]) {
    if (milestone["milestone_id"] == undated_id) {
      REQUIRE(milestone["dates"].empty());
      REQUIRE(milestone["kind"] == "task");
      saw_undated = true;
    }
    if (milestone["milestone_id"] == milestone_id) {
      REQUIRE(milestone["dates"] == dates);
      saw_dated = true;
    }
  }
  REQUIRE(saw_undated);
  REQUIRE(saw_dated);

  const auto scheduled = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::get,
      "/cards/card-a/milestones", nlohmann::json::object(), boost::beast::http::status::ok
  );
  REQUIRE(scheduled["data"].empty());

  const auto day_matches = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::get,
      "/projects/proj-1/milestones-with-dates?role=deadline&value_type=date&from=2026-06-01&to=2026-06-03",
      nlohmann::json::object(), boost::beast::http::status::ok
  );
  REQUIRE(day_matches["data"].size() == 1);
  REQUIRE(day_matches["data"][0]["milestone_id"] == milestone_id);
  REQUIRE(day_matches["data"][0]["dates"] == dates);

  const auto occurred_at = holder::model::milestone_datetime_epoch("2026-06-02T12:30:00+02:30");
  const auto instant_matches = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::get,
      "/projects/proj-1/milestones-with-dates?role=occurred&value_type=datetime&from=" +
          std::to_string(occurred_at) + "&to=" + std::to_string(occurred_at),
      nlohmann::json::object(), boost::beast::http::status::ok
  );
  REQUIRE(instant_matches["data"].size() == 1);
  REQUIRE(instant_matches["data"][0]["milestone_id"] == milestone_id);

  const nlohmann::json revised_dates = nlohmann::json::array({
      {{"role", "deadline"}, {"date", "2026-06-05"}},
      {{"role", "occurred"}, {"datetime", "2026-06-02T12:30:00+02:30"}},
      {{"role", "resolved"}, {"datetime", "2026-06-04T09:00:00Z"}},
  });
  const auto updated = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::patch,
      "/cards/card-a/milestones-with-dates/" + milestone_id,
      {{"dates", revised_dates}}, boost::beast::http::status::ok
  );
  REQUIRE(updated["data"]["dates"] == revised_dates);
  REQUIRE(updated["data"]["description"] == "Track work");

  const auto invalid = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::post,
      "/cards/card-a/milestones-with-dates",
      {{"dates", nlohmann::json::array({{{"role", "deadline"}, {"date", "2026-06-01"},
                                          {"datetime", "2026-06-01T00:00:00Z"}}})}},
      boost::beast::http::status::bad_request
  );
  REQUIRE(invalid["ok"] == false);

  const auto removed = http_json_request(
      running.bound.bind, running.bound.port, token, boost::beast::http::verb::delete_,
      "/cards/card-a/milestones-with-dates/" + milestone_id, nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(removed["data"]["removed"] == true);
}

TEST_CASE("HTTP milestone and calendar routes validate inputs", "[http][milestones]") {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  create_project(db, "proj-1", (dir / "project_repo").string());
  const std::string token = "testtoken";
  RunningServer running(db, token);
  running.cards.create(card("card-a", "Card", 100, 100), "Notes\n");

  const auto missing_range = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/calendar?project_id=proj-1",
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(missing_range["error"]["code"] == "bad_request");

  const auto reversed_range = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/calendar?project_id=proj-1&from=20&to=10",
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(reversed_range["error"]["code"] == "bad_request");

  const auto bad_end = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::post,
      "/cards/card-a/milestones",
      {{"start_at", 200}, {"end_at", 100}},
      boost::beast::http::status::bad_request
  );
  REQUIRE(bad_end["error"]["code"] == "bad_request");

  const auto created = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::post,
      "/cards/card-a/milestones",
      {{"start_at", 200}, {"end_at", 220}, {"kind", "Review"}},
      boost::beast::http::status::created
  );
  const auto milestone_id = created["data"]["milestone_id"].get<std::string>();

  const auto invalid_update = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::patch,
      "/cards/card-a/milestones/" + milestone_id,
      {{"start_at", 221}},
      boost::beast::http::status::bad_request
  );
  REQUIRE(invalid_update["error"]["code"] == "bad_request");

  const auto empty_update = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::patch,
      "/cards/card-a/milestones/" + milestone_id,
      nlohmann::json::object(),
      boost::beast::http::status::bad_request
  );
  REQUIRE(empty_update["error"]["code"] == "bad_request");

  const auto unknown_field = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::patch,
      "/cards/card-a/milestones/" + milestone_id,
      {{"title", "Not a milestone field"}},
      boost::beast::http::status::bad_request
  );
  REQUIRE(unknown_field["error"]["code"] == "bad_request");

  running.cards.create(card("card-b", "Other card", 100, 100), "Other\n");
  const auto other = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::post,
      "/cards/card-b/milestones",
      {{"start_at", 300}},
      boost::beast::http::status::created
  );
  const auto other_milestone_id = other["data"]["milestone_id"].get<std::string>();
  const auto wrong_card = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::patch,
      "/cards/card-a/milestones/" + other_milestone_id,
      {{"kind", "Wrong card"}},
      boost::beast::http::status::not_found
  );
  REQUIRE(wrong_card["error"]["code"] == "not_found");

  const auto missing_milestone = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::patch,
      "/cards/card-a/milestones/00000000-0000-4000-8000-000000000000",
      {{"kind", "Missing"}},
      boost::beast::http::status::not_found
  );
  REQUIRE(missing_milestone["error"]["code"] == "not_found");

  const auto missing_card = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/cards/missing/milestones",
      nlohmann::json::object(),
      boost::beast::http::status::not_found
  );
  REQUIRE(missing_card["error"]["code"] == "not_found");
}

#include "api/routes/MilestoneRoutes.h"

TEST_CASE("Milestone routes validate paths and unavailable card services", "[milestones][routes]") {
  namespace http = boost::beast::http;
  const auto root = make_temp_dir();
  auto db = open_db_with_schema(root / "holder.db");
  create_project(db, "proj-1", (root / "project").string());
  holder::card::CardStore cards(db, nullptr);
  cards.create(card("card-one", "Card", 1, 1), "Card\n");
  auto call = [&](http::verb method,
                  const std::string& path,
                  holder::card::CardStore* store,
                  const std::string& body,
                  const std::map<std::string, std::string>& params = {}) {
    http::request<http::string_body> request{method, path, 11};
    request.body() = body;
    http::response<http::string_body> response;
    const bool handled = holder::api::routes::handle_milestone_routes(
        path,
        request,
        response,
        db,
        store,
        [] {
          return "milestone";
        },
        [&](const std::string& key) {
          auto it = params.find(key);
          return it == params.end() ? "" : it->second;
        }
    );
    return std::pair{handled, response.result()};
  };
  for (const std::string path :
       {"/cards",
        "/cards/id",
        "/cards/id/other",
        "/cards/id/milestones-invalid",
        "/cards/id/milestones/",
        "/cards/id/milestones/a/b"})
    CHECK_FALSE(call(http::verb::get, path, &cards, "{}").first);
  CHECK(
      call(http::verb::get, "/cards//milestones", &cards, "{}").second == http::status::bad_request
  );
  CHECK_FALSE(call(http::verb::get, "/calendar", &cards, "").first);
  CHECK_FALSE(
      call(http::verb::get, "/projects/proj-1/milestones-with-dates", &cards, "").first
  );
  for (auto method : {http::verb::post, http::verb::patch, http::verb::delete_}) {
    const auto path = method == http::verb::post ? "/cards/card-one/milestones"
                                                 : "/cards/card-one/milestones/id";
    CHECK(call(method, path, nullptr, "{}").second == http::status::not_implemented);
  }
  CHECK_FALSE(call(http::verb::put, "/cards/card-one/milestones", &cards, "{}").first);
  for (const std::string body :
       {"{}",
        R"({"start_at":"bad"})",
        R"({"start_at":1,"end_at":"bad"})",
        R"({"start_at":1,"all_day":12})"})
    CHECK(
        call(http::verb::post, "/cards/card-one/milestones", &cards, body).second ==
        http::status::bad_request
    );
  for (const std::string body :
       {"not-json",
        "[]",
        "{}",
        R"({"unknown":true})",
        R"({"start_at":"bad"})",
        R"({"end_at":"bad"})",
        R"({"all_day":12})",
        R"({"kind":12})"})
    CHECK(
        call(http::verb::patch, "/cards/card-one/milestones/id", &cards, body).second ==
        http::status::bad_request
    );
  CHECK(
      call(http::verb::patch, "/cards/card-one/milestones/id", &cards, R"({"kind":"new"})")
          .second == http::status::not_found
  );
}

TEST_CASE("Generated calendar route keeps its range, filters and errors", "[milestones][calendar]") {
  namespace http = boost::beast::http;
  const auto root = make_temp_dir();
  auto db = open_db_with_schema(root / "holder.db");
  create_project(db, "proj-1", (root / "project").string());
  holder::card::CardStore cards(db, nullptr);
  auto calendar = [&](const std::string& target) {
    http::request<http::string_body> request{http::verb::get, target, 11};
    http::response<http::string_body> response;
    REQUIRE(holder::api::dispatch_generated_routes(request, response, db));
    return std::pair{response.result(), nlohmann::json::parse(response.body())};
  };
  int next_milestone = 0;
  auto add_milestone = [&](const std::string& card_id,
                           long long at,
                           std::vector<holder::model::MilestoneDate> dates) {
    holder::model::Milestone milestone;
    milestone.milestone_id = "milestone-" + std::to_string(++next_milestone);
    milestone.project_id = "proj-1";
    milestone.card_id = card_id;
    milestone.dates = std::move(dates);
    milestone.created_at = at;
    milestone.updated_at = at;
    return cards.add_milestone(milestone, at).milestone_id;
  };
  auto start = [](long long at) {
    return holder::model::milestone_date_from_legacy("start", at, false);
  };

  cards.create(card("card-a", "Live", 100, 100), "Live\n");
  cards.create(card("card-b", "Edited", 50, 300), "Edited\n");
  cards.create(card("card-c", "Trashed", 200, 200), "Trashed\n");
  // Milestones keep each card's updated_at, so only card-b counts as updated.
  const auto first = add_milestone("card-a", 100, {start(100)});
  const auto last = add_milestone("card-a", 100, {start(300)});
  add_milestone("card-a", 100, {start(99)});
  add_milestone("card-a", 100, {start(301)});
  add_milestone("card-a", 100, {});
  add_milestone("card-a", 100, {start(150), start(160)});
  add_milestone("card-c", 200, {start(200)});
  cards.trash("card-c", 400);

  SECTION("bounds are inclusive; trashed cards and milestones without one start are left out") {
    const auto [status, body] = calendar("/calendar?project_id=proj-1&from=100&to=300");
    REQUIRE(status == http::status::ok);
    REQUIRE(body["ok"] == true);
    const auto& data = body["data"];
    CHECK(data["project_id"] == "proj-1");
    CHECK(data["from"] == 100);
    CHECK(data["to"] == 300);
    REQUIRE(data["milestones"].size() == 2);
    CHECK(data["milestones"][0] == nlohmann::json{
        {"milestone_id", first},
        {"card_id", "card-a"},
        {"start_at", 100},
        {"end_at", nullptr},
        {"all_day", false},
        {"kind", nullptr},
        {"description", nullptr},
        {"created_at", 100},
        {"updated_at", 100},
        {"card_title", "Live"},
    });
    CHECK(data["milestones"][1]["milestone_id"] == last);
    REQUIRE(data["created_cards"].size() == 1);
    CHECK(data["created_cards"][0] == nlohmann::json{
        {"card_id", "card-a"}, {"title", "Live"}, {"created_at", 100}, {"updated_at", 100}});
    REQUIRE(data["updated_cards"].size() == 1);
    CHECK(data["updated_cards"][0]["card_id"] == "card-b");
  }

  SECTION("a single-second range matches only that second") {
    const auto [status, body] = calendar("/calendar?project_id=proj-1&from=300&to=300");
    REQUIRE(status == http::status::ok);
    CHECK(body["data"]["milestones"].size() == 1);
    CHECK(body["data"]["created_cards"].empty());
    CHECK(body["data"]["updated_cards"].size() == 1);
  }

  SECTION("invalid queries are bad requests") {
    for (const std::string query :
         {"",
          "?from=0&to=1",
          "?project_id=&from=0&to=1",
          "?project_id=proj-1&to=1",
          "?project_id=proj-1&from=0",
          "?project_id=proj-1&from=&to=1",
          "?project_id=proj-1&from=0&to=bad",
          "?project_id=proj-1&from=0&to=11trailing",
          "?project_id=proj-1&from=0&to=99999999999999999999",
          "?project_id=proj-1&from=20&to=10"}) {
      CAPTURE(query);
      const auto [status, body] = calendar("/calendar" + query);
      CHECK(status == http::status::bad_request);
      CHECK(body["ok"] == false);
      CHECK(body["error"]["code"] == "bad_request");
    }
    CHECK(calendar("/calendar?project_id=proj-1&from=0").second["error"]["message"] ==
          "missing query parameter: to");
    CHECK(calendar("/calendar?project_id=proj-1&from=20&to=10").second["error"]["message"] ==
          "from must not be after to.");
  }

  SECTION("an unknown project is not found") {
    const auto [status, body] = calendar("/calendar?project_id=missing&from=0&to=1");
    CHECK(status == http::status::not_found);
    CHECK(body["error"]["code"] == "not_found");
    CHECK(body["error"]["message"] == "Project not found.");
  }

  SECTION("a storage failure is an internal error") {
    db.exec("DROP TABLE milestones");
    CHECK(calendar("/calendar?project_id=proj-1&from=0&to=1").first ==
          http::status::internal_server_error);
  }
}

TEST_CASE(
    "Generated project milestone query keeps its ranges, filters and errors",
    "[milestones][milestones-with-dates]"
) {
  namespace http = boost::beast::http;
  const auto root = make_temp_dir();
  auto db = open_db_with_schema(root / "holder.db");
  create_project(db, "proj-1", (root / "project").string());
  holder::card::CardStore cards(db, nullptr);
  auto query = [&](const std::string& target) {
    http::request<http::string_body> request{http::verb::get, target, 11};
    http::response<http::string_body> response;
    REQUIRE(holder::api::dispatch_generated_routes(request, response, db));
    return std::pair{response.result(), nlohmann::json::parse(response.body())};
  };
  auto add_milestone = [&](const std::string& milestone_id,
                           const std::string& card_id,
                           std::vector<holder::model::MilestoneDate> dates) {
    holder::model::Milestone milestone;
    milestone.milestone_id = milestone_id;
    milestone.project_id = "proj-1";
    milestone.card_id = card_id;
    milestone.dates = std::move(dates);
    milestone.created_at = 10;
    milestone.updated_at = 10;
    cards.add_milestone(milestone, 10);
  };

  cards.create(card("card-a", "Live", 10, 10), "Live\n");
  cards.create(card("card-b", "Trashed", 10, 10), "Trashed\n");
  const std::vector<holder::model::MilestoneDate> dates{
      {"deadline", std::string("2026-06-01"), std::nullopt},
      {"occurred", std::nullopt, std::string("1970-01-01T00:01:40Z")},
  };
  add_milestone("milestone-1", "card-a", dates);
  add_milestone("milestone-2", "card-a", {{"deadline", std::string("2026-06-04"), std::nullopt}});
  add_milestone("milestone-3", "card-b", dates);
  cards.trash("card-b", 20);
  const std::string base = "/projects/proj-1/milestones-with-dates";

  SECTION("day and instant bounds are inclusive and trashed cards are left out") {
    const auto [day_status, days] =
        query(base + "?role=deadline&value_type=date&from=2026-05-01&to=2026-06-01");
    REQUIRE(day_status == http::status::ok);
    REQUIRE(days["ok"] == true);
    REQUIRE(days["data"].size() == 1);
    CHECK(days["data"][0] == nlohmann::json{
        {"milestone_id", "milestone-1"},
        {"card_id", "card-a"},
        {"dates",
         {{{"role", "deadline"}, {"date", "2026-06-01"}},
          {{"role", "occurred"}, {"datetime", "1970-01-01T00:01:40Z"}}}},
        {"kind", nullptr},
        {"description", nullptr},
        {"created_at", 10},
        {"updated_at", 10},
    });

    const auto [instant_status, instants] =
        query(base + "?role=occurred&value_type=datetime&from=100&to=100");
    REQUIRE(instant_status == http::status::ok);
    REQUIRE(instants["data"].size() == 1);
    CHECK(instants["data"][0]["milestone_id"] == "milestone-1");

    CHECK(query(base + "?role=occurred&value_type=datetime&from=101&to=200")
              .second["data"]
              .empty());
  }

  SECTION("invalid queries are bad requests") {
    for (const std::string parameters :
         {"?value_type=date&from=2026-06-01&to=2026-06-01",
          "?role=&value_type=date&from=2026-06-01&to=2026-06-01",
          "?role=deadline&from=2026-06-01&to=2026-06-01",
          "?role=deadline&value_type=week&from=2026-06-01&to=2026-06-01",
          "?role=deadline&value_type=date&to=2026-06-01",
          "?role=deadline&value_type=date&from=2026-6-1&to=2026-06-01",
          "?role=deadline&value_type=date&from=2026-02-30&to=2026-06-01",
          "?role=deadline&value_type=date&from=2026-06-02&to=2026-06-01",
          "?role=occurred&value_type=datetime&from=&to=1",
          "?role=occurred&value_type=datetime&from=0&to=bad",
          "?role=occurred&value_type=datetime&from=0&to=1trailing",
          "?role=occurred&value_type=datetime&from=2&to=1"}) {
      CAPTURE(parameters);
      const auto [status, body] = query(base + parameters);
      CHECK(status == http::status::bad_request);
      CHECK(body["ok"] == false);
      CHECK(body["error"]["code"] == "bad_request");
    }
    CHECK(query(base + "?role=occurred&value_type=datetime&from=&to=1")
              .second["error"]["message"] == "Missing from.");
    CHECK(query(base + "?role=occurred&value_type=datetime&from=0&to=bad")
              .second["error"]["message"] == "to must be an epoch-second integer.");
    CHECK(query(base + "?role=deadline&value_type=date&from=2026-6-1&to=2026-06-01")
              .second["error"]["message"] == "from and to must use YYYY-MM-DD for date queries.");
    CHECK(query(base + "?role=deadline&value_type=date&from=2026-06-02&to=2026-06-01")
              .second["error"]["message"] == "from must not be after to.");
  }

  SECTION("an unknown project is not found") {
    const auto [status, body] = query(
        "/projects/missing/milestones-with-dates?role=deadline&value_type=date&from=2026-06-01&to=2026-06-01"
    );
    CHECK(status == http::status::not_found);
    CHECK(body["error"]["code"] == "not_found");
    CHECK(body["error"]["message"] == "Project not found.");
  }
}
