#ifndef ATTEMPT_HPP
#define ATTEMPT_HPP

#include "question.hpp"
#include <chrono>
#include <cstdint>
#include <vector>

namespace quiz
{
  namespace core
  {
    struct AttemptAnswer
    {
      size_t id;
      size_t attempt_id;
      size_t question_id;
      std::string user_answer;
      bool is_correct;
      size_t position;
    };

    struct Attempt
    {
      size_t id;
      size_t user_id;
      size_t score;
      size_t total_questions;
      std::chrono::system_clock::time_point started_at;
      std::chrono::system_clock::time_point finished_at;
      std::vector< AttemptAnswer > answers;
    };
  }
}

#endif
