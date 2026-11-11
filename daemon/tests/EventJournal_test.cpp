#include "api/generated/AiJson.h"
#include "api/support/EventJournal.h"
#include "api/support/RunEventStore.h"
#include "api/support/SseStream.h"
#include "api/support/StreamServer.h"
#include <boost/beast/core/flat_buffer.hpp>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <future>
#include <thread>

using namespace holder::api::support;

TEST_CASE("Event journal uses absolute replay cursors across trimming", "[events]") {
  EventJournal journal(3);
  const auto baseline = journal.cursor();
  std::string third;
  for (int n = 1; n <= 600; ++n) {
    const auto event = journal.append({"chunk", {{"n", n}}}, n == 600);
    if (n == 597) third = event.id;
  }
  auto batch = journal.read(third);
  REQUIRE_FALSE(batch.resync_required);
  REQUIRE(batch.finished);
  REQUIRE(batch.truncated);
  REQUIRE(batch.events.size() == 3);
  REQUIRE(batch.events[0].data["n"] == 598);
  REQUIRE(batch.events[2].data["n"] == 600);
  REQUIRE(journal.read(batch.cursor).events.empty());
  REQUIRE(journal.read(baseline).resync_required);
  REQUIRE(journal.read(EventJournal{}.cursor()).resync_required);
  REQUIRE(journal.read(batch.cursor.substr(0, 37) + "601").resync_required);
}

TEST_CASE("Event journal bounds bytes and encodes one JSON data line", "[events]") {
  EventJournal journal(10, 180);
  auto first = journal.append({"chunk", {{"delta", "line one\nline two"}}});
  REQUIRE(encode_sse(first).find("line one\\nline two") != std::string::npos);
  journal.append({"chunk", {{"delta", std::string(300, 'x')}}});
  REQUIRE(journal.read().events.empty());
  REQUIRE(journal.read(first.id).resync_required);
  for (const auto& bad :
       {"",
        "hello",
        "g0000000-0000-0000-0000-000000000000:1",
        "00000000-0000-0000-0000-000000000000:-1",
        "00000000-0000-0000-0000-000000000000:18446744073709551616"})
    REQUIRE_FALSE(EventJournal::valid_cursor(bad));
}

TEST_CASE("A reused run ID starts a new event epoch", "[events]") {
  const auto first = append_run_event("journal-reuse", ai_events::run_started, {}, false);
  append_run_event("journal-reuse", ai_events::done, {}, true);
  const auto next = append_run_event("journal-reuse", ai_events::run_started, {}, false);
  REQUIRE(first.id != next.id);
  const auto batch = read_run_events("journal-reuse", "");
  REQUIRE(batch);
  REQUIRE_FALSE(batch->finished);
  REQUIRE(batch->events.size() == 1);
  REQUIRE(read_run_events("journal-reuse", first.id)->resync_required);
}

TEST_CASE("SSE admission restores rejected sockets and bounds producer output", "[events]") {
  boost::asio::io_context ioc;
  auto registry = std::make_shared<SseRegistry>(1);
  boost::asio::ip::tcp::socket one(ioc), two(ioc);
  auto stream = SseStream::reserve(one, registry);
  REQUIRE(stream);
  REQUIRE_FALSE(SseStream::reserve(two, registry));
  REQUIRE(stream->send({"queued", {"chunk", {{"delta", "queued before cancellation"}}}}));
  REQUIRE_FALSE(stream->send({"id", {"chunk", {{"delta", std::string(2 * 1024 * 1024, 'x')}}}}));
  REQUIRE_FALSE(stream->open());
  stream->activate();
  ioc.run();
  REQUIRE_FALSE(stream->open());
}

