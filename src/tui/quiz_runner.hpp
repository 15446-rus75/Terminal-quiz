#ifndef QUIZ_RUNNER_HPP
#define QUIZ_RUNNER_HPP

#include <cstddef>
#include <memory>
#include <vector>

#include "../core/question.hpp"
#include "../service/favorite_service.hpp"
#include "../service/quiz_service.hpp"
#include "answer_arranger.hpp"
#include "quiz_session.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace quiz
{
  namespace tui
  {
    class QuizRunner
    {
    public:
      QuizRunner(std::shared_ptr< ITerminal > terminal, std::shared_ptr< service::QuizService > quiz_service,
                 std::shared_ptr< service::FavoriteService > favorite_service,
                 std::shared_ptr< IAnswerArranger > arranger, size_t user_id);
      void play(const std::vector< core::Question > &questions);

    private:
      Renderer renderer_;
      QuizSession session_;
      std::shared_ptr< service::QuizService > quiz_service_;
      size_t user_id_;
    };
  }
}

#endif
