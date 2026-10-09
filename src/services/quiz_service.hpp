#ifndef QUIZ_SERVICES_QUIZ_SERVICE_HPP
#define QUIZ_SERVICES_QUIZ_SERVICE_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/question.hpp"
#include "../db/i_attempt_repository.hpp"

namespace quiz
{
  namespace service
  {
    struct QuizAnswer
    {
      size_t question_id;
      std::string user_answer;
    };

    struct QuizResult
    {
      size_t attempt_id;
      size_t score;
      size_t total;
      double accuracy;
    };

    class QuizService
    {
    public:
      explicit QuizService(std::shared_ptr< db::IAttemptRepository > attempt_repository);
      size_t startQuiz(size_t user_id, const std::vector< core::Question > &questions);
      void submitAnswers(size_t attempt_id, const std::vector< core::Question > &questions,
                         const std::vector< QuizAnswer > &answers);
      QuizResult finishQuiz(size_t attempt_id);

    private:
      std::shared_ptr< db::IAttemptRepository > attempt_repository_;
    };
  }
}

#endif