TEST_CASE(
    "SSE cancellation keeps its I/O context alive until source work returns",
    "[events][http]"
) {
  using tcp = boost::asio::ip::tcp;
  boost::asio::io_context ioc;
  tcp::acceptor acceptor(ioc, {boost::asio::ip::make_address("127.0.0.1"), 0});
  tcp::socket client(ioc);
  client.connect(acceptor.local_endpoint());
  tcp::socket socket(ioc);
  acceptor.accept(socket);
  auto registry = std::make_shared<SseRegistry>();
  std::promise<void> entered, released, exited;
  auto entered_future = entered.get_future();
  auto release_future = released.get_future().share();
  auto exited_future = exited.get_future();
  auto stream = SseStream::start(socket, registry, [&] {
    entered.set_value();
    release_future.wait();
    EventBatch batch;
    batch.finished = true;
    return batch;
  });
  std::thread io_worker([&] {
    ioc.run();
    exited.set_value();
  });
  struct Cleanup {
    std::function<void()> action;
    ~Cleanup() { action(); }
  } cleanup{[&] {
    stream->cancel();
    try {
      released.set_value();
    } catch (const std::future_error&) {
    }
    io_worker.join();
    registry->poll_workers().join();
  }};
  REQUIRE(entered_future.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
  client.close();
  REQUIRE(exited_future.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout);
  released.set_value();
  REQUIRE(exited_future.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
}

TEST_CASE("Run event retention evicts history when the run limit is exceeded", "[events]") {
  std::vector<std::string> ids;
  for (int i = 0; i < 129; ++i) {
    ids.push_back("retention-test-" + std::to_string(i));
    append_run_event(ids.back(), ai_events::done, {}, true);
  }

  std::size_t retained = 0;
  for (const auto& id : ids) {
    if (const auto batch = read_run_events(id, "")) {
      ++retained;
      REQUIRE(batch->events.size() == 1);
      CHECK(batch->events.front().data.at("run_id") == id);
    }
  }
  CHECK(retained > 0);
  CHECK(retained <= 128);
  REQUIRE(read_run_events(ids.back(), ""));
}

TEST_CASE("Event stream admission reports a full registry as server busy", "[events]") {
  boost::asio::io_context ioc;
  boost::asio::ip::tcp::socket socket(ioc);
  auto registry = std::make_shared<SseRegistry>(0);
  auto serve = serve_events(registry, [] { return EventBatch{}; });
  boost::beast::http::response<boost::beast::http::string_body> response;

  REQUIRE_FALSE(serve(socket, response));
  CHECK(response.result() == boost::beast::http::status::service_unavailable);
  CHECK(nlohmann::json::parse(response.body()).at("error").at("code") == "server_busy");
}

TEST_CASE("SSE closes the client connection when its event source throws", "[events][http]") {
  namespace http = boost::beast::http;
  using tcp = boost::asio::ip::tcp;
  boost::asio::io_context ioc;
  tcp::acceptor acceptor(ioc, {boost::asio::ip::make_address("127.0.0.1"), 0});
  tcp::socket client(ioc);
  client.connect(acceptor.local_endpoint());
  auto socket = acceptor.accept();
  auto registry = std::make_shared<SseRegistry>();
  std::promise<void> called;
  auto source_called = called.get_future();

  auto stream = SseStream::start(socket, registry, [&]() -> EventBatch {
    called.set_value();
    throw std::runtime_error("fixture event source failure");
  });
  REQUIRE(stream);
  ioc.run();
  REQUIRE(source_called.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
  CHECK_FALSE(stream->open());

  boost::beast::flat_buffer buffer;
  http::response<http::string_body> response;
  http::read(client, buffer, response);
  CHECK(response.result() == http::status::ok);
  CHECK(response[http::field::content_type] == "text/event-stream");
  CHECK(response.body().empty());
}

TEST_CASE("SSE closes a socket when writing response headers fails", "[events][sse-boundaries]") {
  boost::asio::io_context io;
  boost::asio::ip::tcp::socket socket(io);
  auto registry = std::make_shared<SseRegistry>();
  auto stream = SseStream::start(socket, registry);
  REQUIRE(stream);
  io.run();
  CHECK_FALSE(stream->open());
  CHECK(registry->open_count() == 0);
}

TEST_CASE("SSE closes a blocked event write when its peer resets the connection", "[events][sse-boundaries]") {
  using tcp = boost::asio::ip::tcp;
  boost::asio::io_context io;
  tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
  tcp::socket peer(io);
  peer.connect(acceptor.local_endpoint());
  auto socket = acceptor.accept();
  socket.set_option(boost::asio::socket_base::send_buffer_size(1024));
  peer.set_option(boost::asio::socket_base::receive_buffer_size(1024));
  auto registry = std::make_shared<SseRegistry>();
  auto stream = SseStream::start(socket, registry);
  REQUIRE(stream);
  REQUIRE(stream->send({"large", {"chunk", {{"delta", std::string(1024 * 1024, 'x')}}}}));
  io.run_for(std::chrono::milliseconds(50));
  REQUIRE(stream->open());
  peer.set_option(boost::asio::socket_base::linger(true, 0));
  peer.close();
  io.restart();
  io.run();
  CHECK_FALSE(stream->open());
  CHECK(registry->open_count() == 0);
}

TEST_CASE("SSE sends keepalive frames while an event stream is idle", "[events][sse-boundaries]") {
  using tcp = boost::asio::ip::tcp;
  boost::asio::io_context io;
  tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
  tcp::socket peer(io);
  peer.connect(acceptor.local_endpoint());
  auto socket = acceptor.accept();
  auto registry = std::make_shared<SseRegistry>();
  auto stream = SseStream::start(socket, registry);
  REQUIRE(stream);
  peer.non_blocking(true);
  std::string received;
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
  while (received.find(": keepalive\n\n") == std::string::npos &&
         std::chrono::steady_clock::now() < deadline) {
    io.restart();
    io.run_for(std::chrono::milliseconds(50));
    std::array<char, 4096> buffer{};
    boost::system::error_code error;
    for (;;) {
      const auto count = peer.read_some(boost::asio::buffer(buffer), error);
      received.append(buffer.data(), count);
      if (error) break;
    }
  }
  stream->cancel();
  io.restart();
  io.run();
  INFO(received);
  CHECK(received.find("200 OK") != std::string::npos);
  CHECK(received.find(": keepalive\n\n") != std::string::npos);
  CHECK_FALSE(stream->open());
  CHECK(registry->open_count() == 0);
}
