#pragma once

// RIG supplies reflection attributes only while generating the contract.
// Ordinary daemon builds do not need RIG or a C++26 compiler.
#ifdef RIG_GENERATING
#include <rig/route.hpp>
#else
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
#endif
