#include "commands/Commands.h"

#include "commands/MilestoneDateTime.h"
#include "commands/Support.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace holder::cli {
namespace {

struct MilestonesOptions {
  bool json_output = false;
  bool help = false;
  std::string card_reference;
};

enum class MilestoneAction {
  Add,
  Edit,
  Remove,
};

// A date as given on the command line: ROLE=WHEN, or WHEN for a role the option implies.
struct DateOption {
  std::string role;
  std::string when;
};

struct MilestoneOptions {
  MilestoneAction action = MilestoneAction::Add;
  bool json_output = false;
  bool help = false;
  bool clear_dates = false;
  bool clear_kind = false;
  bool clear_description = false;
  std::string card_reference;
  std::string milestone_id;
  std::vector<DateOption> dates;
  std::optional<std::string> kind;
  std::optional<std::string> description;
};

struct CalendarOptions {
  bool json_output = false;
  bool help = false;
  std::optional<std::string> from;
  std::optional<std::string> to;
};

std::string milestones_usage() { return "Usage: holderctl milestones CARD [--json]"; }

std::string milestone_usage() {
  return "Usage:\n"
         "  holderctl milestone add CARD [START] [--end WHEN] [--date ROLE=WHEN]... [--kind KIND] "
         "[--description TEXT] [--json]\n"
         "  holderctl milestone edit CARD MILESTONE_ID [[--start WHEN] [--end WHEN] [--date ROLE=WHEN]... | "
         "--clear-dates] [--kind KIND | --clear-kind] [--description TEXT | --clear-description] [--json]\n"
         "  holderctl milestone remove CARD MILESTONE_ID [--json]\n"
         "\n"
         "A milestone has any number of dates, each with a role such as start, end or due. WHEN is "
         "YYYY-MM-DD for a calendar day, or an RFC 3339 timestamp with an explicit offset for an instant. "
         "START and --start give the start role, and --end the end role. Dates keep the order given; "
         "giving dates to edit replaces all of the milestone's dates.";
}

std::string calendar_usage() {
  return "Usage: holderctl calendar [--from WHEN] [--to WHEN] [--json]\n"
         "Date-only bounds cover inclusive local calendar days. Timed bounds require RFC 3339 "
         "with an explicit offset. Missing bounds use the local day containing the other bound; "
         "with neither bound, calendar shows today.";
}

ParsedMilestoneWhen parse_cli_milestone_when(const std::string& value) {
  try {
    return parse_milestone_when(value);
  } catch (const std::invalid_argument& ex) {
    throw CliError(
        "invalid_milestone_time",
        "Invalid milestone date/time.",
        {{"value", value}, {"reason", ex.what()}}, // LCOV_EXCL_LINE: initializer cleanup duplicate.
        std::string("Invalid milestone date/time: ") + ex.what()
    );
  }
}

// A date as the API takes it: a day as given, or an instant in RFC 3339 with whole seconds and its
// own offset, so a fraction of a second is dropped.
nlohmann::json milestone_date(const DateOption& option) {
  const auto parsed = parse_cli_milestone_when(option.when);
  if (parsed.kind == MilestoneWhenKind::AllDay) return {{"role", option.role}, {"date", option.when}};
  auto offset = option.when.substr(19);
  if (offset.front() == '.') offset.erase(0, offset.find_first_not_of("0123456789", 1));
  if (offset == "z") offset = "Z";
  return {{"role", option.role},
          {"datetime", option.when.substr(0, 10) + "T" + option.when.substr(11, 8) + offset}};
}

nlohmann::json milestone_dates(const std::vector<DateOption>& options) {
  auto dates = nlohmann::json::array();
  for (const auto& option : options) dates.push_back(milestone_date(option));
  return dates;
}

CalendarRange resolve_cli_calendar_range(
    const std::optional<std::string>& from,
    const std::optional<std::string>& to
) {
  try {
    return resolve_calendar_range(from, to, now_epoch_seconds());
  } catch (const std::invalid_argument& ex) {
    nlohmann::json details{{"reason", ex.what()}};
    if (from.has_value()) details["from"] = *from;
    if (to.has_value()) details["to"] = *to;
    throw CliError(
        "invalid_calendar_range",
        "Invalid calendar range.",
        std::move(details),
        std::string("Invalid calendar range: ") + ex.what()
    );
  }
}

MilestonesOptions parse_milestones_options(int argc, char* argv[]) {
  MilestonesOptions options;
  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--json")
      options.json_output = true;
    else if (arg == "--help" || arg == "-h")
      options.help = true;
    else if (arg.rfind("--", 0) == 0)
      throw std::runtime_error("Unknown milestones option: " + arg);
    else if (options.card_reference.empty())
      options.card_reference = arg;
    else
      throw std::runtime_error(milestones_usage());
  }
  if (!options.help && options.card_reference.empty()) {
    throw std::runtime_error(milestones_usage());
  }
  return options;
}

