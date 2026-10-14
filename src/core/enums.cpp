#include "enums.hpp"

#include <algorithm>
#include <cctype>
#include <string>

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
    std::transform(text.begin(), text.end(), text.begin(), Lowerer());
    return text;
  }
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
  return "";
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
  return "";
}

std::string quiz::core::toString(Category category)
{
  switch (category)
  {
  case Category::ANY:
    return "any";
  case Category::GENERAL_KNOWLEDGE:
    return "General knowledge";
  case Category::ENTERTAINMENT_BOOKS:
    return "Entertainment: books";
  case Category::ENTERTAINMENT_FILM:
    return "Entertainment: film";
  case Category::ENTERTAINMENT_MUSIC:
    return "Entertainment: music";
  case Category::ENTERTAINMENT_MUSICALS_THEATRES:
    return "Entertainment: musicals & theatres";
  case Category::ENTERTAINMENT_TELEVISION:
    return "Entertainment: television";
  case Category::ENTERTAINMENT_VIDEO_GAMES:
    return "Entertainment: video games";
  case Category::ENTERTAINMENT_BOARD_GAMES:
    return "Entertainment: board games";
  case Category::SCIENCE_NATURE:
    return "Science & nature";
  case Category::SCIENCE_COMPUTERS:
    return "Science: computers";
  case Category::SCIENCE_MATHEMATICS:
    return "Science: mathematics";
  case Category::MYTHOLOGY:
    return "Mythology";
  case Category::SPORTS:
    return "Sports";
  case Category::GEOGRAPHY:
    return "Geography";
  case Category::HISTORY:
    return "History";
  case Category::POLITICS:
    return "Politics";
  case Category::ART:
    return "Art";
  case Category::CELEBRITIES:
    return "Celebrities";
  case Category::ANIMALS:
    return "Animals";
  case Category::VEHICLES:
    return "Vehicles";
  case Category::ENTERTAINMENT_COMICS:
    return "Entertainment: comics";
  case Category::SCIENCE_GADGETS:
    return "Science: gadgets";
  case Category::ENTERTAINMENT_ANIME:
    return "Entertainment: japanese anime & manga";
  case Category::ENTERTAINMENT_CARTOON_ANIMATIONS:
    return "Entertainment: cartoon & animations";
  }
  return "";
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

  return Difficulty::ANY;
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

  return QuestionType::ANY;
}

quiz::core::Category quiz::core::parseCategory(const std::string &text)
{
  const std::string LOWER_TEXT = toLower(text);

  if (LOWER_TEXT == "any" || LOWER_TEXT == "0")
  {
    return Category::ANY;
  }
  if (LOWER_TEXT == "general knowledge" || LOWER_TEXT == "9")
  {
    return Category::GENERAL_KNOWLEDGE;
  }
  if (LOWER_TEXT == "entertainment: books" || LOWER_TEXT == "10")
  {
    return Category::ENTERTAINMENT_BOOKS;
  }
  if (LOWER_TEXT == "entertainment: film" || LOWER_TEXT == "11")
  {
    return Category::ENTERTAINMENT_FILM;
  }
  if (LOWER_TEXT == "entertainment: music" || LOWER_TEXT == "12")
  {
    return Category::ENTERTAINMENT_MUSIC;
  }
  if (LOWER_TEXT == "entertainment: musicals & theatres" || LOWER_TEXT == "13")
  {
    return Category::ENTERTAINMENT_MUSICALS_THEATRES;
  }
  if (LOWER_TEXT == "entertainment: television" || LOWER_TEXT == "14")
  {
    return Category::ENTERTAINMENT_TELEVISION;
  }
  if (LOWER_TEXT == "entertainment: video games" || LOWER_TEXT == "15")
  {
    return Category::ENTERTAINMENT_VIDEO_GAMES;
  }
  if (LOWER_TEXT == "entertainment: board games" || LOWER_TEXT == "16")
  {
    return Category::ENTERTAINMENT_BOARD_GAMES;
  }
  if (LOWER_TEXT == "science & nature" || LOWER_TEXT == "17")
  {
    return Category::SCIENCE_NATURE;
  }
  if (LOWER_TEXT == "science: computers" || LOWER_TEXT == "18")
  {
    return Category::SCIENCE_COMPUTERS;
  }
  if (LOWER_TEXT == "science: mathematics" || LOWER_TEXT == "19")
  {
    return Category::SCIENCE_MATHEMATICS;
  }
  if (LOWER_TEXT == "mythology" || LOWER_TEXT == "20")
  {
    return Category::MYTHOLOGY;
  }
  if (LOWER_TEXT == "sports" || LOWER_TEXT == "21")
  {
    return Category::SPORTS;
  }
  if (LOWER_TEXT == "geography" || LOWER_TEXT == "22")
  {
    return Category::GEOGRAPHY;
  }
  if (LOWER_TEXT == "history" || LOWER_TEXT == "23")
  {
    return Category::HISTORY;
  }
  if (LOWER_TEXT == "politics" || LOWER_TEXT == "24")
  {
    return Category::POLITICS;
  }
  if (LOWER_TEXT == "art" || LOWER_TEXT == "25")
  {
    return Category::ART;
  }
  if (LOWER_TEXT == "celebrities" || LOWER_TEXT == "26")
  {
    return Category::CELEBRITIES;
  }
  if (LOWER_TEXT == "animals" || LOWER_TEXT == "27")
  {
    return Category::ANIMALS;
  }
  if (LOWER_TEXT == "vehicles" || LOWER_TEXT == "28")
  {
    return Category::VEHICLES;
  }
  if (LOWER_TEXT == "entertainment: comics" || LOWER_TEXT == "29")
  {
    return Category::ENTERTAINMENT_COMICS;
  }
  if (LOWER_TEXT == "science: gadgets" || LOWER_TEXT == "30")
  {
    return Category::SCIENCE_GADGETS;
  }
  if (LOWER_TEXT == "entertainment: japanese anime & manga" || LOWER_TEXT == "31")
  {
    return Category::ENTERTAINMENT_ANIME;
  }
  if (LOWER_TEXT == "entertainment: cartoon & animations" || LOWER_TEXT == "32")
  {
    return Category::ENTERTAINMENT_CARTOON_ANIMATIONS;
  }
  return Category::ANY;
}
