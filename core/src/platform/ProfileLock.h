#pragma once

#include <filesystem>
#include <memory>

namespace holder::platform {

class ProfileLock {
 public:
  static std::unique_ptr<ProfileLock> try_acquire(const std::filesystem::path& path);
  ~ProfileLock();

  ProfileLock(const ProfileLock&) = delete;
  ProfileLock& operator=(const ProfileLock&) = delete;

 private:
  ProfileLock() = default;

#ifdef _WIN32
  void* handle_ = nullptr;
#else
  std::filesystem::path path_;
  int fd_ = -1;
#endif
};

} // namespace holder::platform
