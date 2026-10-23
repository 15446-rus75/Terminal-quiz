#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/custom_test.hpp"
#include "../core/question.hpp"
#include "../core/statistics.hpp"
#include "../services/history_service.hpp"
#include "../services/quiz_service.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    struct QuestionPosition
    {
      size_t number;
      size_t total;
    };

    class Renderer
    {
    public:
      explicit Renderer(std::shared_ptr< ITerminal > terminal);
      void showTitle(const std::string &title);
      void showMenu(const std::vector< std::string > &items, const std::string &exit_label);
      void showInfo(const std::string &message);
      void showSuccess(const std::string &message);
      void showError(const std::string &message);
      void showHelp(const std::vector< std::string > &lines);
      void showQuestion(const QuestionPosition &position, const core::Question &question,
                        const std::vector< std::string > &options);
      void showAnswerFeedback(bool is_correct, const core::Question &question);
      void showQuizResult(const service::QuizResult &result);
      void showQuestionList(const std::vector< core::Question > &questions);
      void showCustomTests(const std::vector< core::CustomTest > &tests);
      void showAttemptList(const std::vector< core::Attempt > &attempts);
      void showAttemptDetails(const service::AttemptDetails &details);
      void showStatistics(const core::OverallStats &overall, const std::vector< core::CategoryStats > &by_category,
                          const std::vector< core::DifficultyStats > &by_difficulty);

    private:
      void writeLine(const std::string &text, TextStyle style);

      std::shared_ptr< ITerminal > terminal_;
    };
  }
}

#endif
