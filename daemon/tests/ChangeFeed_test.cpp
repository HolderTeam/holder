#include "api/support/ChangeFeed.h"
#include "api/support/EventService.h"
#include "http_test_helpers.h"

#include <sqlite3.h>

using namespace holder::api::support;

TEST_CASE("Change feed observes committed background writes and same-second edits", "[events]") {
  const auto path = holder::test::make_temp_dir() / "holder.db";
  auto writer = holder::test::open_db_with_schema(path);
  writer.exec(
      "INSERT INTO projects(project_id,name,root_path,created_at,updated_at) VALUES('p','P','/missing-holder-event-test',1,1)"
  );
  holder::platform::Db observer;
  observer.open(path);
  EventJournal journal;
  ChangeFeed feed(observer, journal);
  REQUIRE(feed.revisions()["p"].is_null());
  writer.exec("BEGIN");
  writer.exec(
      "INSERT INTO cards(card_id,project_id,title,rel_path,created_at,updated_at) VALUES('c','p','C','c.md',1,1)"
  );
  feed.poll();
  REQUIRE(journal.read().events.empty());
  writer.exec("COMMIT");
  feed.poll();
  auto batch = journal.read();
  REQUIRE(batch.events.size() == 1);
  REQUIRE(batch.events[0].name == "card.changed");
  REQUIRE(batch.events[0].data["entity_id"] == "c");
  REQUIRE(batch.events[0].data["git_revision"].is_null());
  const auto before = batch.cursor;
  writer.exec("UPDATE cards SET title='Edited' WHERE card_id='c'");
  feed.poll();
  REQUIRE(journal.read(before).events.size() == 1);
  auto cursor = journal.cursor();
  writer.exec("BEGIN; UPDATE cards SET title='Rolled back' WHERE card_id='c'; ROLLBACK;");
  feed.poll();
  REQUIRE(journal.read(cursor).events.empty());
  writer.exec("INSERT INTO card_tags(card_id,project_id,tag,created_at) VALUES('c','p','new',1)");
  feed.poll();
  REQUIRE(journal.read(cursor).events.size() == 1);
  cursor = journal.cursor();
  writer.exec("DELETE FROM cards WHERE card_id='c'");
  feed.poll();
  REQUIRE(journal.read(cursor).events[0].data["deleted"] == true);
}

TEST_CASE("Change feed tracks Git-only commits without a database mutation", "[events]") {
  const auto dir = holder::test::make_temp_dir();
  holder::git::GitRepo git;
  git.open_or_init(dir / "project");
  git.write_file("card.md", "first");
  git.stage_path("card.md");
  git.commit("first");
  const auto first = git.head_oid();
  auto writer = holder::test::open_db_with_schema(dir / "holder.db");
  holder::model::Project project;
  project.project_id = "p";
  project.name = "P";
  project.root_path = (dir / "project").string();
  project.created_at = project.updated_at = 1;
  holder::project::ProjectRepo(writer).create(project);
  holder::platform::Db observer;
  observer.open(dir / "holder.db");
  EventJournal journal;
  ChangeFeed feed(observer, journal);
  REQUIRE(feed.revisions()["p"] == first.value());
  git.write_file("card.md", "second");
  git.stage_path("card.md");
  git.commit("second");
  feed.poll();
  auto events = journal.read().events;
  REQUIRE(events.size() == 1);
  REQUIRE(events[0].name == "project.changed");
  REQUIRE(events[0].data["git_revision"] == git.head_oid().value());
  REQUIRE(events[0].data["deleted"] == false);
  feed.poll();
  REQUIRE(journal.read().events.size() == 1);
}

