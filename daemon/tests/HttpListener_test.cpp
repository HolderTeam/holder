#include "http_test_helpers.h"

#include "api/Listener.h"
#include "card/CardStore.h"
#include "index/FtsIndexer.h"
#include "llm/RunnerRegistry.h"
#include "platform/Signal.h"
#include "privacy/SecretStore.h"

#include <atomic>
#include <boost/asio.hpp>
#include <cstddef>
#include <functional>
#include <future>
#include <optional>
#include <string>
#include <thread>
#include <utility>

using holder::test::create_project;
using holder::test::make_temp_dir;
using holder::test::open_db_with_schema;
using holder::test::wait_for_http_listener;

namespace {

// A secret store whose reads and removals run the test's hooks, so that tests can hold real
// routes open: GET /locations?project_id=P reads P's preferred location (foreground lane), and
// DELETE /ai/providers/credentials/P removes P's key (background lane).
class HookedSecretStore final : public holder::privacy::SecretStore {
 public:
  std::function<std::optional<holder::privacy::StoredSecret>(const std::string& account)> on_get;
  std::function<void(const std::string& account)> on_remove;

  std::optional<holder::privacy::StoredSecret> get(const std::string&,
                                                   const std::string& account) const override {
    return on_get ? on_get(account) : std::nullopt;
  }
  std::vector<holder::privacy::SecretMetadata> list(const std::string&) const override {
    return {};
  }
  void set(const std::string&, const std::string&, const std::string&, const std::string&,
           long long, long long) override {}
  void remove(const std::string&, const std::string& account) override {
    if (on_remove) on_remove(account);
  }
};

boost::asio::ip::tcp::socket connect_test_socket(
    boost::asio::io_context& ioc,
    const std::string& bind,
    unsigned short port
) {
  using tcp = boost::asio::ip::tcp;
  tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(bind, std::to_string(port));
  tcp::socket socket(ioc);
  boost::asio::connect(socket, endpoints);
  return socket;
}

void write_raw_request(boost::asio::ip::tcp::socket& socket, const std::string& request) {
  boost::asio::write(socket, boost::asio::buffer(request));
}

/** Ensures listener.stop() and the runner thread's join() always run, even when a REQUIRE
 * fails partway through a test and unwinds the stack before reaching the test's own trailing
 * stop()/join() calls. Without this, std::thread's destructor calls std::terminate()
 * unconditionally on a still-joinable thread -- so an ordinary assertion failure crashed the
 * whole test process (visible as "Subprocess aborted" from CTest) instead of reporting a clean
 * failure. listener.stop() is idempotent (guarded by compare_exchange_strong), so a test that
 * still calls it explicitly before this guard destructs is unaffected. */
class ListenerRunGuard {
 public:
  ListenerRunGuard(holder::api::Listener& listener, std::thread thread)
      : listener_(listener),
        thread_(std::move(thread)) {}

  ListenerRunGuard(const ListenerRunGuard&) = delete;
  ListenerRunGuard& operator=(const ListenerRunGuard&) = delete;

  ~ListenerRunGuard() {
    listener_.stop();
    if (thread_.joinable()) thread_.join();
  }

 private:
  holder::api::Listener& listener_;
  std::thread thread_;
};

} // namespace

TEST_CASE("Listener start fails when port already in use", "[listener]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);

  const std::string token = "testtoken";
  holder::api::HttpServer server1("127.0.0.1", 0, db, token, nullptr, nullptr);
  holder::api::HttpServer::BoundInfo bound;
  try {
    bound = server1.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::api::HttpServer server2("127.0.0.1", bound.port, db, token, nullptr, nullptr);
  REQUIRE_THROWS(server2.start());
}

TEST_CASE("HttpServer run throws if start was not called", "[listener]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);

  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr);
  holder::core::SignalHandler signals;
  REQUIRE_THROWS(server.run(signals));
}

