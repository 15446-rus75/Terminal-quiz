#include "quiz_service.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/question.hpp"
#include "../db/iattempt_repository.hpp"

namespace
{
  class AnswerBuilder
  {
  public:
    AnswerBuilder(size_t attempt_id, const std::vector< quiz::core::Question > &questions):
      attempt_id_(attempt_id),
      questions_(questions)
    {
    }

    quiz::core::AttemptAnswer operator()(const quiz::service::QuizAnswer &answer) const
    {
      const auto FOUND = std::ranges::find(questions_, answer.question_id, &quiz::core::Question::id);
      if (FOUND == questions_.end())
      {
        throw std::invalid_argument("Answer refers to a question that is not part of the quiz");
      }

      return quiz::core::AttemptAnswer{.id = 0,
                                       .attempt_id = attempt_id_,
                                       .question_id = answer.question_id,
                                       .user_answer = answer.user_answer,
                                       .is_correct = FOUND->isCorrectAnswer(answer.user_answer),
                                       .position = static_cast< size_t >(std::distance(questions_.begin(), FOUND))};
    }

  private:
    size_t attempt_id_;
    const std::vector< quiz::core::Question > &questions_;
  };
}

quiz::service::QuizService::QuizService(std::shared_ptr< db::IAttemptRepository > attempt_repository):
  attempt_repository_(std::move(attempt_repository))
{
  if (attempt_repository_ == nullptr)
  {
    throw std::invalid_argument("Attempt repository must not be null");
  }
}

size_t quiz::service::QuizService::startQuiz(size_t user_id, const std::vector< core::Question > &questions)
{
  if (questions.empty())
  {
    throw std::invalid_argument("Quiz must contain at least one question");
  }

  const auto STARTED_AT = std::chrono::system_clock::now();
  const core::Attempt ATTEMPT{.id = 0,
                              .user_id = user_id,
                              .score = 0,
                              .total_questions = questions.size(),
                              .started_at = STARTED_AT,
                              .finished_at = STARTED_AT,
                              .answers = {}};
  return attempt_repository_->save(ATTEMPT);
}

void quiz::service::QuizService::submitAnswers(size_t attempt_id, const std::vector< core::Question > &questions,
                                               const std::vector< QuizAnswer > &answers)
{
  std::vector< core::AttemptAnswer > attempt_answers;
  attempt_answers.reserve(answers.size());
  std::ranges::transform(answers, std::back_inserter(attempt_answers), AnswerBuilder(attempt_id, questions));
  attempt_repository_->saveAnswers(attempt_id, attempt_answers);
}

quiz::service::QuizResult quiz::service::QuizService::finishQuiz(size_t attempt_id)
{
  const std::optional< core::Attempt > ATTEMPT = attempt_repository_->findById(attempt_id);
  if (!ATTEMPT.has_value())
  {
    throw std::invalid_argument("Attempt not found");
  }

  const size_t SCORE =
    static_cast< size_t >(std::ranges::count(ATTEMPT->answers, true, &core::AttemptAnswer::is_correct));
  const size_t TOTAL = ATTEMPT->total_questions;
  attempt_repository_->finish(attempt_id, SCORE, std::chrono::system_clock::now());

  const double ACCURACY = TOTAL == 0 ? 0.0 : static_cast< double >(SCORE) / static_cast< double >(TOTAL);
  return QuizResult{.attempt_id = attempt_id, .score = SCORE, .total = TOTAL, .accuracy = ACCURACY};
}
