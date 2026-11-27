#include "platform/ProfileLock.h"

#include <map>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace holder::platform {

struct ProfileLock::State {
  std::filesystem::path path;
  Mode mode;
  std::mutex initialization_mutex;
  bool initialized = false;
#ifdef _WIN32
  HANDLE handle = nullptr;
#else
  int fd = -1;
  pid_t owner_pid = ::getpid();
#ifdef F_OFD_SETLK
  bool locked = false;
#endif
#endif

  bool try_lock();
  ~State();
};

struct ProfileLock::Registry {
  std::mutex mutex;
  std::map<std::filesystem::path, std::weak_ptr<State>> profiles;
};

ProfileLock::Registry& ProfileLock::registry() {
  static Registry value;
  return value;
}

std::unique_ptr<ProfileLock> ProfileLock::try_acquire(
    const std::filesystem::path& path,
    Mode mode
) {
  auto canonical_path = std::filesystem::weakly_canonical(path);
  auto lock = std::unique_ptr<ProfileLock>(new ProfileLock);
  auto& entries = registry();
  const std::lock_guard guard(entries.mutex);
  if (const auto found = entries.profiles.find(canonical_path); found != entries.profiles.end()) {
    if (auto state = found->second.lock()) {
#ifndef _WIN32
      if (state->owner_pid != ::getpid()) return nullptr;
#endif
      if (mode == Mode::Exclusive || state->mode == Mode::Exclusive) return nullptr;
      lock->state_ = std::move(state);
      return lock;
    }
    entries.profiles.erase(found);
  }

  auto state = std::make_shared<State>();
  state->path = std::move(canonical_path);
  state->mode = mode;
  if (!state->try_lock()) return nullptr;
  entries.profiles.emplace(state->path, state);
  lock->state_ = std::move(state);
  return lock;
}

ProfileLock::~ProfileLock() {
  if (!state_) return;
  auto& entries = registry();
  const std::lock_guard guard(entries.mutex);
  if (state_.use_count() == 1) entries.profiles.erase(state_->path);
  // On POSIX, closing a descriptor can release a process record lock. Keep
  // teardown serialized with opening a new descriptor for this path.
  state_.reset();
}

std::mutex& ProfileLock::initialization_mutex() { return state_->initialization_mutex; }

bool ProfileLock::initialized() const { return state_->initialized; }

void ProfileLock::mark_initialized() { state_->initialized = true; }

bool ProfileLock::State::try_lock() {
#ifdef _WIN32
  const auto file = CreateFileW(
      path.c_str(),
      GENERIC_READ | GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE,
      nullptr,
      OPEN_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      nullptr
  );
  if (file == INVALID_HANDLE_VALUE) {
    throw std::system_error(
        static_cast<int>(GetLastError()),
        std::system_category(),
        "open Holder profile lock"
    );
  }
  handle = file;
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
    if (error == ERROR_LOCK_VIOLATION) return false;
    throw std::system_error(
        static_cast<int>(error),
        std::system_category(),
        "acquire Holder profile lock"
    );
  }
#else
  do {
    fd = ::open(path.c_str(), O_CREAT | O_RDWR | O_CLOEXEC, 0600);
  } while (fd == -1 && errno == EINTR);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category(), "open Holder profile lock");
  }
  // Open-file-description locks conflict with the daemon's POSIX record locks.
#ifdef F_OFD_SETLK
  constexpr int command = F_OFD_SETLK;
#else
  constexpr int command = F_SETLK;
#endif
  struct flock region {};
  region.l_type = F_WRLCK;
  region.l_whence = SEEK_SET;
  int result;
  do {
    result = ::fcntl(fd, command, &region);
  } while (result == -1 && errno == EINTR);
  if (result == -1) {
    const int error = errno;
    if (error == EACCES || error == EAGAIN) return false;
    throw std::system_error(error, std::generic_category(), "acquire Holder profile lock");
  }
#ifdef F_OFD_SETLK
  locked = true;
#endif
#endif
  return true;
}

ProfileLock::State::~State() {
#ifdef _WIN32
  if (handle) {
    OVERLAPPED offset{};
    UnlockFileEx(handle, 0, MAXDWORD, MAXDWORD, &offset);
    CloseHandle(handle);
  }
#else
  if (fd == -1) return;
#ifdef F_OFD_SETLK
  // A forked child can still hold a descriptor for this open file description.
  // Only the process that owns the lock may release it explicitly.
  if (locked && owner_pid == ::getpid()) {
    struct flock region {};
    region.l_type = F_UNLCK;
    region.l_whence = SEEK_SET;
    int result;
    do {
      result = ::fcntl(fd, F_OFD_SETLK, &region);
    } while (result == -1 && errno == EINTR);
  }
#endif
  ::close(fd);
#endif
}

} // namespace holder::platform
