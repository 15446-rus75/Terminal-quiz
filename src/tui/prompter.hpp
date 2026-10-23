#ifndef PROMPTER_HPP
#define PROMPTER_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "input_parsing.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class Prompter
    {
    public:
      explicit Prompter(std::shared_ptr< ITerminal > terminal);
      std::optional< std::string > askText(const std::string &prompt);
      std::optional< size_t > askNumber(const std::string &prompt, const NumberRange &range);
      std::optional< size_t > askChoice(const std::string &prompt, const std::vector< std::string > &options);
      std::optional< bool > askYesNo(const std::string &prompt, bool default_answer);
      std::optional< std::vector< size_t > > askSelection(const std::string &prompt, size_t max);

    private:
      std::shared_ptr< ITerminal > terminal_;
    };
  }
}

#endif
