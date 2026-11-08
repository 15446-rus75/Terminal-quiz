#include "terminal.hpp"

#include <algorithm>
#include <cstddef>
#include <istream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOGDI
#define NOGDI
#endif
#include <windows.h>
#else
#if defined(__linux__)
#include <asm-generic/ioctls.h>
#endif
#include <sys/ioctl.h>
#include <unistd.h>
#endif

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

#if defined(_WIN32)
  bool prepareConsole()
  {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    const HANDLE OUTPUT_HANDLE = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (OUTPUT_HANDLE == INVALID_HANDLE_VALUE || GetConsoleMode(OUTPUT_HANDLE, &mode) == 0)
    {
      return false;
    }
    return SetConsoleMode(OUTPUT_HANDLE, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
  }

  size_t terminalColumns()
  {
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info) == 0)
    {
      return 0;
    }
    return static_cast< size_t >(info.srWindow.Right - info.srWindow.Left + 1);
  }
#else
  bool prepareConsole()
  {
    return true;
  }

  size_t terminalColumns()
  {
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) != 0)
    {
      return 0;
    }
    return size.ws_col;
  }
#endif
}

quiz::tui::StreamTerminal::StreamTerminal(std::istream &input, std::ostream &output, bool use_colors):
  input_(input), output_(output), use_colors_(prepareConsole() && use_colors)
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
  const size_t COLUMNS = terminalColumns();
  if (COLUMNS == 0)
  {
    return DEFAULT_WIDTH;
  }
  return std::max(COLUMNS, MIN_WIDTH);
}