TEST_CASE("Change feed can retry a snapshot after a required table is restored", "[events]") {
  const auto path = holder::test::make_temp_dir() / "holder.db";
  auto writer = holder::test::open_db_with_schema(path);
  holder::test::create_project(writer, "p", (path.parent_path() / "project").string());
  holder::platform::Db observer;
  observer.open(path);
  EventJournal journal;
  ChangeFeed feed(observer, journal);
  const auto cursor = journal.cursor();

  writer.exec("ALTER TABLE cards RENAME TO hidden_cards");
  REQUIRE_THROWS(feed.poll());
  CHECK(journal.read(cursor).events.empty());

  writer.exec("ALTER TABLE hidden_cards RENAME TO cards");
  writer.exec("INSERT INTO cards(card_id,project_id,title,rel_path,created_at,updated_at) "
              "VALUES('c','p','C','c.md',1,1)");
  REQUIRE_NOTHROW(feed.poll());
  const auto batch = journal.read(cursor);
  REQUIRE(batch.events.size() == 1);
  CHECK(batch.events.front().name == "card.changed");
  CHECK(batch.events.front().data.at("entity_id") == "c");
}

TEST_CASE("Event checkpoints encode project IDs as one history URL segment", "[events]") {
  const auto path = holder::test::make_temp_dir() / "holder.db";
  auto db = holder::test::open_db_with_schema(path);
  const std::string id = "a b/c%";
  holder::test::create_project(db, id, (path.parent_path() / "project").string());
  EventService service(path);
  const auto checkpoint = service.current_checkpoint(id);
  REQUIRE(checkpoint);
  CHECK(checkpoint->history_urls.at(id) == "/projects/a%20b%2Fc%25/history");
  CHECK_FALSE(checkpoint->git_revisions.at(id));
  CHECK_THROWS_AS(service.subscribe(id, "", "invalid-cursor"), std::invalid_argument);
}

TEST_CASE(
    "Change feed rejects a suppressed data-version probe without damaging its connection",
    "[events][feed-boundaries]"
) {
  const auto path = holder::test::make_temp_dir() / "holder.db";
  auto db = holder::test::open_db_with_schema(path);
  EventJournal journal;
  REQUIRE(
      sqlite3_set_authorizer(
          db.handle(),
          [](void*, int action, const char* name, const char*, const char*, const char*) {
            return action == SQLITE_PRAGMA && name && std::string_view(name) == "data_version"
                       ? SQLITE_IGNORE
                       : SQLITE_OK;
          },
          nullptr
      ) == SQLITE_OK
  );
  CHECK_THROWS_AS(ChangeFeed(db, journal), std::runtime_error);
  REQUIRE(sqlite3_set_authorizer(db.handle(), nullptr, nullptr) == SQLITE_OK);
  CHECK_NOTHROW(ChangeFeed(db, journal));
}

TEST_CASE(
    "Event service replays an observer failure after recovery with subscriber-specific data",
    "[events][feed-boundaries]"
) {
  const auto path = holder::test::make_temp_dir() / "holder.db";
  auto db = holder::test::open_db_with_schema(path);
  holder::test::create_project(db, "p", (path.parent_path() / "project").string());
  EventService service(path);
  const auto before = service.current_checkpoint("p");
  REQUIRE(before);
  const std::string revision(40, 'a');
  // Hold this subscription's source until the observer has failed and recovered.
  auto serve = service.subscribe("p", revision, before->cursor);
  db.exec("ALTER TABLE cards RENAME TO hidden_cards");
  auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
  while (service.current_checkpoint("p") && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  REQUIRE_FALSE(service.current_checkpoint("p"));
  db.exec("ALTER TABLE hidden_cards RENAME TO cards");
  deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
  while (!service.current_checkpoint("p") && std::chrono::steady_clock::now() < deadline)
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  REQUIRE(service.current_checkpoint("p"));
  boost::asio::io_context io;
  using tcp = boost::asio::ip::tcp;
  tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
  tcp::socket client(io);
  client.connect(acceptor.local_endpoint());
  auto socket = acceptor.accept();
  boost::beast::http::response<boost::beast::http::string_body> response;
  REQUIRE(serve(socket, response));
  io.run_for(std::chrono::seconds(3));
  REQUIRE(service.streams()->open_count() == 0);
  boost::beast::flat_buffer buffer;
  boost::beast::http::read(client, buffer, response);
  INFO(response.body());
  CHECK(response.body().find("event: resync_required") != std::string::npos);
  CHECK(response.body().find("observer_unavailable") != std::string::npos);
  CHECK(response.body().find(revision) != std::string::npos);
  CHECK(response.body().find("/projects/p/history") != std::string::npos);
}
