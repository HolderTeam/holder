#include "api/Session.h"
#include "api/GeneratedRoutes.h"
#include "api/McpEndpoint.h"
#include "api/support/HttpAuth.h"
#include "api/support/HttpResponses.h"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <future>
#include <mutex>
#include <random>
#include <string>
#include <string_view>

namespace holder::api {
namespace {

namespace http = boost::beast::http;

// Allow 8,192 cursor characters at up to 12 wire bytes per character (four UTF-8
// bytes, each percent encoded), plus the path, parameter names, and filters.
constexpr std::size_t request_target_limit = std::size_t{128} * 1024;
constexpr std::size_t request_header_limit = std::size_t{16} * 1024;
constexpr std::size_t request_line_limit = request_target_limit + 64;

template <typename T> void ignore_result(T&&) noexcept {}

std::string generate_uuid_v4() {
  if (const char* seed_env = std::getenv("HOLDER_UUID_SEED")) {
    try {
      const auto seed = static_cast<unsigned int>(std::stoul(seed_env));
      static std::mutex mutex;
      std::lock_guard<std::mutex> lock(mutex);
      static std::mt19937 rng(seed);
      boost::uuids::basic_random_generator<std::mt19937> gen(&rng);
      return boost::uuids::to_string(gen());
    } catch (const std::exception& ex) {
      (void)ex;
      // Fall through to random generator.
    }
  }
  boost::uuids::random_generator gen;
  return boost::uuids::to_string(gen());
}

struct AsyncReadResult {
  boost::system::error_code ec;
  http::status rejection;
  Session::tcp::socket socket;
  Session::Request req;
};

struct AsyncWriteResult {
  boost::system::error_code ec;
  Session::tcp::socket socket;
};

AsyncReadResult async_read_request(
    Session::tcp::socket socket,
    const Session::SocketHook& on_io_start,
    const Session::SocketHook& on_io_done
) {
  auto promise = std::make_shared<std::promise<AsyncReadResult>>();
  auto future = promise->get_future();

  struct State {
    explicit State(Session::tcp::socket s)
        : socket(std::move(s)),
          strand(boost::asio::make_strand(socket.get_executor())) {}
    std::mutex mu;
    bool done = false;
    bool cancel_requested = false;
    Session::tcp::socket socket;
    boost::asio::strand<boost::asio::any_io_executor> strand;
    std::string wire;
    decltype(boost::asio::dynamic_buffer(wire)) buffer = boost::asio::dynamic_buffer(wire);
    http::request_parser<http::string_body> parser;
  };

  auto state = std::make_shared<State>(std::move(socket));
  auto io_handle = std::make_shared<Session::IoHandle>();
  io_handle->cancel = [weak_state = std::weak_ptr<State>(state)]() {
    if (auto locked = weak_state.lock()) {
      boost::asio::strand<boost::asio::any_io_executor> strand(locked->strand);
      {
        std::lock_guard<std::mutex> lock(locked->mu);
        if (locked->done) {
          return; // LCOV_EXCL_LINE: defensive late-cancel race.
        }
        locked->cancel_requested = true;
      }
      boost::asio::post(strand, [weak_state]() {
        if (auto posted = weak_state.lock()) {
          std::lock_guard<std::mutex> lock(posted->mu);
          if (posted->done) {
            return; // LCOV_EXCL_LINE: defensive late-cancel race.
          }
          boost::system::error_code ec;
          ignore_result(posted->socket.cancel(ec)); // NOLINT(bugprone-unused-return-value)
          ignore_result(posted->socket.shutdown(Session::tcp::socket::shutdown_both, ec)
          ); // NOLINT(bugprone-unused-return-value)
          ec.clear();
          ignore_result(posted->socket.close(ec)); // NOLINT(bugprone-unused-return-value)
        }
      });
    }
  };
  if (on_io_start) {
    on_io_start(io_handle);
  }
  const auto finish = [state, promise, on_io_done, io_handle](
                          boost::system::error_code ec,
                          http::status rejection = http::status::unknown
                      ) {
    std::unique_lock<std::mutex> lock(state->mu);
    state->done = true;
    AsyncReadResult result{ec, rejection, std::move(state->socket), state->parser.release()};
    lock.unlock();
    if (on_io_done) {
      on_io_done(io_handle);
    }
    promise->set_value(std::move(result));
  };

  // Frame the line and headers separately so neither can consume the other's budget.
  // Leave the bytes in the buffer for Beast to parse, including any body read ahead.
  const auto read_headers = [state, finish](std::size_t line_size) {
    const auto header_end_limit = line_size + request_header_limit;
    const auto read_limit = std::max(header_end_limit, state->wire.size());
    boost::asio::async_read_until(
        state->socket,
        boost::asio::dynamic_buffer(state->wire, read_limit),
        "\r\n\r\n",
        boost::asio::bind_executor(
            state->strand,
            [state,
             finish,
             header_end_limit](boost::system::error_code ec, std::size_t header_end) {
              if (ec == boost::asio::error::not_found || header_end > header_end_limit) {
                return finish({}, http::status::request_header_fields_too_large);
              }
              if (ec) {
                return finish(ec);
              }
              state->parser.header_limit(static_cast<std::uint32_t>(header_end_limit));
              http::async_read(
                  state->socket,
                  state->buffer,
                  state->parser,
                  boost::asio::bind_executor(
                      state->strand,
                      [finish](boost::system::error_code error, std::size_t) {
                        finish(error);
                      }
                  )
              );
            }
        )
    );
  };
  boost::asio::post(state->strand, [state, finish, read_headers]() {
    boost::asio::async_read_until(
        state->socket,
        boost::asio::dynamic_buffer(state->wire, request_line_limit),
        "\r\n",
        boost::asio::bind_executor(
            state->strand,
            [state, finish, read_headers](boost::system::error_code ec, std::size_t line_size) {
              if (ec == boost::asio::error::not_found) {
                return finish({}, http::status::uri_too_long);
              }
              if (ec == boost::asio::error::eof && state->wire.empty()) {
                return finish(http::error::end_of_stream);
              }
              if (ec) {
                return finish(ec);
              }
              const std::string_view line{state->wire.data(), line_size - 2};
              const auto method_end = line.find(' ');
              if (method_end == line.npos) {
                return finish(http::error::bad_target);
              }
              const auto target_end = line.find(' ', method_end + 1);
              if (target_end == line.npos) {
                return finish(http::error::bad_target);
              }
              if (target_end - method_end - 1 > request_target_limit) {
                return finish({}, http::status::uri_too_long);
              }

              read_headers(line_size);
            }
        )
    );
  });
  return future.get();
}

AsyncWriteResult async_write_response(
    Session::PreparedResponse prepared,
    const Session::SocketHook& on_io_start,
    const Session::SocketHook& on_io_done
) {
  auto promise = std::make_shared<std::promise<AsyncWriteResult>>();
  auto future = promise->get_future();
  struct State {
    explicit State(Session::PreparedResponse prepared_response)
        : response(std::move(prepared_response)),
          strand(boost::asio::make_strand(response.socket.get_executor())) {}
    std::mutex mu;
    bool done = false;
    bool cancel_requested = false;
    Session::PreparedResponse response;
    boost::asio::strand<boost::asio::any_io_executor> strand;
  };

  auto state = std::make_shared<State>(std::move(prepared));
  auto io_handle = std::make_shared<Session::IoHandle>();
  io_handle->cancel = [weak_state = std::weak_ptr<State>(state)]() {
    if (auto locked = weak_state.lock()) {
      boost::asio::strand<boost::asio::any_io_executor> strand(locked->strand);
      {
        std::lock_guard<std::mutex> lock(locked->mu);
        if (locked->done) {
          return; // LCOV_EXCL_LINE: defensive late-cancel race.
        }
        locked->cancel_requested = true;
      }
      boost::asio::post(strand, [weak_state]() {
        if (auto posted = weak_state.lock()) {
          std::lock_guard<std::mutex> lock(posted->mu);
          if (posted->done) {
            return; // LCOV_EXCL_LINE: defensive late-cancel race.
          }
          boost::system::error_code ec;
          ignore_result(posted->response.socket.cancel(ec)); // NOLINT(bugprone-unused-return-value)
          ignore_result(posted->response.socket.shutdown(Session::tcp::socket::shutdown_both, ec)
          ); // NOLINT(bugprone-unused-return-value)
          ec.clear();
          ignore_result(posted->response.socket.close(ec)); // NOLINT(bugprone-unused-return-value)
        }
      });
    }
  };

  if (on_io_start) {
    on_io_start(io_handle);
  }
  boost::asio::post(state->strand, [state, promise, on_io_done, io_handle]() mutable {
    http::async_write(
        state->response.socket,
        state->response.res,
        boost::asio::bind_executor(
            state->strand,
            [state,
             promise,
             on_io_done,
             io_handle](boost::system::error_code ec, std::size_t) mutable {
              std::unique_lock<std::mutex> lock(state->mu);
              state->done = true;
              AsyncWriteResult result{ec, std::move(state->response.socket)};
              lock.unlock();
              if (on_io_done) {
                on_io_done(io_handle);
              }
              promise->set_value(std::move(result));
            }
        )
    );
  });
  return future.get();
}

// A project's cards: /projects/{project_id}/cards and below.
bool is_card_path(std::string_view path) {
  constexpr std::string_view projects = "/projects/";
  if (!path.starts_with(projects)) return false;
  const auto slash = path.find('/', projects.size());
  if (slash == std::string_view::npos) return false;
  path.remove_prefix(slash);
  return path == "/cards" || path.starts_with("/cards/");
}

} // namespace

Session::Session(
    tcp::socket socket,
    holder::platform::Db& db,
    const std::string& auth_token,
    std::chrono::steady_clock::time_point started_at,
    holder::card::CardStore* card_store,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    holder::llm::RunnerRegistry* runner_registry,
    holder::api::support::EventService* events,
    const mcp::Access* mcp_access
)
    : socket_(std::move(socket)),
      db_(db),
      auth_token_(auth_token),
      started_at_(started_at),
      card_store_(card_store),
      fts_(fts),
      secret_store_(secret_store),
      git_ops_(git_ops),
      runner_registry_(runner_registry),
      events_(events),
      mcp_access_(mcp_access) {}

Session::Session(
    PreparedRequest prepared,
    holder::platform::Db& db,
    const std::string& auth_token,
    std::chrono::steady_clock::time_point started_at,
    holder::card::CardStore* card_store,
    holder::index::FtsIndexer* fts,
    holder::privacy::SecretStore* secret_store,
    holder::git::GitOps* git_ops,
    holder::llm::RunnerRegistry* runner_registry,
    holder::api::support::EventService* events,
    const mcp::Access* mcp_access
)
    : socket_(std::move(prepared.socket)),
      db_(db),
      auth_token_(auth_token),
      started_at_(started_at),
      card_store_(card_store),
      fts_(fts),
      secret_store_(secret_store),
      git_ops_(git_ops),
      runner_registry_(runner_registry),
      events_(events),
      mcp_access_(mcp_access),
      req_(std::move(prepared.req)),
      request_started_(prepared.request_started),
      path_(std::move(prepared.path)),
      query_string_(std::move(prepared.query_string)),
      has_loaded_request_(true) {}

std::optional<Session::PreparedRequest> Session::prepare_request(
    tcp::socket socket,
    const SocketHook& on_io_start,
    const SocketHook& on_io_done
) {
  const auto request_started = std::chrono::steady_clock::now();
  auto read_result = async_read_request(std::move(socket), on_io_start, on_io_done);
  if (read_result.rejection != http::status::unknown) {
    const bool target = read_result.rejection == http::status::uri_too_long;
    auto response = support::error_response(
        read_result.rejection,
        target ? "uri_too_long" : "request_header_fields_too_large",
        target ? "request target exceeds 128 KiB" : "request headers exceed 16 KiB"
    );
    write_prepared_response(
        {std::move(read_result.socket),
         std::move(read_result.req),
         std::move(response),
         request_started},
        on_io_start,
        on_io_done
    );
    return std::nullopt;
  }
  auto& ec = read_result.ec;
  if (ec == http::error::end_of_stream) {
    ignore_result(read_result.socket.shutdown(tcp::socket::shutdown_send, ec)
    ); // NOLINT(bugprone-unused-return-value)
    return std::nullopt;
  }
  if (ec) {
    spdlog::warn("read failed: {}", ec.message());
    ignore_result(read_result.socket.shutdown(tcp::socket::shutdown_send, ec)
    ); // NOLINT(bugprone-unused-return-value)
    return std::nullopt;
  }

  const auto target = read_result.req.target();
  const std::string target_str(target.data(), target.size());
  const auto query_pos = target_str.find('?');
  const std::string path = (query_pos == std::string::npos) ? target_str
                                                            : target_str.substr(0, query_pos);
  const std::string query_string = (query_pos == std::string::npos)
                                       ? ""
                                       : target_str.substr(query_pos + 1);

  const auto lane = classify_request_lane(read_result.req, path);
  PreparedRequest prepared{
      std::move(read_result.socket),
      std::move(read_result.req),
      request_started,
      path,
      query_string,
      lane,
  };
  return prepared;
}

void Session::run() {
  auto prepared = execute();
  if (prepared.has_value()) {
    write_prepared_response(std::move(*prepared));
  }
}

std::optional<Session::PreparedResponse> Session::execute() {
  if (!ensure_request_loaded()) {
    return std::nullopt;
  }
  return process_loaded_request();
}

bool Session::ensure_request_loaded() {
  if (has_loaded_request_) {
    return true;
  }
  auto prepared = prepare_request(std::move(socket_));
  if (!prepared.has_value()) {
    return false;
  }
  socket_ = std::move(prepared->socket);
  req_ = std::move(prepared->req);
  request_started_ = prepared->request_started;
  path_ = std::move(prepared->path);
  query_string_ = std::move(prepared->query_string);
  has_loaded_request_ = true;
  return true;
}

std::optional<Session::PreparedResponse> Session::process_loaded_request() {
  Response res;
  support::StreamServer stream;
  if (mcp_access_ && req_.target() == "/mcp") {
    res = mcp::handle(req_, socket_, *mcp_access_, db_, fts_, secret_store_, git_ops_);
  } else if (!is_public_generated_route(req_) &&
             !support::is_authorized_bearer(req_, auth_token_)) {
    res = support::error_response(
        http::status::unauthorized,
        "unauthorized",
        "Missing or invalid token."
    );
  } else if (dispatch_generated_routes(
                 req_,
                 res,
                 db_,
                 runner_registry_,
                 fts_,
                 secret_store_,
                 git_ops_,
                 [] {
                   return generate_uuid_v4();
                 },
                 started_at_,
                 card_store_,
                 events_,
                 &stream,
                 &socket_
             )) {
    // Generated, or a 405 for a declared path with another method. A stream the generated
    // route has checked is written here, on this request's socket.
    if (stream && stream(socket_, res)) return std::nullopt;
  } else {
    // Every route the daemon serves is declared and dispatched by generated code.
    res = support::error_response(http::status::not_found, "not_found", "Route not found.");
  }

  PreparedResponse prepared{
      std::move(socket_),
      req_,
      std::move(res),
      request_started_,
      classify_request_lane(req_, path_),
  };
  return prepared;
}

void Session::write_prepared_response(
    PreparedResponse prepared,
    const SocketHook& on_io_start,
    const SocketHook& on_io_done
) {
  const auto method = std::string(prepared.req.method_string());
  const auto target = std::string(prepared.req.target());
  const auto status = prepared.res.result_int();
  const auto lane = prepared.lane;
  const auto request_started = prepared.request_started;

  auto write_result = async_write_response(std::move(prepared), on_io_start, on_io_done);
  auto& ec = write_result.ec;
  if (ec) {
    spdlog::warn("write failed: {}", ec.message());
  }

  const auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - request_started
  )
                               .count();
  spdlog::info(
      "HTTP {} {} -> {} ({}ms)",
      method,
      target, // LCOV_EXCL_LINE
      status,
      duration_ms
  );
  spdlog::debug( // LCOV_EXCL_LINE: compiled-out debug call retains a GCC line record.
      "HTTP lane={} method={} target={} status={} duration_ms={}",
      lane_name(lane),
      method,
      target, // LCOV_EXCL_LINE
      status,
      duration_ms
  );

