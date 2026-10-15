#include "enums.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>
#include <string>
#include <string_view>

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

  struct CategoryName
  {
    quiz::core::Category category;
    std::string_view name;
  };

  constexpr std::array< CategoryName, 25 > CATEGORY_NAMES = {
    {{quiz::core::Category::ANY, "Any Category"},
     {quiz::core::Category::GENERAL_KNOWLEDGE, "General Knowledge"},
     {quiz::core::Category::ENTERTAINMENT_BOOKS, "Entertainment: Books"},
     {quiz::core::Category::ENTERTAINMENT_FILM, "Entertainment: Film"},
     {quiz::core::Category::ENTERTAINMENT_MUSIC, "Entertainment: Music"},
     {quiz::core::Category::ENTERTAINMENT_MUSICALS_THEATRES, "Entertainment: Musicals & Theatres"},
     {quiz::core::Category::ENTERTAINMENT_TELEVISION, "Entertainment: Television"},
     {quiz::core::Category::ENTERTAINMENT_VIDEO_GAMES, "Entertainment: Video Games"},
     {quiz::core::Category::ENTERTAINMENT_BOARD_GAMES, "Entertainment: Board Games"},
     {quiz::core::Category::SCIENCE_NATURE, "Science & Nature"},
     {quiz::core::Category::SCIENCE_COMPUTERS, "Science: Computers"},
     {quiz::core::Category::SCIENCE_MATHEMATICS, "Science: Mathematics"},
     {quiz::core::Category::MYTHOLOGY, "Mythology"},
     {quiz::core::Category::SPORTS, "Sports"},
     {quiz::core::Category::GEOGRAPHY, "Geography"},
     {quiz::core::Category::HISTORY, "History"},
     {quiz::core::Category::POLITICS, "Politics"},
     {quiz::core::Category::ART, "Art"},
     {quiz::core::Category::CELEBRITIES, "Celebrities"},
     {quiz::core::Category::ANIMALS, "Animals"},
     {quiz::core::Category::VEHICLES, "Vehicles"},
     {quiz::core::Category::ENTERTAINMENT_COMICS, "Entertainment: Comics"},
     {quiz::core::Category::SCIENCE_GADGETS, "Science: Gadgets"},
     {quiz::core::Category::ENTERTAINMENT_ANIME, "Entertainment: Japanese Anime & Manga"},
     {quiz::core::Category::ENTERTAINMENT_CARTOON_ANIMATIONS, "Entertainment: Cartoon & Animations"}}};

  struct HasCategory
  {
    quiz::core::Category category;

    bool operator()(const CategoryName &entry) const
    {
      return entry.category == category;
    }
  };

  struct HasText
  {
    std::string lower_text;

    bool operator()(const CategoryName &entry) const
    {
      return lower_text == toLower(std::string(entry.name)) ||
             lower_text == std::to_string(static_cast< int >(entry.category));
    }
  };
}

std::string quiz::core::toString(Difficulty difficulty)
{
  switch (difficulty)
  {
  case Difficulty::ANY:
    return "any";
  case Difficulty::EASY:
    return "easy";
  case Difficulty::MEDIUM:
    return "medium";
  case Difficulty::HARD:
    return "hard";
  }
  throw std::invalid_argument("Unknown difficulty");
}

std::string quiz::core::toString(QuestionType type)
{
  switch (type)
  {
  case QuestionType::ANY:
    return "any";
  case QuestionType::MULTIPLE:
    return "multiple";
  case QuestionType::BOOLEAN:
    return "boolean";
  }
  throw std::invalid_argument("Unknown question type");
}

std::string quiz::core::toString(Category category)
{
  const auto *it = std::find_if(CATEGORY_NAMES.begin(), CATEGORY_NAMES.end(), HasCategory{category});
  if (it == CATEGORY_NAMES.end())
  {
    throw std::invalid_argument("Unknown category");
  }
  return std::string(it->name);
}

quiz::core::Difficulty quiz::core::parseDifficulty(const std::string &text)
{
  const std::string LOWER_TEXT = toLower(text);

  if (LOWER_TEXT == "any")
  {
    return Difficulty::ANY;
  }
  if (LOWER_TEXT == "easy")
  {
    return Difficulty::EASY;
  }
  if (LOWER_TEXT == "medium")
  {
    return Difficulty::MEDIUM;
  }
  if (LOWER_TEXT == "hard")
  {
    return Difficulty::HARD;
  }

  throw std::invalid_argument("Unknown difficulty");
}

quiz::core::QuestionType quiz::core::parseQuestionType(const std::string &text)
{
  const std::string LOWER_TEXT = toLower(text);

  if (LOWER_TEXT == "any")
  {
    return QuestionType::ANY;
  }
  if (LOWER_TEXT == "multiple")
  {
    return QuestionType::MULTIPLE;
  }
  if (LOWER_TEXT == "boolean")
  {
    return QuestionType::BOOLEAN;
  }

  throw std::invalid_argument("Unknown question type");
}

quiz::core::Category quiz::core::parseCategory(const std::string &text)
{
  const std::string LOWER_TEXT = toLower(text);

  if (LOWER_TEXT == "any")
  {
    return Category::ANY;
  }
  const auto *it = std::find_if(CATEGORY_NAMES.begin(), CATEGORY_NAMES.end(), HasText{LOWER_TEXT});
  if (it == CATEGORY_NAMES.end())
  {
    throw std::invalid_argument("Unknown category");
  }
  return it->category;
}
