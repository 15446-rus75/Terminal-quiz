#include "question.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace
{
  struct Lowerer
  {
    char operator()(unsigned char ch) const
    {
      return static_cast< char >(std::tolower(ch));
    }
  };

  std::string toLower(std::string text)
  {
    std::transform(text.begin(), text.end(), text.begin(), Lowerer{});
    return text;
  }
}

std::vector< std::string > quiz::core::Question::allAnswers() const
{
  std::vector< std::string > all_answers;
  all_answers.push_back(correct_answer);
  all_answers.insert(all_answers.end(), incorrect_answers.begin(), incorrect_answers.end());
  return all_answers;
}

bool quiz::core::Question::isCorrectAnswer(const std::string &user_answer) const
{
  return toLower(user_answer) == toLower(correct_answer);
}
