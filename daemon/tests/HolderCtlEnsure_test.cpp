#include "ProcessTestSupport.h"
#include "TestCommand.h"
#include "http_test_helpers.h"

#include "commands/Common.h"
#include "commands/Commands.h"
#include "commands/Ensure.h"
#include "platform/ServerInfo.h"
#include "storage_http_test_server.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <csignal>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace {

using holder::cli::ApiRange;
using holder::cli::check_api_compatibility;
using holder::cli::compare_api_versions;
using holder::cli::parse_api_version;

std::vector<unsigned long> version(const std::string& text) {
  const auto parsed = parse_api_version(text);
  REQUIRE(parsed.has_value());
  return *parsed;
}

} // namespace

namespace {

std::filesystem::path make_dirs(const std::filesystem::path& path) {
  std::filesystem::create_directories(path);
  return path;
}

} // namespace

TEST_CASE("the daemon starts beside its binary when nothing says otherwise", "[ensure][workdir]") {
  const auto root = holder::test::make_temp_dir();
  make_dirs(root / "build");
  REQUIRE(holder::cli::default_daemon_working_dir(root / "build" / "holderd") == root / "build");
}

TEST_CASE("an installed layout starts the daemon in share/holder-daemon", "[ensure][workdir]") {
  const auto root = holder::test::make_temp_dir();
  make_dirs(root / "bin");
  make_dirs(root / "share" / "holder-daemon");
  REQUIRE(
      holder::cli::default_daemon_working_dir(root / "bin" / "holderd") ==
      root / "share" / "holder-daemon"
  );
}

TEST_CASE("a macOS app bundle starts the daemon in Contents/Resources", "[ensure][workdir]") {
  const auto root = holder::test::make_temp_dir();
  const auto resources = root / "Holder.app" / "Contents" / "Resources";
  make_dirs(resources / "bin");
  make_dirs(resources / "schema");
  make_dirs(resources / "config");
  REQUIRE(holder::cli::default_daemon_working_dir(resources / "bin" / "holderd") == resources);
}

TEST_CASE("a Windows install starts the daemon in the install directory", "[ensure][workdir]") {
  // <install>\bin\holderd.exe, with schema\, config\ and assets\ in <install>.
  const auto install = holder::test::make_temp_dir() / "Programs" / "Holder";
  make_dirs(install / "bin");
  make_dirs(install / "schema");
  make_dirs(install / "config");
  make_dirs(install / "assets");
  REQUIRE(holder::cli::default_daemon_working_dir(install / "bin" / "holderd.exe") == install);
}

TEST_CASE("a bin directory without the daemon's data stays where it is", "[ensure][workdir]") {
  const auto root = holder::test::make_temp_dir();
  make_dirs(root / "bin");
  REQUIRE(holder::cli::default_daemon_working_dir(root / "bin" / "holderd") == root / "bin");

  // Only one of the two data directories is not enough.
  make_dirs(root / "schema");
  REQUIRE(holder::cli::default_daemon_working_dir(root / "bin" / "holderd") == root / "bin");
}

TEST_CASE("a development tree keeps starting the daemon in build/", "[ensure][workdir]") {
  // daemon/ has schema/ and config/ one level above build/, as an install has above bin/.
  const auto root = holder::test::make_temp_dir();
  make_dirs(root / "daemon" / "build");
  make_dirs(root / "daemon" / "schema");
  make_dirs(root / "daemon" / "config");
  REQUIRE(
      holder::cli::default_daemon_working_dir(root / "daemon" / "build" / "holderd") ==
      root / "daemon" / "build"
  );
}

TEST_CASE("ensure parses dotted API versions", "[ensure]") {
  REQUIRE(parse_api_version("0.1") == std::optional<std::vector<unsigned long>>({0, 1}));
  REQUIRE(parse_api_version("1") == std::optional<std::vector<unsigned long>>({1}));
  REQUIRE(parse_api_version("12.0.345") == std::optional<std::vector<unsigned long>>({12, 0, 345}));

  for (const char* bad :
       {"", ".", "1.", ".1", "1..2", "a", "1.x", "1.2-rc.1", "v1", " 1", "1 ", "-1", "1234567890"
       }) {
    INFO("input: '" << bad << "'");
    REQUIRE_FALSE(parse_api_version(bad).has_value());
  }
}