TEST_CASE("Slow background route does not block foreground route", "[listener]") {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  std::atomic<bool> slow_started{false};

  HookedSecretStore secrets;
  secrets.on_remove = [&slow_started](const std::string&) {
    slow_started.store(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  };

  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr,
      &secrets
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread listener_thread([&listener, &signals]() {
    listener.run(signals);
  });
  ListenerRunGuard listener_guard(listener, std::move(listener_thread));
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  auto slow_future = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::delete_,
        "/ai/providers/credentials/slow"
    );
  });

  for (int i = 0; i < 50 && !slow_started.load(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(slow_started.load());

  const auto fast_started = std::chrono::steady_clock::now();
  const auto fast = holder::test::http_request_raw(
      bound.bind,
      bound.port,
      token,
      http::verb::get,
      "/locations?project_id=fast"
  );
  const auto fast_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - fast_started
  )
                                   .count();

  REQUIRE(fast.status == http::status::ok);
  REQUIRE(fast_elapsed_ms < 200);

  const auto slow = slow_future.get();
  REQUIRE(slow.status == http::status::ok);

  // listener_guard's destructor stops the listener and joins its thread.
}

TEST_CASE("Slow background route does not block save lane route", "[listener]") {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  std::atomic<bool> slow_started{false};

  HookedSecretStore secrets;
  secrets.on_remove = [&slow_started](const std::string&) {
    slow_started.store(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  };

  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr,
      &secrets
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread listener_thread([&listener, &signals]() {
    listener.run(signals);
  });
  ListenerRunGuard listener_guard(listener, std::move(listener_thread));
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  auto slow_future = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::delete_,
        "/ai/providers/credentials/slow"
    );
  });

  for (int i = 0; i < 50 && !slow_started.load(); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(slow_started.load());

  const auto save_started = std::chrono::steady_clock::now();
  const auto saved = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      http::verb::patch,
      "/cards/save-test",
      // Without a card store the save is refused at once; only its lane matters here.
      nlohmann::json{{"updated_at", 1}, {"content", "x"}},
      http::status::not_implemented
  );
  const auto save_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - save_started
  )
                                   .count();

  REQUIRE(saved["ok"] == false);
  REQUIRE(save_elapsed_ms < 200);

  const auto slow = slow_future.get();
  REQUIRE(slow.status == http::status::ok);

  // listener_guard's destructor stops the listener and joins its thread.
}

TEST_CASE("Listener worker-owned DB handles support concurrent mixed request load", "[listener]") {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  create_project(db, "proj-1", (dir / "project").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  holder::test::EnvGuard fake_runner_env("HOLDER_MODEL_RUNNER_FAKE", "1");
  holder::llm::RunnerRegistry runner_registry(&db, nullptr);
  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      &card_store,
      &fts,
      nullptr,
      nullptr,
      &runner_registry
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread listener_thread([&listener, &signals]() {
    listener.run(signals);
  });
  ListenerRunGuard listener_guard(listener, std::move(listener_thread));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  for (int i = 1; i <= 3; ++i) {
    const auto card_id = "card-" + std::to_string(i);
    holder::test::create_card_fixture(
        card_store,
        card_id,
        "proj-1",
        "Card " + std::to_string(i),
        "start",
        i
    );
  }

  auto list_future = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/cards?project_id=proj-1"
    );
  });
  auto project_future = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/projects"
    );
  });
  auto status_future = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/ai/status"
    );
  });
  auto runner_future = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::post,
        "/ai/runners",
        nlohmann::json{
            {"name", "Concurrent Runner"},
            {"kind", "ollama"},
            {"base_url", "http://concurrent:11434"},
        }
    );
  });

  auto card_future = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/cards/card-1"
    );
  });

  const auto listed_response = list_future.get();
  const auto projects_response = project_future.get();
  const auto status_response = status_future.get();
  const auto runner_response = runner_future.get();
  const auto card_response = card_future.get();

  REQUIRE(listed_response.status == http::status::ok);
  REQUIRE(projects_response.status == http::status::ok);
  REQUIRE(status_response.status == http::status::ok);
  REQUIRE(runner_response.status == http::status::created);
  REQUIRE(card_response.status == http::status::ok);

  const auto listed = nlohmann::json::parse(listed_response.body);
  const auto projects = nlohmann::json::parse(projects_response.body);
  const auto status = nlohmann::json::parse(status_response.body);
  const auto runner = nlohmann::json::parse(runner_response.body);
  const auto card = nlohmann::json::parse(card_response.body);

  REQUIRE(listed["ok"] == true);
  REQUIRE(listed["data"].is_array());
  REQUIRE(projects["ok"] == true);
  REQUIRE(projects["data"].is_array());
  REQUIRE(status["ok"] == true);
  REQUIRE(status["data"]["runners"].is_array());
  REQUIRE(runner["ok"] == true);
  REQUIRE(runner["data"]["runner_id"].get<std::string>().rfind("manual-", 0) == 0);
  REQUIRE(card["ok"] == true);
  REQUIRE(card["data"]["card_id"] == "card-1");

  // listener_guard's destructor stops the listener and joins its thread.
}

