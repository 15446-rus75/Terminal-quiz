#include "prompter.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "input_parsing.hpp"
#include "quit_requested.hpp"
#include "terminal.hpp"

namespace
{
  constexpr std::array< std::string_view, 4 > HELP_LINES = {
    "Commands available at any prompt:", "  /help  show this help", "  /back  cancel and go back",
    "  /quit  exit the program"};

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

  void writeLine(quiz::tui::ITerminal &terminal, const std::string &text, quiz::tui::TextStyle style)
  {
    terminal.write(text + "\n", style);
  }

  std::optional< std::string > readReply(quiz::tui::ITerminal &terminal, const std::string &prompt)
  {
    while (true)
    {
      std::optional< std::string > line = terminal.readLine(prompt);
      if (!line.has_value())
      {
        throw quiz::tui::QuitRequested();
      }

      const quiz::tui::Command COMMAND = quiz::tui::parseCommand(*line);
      if (COMMAND == quiz::tui::Command::BACK)
      {
        return std::nullopt;
      }
      if (COMMAND == quiz::tui::Command::QUIT)
      {
        throw quiz::tui::QuitRequested();
      }
      if (COMMAND != quiz::tui::Command::HELP)
      {
        return line;
      }
      for (const std::string_view line : HELP_LINES)
      {
        writeLine(terminal, std::string(line), quiz::tui::TextStyle::MUTED);
      }
    }
  }

  std::optional< bool > parseYesNo(const std::string &text, bool default_answer)
  {
    std::string answer = trim(text);
    std::ranges::transform(answer, answer.begin(), Lowerer{});
    if (answer.empty())
    {
      return default_answer;
    }
    if (answer == "y" || answer == "yes")
    {
      return true;
    }
    if (answer == "n" || answer == "no")
    {
      return false;
    }
    return std::nullopt;
  }
}

quiz::tui::Prompter::Prompter(std::shared_ptr< ITerminal > terminal): terminal_(std::move(terminal))
{
  if (terminal_ == nullptr)
  {
    throw std::invalid_argument("Terminal must not be null");
  }
}

std::optional< std::string > quiz::tui::Prompter::askText(const std::string &prompt)
{
  const std::string FULL_PROMPT = prompt + ": ";
  while (true)
  {
    const std::optional< std::string > REPLY = readReply(*terminal_, FULL_PROMPT);
    if (!REPLY.has_value())
    {
      return std::nullopt;
    }
    std::string text = trim(*REPLY);
    if (!text.empty())
    {
      return text;
    }
    writeLine(*terminal_, "Input must not be empty", TextStyle::ERROR);
  }
}

std::optional< size_t > quiz::tui::Prompter::askNumber(const std::string &prompt, const NumberRange &range)
{
  const std::string RANGE_TEXT = std::to_string(range.min) + "-" + std::to_string(range.max);
  const std::string FULL_PROMPT = prompt + " [" + RANGE_TEXT + "]: ";
  const std::string ERROR_TEXT =
    "Enter a number from " + std::to_string(range.min) + " to " + std::to_string(range.max);
  while (true)
  {
    const std::optional< std::string > REPLY = readReply(*terminal_, FULL_PROMPT);
    if (!REPLY.has_value())
    {
      return std::nullopt;
    }
    const std::optional< size_t > NUMBER = parseNumber(*REPLY, range);
    if (NUMBER.has_value())
    {
      return NUMBER;
    }
    writeLine(*terminal_, ERROR_TEXT, TextStyle::ERROR);
  }
}

std::optional< size_t > quiz::tui::Prompter::askChoice(const std::string &prompt,
                                                       const std::vector< std::string > &options)
{
  if (options.empty())
  {
    throw std::invalid_argument("Choice options must not be empty");
  }

  for (size_t i = 0; i < options.size(); ++i)
  {
    writeLine(*terminal_, "  " + std::to_string(i + 1) + ". " + options[i], TextStyle::PLAIN);
  }
  const std::optional< size_t > NUMBER = askNumber(prompt, NumberRange{1, options.size()});
  if (!NUMBER.has_value())
  {
    return std::nullopt;
  }
  return *NUMBER - 1;
}

std::optional< bool > quiz::tui::Prompter::askYesNo(const std::string &prompt, bool default_answer)
{
  const std::string FULL_PROMPT = prompt + (default_answer ? " [Y/n]: " : " [y/N]: ");
  while (true)
  {
    const std::optional< std::string > REPLY = readReply(*terminal_, FULL_PROMPT);
    if (!REPLY.has_value())
    {
      return std::nullopt;
    }
    const std::optional< bool > ANSWER = parseYesNo(*REPLY, default_answer);
    if (ANSWER.has_value())
    {
      return ANSWER;
    }
    writeLine(*terminal_, "Answer y or n", TextStyle::ERROR);
  }
}

std::optional< std::vector< size_t > > quiz::tui::Prompter::askSelection(const std::string &prompt, size_t max)
{
  const std::string FULL_PROMPT = prompt + " (e.g. 1,3-5) [1-" + std::to_string(max) + "]: ";
  const std::string ERROR_TEXT = "Enter numbers from 1 to " + std::to_string(max) + ", for example 1,3-5";
  while (true)
  {
    const std::optional< std::string > REPLY = readReply(*terminal_, FULL_PROMPT);
    if (!REPLY.has_value())
    {
      return std::nullopt;
    }
    std::optional< std::vector< size_t > > selection = parseSelection(*REPLY, max);
    if (selection.has_value())
    {
      return selection;
    }
    writeLine(*terminal_, ERROR_TEXT, TextStyle::ERROR);
  }
}