MilestoneOptions parse_milestone_options(int argc, char* argv[]) {
  MilestoneOptions options;
  std::vector<std::string> positional;
  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--json") {
      options.json_output = true;
    } else if (arg == "--help" || arg == "-h") {
      options.help = true;
    } else if (arg == "--clear-dates") {
      options.clear_dates = true;
    } else if (arg == "--clear-kind") {
      options.clear_kind = true;
    } else if (arg == "--clear-description") {
      options.clear_description = true;
    } else if (arg == "--start" || arg == "--end" || arg == "--date" || arg == "--kind" ||
               arg == "--description") {
      if (i + 1 >= argc) throw std::runtime_error(milestone_usage());
      const std::string value = argv[++i];
      if (value.rfind("--", 0) == 0) throw std::runtime_error(milestone_usage());
      if (arg == "--start" || arg == "--end") {
        options.dates.push_back({arg.substr(2), value});
      } else if (arg == "--date") {
        const auto equals = value.find('=');
        if (equals == 0 || equals == std::string::npos) throw std::runtime_error(milestone_usage());
        options.dates.push_back({value.substr(0, equals), value.substr(equals + 1)});
      } else if (arg == "--kind") {
        options.kind = value;
      } else {
        options.description = value;
      }
    } else if (arg.rfind("--", 0) == 0) {
      throw std::runtime_error("Unknown milestone option: " + arg);
    } else {
      positional.push_back(arg);
    }
  }

  if (options.help) return options;
  if (positional.size() < 2 || (positional.front() != "add" && positional.front() != "edit" &&
                                positional.front() != "remove")) {
    throw std::runtime_error(milestone_usage());
  }
  if (positional.front() == "add") {
    options.action = MilestoneAction::Add;
    if (positional.size() > 3) throw std::runtime_error(milestone_usage());
    // START comes first.
    if (positional.size() == 3) options.dates.insert(options.dates.begin(), {"start", positional.at(2)});
  } else {
    options.action = positional.front() == "edit" ? MilestoneAction::Edit : MilestoneAction::Remove;
    if (positional.size() != 3) throw std::runtime_error(milestone_usage());
    options.milestone_id = positional.at(2);
  }
  options.card_reference = positional.at(1);
  const bool changes = !options.dates.empty() || options.clear_dates || options.kind.has_value() ||
                       options.clear_kind || options.description.has_value() || options.clear_description;
  if ((options.action == MilestoneAction::Remove && changes) ||
      (options.action == MilestoneAction::Add &&
       (options.clear_dates || options.clear_kind || options.clear_description)) ||
      (options.action == MilestoneAction::Edit && !changes) ||
      (!options.dates.empty() && options.clear_dates) || (options.kind.has_value() && options.clear_kind) ||
      (options.description.has_value() && options.clear_description)) {
    throw std::runtime_error(milestone_usage());
  }
  return options;
}

CalendarOptions parse_calendar_options(int argc, char* argv[]) {
  CalendarOptions options;
  for (int i = 2; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--json") {
      options.json_output = true;
    } else if (arg == "--help" || arg == "-h") {
      options.help = true;
    } else if (arg == "--from" || arg == "--to") {
      if (i + 1 >= argc) throw std::runtime_error(calendar_usage());
      const std::string value = argv[++i];
      if (value.empty() || value.rfind("--", 0) == 0) {
        throw std::runtime_error(calendar_usage());
      }
      if (arg == "--from")
        options.from = value;
      else
        options.to = value;
    } else if (arg.rfind("--", 0) == 0) {
      throw std::runtime_error("Unknown calendar option: " + arg);
    } else {
      throw std::runtime_error(calendar_usage());
    }
  }
  return options;
}

nlohmann::json milestone_api_request(
    const holder::core::Paths& paths,
    boost::beast::http::verb method,
    const std::string& target,
    boost::beast::http::status success,
    const std::optional<nlohmann::json>& body = std::nullopt
) {
  const auto connection = read_secure_daemon_connection(paths);
  const auto response = http_json_request(
      connection,
      method,
      target,
      // LCOV_EXCL_START: timeout selection is exercised through both request methods; GCC leaves
      // the first line of this ternary uncounted as an expression cleanup artifact.
      method == boost::beast::http::verb::get ? std::chrono::seconds(10) : std::chrono::seconds(30),
      // LCOV_EXCL_STOP
      body
  );
  if (response.status == success && response.payload.value("ok", false)) return response.payload;

  const auto fallback = "HTTP " + std::to_string(static_cast<unsigned>(response.status));
  const auto message = api_error_message(response, fallback);
  std::string code = "milestone_request_failed";
  nlohmann::json details = nlohmann::json::object();
  if (response.payload.contains("error") && response.payload.at("error").is_object()) {
    const auto& error = response.payload.at("error");
    code = json_string(error, "code", code);
    if (error.contains("details")) details = error.at("details");
  }
  throw CliError(code, message, std::move(details), message);
}