TEST_CASE("ensure compares API versions numerically", "[ensure]") {
  REQUIRE(compare_api_versions(version("0.9"), version("0.10")) < 0);
  REQUIRE(compare_api_versions(version("0.10"), version("0.9")) > 0);
  REQUIRE(compare_api_versions(version("1"), version("1.0")) == 0);
  REQUIRE(compare_api_versions(version("1.0.0"), version("1")) == 0);
  REQUIRE(compare_api_versions(version("1.0.1"), version("1")) > 0);
  REQUIRE(compare_api_versions(version("2"), version("10")) < 0);
}

TEST_CASE("ensure checks the supported API range", "[ensure]") {
  SECTION("no bounds accepts anything, even an unknown version") {
    REQUIRE(check_api_compatibility("0.1", ApiRange{}).compatible);
    REQUIRE(check_api_compatibility("", ApiRange{}).compatible);
    REQUIRE(check_api_compatibility("not-a-version", ApiRange{}).compatible);
  }

  SECTION("the minimum is inclusive") {
    const ApiRange range{"0.2", ""};
    REQUIRE_FALSE(check_api_compatibility("0.1", range).compatible);
    REQUIRE(check_api_compatibility("0.2", range).compatible);
    REQUIRE(check_api_compatibility("1.0", range).compatible);
  }

  SECTION("the maximum is exclusive") {
    const ApiRange range{"", "1.0"};
    REQUIRE(check_api_compatibility("0.9", range).compatible);
    REQUIRE_FALSE(check_api_compatibility("1.0", range).compatible);
    REQUIRE_FALSE(check_api_compatibility("1.1", range).compatible);
  }

  SECTION("both bounds together") {
    const ApiRange range{"0.1", "0.3"};
    REQUIRE_FALSE(check_api_compatibility("0.0", range).compatible);
    REQUIRE(check_api_compatibility("0.1", range).compatible);
    REQUIRE(check_api_compatibility("0.2", range).compatible);
    REQUIRE_FALSE(check_api_compatibility("0.3", range).compatible);
  }

  SECTION("an unusable version is rejected only when a bound applies") {
    const ApiRange range{"0.1", ""};
    const auto missing = check_api_compatibility("", range);
    REQUIRE_FALSE(missing.compatible);
    REQUIRE(missing.reason.find("did not report") != std::string::npos);
    const auto garbled = check_api_compatibility("dev-build", range);
    REQUIRE_FALSE(garbled.compatible);
    REQUIRE(garbled.reason.find("dev-build") != std::string::npos);
  }

  SECTION("the reason names the offending and the required version") {
    const auto result = check_api_compatibility("0.1", ApiRange{"0.2", ""});
    REQUIRE(result.reason.find("0.1") != std::string::npos);
    REQUIRE(result.reason.find("0.2") != std::string::npos);
  }
}

namespace {

holder::cli::EnsureOptions parse(std::vector<std::string> args) {
  std::vector<std::string> storage = {"holderctl", "ensure"};
  storage.insert(storage.end(), args.begin(), args.end());
  std::vector<char*> argv;
  for (auto& item : storage)
    argv.push_back(item.data());
  return holder::cli::parse_ensure_options(static_cast<int>(argv.size()), argv.data());
}

int usage_exit_code(std::vector<std::string> args) {
  try {
    parse(std::move(args));
  } catch (const holder::cli::CliError& error) {
    return error.exit_code();
  }
  return -1;
}

} // namespace