  ignore_result(write_result.socket.shutdown(tcp::socket::shutdown_send, ec)
  ); // NOLINT(bugprone-unused-return-value)
}

Session::RequestLane Session::classify_request_lane(const Request& req, const std::string& path) {
  // Initial route-to-lane mapping:
  // - save: card writes except link graph mutations
  // - background: AI routes, history reads, links/backlinks refresh work, and forced Git sync
  // - foreground: everything else
  const auto method = req.method();
  const bool is_get_like = method == http::verb::get || method == http::verb::head;
  const bool is_card_route = is_card_path(path);
  const bool is_ai_route = path == "/ai" || path.rfind("/ai/", 0) == 0;
  const bool is_history_route = path.ends_with("/history") ||
                                path.find("/history/") != std::string::npos;
  const bool is_link_route = path.find("/links") != std::string::npos ||
                             path.find("/backlinks") != std::string::npos;
  const bool is_forced_sync_route = path.ends_with("/git/pull") || path.ends_with("/git/sync");

  if (is_card_route && !is_get_like && !is_link_route) {
    return RequestLane::Save;
  }
  if (is_ai_route || is_history_route || is_link_route || is_forced_sync_route) {
    return RequestLane::Background;
  }
  return RequestLane::Foreground;
}

const char* Session::lane_name(RequestLane lane) {
  switch (lane) {
  case RequestLane::Save:
    return "save";
  case RequestLane::Foreground:
    return "foreground";
  case RequestLane::Background:
    return "background";
  }
  return "unknown"; // LCOV_EXCL_LINE
}

} // namespace holder::api
