#include "api/routes/MilestoneRoutes.h"

#include "api/CardMilestones.h"
#include "api/generated/RuntimeJson.h"

#include "api/support/HttpResponses.h"
#include "api/support/Time.h"

#include "card/CardRepo.h"
#include "card/MilestoneRepo.h"

#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace holder::api::routes {
namespace {

namespace http = boost::beast::http;

nlohmann::json optional_string_json(const std::optional<std::string>& value) {
  return value.has_value() ? nlohmann::json(*value) : nlohmann::json(nullptr);
}


nlohmann::json milestone_with_dates_json(const holder::model::Milestone& milestone) {
  nlohmann::json dates = nlohmann::json::array();
  for (const auto& value : milestone.dates) {
    nlohmann::json item = {{"role", value.role}};
    if (value.date) item["date"] = *value.date;
    if (value.datetime) item["datetime"] = *value.datetime;
    dates.push_back(std::move(item));
  }
  return {
      {"milestone_id", milestone.milestone_id},
      {"card_id", milestone.card_id},
      {"kind", optional_string_json(milestone.kind)},
      {"description", optional_string_json(milestone.description)},
      {"created_at", milestone.created_at},
      {"updated_at", milestone.updated_at},
      {"dates", std::move(dates)},
  };
}

std::vector<holder::model::MilestoneDate> parse_milestone_dates(const nlohmann::json& body) {
  if (!body.is_array()) throw std::invalid_argument("dates must be an array.");
  std::vector<holder::model::MilestoneDate> dates;
  for (const auto& item : body) {
    if (!item.is_object()) throw std::invalid_argument("milestone date must be an object.");
    for (const auto& [name, value] : item.items()) {
      (void)value;
      if (name != "role" && name != "date" && name != "datetime")
        throw std::invalid_argument("unknown milestone date field: " + name);
    }
    holder::model::MilestoneDate date;
    date.role = item.at("role").get<std::string>();
    if (item.contains("date")) date.date = item.at("date").get<std::string>();
    if (item.contains("datetime")) date.datetime = item.at("datetime").get<std::string>();
    holder::model::validate_milestone_date(date);
    dates.push_back(std::move(date));
  }
  return dates;
}

void validate_labelled_fields(const nlohmann::json& body) {
  if (!body.is_object()) throw std::invalid_argument("milestone body must be an object.");
  for (const auto& [name, value] : body.items()) {
    (void)value;
    if (name != "dates" && name != "kind" && name != "description")
      throw std::invalid_argument("unknown milestone field: " + name);
  }
}

std::optional<std::string> nullable_string(const nlohmann::json& body, const std::string& name) {
  if (!body.contains(name) || body.at(name).is_null()) return std::nullopt;
  if (!body.at(name).is_string()) throw std::invalid_argument(name + " must be a string or null.");
  const auto value = body.at(name).get<std::string>();
  return value.empty() ? std::nullopt : std::optional<std::string>(value);
}

bool parse_card_milestone_path(
    const std::string& path,
    std::string& card_id,
    std::optional<std::string>& milestone_id,
    bool& with_dates
) {
  static const std::string prefix = "/cards/";
  static const std::string legacy_marker = "/milestones";
  static const std::string labelled_marker = "/milestones-with-dates";
  if (path.rfind(prefix, 0) != 0) return false;
  const auto marker_pos = path.find('/', prefix.size());
  if (marker_pos == std::string::npos) return false;
  with_dates = path.compare(marker_pos, labelled_marker.size(), labelled_marker) == 0;
  const auto& marker = with_dates ? labelled_marker : legacy_marker;
  if (path.compare(marker_pos, marker.size(), marker) != 0) return false;
  card_id = path.substr(prefix.size(), marker_pos - prefix.size());
  const auto suffix = path.substr(marker_pos + marker.size());
  if (suffix.empty()) {
    milestone_id.reset();
    return true;
  }
  if (suffix[0] != '/' || suffix.size() == 1 || suffix.find('/', 1) != std::string::npos) {
    return false;
  }
  milestone_id = suffix.substr(1);
  return true;
}

} // namespace

