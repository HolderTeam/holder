#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "core_test_helpers.h"
#include "git/GitRepo.h"
#include "git/RevisionReferenceResolver.h"
#include "git_revision_test_helpers.h"
#include "history/CardHistory.h"
#include "model/Project.h"

#include <git2.h>

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

namespace {

using holder::git::RevisionReferenceResolver;
using holder::git::RevisionReferenceResult;
using holder::git::RevisionReferenceStatus;

void require_resolved(const RevisionReferenceResult& result, const std::string& oid) {
  REQUIRE(result.status == RevisionReferenceStatus::Resolved);
  REQUIRE(result.oid == oid);
}

void require_not_found(const RevisionReferenceResult& result) {
  REQUIRE(result.status == RevisionReferenceStatus::NotFound);
  REQUIRE_FALSE(result.oid.has_value());
}

std::string uppercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
    if (character >= 'a' && character <= 'f') {
      return static_cast<char>(character - 'a' + 'A');
    }
    return static_cast<char>(character);
  });
  return value;
}

} // namespace

TEST_CASE(
    "RevisionReferenceResolver resolves exact and unique abbreviated commit IDs",
    "[git][revision-reference]"
) {
  const auto root = holder::test::make_temp_dir() / "repo";
  holder::git::GitRepo repo;
  repo.open_or_init(root);
  repo.write_file("first.txt", "first");
  repo.stage_path("first.txt");
  repo.commit("first");
  const auto first_oid = repo.head_oid().value();
  repo.write_file("second.txt", "second");
  repo.stage_path("second.txt");
  repo.commit("second");

  RevisionReferenceResolver resolver(repo);
  require_resolved(resolver.resolve(first_oid), first_oid);
  require_resolved(resolver.resolve(uppercase(first_oid)), first_oid);
  const auto shortest_prefix = first_oid.substr(0, RevisionReferenceResolver::kMinimumPrefixLength);
  require_resolved(resolver.resolve(shortest_prefix), first_oid);
  require_resolved(resolver.resolve(uppercase(shortest_prefix)), first_oid);
}

TEST_CASE(
    "RevisionReferenceResolver enforces its own conservative hex-prefix policy",
    "[git][revision-reference]"
) {
  STATIC_REQUIRE(RevisionReferenceResolver::kMinimumPrefixLength == 8);

  const auto root = holder::test::make_temp_dir() / "repo";
  holder::git::GitRepo repo;
  repo.open_or_init(root);
  repo.write_file("card.md", "body");
  repo.stage_path("card.md");
  repo.commit("seed");
  const auto oid = repo.head_oid().value();
  RevisionReferenceResolver resolver(repo);

  require_not_found(
      resolver.resolve(oid.substr(0, RevisionReferenceResolver::kMinimumPrefixLength - 1))
  );
  require_not_found(resolver.resolve("HEAD"));
  require_not_found(resolver.resolve("1234567g"));
  require_not_found(resolver.resolve(std::string(GIT_OID_HEXSZ + 1, 'a')));
  require_not_found(resolver.resolve(std::string(GIT_OID_HEXSZ, '0')));
}

TEST_CASE(
    "RevisionReferenceResolver reports ambiguous repository-native commit prefixes",
    "[git][revision-reference]"
) {
  const auto root = holder::test::make_temp_dir() / "repo";
  holder::git::GitRepo repo;
  repo.open_or_init(root);
  repo.write_file("seed.txt", "seed");
  repo.stage_path("seed.txt");
  repo.commit("seed");

  const auto [first_oid, second_oid] = holder::test::write_ambiguous_commit_prefix(root);
  REQUIRE(repo.head_oid() != first_oid);
  REQUIRE(repo.head_oid() != second_oid);
  RevisionReferenceResolver resolver(repo);
  const auto ambiguous = resolver.resolve(
      first_oid.substr(0, RevisionReferenceResolver::kMinimumPrefixLength)
  );
  REQUIRE(ambiguous.status == RevisionReferenceStatus::Ambiguous);
  REQUIRE_FALSE(ambiguous.oid.has_value());

  holder::model::Project project;
  project.project_id = "project-history";
  project.root_path = root.string();
  project.privacy_mode = "plain";
  const auto change = holder::history::CardHistoryService().compare_change(
      project,
      "abcd-ambiguous-revision",
      first_oid.substr(0, RevisionReferenceResolver::kMinimumPrefixLength)
  );
  REQUIRE(change.revision.status == RevisionReferenceStatus::Ambiguous);
  REQUIRE_FALSE(change.revision.oid.has_value());
  REQUIRE_FALSE(change.comparison.has_value());

  require_resolved(resolver.resolve(first_oid), first_oid);
  require_resolved(resolver.resolve(second_oid), second_oid);
}

TEST_CASE("RevisionReferenceResolver requires an opened repository", "[git][revision-reference]") {
  holder::git::GitRepo repo;
  RevisionReferenceResolver resolver(repo);
  REQUIRE_THROWS_WITH(
      resolver.resolve("12345678"),
      Catch::Matchers::ContainsSubstring("GitRepo not opened")
  );
}
