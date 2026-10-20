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
#define RIG_ERROR_RESPONSE(status, body, description)
#endif
