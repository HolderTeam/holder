#pragma once

#include "model/Location.h"
#include "resource/LocationBindingStore.h"
#include "resource/StorageProvider.h"

#include <memory>
#include <string>

namespace holder::api::support {

// The provider for a location and this device's binding to it. A declaration or binding that
// cannot make one is a StorageError with the InvalidConfiguration code.
std::unique_ptr<holder::resource::StorageProvider> storage_provider(
    const holder::model::Location& location,
    const holder::resource::LocationBinding& binding
);

// The provider object key for a key relative to the location, under its configured prefix.
std::string location_object_key(
    const holder::model::Location& location,
    const std::string& relative_key
);

} // namespace holder::api::support