TEST_CASE("Listener stop cancels in-flight ingress read and returns promptly", "[listener]") {
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  holder::platform::Db db;
  db.open(db_path);
  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr);
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  auto run_future = std::async(std::launch::async, [&listener, &signals]() {
    listener.run(signals);
  });
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  boost::asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));
  tcp::socket socket(ioc);
  boost::asio::connect(socket, endpoints);

  const std::string partial_request = "GET /health HTTP/1.1\r\n"
                                      "Host: " +
                                      bound.bind + "\r\n";
  boost::asio::write(socket, boost::asio::buffer(partial_request));

  for (int i = 0; i < 50 && listener.active_read_socket_count() < 1; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(listener.active_read_socket_count() >= 1);

  listener.stop();
  REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  run_future.get();
}

TEST_CASE("Listener stop closes queued ingress sockets and returns promptly", "[listener]") {
  using tcp = boost::asio::ip::tcp;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  holder::platform::Db db;
  db.open(db_path);
  const std::string token = "testtoken";
  holder::api::ConcurrencyProfile concurrency;
  concurrency.io_threads = 1;
  concurrency.ingress_workers = 0;
  concurrency.save_workers = 1;
  concurrency.general_workers = 1;
  concurrency.writer_workers = 1;

  holder::api::Listener listener(

      "127.0.0.1",

      0,

      db,

      token,

      std::chrono::steady_clock::now(),

      nullptr,

      nullptr,

      nullptr,

      nullptr,

      nullptr,

      concurrency

  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  auto run_future = std::async(std::launch::async, [&listener, &signals]() {
    listener.run(signals);
  });
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  boost::asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));
  tcp::socket queued_socket(ioc);
  boost::asio::connect(queued_socket, endpoints);

  const std::string partial_request = "GET /health HTTP/1.1\r\n"
                                      "Host: " +
                                      bound.bind + "\r\n";
  boost::asio::write(queued_socket, boost::asio::buffer(partial_request));

  for (int i = 0; i < 50 && listener.pending_socket_count() < 1; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(listener.pending_socket_count() >= 1);

  listener.stop();
  REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  run_future.get();
}

TEST_CASE("Listener test hook can enqueue a pending socket", "[listener]") {
  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  holder::platform::Db db;
  db.open(db_path);

  holder::api::ConcurrencyProfile concurrency;
  concurrency.io_threads = 1;
  concurrency.ingress_workers = 0;
  concurrency.save_workers = 1;
  concurrency.general_workers = 1;
  concurrency.writer_workers = 1;
  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      concurrency
  );

  listener.enqueue_pending_socket_for_test();
  REQUIRE(listener.pending_socket_count() == 1);

  listener.stop();
  REQUIRE(listener.pending_socket_count() == 0);
}

