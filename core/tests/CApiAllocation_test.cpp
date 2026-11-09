#include "capi/Context.h"
#include <holder/holder_api.h>

#include <git2.h>
#include <git2/sys/alloc.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <string>
#include <unistd.h>

// GNU/LLVM linkers can fail the next allocation in the statically linked core
// without replacing the allocator in shared dependencies or the test framework.
// This executable is deliberately separate from the Catch2 test runner.
namespace {
bool fail_next_allocation = false;
bool throw_nonstandard = false;
int allocations_until_failure = 1;
bool fail_next_git_allocation = false;

void* git_malloc(size_t size, const char*, int) {
  if (fail_next_git_allocation) {
    fail_next_git_allocation = false;
    return nullptr;
  }
  return std::malloc(size);
}

void* git_realloc(void* pointer, size_t size, const char*, int) {
  if (fail_next_git_allocation) {
    fail_next_git_allocation = false;
    return nullptr;
  }
  return std::realloc(pointer, size);
}
} // namespace

extern "C" void* __real__Znwm(std::size_t size);
extern "C" void* __wrap__Znwm(std::size_t size) {
  if (fail_next_allocation && --allocations_until_failure == 0) {
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
    allocations_until_failure = 1;
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
  // The first allocation takes ownership of the callback data. Fail a later
  // allocation in the runtime adapter and verify that ownership is released once.
  const holder_time_zone_provider_callbacks time_zone{
      +[](void*, const char*, int64_t, int64_t*, int*, char**) { return 0; },
      +[](void*, char**, char**) { return 0; }};
  const holder_keyring_provider_callbacks keyring{
      +[](void*, int32_t, const char*, const char*, const char*, char**, char**) { return 0; },
      +[](void*, int32_t, const char*, const char*, const char*, const char*, const char*, char**) { return 0; },
      +[](void*, int32_t, const char*, const char*, const char*, char**) { return 0; }};
  const holder_ssh_signer_callbacks signer{
      +[](void*, const unsigned char*, size_t, unsigned char**, size_t*, char**) { return 0; }};
  const holder_storage_provider_callbacks storage{
      +[](void*, const char*, const char*, int64_t, const char*, int*, char**) { return 0; },
      +[](void*, const char*, const char*, int*, char**) { return 0; },
      +[](void*, const char*, int*, int*, char**) { return 0; },
      +[](void*, const char*, int*, char**) { return 0; }};
  for (int api = 0; api < 4; ++api) {
    const auto destroyed = std::make_shared<int>(0);
    holder_context context;
    auto* data = new std::shared_ptr<int>(destroyed);
    auto destroy = +[](void* raw) {
      auto* count = static_cast<std::shared_ptr<int>*>(raw);
      ++**count;
      delete count;
    };
    holder_error* error = nullptr;
    allocations_until_failure = 2;
    fail_next_allocation = true;
    int result = HOLDER_OK;
    const unsigned char public_key[] = {1, 2, 3};
    switch (api) {
    case 0:
      result = holder_register_storage_provider(HOLDER_REGISTER_STORAGE_PROVIDER_KIND_S3_COMPATIBLE,
                                                 &storage, data, destroy, &error);
      break;
    case 1:
      result = holder_register_keyring_provider(&keyring, data, destroy, &error);
      break;
    case 2:
      result = holder_register_ssh_signer(&context, "git", public_key, sizeof(public_key), &signer,
                                          data, destroy, &error);
      break;
    case 3:
      result = holder_register_time_zone_provider(&time_zone, data, destroy, &error);
      break;
    }
    const bool injected = !fail_next_allocation;
    fail_next_allocation = false;
    if (!injected || result != HOLDER_ERROR_ALLOCATION || *destroyed != 1 || error == nullptr) {
      std::fprintf(stderr, "Adapter %d: injected=%d, result=%d, cleanup calls=%d\n",
                   api, injected, result, *destroyed);
      ++failures;
    }
    holder_error_destroy(error);
  }
  // Failing arena bookkeeping must stay inside the C ABI, release its temporary
  // result, and leave the caller's arena usable for a later successful call.
  holder_arena* arena = holder_arena_new();
  if (!arena) return 1;
  for (int mode = 0; mode < 2; ++mode) {
    const holder_core_version* version = nullptr;
    holder_error* error = nullptr;
    throw_nonstandard = mode != 0;
    allocations_until_failure = 1;
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
  // libgit2 exposes an allocator seam, so its home-path allocation failure can
  // be exercised without faking git_libgit2_opts or exhausting process memory.
  std::string git_home = "/tmp/holder-git-home-" + std::string(180, 'x') + "-XXXXXX";
  if (!mkdtemp(git_home.data())) return 1;
  git_libgit2_init();
  git_allocator failing_git_allocator{git_malloc, git_realloc, std::free};
  if (git_libgit2_opts(GIT_OPT_SET_ALLOCATOR, &failing_git_allocator) != 0) {
    std::fprintf(stderr, "Cannot install libgit2 allocator: %s\n", git_error_last()->message);
    return 1;
  }
  error = nullptr;
  fail_next_git_allocation = true;
  const int configured = holder_configure_git_home(git_home.c_str(), &error);
  const bool injected = !fail_next_git_allocation;
  fail_next_git_allocation = false;
  if (git_libgit2_opts(GIT_OPT_SET_ALLOCATOR, static_cast<git_allocator*>(nullptr)) != 0) {
    std::fprintf(stderr, "Cannot restore libgit2 allocator: %s\n", git_error_last()->message);
    return 1;
  }
  if (!injected || configured != HOLDER_ERROR_RUNTIME || error == nullptr) {
    std::fprintf(
        stderr,
        "Git home: injected=%d, result=%d, error=%p\n",
        injected,
        configured,
        static_cast<void*>(error)
    );
    ++failures;
  }
  holder_error_destroy(error);
  // libgit2 keeps an OOM sentinel in its global home-path buffer. This isolated
  // process exits here rather than assuming that restoring the allocator repairs it.
  rmdir(git_home.c_str());
  return failures == 0 ? 0 : 1;
}
