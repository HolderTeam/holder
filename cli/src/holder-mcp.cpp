#include <utility>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <nlohmann/json.hpp>

#include <charconv>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <string_view>
#include <thread>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

namespace {
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using asio::ip::tcp;
using nlohmann::json;
using namespace std::chrono_literals;

constexpr auto modern = "2026-07-28";
constexpr auto legacy = "2025-11-25";
constexpr auto max_frame = 64 * 1024;
constexpr auto max_pending = 16;

json error(const json& id, int code, std::string_view message) {
  return {{"jsonrpc", "2.0"}, {"id", id}, {"error", {{"code", code}, {"message", message}}}};
}

json success(const json& id, json result) {
  return {{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}};
}

bool valid_id(const json& id) { return id.is_string() || id.is_number_integer(); }

void send(const json& message) { std::cout << message.dump() << '\n' << std::flush; }

struct Pending {
  beast::tcp_stream stream;
  bool cancelled = false;

  explicit Pending(asio::io_context& io)
      : stream(io) {}

  void cancel() {
    cancelled = true;
    boost::system::error_code ignored;
    stream.socket().close(ignored);
  }
};

class Bridge {
 public:
  Bridge(asio::io_context& io, tcp::endpoint endpoint, std::string token)
      : io_(io),
        endpoint_(endpoint),
        token_(std::move(token)) {}

