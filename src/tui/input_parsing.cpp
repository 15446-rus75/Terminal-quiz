#include "input_parsing.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace
{
  struct CommandAlias
  {
    quiz::tui::Command command;
    std::string_view text;
  };

  constexpr std::array< CommandAlias, 12 > COMMAND_ALIASES = {{{quiz::tui::Command::HELP, "/help"},
                                                               {quiz::tui::Command::HELP, "/h"},
                                                               {quiz::tui::Command::BACK, "/back"},
                                                               {quiz::tui::Command::BACK, "/b"},
                                                               {quiz::tui::Command::QUIT, "/quit"},
                                                               {quiz::tui::Command::QUIT, "/q"},
                                                               {quiz::tui::Command::SKIP, "/skip"},
                                                               {quiz::tui::Command::SKIP, "/s"},
                                                               {quiz::tui::Command::FAVORITE, "/fav"},
                                                               {quiz::tui::Command::FAVORITE, "/f"},
                                                               {quiz::tui::Command::STOP, "/stop"},
                                                               {quiz::tui::Command::HELP, "?"}}};

  struct Lowerer
  {
    char operator()(unsigned char ch) const
    {
      return static_cast< char >(std::tolower(ch));
    }
  };

  struct IsSpace
  {
    bool operator()(unsigned char ch) const
    {
      return std::isspace(ch) != 0;
    }
  };

  std::string trim(const std::string &text)
  {
    const auto FIRST = std::ranges::find_if_not(text, IsSpace{});
    if (FIRST == text.end())
    {
      return "";
    }
    const auto LAST = std::ranges::find_if_not(text.rbegin(), text.rend(), IsSpace{}).base();
    return {FIRST, LAST};
  }

  std::optional< std::vector< size_t > > parseToken(const std::string &token, size_t max)
  {
    const quiz::tui::NumberRange RANGE{1, max};
    const size_t DASH = token.find('-');
    const bool IS_RANGE = DASH != std::string::npos;
    const std::optional< size_t > FIRST = quiz::tui::parseNumber(IS_RANGE ? token.substr(0, DASH) : token, RANGE);
    const std::optional< size_t > LAST = quiz::tui::parseNumber(IS_RANGE ? token.substr(DASH + 1) : token, RANGE);
    if (!FIRST.has_value() || !LAST.has_value() || *FIRST > *LAST)
    {
      return std::nullopt;
    }

    std::vector< size_t > values;
    values.reserve(*LAST - *FIRST + 1);
    std::ranges::copy(std::views::iota(*FIRST, *LAST + 1), std::back_inserter(values));
    return values;
  }
}

quiz::tui::Command quiz::tui::parseCommand(const std::string &line)
{
  std::string text = trim(line);
  std::ranges::transform(text, text.begin(), Lowerer{});

  const auto *const FOUND = std::ranges::find(COMMAND_ALIASES, std::string_view(text), &CommandAlias::text);
  if (FOUND == COMMAND_ALIASES.end())
  {
    return Command::NONE;
  }
  return FOUND->command;
}

std::optional< size_t > quiz::tui::parseNumber(const std::string &text, const NumberRange &range)
{
  const std::string TRIMMED = trim(text);
  const char *const FIRST = std::to_address(TRIMMED.begin());
  const char *const LAST = std::to_address(TRIMMED.end());

  size_t value = 0;
  const std::from_chars_result RESULT = std::from_chars(FIRST, LAST, value);
  if (RESULT.ec != std::errc{} || RESULT.ptr != LAST)
  {
    return std::nullopt;
  }
  if (value < range.min || value > range.max)
  {
    return std::nullopt;
  }
  return value;
}

std::optional< std::vector< size_t > > quiz::tui::parseSelection(const std::string &text, size_t max)
{
  std::string normalized = text;
  std::ranges::replace(normalized, ',', ' ');

  std::istringstream stream(normalized);
  std::vector< size_t > selection;
  std::string token;
  while (stream >> token)
  {
    const std::optional< std::vector< size_t > > PART = parseToken(token, max);
    if (!PART.has_value())
    {
      return std::nullopt;
    }
    selection.insert(selection.end(), PART->begin(), PART->end());
  }
  if (selection.empty())
  {
    return std::nullopt;
  }

  std::ranges::sort(selection);
  const auto DUPLICATES = std::ranges::unique(selection);
  selection.erase(DUPLICATES.begin(), DUPLICATES.end());
  return selection;
}
