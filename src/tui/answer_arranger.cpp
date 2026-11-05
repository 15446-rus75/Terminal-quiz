#include "answer_arranger.hpp"

#include <algorithm>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "../core/enums.hpp"
#include "../core/question.hpp"

quiz::tui::RandomAnswerArranger::RandomAnswerArranger(): generator_(std::random_device{}())
{
}

quiz::tui::RandomAnswerArranger::RandomAnswerArranger(unsigned int seed): generator_(seed)
{
}

std::vector< std::string > quiz::tui::RandomAnswerArranger::arrange(const core::Question &question)
{
  std::vector< std::string > answers = question.allAnswers();
  if (question.type == core::QuestionType::BOOLEAN)
  {
    std::ranges::sort(answers, std::greater{});
    return answers;
  }
  std::ranges::shuffle(answers, generator_);
  return answers;
}
