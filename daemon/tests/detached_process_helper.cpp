#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#endif

int main(int argc, char** argv) {
#ifndef _WIN32
  if (::getsid(0) != ::getpid()) return 90;
  if (argc == 2 && std::string_view(argv[1]) == "--signal") {
    std::raise(SIGTERM);
    return 91;
  }
#endif
  if (argc != 2) return 92;
  std::cout << "stdout: " << argv[1] << '\n';
  std::cerr << "stderr: " << argv[1] << '\n';

  // A file gate lets the parent observe the running child without a timing race.
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
  while (!std::filesystem::exists("release")) {
    if (std::chrono::steady_clock::now() >= deadline) return 93;
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }
  return 23;
}
