#include "platform/ProfileLock.h"

#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <mutex>
#include <set>
#include <unistd.h>
#endif

namespace holder::platform {

#if !defined(_WIN32) && !defined(F_OFD_SETLK)
namespace {

// POSIX record locks are per process. Reserve the canonical path before opening
// it because closing even an extra descriptor for that file releases the lock.
std::mutex profiles_mutex;
std::set<std::filesystem::path> open_profiles;

} // namespace
#endif

std::unique_ptr<ProfileLock> ProfileLock::try_acquire(const std::filesystem::path& path) {
  auto lock = std::unique_ptr<ProfileLock>(new ProfileLock);
#ifdef _WIN32
  const auto handle = CreateFileW(
      path.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      nullptr
  );
  if (handle == INVALID_HANDLE_VALUE) {
    throw std::system_error(
        static_cast<int>(GetLastError()),
        std::system_category(),
        "open Holder profile lock"
    );
  }
  lock->handle_ = handle;
  OVERLAPPED offset{};
  // Match Boost.Interprocess file_lock, which the daemon uses for holder.lock.
  if (!LockFileEx(
          handle,
          LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
          0,
          MAXDWORD,
          MAXDWORD,
          &offset
      )) {
    const auto error = GetLastError();
    if (error == ERROR_LOCK_VIOLATION) return nullptr;
    throw std::system_error(
        static_cast<int>(error),
        std::system_category(),
        "acquire Holder profile lock"
    );
  }
#else
#ifndef F_OFD_SETLK
  auto canonical_path = std::filesystem::weakly_canonical(path);
  {
    const std::lock_guard guard(profiles_mutex);
    if (!open_profiles.insert(canonical_path).second) return nullptr;
    lock->path_.swap(canonical_path);
  }
#endif
  do {
    lock->fd_ = ::open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
  } while (lock->fd_ == -1 && errno == EINTR);
  if (lock->fd_ == -1) {
    throw std::system_error(errno, std::generic_category(), "open Holder profile lock");
  }
  // Open-file-description locks conflict with the daemon's POSIX record locks
  // and also detect another context in this process.
#ifdef F_OFD_SETLK
  constexpr int command = F_OFD_SETLK;
#else
  constexpr int command = F_SETLK;
#endif
  struct flock region{};
  region.l_type = F_WRLCK;
  region.l_whence = SEEK_SET;
  int result;
  do {
    result = ::fcntl(lock->fd_, command, &region);
  } while (result == -1 && errno == EINTR);
  if (result == -1) {
    const int error = errno;
    if (error == EACCES || error == EAGAIN) return nullptr;
    throw std::system_error(error, std::generic_category(), "acquire Holder profile lock");
  }
#endif
  return lock;
}

ProfileLock::~ProfileLock() {
#ifdef _WIN32
  if (handle_) {
    OVERLAPPED offset{};
    UnlockFileEx(handle_, 0, MAXDWORD, MAXDWORD, &offset);
    CloseHandle(handle_);
  }
#elif defined(F_OFD_SETLK)
  if (fd_ != -1) ::close(fd_);
#else
  if (path_.empty()) return;
  const std::lock_guard guard(profiles_mutex);
  if (fd_ != -1) ::close(fd_);
  open_profiles.erase(path_);
#endif
}

} // namespace holder::platform