TEST_CASE("Listener drops accepted sockets when ingress queue is full", "[listener]") {
  using tcp = boost::asio::ip::tcp;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  holder::platform::Db db;
  db.open(db_path);

  holder::api::ConcurrencyProfile concurrency;
  concurrency.io_threads = 1;
  concurrency.ingress_workers = 0;
  concurrency.save_workers = 1;
  concurrency.general_workers = 1;
  concurrency.writer_workers = 1;
  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      concurrency
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  auto run_future = std::async(std::launch::async, [&listener, &signals]() {
    listener.run(signals);
  });
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  boost::asio::io_context ioc;
  std::vector<tcp::socket> sockets;
  sockets.reserve(65);
  for (int i = 0; i < 64; ++i) {
    sockets.emplace_back(connect_test_socket(ioc, bound.bind, bound.port));
  }

  for (int i = 0; i < 50 && listener.pending_socket_count() < 64; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(listener.pending_socket_count() == 64);

  auto overflow_socket = connect_test_socket(ioc, bound.bind, bound.port);

  const std::string request = "GET /health HTTP/1.1\r\nHost: " + bound.bind + "\r\n\r\n";
  boost::system::error_code write_ec;
  boost::asio::write(overflow_socket, boost::asio::buffer(request), write_ec);

  for (int i = 0; i < 100 && listener.dropped_socket_count_for_test() == 0; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  listener.stop();
  REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  run_future.get();
  REQUIRE(listener.dropped_socket_count_for_test() > 0);
}

TEST_CASE("Listener rejects background requests when lane queue is full", "[listener]") {
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);

  holder::api::ConcurrencyProfile concurrency;
  concurrency.io_threads = 1;
  concurrency.ingress_workers = 1;
  concurrency.save_workers = 1;
  concurrency.general_workers = 0;
  concurrency.writer_workers = 1;
  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      concurrency
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  auto run_future = std::async(std::launch::async, [&listener, &signals]() {
    listener.run(signals);
  });
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  boost::asio::io_context ioc;
  std::vector<tcp::socket> sockets;
  sockets.reserve(64);
  const std::string queued_request = "GET /ai/queued HTTP/1.1\r\n"
                                     "Host: " +
                                     bound.bind +
                                     "\r\n"
                                     "Authorization: Bearer " +
                                     token +
                                     "\r\n"
                                     "Connection: close\r\n"
                                     "\r\n";
  // One connection at a time, waiting for each request to reach the lane queue. Sending all 64
  // at once races the single ingress worker: together with the readiness probe's connection
  // they can exceed the 64 accepted sockets allowed to wait, and one is dropped.
  for (std::size_t i = 0; i < 64; ++i) {
    sockets.emplace_back(connect_test_socket(ioc, bound.bind, bound.port));
    write_raw_request(sockets.back(), queued_request);
    for (int wait = 0; wait < 500 && listener.background_queue_count() < i + 1; ++wait) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(listener.background_queue_count() == i + 1);
  }
  REQUIRE(listener.background_queue_count() == 64);

  const auto overflow = holder::test::http_request_raw(
      bound.bind,
      bound.port,
      token,
      http::verb::get,
      "/ai/overflow"
  );
  REQUIRE(overflow.status == http::status::service_unavailable);
  REQUIRE(overflow.body.find("server_busy") != std::string::npos);

  listener.stop();
  REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  run_future.get();
}

TEST_CASE("Listener stop closes queued save requests and queued responses", "[listener]") {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);

  SECTION("queued save requests are closed during stop") {
    holder::api::ConcurrencyProfile concurrency;
    concurrency.io_threads = 1;
    concurrency.ingress_workers = 1;
    concurrency.save_workers = 0;
    concurrency.general_workers = 1;
    concurrency.writer_workers = 1;

    const std::string token = "testtoken";
    holder::api::Listener listener(
        "127.0.0.1",
        0,
        db,
        token,
        std::chrono::steady_clock::now(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        concurrency
    );
    holder::api::Listener::BoundInfo bound;
    try {
      bound = listener.start();
    } catch (const std::exception& ex) {
      SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
    }

    holder::core::SignalHandler signals;
    auto run_future = std::async(std::launch::async, [&listener, &signals]() {
      listener.run(signals);
    });
    REQUIRE(wait_for_http_listener(bound.bind, bound.port));

    boost::asio::io_context ioc;
    auto socket = connect_test_socket(ioc, bound.bind, bound.port);
    const std::string save_request = "PATCH /cards/save-test HTTP/1.1\r\n"
                                     "Host: " +
                                     bound.bind +
                                     "\r\n"
                                     "Authorization: Bearer " +
                                     token +
                                     "\r\n"
                                     "Content-Type: application/json\r\n"
                                     "Content-Length: 2\r\n"
                                     "Connection: close\r\n"
                                     "\r\n{}";
    write_raw_request(socket, save_request);

    for (int i = 0; i < 50 && listener.save_queue_count() < 1; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(listener.save_queue_count() >= 1);

    listener.stop();
    REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
    run_future.get();
  }

  SECTION("queued responses are closed during stop") {
    holder::api::ConcurrencyProfile concurrency;
    concurrency.io_threads = 1;
    concurrency.ingress_workers = 1;
    concurrency.save_workers = 1;
    concurrency.general_workers = 1;
    concurrency.writer_workers = 0;

    const std::string token = "testtoken";
    holder::api::Listener listener(
        "127.0.0.1",
        0,
        db,
        token,
        std::chrono::steady_clock::now(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        concurrency
    );
    holder::api::Listener::BoundInfo bound;
    try {
      bound = listener.start();
    } catch (const std::exception& ex) {
      SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
    }

    holder::core::SignalHandler signals;
    auto run_future = std::async(std::launch::async, [&listener, &signals]() {
      listener.run(signals);
    });
    REQUIRE(wait_for_http_listener(bound.bind, bound.port));

    boost::asio::io_context ioc;
    auto socket = connect_test_socket(ioc, bound.bind, bound.port);
    const std::string request = "GET /health HTTP/1.1\r\n"
                                "Host: " +
                                bound.bind +
                                "\r\n"
                                "Authorization: Bearer " +
                                token +
                                "\r\n"
                                "Connection: close\r\n"
                                "\r\n";
    write_raw_request(socket, request);

    for (int i = 0; i < 50 && listener.response_queue_count() < 1; ++i) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    REQUIRE(listener.response_queue_count() >= 1);

    listener.stop();
    REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
    run_future.get();
  }
}

TEST_CASE("Listener stop drops queued background work and returns promptly", "[listener]") {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  holder::platform::Db db;
  db.open(db_path);
  std::atomic<int> foreground_started{0};
  std::atomic<int> background_runs{0};

  HookedSecretStore secrets;
  secrets.on_get = [&foreground_started](const std::string&) {
    foreground_started.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    return std::optional<holder::privacy::StoredSecret>();
  };
  secrets.on_remove = [&background_runs](const std::string&) { background_runs.fetch_add(1); };

  const std::string token = "testtoken";
  holder::api::ConcurrencyProfile concurrency;
  concurrency.io_threads = 1;
  concurrency.ingress_workers = 1;
  concurrency.save_workers = 1;
  concurrency.general_workers = 1;
  concurrency.writer_workers = 1;

  holder::api::Listener listener(

      "127.0.0.1",

      0,

      db,

      token,

      std::chrono::steady_clock::now(),

      nullptr,

      nullptr,

      &secrets,

      nullptr,

      nullptr,

      concurrency

  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  auto run_future = std::async(std::launch::async, [&listener, &signals]() {
    listener.run(signals);
  });
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  auto foreground = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=slow"
    );
  });

  for (int i = 0; i < 50 && foreground_started.load() < 1; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(foreground_started.load() == 1);

  boost::asio::io_context ioc;
  boost::asio::ip::tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));
  boost::asio::ip::tcp::socket background_socket(ioc);
  boost::asio::connect(background_socket, endpoints);
  const std::string background_request = "DELETE /ai/providers/credentials/queued HTTP/1.1\r\n"
                                         "Host: " +
                                         bound.bind +
                                         "\r\n"
                                         "Authorization: Bearer " +
                                         token +
                                         "\r\n"
                                         "Connection: close\r\n"
                                         "\r\n";
  boost::asio::write(background_socket, boost::asio::buffer(background_request));

  for (int i = 0; i < 50 && listener.background_queue_count() < 1; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(listener.background_queue_count() >= 1);

  listener.stop();
  REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  run_future.get();
  REQUIRE(background_runs.load() == 0);

  try {
    (void)foreground.get();
  } catch (const std::exception& ex) {
    (void)ex;
    // Stop may cancel active non-save work as part of deterministic shutdown.
  }
}

TEST_CASE("Listener stop cancels in-flight writer response and returns promptly", "[listener]") {
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  std::atomic<int> route_started{0};

  HookedSecretStore secrets;
  // The preferred location ID is the response's large payload.
  secrets.on_get = [&route_started](const std::string&) {
    route_started.fetch_add(1);
    return std::optional<holder::privacy::StoredSecret>(
        holder::privacy::StoredSecret{{}, std::string(std::size_t{32} * 1024 * 1024, 'x')});
  };

  const std::string token = "testtoken";
  holder::api::ConcurrencyProfile concurrency;
  concurrency.io_threads = 1;
  concurrency.ingress_workers = 1;
  concurrency.save_workers = 1;
  concurrency.general_workers = 1;
  concurrency.writer_workers = 1;

  holder::api::Listener listener(

      "127.0.0.1",

      0,

      db,

      token,

      std::chrono::steady_clock::now(),

      nullptr,

      nullptr,

      &secrets,

      nullptr,

      nullptr,

      concurrency

  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  auto run_future = std::async(std::launch::async, [&listener, &signals]() {
    listener.run(signals);
  });
  REQUIRE(wait_for_http_listener(bound.bind, bound.port));

  boost::asio::io_context ioc;
  tcp::resolver resolver(ioc);
  auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));
  tcp::socket socket(ioc);
  boost::asio::connect(socket, endpoints);
  const std::string request = "GET /locations?project_id=large HTTP/1.1\r\n"
                              "Host: " +
                              bound.bind +
                              "\r\n"
                              "Authorization: Bearer " +
                              token +
                              "\r\n"
                              "Connection: close\r\n"
                              "\r\n";
  boost::asio::write(socket, boost::asio::buffer(request));

  for (int i = 0; i < 50 && route_started.load() < 1; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(route_started.load() == 1);

  listener.stop();
  REQUIRE(run_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready);
  run_future.get();
}

TEST_CASE("Listener serves card nudge and ai status routes without regression", "[listener]") {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  create_project(db, "proj-1", (dir / "project").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  holder::llm::RunnerRegistry runner_registry(&db, nullptr);
  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      &card_store,
      &fts,
      nullptr,
      nullptr,
      &runner_registry
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread listener_thread([&listener, &signals]() {
    listener.run(signals);
  });
  ListenerRunGuard listener_guard(listener, std::move(listener_thread));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  holder::test::create_card_fixture(card_store, "card-1", "proj-1", "Frog", "", 1);

  const auto status = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      http::verb::get,
      "/ai/status",
      nlohmann::json::object(),
      http::status::ok
  );
  REQUIRE(status["ok"] == true);
  REQUIRE(status["data"]["runners"].is_array());

  const auto listed = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      http::verb::get,
      "/ai/nudges?project_id=proj-1&card_id=card-1",
      nlohmann::json::object(),
      http::status::ok
  );
  REQUIRE(listed["ok"] == true);
  REQUIRE(listed["data"]["nudges"].is_array());

  const auto patched = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      http::verb::patch,
      "/cards/card-1",
      nlohmann::json{
          {"title", "Frog Updated"},
          {"content", "Now has body"},
          {"updated_at", 2},
      },
      http::status::ok
  );
  REQUIRE(patched["ok"] == true);

  // listener_guard's destructor stops the listener and joins its thread.
}

