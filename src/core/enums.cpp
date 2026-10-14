#include "enums.hpp"
#include <algorithm>
#include <cctype>

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
  const std::string lower_text = toLower(text);

  if (lower_text == "any")
  {
    return Difficulty::ANY;
  }
  if (lower_text == "easy")
  {
    return Difficulty::EASY;
  }
  if (lower_text == "medium")
  {
    return Difficulty::MEDIUM;
  }
  if (lower_text == "hard")
  {
    return Difficulty::HARD;
  }

  return Difficulty::ANY;
}

quiz::core::QuestionType quiz::core::parseQuestionType(const std::string &text)
{
  const std::string lower_text = toLower(text);

  if (lower_text == "any")
  {
    return QuestionType::ANY;
  }
  if (lower_text == "multiple")
  {
    return QuestionType::MULTIPLE;
  }
  if (lower_text == "boolean")
  {
    return QuestionType::BOOLEAN;
  }

  return QuestionType::ANY;
}

quiz::core::Category quiz::core::parseCategory(const std::string &text)
{
  const std::string lower_text = toLower(text);

  if (lower_text == "any" || lower_text == "0")
  {
    return Category::ANY;
  }
  if (lower_text == "general knowledge" || lower_text == "9")
  {
    return Category::GENERAL_KNOWLEDGE;
  }
  if (lower_text == "entertainment: books" || lower_text == "10")
  {
    return Category::ENTERTAINMENT_BOOKS;
  }
  if (lower_text == "entertainment: film" || lower_text == "11")
  {
    return Category::ENTERTAINMENT_FILM;
  }
  if (lower_text == "entertainment: music" || lower_text == "12")
  {
    return Category::ENTERTAINMENT_MUSIC;
  }
  if (lower_text == "entertainment: musicals & theatres" || lower_text == "13")
  {
    return Category::ENTERTAINMENT_MUSICALS_THEATRES;
  }
  if (lower_text == "entertainment: television" || lower_text == "14")
  {
    return Category::ENTERTAINMENT_TELEVISION;
  }
  if (lower_text == "entertainment: video games" || lower_text == "15")
  {
    return Category::ENTERTAINMENT_VIDEO_GAMES;
  }
  if (lower_text == "entertainment: board games" || lower_text == "16")
  {
    return Category::ENTERTAINMENT_BOARD_GAMES;
  }
  if (lower_text == "science & nature" || lower_text == "17")
  {
    return Category::SCIENCE_NATURE;
  }
  if (lower_text == "science: computers" || lower_text == "18")
  {
    return Category::SCIENCE_COMPUTERS;
  }
  if (lower_text == "science: mathematics" || lower_text == "19")
  {
    return Category::SCIENCE_MATHEMATICS;
  }
  if (lower_text == "mythology" || lower_text == "20")
  {
    return Category::MYTHOLOGY;
  }
  if (lower_text == "sports" || lower_text == "21")
  {
    return Category::SPORTS;
  }
  if (lower_text == "geography" || lower_text == "22")
  {
    return Category::GEOGRAPHY;
  }
  if (lower_text == "history" || lower_text == "23")
  {
    return Category::HISTORY;
  }
  if (lower_text == "politics" || lower_text == "24")
  {
    return Category::POLITICS;
  }
  if (lower_text == "art" || lower_text == "25")
  {
    return Category::ART;
  }
  if (lower_text == "celebrities" || lower_text == "26")
  {
    return Category::CELEBRITIES;
  }
  if (lower_text == "animals" || lower_text == "27")
  {
    return Category::ANIMALS;
  }
  if (lower_text == "vehicles" || lower_text == "28")
  {
    return Category::VEHICLES;
  }
  if (lower_text == "entertainment: comics" || lower_text == "29")
  {
    return Category::ENTERTAINMENT_COMICS;
  }
  if (lower_text == "science: gadgets" || lower_text == "30")
  {
    return Category::SCIENCE_GADGETS;
  }
  if (lower_text == "entertainment: japanese anime & manga" || lower_text == "31")
  {
    return Category::ENTERTAINMENT_ANIME;
  }
  if (lower_text == "entertainment: cartoon & animations" || lower_text == "32")
  {
    return Category::ENTERTAINMENT_CARTOON_ANIMATIONS;
  }
  return Category::ANY;
}

