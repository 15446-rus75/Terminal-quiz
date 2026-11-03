#include "quiz_screen.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/question.hpp"
#include "../services/question_service.hpp"
#include "forms.hpp"
#include "quiz_runner.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

quiz::tui::QuizScreen::QuizScreen(std::shared_ptr< ITerminal > terminal,
                                  std::shared_ptr< service::QuestionService > question_service,
                                  std::shared_ptr< QuizRunner > runner):
  renderer_(terminal), forms_(std::move(terminal)), question_service_(std::move(question_service)),
  runner_(std::move(runner))
{
  if (question_service_ == nullptr)
  {
    throw std::invalid_argument("Question service must not be null");
  }
  if (runner_ == nullptr)
  {
    throw std::invalid_argument("Quiz runner must not be null");
  }
}

std::string quiz::tui::QuizScreen::title() const
{
  return "Play a quiz";
}

void quiz::tui::QuizScreen::run()
{
  renderer_.showTitle(title());
  const std::optional< QuizParameters > PARAMETERS = forms_.askQuizParameters();
  if (!PARAMETERS.has_value())
  {
    return;
  }

  renderer_.showInfo("Loading questions...");
  const std::vector< core::Question > QUESTIONS = question_service_->fetchQuestions(
    PARAMETERS->amount, PARAMETERS->category, PARAMETERS->difficulty, PARAMETERS->type);
  if (QUESTIONS.empty())
  {
    renderer_.showError("No questions found for these parameters.");
    return;
  }
  if (QUESTIONS.size() < PARAMETERS->amount)
  {
    renderer_.showInfo("Only " + std::to_string(QUESTIONS.size()) + " question(s) available for these parameters.");
  }
  runner_->play(QUESTIONS);
}