TEST_CASE(
    "Multiple configured runners do not block card save path under background saturation",
    "[listener]"
) {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  create_project(db, "proj-1", (dir / "project").string());
  holder::index::FtsIndexer fts(db);
  holder::card::CardStore card_store(db, &fts);
  holder::test::EnvGuard fake_runner_env("HOLDER_MODEL_RUNNER_FAKE", "1");
  holder::llm::RunnerRegistry runner_registry(&db, nullptr);
  std::atomic<int> slow_started{0};

  HookedSecretStore secrets;
  secrets.on_remove = [&slow_started](const std::string&) {
    slow_started.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  };

  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      &card_store,
      &fts,
      &secrets,
      nullptr,
      &runner_registry
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread listener_thread([&listener, &signals]() {
    listener.run(signals);
  });
  ListenerRunGuard listener_guard(listener, std::move(listener_thread));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  holder::test::create_card_fixture(card_store, "card-1", "proj-1", "Card", "start", 1);

  for (int i = 0; i < 3; ++i) {
    const auto created_runner = holder::test::http_json_request(
        bound.bind,
        bound.port,
        token,
        http::verb::post,
        "/ai/runners",
        nlohmann::json{
            {"name", "Runner " + std::to_string(i + 1)},
            {"kind", "ollama"},
            {"base_url", "http://runner" + std::to_string(i + 1) + ":11434"},
        },
        http::status::created
    );
    REQUIRE(created_runner["ok"] == true);
  }

  auto slow1 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::delete_,
        "/ai/providers/credentials/slow"
    );
  });
  auto slow2 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::delete_,
        "/ai/providers/credentials/slow"
    );
  });
  auto slow3 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::delete_,
        "/ai/providers/credentials/slow"
    );
  });

  for (int i = 0; i < 50 && slow_started.load() < 3; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(slow_started.load() == 3);

  const auto save_started = std::chrono::steady_clock::now();
  const auto patched = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      http::verb::patch,
      "/cards/card-1",
      nlohmann::json{
          {"title", "Saved While Busy"},
          {"content", "updated"},
          {"updated_at", 2},
      },
      http::status::ok
  );
  const auto save_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - save_started
  )
                                   .count();
  REQUIRE(patched["ok"] == true);
  REQUIRE(save_elapsed_ms < 800);

  REQUIRE(slow1.get().status == http::status::ok);
  REQUIRE(slow2.get().status == http::status::ok);
  REQUIRE(slow3.get().status == http::status::ok);

  // listener_guard's destructor stops the listener and joins its thread.
}

