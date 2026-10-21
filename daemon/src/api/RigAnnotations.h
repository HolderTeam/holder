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
#define RIG_INT_VALUES(...)
#define RIG_STRING_VALUES(...)
#define RIG_PARAMETER_DESCRIPTION(text)
#define RIG_MINIMUM(value)
#define RIG_FIELD_DESCRIPTION(text)
#define RIG_OMIT_EMPTY
#define RIG_NULLABLE
#define RIG_API(title, version)
#define RIG_SERVER(url)
#define RIG_BEARER_AUTH(name, format)
#define RIG_VALIDATION_ERROR(body, description)
#define RIG_UNAUTHORIZED(body, description)
#define RIG_PUBLIC
#endif
