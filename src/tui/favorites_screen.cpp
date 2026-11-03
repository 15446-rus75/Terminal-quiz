#include "favorites_screen.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/question.hpp"
#include "../services/favorite_service.hpp"
#include "prompter.hpp"
#include "quiz_runner.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace
{
  constexpr size_t PLAY_ACTION = 0;
  constexpr size_t REMOVE_ACTION = 1;

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
}

quiz::tui::FavoritesScreen::FavoritesScreen(std::shared_ptr< ITerminal > terminal,
                                            std::shared_ptr< service::FavoriteService > favorite_service,
                                            std::shared_ptr< QuizRunner > runner, size_t user_id):
  renderer_(terminal), prompter_(std::move(terminal)), favorite_service_(std::move(favorite_service)),
  runner_(std::move(runner)), user_id_(user_id)
{
  if (favorite_service_ == nullptr)
  {
    throw std::invalid_argument("Favorite service must not be null");
  }
  if (runner_ == nullptr)
  {
    throw std::invalid_argument("Quiz runner must not be null");
  }
}

std::string quiz::tui::FavoritesScreen::title() const
{
  return "Favorites";
}

void quiz::tui::FavoritesScreen::run()
{
  renderer_.showTitle(title());
  while (true)
  {
    const std::vector< core::Question > QUESTIONS = favorite_service_->getAll(user_id_);
    if (QUESTIONS.empty())
    {
      renderer_.showInfo("You have no favorite questions yet.");
      return;
    }

    renderer_.showQuestionList(QUESTIONS);
    const std::optional< size_t > ACTION =
      prompter_.askChoice("Action (/back to return)", {"Play favorites", "Remove from favorites"});
    if (!ACTION.has_value())
    {
      return;
    }
    if (*ACTION == PLAY_ACTION)
    {
      playFavorites(QUESTIONS);
    }
    else if (*ACTION == REMOVE_ACTION)
    {
      removeFavorites(QUESTIONS);
    }
  }
}

void quiz::tui::FavoritesScreen::removeFavorites(const std::vector< core::Question > &questions)
{
  const std::optional< std::vector< size_t > > SELECTION =
    prompter_.askSelection("Questions to remove", questions.size());
  if (!SELECTION.has_value())
  {
    return;
  }
  const std::optional< bool > CONFIRMED =
    prompter_.askYesNo("Remove " + std::to_string(SELECTION->size()) + " question(s) from favorites?", false);
  if (!CONFIRMED.has_value() || !*CONFIRMED)
  {
    return;
  }

  for (const size_t number : *SELECTION)
  {
    favorite_service_->remove(user_id_, questions[number - 1].id);
  }
  renderer_.showSuccess("Removed " + std::to_string(SELECTION->size()) + " question(s) from favorites");
}

void quiz::tui::FavoritesScreen::playFavorites(const std::vector< core::Question > &questions)
{
  const std::optional< bool > PLAY_ALL = prompter_.askYesNo("Play all favorite questions?", true);
  if (!PLAY_ALL.has_value())
  {
    return;
  }
  if (*PLAY_ALL)
  {
    runner_->play(questions);
    return;
  }

  const std::optional< std::vector< size_t > > SELECTION =
    prompter_.askSelection("Questions to play", questions.size());
  if (!SELECTION.has_value())
  {
    return;
  }
  std::vector< core::Question > selected;
  selected.reserve(SELECTION->size());
  std::ranges::transform(*SELECTION, std::back_inserter(selected), QuestionAt(questions));
  runner_->play(selected);
}
