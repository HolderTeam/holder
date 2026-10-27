// The contract's operations on a context a host makes from its own services, as a daemon worker
// does, and core's failures as the contract's error kinds.
#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include "api/Contract.h"
#include "api/CoreErrors.h"
#include "card/CardRepo.h"
#include "capi/Context.h"
#include "core_test_helpers.h"
#include "git/GitOps.h"
#include "model/ConflictError.h"
#include "privacy/PrivacyError.h"
#include "project/ProjectRepo.h"

#include <holder/holder.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace {

// The host's Git, which counts the commits made through it.
class CountingGit final : public holder::git::GitOps {
 public:
  holder::git::RealGitOps real;
  int commits = 0;
  void open_or_init(const std::filesystem::path& p) override { real.open_or_init(p); }
  void write_file(const std::filesystem::path& p, const std::string& bytes) override { real.write_file(p, bytes); }
  void stage_path(const std::filesystem::path& p) override { real.stage_path(p); }
  void remove_path(const std::filesystem::path& p) override { real.remove_path(p); }
  void commit(const std::string& message) override {
    real.commit(message);
    ++commits;
  }
  void set_remote(const std::string& n, const std::string& u) override { real.set_remote(n, u); }
  void remove_remote(const std::string& n) override { real.remove_remote(n); }
  void pull_remote_ff_only(const std::string& n) override { real.pull_remote_ff_only(n); }
  holder::git::RemoteProbeResult probe_remote(const std::string& n) override { return real.probe_remote(n); }
  holder::git::PushResult push_branch(const std::string& n, const std::string& b, bool u) override {
    return real.push_branch(n, b, u);
  }
  std::filesystem::path repo_dir() const override { return real.repo_dir(); }
};

template <class Kind>
void translated_as(holder::privacy::PrivacyErrorCode code) {
  REQUIRE_THROWS_AS(holder::contract::with_contract_errors(
                        [&]() -> int { throw holder::privacy::PrivacyError(code, "privacy"); }),
                    Kind);
}

} // namespace

TEST_CASE("The contract's operations run on a host's database, indexer and Git", "[contract]") {
  const auto dir = holder::test::make_temp_dir();
  auto db = holder::test::open_db_with_schema(dir / "holder.db");
  holder::index::FtsIndexer fts{db};
  CountingGit git;
  holder::model::Project project;
  project.project_id = "project-host";
  project.name = "Host";
  project.root_path = (dir / "repo").string();
  project.privacy_mode = "plain";
  project.created_at = project.updated_at = 1;
  holder::project::ProjectRepo(db).create(project);

  holder_context context(db, fts, &git);
  holder::contract::NewCard request;
  request.title = "Hosted";
  request.content = "Body #draft";
  const auto card = holder::contract::create_card(context, project.project_id, request);
  REQUIRE(git.commits == 1);
  REQUIRE(holder::contract::get_card(context, project.project_id, card.card_id).content == "Body #draft");
  REQUIRE(holder::card::CardRepo(db).get(card.card_id).has_value());
  const auto page = holder::contract::list_cards(context, project.project_id, std::nullopt, std::string("draft"),
                                                 holder::contract::ListCardsOrder::CardIdAsc, 10, std::nullopt,
                                                 false, false);
  REQUIRE(page.items.size() == 1);
}

TEST_CASE("Core's failures become the contract's error kinds", "[contract]") {
  using holder::privacy::PrivacyErrorCode;
  translated_as<holder::contract::KeyringUnavailable>(PrivacyErrorCode::KeyringUnavailable);
  translated_as<holder::contract::KeyMissing>(PrivacyErrorCode::KeyMaterialMissing);
  translated_as<holder::contract::DataCorrupted>(PrivacyErrorCode::EnvelopeInvalid);
  translated_as<holder::contract::DataCorrupted>(PrivacyErrorCode::EnvelopeMetadataMismatch);
  translated_as<holder::contract::DataCorrupted>(PrivacyErrorCode::CryptMetadataMissing);
  translated_as<holder::contract::DataCorrupted>(PrivacyErrorCode::EncryptionSafetyCheckFailed);
  // Failures the contract does not name stay unexpected.
  translated_as<holder::privacy::PrivacyError>(PrivacyErrorCode::PrivacyCryptoFailed);
  translated_as<holder::privacy::PrivacyError>(PrivacyErrorCode::RecoveryTokenInvalid);
  REQUIRE_THROWS_AS(holder::contract::with_contract_errors(
                        []() -> int { throw holder::model::ConflictError("card_id already exists"); }),
                    holder::contract::Conflict);
  REQUIRE(holder::contract::with_contract_errors([] { return 7; }) == 7);
}

