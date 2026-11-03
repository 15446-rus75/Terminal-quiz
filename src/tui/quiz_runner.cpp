#include "quiz_runner.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../core/question.hpp"
#include "../services/favorite_service.hpp"
#include "../services/quiz_service.hpp"
#include "answer_arranger.hpp"
#include "quiz_session.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

quiz::tui::QuizRunner::QuizRunner(std::shared_ptr< ITerminal > terminal,
                                  std::shared_ptr< service::QuizService > quiz_service,
                                  std::shared_ptr< service::FavoriteService > favorite_service,
                                  std::shared_ptr< IAnswerArranger > arranger, size_t user_id):
  renderer_(terminal), session_(std::move(terminal), std::move(favorite_service), std::move(arranger), user_id),
  quiz_service_(std::move(quiz_service)), user_id_(user_id)
{
  if (quiz_service_ == nullptr)
  {
    throw std::invalid_argument("Quiz service must not be null");
  }
}

void quiz::tui::QuizRunner::play(const std::vector< core::Question > &questions)
{
  if (questions.empty())
  {
    renderer_.showInfo("There are no questions to play.");
    return;
  }

  const size_t ATTEMPT_ID = quiz_service_->startQuiz(user_id_, questions);
  const SessionOutcome OUTCOME = session_.run(questions);
  quiz_service_->submitAnswers(ATTEMPT_ID, questions, OUTCOME.answers);
  const service::QuizResult RESULT = quiz_service_->finishQuiz(ATTEMPT_ID);

  if (OUTCOME.stopped_early)
  {
    renderer_.showInfo("Quiz stopped early. Unanswered questions count as wrong.");
  }
  renderer_.showQuizResult(RESULT);
}
