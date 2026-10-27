#include "commands/Commands.h"

#include "commands/Support.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <stdexcept>
#include <string>

namespace holder::cli {
namespace {

constexpr const char* move_usage =
    "Usage: holderctl move <card-reference> (--into <card-reference> | --before <card-reference> | "
    "--after <card-reference> | --top-level) [--json]";
constexpr const char* repair_usage = "Usage: holderctl repair [--dry-run] [--json]";

} // namespace

int command_move(const holder::core::Paths& paths, int argc, char* argv[]) {
  std::string card_reference;
  std::string intent;
  std::string target_reference;
  bool json_output = false;
  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--json") {
      json_output = true;
    } else if (arg == "--into" || arg == "--before" || arg == "--after") {
      if (!intent.empty() || i + 1 >= argc) throw std::runtime_error(move_usage);
      intent = arg.substr(2);
      target_reference = argv[++i];
    } else if (arg == "--top-level") {
      if (!intent.empty()) throw std::runtime_error(move_usage);
      intent = "top-level";
    } else if (arg.rfind("--", 0) == 0 || !card_reference.empty()) {
      throw std::runtime_error(move_usage);
    } else {
      card_reference = arg;
    }
  }
  if (card_reference.empty() || intent.empty()) throw std::runtime_error(move_usage);

  const auto project = require_current_project_payload(paths);
  const auto project_id = json_string(project, "project_id");
  const auto card_id =
      resolve_card_reference(paths, project_id, card_reference, CardReferenceScope::Live);
  nlohmann::json body = nlohmann::json::object();
  if (intent == "top-level") {
    // An explicit null parent is the project's top level; the card goes after the last root.
    body["intent"] = "to_end";
    body["parent_card_id"] = nullptr;
  } else {
    body["intent"] = intent;
    body["target_card_id"] =
        resolve_card_reference(paths, project_id, target_reference, CardReferenceScope::Live);
  }
  const auto payload = card_api_request(
      paths,
      boost::beast::http::verb::post,
      "/cards/" + url_encode_component(card_id) + "/move",
      body
  );
  if (json_output) {
    std::cout << payload.dump(2) << "\n";
    return 0;
  }
  const auto& data = payload.at("data");
  if (intent == "top-level") {
    std::cout << "Moved card " << display_card_id(card_id) << " to the top level\n";
  } else if (intent == "into") {
    // The move answers with the card; its new parent's title comes from the parent.
    const auto parent_id = json_string(data, "parent_card_id");
    const auto parent = card_api_request(
        paths, boost::beast::http::verb::get, "/cards/" + url_encode_component(parent_id)
    );
    std::cout << "Moved card " << display_card_id(card_id) << " into "
              << json_string(parent.at("data"), "title") << "\n";
  } else {
    std::cout << "Moved card " << display_card_id(card_id) << " " << intent << " "
              << display_card_id(json_string(body, "target_card_id")) << "\n";
  }
  return 0;
}

int command_repair(const holder::core::Paths& paths, int argc, char* argv[]) {
  bool dry_run = false;
  bool json_output = false;
  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--dry-run") {
      dry_run = true;
    } else if (arg == "--json") {
      json_output = true;
    } else {
      throw std::runtime_error(repair_usage);
    }
  }

  const auto project = require_current_project_payload(paths);
  const auto project_id = json_string(project, "project_id");
  const auto payload = card_api_request(
      paths,
      boost::beast::http::verb::post,
      "/projects/" + url_encode_component(project_id) + "/repair-hierarchy",
      {{"apply", !dry_run}}
  );
  if (json_output) {
    std::cout << payload.dump(2) << "\n";
    return 0;
  }
  const auto& data = payload.at("data");
  const auto& moved = data.at("moved");
  if (moved.empty()) {
    std::cout << "No hidden cards: every live card is reachable.\n";
  } else {
    std::cout << (dry_run ? "Would make " : "Made ") << moved.size()
              << (moved.size() == 1 ? " hidden card" : " hidden cards") << " reachable:\n";
    for (const auto& entry : moved) {
      const auto& parent = entry.at("parent_card_id");
      std::cout << "  " << display_card_id(entry.at("card_id").get<std::string>()) << " -> "
                << (parent.is_string() ? display_card_id(parent.get<std::string>())
                                       : std::string("top level"))
                << "\n";
    }
    if (dry_run) std::cout << "Run holderctl repair without --dry-run to apply.\n";
  }
  const auto& cycles = data.at("cycle_card_ids");
  if (!cycles.empty()) {
    std::cout << cycles.size() << (cycles.size() == 1 ? " card is" : " cards are")
              << " in a parent cycle and left alone; move them to fix it:\n";
    for (const auto& card_id : cycles)
      std::cout << "  " << display_card_id(card_id.get<std::string>()) << "\n";
  }
  return 0;
}

} // namespace holder::cli
