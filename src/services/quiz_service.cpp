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
  class HasQuestionId
  {
  public:
    explicit HasQuestionId(size_t question_id): question_id_(question_id)
    {
    }

    bool operator()(const quiz::core::Question &question) const
    {
      return question.id == question_id_;
    }

  private:
    size_t question_id_;
  };

  class IsCorrect
  {
  public:
    bool operator()(const quiz::core::AttemptAnswer &answer) const
    {
      return answer.is_correct;
    }
  };

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
      const auto FOUND = std::find_if(questions_.begin(), questions_.end(), HasQuestionId(answer.question_id));
      if (FOUND == questions_.end())
      {
        throw std::invalid_argument("Answer refers to a question that is not part of the quiz");
      }

      quiz::core::AttemptAnswer result{};
      result.attempt_id = attempt_id_;
      result.question_id = answer.question_id;
      result.user_answer = answer.user_answer;
      result.is_correct = FOUND->isCorrectAnswer(answer.user_answer);
      result.position = static_cast< size_t >(std::distance(questions_.begin(), FOUND));
      return result;
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

  core::Attempt attempt{};
  attempt.user_id = user_id;
  attempt.score = 0;
  attempt.total_questions = questions.size();
  attempt.started_at = std::chrono::system_clock::now();
  attempt.finished_at = attempt.started_at;
  return attempt_repository_->save(attempt);
}

void quiz::service::QuizService::submitAnswers(size_t attempt_id, const std::vector< core::Question > &questions,
                                               const std::vector< QuizAnswer > &answers)
{
  std::vector< core::AttemptAnswer > attempt_answers;
  attempt_answers.reserve(answers.size());
  std::transform(answers.begin(), answers.end(), std::back_inserter(attempt_answers),
                 AnswerBuilder(attempt_id, questions));
  attempt_repository_->saveAnswers(attempt_id, attempt_answers);
}

quiz::service::QuizResult quiz::service::QuizService::finishQuiz(size_t attempt_id)
{
  const std::optional< core::Attempt > ATTEMPT = attempt_repository_->findById(attempt_id);
  if (!ATTEMPT.has_value())
  {
    throw std::invalid_argument("Attempt not found");
  }

  const size_t SCORE = static_cast< size_t >(
    std::count_if(ATTEMPT->answers.begin(), ATTEMPT->answers.end(), IsCorrect{}));
  const size_t TOTAL = ATTEMPT->total_questions;
  attempt_repository_->finish(attempt_id, SCORE, std::chrono::system_clock::now());

  QuizResult result{};
  result.attempt_id = attempt_id;
  result.score = SCORE;
  result.total = TOTAL;
  result.accuracy = TOTAL == 0 ? 0.0 : static_cast< double >(SCORE) / static_cast< double >(TOTAL);
  return result;
}
