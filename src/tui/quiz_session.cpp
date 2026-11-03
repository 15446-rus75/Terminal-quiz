#include "quiz_session.hpp"

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../core/question.hpp"
#include "../services/favorite_service.hpp"
#include "../services/quiz_service.hpp"
#include "answer_arranger.hpp"
#include "input_parsing.hpp"
#include "quit_requested.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace
{
  constexpr std::array< std::string_view, 6 > HELP_LINES = {
    "Enter the number of your answer.",
    "  /skip  skip this question",
    "  /fav   add this question to favorites",
    "  /stop  finish the quiz now (unanswered questions count as wrong)",
    "  /quit  exit the program",
    "  /help  show this help"};

  void addToFavorites(quiz::tui::Renderer &renderer, quiz::service::FavoriteService &favorites, size_t user_id,
                      size_t question_id)
  {
    try
    {
      if (favorites.isFavorite(user_id, question_id))
      {
        renderer.showInfo("Already in favorites");
        return;
      }
      favorites.add(user_id, question_id);
      renderer.showSuccess("Added to favorites");
    }
    catch (const std::runtime_error &error)
    {
      renderer.showError(std::string("Could not update favorites: ") + error.what());
    }
  }
}

quiz::tui::QuizSession::QuizSession(std::shared_ptr< ITerminal > terminal,
                                    std::shared_ptr< service::FavoriteService > favorite_service,
                                    std::shared_ptr< IAnswerArranger > arranger, size_t user_id):
  terminal_(std::move(terminal)), renderer_(terminal_), favorite_service_(std::move(favorite_service)),
  arranger_(std::move(arranger)), user_id_(user_id)
{
  if (favorite_service_ == nullptr)
  {
    throw std::invalid_argument("Favorite service must not be null");
  }
  if (arranger_ == nullptr)
  {
    throw std::invalid_argument("Answer arranger must not be null");
  }
}

quiz::tui::SessionOutcome quiz::tui::QuizSession::run(const std::vector< core::Question > &questions)
{
  SessionOutcome outcome{.answers = {}, .stopped_early = false};
  for (size_t i = 0; i < questions.size(); ++i)
  {
    const QuestionPosition POSITION{.number = i + 1, .total = questions.size()};
    std::string answer;
    const QuestionResult RESULT = askQuestion(questions[i], POSITION, answer);
    if (RESULT == QuestionResult::STOPPED)
    {
      outcome.stopped_early = true;
      break;
    }
    if (RESULT == QuestionResult::ANSWERED)
    {
      outcome.answers.push_back(service::QuizAnswer{.question_id = questions[i].id, .user_answer = std::move(answer)});
    }
  }
  return outcome;
}

quiz::tui::QuizSession::QuestionResult quiz::tui::QuizSession::askQuestion(const core::Question &question,
                                                                           const QuestionPosition &position,
                                                                           std::string &answer)
{
  const std::vector< std::string > OPTIONS = arranger_->arrange(question);
  if (OPTIONS.empty())
  {
    throw std::invalid_argument("Question has no answer options");
  }

  renderer_.showQuestion(position, question, OPTIONS);
  const NumberRange RANGE{.min = 1, .max = OPTIONS.size()};
  const std::string PROMPT = "Your answer [1-" + std::to_string(OPTIONS.size()) + "] (/help): ";
  const std::string INVALID_INPUT = "Enter a number from 1 to " + std::to_string(OPTIONS.size()) + ", or use /help";
  while (true)
  {
    const std::optional< std::string > LINE = terminal_->readLine(PROMPT);
    if (!LINE.has_value())
    {
      throw QuitRequested();
    }

    switch (parseCommand(*LINE))
    {
    case Command::HELP:
      renderer_.showHelp(std::vector< std::string >(HELP_LINES.begin(), HELP_LINES.end()));
      break;
    case Command::BACK:
    case Command::STOP:
      return QuestionResult::STOPPED;
    case Command::QUIT:
      throw QuitRequested();
    case Command::SKIP:
      return QuestionResult::SKIPPED;
    case Command::FAVORITE:
      addToFavorites(renderer_, *favorite_service_, user_id_, question.id);
      break;
    case Command::NONE:
      const std::optional< size_t > NUMBER = parseNumber(*LINE, RANGE);
      if (NUMBER.has_value())
      {
        answer = OPTIONS[*NUMBER - 1];
        renderer_.showAnswerFeedback(question.isCorrectAnswer(answer), question);
        return QuestionResult::ANSWERED;
      }
      renderer_.showError(INVALID_INPUT);
      break;
    }
  }
}
