#pragma once

// RIG supplies reflection attributes only while generating the contract.
// Ordinary daemon builds do not need RIG or a C++26 compiler.
#ifdef RIG_GENERATING
#include <rig/result.hpp>
#include <rig/route.hpp>
#else
#include <any>
#include <string>

#define RIG_ROUTE(method, path)
#define RIG_SUMMARY(text)
#define RIG_DESCRIPTION(text)
#define RIG_RESPONSE_DESCRIPTION(text)
#define RIG_BOOL_VALUE(value)
#define RIG_ERROR_RESPONSE(status, body, description, ...)
#define RIG_OPEN_RECORD
#define RIG_INLINE_RECORD
#define RIG_CONTEXT
#define RIG_QUERY
#define RIG_RANGE(minimum, maximum)
#define RIG_INT_VALUES(...)
#define RIG_STRING_VALUES(...)
#define RIG_PARAMETER_DESCRIPTION(text)
#define RIG_MINIMUM(value)
#define RIG_MIN_LENGTH(value)
#define RIG_MAX_LENGTH(value)
#define RIG_FORMAT(text)
#define RIG_PATTERN(text)
#define RIG_VALUE_TYPE
#define RIG_MIN_PROPERTIES(value)
#define RIG_SUCCESS_RESPONSE(status, description)
#define RIG_OPERATION_ID(text)
#define RIG_RESPONSE_HEADER(name, description)
#define RIG_HEADER(name)
#define RIG_SSE_EVENT(name, body)
#define RIG_DEFAULT(value)
#define RIG_FIELD_DESCRIPTION(text)
#define RIG_OMIT_EMPTY
#define RIG_NULLABLE
#define RIG_WRITE_ONLY
#define RIG_API(title, version)
#define RIG_SERVER(url)
#define RIG_BEARER_AUTH(name, format)
#define RIG_VALIDATION_ERROR(body, description)
#define RIG_UNAUTHORIZED(body, description)
#define RIG_PUBLIC
#define RIG_HOST_RESPONSE(media_type)
#define RIG_SUCCESS_STATUS(status)
#define RIG_INTERNAL_ERROR(body, description)
#define RIG_METHOD_NOT_ALLOWED(body, description)

namespace rig {

// RIG's rig::success: a success body with the status the operation chose from its declared ones.
template <typename Body>
struct success {
  int status;
  Body body;
};

// RIG's rig::host_value: the status a RIG_HOST_RESPONSE operation chose, and what the host
// needs to write the body itself.
struct host_value {
  int status = 200;
  std::any value;
};

// RIG's rig::event: a declared event's name and encoded data. Build it with the generated
// rig_events::<name>(data), which pairs each declared name with its declared type.
template <typename Json> struct event {
  std::string name;
  Json data;
};

} // namespace rig
#endif
