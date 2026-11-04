#pragma once

#include "holder_api.h"

#ifdef __cplusplus
extern "C" {
#endif

// Host callbacks awaiting generated bindings (see issue #364).

// -- Asset storage providers --
//
// A Location's `provider` field (e.g. "local_directory", "google-drive") selects which
// storage provider actually holds an Asset's bytes; import_asset_file and retrieve_asset_file
// (holder_api.h) look one up by that name. "local_directory" always resolves to
// a built-in filesystem provider rooted at data_dir/resource-store, with no
// registration required -- named to match holder-desktop's own local-storage Location
// provider string, since Locations are portable/git-synced. Any other provider
// name must be registered first via holder_storage_provider_register -- there is
// deliberately no bundled network client in this library; the host supplies one, exactly
// like holder_register_keyring_provider does for platform keyrings.

typedef enum {
  HOLDER_STORAGE_ERROR_UNAVAILABLE = 0,
  HOLDER_STORAGE_ERROR_AUTHENTICATION = 1,
  HOLDER_STORAGE_ERROR_PERMISSION = 2,
  HOLDER_STORAGE_ERROR_CAPACITY = 3,
  HOLDER_STORAGE_ERROR_INTEGRITY = 4,
  HOLDER_STORAGE_ERROR_CONFLICT = 5,
  HOLDER_STORAGE_ERROR_INVALID_CONFIGURATION = 6,
  HOLDER_STORAGE_ERROR_TRANSIENT = 7,
} holder_storage_error_code;

// Each callback returns 0 on success. On failure, return nonzero and set *out_error_code
// (one of the holder_storage_error_code values above) and, optionally, *out_error
// (malloc'd) with a human-readable message.
typedef int (*holder_storage_put_fn)(
    void* user_data,
    const char* object_key,
    const char* staged_file_path,
    long long stored_size,
    const char* stored_sha256,
    int* out_error_code,
    char** out_error
);
typedef int (*holder_storage_get_fn)(
    void* user_data,
    const char* object_key,
    const char* destination_file_path,
    int* out_error_code,
    char** out_error
);
// Must return 0 and set *out_exists on success (whether or not the object is present --
// that is not a failure). Return nonzero, with *out_error_code/*out_error set, only for a
// real failure to determine existence (e.g. the provider is unreachable).
typedef int (*holder_storage_exists_fn)(
    void* user_data,
    const char* object_key,
    int* out_exists,
    int* out_error_code,
    char** out_error
);
typedef int (*holder_storage_remove_fn)(
    void* user_data,
    const char* object_key,
    int* out_error_code,
    char** out_error
);

// Installs (or replaces) the process-wide storage provider for provider_name -- not scoped
// to a holder_context, matching holder_register_keyring_provider.
//
// OWNERSHIP: unconditional from the moment this is called, exactly like
// holder_register_keyring_provider -- destroy_user_data runs exactly once, either immediately if
// this call fails for any reason, or after a subsequent registration replaces this
// provider and all in-flight asset operations using it finish (or at process exit).
// Replacement does not interrupt active operations. Cleanup runs without the registry
// lock held, so it may register another provider when triggered by replacement.
int holder_storage_provider_register(
    const char* provider_name,
    holder_storage_put_fn put_fn,
    holder_storage_get_fn get_fn,
    holder_storage_exists_fn exists_fn,
    holder_storage_remove_fn remove_fn,
    void* user_data,
    holder_destroy_fn destroy_user_data,
    holder_error** out_error
);

#ifdef __cplusplus
}
#endif