std::string optional_text(const nlohmann::json& value, const char* key) {
  if (!value.contains(key) || value.at(key).is_null()) return "";
  return value.at(key).get<std::string>();
}

// A day as it is; an instant in local time, as format_milestone_when shows it.
std::string date_text(const nlohmann::json& date) {
  if (date.contains("date")) return date.at("date").get<std::string>();
  const auto at = parse_milestone_when(date.at("datetime").get<std::string>()).epoch_seconds;
  return format_milestone_when(at, false);
}

std::string milestone_dates_text(const nlohmann::json& milestone) {
  std::string text;
  for (const auto& date : milestone.at("dates")) {
    if (!text.empty()) text += ", ";
    text += json_string(date, "role") + " " + date_text(date);
  }
  return text.empty() ? "-" : text;
}

void print_milestones(const nlohmann::json& data) {
  if (data.empty()) {
    std::cout << "No milestones.\n";
    return;
  }
  std::cout << "MILESTONE_ID\tDATES\tKIND\tDESCRIPTION\n";
  for (const auto& milestone : data) {
    std::cout << json_string(milestone, "milestone_id") << "\t" << milestone_dates_text(milestone) << "\t"
              << optional_text(milestone, "kind") << "\t" << optional_text(milestone, "description") << "\n";
  }
}

struct CalendarEvent {
  long long when = 0;
  std::string when_text;
  std::string type;
  std::string event_id;
  std::string card_id;
  std::string title;
  std::string details;
};

// One event for each of a milestone's dates in the window, and one for each card made or changed in it.
// A day sorts at its local midnight.
std::vector<CalendarEvent> calendar_events(const nlohmann::json& data, const CalendarRange& range,
                                           const std::string& first_day, const std::string& last_day) {
  std::vector<CalendarEvent> events;
  for (const auto& milestone : data.at("milestones")) {
    auto details = optional_text(milestone, "kind");
    const auto description = optional_text(milestone, "description");
    if (!description.empty()) details += (details.empty() ? "" : ": ") + description;
    for (const auto& date : milestone.at("dates")) {
      const auto parsed = parse_milestone_when(
          date.contains("date") ? date.at("date").get<std::string>() : date.at("datetime").get<std::string>());
      const bool in_window = date.contains("date")
                                 ? date.at("date").get<std::string>() >= first_day &&
                                       date.at("date").get<std::string>() <= last_day
                                 : parsed.epoch_seconds >= range.from && parsed.epoch_seconds <= range.to;
      if (!in_window) continue;
      events.push_back({
          .when = parsed.epoch_seconds,
          .when_text = date_text(date),
          .type = "milestone-" + json_string(date, "role"),
          .event_id = json_string(milestone, "milestone_id"),
          .card_id = json_string(milestone, "card_id"),
          .title = json_string(milestone, "card_title"),
          .details = details,
      });
    }
  }
  for (const auto& [key, type, at] : {std::tuple{"created_cards", "card-created", "created_at"},
                                      std::tuple{"updated_cards", "card-updated", "updated_at"}}) {
    for (const auto& card : data.at(key)) {
      const auto when = card.at(at).get<long long>();
      events.push_back({
          .when = when,
          .when_text = format_milestone_when(when, false),
          .type = type,
          .event_id = {},
          .card_id = json_string(card, "card_id"),
          .title = json_string(card, "title"),
          .details = {},
      });
    }
  }
  std::sort(events.begin(), events.end(), [](const auto& lhs, const auto& rhs) {
    return std::tie(lhs.when, lhs.type, lhs.card_id, lhs.event_id) <
           std::tie(rhs.when, rhs.type, rhs.card_id, rhs.event_id);
  });
  return events;
}

void print_calendar(const std::vector<CalendarEvent>& events) {
  if (events.empty()) {
    std::cout << "No calendar events.\n";
    return;
  }
  std::vector<std::string> card_ids;
  card_ids.reserve(events.size());
  for (const auto& event : events)
    card_ids.push_back(event.card_id);
  const auto displayed_ids = display_card_ids(card_ids);

  std::cout << "WHEN\tTYPE\tEVENT_ID\tCARD_ID\tTITLE\tDETAILS\n";
  for (std::size_t i = 0; i < events.size(); ++i) {
    const auto& event = events.at(i);
    std::cout << event.when_text << "\t" << event.type << "\t" << (event.event_id.empty() ? "-" : event.event_id)
              << "\t" << displayed_ids.at(i) << "\t" << event.title << "\t" << event.details << "\n";
  }
}

