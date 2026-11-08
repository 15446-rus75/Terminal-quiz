#include "format.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <iomanip>
#include <ios>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

namespace
{
  constexpr unsigned int CONTINUATION_MASK = 0xC0U;
  constexpr unsigned int CONTINUATION_BITS = 0x80U;
  constexpr double PERCENT = 100.0;
  constexpr int ACCURACY_DIGITS = 1;
  constexpr int TWO_DIGITS = 2;
  constexpr std::chrono::seconds::rep SECONDS_PER_MINUTE = 60;
  constexpr std::chrono::seconds::rep SECONDS_PER_HOUR = 3600;
  constexpr const char *TIME_FORMAT = "%Y-%m-%d %H:%M";
  constexpr const char *UNKNOWN_TIME = "-";

  std::optional< std::tm > toLocalTime(std::time_t raw)
  {
    std::tm local{};
#if defined(_WIN32)
    if (localtime_s(&local, &raw) != 0)
    {
      return std::nullopt;
    }
#else
    if (localtime_r(&raw, &local) == nullptr)
    {
      return std::nullopt;
    }
#endif
    return local;
  }

  struct IsContinuationByte
  {
    bool operator()(unsigned char ch) const
    {
      return (ch & CONTINUATION_MASK) == CONTINUATION_BITS;
    }
  };

  size_t displayLength(const std::string &text)
  {
    return text.size() - static_cast< size_t >(std::ranges::count_if(text, IsContinuationByte{}));
  }

  std::vector< std::string > wrapLine(const std::string &line, size_t width)
  {
    std::vector< std::string > result;
    std::string current;
    std::istringstream stream(line);
    std::string word;
    while (stream >> word)
    {
      if (current.empty())
      {
        current = word;
      }
      else if (displayLength(current) + 1 + displayLength(word) <= width)
      {
        current += ' ';
        current += word;
      }
      else
      {
        result.push_back(current);
        current = word;
      }
    }
    result.push_back(current);
    return result;
  }
}

std::vector< std::string > quiz::tui::wrapText(const std::string &text, size_t width)
{
  std::vector< std::string > result;
  for (const auto &part : std::views::split(text, '\n'))
  {
    const std::vector< std::string > LINES = wrapLine(std::string(part.begin(), part.end()), width);
    result.insert(result.end(), LINES.begin(), LINES.end());
  }
  return result;
}

std::string quiz::tui::padRight(const std::string &text, size_t width)
{
  const size_t LENGTH = displayLength(text);
  if (LENGTH >= width)
  {
    return text;
  }
  return text + std::string(width - LENGTH, ' ');
}

std::string quiz::tui::formatAccuracy(double accuracy)
{
  std::ostringstream stream;
  stream << std::fixed << std::setprecision(ACCURACY_DIGITS) << accuracy * PERCENT << '%';
  return stream.str();
}

std::string quiz::tui::formatTimePoint(std::chrono::system_clock::time_point time)
{
  if (time == std::chrono::system_clock::time_point{})
  {
    return UNKNOWN_TIME;
  }

  const std::time_t RAW = std::chrono::system_clock::to_time_t(time);
  const std::optional< std::tm > LOCAL = toLocalTime(RAW);
  if (!LOCAL.has_value())
  {
    return UNKNOWN_TIME;
  }
  std::ostringstream stream;
  stream << std::put_time(&*LOCAL, TIME_FORMAT);
  return stream.str();
}

std::string quiz::tui::formatDuration(std::chrono::seconds duration)
{
  const std::chrono::seconds::rep TOTAL = std::max< std::chrono::seconds::rep >(duration.count(), 0);
  const std::chrono::seconds::rep HOURS = TOTAL / SECONDS_PER_HOUR;
  const std::chrono::seconds::rep MINUTES = (TOTAL % SECONDS_PER_HOUR) / SECONDS_PER_MINUTE;
  const std::chrono::seconds::rep SECONDS = TOTAL % SECONDS_PER_MINUTE;

  std::ostringstream stream;
  if (HOURS > 0)
  {
    stream << HOURS << "h " << std::setfill('0') << std::setw(TWO_DIGITS) << MINUTES << "m "
           << std::setw(TWO_DIGITS) << SECONDS << 's';
  }
  else if (MINUTES > 0)
  {
    stream << MINUTES << "m " << std::setfill('0') << std::setw(TWO_DIGITS) << SECONDS << 's';
  }
  else
  {
    stream << SECONDS << 's';
  }
  return stream.str();
}
