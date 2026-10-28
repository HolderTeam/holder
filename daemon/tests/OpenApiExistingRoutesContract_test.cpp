#if __has_include(<catch2/catch_test_macros.hpp>)
#include <catch2/catch_test_macros.hpp>
#else
#include <catch2/catch.hpp>
#endif

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {

YAML::Node load_openapi() { return YAML::LoadFile(OPENAPI_YAML_PATH); }

bool schema_has_type(const YAML::Node& schema, const std::string& expected) {
  const auto type = schema["type"];
  if (type.IsScalar()) return type.as<std::string>() == expected;
  if (type.IsSequence()) {
    for (const auto& item : type) {
      if (item.as<std::string>() == expected) return true;
    }
  }
  return false;
}

YAML::Node parameter_named(const YAML::Node& operation, const std::string& name) {
  for (const auto& parameter : operation["parameters"]) {
    if (parameter["name"].as<std::string>() == name) return parameter;
  }
  return {};
}

std::vector<std::string> required_properties(const YAML::Node& schema) {
  std::vector<std::string> required;
  for (const auto& property : schema["required"]) {
    required.push_back(property.as<std::string>());
  }
  std::sort(required.begin(), required.end());
  return required;
}

void require_json_response_ref(
    const YAML::Node& operation,
    const std::string& status,
    const std::string& schema_name
) {
  const auto response = operation["responses"][status];
  REQUIRE(response.IsDefined());
  REQUIRE(
      response["content"]["application/json"]["schema"]["$ref"].as<std::string>() ==
      "#/components/schemas/" + schema_name
  );
}

// The data of one named event in an operation's text/event-stream itemSchema.
YAML::Node sse_event_data(const YAML::Node& operation, const std::string& name) {
  const auto events =
      operation["responses"]["200"]["content"]["text/event-stream"]["itemSchema"]["oneOf"];
  for (const auto& event : events) {
    if (event["properties"]["event"]["const"].as<std::string>() == name)
      return event["properties"]["data"];
  }
  return YAML::Node();
}

} // namespace

TEST_CASE("OpenAPI contracts card item mutations on the card path", "[openapi][cards]") {
  const auto document = load_openapi();
  const auto card = document["paths"]["/projects/{project_id}/cards/{card_id}"];
  for (const auto* method : {"patch", "delete"}) {
    const auto operation = card[method];
    REQUIRE(operation.IsDefined());
    for (const auto* name : {"project_id", "card_id"}) {
      const auto parameter = parameter_named(operation, name);
      REQUIRE(parameter.IsDefined());
      CHECK(parameter["in"].as<std::string>() == "path");
      CHECK(parameter["required"].as<bool>());
    }
    // The contract's update_card and trash_card answer with the card.
    require_json_response_ref(operation, "200", "CardResponse");
    for (const auto* status : {"401", "404"}) {
      require_json_response_ref(operation, status, "ErrorResponse");
    }
  }
  CHECK(
      card["patch"]["requestBody"]["content"]["application/json"]["schema"]["$ref"].as<std::string>(
      ) == "#/components/schemas/CardUpdate"
  );
  const auto backlinks = document["paths"]["/ai/messages/{message_id}/backlinks"];
  CHECK_FALSE(backlinks["patch"].IsDefined());
  CHECK_FALSE(backlinks["delete"].IsDefined());
}

TEST_CASE(
    "OpenAPI contracts binary asset content metadata and structured failures",
    "[openapi][resources][export]"
) {
  const auto document = load_openapi();
  const auto op = document["paths"]["/resources/{resource_id}/assets/{asset_id}/content"]["get"];
  CHECK(parameter_named(op, "resource_id")["required"].as<bool>());
  CHECK(parameter_named(op, "asset_id")["required"].as<bool>());
  CHECK(
      op["responses"]["200"]["headers"]["Content-Type"]["schema"]["type"].as<std::string>() ==
      "string"
  );
  CHECK(
      op["responses"]["200"]["headers"]["Content-Disposition"]["schema"]["type"].as<std::string>(
      ) == "string"
  );
  // OpenAPI 3.2 describes raw bytes by their media type alone.
  CHECK(op["responses"]["200"]["content"]["application/octet-stream"].IsMap());
  CHECK_FALSE(op["responses"]["200"]["content"]["application/octet-stream"]["schema"].IsDefined());
  for (const auto& code : {"400", "401", "404", "409", "422", "502", "503", "507"}) {
    require_json_response_ref(op, code, "ErrorResponse");
  }
}

