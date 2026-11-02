#include "terminal.hpp"

#include <algorithm>
#include <cstddef>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <unistd.h>

#include <asm-generic/ioctls.h>

namespace
{
  constexpr size_t DEFAULT_WIDTH = 80;
  constexpr size_t MIN_WIDTH = 20;
  constexpr std::string_view RESET_STYLE = "\033[0m";
  constexpr std::string_view CLEAR_SCREEN = "\033[2J\033[H";

  std::string_view styleCode(quiz::tui::TextStyle style)
  {
    switch (style)
    {
    case quiz::tui::TextStyle::PLAIN:
      return "";
    case quiz::tui::TextStyle::TITLE:
      return "\033[1;36m";
    case quiz::tui::TextStyle::SUCCESS:
      return "\033[32m";
    case quiz::tui::TextStyle::ERROR:
      return "\033[31m";
    case quiz::tui::TextStyle::MUTED:
      return "\033[90m";
    }
    throw std::invalid_argument("Unknown text style");
  }
}

quiz::tui::StreamTerminal::StreamTerminal(std::istream &input, std::ostream &output, bool use_colors):
  input_(input), output_(output), use_colors_(use_colors)
{
}

std::optional< std::string > quiz::tui::StreamTerminal::readLine(const std::string &prompt)
{
  output_ << prompt << std::flush;

  std::string line;
  if (!std::getline(input_, line))
  {
    output_ << '\n';
    return std::nullopt;
  }
  if (!line.empty() && line.back() == '\r')
  {
    line.pop_back();
  }
  return line;
}

void quiz::tui::StreamTerminal::write(const std::string &text, TextStyle style)
{
  const std::string_view CODE = use_colors_ ? styleCode(style) : std::string_view{};
  if (CODE.empty())
  {
    output_ << text;
    return;
  }
  output_ << CODE << text << RESET_STYLE;
}

void quiz::tui::StreamTerminal::clear()
{
  if (use_colors_)
  {
    output_ << CLEAR_SCREEN << std::flush;
  }
}

size_t quiz::tui::StreamTerminal::width() const
{
  winsize size{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0 || size.ws_col == 0)
  {
    return DEFAULT_WIDTH;
  }
  return std::max(static_cast< size_t >(size.ws_col), MIN_WIDTH);
}
