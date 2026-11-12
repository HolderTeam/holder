#pragma once

// The storage provider this device uses for a storage location's provider name: the built-in
// local directory under the context's data directory, or one a host registered with
// holder_register_storage_provider. Throws when none is registered for the name.

#include "capi/Context.h"
#include "resource/StorageProvider.h"

#include <memory>
#include <string>

namespace holder::capi {

std::shared_ptr<holder::resource::StorageProvider> storage_provider(holder_context& context,
                                                                    const std::string& provider_name);

// Forgets every provider hosts registered, releasing each one's user data. Registrations last for
// the process, so tests that register providers holding test-local data clear them afterwards.
void clear_storage_providers();

} // namespace holder::capi