TEST_CASE(
    "OpenAPI contracts project tags and exact card tag filtering",
    "[openapi][holderctl-foundation][tags]"
) {
  const auto document = load_openapi();
  const auto schemas = document["components"]["schemas"];

  const auto list_tags = document["paths"]["/projects/{project_id}/tags"]["get"];
  REQUIRE(list_tags.IsDefined());
  const auto project_id = parameter_named(list_tags, "project_id");
  REQUIRE(project_id.IsDefined());
  CHECK(project_id["in"].as<std::string>() == "path");
  CHECK(project_id["required"].as<bool>());
  require_json_response_ref(list_tags, "200", "TagCountListResponse");
  require_json_response_ref(list_tags, "404", "ErrorResponse");

  CHECK(required_properties(schemas["TagCount"]) == std::vector<std::string>{"card_count", "tag"});
  CHECK(
      schemas["TagCountListResponse"]["properties"]["data"]["items"]["$ref"].as<std::string>() ==
      "#/components/schemas/TagCount"
  );

  const auto list_cards = document["paths"]["/cards"]["get"];
  const auto tag = parameter_named(list_cards, "tag");
  REQUIRE(tag.IsDefined());
  CHECK(tag["in"].as<std::string>() == "query");
  CHECK_FALSE(tag["required"].as<bool>());
  CHECK(tag["schema"]["type"].as<std::string>() == "string");
  CHECK(tag["description"].as<std::string>().find("Exact normalized tag") != std::string::npos);
  require_json_response_ref(list_cards, "200", "CardListResponse");
}

TEST_CASE("OpenAPI exposes card history through the shared contract", "[openapi][history]") {
  const auto document = load_openapi();
  const auto schemas = document["components"]["schemas"];
  const auto path = document["paths"]["/projects/{project_id}/history/cards/{card_id}"];
  require_json_response_ref(path["get"], "200", "CardHistoryPageResponse");

  const auto compare =
      document["paths"]["/projects/{project_id}/history/cards/{card_id}/compare"]["get"];
  REQUIRE(parameter_named(compare, "from_oid").IsDefined());
  REQUIRE(parameter_named(compare, "to_oid").IsDefined());
  require_json_response_ref(compare, "200", "CardHistoryComparisonResponse");
  CHECK(schemas["CardHistoryComparison"]["properties"]["before"].IsDefined());
  CHECK(schemas["CardHistoryComparison"]["properties"]["after"].IsDefined());

  const auto snapshot =
      document["paths"]["/projects/{project_id}/history/cards/{card_id}/snapshot"]["get"];
  require_json_response_ref(snapshot, "200", "CardHistorySnapshotResponse");
  const auto restore =
      document["paths"]["/projects/{project_id}/history/cards/{card_id}/restore"]["post"];
  require_json_response_ref(restore, "200", "CardHistoryRestoreResultResponse");
}

TEST_CASE("OpenAPI exposes Trash through the shared contract", "[openapi][trash]") {
  const auto document = load_openapi();
  const auto trash = document["paths"]["/trash"];
  REQUIRE(trash.IsDefined());
  require_json_response_ref(trash["get"], "200", "TrashItemListResponse");
  require_json_response_ref(trash["delete"], "200", "TrashEmptyResultResponse");
  REQUIRE(parameter_named(trash["get"], "project_id").IsDefined());

  const auto legacy = document["paths"]["/trash/{type}/{id}"]["delete"];
  const auto scoped = document["paths"]["/projects/{project_id}/trash/{type}/{id}"]["delete"];
  require_json_response_ref(legacy, "200", "TrashDeleteResultResponse");
  require_json_response_ref(scoped, "200", "TrashDeleteResultResponse");
  require_json_response_ref(scoped, "409", "ErrorResponse");
}

TEST_CASE("OpenAPI exposes Search through the shared contract", "[openapi][search]") {
  const auto document = load_openapi();
  const auto cards = document["paths"]["/search/cards"]["get"];
  const auto messages = document["paths"]["/search/ai"]["get"];
  require_json_response_ref(cards, "200", "SearchCardListResponse");
  require_json_response_ref(messages, "200", "SearchMessageListResponse");
  for (const auto& operation : {cards, messages}) {
    REQUIRE(parameter_named(operation, "project_id").IsDefined());
    REQUIRE(parameter_named(operation, "q").IsDefined());
    REQUIRE(parameter_named(operation, "limit").IsDefined());
    REQUIRE(parameter_named(operation, "offset").IsDefined());
  }
}

TEST_CASE("OpenAPI contracts live-card tag mutations", "[openapi][holderctl-tags][tags]") {
  const auto document = load_openapi();
  const auto path = document["paths"]["/projects/{project_id}/cards/{card_id}/tags/{tag}"];

  for (const auto& method : {"put", "delete"}) {
    const auto operation = path[method];
    REQUIRE(operation.IsDefined());
    for (const auto* name : {"project_id", "card_id", "tag"}) {
      const auto parameter = parameter_named(operation, name);
      REQUIRE(parameter.IsDefined());
      CHECK(parameter["in"].as<std::string>() == "path");
      CHECK(parameter["required"].as<bool>());
    }
    CHECK_FALSE(operation["requestBody"].IsDefined());
    // The contract's add_card_tag and remove_card_tag answer with the card.
    require_json_response_ref(operation, "200", "CardResponse");
    for (const auto& status : {"400", "401", "404"}) {
      require_json_response_ref(operation, status, "ErrorResponse");
    }
  }
  // Removing a tag a card carries only in its prose is a conflict.
  require_json_response_ref(path["delete"], "409", "ErrorResponse");
}

