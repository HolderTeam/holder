#include "git/ExecutorGitOps.h"

namespace holder::git {

ExecutorGitOps::ExecutorGitOps(GitOps& inner, const holder::core::SerialExecutor& executor)
    : inner_(inner),
      executor_(executor) {}

void ExecutorGitOps::open_or_init(const std::filesystem::path& repo_dir) {
  executor_.call([&]() {
    inner_.open_or_init(repo_dir);
  });
}

void ExecutorGitOps::write_file(
    const std::filesystem::path& relative_path,
    const std::string& content
) {
  executor_.call([&]() {
    inner_.write_file(relative_path, content);
  });
}

void ExecutorGitOps::stage_path(const std::filesystem::path& relative_path) {
  executor_.call([&]() {
    inner_.stage_path(relative_path);
  });
}

void ExecutorGitOps::remove_path(const std::filesystem::path& relative_path) {
  executor_.call([&]() {
    inner_.remove_path(relative_path);
  });
}

void ExecutorGitOps::commit(const std::string& message) {
  executor_.call([&]() {
    inner_.commit(message);
  });
}

void ExecutorGitOps::set_remote(const std::string& name, const std::string& url) {
  executor_.call([&]() {
    inner_.set_remote(name, url);
  });
}

void ExecutorGitOps::remove_remote(const std::string& name) {
  executor_.call([&]() {
    inner_.remove_remote(name);
  });
}

void ExecutorGitOps::pull_remote_ff_only(const std::string& name) {
  executor_.call([&]() {
    inner_.pull_remote_ff_only(name);
  });
}

RemoteProbeResult ExecutorGitOps::probe_remote(const std::string& name) {
  return executor_.call([&]() {
    return inner_.probe_remote(name);
  });
}

RemoteProbeResult ExecutorGitOps::probe_remote_url(const std::string& url) {
  return executor_.call([&]() {
    return inner_.probe_remote_url(url);
  });
}

PushResult ExecutorGitOps::push_branch(
    const std::string& name,
    const std::string& branch,
    bool set_upstream
) {
  return executor_.call([&]() {
    return inner_.push_branch(name, branch, set_upstream);
  });
}

std::filesystem::path ExecutorGitOps::repo_dir() const {
  return executor_.call([&]() {
    return inner_.repo_dir();
  });
}

GitRepo::DivergedMergeResult ExecutorGitOps::merge_remote_taking_theirs_for_conflicts(
    const std::string& name,
    const std::string& local_oid_hex,
    const std::string& remote_oid_hex
) {
  return executor_.call([&]() {
    return inner_.merge_remote_taking_theirs_for_conflicts(name, local_oid_hex, remote_oid_hex);
  });
}

std::optional<std::string> ExecutorGitOps::read_blob_at(
    const std::string& commit_oid_hex,
    const std::filesystem::path& relative_path
) {
  return executor_.call([&]() {
    return inner_.read_blob_at(commit_oid_hex, relative_path);
  });
}

} // namespace holder::git
