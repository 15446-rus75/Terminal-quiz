#include "search_screen.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/question.hpp"
#include "../services/favorite_service.hpp"
#include "../services/question_service.hpp"
#include "forms.hpp"
#include "prompter.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

quiz::tui::SearchScreen::SearchScreen(std::shared_ptr< ITerminal > terminal,
                                      std::shared_ptr< service::QuestionService > question_service,
                                      std::shared_ptr< service::FavoriteService > favorite_service, size_t user_id):
  renderer_(terminal), forms_(terminal), prompter_(std::move(terminal)), question_service_(std::move(question_service)),
  favorite_service_(std::move(favorite_service)), user_id_(user_id)
{
  if (question_service_ == nullptr)
  {
    throw std::invalid_argument("Question service must not be null");
  }
  if (favorite_service_ == nullptr)
  {
    throw std::invalid_argument("Favorite service must not be null");
  }
}

std::string quiz::tui::SearchScreen::title() const
{
  return "Search questions";
}

void quiz::tui::SearchScreen::run()
{
  renderer_.showTitle(title());
  const std::optional< SearchParameters > PARAMETERS = forms_.askSearchParameters();
  if (!PARAMETERS.has_value())
  {
    return;
  }

  const std::vector< core::Question > QUESTIONS =
    question_service_->searchByText(PARAMETERS->query, PARAMETERS->category, PARAMETERS->difficulty, PARAMETERS->limit);
  renderer_.showQuestionList(QUESTIONS);
  if (!QUESTIONS.empty())
  {
    offerFavorites(QUESTIONS);
  }
}

void quiz::tui::SearchScreen::offerFavorites(const std::vector< core::Question > &questions)
{
  const std::optional< bool > WANTED = prompter_.askYesNo("Add some of these questions to favorites?", false);
  if (!WANTED.has_value() || !*WANTED)
  {
    return;
  }
  const std::optional< std::vector< size_t > > SELECTION = prompter_.askSelection("Questions to add", questions.size());
  if (!SELECTION.has_value())
  {
    return;
  }

  size_t added = 0;
  for (const size_t number : *SELECTION)
  {
    const size_t QUESTION_ID = questions[number - 1].id;
    if (!favorite_service_->isFavorite(user_id_, QUESTION_ID))
    {
      favorite_service_->add(user_id_, QUESTION_ID);
      ++added;
    }
  }
  renderer_.showSuccess("Added " + std::to_string(added) + " question(s) to favorites");
}
