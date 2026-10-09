#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <cstdint>
#include <string>

namespace quiz
{
  namespace core
  {
    enum class Difficulty
    {
      ANY = 0,
      EASY = 1,
      MEDIUM = 2,
      HARD = 3
    };

    enum class QuestionTypes
    {
      ANY = 0,
      MULTIPLE = 1,
      BOOLEAN = 2
    };

    enum class Category
    {
      ANY = 0,
      GENERAL_KNOWLEDGE = 9,
      ENTERTAINMENT_BOOKS = 10,
      ENTERTAINMENT_FILM = 11,
      ENTERTAINMENT_MUSIC = 12,
      ENTERTAINMENT_MUSICALS_THEATRES = 13,
      ENTERTAINMENT_TELEVISION = 14,
      ENTERTAINMENT_VIDEO_GAMES = 15,
      ENTERTAINMENT_BOARD_GAMES = 16,
      SCIENCE_NATURE = 17,
      SCIENCE_COMPUTERS = 18,
      SCIENCE_MATHEMATICS = 19,
      MYTHOLOGY = 20,
      SPORTS = 21,
      GEOGRAPHY = 22,
      HISTORY = 23,
      POLITICS = 24,
      ART = 25,
      CELEBRITIES = 26,
      ANIMALS = 27,
      VEHICLES = 28,
      ENTERTAINMENT_COMICS = 29,
      SCIENCE_GADGETS = 30,
      ENTERTAINMENT_ANIME = 31,
      ENTERTAINMENT_CARTOON_ANIMATIONS = 32
    };

    std::string toString(Difficulty difficulty);
    std::string toString(QuestionType type);
    std::string toString(Category category);
  }
}

#endif