bool handle_milestone_routes(
    const std::string& path,
    const http::request<http::string_body>& req,
    http::response<http::string_body>& res,
    holder::platform::Db& db,
    holder::card::CardStore* card_store,
    const std::function<std::string()>& uuid_v4,
    const std::function<std::string(const std::string&)>& /*param_get*/
) {
  std::string card_id;
  std::optional<std::string> milestone_id;
  bool with_dates = false;
  if (!parse_card_milestone_path(path, card_id, milestone_id, with_dates)) return false;
  if (card_id.empty()) {
    res = support::error_response(http::status::bad_request, "bad_request", "Missing card_id.");
    return true;
  }

  try {
    const auto card = holder::card::CardRepo(db).get(card_id);
    if (!card.has_value() || card->deleted_at.has_value()) {
      res = support::error_response(http::status::not_found, "not_found", "Card not found.");
      return true;
    }
    holder::card::MilestoneRepo repo(db);

    if (!milestone_id.has_value() && req.method() == http::verb::post) {
      if (card_store == nullptr) {
        res = support::error_response(
            http::status::not_implemented,
            "not_implemented",
            "Card store unavailable."
        );
        return true;
      }
      const auto body = nlohmann::json::parse(req.body());
      holder::model::Milestone milestone;
      milestone.milestone_id = uuid_v4();
      milestone.project_id = card->project_id;
      milestone.card_id = card_id;
      if (with_dates) {
        validate_labelled_fields(body);
        if (!body.contains("dates")) throw std::invalid_argument("dates must be an array.");
        milestone.dates = parse_milestone_dates(body.at("dates"));
      } else {
        if (!body.contains("start_at") || !body.at("start_at").is_number_integer()) {
          throw std::invalid_argument("start_at must be an epoch-second integer.");
        }
        const auto start_at = body.at("start_at").get<long long>();
        std::optional<long long> end_at;
        if (body.contains("end_at") && !body.at("end_at").is_null()) {
          if (!body.at("end_at").is_number_integer()) {
            throw std::invalid_argument("end_at must be an epoch-second integer or null.");
          }
          end_at = body.at("end_at").get<long long>();
          if (*end_at < start_at) {
            throw std::invalid_argument("end_at must not be before start_at.");
          }
        }
        if (body.contains("all_day") && !body.at("all_day").is_boolean()) {
          throw std::invalid_argument("all_day must be a boolean.");
        }
        const bool all_day = body.value("all_day", false);
        milestone.dates.push_back(
            holder::model::milestone_date_from_legacy("start", start_at, all_day)
        );
        if (end_at)
          milestone.dates.push_back(holder::model::milestone_date_from_legacy("end", *end_at, all_day)
          );
      }
      milestone.kind = nullable_string(body, "kind");
      milestone.description = nullable_string(body, "description");
      milestone.created_at = support::now_epoch_seconds();
      milestone.updated_at = milestone.created_at;

      milestone = card_store->add_milestone(milestone, milestone.created_at);
      res = support::json_response(
          http::status::created,
          {{"ok", true},
           {"data", with_dates ? milestone_with_dates_json(milestone) : rig_json::encode(timestamp_milestone(milestone))}}
      );
      return true;
    }

    if (milestone_id.has_value() && req.method() == http::verb::delete_) {
      if (card_store == nullptr) {
        res = support::error_response(
            http::status::not_implemented,
            "not_implemented",
            "Card store unavailable."
        );
        return true;
      }
      const bool removed = card_store->remove_milestone(
          card->project_id,
          card_id,
          *milestone_id,
          support::now_epoch_seconds()
      );
      res = support::json_response(
          http::status::ok,
          {{"ok", true},
           {"data", {{"card_id", card_id}, {"milestone_id", *milestone_id}, {"removed", removed}}}}
      );
      return true;
    } // LCOV_EXCL_LINE: GCC duplicate cleanup line after the covered response.

    if (milestone_id.has_value() && req.method() == http::verb::patch) {
      if (card_store == nullptr) {
        res = support::error_response(
            http::status::not_implemented,
            "not_implemented",
            "Card store unavailable."
        );
        return true;
      }
      if (!with_dates) {
        const auto milestones = repo.list_for_card(card->project_id, card_id);
        const auto existing = std::find_if(
            milestones.begin(), milestones.end(),
            [&](const auto& value) { return value.milestone_id == *milestone_id; }
        );
        if (existing != milestones.end() && !has_timestamp_view(*existing)) {
          res = support::error_response(
              http::status::bad_request, "bad_request",
              "Milestone requires the labelled dates route."
          );
          return true;
        }
      }
      const auto body = nlohmann::json::parse(req.body());
      if (!body.is_object() || body.empty()) {
        throw std::invalid_argument("milestone update must contain at least one field.");
      }
      if (with_dates) validate_labelled_fields(body);
      for (const auto& [name, value] : body.items()) {
        (void)value;
        if (!with_dates && name != "start_at" && name != "end_at" && name != "all_day" &&
            name != "kind" && name != "description") {
          throw std::invalid_argument("unknown milestone update field: " + name);
        }
      }

      holder::card::MilestoneUpdate update;
      if (with_dates && body.contains("dates"))
        update.dates = parse_milestone_dates(body.at("dates"));
      if (body.contains("start_at")) {
        if (!body.at("start_at").is_number_integer()) {
          throw std::invalid_argument("start_at must be an epoch-second integer.");
        }
        update.start_at = body.at("start_at").get<long long>();
      }
      if (body.contains("end_at")) {
        update.has_end_at = true;
        if (!body.at("end_at").is_null()) {
          if (!body.at("end_at").is_number_integer()) {
            throw std::invalid_argument("end_at must be an epoch-second integer or null.");
          }
          update.end_at = body.at("end_at").get<long long>();
        }
      }
      if (body.contains("all_day")) {
        if (!body.at("all_day").is_boolean()) {
          throw std::invalid_argument("all_day must be a boolean.");
        }
        update.all_day = body.at("all_day").get<bool>();
      }
      if (body.contains("kind")) {
        update.has_kind = true;
        update.kind = nullable_string(body, "kind");
      }
      if (body.contains("description")) {
        update.has_description = true;
        update.description = nullable_string(body, "description");
      }

      const auto updated = card_store->update_milestone(
          card->project_id,
          card_id,
          *milestone_id,
          update,
          support::now_epoch_seconds()
      );
      if (!updated.has_value()) {
        res = support::error_response(http::status::not_found, "not_found", "Milestone not found.");
        return true;
      }
      res = support::json_response(
          http::status::ok,
          {{"ok", true},
           {"data", with_dates ? milestone_with_dates_json(*updated) : rig_json::encode(timestamp_milestone(*updated))}}
      );
      return true;
    }
  } catch (const nlohmann::json::exception& ex) {
    res = support::error_response(http::status::bad_request, "bad_request", ex.what());
    return true;
  } catch (const std::exception& ex) {
    res = support::error_response(http::status::bad_request, "bad_request", ex.what());
    return true;
  }

  return false;
}

} // namespace holder::api::routes