TEST_CASE("ensure parses its options", "[ensure]") {
  SECTION("defaults") {
    const auto options = parse({});
    REQUIRE(options.allow_start);
    REQUIRE(options.mode == holder::cli::EnsureMode::Auto);
    REQUIRE(options.timeout == std::chrono::milliseconds(60000));
    REQUIRE(options.api.minimum.empty());
    REQUIRE(options.api.maximum_exclusive.empty());
    REQUIRE(options.daemon_args.empty());
  }

  SECTION("every option, in both spellings") {
    const auto options = parse({
        "--json",
        "--api-min",
        "0.1",
        "--api-max-exclusive=2",
        "--no-start",
        "--timeout",
        "2.5",
        "--mode=spawn",
        "--daemon",
        "/opt/holder/bin/holderd",
        "--daemon-arg",
        "--port",
        "--daemon-arg=0",
        "--idle-exit",
        "30",
        "--workdir",
        "/opt/holder/share/holder-daemon",
    });
    REQUIRE_FALSE(options.allow_start);
    REQUIRE(options.api.minimum == "0.1");
    REQUIRE(options.api.maximum_exclusive == "2");
    REQUIRE(options.timeout == std::chrono::milliseconds(2500));
    REQUIRE(options.mode == holder::cli::EnsureMode::Spawn);
    REQUIRE(options.daemon_path == std::filesystem::path("/opt/holder/bin/holderd"));
    REQUIRE(options.daemon_args == std::vector<std::string>({"--port", "0"}));
    REQUIRE(options.working_dir == std::filesystem::path("/opt/holder/share/holder-daemon"));
    REQUIRE(options.idle_exit_seconds == std::optional<int>(30));
  }

  SECTION("invalid options are usage errors with exit code 2") {
    REQUIRE(usage_exit_code({"--bogus"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--api-min"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--api-min", "one"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--api-max-exclusive", "1."}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--timeout", "0"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--timeout", "-3"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--timeout", "soon"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--timeout", "99999"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--mode", "magic"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--idle-exit", "0"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--idle-exit", "1.5"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--idle-exit", "soon"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--idle-exit", "100000"}) == holder::cli::kEnsureExitUsage);
    REQUIRE(usage_exit_code({"--daemon"}) == holder::cli::kEnsureExitUsage);
  }
}

TEST_CASE("ensure renders the documented JSON shape", "[ensure]") {
  holder::cli::EnsureResult started;
  started.ok = true;
  started.state = "started";
  started.mode = "spawned";
  started.daemon = {
      {"pid", 42},
      {"url", "http://127.0.0.1:1"},
      {"api_version", "0.1"},
      {"server_version", "0.2.1"}
  };
  started.spawned_pid = 42;
  started.idle_exit_seconds = 30;
  started.log_path = "/tmp/holderd-start.log";
  started.elapsed_ms = 120;

  const auto ok_json = holder::cli::ensure_result_to_json(started);
  REQUIRE(ok_json["ok"] == true);
  REQUIRE(ok_json["state"] == "started");
  REQUIRE(ok_json["started"] == true);
  REQUIRE(ok_json["mode"] == "spawned");
  REQUIRE(ok_json["exit_code"] == 0);
  REQUIRE(ok_json["spawned_pid"] == 42);
  REQUIRE(ok_json["idle_exit_seconds"] == 30);
  REQUIRE(ok_json["daemon"]["api_version"] == "0.1");
  REQUIRE(ok_json["log"] == "/tmp/holderd-start.log");
  REQUIRE_FALSE(ok_json.contains("error"));

  holder::cli::EnsureResult failed;
  failed.state = "failed";
  failed.exit_code = holder::cli::kEnsureExitTimeout;
  failed.error_code = "timeout";
  failed.message = "slow";

  const auto failed_json = holder::cli::ensure_result_to_json(failed);
  REQUIRE(failed_json["ok"] == false);
  REQUIRE(failed_json["started"] == false);
  REQUIRE(failed_json["exit_code"] == 13);
  REQUIRE(failed_json["error"]["code"] == "timeout");
  REQUIRE(failed_json["error"]["message"] == "slow");
  REQUIRE_FALSE(failed_json.contains("daemon"));
  REQUIRE_FALSE(failed_json.contains("mode"));
  REQUIRE_FALSE(failed_json.contains("spawned_pid"));
  REQUIRE_FALSE(failed_json.contains("idle_exit_seconds"));
}

#ifndef _WIN32

using holder::test::IsolatedHome;
using holder::test::process_alive;

TEST_CASE("ensure reports that nothing is running with --no-start", "[ensure][process]") {
  IsolatedHome home;
  const auto run = home.run("--no-start");
  REQUIRE(run.exit_code == holder::cli::kEnsureExitNotRunning);
  REQUIRE(run.json["ok"] == false);
  REQUIRE(run.json["state"] == "failed");
  REQUIRE(run.json["error"]["code"] == "not_running");
  REQUIRE(run.json["exit_code"] == 11);
  REQUIRE_FALSE(run.json.contains("spawned_pid"));
}

TEST_CASE("ensure reports a missing holderd", "[ensure][process]") {
  IsolatedHome home;
  const auto run = home.run(
      "--daemon \"" + (std::filesystem::temp_directory_path() / "no-such-holderd").string() + "\""
  );
  REQUIRE(run.exit_code == holder::cli::kEnsureExitDaemonNotFound);
  REQUIRE(run.json["error"]["code"] == "daemon_not_found");
}

