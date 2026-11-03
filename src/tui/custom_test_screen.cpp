#include "custom_test_screen.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/custom_test.hpp"
#include "../core/question.hpp"
#include "../services/custom_test_service.hpp"
#include "../services/question_service.hpp"
#include "forms.hpp"
#include "input_parsing.hpp"
#include "prompter.hpp"
#include "quiz_runner.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace
{
  constexpr size_t CREATE_ACTION = 0;
  constexpr size_t PLAY_ACTION = 1;
  constexpr size_t DELETE_ACTION = 2;
  constexpr size_t FILTER_SOURCE = 0;
  constexpr size_t SEARCH_SOURCE = 1;
  constexpr size_t SAVE_SOURCE = 2;

  class QuestionAt
  {
  public:
    explicit QuestionAt(const std::vector< quiz::core::Question > &questions): questions_(questions)
    {
    }

    quiz::core::Question operator()(size_t number) const
    {
      return questions_[number - 1];
    }

  private:
    const std::vector< quiz::core::Question > &questions_;
  };

  struct QuestionId
  {
    size_t operator()(const quiz::core::Question &question) const
    {
      return question.id;
    }
  };

  void appendUnique(std::vector< quiz::core::Question > &target, const std::vector< quiz::core::Question > &more)
  {
    for (const quiz::core::Question &question : more)
    {
      if (std::ranges::find(target, question.id, &quiz::core::Question::id) == target.end())
      {
        target.push_back(question);
      }
    }
  }
}

quiz::tui::CustomTestScreen::CustomTestScreen(std::shared_ptr< ITerminal > terminal,
                                              std::shared_ptr< service::CustomTestService > custom_test_service,
                                              std::shared_ptr< service::QuestionService > question_service,
                                              std::shared_ptr< QuizRunner > runner, size_t user_id):
  renderer_(terminal), forms_(terminal), prompter_(std::move(terminal)),
  custom_test_service_(std::move(custom_test_service)), question_service_(std::move(question_service)),
  runner_(std::move(runner)), user_id_(user_id)
{
  if (custom_test_service_ == nullptr)
  {
    throw std::invalid_argument("Custom test service must not be null");
  }
  if (question_service_ == nullptr)
  {
    throw std::invalid_argument("Question service must not be null");
  }
  if (runner_ == nullptr)
  {
    throw std::invalid_argument("Quiz runner must not be null");
  }
}

std::string quiz::tui::CustomTestScreen::title() const
{
  return "Custom tests";
}

void quiz::tui::CustomTestScreen::run()
{
  renderer_.showTitle(title());
  while (true)
  {
    const std::optional< size_t > ACTION =
      prompter_.askChoice("Action (/back to return)", {"Create a test", "Play a test", "Delete a test"});
    if (!ACTION.has_value())
    {
      return;
    }
    if (*ACTION == CREATE_ACTION)
    {
      createTest();
    }
    else if (*ACTION == PLAY_ACTION)
    {
      playTest();
    }
    else if (*ACTION == DELETE_ACTION)
    {
      deleteTest();
    }
  }
}

void quiz::tui::CustomTestScreen::createTest()
{
  const std::optional< std::string > NAME = prompter_.askText("Test name");
  if (!NAME.has_value())
  {
    return;
  }

  std::vector< core::Question > picked;
  while (true)
  {
    const std::optional< size_t > SOURCE =
      prompter_.askChoice("Add questions (/back to cancel)",
                          {"Get new questions by filter", "Pick from search in saved questions", "Save the test"});
    if (!SOURCE.has_value())
    {
      return;
    }

    if (*SOURCE == FILTER_SOURCE)
    {
      appendUnique(picked, pickByFilter());
      renderer_.showInfo("Selected " + std::to_string(picked.size()) + " question(s) so far");
    }
    else if (*SOURCE == SEARCH_SOURCE)
    {
      appendUnique(picked, pickFromSearch());
      renderer_.showInfo("Selected " + std::to_string(picked.size()) + " question(s) so far");
    }
    else if (*SOURCE == SAVE_SOURCE)
    {
      if (picked.empty())
      {
        renderer_.showError("Add at least one question first.");
        continue;
      }
      break;
    }
  }

  std::vector< size_t > ids;
  ids.reserve(picked.size());
  std::ranges::transform(picked, std::back_inserter(ids), QuestionId{});
  custom_test_service_->create(user_id_, *NAME, ids);
  renderer_.showSuccess("Test \"" + *NAME + "\" saved with " + std::to_string(ids.size()) + " question(s)");
}

