#pragma once

#include <filesystem>
#include <memory>
#include <mutex>

namespace holder::platform {

class ProfileLock {
 public:
  enum class Mode { SharedInProcess, Exclusive };

  static std::unique_ptr<ProfileLock> try_acquire(const std::filesystem::path& path, Mode mode);
  ~ProfileLock();

  ProfileLock(const ProfileLock&) = delete;
  ProfileLock& operator=(const ProfileLock&) = delete;

  // Access the initialization state while holding this mutex.
  std::mutex& initialization_mutex();
  bool initialized() const;
  void mark_initialized();

 private:
  struct State;
  struct Registry;
  static Registry& registry();

  ProfileLock() = default;

  std::shared_ptr<State> state_;
};

} // namespace holder::platform