TEST_CASE(
    "Queued save request jumps ahead of queued non-save work at dispatch time",
    "[listener]"
) {
  namespace http = boost::beast::http;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  std::atomic<int> slow_foreground_started{0};
  // Set the instant this handler actually runs -- proof that the general-worker pool has
  // dispatched it, independent of any wall-clock assumption about how long that takes to
  // happen. See the REQUIRE_FALSE below for why this replaces a timing-based wait_for check.
  std::atomic<bool> fast_foreground_started{false};

  HookedSecretStore secrets;
  secrets.on_get = [&](const std::string& account) {
    if (account == "slow") {
      slow_foreground_started.fetch_add(1);
      std::this_thread::sleep_for(std::chrono::milliseconds(300));
    } else {
      fast_foreground_started.store(true);
    }
    return std::optional<holder::privacy::StoredSecret>();
  };

  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr,
      &secrets
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread listener_thread([&listener, &signals]() {
    listener.run(signals);
  });
  ListenerRunGuard listener_guard(listener, std::move(listener_thread));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto slow1 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=slow"
    );
  });
  auto slow2 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=slow"
    );
  });
  auto slow3 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=slow"
    );
  });

  for (int i = 0; i < 50 && slow_foreground_started.load() < 3; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(slow_foreground_started.load() == 3);

  auto queued_foreground = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=fast"
    );
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(30));

  const auto save_started = std::chrono::steady_clock::now();
  const auto saved = holder::test::http_json_request(
      bound.bind,
      bound.port,
      token,
      http::verb::patch,
      "/cards/save-priority",
      // Without a card store the save is refused at once; only its lane matters here.
      nlohmann::json{{"updated_at", 1}, {"content", "x"}},
      http::status::not_implemented
  );
  const auto save_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::steady_clock::now() - save_started
  )
                                   .count();

  REQUIRE(saved["ok"] == false);
  REQUIRE(save_elapsed_ms < 200);
  // Deterministic, not a timing race: the default ConcurrencyProfile has exactly 3
  // general_workers_, all still occupied by the still-sleeping slow1/slow2/slow3 handlers at
  // this point (each holds its worker for a fixed 300ms, far longer than this test body takes
  // to reach here) -- so it is architecturally impossible for the fast foreground request to have been
  // dispatched yet, regardless of scheduling delays on this thread. A wait_for(...)-based
  // check here previously inferred that from a fixed wall-clock window instead of observing it
  // directly, which is what made this assertion flaky under CI scheduling contention.
  REQUIRE_FALSE(fast_foreground_started.load());

  REQUIRE(slow1.get().status == http::status::ok);
  REQUIRE(slow2.get().status == http::status::ok);
  REQUIRE(slow3.get().status == http::status::ok);
  REQUIRE(queued_foreground.get().status == http::status::ok);

  // listener_guard's destructor stops the listener and joins its thread.
}

