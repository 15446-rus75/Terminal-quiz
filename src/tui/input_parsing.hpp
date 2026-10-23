#ifndef INPUT_PARSING_HPP
#define INPUT_PARSING_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace quiz
{
  namespace tui
  {
    enum class Command
    {
      NONE = 0,
      HELP = 1,
      BACK = 2,
      QUIT = 3,
      SKIP = 4,
      FAVORITE = 5,
      STOP = 6
    };

    struct NumberRange
    {
      size_t min;
      size_t max;
    };

    Command parseCommand(const std::string &line);
    std::optional< size_t > parseNumber(const std::string &text, const NumberRange &range);
    std::optional< std::vector< size_t > > parseSelection(const std::string &text, size_t max);
  }
}

#endif
