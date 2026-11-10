// Exercise mutex failure at the C ABI boundary without changing production calls.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "capi/StorageProviders.h"
#include "platform/time_zone/TimeZones.h"
#include "privacy/PlatformKeyring.h"

#include <holder/holder_api.h>
#include <pthread.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <system_error>

namespace {
thread_local bool fail_next_lock = false;

struct RejectLock {
  RejectLock() { fail_next_lock = true; }
  ~RejectLock() { fail_next_lock = false; }
  bool consumed() const { return !fail_next_lock; }
};

enum class Service { TimeZone, Keyring, Storage };
struct State {
  int destroyed = 0;
  int calls = 0;
};
using Owner = std::shared_ptr<State>;

State& state(void* data) { return **static_cast<Owner*>(data); }
void destroy(void* data) {
  std::unique_ptr<Owner> owner(static_cast<Owner*>(data));
  ++(*owner)->destroyed;
}

int register_provider(Service service, const Owner& owner, holder_error** error) {
  // Ownership of this handle transfers even when registration fails.
  auto* data = new Owner(owner);
  switch (service) {
  case Service::TimeZone: {
    const holder_time_zone_provider_callbacks callbacks{
        +[](void* raw, const char*, int64_t, int64_t* value, int* present, char**) {
          ++state(raw).calls;
          *value = 60;
          *present = 1;
          return 0;
        },
        +[](void* raw, char** value, char**) {
          ++state(raw).calls;
          *value = strdup("Test/Zone");
          return 0;
        }
    };
    return holder_register_time_zone_provider(&callbacks, data, destroy, error);
  }
  case Service::Keyring: {
    const holder_keyring_provider_callbacks callbacks{
        +[](void* raw, int32_t, const char*, const char*, const char*, char** value, char**) {
          ++state(raw).calls;
          *value = strdup("probe-secret");
          return 0;
        },
        +[](void* raw,
            int32_t,
            const char*,
            const char*,
            const char*,
            const char*,
            const char*,
            char**) {
          ++state(raw).calls;
          return 0;
        },
        +[](void* raw, int32_t, const char*, const char*, const char*, char**) {
          ++state(raw).calls;
          return 0;
        }
    };
    return holder_register_keyring_provider(&callbacks, data, destroy, error);
  }
  case Service::Storage: {
    const holder_storage_provider_callbacks callbacks{
        +[](void* raw, const char*, const char*, int64_t, const char*, int*, char**) {
          ++state(raw).calls;
          return 0;
        },
        +[](void* raw, const char*, const char*, int*, char**) {
          ++state(raw).calls;
          return 0;
        },
        +[](void* raw, const char*, int* value, int*, char**) {
          ++state(raw).calls;
          *value = 1;
          return 0;
        },
        +[](void* raw, const char*, int*, char**) {
          ++state(raw).calls;
          return 0;
        }
    };
    return holder_register_storage_provider(
        HOLDER_REGISTER_STORAGE_PROVIDER_KIND_S3_COMPATIBLE,
        &callbacks,
        data,
        destroy,
        error
    );
  }
  }
  delete data;
  return HOLDER_ERROR_INVALID_ARGUMENT;
}

void probe(Service service) {
  switch (service) {
  case Service::TimeZone:
    CHECK(holder::platform::time_zone_provider()->utc_offset("Test/Zone", 0) == 60);
    break;
  case Service::Keyring: {
    const auto result = holder::privacy::platform_keyring_lookup_secret(
        {holder::privacy::PlatformKeyringSecretKind::GenericSecret,
         "test-service",
         "account",
         std::nullopt}
    );
    CHECK(result.secret == "probe-secret");
    CHECK_FALSE(result.error_message.has_value());
    break;
  }
  case Service::Storage: {
    holder_context context;
    CHECK(holder::capi::storage_provider(context, "s3_compatible")->exists("probe-object"));
    break;
  }
  }
}
} // namespace

extern "C" int __real_pthread_mutex_lock(pthread_mutex_t*);
extern "C" int __wrap_pthread_mutex_lock(pthread_mutex_t* mutex) {
  if (fail_next_lock) {
    fail_next_lock = false;
    return EAGAIN;
  }
  return __real_pthread_mutex_lock(mutex);
}

TEST_CASE(
    "Failed host provider registration retains the old provider and releases callback ownership",
    "[host-registration][dependency-failure]"
) {
  Service service = Service::TimeZone;
  SECTION("time zone") { service = Service::TimeZone; }
  SECTION("keyring") { service = Service::Keyring; }
  SECTION("storage") { service = Service::Storage; }

  const auto previous = std::make_shared<State>();
  const auto rejected = std::make_shared<State>();
  const auto replacement = std::make_shared<State>();
  holder_error* raw_error = nullptr;
  REQUIRE(register_provider(service, previous, &raw_error) == HOLDER_OK);
  std::unique_ptr<holder_error, decltype(&holder_error_destroy)> error(
      raw_error,
      holder_error_destroy
  );
  CHECK(error == nullptr);
  CHECK(previous->calls == 0);

  int status = HOLDER_OK;
  bool injected = false;
  {
    RejectLock fault;
    status = register_provider(service, rejected, &raw_error);
    injected = fault.consumed();
  }
  error.reset(raw_error);
  CHECK(injected);
  CHECK(status == HOLDER_ERROR_RUNTIME);
  REQUIRE(error != nullptr);
  const std::string message = holder_error_message(error.get());
  CHECK(message.find(std::generic_category().message(EAGAIN)) != std::string::npos);
  CHECK(rejected->destroyed == 1);
  CHECK(rejected->calls == 0);
  CHECK(previous->destroyed == 0);
  probe(service);
  CHECK(previous->calls == 1);
  CHECK(rejected->calls == 0);

  // Disable injection and publish a new provider through the same C API.
  error.reset();
  REQUIRE(register_provider(service, replacement, &raw_error) == HOLDER_OK);
  error.reset(raw_error);
  CHECK(error == nullptr);
  CHECK(previous->destroyed == 1);
  CHECK(rejected->destroyed == 1);
  CHECK(replacement->destroyed == 0);
  CHECK(replacement->calls == 0);
  probe(service);
  CHECK(replacement->calls == 1);
  CHECK(previous->calls == 1);

  // Replace again to observe successful-provider cleanup before the process exits.
  REQUIRE(register_provider(service, std::make_shared<State>(), &raw_error) == HOLDER_OK);
  error.reset(raw_error);
  CHECK(error == nullptr);
  CHECK(replacement->destroyed == 1);
  CHECK(previous->destroyed == 1);
  CHECK(rejected->destroyed == 1);
}
