#include "http_test_helpers.h"
#include "platform/DetachedProcess.h"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <utility>

#ifndef _WIN32
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {
using holder::platform::DetachedProcess;
using holder::platform::DetachedProcessRequest;

constexpr char helper_path[] = DETACHED_PROCESS_HELPER_PATH;

int wait_for_exit(DetachedProcess& process) {
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(6);
  int code = -1;
  while (std::chrono::steady_clock::now() < deadline) {
    if (process.has_exited(&code)) return code;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  FAIL("Timed out waiting for detached child " << process.pid());
  return -1;
}

std::string read_log(const std::filesystem::path& path) {
  std::ifstream in(path, std::ios::binary);
  REQUIRE(in.is_open());
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
} // namespace

TEST_CASE(
    "DetachedProcess appends stdout and stderr and preserves the child across moves",
    "[detached-process]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto log = dir / "logs" / "child.log";
  const DetachedProcessRequest request{
      .executable = helper_path,
      .args = {"argument with spaces"},
      .working_dir = dir,
      .log_path = log,
  };
  std::string error;
  auto first = DetachedProcess::start(request, &error);
  INFO(error);
  REQUIRE(first);
  CHECK(first->pid() > 0);
  int code = 100;
  REQUIRE_FALSE(first->has_exited(&code));
  CHECK(code == 100);
  std::ofstream(dir / "release").put('\n');
  REQUIRE(wait_for_exit(*first) == 23);

  auto second = DetachedProcess::start(request, &error);
  REQUIRE(second);
  const auto pid = second->pid();
  *first = std::move(*second);
  CHECK(first->pid() == pid);
  CHECK(second->pid() == 0);
  CHECK(second->has_exited());
  REQUIRE(wait_for_exit(*first) == 23);
  REQUIRE(first->has_exited(&code));
  CHECK(code == 23);

  const auto contents = read_log(log);
  for (const std::string marker :
       {"stdout: argument with spaces", "stderr: argument with spaces"}) {
    CAPTURE(marker);
    const auto occurrence = contents.find(marker);
    REQUIRE(occurrence != std::string::npos);
    CHECK(contents.find(marker, occurrence + marker.size()) != std::string::npos);
  }
}

TEST_CASE("DetachedProcess reports invalid log paths and launch inputs", "[detached-process]") {
  const auto dir = holder::test::make_temp_dir();
  DetachedProcessRequest request{.executable = helper_path};
  std::string expected;
  SECTION("log destination is a directory") {
    request.log_path = dir;
    expected = "cannot open log file";
  }
  SECTION("working directory does not exist") {
    request.working_dir = dir / "missing-directory";
    expected = "working directory";
  }
  SECTION("program does not exist") {
    request.executable = dir / "missing-program";
    expected = "cannot start";
  }
  std::string error;
  REQUIRE_FALSE(DetachedProcess::start(request, &error));
  CHECK(error.find(expected) != std::string::npos);
  CHECK_FALSE(DetachedProcess::start(request, nullptr));
}

TEST_CASE("DetachedProcess can launch without a log", "[detached-process]") {
  const auto dir = holder::test::make_temp_dir();
  std::ofstream(dir / "release").put('\n');
  auto process = DetachedProcess::start(
      {.executable = helper_path, .args = {"discard this output"}, .working_dir = dir},
      nullptr
  );
  REQUIRE(process);
  CHECK(wait_for_exit(*process) == 23);
  CHECK(
      std::distance(
          std::filesystem::directory_iterator(dir),
          std::filesystem::directory_iterator{}
      ) == 1
  );
}

#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__)
TEST_CASE("DetachedProcess identifies its executable", "[detached-process]") {
  const auto path = holder::platform::current_executable_path();
  REQUIRE(path.is_absolute());
  CHECK(std::filesystem::is_regular_file(path));
}

#endif

#ifndef _WIN32
TEST_CASE("DetachedProcess reports signal termination", "[detached-process]") {
  auto process = DetachedProcess::start({.executable = helper_path, .args = {"--signal"}}, nullptr);
  REQUIRE(process);
  CHECK(wait_for_exit(*process) == 128 + SIGTERM);
}

TEST_CASE("DetachedProcess resolves only runnable files on PATH", "[detached-process]") {
  using holder::platform::find_executable_on_path;
  const auto dir = holder::test::make_temp_dir();
  const std::string name = "holder-detached-process-test-program";
  const auto rejected = dir / "rejected";
  const auto accepted = dir / "accepted";
  std::filesystem::create_directories(rejected);
  std::filesystem::create_directories(accepted);
  std::ofstream(rejected / name).put('\n');
  std::filesystem::permissions(rejected / name, std::filesystem::perms::owner_read);
  std::filesystem::copy_file(helper_path, accepted / name);
  std::filesystem::permissions(accepted / name, std::filesystem::perms::owner_all);
  holder::test::EnvGuard path("PATH", ":" + rejected.string() + ":" + accepted.string());

  CHECK(find_executable_on_path(name) == accepted / name);
  CHECK(find_executable_on_path((accepted / name).string()) == accepted / name);
  CHECK(find_executable_on_path((rejected / name).string()).empty());
  CHECK(find_executable_on_path("missing-test-program").empty());
  CHECK(find_executable_on_path("").empty());
  {
    holder::test::EnvGuard unset("PATH", std::nullopt);
    CHECK(find_executable_on_path(name).empty());
  }
}

TEST_CASE(
    "DetachedProcess reports unknown status after another caller reaps the child",
    "[detached-process]"
) {
  const auto dir = holder::test::make_temp_dir();
  std::ofstream(dir / "release").put('\n');
  auto process = DetachedProcess::start(
      {.executable = helper_path, .args = {"reaped externally"}, .working_dir = dir},
      nullptr
  );
  REQUIRE(process);
  const auto pid = static_cast<pid_t>(process->pid());
  REQUIRE(pid > 0);
  // Where Boost.Process has no pidfd (macOS), it watches SIGCHLD through a handler installed
  // without SA_RESTART, so the child's exit can interrupt this wait.
  int status = 0;
  pid_t reaped = -1;
  do {
    reaped = ::waitpid(pid, &status, 0);
  } while (reaped == -1 && errno == EINTR);
  INFO("waitpid errno: " << std::strerror(errno));
  REQUIRE(reaped == pid);
  REQUIRE(WIFEXITED(status));
  CHECK(WEXITSTATUS(status) == 23);
  CHECK(wait_for_exit(*process) == -1);
}

TEST_CASE("DetachedProcess keeps logging when standard input was closed", "[detached-process]") {
  const auto dir = holder::test::make_temp_dir();
  std::ofstream(dir / "release").put('\n');
  const auto log = dir / "child.log";
  const int saved = ::fcntl(STDIN_FILENO, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
  REQUIRE(saved >= 0);
  std::string error;
  auto process = [&] {
    struct RestoreStdin {
      int saved;
      ~RestoreStdin() {
        ::dup2(saved, STDIN_FILENO);
        ::close(saved);
      }
    } restore{saved};
    ::close(STDIN_FILENO);
    return DetachedProcess::start(
        {.executable = helper_path, .args = {"closed stdin"}, .working_dir = dir, .log_path = log},
        &error
    );
  }();
  INFO(error);
  REQUIRE(process);
  CHECK(wait_for_exit(*process) == 23);
  const auto contents = read_log(log);
  CHECK(contents.find("stdout: closed stdin") != std::string::npos);
  CHECK(contents.find("stderr: closed stdin") != std::string::npos);
}
#endif

#ifdef __linux__
TEST_CASE(
    "DetachedProcess does not leak a log descriptor when its working directory is missing",
    "[detached-process]"
) {
  const auto dir = holder::test::make_temp_dir();
  const auto descriptor_count = [] {
    return std::distance(
        std::filesystem::directory_iterator("/proc/self/fd"),
        std::filesystem::directory_iterator{}
    );
  };
  const auto before = descriptor_count();
  for (int i = 0; i < 4; ++i) {
    CHECK_FALSE(DetachedProcess::start(
        {.executable = helper_path, .working_dir = dir / "missing", .log_path = dir / "child.log"},
        nullptr
    ));
  }
  CHECK(descriptor_count() == before);
}
#endif