  void receive(std::string_view frame) {
    json request;
    try {
      request = json::parse(frame, [](int depth, json::parse_event_t, json&) {
        if (depth > 32) throw std::invalid_argument("Request nesting limit exceeded.");
        return true;
      });
    } catch (const std::exception&) {
      send(error(nullptr, -32700, "Invalid JSON frame."));
      return;
    }
    if (!request.is_object() || request.value("jsonrpc", json()) != "2.0" ||
        !request.value("method", json()).is_string()) {
      send(error(nullptr, -32600, "Invalid JSON-RPC request."));
      return;
    }
    const auto method = request.at("method").get<std::string>();
    const auto id = request.value("id", json());
    auto params = request.value("params", json::object());
    if (!request.contains("id")) {
      if (method == "notifications/cancelled" && params.is_object()) {
        const auto found = pending_.find(params.value("requestId", json()));
        if (found != pending_.end()) found->second->cancel();
      } else if (method == "notifications/initialized" && state_ == State::awaiting_notification) {
        state_ = State::ready;
      }
      return;
    }
    if (!valid_id(id)) {
      send(error(nullptr, -32600, "Invalid JSON-RPC request ID."));
      return;
    }
    if (!params.is_object()) {
      send(error(id, -32602, "Parameters must be an object."));
      return;
    }
    if (pending_.contains(id)) {
      // The original call still owns this ID.
      return;
    }

    const bool initialize = method == "initialize";
    if (initialize) {
      if (state_ != State::fresh || !params.value("protocolVersion", json()).is_string() ||
          !params.value("clientInfo", json()).is_object() ||
          !params.value("capabilities", json()).is_object()) {
        send(error(id, -32602, "Invalid initialize request."));
        return;
      }
    } else if (method == "ping" && !params.contains("_meta")) {
      send(success(id, json::object()));
      return;
    } else if (method != "server/discover" && method != "tools/list" && method != "tools/call") {
      send(error(id, -32601, "Method not found."));
      return;
    }

    const bool translate = initialize || state_ == State::ready;
    if (state_ == State::awaiting_notification || state_ == State::initializing) {
      send(error(id, -32600, "MCP initialization is incomplete."));
      return;
    }
    if (translate) {
      auto meta = params.value("_meta", json::object());
      if (!meta.is_object()) {
        send(error(id, -32602, "MCP metadata must be an object."));
        return;
      }
      if (initialize) {
        meta["io.modelcontextprotocol/clientInfo"] = params.at("clientInfo");
        meta["io.modelcontextprotocol/clientCapabilities"] = params.at("capabilities");
        params = json::object();
      } else {
        meta.update(legacy_meta_);
      }
      meta["io.modelcontextprotocol/protocolVersion"] = modern;
      params["_meta"] = std::move(meta);
    }
    const auto meta = params.value("_meta", json());
    const auto version = meta.is_object()
                             ? meta.value("io.modelcontextprotocol/protocolVersion", json())
                             : json();
    if (!version.is_string()) {
      send(error(id, -32602, "MCP protocol version is required."));
      return;
    }
    if (version != modern) {
      auto failure = error(id, -32022, "Unsupported protocol version");
      failure["error"]["data"] = {{"supported", json::array({modern})}, {"requested", version}};
      send(failure);
      return;
    }

    std::string name;
    if (method == "tools/call") {
      const auto tool = params.value("name", json());
      if (!tool.is_string()) {
        send(error(id, -32602, "A tool name is required."));
        return;
      }
      name = tool.get<std::string>();
      if (name.empty() || name.size() > 128 ||
          name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_.-"
          ) != std::string::npos) {
        send(error(id, -32602, "Invalid tool name."));
        return;
      }
    }
    if (pending_.size() >= max_pending) {
      send(error(id, -32000, "Too many pending MCP requests."));
      return;
    }
    if (initialize) state_ = State::initializing;
    auto pending = std::make_shared<Pending>(io_);
    pending_.emplace(id, pending);
    asio::co_spawn(
        io_,
        forward(pending, id, initialize ? "server/discover" : method, name, params),
        [this, pending, id, translate, initialize, meta](std::exception_ptr failure, json reply) {
          pending_.erase(id);
          if (initialize) state_ = State::fresh;
          if (pending->cancelled) return;
          if (failure) {
            send(error(id, -32000, "Could not reach the selected Holder MCP endpoint."));
            return;
          }
          if (translate && reply.contains("result")) {
            auto& result = reply.at("result");
            if (initialize) {
              const auto metadata = result.value("_meta", json());
              if (!metadata.is_object() ||
                  !metadata.value("io.modelcontextprotocol/serverInfo", json()).is_object() ||
                  !result.value("capabilities", json()).is_object()) {
                send(error(id, -32000, "The selected daemon returned invalid MCP discovery."));
                return;
              }
              result = {
                  {"protocolVersion", legacy},
                  {"capabilities", result.at("capabilities")},
                  {"serverInfo", metadata.at("io.modelcontextprotocol/serverInfo")}
              };
              legacy_meta_ = {
                  {"io.modelcontextprotocol/clientInfo",
                   meta.at("io.modelcontextprotocol/clientInfo")},
                  {"io.modelcontextprotocol/clientCapabilities",
                   meta.at("io.modelcontextprotocol/clientCapabilities")}
              };
              state_ = State::awaiting_notification;
            } else {
              result.erase("resultType");
              result.erase("ttlMs");
              result.erase("cacheScope");
            }
          }
          send(reply);
        }
    );
  }

  void close() {
    for (auto& [id, pending] : pending_)
      pending->cancel();
  }

 private:
  enum class State { fresh, initializing, awaiting_notification, ready };

