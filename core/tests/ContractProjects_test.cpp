// The contract's project operations on a context a host makes from its own services, with its own
// projects directory and project registry.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "project/ProjectRegistry.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace contract = holder::contract;

struct Host {
  std::filesystem::path dir = holder::test::make_temp_dir();
  holder::platform::Db db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  holder::git::RealGitOps git;
  holder_context context{db, fts, &git};

  Host() {
    context.projects_root = dir / "projects";
    context.registry_path = dir / "projects.json";
  }

  contract::Project make(const std::string& name, std::optional<std::string> project_id = std::nullopt,
                         std::optional<long long> created_at = std::nullopt) {
    contract::NewProject request;
    request.name = name;
    request.project_id = std::move(project_id);
    request.privacy_mode = contract::NewProjectPrivacyMode::Plain;
    request.created_at = created_at;
    return contract::create_project(context, request);
  }

  std::vector<std::string> names(std::optional<contract::ListProjectsOrder> order = std::nullopt,
                                 std::optional<std::string> name = std::nullopt, std::int32_t limit = 100,
                                 std::int32_t offset = 0) {
    std::vector<std::string> out;
    for (const auto& project :
         contract::list_projects(context, name, std::nullopt, std::nullopt, order, limit, offset, false))
      out.push_back(project.name);
    return out;
  }
};

// The registry keeps canonical paths, which may spell a directory differently, as macOS's /private/var.
bool registered(const Host& host, const contract::Project& project) {
  std::error_code error;
  for (const auto& root : holder::core::ProjectRegistry(host.context.registry_path).roots())
    if (std::filesystem::equivalent(root, project.root_path, error)) return true;
  return false;
}

} // namespace

TEST_CASE("Projects are made under the host's directory and known to its registry", "[contract][projects]") {
  Host host;
  const auto made = host.make("Research notes", "proj-research", 1700000000);
  REQUIRE(made.project_id == "proj-research");
  REQUIRE(made.name == "Research notes");
  REQUIRE(made.privacy_mode == contract::ProjectPrivacyMode::Plain);
  REQUIRE(made.created_at == 1700000000);
  REQUIRE(made.updated_at == 1700000000);
  REQUIRE(made.git_remote_url == std::nullopt);
  REQUIRE(std::filesystem::path(made.root_path).parent_path() == host.dir / "projects");
  REQUIRE(std::filesystem::is_regular_file(std::filesystem::path(made.root_path) / ".holder" / "project.json"));
  REQUIRE(registered(host, made));
  REQUIRE(contract::get_project(host.context, "proj-research").name == "Research notes");
  REQUIRE_THROWS_AS(host.make("Again", "proj-research"), contract::Conflict);
  contract::NewProject relative;
  relative.name = "Relative";
  relative.root_path = "notes/relative";
  REQUIRE_THROWS_AS(contract::create_project(host.context, relative), contract::InvalidArgument);
  REQUIRE_FALSE(std::filesystem::exists("notes/relative"));
  REQUIRE_THROWS_AS(contract::get_project(host.context, "no-such-project"), contract::NotFound);
}

TEST_CASE("Project settings change one at a time, and a change of nothing writes nothing", "[contract][projects]") {
  Host host;
  const auto made = host.make("Work", std::nullopt, 1700000000);
  contract::ProjectUpdate nothing;
  REQUIRE(contract::update_project(host.context, made.project_id, nothing).updated_at == made.updated_at);

  contract::ProjectUpdate remote;
  remote.git_remote_url = std::optional<std::string>("https://example.com/work.git");
  auto updated = contract::update_project(host.context, made.project_id, remote);
  REQUIRE(updated.git_remote_url == "https://example.com/work.git");
  REQUIRE(updated.name == "Work");
  REQUIRE(updated.updated_at > made.updated_at);
  // The remote is in the repository's own configuration.
  {
    // Closed again before the next update, which Windows would otherwise stop rewriting the file.
    std::ifstream config(std::filesystem::path(updated.root_path) / ".git" / "config");
    const std::string text((std::istreambuf_iterator<char>(config)), std::istreambuf_iterator<char>());
    REQUIRE(text.find("https://example.com/work.git") != std::string::npos);
  }

  contract::ProjectUpdate rename;
  rename.name = "Day job";
  rename.git_remote_url = std::optional<std::string>{};
  updated = contract::update_project(host.context, made.project_id, rename);
  REQUIRE(updated.name == "Day job");
  REQUIRE(updated.git_remote_url == std::nullopt);
  REQUIRE_THROWS_AS(contract::update_project(host.context, "no-such-project", rename), contract::NotFound);
}

TEST_CASE("Projects list with filters, orders and pages, Home first by default", "[contract][projects]") {
  Host host;
  host.make("Alpha", std::nullopt, 100);
  host.make("Home", std::nullopt, 50);
  host.make("Beta", std::nullopt, 200);
  using Order = contract::ListProjectsOrder;
  REQUIRE(host.names() == std::vector<std::string>{"Home", "Beta", "Alpha"});
  REQUIRE(host.names(Order::UpdatedAtDesc) == std::vector<std::string>{"Beta", "Alpha", "Home"});
  REQUIRE(host.names(Order::NameAsc) == std::vector<std::string>{"Alpha", "Beta", "Home"});
  REQUIRE(host.names(Order::CreatedAtAsc, std::nullopt, 1, 1) == std::vector<std::string>{"Alpha"});
  REQUIRE(host.names(std::nullopt, "ta") == std::vector<std::string>{"Beta"});
  const auto after = contract::list_projects(host.context, std::nullopt, 100, 150, std::nullopt, 100, 0, true);
  REQUIRE(after.size() == 1);
  REQUIRE(after[0].name == "Alpha");
  REQUIRE(after[0].card_count == 0);
  REQUIRE(after[0].root_card_count == 0);
}

TEST_CASE("Removing a project keeps its files and remembers the removal", "[contract][projects]") {
  Host host;
  const auto made = host.make("Old", "proj-old");
  const auto removed = contract::delete_project(host.context, "proj-old");
  REQUIRE(removed.project_id == "proj-old");
  REQUIRE_THROWS_AS(contract::get_project(host.context, "proj-old"), contract::NotFound);
  REQUIRE(std::filesystem::is_directory(made.root_path));
  REQUIRE_FALSE(registered(host, made));
  REQUIRE_THROWS_AS(contract::delete_project(host.context, "proj-old"), contract::NotFound);
}
