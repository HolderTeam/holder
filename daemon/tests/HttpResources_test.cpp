#include "http_test_helpers.h"

#include "model/Card.h"
#include "platform/Paths.h"
#include <future>

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

} // namespace

TEST_CASE(
    "HTTP local Location import runs as a polled background job",
    "[http][resources][imports]"
) {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  create_project(db, "proj-1", (dir / "project_repo").string());

  holder::index::FtsIndexer setup_fts(db);
  holder::card::CardStore setup_cards(db, &setup_fts);
  holder::model::Card card;
  card.card_id = "card-1234";
  card.project_id = "proj-1";
  card.title = "Homework";
  card.created_at = 10;
  card.updated_at = 10;
  setup_cards.create(card, "Drop the source here.\n");

  const auto source = dir / "homework.pdf";
  std::ofstream(source, std::ios::binary) << "%PDF-1.7\nHolder test\n";
  const auto object_root = dir / "objects";
  const std::string token = "testtoken";
  RunningServer running(db, token);

  const auto location = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::post,
      "/locations",
      {{"project_id", "proj-1"},
       {"name", "Family Assets"},
       {"provider", "local_directory"},
       {"configuration", nlohmann::json::object()}},
      boost::beast::http::status::created
  );
  REQUIRE(location["data"]["bound"] == false);
  const auto location_id = location["data"]["location_id"].get<std::string>();

  const auto bound = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::put,
      "/locations/" + location_id + "/binding",
      {{"values", {{"root_path", object_root.string()}}}, {"preview", object_root.string()}},
      boost::beast::http::status::ok
  );
  REQUIRE(bound["data"]["bound"] == true);
  REQUIRE(bound["data"].dump().find("root_path") == std::string::npos);

  const auto import_and_wait = [&]() {
    const auto started = http_json_request(
        running.bound.bind,
        running.bound.port,
        token,
        boost::beast::http::verb::post,
        "/imports",
        {{"project_id", "proj-1"},
         {"card_id", "card-1234"},
         {"location_id", location_id},
         {"source_path", source.string()}},
        boost::beast::http::status::accepted
    );
    const auto job_id = started["data"]["job_id"].get<std::string>();

    nlohmann::json job;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    do {
      job = http_json_request(
          running.bound.bind,
          running.bound.port,
          token,
          boost::beast::http::verb::get,
          "/imports/" + job_id,
          nlohmann::json::object(),
          boost::beast::http::status::ok
      )["data"];
      if (job["status"] == "completed" || job["status"] == "failed") break;
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    } while (std::chrono::steady_clock::now() < deadline);
    return job;
  };

  const auto job = import_and_wait();

  REQUIRE(job["status"] == "completed");
  REQUIRE(job["resource_id"].is_string());
  REQUIRE(job["duplicate_reused"] == false);
  REQUIRE(job["link_created"] == true);
  REQUIRE(std::filesystem::exists(object_root));

  const auto duplicate_job = import_and_wait();
  REQUIRE(duplicate_job["status"] == "completed");
  REQUIRE(duplicate_job["resource_id"] == job["resource_id"]);
  REQUIRE(duplicate_job["asset_id"] == job["asset_id"]);
  REQUIRE(duplicate_job["duplicate_reused"] == true);
  REQUIRE(duplicate_job["link_created"] == false);

  for (const auto& [extra_id, title] : std::vector<std::pair<std::string, std::string>>{
           {"card-alpha", "Alpha"},
           {"card-homework", "Homework"}
       }) {
    holder::model::Card extra;
    extra.card_id = extra_id;
    extra.project_id = "proj-1";
    extra.title = title;
    extra.created_at = 20;
    extra.updated_at = 20;
    setup_cards.create(extra, title + "\n");
    const auto attached = http_json_request(
        running.bound.bind,
        running.bound.port,
        token,
        boost::beast::http::verb::put,
        "/projects/proj-1/cards/" + extra_id + "/resources/" + job["resource_id"].get<std::string>(),
        nlohmann::json::object(),
        boost::beast::http::status::ok
    );
    REQUIRE(attached["data"].size() == 1);
  }

  const auto resources = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/resources",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(resources["data"].size() == 1);
  REQUIRE(resources["data"][0]["assets"][0]["media_type"] == "application/pdf");
  // The import's card and the two attached here link to the resource.
  const auto backlinks = http_json_request(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/projects/proj-1/resources/" + job["resource_id"].get<std::string>() + "/backlinks",
      nlohmann::json::object(),
      boost::beast::http::status::ok
  );
  REQUIRE(backlinks["data"].size() == 3);
  for (const auto& link : backlinks["data"]) REQUIRE(link["kind"] == "attachment");

  const auto cache = holder::core::Paths::resolve("holder").cache_dir / "asset-cache";
  std::filesystem::create_directories(cache / "directory-entry");
  const auto stale = cache / "stale.tmp";
  const auto fresh = cache / "fresh.tmp";
  std::ofstream(stale) << "stale";
  std::ofstream(fresh) << "fresh";
  std::filesystem::last_write_time(
      stale,
      std::filesystem::file_time_type::clock::now() - std::chrono::hours(48)
  );
  const auto downloaded = holder::test::http_request_raw(
      running.bound.bind,
      running.bound.port,
      token,
      boost::beast::http::verb::get,
      "/resources/" + job["resource_id"].get<std::string>() + "/assets/" +
          job["asset_id"].get<std::string>() + "/content"
  );
  REQUIRE(downloaded.status == boost::beast::http::status::ok);
  REQUIRE(downloaded.body == "%PDF-1.7\nHolder test\n");
  CHECK_FALSE(std::filesystem::exists(stale));
  CHECK(std::filesystem::exists(fresh));
  CHECK(std::filesystem::is_directory(cache / "directory-entry"));
}