  asio::awaitable<json> forward(
      std::shared_ptr<Pending> pending,
      json id,
      std::string method,
      std::string name,
      json params
  ) const {
    if (pending->cancelled) co_return json();
    auto& stream = pending->stream;
    stream.expires_after(5s);
    co_await stream.async_connect(endpoint_, asio::use_awaitable);
    http::request<http::string_body> request{http::verb::post, "/mcp", 11};
    const auto address = endpoint_.address().to_string();
    const auto host = endpoint_.address().is_v6() ? "[" + address + "]" : address;
    request.set(http::field::host, host + ":" + std::to_string(endpoint_.port()));
    request.set(http::field::authorization, "Bearer " + token_);
    request.set(http::field::accept, "application/json, text/event-stream");
    request.set(http::field::content_type, "application/json");
    request.set("MCP-Protocol-Version", modern);
    request.set("Mcp-Method", method);
    if (!name.empty()) request.set("Mcp-Name", name);
    request.keep_alive(false);
    request.body(
    ) = json{{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", params}}.dump();
    request.prepare_payload();
    stream.expires_after(5s);
    co_await http::async_write(stream, request, asio::use_awaitable);

    beast::flat_buffer buffer;
    http::response_parser<http::string_body> response;
    response.body_limit(2 * 1024 * 1024);
    stream.expires_after(5s);
    co_await http::async_read(stream, buffer, response, asio::use_awaitable);
    const auto status = response.get().result();
    if (status == http::status::unauthorized || status == http::status::forbidden) {
      co_return error(id, -32000, "Holder MCP access was rejected. Check the endpoint and token.");
    }
    const auto reply = json::parse(response.get().body(), nullptr, false);
    if (!reply.is_object() || reply.value("jsonrpc", json()) != "2.0" ||
        reply.value("id", json()) != id ||
        reply.value("result", json()).is_object() == reply.value("error", json()).is_object()) {
      co_return error(id, -32000, "The selected daemon did not return a matching MCP response.");
    }
    co_return reply;
  }

  asio::io_context& io_;
  tcp::endpoint endpoint_;
  std::string token_;
  std::map<json, std::shared_ptr<Pending>> pending_;
  json legacy_meta_ = json::object();
  State state_ = State::fresh;
};

// Submit one stdio frame at a time to the I/O thread so the submission queue stays bounded.
// An accepted call runs asynchronously and can be cancelled by a later frame.
template <typename Action> void submit(asio::io_context& io, Action action) {
  std::promise<void> completed;
  auto done = completed.get_future();
  asio::post(io, [action = std::move(action), completed = std::move(completed)]() mutable {
    try {
      action();
      completed.set_value();
    } catch (...) {
      completed.set_exception(std::current_exception());
    }
  });
  done.get();
}

int run(tcp::endpoint endpoint, std::string token) {
  std::cin.tie(nullptr);
  asio::io_context io;
  auto work = asio::make_work_guard(io);
  Bridge bridge(io, endpoint, std::move(token));
  std::thread worker([&io] {
    io.run();
  });
  std::string line;
  bool oversized = false;
  try {
    for (int character; (character = std::cin.get()) != EOF;) {
      if (character == '\n') {
        submit(io, [&] {
          if (oversized) {
            send(error(nullptr, -32600, "MCP frame exceeds the size limit."));
          } else {
            bridge.receive(line);
          }
        });
        line.clear();
        oversized = false;
      } else if (line.size() < max_frame) {
        line.push_back(static_cast<char>(character));
      } else {
        oversized = true;
      }
    }
  } catch (const std::exception&) {
    std::cerr << "holder-mcp: could not process the input stream.\n";
  }
  submit(io, [&bridge] {
    bridge.close();
  });
  work.reset();
  worker.join();
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  constexpr auto usage = "Usage: holder-mcp --port <port> [--bind <loopback-address>]\n"
                         "Set HOLDER_MCP_TOKEN to the selected daemon's MCP token.\n";
  if (argc == 2 && std::string_view(argv[1]) == "--help") {
    std::cerr << usage;
    return 0;
  }
  unsigned short port = 0;
  std::string_view address = "127.0.0.1";
  for (int index = 1; index < argc; ++index) {
    const std::string_view flag = argv[index];
    if (flag == "--port" && index + 1 < argc) {
      const std::string_view value = argv[++index];
      const auto parsed = std::from_chars(value.data(), value.data() + value.size(), port);
      if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size()) port = 0;
    } else if (flag == "--bind" && index + 1 < argc) {
      address = argv[++index];
    } else {
      std::cerr << usage;
      return 2;
    }
  }
  boost::system::error_code ec;
  const auto bind = asio::ip::make_address(address, ec);
  const auto* token = std::getenv("HOLDER_MCP_TOKEN");
  const std::string_view credential = token ? token : "";
  constexpr auto safe = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
  if (port == 0 || ec || !bind.is_loopback() || credential.size() < 32 || credential.size() > 128 ||
      credential.find_first_not_of(safe) != std::string_view::npos) {
    std::cerr << usage;
    return 2;
  }
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  return run({bind, port}, std::string(credential));
}