TEST_CASE(
    "Queued background request is dropped if client disconnects before execution",
    "[listener]"
) {
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;

  const auto dir = make_temp_dir();
  const auto db_path = dir / "holder.db";
  auto db = open_db_with_schema(db_path);
  std::atomic<int> slow_foreground_started{0};
  std::atomic<int> canceled_background_runs{0};

  HookedSecretStore secrets;
  secrets.on_get = [&slow_foreground_started](const std::string&) {
    slow_foreground_started.fetch_add(1);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    return std::optional<holder::privacy::StoredSecret>();
  };
  secrets.on_remove = [&canceled_background_runs](const std::string&) {
    canceled_background_runs.fetch_add(1);
  };

  const std::string token = "testtoken";
  holder::api::Listener listener(
      "127.0.0.1",
      0,
      db,
      token,
      std::chrono::steady_clock::now(),
      nullptr,
      nullptr,
      &secrets
  );
  holder::api::Listener::BoundInfo bound;
  try {
    bound = listener.start();
  } catch (const std::exception& ex) {
    SKIP(std::string("Socket bind not available in test environment: ") + ex.what());
  }

  holder::core::SignalHandler signals;
  std::thread listener_thread([&listener, &signals]() {
    listener.run(signals);
  });
  ListenerRunGuard listener_guard(listener, std::move(listener_thread));
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto slow1 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=slow"
    );
  });
  auto slow2 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=slow"
    );
  });
  auto slow3 = std::async(std::launch::async, [&]() {
    return holder::test::http_request_raw(
        bound.bind,
        bound.port,
        token,
        http::verb::get,
        "/locations?project_id=slow"
    );
  });

  for (int i = 0; i < 50 && slow_foreground_started.load() < 3; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  REQUIRE(slow_foreground_started.load() == 3);

  {
    boost::asio::io_context ioc;
    tcp::resolver resolver(ioc);
    auto endpoints = resolver.resolve(bound.bind, std::to_string(bound.port));
    tcp::socket socket(ioc);
    boost::asio::connect(socket, endpoints);

    const std::string request = "DELETE /ai/providers/credentials/canceled HTTP/1.1\r\n"
                                "Host: " +
                                bound.bind +
                                "\r\n"
                                "Authorization: Bearer " +
                                token +
                                "\r\n"
                                "Connection: close\r\n"
                                "\r\n";
    boost::asio::write(socket, boost::asio::buffer(request));
    holder::test::close_http_socket(socket);
  }

  REQUIRE(slow1.get().status == http::status::ok);
  REQUIRE(slow2.get().status == http::status::ok);
  REQUIRE(slow3.get().status == http::status::ok);

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  REQUIRE(canceled_background_runs.load() == 0);

  // listener_guard's destructor stops the listener and joins its thread.
}

TEST_CASE("Listener joins its workers and reports database initialization failures", "[listener]") {
  const auto dir = make_temp_dir();
  auto db = open_db_with_schema(dir / "holder.db");
  holder::llm::RunnerRegistry registry(&db, nullptr);
  const std::string token = "testtoken";
  holder::api::HttpServer server("127.0.0.1", 0, db, token, nullptr, nullptr, nullptr, &registry);
  holder::core::SignalHandler signals;
  server.start();
  // The root connection is valid, but each request worker must initialize its
  // own runner registry. Previously this exception escaped std::thread.
  db.exec("DROP TABLE ai_runners");
  REQUIRE_THROWS_AS(server.run(signals), std::runtime_error);
  CHECK_NOTHROW(server.stop());
}
