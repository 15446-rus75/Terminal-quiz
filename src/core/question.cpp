#include "question.hpp"

#include <string>
#include <vector>

std::vector< std::string > quiz::core::Question::allAnswers() const
{
  std::vector< std::string > all_answers;
  all_answers.push_back(correct_answer);
  all_answers.insert(all_answers.end(), incorrect_answers.begin(), incorrect_answers.end());
  return all_answers;
}

bool quiz::core::Question::isCorrectAnswer(const std::string &user_answer) const
{
  return user_answer == correct_answer;
}