TEST_CASE("ensure says why a holderd could not be started", "[ensure][process]") {
  IsolatedHome home;

  SECTION("the working directory does not exist") {
    const auto run = home.run(
        "--daemon /bin/sleep --workdir \"" + (home.root() / "no-such-dir").string() +
        "\" --timeout 5"
    );
    REQUIRE(run.exit_code == holder::cli::kEnsureExitStartFailed);
    REQUIRE(run.json["error"]["code"] == "start_failed");
    const auto message = run.json["error"]["message"].get<std::string>();
    REQUIRE(message.find("working directory") != std::string::npos);
    REQUIRE(message.find("no-such-dir") != std::string::npos);
  }

  SECTION("the program cannot be run") {
    const auto program = home.root() / "not-a-program";
    std::ofstream(program) << "not executable";
    const auto run = home.run("--daemon \"" + program.string() + "\" --timeout 5");
    REQUIRE(run.exit_code == holder::cli::kEnsureExitStartFailed);
    REQUIRE(run.json["error"]["code"] == "start_failed");
    REQUIRE(
        run.json["error"]["message"].get<std::string>().find("Permission denied") !=
        std::string::npos
    );
  }
}

TEST_CASE("ensure notices a daemon that exits before becoming healthy", "[ensure][process]") {
  IsolatedHome home;
  // holderctl itself stands in for a daemon that stops at once.
  const auto started = std::chrono::steady_clock::now();
  const auto run = home.run(
      "--daemon \"" + std::string(HOLDER_CTL_PATH) + "\" --daemon-arg version --timeout 30"
  );
  const auto elapsed = std::chrono::steady_clock::now() - started;
  REQUIRE(run.exit_code == holder::cli::kEnsureExitStartFailed);
  REQUIRE(run.json["error"]["code"] == "start_failed");
  REQUIRE(run.json["error"]["message"].get<std::string>().find("exited") != std::string::npos);
  REQUIRE(elapsed < std::chrono::seconds(20));
}

TEST_CASE("ensure times out on a daemon that never becomes healthy", "[ensure][process]") {
  IsolatedHome home;
  // A long sleep stands in for a daemon that starts but never serves.
  const auto run = home.run("--daemon /bin/sleep --daemon-arg 30 --timeout 1");
  REQUIRE(run.json.contains("spawned_pid"));
  home.track(run.json["spawned_pid"].get<long long>());
  REQUIRE(run.exit_code == holder::cli::kEnsureExitTimeout);
  REQUIRE(run.json["error"]["code"] == "timeout");
  // ensure leaves the process alone; the caller decides what to do with spawned_pid.
  REQUIRE(process_alive(run.json["spawned_pid"].get<long long>()));
}

TEST_CASE(
    "ensure starts a detached daemon, reuses it, and enforces the API range",
    "[ensure][process]"
) {
  IsolatedHome home;

  const auto first = home.run(home.daemon_arguments());
  REQUIRE(first.exit_code == 0);
  REQUIRE(first.json["ok"] == true);
  REQUIRE(first.json["state"] == "started");
  REQUIRE(first.json["mode"] == "spawned");
  const auto pid = first.json["daemon"]["pid"].get<long long>();
  REQUIRE(pid > 0);
  REQUIRE(first.json["spawned_pid"].get<long long>() == pid);
  REQUIRE(first.json["daemon"]["url"].get<std::string>().rfind("http://127.0.0.1:", 0) == 0);
  REQUIRE_FALSE(first.json["daemon"]["api_version"].get<std::string>().empty());
  REQUIRE(std::filesystem::exists(home.start_log_path()));

  // It outlived holderctl, and it has its own session and process group.
  REQUIRE(process_alive(pid));
  REQUIRE(::getsid(static_cast<pid_t>(pid)) == static_cast<pid_t>(pid));
  REQUIRE(::getpgid(static_cast<pid_t>(pid)) == static_cast<pid_t>(pid));

  const std::string api = first.json["daemon"]["api_version"].get<std::string>();

  SECTION("a second call finds the running daemon and does not start another") {
    const auto again = home.run("");
    REQUIRE(again.exit_code == 0);
    REQUIRE(again.json["state"] == "running");
    REQUIRE(again.json["mode"] == "existing");
    REQUIRE(again.json["daemon"]["pid"].get<long long>() == pid);
    REQUIRE_FALSE(again.json.contains("spawned_pid"));
  }

  SECTION("a supported range is accepted") {
    const auto in_range = home.run("--api-min 0 --api-max-exclusive 1000");
    REQUIRE(in_range.exit_code == 0);
    REQUIRE(in_range.json["state"] == "running");
  }

  SECTION("an unsupported range is refused and the daemon is left running") {
    const auto too_new = home.run("--api-min 999");
    REQUIRE(too_new.exit_code == holder::cli::kEnsureExitApiIncompatible);
    REQUIRE(too_new.json["error"]["code"] == "api_incompatible");
    REQUIRE(too_new.json["daemon"]["api_version"] == api);
    REQUIRE(process_alive(pid));

    const auto too_old = home.run("--api-max-exclusive 0.0");
    REQUIRE(too_old.exit_code == holder::cli::kEnsureExitApiIncompatible);
  }

  SECTION("--no-start sees the running daemon") {
    const auto check = home.run("--no-start");
    REQUIRE(check.exit_code == 0);
    REQUIRE(check.json["state"] == "running");
  }
}

