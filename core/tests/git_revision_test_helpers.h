#pragma once

#include "core_test_helpers.h"

#include <git2.h>

#include <memory>
#include <string>
#include <utility>

namespace holder::test {

// These two commit messages, with the empty tree and fixed identities below,
// produce distinct SHA-1 objects sharing the prefix afeb6d7f. Keep the actual
// objects in the repository so libgit2 performs its normal ambiguity checks.
inline std::pair<std::string, std::string> write_ambiguous_commit_prefix(
    const std::filesystem::path& root
) {
  git_repository* raw_repo = nullptr;
  REQUIRE(git_repository_open(&raw_repo, root.string().c_str()) == 0);
  const std::unique_ptr<git_repository, decltype(&git_repository_free)> repo(raw_repo, git_repository_free);
  git_odb* raw_odb = nullptr;
  REQUIRE(git_repository_odb(&raw_odb, repo.get()) == 0);
  const std::unique_ptr<git_odb, decltype(&git_odb_free)> odb(raw_odb, git_odb_free);
  git_oid tree{};
  REQUIRE(git_odb_write(&tree, odb.get(), "", 0, GIT_OBJECT_TREE) == 0);
  const std::string tree_oid = git_oid_tostr_s(&tree);
  REQUIRE(tree_oid == "4b825dc642cb6eb9a060e54bf8d69288fbee4904");
  std::pair<std::string, std::string> result;
  std::string* outputs[] = {&result.first, &result.second};
  const int nonces[] = {660, 52535};
  for (int index = 0; index < 2; ++index) {
    const auto content = "tree " + tree_oid +
        "\nauthor Holder <holder@example.invalid> 1 +0000\n"
        "committer Holder <holder@example.invalid> 1 +0000\n\nrevision collision " +
        std::to_string(nonces[index]) + "\n";
    git_oid oid{};
    REQUIRE(git_odb_write(&oid, odb.get(), content.data(), content.size(), GIT_OBJECT_COMMIT) == 0);
    *outputs[index] = git_oid_tostr_s(&oid);
    REQUIRE(outputs[index]->substr(0, 8) == "afeb6d7f");
  }
  REQUIRE(result.first != result.second);
  return result;
}

} // namespace holder::test
