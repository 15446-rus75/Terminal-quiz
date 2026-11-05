#include "forms.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "../core/enums.hpp"
#include "input_parsing.hpp"
#include "prompter.hpp"
#include "terminal.hpp"

namespace
{
  constexpr size_t MIN_AMOUNT = 1;
  constexpr size_t MAX_AMOUNT = 50;
  constexpr size_t MIN_LIMIT = 1;
  constexpr size_t MAX_LIMIT = 50;

  constexpr std::array< quiz::core::Category, 25 > CATEGORIES = {
    {quiz::core::Category::ANY,
     quiz::core::Category::GENERAL_KNOWLEDGE,
     quiz::core::Category::ENTERTAINMENT_BOOKS,
     quiz::core::Category::ENTERTAINMENT_FILM,
     quiz::core::Category::ENTERTAINMENT_MUSIC,
     quiz::core::Category::ENTERTAINMENT_MUSICALS_THEATRES,
     quiz::core::Category::ENTERTAINMENT_TELEVISION,
     quiz::core::Category::ENTERTAINMENT_VIDEO_GAMES,
     quiz::core::Category::ENTERTAINMENT_BOARD_GAMES,
     quiz::core::Category::SCIENCE_NATURE,
     quiz::core::Category::SCIENCE_COMPUTERS,
     quiz::core::Category::SCIENCE_MATHEMATICS,
     quiz::core::Category::MYTHOLOGY,
     quiz::core::Category::SPORTS,
     quiz::core::Category::GEOGRAPHY,
     quiz::core::Category::HISTORY,
     quiz::core::Category::POLITICS,
     quiz::core::Category::ART,
     quiz::core::Category::CELEBRITIES,
     quiz::core::Category::ANIMALS,
     quiz::core::Category::VEHICLES,
     quiz::core::Category::ENTERTAINMENT_COMICS,
     quiz::core::Category::SCIENCE_GADGETS,
     quiz::core::Category::ENTERTAINMENT_ANIME,
     quiz::core::Category::ENTERTAINMENT_CARTOON_ANIMATIONS}};

  constexpr std::array< quiz::core::Difficulty, 4 > DIFFICULTIES = {
    {quiz::core::Difficulty::ANY, quiz::core::Difficulty::EASY, quiz::core::Difficulty::MEDIUM,
     quiz::core::Difficulty::HARD}};

  constexpr std::array< quiz::core::QuestionType, 3 > QUESTION_TYPES = {
    {quiz::core::QuestionType::ANY, quiz::core::QuestionType::MULTIPLE, quiz::core::QuestionType::BOOLEAN}};

  struct Labeler
  {
    template < class Value >
    std::string operator()(Value value) const
    {
      std::string label = quiz::core::toString(value);
      if (!label.empty())
      {
        label.front() = static_cast< char >(std::toupper(static_cast< unsigned char >(label.front())));
      }
      return label;
    }
  };

  template < class Value, size_t Size >
  std::optional< Value > askFromList(quiz::tui::Prompter &prompter, const std::string &prompt,
                                     const std::array< Value, Size > &values)
  {
    std::vector< std::string > labels;
    labels.reserve(values.size());
    std::ranges::transform(values, std::back_inserter(labels), Labeler{});

    const std::optional< size_t > INDEX = prompter.askChoice(prompt, labels);
    if (!INDEX.has_value())
    {
      return std::nullopt;
    }
    return values[*INDEX];
  }
}

quiz::tui::Forms::Forms(std::shared_ptr< ITerminal > terminal): prompter_(std::move(terminal))
{
}

std::optional< quiz::tui::QuizParameters > quiz::tui::Forms::askQuizParameters()
{
  const std::optional< size_t > AMOUNT =
    prompter_.askNumber("Number of questions", NumberRange{MIN_AMOUNT, MAX_AMOUNT});
  if (!AMOUNT.has_value())
  {
    return std::nullopt;
  }
  const std::optional< core::Category > CATEGORY = askCategory();
  if (!CATEGORY.has_value())
  {
    return std::nullopt;
  }
  const std::optional< core::Difficulty > DIFFICULTY = askDifficulty();
  if (!DIFFICULTY.has_value())
  {
    return std::nullopt;
  }
  const std::optional< core::QuestionType > TYPE = askQuestionType();
  if (!TYPE.has_value())
  {
    return std::nullopt;
  }
  return QuizParameters{.amount = *AMOUNT, .category = *CATEGORY, .difficulty = *DIFFICULTY, .type = *TYPE};
}

std::optional< quiz::tui::SearchParameters > quiz::tui::Forms::askSearchParameters()
{
  std::optional< std::string > query = prompter_.askText("Search text");
  if (!query.has_value())
  {
    return std::nullopt;
  }
  const std::optional< core::Category > CATEGORY = askCategory();
  if (!CATEGORY.has_value())
  {
    return std::nullopt;
  }
  const std::optional< core::Difficulty > DIFFICULTY = askDifficulty();
  if (!DIFFICULTY.has_value())
  {
    return std::nullopt;
  }
  const std::optional< size_t > LIMIT =
    prompter_.askNumber("Maximum number of results", NumberRange{MIN_LIMIT, MAX_LIMIT});
  if (!LIMIT.has_value())
  {
    return std::nullopt;
  }
  return SearchParameters{
    .query = std::move(*query), .category = *CATEGORY, .difficulty = *DIFFICULTY, .limit = *LIMIT};
}

std::optional< quiz::core::Category > quiz::tui::Forms::askCategory()
{
  return askFromList(prompter_, "Category", CATEGORIES);
}

std::optional< quiz::core::Difficulty > quiz::tui::Forms::askDifficulty()
{
  return askFromList(prompter_, "Difficulty", DIFFICULTIES);
}

std::optional< quiz::core::QuestionType > quiz::tui::Forms::askQuestionType()
{
  return askFromList(prompter_, "Question type", QUESTION_TYPES);
}