TEST_CASE("ensure copes with a stale info file from a daemon that has gone", "[ensure][process]") {
  IsolatedHome home;
  const auto first = home.run(home.daemon_arguments());
  REQUIRE(first.exit_code == 0);
  const auto pid = first.json["daemon"]["pid"].get<long long>();

  // Kill it without letting it tidy up, leaving its info file behind.
  ::kill(static_cast<pid_t>(pid), SIGKILL);
  for (int i = 0; i < 100 && process_alive(pid); ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  REQUIRE(std::filesystem::exists(home.info_path()));

  const auto check = home.run("--no-start");
  REQUIRE(check.exit_code == holder::cli::kEnsureExitNotRunning);

  const auto restarted = home.run(home.daemon_arguments());
  REQUIRE(restarted.exit_code == 0);
  REQUIRE(restarted.json["state"] == "started");
  REQUIRE(restarted.json["daemon"]["pid"].get<long long>() != pid);
}

TEST_CASE("start starts a daemon that stays running, and reuses it", "[start][process]") {
  IsolatedHome home;
  const auto first = home.run_ctl("start", home.daemon_arguments());
  REQUIRE(first.exit_code == 0);
  REQUIRE(first.json["ok"] == true);
  REQUIRE(first.json["state"] == "started");
  REQUIRE_FALSE(first.json.contains("idle_exit_seconds"));
  REQUIRE_FALSE(first.json["daemon"].contains("idle_exit_seconds"));
  const auto pid = first.json["daemon"]["pid"].get<long long>();
  REQUIRE(process_alive(pid));

  // The info file says it is not ephemeral.
  const auto info_text = holder::test::read_file(home.info_path());
  REQUIRE(info_text.find("idle_exit_seconds") == std::string::npos);

  const auto second = home.run_ctl("start", "");
  REQUIRE(second.exit_code == 0);
  REQUIRE(second.json["state"] == "running");
  REQUIRE(second.json["daemon"]["pid"] == pid);
}

TEST_CASE("start refuses a running daemon that will stop itself", "[start][process]") {
  IsolatedHome home;
  const auto ephemeral = home.run("--idle-exit 30 " + home.daemon_arguments());
  REQUIRE(ephemeral.exit_code == 0);
  REQUIRE(ephemeral.json["daemon"]["idle_exit_seconds"] == 30);
  const auto pid = ephemeral.json["daemon"]["pid"].get<long long>();

  const auto start = home.run_ctl("start", "");
  REQUIRE(start.exit_code == holder::cli::kEnsureExitEphemeral);
  REQUIRE(start.json["ok"] == false);
  REQUIRE(start.json["error"]["code"] == "ephemeral");
  const auto message = start.json["error"]["message"].get<std::string>();
  REQUIRE(message.find(std::to_string(pid)) != std::string::npos);
  REQUIRE(message.find("30") != std::string::npos);
  // It leaves the daemon alone.
  REQUIRE(process_alive(pid));
}

TEST_CASE("ensure still accepts a daemon that will stop itself", "[start][process]") {
  IsolatedHome home;
  REQUIRE(home.run("--idle-exit 30 " + home.daemon_arguments()).exit_code == 0);
  const auto again = home.run("");
  REQUIRE(again.exit_code == 0);
  REQUIRE(again.json["state"] == "running");
  REQUIRE(again.json["daemon"]["idle_exit_seconds"] == 30);
}

TEST_CASE("status says when a daemon will stop itself", "[start][process]") {
  IsolatedHome home;
  REQUIRE(home.run("--idle-exit 45 " + home.daemon_arguments()).exit_code == 0);
  const auto status = home.run_ctl("status", "", false);
  REQUIRE(status.exit_code == 0);
  REQUIRE(status.stdout_text.find("Idle exit: stops after 45 seconds") != std::string::npos);
}

TEST_CASE("status does not mention an idle exit for a daemon that stays", "[start][process]") {
  IsolatedHome home;
  REQUIRE(home.run_ctl("start", home.daemon_arguments()).exit_code == 0);
  const auto status = home.run_ctl("status", "", false);
  REQUIRE(status.exit_code == 0);
  REQUIRE(status.stdout_text.find("Idle exit") == std::string::npos);
}

TEST_CASE("start does not take --idle-exit", "[start][process]") {
  IsolatedHome home;
  const auto run = home.run_ctl("start", "--idle-exit 5 --no-start", false);
  REQUIRE(run.exit_code == holder::cli::kEnsureExitUsage);
  REQUIRE(run.stderr_text.find("ensure --idle-exit") != std::string::npos);
}

TEST_CASE("start says in words what it did", "[start][process]") {
  IsolatedHome home;
  const auto run = home.run_ctl("start", home.daemon_arguments(), false);
  REQUIRE(run.exit_code == 0);
  REQUIRE(run.stdout_text.find("Holder daemon: started") != std::string::npos);
  REQUIRE(run.stdout_text.find("stays running until it is stopped") != std::string::npos);
  REQUIRE(run.stdout_text.find("URL: http://") != std::string::npos);
}

TEST_CASE("start reports a failure like ensure does", "[start][process]") {
  IsolatedHome home;
  const auto run = home.run_ctl("start", "--no-start");
  REQUIRE(run.exit_code == holder::cli::kEnsureExitNotRunning);
  REQUIRE(run.json["error"]["code"] == "not_running");
}

TEST_CASE("ensure --idle-exit starts a daemon that stops by itself", "[ensure][process]") {
  IsolatedHome home;
  const auto started = home.run(home.daemon_arguments() + " --idle-exit 2");
  REQUIRE(started.exit_code == 0);
  REQUIRE(started.json["state"] == "started");
  REQUIRE(started.json["idle_exit_seconds"] == 2);
  const auto pid = started.json["daemon"]["pid"].get<long long>();

  // It is still there shortly afterwards, then goes away without being asked.
  REQUIRE(process_alive(pid));
  REQUIRE(holder::test::wait_until(std::chrono::seconds(30), [&]() {
    return !process_alive(pid);
  }));

  // The next call finds nothing running and starts a fresh daemon.
  const auto again = home.run(home.daemon_arguments());
  REQUIRE(again.exit_code == 0);
  REQUIRE(again.json["state"] == "started");
  REQUIRE(again.json["daemon"]["pid"].get<long long>() != pid);
}

#endif

TEST_CASE("Ensure and start help explain their options without starting a daemon", "[ensure]") {
  const auto paths = holder::core::Paths::resolve("holder-help-test");
  for (const std::string command : {"ensure", "start"}) {
    CAPTURE(command);
    std::vector<std::string> args{"holderctl", command, "--help"};
    std::vector<char*> argv;
    for (auto& arg : args) argv.push_back(arg.data());
    std::ostringstream output;
    struct RestoreOutput {
      std::streambuf* previous;
      ~RestoreOutput() { std::cout.rdbuf(previous); }
    } restore{std::cout.rdbuf(output.rdbuf())};
    const auto code = command == "ensure"
                          ? holder::cli::command_ensure(paths, static_cast<int>(argv.size()), argv.data())
                          : holder::cli::command_start(paths, static_cast<int>(argv.size()), argv.data());
    CHECK(code == 0);
    CHECK(output.str().find("Usage: holderctl " + command) != std::string::npos);
    CHECK(output.str().find("--idle-exit") != std::string::npos);
  }
}

#ifndef _WIN32
TEST_CASE("Ensure reports an unhealthy daemon response without starting another", "[ensure][process]") {
  using Server = holder::test::StorageHttpTestServer;
  const auto dir = holder::test::make_temp_dir();
  holder::test::EnvGuard data("XDG_DATA_HOME", (dir / "data").string());
  holder::test::EnvGuard cache("XDG_CACHE_HOME", (dir / "cache").string());
  const auto paths = holder::core::Paths::resolve("holder");
  unsigned status = 503;
  std::string body = R"({"ok":true})";
  std::string expected = "HTTP 503";
  SECTION("health returned a failing HTTP status") {}
  SECTION("health declined readiness in its JSON") {
    status = 200;
    body = R"({"ok":false})";
    expected = "not ok";
  }
  SECTION("health returned malformed JSON") {
    status = 200;
    body = "{";
    expected = "parse error";
  }
  Server server([&](const Server::Request&) {
    return Server::response(status, body);
  });
  std::filesystem::create_directories(paths.info_path().parent_path());
  std::filesystem::permissions(paths.info_path().parent_path(), std::filesystem::perms::owner_all);
  holder::core::write_server_info(paths.info_path(),
      {.pid = holder::core::current_pid(), .bind = "127.0.0.1",
       .port = server.address().port(), .api_version = "0.1", .server_version = "fixture",
       .auth_token = "fixture-token"});
  holder::cli::EnsureOptions options;
  options.allow_start = false;
  const auto result = holder::cli::ensure_daemon(paths, options);
  CHECK(result.exit_code == holder::cli::kEnsureExitNotRunning);
  CHECK(result.error_code == "not_running");
  INFO(result.message);
  CHECK(result.message.find(expected) != std::string::npos);
  CHECK(result.spawned_pid == 0);
  server.stop();
  server.rethrow_error();
  REQUIRE(server.requests().size() == 1);
  CHECK(server.requests().front().target() == "/health");
  CHECK(server.requests().front()["Authorization"] == "Bearer fixture-token");
}
#endif

#ifdef __linux__
namespace {
void write_systemctl_fixture(const std::filesystem::path& path, const std::string& body) {
  std::filesystem::create_directories(path.parent_path());
  std::ofstream(path) << "#!/bin/sh\n" << body;
  std::filesystem::permissions(path, std::filesystem::perms::owner_all);
}
} // namespace

TEST_CASE("Ensure service mode reports missing tools and failed starts", "[ensure][service]") {
  IsolatedHome home;
  const auto bin = make_dirs(home.root() / "bin");
  holder::test::EnvGuard path("PATH", bin.string());
  std::string expected;
  SECTION("systemctl is absent") {
    expected = "systemctl was not found";
  }
  SECTION("systemctl exits with a failure") {
    write_systemctl_fixture(bin / "systemctl", "exit 7\n");
    expected = "failed with status 7";
  }
  SECTION("systemctl cannot be executed") {
    std::ofstream(bin / "systemctl") << "#!/missing-fixture-interpreter\n";
    std::filesystem::permissions(bin / "systemctl", std::filesystem::perms::owner_all);
    expected = "failed";
  }
  const auto run = home.run("--mode service --timeout 2");
  REQUIRE(run.exit_code == holder::cli::kEnsureExitStartFailed);
  CHECK(run.json.at("error").at("code") == "start_failed");
  CHECK(run.json.at("error").at("message").get<std::string>().find(expected) != std::string::npos);
}

TEST_CASE("Ensure detects a failed service after systemctl accepted the start", "[ensure][service]") {
  IsolatedHome home;
  const auto bin = make_dirs(home.root() / "bin");
  holder::test::EnvGuard path("PATH", bin.string());
  holder::test::EnvGuard appdir("APPDIR", std::nullopt);
  const auto calls = home.root() / "calls";
  holder::test::EnvGuard call_log("HOLDER_FIXTURE_CALLS", calls.string());
  write_systemctl_fixture(bin / "systemctl",
      "printf '%s\n' \"$*\" >> \"$HOLDER_FIXTURE_CALLS\"\n"
      "case \"$2\" in cat|start|is-failed) exit 0;; *) exit 8;; esac\n");
  const auto run = home.run("--mode auto --timeout 3");
  REQUIRE(run.exit_code == holder::cli::kEnsureExitStartFailed);
  CHECK(run.json.at("error").at("message").get<std::string>().find("journalctl") != std::string::npos);
  const auto commands = holder::test::read_file(calls);
  CHECK(commands.find("--user cat holder-daemon.service") != std::string::npos);
  CHECK(commands.find("--user start holder-daemon.service") != std::string::npos);
  CHECK(commands.find("--user is-failed --quiet holder-daemon.service") != std::string::npos);
}

TEST_CASE("Ensure service mode times out while a unit remains neither healthy nor failed",
          "[ensure][service]") {
  IsolatedHome home;
  const auto bin = make_dirs(home.root() / "bin");
  holder::test::EnvGuard path("PATH", bin.string());
  write_systemctl_fixture(bin / "systemctl",
      "case \"$2\" in start) exit 0;; is-failed) exit 1;; *) exit 8;; esac\n");
  const auto run = home.run("--mode service --timeout 1.2");
  REQUIRE(run.exit_code == holder::cli::kEnsureExitTimeout);
  CHECK(run.json.at("error").at("code") == "timeout");
  CHECK(run.json.at("error").at("message").get<std::string>().find("1.2 seconds") != std::string::npos);
}

TEST_CASE("Ensure auto mode can start a healthy daemon through its service", "[ensure][service]") {
  IsolatedHome home;
  const auto bin = make_dirs(home.root() / "bin");
  holder::test::EnvGuard path("PATH", bin.string());
  holder::test::EnvGuard appdir("APPDIR", std::nullopt);
  holder::test::EnvGuard daemon("HOLDER_FIXTURE_DAEMON", HOLDER_BIN_PATH);
  holder::test::EnvGuard workdir("HOLDER_FIXTURE_WORKDIR", IsolatedHome::source_root().string());
  write_systemctl_fixture(bin / "systemctl",
      "case \"$2\" in\n"
      " cat) exit 0;;\n"
      " start) cd \"$HOLDER_FIXTURE_WORKDIR\" || exit 8; "
      "\"$HOLDER_FIXTURE_DAEMON\" --port 0 & exit 0;;\n"
      " is-failed) exit 1;;\n"
      " *) exit 8;;\nesac\n");
  const auto run = home.run("--mode auto --timeout 15");
  REQUIRE(run.exit_code == 0);
  CHECK(run.json.at("state") == "started");
  CHECK(run.json.at("mode") == "service");
  CHECK(run.json.at("daemon").at("pid").get<long long>() > 0);
  CHECK_FALSE(run.json.contains("spawned_pid"));
  const auto words = home.run_ctl("ensure", "", false);
  CHECK(words.exit_code == 0);
  CHECK(words.stdout_text.find("Holder daemon: running") != std::string::npos);
  CHECK(words.stdout_text.find("Mode: existing") != std::string::npos);
}

TEST_CASE("Ensure locates a daemon beside holderctl before trying PATH", "[ensure][service]") {
  IsolatedHome home;
  const auto bin = make_dirs(home.root() / "bin");
  const auto sibling_dir = make_dirs(home.root() / "sibling");
  holder::test::EnvGuard path("PATH", bin.string());
  holder::test::EnvGuard appdir("APPDIR", std::nullopt);
  write_systemctl_fixture(bin / "systemctl", "exit 1\n");
  write_systemctl_fixture(bin / "holderd", "exit 4\n");
  holder::cli::EnsureOptions options;
  options.holderctl_path = sibling_dir / "holderctl";
  options.timeout = std::chrono::seconds(2);
  int expected_status = 4;
  SECTION("PATH supplies the executable") {}
  SECTION("the sibling takes precedence") {
    write_systemctl_fixture(sibling_dir / "holderd", "exit 7\n");
    expected_status = 7;
  }
  SECTION("an AppImage skips the service check") {
    holder::test::EnvGuard bundled("APPDIR", home.root().string());
    write_systemctl_fixture(bin / "systemctl", "exit 0\n");
    const auto result = holder::cli::ensure_daemon(holder::core::Paths::resolve("holder"), options);
    CHECK(result.exit_code == holder::cli::kEnsureExitStartFailed);
    CHECK(result.message.find("status 4") != std::string::npos);
    return;
  }
  const auto result = holder::cli::ensure_daemon(holder::core::Paths::resolve("holder"), options);
  CHECK(result.exit_code == holder::cli::kEnsureExitStartFailed);
  CHECK(result.message.find("status " + std::to_string(expected_status)) != std::string::npos);
}
#endif

#ifndef _WIN32
TEST_CASE("Ensure prints its start log when an executable cannot run", "[ensure][process]") {
  IsolatedHome home;
  const auto program = home.root() / "not-runnable";
  std::ofstream(program) << "fixture";
  const auto result = home.run_ctl("ensure", "--daemon \"" + program.string() + "\"", false);
  REQUIRE(result.exit_code == holder::cli::kEnsureExitStartFailed);
  CHECK(result.stderr_text.find("Holder daemon: failed (start_failed)") != std::string::npos);
  CHECK(result.stderr_text.find("Log: " + home.start_log_path().string()) != std::string::npos);
}
#endif
