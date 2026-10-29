#include "api/generated/AiJson.h"
#include "api/generated/AiRoutes.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("AI contract codec rejects a non-object nudge facts field") {
  const nlohmann::json body = {
      {"kind", "unknown"}, {"project_id", "p1"}, {"created_at", 0}, {"facts", nlohmann::json::array()}};
  REQUIRE_THROWS_AS(ai_json::decode<holder::api::AiNudgeCandidate>(body), std::invalid_argument);
}

TEST_CASE("AI contract preserves omitted and explicit null update fields") {
  const auto omitted = ai_json::decode<holder::api::AiRunnerUpdateRequest>(nlohmann::json::object());
  REQUIRE_FALSE(omitted.base_url.has_value());

  const auto cleared = ai_json::decode<holder::api::AiRunnerUpdateRequest>(
      nlohmann::json{{"base_url", nullptr}});
  REQUIRE(cleared.base_url.has_value());
  REQUIRE_FALSE(cleared.base_url->has_value());
}

TEST_CASE("AI request body rejects negative int64 overflow without rejecting quoted text") {
  REQUIRE_THROWS_AS(ai_http::check_negative_integer_tokens(
      R"({"updated_at":-9223372036854775809})"), std::invalid_argument);
  REQUIRE_NOTHROW(ai_http::check_negative_integer_tokens(
      R"({"updated_at":-9223372036854775808.0,"note":"-9223372036854775809"})"));
}