std::string milestones_path(const std::string& project_id, const std::string& card_id) {
  return card_api_path(project_id, card_id) + "/milestones";
}

} // namespace

int command_milestones(const holder::core::Paths& paths, int argc, char* argv[]) {
  const auto options = parse_milestones_options(argc, argv);
  if (options.help) {
    std::cout << milestones_usage() << "\n";
    return 0;
  }
  try {
    const auto project = require_current_project_payload(paths);
    const auto project_id = json_string(project, "project_id");
    const auto card_id =
        resolve_card_reference(paths, project_id, options.card_reference, CardReferenceScope::Live);
    const auto payload = milestone_api_request(
        paths,
        boost::beast::http::verb::get,
        milestones_path(project_id, card_id),
        boost::beast::http::status::ok
    );
    if (options.json_output)
      std::cout << payload.dump(2) << "\n";
    else
      print_milestones(payload.at("data"));
    return 0;
  } catch (const CliError&) {
    throw;
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Failed to list milestones: ") + ex.what());
  }
}

int command_milestone(const holder::core::Paths& paths, int argc, char* argv[]) {
  const auto options = parse_milestone_options(argc, argv);
  if (options.help) {
    std::cout << milestone_usage() << "\n";
    return 0;
  }
  try {
    const auto project = require_current_project_payload(paths);
    const auto project_id = json_string(project, "project_id");
    const auto card_id =
        resolve_card_reference(paths, project_id, options.card_reference, CardReferenceScope::Live);

    if (options.action == MilestoneAction::Remove) {
      const auto payload = milestone_api_request(
          paths,
          boost::beast::http::verb::delete_,
          milestones_path(project_id, card_id) + "/" + url_encode_component(options.milestone_id),
          boost::beast::http::status::ok
      );
      if (options.json_output) {
        std::cout << payload.dump(2) << "\n";
      } else {
        std::cout << "Milestone " << options.milestone_id << " is not on " << display_card_id(card_id)
                  << ".\n";
      }
      return 0;
    }

    nlohmann::json body = nlohmann::json::object();
    if (!options.dates.empty() || options.clear_dates || options.action == MilestoneAction::Add)
      body["dates"] = milestone_dates(options.dates);
    if (options.kind.has_value())
      body["kind"] = *options.kind;
    else if (options.clear_kind)
      body["kind"] = nullptr;
    if (options.description.has_value())
      body["description"] = *options.description;
    else if (options.clear_description)
      body["description"] = nullptr;

    const bool add = options.action == MilestoneAction::Add;
    const auto payload = milestone_api_request(
        paths,
        add ? boost::beast::http::verb::post : boost::beast::http::verb::patch,
        milestones_path(project_id, card_id) + (add ? "" : "/" + url_encode_component(options.milestone_id)),
        add ? boost::beast::http::status::created : boost::beast::http::status::ok,
        std::make_optional<nlohmann::json>(body)
    );
    if (options.json_output) {
      std::cout << payload.dump(2) << "\n";
    } else {
      const auto& data = payload.at("data");
      std::cout << (add ? "Added milestone " : "Updated milestone ") << json_string(data, "milestone_id")
                << (add ? " to " : " on ") << display_card_id(card_id) << ": " << milestone_dates_text(data)
                << ".\n";
    }
    return 0;
  } catch (const CliError&) {
    throw;
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Failed to update milestone: ") + ex.what());
  }
}

int command_calendar(const holder::core::Paths& paths, int argc, char* argv[]) {
  const auto options = parse_calendar_options(argc, argv);
  if (options.help) {
    std::cout << calendar_usage() << "\n";
    return 0;
  }
  try {
    const auto project = require_current_project_payload(paths);
    const auto project_id = json_string(project, "project_id");
    const auto range = resolve_cli_calendar_range(options.from, options.to);
    // The window again as the local days it covers, for milestones on calendar days.
    const auto first_day = format_milestone_when(range.from, true);
    const auto last_day = format_milestone_when(range.to, true);
    const auto target = "/projects/" + url_encode_component(project_id) +
                        "/calendar?first_instant=" + std::to_string(range.from) +
                        "&last_instant=" + std::to_string(range.to) + "&first_day=" + first_day +
                        "&last_day=" + last_day;
    const auto payload = milestone_api_request(
        paths,
        boost::beast::http::verb::get,
        target,
        boost::beast::http::status::ok
    );
    if (options.json_output)
      std::cout << payload.dump(2) << "\n";
    else
      print_calendar(calendar_events(payload.at("data"), range, first_day, last_day));
    return 0;
  } catch (const CliError&) {
    throw;
  } catch (const std::exception& ex) {
    throw std::runtime_error(std::string("Failed to read calendar: ") + ex.what());
  }
}

} // namespace holder::cli
