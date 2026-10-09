#ifndef QUESTION_HPP
#define QUESTION_HPP

#include "enums.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace quiz
{
  namespace core
  {
    struct Question
    {
      size_t id;
      Category category;
      Difficulty difficulty;
      QuestionType type;
      std::string text;
      std::string correct_answer;
      std::vector< std::string > incorrect_answers;

      std::vector< std::string > allAnswers() const;
      bool isCorrectAnswer(const std::string &user_answer) const;
    };
  } // namespace core
} // namespace quiz

#endif