TEST_CASE("The C API reports an encrypted project's missing key as HOLDER_ERROR_KEY_MISSING", "[capi][contract]") {
  const auto dir = holder::test::make_temp_dir();
  holder::test::EnvGuard keystore("HOLDER_TEST_KEYSTORE_DIR", (dir / "keystore").string());
  std::ifstream schema_file(SCHEMA_SQL_PATH);
  const std::string schema((std::istreambuf_iterator<char>(schema_file)), std::istreambuf_iterator<char>());
  holder_context* context = nullptr;
  holder_error* error = nullptr;
  holder_arena* arena = holder_arena_new();
  REQUIRE(holder_context_open((dir / "data").string().c_str(), schema.c_str(), &context, &error) == HOLDER_OK);
  char* json = nullptr;
  REQUIRE(holder_project_create(context, "Secret", nullptr, "encrypted_git", &json, &error) == HOLDER_OK);
  const auto project_id = nlohmann::json::parse(json).at("project_id").get<std::string>();
  holder_string_free(json);
  holder_new_card request{};
  request.title = "Sealed";
  request.content = "Secret body";
  const holder_card* card = nullptr;
  REQUIRE(holder_create_card(context, project_id.c_str(), &request, arena, &card, &error) == HOLDER_OK);
  const std::string card_id = card->card_id;

  {
    holder::test::EnvGuard elsewhere("HOLDER_TEST_KEYSTORE_DIR", (dir / "empty-keystore").string());
    REQUIRE(holder_get_card(context, project_id.c_str(), card_id.c_str(), arena, &card, &error) ==
            HOLDER_ERROR_KEY_MISSING);
    holder_error_destroy(error);
    error = nullptr;
    const holder_card_list_item_page* page = nullptr;
    REQUIRE(holder_list_cards(context, project_id.c_str(), {}, nullptr, HOLDER_LIST_CARDS_ORDER_DEFAULT, nullptr,
                              nullptr, true, false, arena, &page, &error) == HOLDER_ERROR_KEY_MISSING);
    holder_error_destroy(error);
    error = nullptr;
    REQUIRE(holder_list_cards(context, project_id.c_str(), {}, nullptr, HOLDER_LIST_CARDS_ORDER_DEFAULT, nullptr,
                              nullptr, false, false, arena, &page, &error) == HOLDER_OK);
    REQUIRE(holder_create_card(context, project_id.c_str(), &request, arena, &card, &error) ==
            HOLDER_ERROR_KEY_MISSING);
    holder_error_destroy(error);
    error = nullptr;
    // A link whose card cannot be written is not made: the index changes with the file or not at all.
    holder_new_link link{};
    link.target_type = HOLDER_NEW_LINK_TARGET_TYPE_CARD;
    link.target_id = card_id.c_str();
    const holder_link* made = nullptr;
    REQUIRE(holder_add_card_link(context, project_id.c_str(), card_id.c_str(), &link, arena, &made, &error) ==
            HOLDER_ERROR_KEY_MISSING);
    holder_error_destroy(error);
    error = nullptr;
  }
  const holder_link_list* links = nullptr;
  REQUIRE(holder_list_card_links(context, project_id.c_str(), card_id.c_str(), true, arena, &links, &error) ==
          HOLDER_OK);
  REQUIRE(links->count == 0);
  REQUIRE(holder_get_card(context, project_id.c_str(), card_id.c_str(), arena, &card, &error) == HOLDER_OK);
  REQUIRE(std::string(card->content) == "Secret body");
  holder_arena_free(arena);
  holder_context_destroy(context);
}
