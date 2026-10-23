#ifndef FORMS_HPP
#define FORMS_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

#include "../core/enums.hpp"
#include "prompter.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    struct QuizParameters
    {
      size_t amount;
      core::Category category;
      core::Difficulty difficulty;
      core::QuestionType type;
    };

    struct SearchParameters
    {
      std::string query;
      core::Category category;
      core::Difficulty difficulty;
      size_t limit;
    };

    class Forms
    {
    public:
      explicit Forms(std::shared_ptr< ITerminal > terminal);
      std::optional< QuizParameters > askQuizParameters();
      std::optional< SearchParameters > askSearchParameters();
      std::optional< core::Category > askCategory();
      std::optional< core::Difficulty > askDifficulty();
      std::optional< core::QuestionType > askQuestionType();

    private:
      Prompter prompter_;
    };
  }
}

#endif
