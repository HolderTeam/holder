#pragma once

#include <chrono>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace holder::model {

// A milestone has at most one current value for each free-form role. A calendar day is not
// an instant: it has no timezone and must not move when displayed on another device.
// legacy_all_day_at preserves the old local-midnight epoch without guessing its timezone.
struct MilestoneDate {
  std::string role;
  std::optional<std::string> date;
  std::optional<std::string> datetime;
  std::optional<long long> legacy_all_day_at;

  bool operator==(const MilestoneDate&) const = default;
};

inline int milestone_date_digits(const std::string& value, std::size_t pos, std::size_t count) {
  int result = 0;
  for (std::size_t i = pos; i < pos + count; ++i) {
    if (i >= value.size() || value[i] < '0' || value[i] > '9')
      throw std::invalid_argument("invalid milestone date");
    result = result * 10 + (value[i] - '0');
  }
  return result;
}

inline std::chrono::sys_days milestone_calendar_day(const std::string& value) {
  if (value.size() < 10 || value[4] != '-' || value[7] != '-')
    throw std::invalid_argument("milestone date must use YYYY-MM-DD");
  const auto year = milestone_date_digits(value, 0, 4);
  const auto month = milestone_date_digits(value, 5, 2);
  const auto day = milestone_date_digits(value, 8, 2);
  const std::chrono::year_month_day calendar{
      std::chrono::year{year},
      std::chrono::month{static_cast<unsigned>(month)},
      std::chrono::day{static_cast<unsigned>(day)}
  };
  if (year == 0 || !calendar.ok()) throw std::invalid_argument("invalid milestone calendar day");
  return std::chrono::sys_days{calendar};
}

inline long long milestone_datetime_epoch(const std::string& value) {
  if ((value.size() != 20 && value.size() != 25) || value[10] != 'T' || value[13] != ':' ||
      value[16] != ':')
    throw std::invalid_argument("milestone datetime must use RFC 3339 seconds and offset");
  const auto day = milestone_calendar_day(value);
  const int hour = milestone_date_digits(value, 11, 2);
  const int minute = milestone_date_digits(value, 14, 2);
  const int second = milestone_date_digits(value, 17, 2);
  if (hour > 23 || minute > 59 || second > 59)
    throw std::invalid_argument("invalid milestone datetime time");
  int offset = 0;
  if (value.size() == 20) {
    if (value[19] != 'Z') throw std::invalid_argument("invalid milestone datetime offset");
  } else {
    if ((value[19] != '+' && value[19] != '-') || value[22] != ':')
      throw std::invalid_argument("invalid milestone datetime offset");
    const int offset_hour = milestone_date_digits(value, 20, 2);
    const int offset_minute = milestone_date_digits(value, 23, 2);
    if (offset_hour > 23 || offset_minute > 59)
      throw std::invalid_argument("invalid milestone datetime offset");
    offset = (offset_hour * 60 + offset_minute) * (value[19] == '+' ? 1 : -1);
  }
  return std::chrono::duration_cast<std::chrono::seconds>(day.time_since_epoch()).count() +
         hour * 3600 + minute * 60 + second - offset * 60;
}

inline std::string milestone_datetime_utc(long long at) {
  const auto time = std::chrono::sys_seconds{std::chrono::seconds{at}};
  const auto day = std::chrono::floor<std::chrono::days>(time);
  const std::chrono::year_month_day calendar{day};
  const auto within = std::chrono::duration_cast<std::chrono::seconds>(time - day).count();
  if (int(calendar.year()) < 1 || int(calendar.year()) > 9999)
    throw std::invalid_argument("milestone datetime year is outside RFC 3339 range");
  std::ostringstream out;
  out << std::setfill('0') << std::setw(4) << int(calendar.year()) << '-' << std::setw(2)
      << unsigned(calendar.month()) << '-' << std::setw(2) << unsigned(calendar.day()) << 'T'
      << std::setw(2) << within / 3600 << ':' << std::setw(2) << (within / 60) % 60 << ':'
      << std::setw(2) << within % 60 << 'Z';
  return out.str();
}

inline void validate_milestone_date(const MilestoneDate& value) {
  if (value.role.empty()) throw std::invalid_argument("milestone date role must not be empty");
  const int forms = int(value.date.has_value()) + int(value.datetime.has_value()) +
                    int(value.legacy_all_day_at.has_value());
  if (forms != 1) throw std::invalid_argument("milestone date needs exactly one value");
  if (value.date) {
    if (value.date->size() != 10) throw std::invalid_argument("milestone date must use YYYY-MM-DD");
    milestone_calendar_day(*value.date);
  }
  if (value.datetime) milestone_datetime_epoch(*value.datetime);
}

inline std::optional<long long> milestone_date_epoch(const MilestoneDate& value) {
  if (value.datetime) return milestone_datetime_epoch(*value.datetime);
  return value.legacy_all_day_at;
}

inline MilestoneDate milestone_date_from_legacy(std::string role, long long at, bool all_day) {
  MilestoneDate value;
  value.role = std::move(role);
  if (all_day)
    value.legacy_all_day_at = at;
  else
    value.datetime = milestone_datetime_utc(at);
  return value;
}

} // namespace holder::model