void quiz::tui::CustomTestScreen::playTest()
{
  const std::vector< core::CustomTest > TESTS = custom_test_service_->findByUserId(user_id_);
  if (TESTS.empty())
  {
    renderer_.showInfo("You have no custom tests yet.");
    return;
  }

  renderer_.showCustomTests(TESTS);
  const std::optional< size_t > TEST_ID = chooseTest(TESTS);
  if (!TEST_ID.has_value())
  {
    return;
  }
  runner_->play(custom_test_service_->loadQuestions(*TEST_ID));
}

void quiz::tui::CustomTestScreen::deleteTest()
{
  const std::vector< core::CustomTest > TESTS = custom_test_service_->findByUserId(user_id_);
  if (TESTS.empty())
  {
    renderer_.showInfo("You have no custom tests yet.");
    return;
  }

  renderer_.showCustomTests(TESTS);
  const std::optional< size_t > TEST_ID = chooseTest(TESTS);
  if (!TEST_ID.has_value())
  {
    return;
  }
  const std::optional< bool > CONFIRMED = prompter_.askYesNo("Delete this test?", false);
  if (!CONFIRMED.has_value() || !*CONFIRMED)
  {
    return;
  }
  custom_test_service_->remove(*TEST_ID);
  renderer_.showSuccess("Test deleted");
}

std::optional< size_t > quiz::tui::CustomTestScreen::chooseTest(const std::vector< core::CustomTest > &tests)
{
  const std::optional< size_t > NUMBER = prompter_.askNumber("Test number", NumberRange{.min = 1, .max = tests.size()});
  if (!NUMBER.has_value())
  {
    return std::nullopt;
  }
  return tests[*NUMBER - 1].id;
}

std::vector< quiz::core::Question > quiz::tui::CustomTestScreen::pickByFilter()
{
  const std::optional< QuizParameters > PARAMETERS = forms_.askQuizParameters();
  if (!PARAMETERS.has_value())
  {
    return {};
  }

  renderer_.showInfo("Loading questions...");
  const std::vector< core::Question > QUESTIONS = question_service_->fetchQuestions(
    PARAMETERS->amount, PARAMETERS->category, PARAMETERS->difficulty, PARAMETERS->type);
  renderer_.showQuestionList(QUESTIONS);
  if (QUESTIONS.empty())
  {
    return {};
  }

  const std::optional< std::vector< size_t > > SELECTION = prompter_.askSelection("Questions to add", QUESTIONS.size());
  if (!SELECTION.has_value())
  {
    return {};
  }
  std::vector< core::Question > selected;
  selected.reserve(SELECTION->size());
  std::ranges::transform(*SELECTION, std::back_inserter(selected), QuestionAt(QUESTIONS));
  return selected;
}

std::vector< quiz::core::Question > quiz::tui::CustomTestScreen::pickFromSearch()
{
  const std::optional< SearchParameters > PARAMETERS = forms_.askSearchParameters();
  if (!PARAMETERS.has_value())
  {
    return {};
  }

  const std::vector< core::Question > QUESTIONS =
    question_service_->searchByText(PARAMETERS->query, PARAMETERS->category, PARAMETERS->difficulty, PARAMETERS->limit);
  renderer_.showQuestionList(QUESTIONS);
  if (QUESTIONS.empty())
  {
    return {};
  }

  const std::optional< std::vector< size_t > > SELECTION = prompter_.askSelection("Questions to add", QUESTIONS.size());
  if (!SELECTION.has_value())
  {
    return {};
  }
  std::vector< core::Question > selected;
  selected.reserve(SELECTION->size());
  std::ranges::transform(*SELECTION, std::back_inserter(selected), QuestionAt(QUESTIONS));
  return selected;
}
