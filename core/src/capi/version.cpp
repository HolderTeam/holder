#include "api/Contract.h"

#ifndef HOLDER_VERSION_STRING
#define HOLDER_VERSION_STRING "0.0.0"
#endif

#ifndef HOLDER_VERSION_MAJOR
#define HOLDER_VERSION_MAJOR 0
#endif

#ifndef HOLDER_VERSION_MINOR
#define HOLDER_VERSION_MINOR 0
#endif

#ifndef HOLDER_VERSION_PATCH
#define HOLDER_VERSION_PATCH 0
#endif

holder::contract::CoreVersion holder::contract::version() {
  return {HOLDER_VERSION_STRING, HOLDER_VERSION_MAJOR, HOLDER_VERSION_MINOR, HOLDER_VERSION_PATCH};
}
