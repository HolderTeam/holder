#include <holder/holder_api.h>

#include <cstddef>
#include <cstdio>
#include <new>

// GNU/LLVM linkers can fail the next allocation in the statically linked core
// without replacing the allocator in shared dependencies or the test framework.
// This executable is deliberately separate from the Catch2 test runner.
namespace {
bool fail_next_allocation = false;
bool throw_nonstandard = false;
} // namespace

extern "C" void* __real__Znwm(std::size_t size);
extern "C" void* __wrap__Znwm(std::size_t size) {
  if (fail_next_allocation) {
    fail_next_allocation = false;
    if (throw_nonstandard) throw 42;
    throw std::bad_alloc();
  }
  return __real__Znwm(size);
}

int main() {
  int failures = 0;
  for (int api = 0; api < 4; ++api) {
    int destroyed = 0;
    auto destroy = [](void* data) {
      ++*static_cast<int*>(data);
    };
    holder_error* error = nullptr;
    fail_next_allocation = true;
    int result = HOLDER_OK;
    switch (api) {
    case 0:
      result = holder_register_storage_provider(
          HOLDER_REGISTER_STORAGE_PROVIDER_KIND_S3_COMPATIBLE,
          nullptr,
          &destroyed,
          destroy,
          &error
      );
      break;
    case 1:
      result = holder_register_keyring_provider(nullptr, &destroyed, destroy, &error);
      break;
    case 2:
      result = holder_register_ssh_signer(
          nullptr,
          nullptr,
          nullptr,
          0,
          nullptr,
          &destroyed,
          destroy,
          &error
      );
      break;
    case 3:
      result = holder_register_time_zone_provider(nullptr, &destroyed, destroy, &error);
      break;
    }
    const bool injected = !fail_next_allocation;
    fail_next_allocation = false;
    holder_error_destroy(error);
    if (!injected || result != HOLDER_ERROR_ALLOCATION || destroyed != 1) {
      std::fprintf(
          stderr,
          "API %d: injected=%d, result=%d, cleanup calls=%d\n",
          api,
          injected,
          result,
          destroyed
      );
      ++failures;
    }
  }
  // Failing arena bookkeeping must stay inside the C ABI, release its temporary
  // result, and leave the caller's arena usable for a later successful call.
  holder_arena* arena = holder_arena_new();
  if (!arena) return 1;
  for (int mode = 0; mode < 2; ++mode) {
    const holder_core_version* version = nullptr;
    holder_error* error = nullptr;
    throw_nonstandard = mode != 0;
    fail_next_allocation = true;
    const int result = holder_version(arena, &version, &error);
    const bool injected = !fail_next_allocation;
    fail_next_allocation = false;
    throw_nonstandard = false;
    const int expected = mode == 0 ? HOLDER_ERROR_ALLOCATION : HOLDER_ERROR_RUNTIME;
    if (!injected || result != expected || version != nullptr || error == nullptr) {
      std::fprintf(
          stderr,
          "Version mode %d: injected=%d, result=%d, version=%p, error=%p\n",
          mode,
          injected,
          result,
          static_cast<const void*>(version),
          static_cast<void*>(error)
      );
      ++failures;
    }
    holder_error_destroy(error);
  }
  const holder_core_version* version = nullptr;
  holder_error* error = nullptr;
  if (holder_version(arena, &version, &error) != HOLDER_OK || !version || error) ++failures;
  holder_error_destroy(error);
  holder_arena_free(arena);
  return failures == 0 ? 0 : 1;
}
