#pragma once

#include <any>
#include <string>

namespace api_runtime {

template <typename Body>
struct success {
  int status;
  Body body;
};

// A response whose body is written by the daemon's stream server.
struct host_value {
  int status = 200;
  std::any value;
};

// A named event and its already encoded JSON data.
template <typename Json>
struct event {
  std::string name;
  Json data;
};

} // namespace api_runtime
