#ifndef QUIZ_SESSION_HPP
#define QUIZ_SESSION_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../core/question.hpp"
#include "../services/favorite_service.hpp"
#include "../services/quiz_service.hpp"
#include "answer_arranger.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    struct SessionOutcome
    {
      std::vector< service::QuizAnswer > answers;
      bool stopped_early;
    };

    class QuizSession
    {
    public:
      QuizSession(std::shared_ptr< ITerminal > terminal, std::shared_ptr< service::FavoriteService > favorite_service,
                  std::shared_ptr< IAnswerArranger > arranger, size_t user_id);
      SessionOutcome run(const std::vector< core::Question > &questions);

    private:
      enum class QuestionResult
      {
        ANSWERED = 0,
        SKIPPED = 1,
        STOPPED = 2
      };

      QuestionResult askQuestion(const core::Question &question, const QuestionPosition &position, std::string &answer);

      std::shared_ptr< ITerminal > terminal_;
      Renderer renderer_;
      std::shared_ptr< service::FavoriteService > favorite_service_;
      std::shared_ptr< IAnswerArranger > arranger_;
      size_t user_id_;
    };
  }
}

#endif
