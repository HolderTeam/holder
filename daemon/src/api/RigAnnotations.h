#pragma once

// RIG supplies reflection attributes only while generating the contract.
// Ordinary daemon builds do not need RIG or a C++26 compiler.
#ifdef RIG_GENERATING
#include <rig/route.hpp>
#else
#define RIG_ROUTE(method, path)
#endif
