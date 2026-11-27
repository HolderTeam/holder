#pragma once

namespace holder::privacy {

// While one is open on this thread, each project key is fetched from the platform keyring once
// and kept until the outermost one closes, which wipes them. An operation that encrypts or
// decrypts many files (trashing a card with its children, a sync merge) then asks the keyring
// once, not once per file: on Android each lookup goes through the Keystore and takes 35-70 ms.
// Every C API call and contract operation opens one, so no key outlives the call that fetched it.
class ProjectKeyScope {
 public:
  ProjectKeyScope();
  ~ProjectKeyScope();
  ProjectKeyScope(const ProjectKeyScope&) = delete;
  ProjectKeyScope& operator=(const ProjectKeyScope&) = delete;
};

} // namespace holder::privacy