TEST_CASE("OpenAPI describes change feed recovery and SSE cursors", "[openapi][events]") {
  const auto document = load_openapi();
  const auto paths = document["paths"];
  const auto events = paths["/events"]["get"];
  CHECK(parameter_named(events, "Last-Event-ID")["in"].as<std::string>() == "header");
  CHECK(
      parameter_named(events, "last_revision")["schema"]["pattern"].as<std::string>() ==
      "^[0-9a-f]{40}$"
  );
  CHECK(
      sse_event_data(events, "resync_required")["contentSchema"]["$ref"].as<std::string>() ==
      "#/components/schemas/EventResyncData"
  );
  // A reconnect to a finished run or pull is told to stop with 204, which has no body.
  for (const auto* path : {"/ai/runs/{run_id}/events", "/ai/runner/pull/{job_id}/events"}) {
    const auto finished = paths[path]["get"]["responses"]["204"];
    CHECK(finished.IsDefined());
    CHECK_FALSE(finished["content"].IsDefined());
  }
  CHECK(sse_event_data(paths["/ai/runs"]["post"], "fallback").IsDefined());
  CHECK(
      sse_event_data(paths["/ai/runner/pull/{job_id}/events"]["get"], "completed")["contentSchema"]
                    ["$ref"]
                        .as<std::string>() == "#/components/schemas/RunnerPullJob"
  );
  require_json_response_ref(paths["/events/cursor"]["get"], "200", "EventCheckpointResponse");
  for (const auto* status : {"400", "401", "404", "503"})
    require_json_response_ref(events, status, "ErrorResponse");
  // A request with an operation's own method never gets a 405, so no operation lists one.
  CHECK_FALSE(events["responses"]["405"].IsDefined());
  for (const auto* path : {"/ai/runs/{run_id}/events", "/ai/runner/pull/{job_id}/events"})
    CHECK(parameter_named(paths[path]["get"], "Last-Event-ID").IsDefined());
  CHECK(
      required_properties(document["components"]["schemas"]["ChangeEventData"]) ==
      std::vector<std::string>{"deleted", "entity", "entity_id", "git_revision", "project_id"}
  );
}

TEST_CASE("OpenAPI contracts the goodbye hint", "[openapi][idle]") {
  const auto document = load_openapi();
  const auto bye = document["paths"]["/bye"]["post"];
  REQUIRE(bye.IsDefined());
  require_json_response_ref(bye, "200", "ByeResponse");
  require_json_response_ref(bye, "401", "ErrorResponse");
  REQUIRE_FALSE(bye["requestBody"].IsDefined());
}

TEST_CASE(
    "OpenAPI separates project card pages from the existing display route",
    "[openapi][card-pages]"
) {
  const auto document = load_openapi();
  const auto display = document["paths"]["/cards"]["get"];
  CHECK(
      parameter_named(display, "view")["schema"]["enum"].as<std::vector<std::string>>() ==
      std::vector<std::string>{"tree", "recent"}
  );
  for (const auto& param : display["parameters"])
    CHECK(param["name"].as<std::string>() != "after_card_id");
  const auto operation = document["paths"]["/projects/{project_id}/cards"]["get"];
  CHECK(parameter_named(operation, "project_id")["in"].as<std::string>() == "path");
  CHECK(parameter_named(operation, "project_id")["required"].as<bool>());
  CHECK(parameter_named(operation, "limit")["schema"]["maximum"].as<int>() == 5000);
  CHECK(
      parameter_named(operation, "include_child_count")["schema"]["type"].as<std::string>() ==
      "boolean"
  );
  // roots, a card ID, or null, as FastAPI writes the contract's choice.
  CHECK(parameter_named(operation, "parent")["schema"]["anyOf"].size() == 3);
  CHECK(
      parameter_named(operation, "order")["schema"]["enum"].as<std::vector<std::string>>() ==
      std::vector<std::string>{"card_id_asc", "updated_desc", "sort_key_asc"}
  );
  for (const auto& param : operation["parameters"])
    CHECK(param["name"].as<std::string>() != "include_deleted");
  require_json_response_ref(operation, "200", "CardListItemPageResponse");
  const auto breadcrumbs =
      document["paths"]["/projects/{project_id}/cards/{card_id}/breadcrumbs"]["get"];
  require_json_response_ref(breadcrumbs, "200", "CardBreadcrumbListResponse");
  CHECK(breadcrumbs["responses"]["409"].IsDefined());
  CHECK(
      required_properties(document["components"]["schemas"]["CardListItemPage"]) ==
      std::vector<std::string>{"items", "next_cursor"}
  );
}
