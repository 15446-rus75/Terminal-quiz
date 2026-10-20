#define BOOST_TEST_MODULE core_tests
#include <array>
#include <stdexcept>
#include <string>

#include "core/enums.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;

BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::Difficulty)
BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::QuestionType)
BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::Category)

namespace
{
  struct CategoryRaw
  {
    core::Category category;
    int id;
    const char *name;
  };

  const std::array< CategoryRaw, 25 > CATEGORIES = {
    {{core::Category::ANY, 0, "Any Category"},
     {core::Category::GENERAL_KNOWLEDGE, 9, "General Knowledge"},
     {core::Category::ENTERTAINMENT_BOOKS, 10, "Entertainment: Books"},
     {core::Category::ENTERTAINMENT_FILM, 11, "Entertainment: Film"},
     {core::Category::ENTERTAINMENT_MUSIC, 12, "Entertainment: Music"},
     {core::Category::ENTERTAINMENT_MUSICALS_THEATRES, 13, "Entertainment: Musicals & Theatres"},
     {core::Category::ENTERTAINMENT_TELEVISION, 14, "Entertainment: Television"},
     {core::Category::ENTERTAINMENT_VIDEO_GAMES, 15, "Entertainment: Video Games"},
     {core::Category::ENTERTAINMENT_BOARD_GAMES, 16, "Entertainment: Board Games"},
     {core::Category::SCIENCE_NATURE, 17, "Science & Nature"},
     {core::Category::SCIENCE_COMPUTERS, 18, "Science: Computers"},
     {core::Category::SCIENCE_MATHEMATICS, 19, "Science: Mathematics"},
     {core::Category::MYTHOLOGY, 20, "Mythology"},
     {core::Category::SPORTS, 21, "Sports"},
     {core::Category::GEOGRAPHY, 22, "Geography"},
     {core::Category::HISTORY, 23, "History"},
     {core::Category::POLITICS, 24, "Politics"},
     {core::Category::ART, 25, "Art"},
     {core::Category::CELEBRITIES, 26, "Celebrities"},
     {core::Category::ANIMALS, 27, "Animals"},
     {core::Category::VEHICLES, 28, "Vehicles"},
     {core::Category::ENTERTAINMENT_COMICS, 29, "Entertainment: Comics"},
     {core::Category::SCIENCE_GADGETS, 30, "Science: Gadgets"},
     {core::Category::ENTERTAINMENT_ANIME, 31, "Entertainment: Japanese Anime & Manga"},
     {core::Category::ENTERTAINMENT_CARTOON_ANIMATIONS, 32, "Entertainment: Cartoon & Animations"}}};
}

BOOST_AUTO_TEST_SUITE(enums_to_string)

BOOST_AUTO_TEST_CASE(difficulty_values)
{
  BOOST_TEST(core::toString(core::Difficulty::ANY) == "any");
  BOOST_TEST(core::toString(core::Difficulty::EASY) == "easy");
  BOOST_TEST(core::toString(core::Difficulty::MEDIUM) == "medium");
  BOOST_TEST(core::toString(core::Difficulty::HARD) == "hard");
}

BOOST_AUTO_TEST_CASE(difficulty_unknown_throws)
{
  BOOST_CHECK_THROW(core::toString(static_cast< core::Difficulty >(100)), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(question_type_values)
{
  BOOST_TEST(core::toString(core::QuestionType::ANY) == "any");
  BOOST_TEST(core::toString(core::QuestionType::MULTIPLE) == "multiple");
  BOOST_TEST(core::toString(core::QuestionType::BOOLEAN) == "boolean");
}

BOOST_AUTO_TEST_CASE(question_type_unknown_throws)
{
  BOOST_CHECK_THROW(core::toString(static_cast< core::QuestionType >(100)), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(category_unknown_throws)
{
  BOOST_CHECK_THROW(core::toString(static_cast< core::Category >(-1)), std::invalid_argument);
  BOOST_CHECK_THROW(core::toString(static_cast< core::Category >(8)), std::invalid_argument);
  BOOST_CHECK_THROW(core::toString(static_cast< core::Category >(33)), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(enums_parse_difficulty)

BOOST_AUTO_TEST_CASE(valid_values)
{
  BOOST_TEST(core::parseDifficulty("any") == core::Difficulty::ANY);
  BOOST_TEST(core::parseDifficulty("easy") == core::Difficulty::EASY);
  BOOST_TEST(core::parseDifficulty("medium") == core::Difficulty::MEDIUM);
  BOOST_TEST(core::parseDifficulty("hard") == core::Difficulty::HARD);
}

BOOST_AUTO_TEST_CASE(case_insensitive)
{
  BOOST_TEST(core::parseDifficulty("EASY") == core::Difficulty::EASY);
  BOOST_TEST(core::parseDifficulty("MeDiUm") == core::Difficulty::MEDIUM);
  BOOST_TEST(core::parseDifficulty("HARD") == core::Difficulty::HARD);
}

BOOST_AUTO_TEST_CASE(unknown_throws)
{
  BOOST_CHECK_THROW(core::parseDifficulty(""), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseDifficulty("impossible"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseDifficulty(" easy"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(round_trip)
{
  const std::array< core::Difficulty, 4 > VALUES = {core::Difficulty::ANY, core::Difficulty::EASY,
                                                    core::Difficulty::MEDIUM, core::Difficulty::HARD};
  for (const core::Difficulty value : VALUES)
  {
    BOOST_TEST(core::parseDifficulty(core::toString(value)) == value);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(enums_parse_question_type)

BOOST_AUTO_TEST_CASE(valid_values)
{
  BOOST_TEST(core::parseQuestionType("any") == core::QuestionType::ANY);
  BOOST_TEST(core::parseQuestionType("multiple") == core::QuestionType::MULTIPLE);
  BOOST_TEST(core::parseQuestionType("boolean") == core::QuestionType::BOOLEAN);
}

BOOST_AUTO_TEST_CASE(case_insensitive)
{
  BOOST_TEST(core::parseQuestionType("MULTIPLE") == core::QuestionType::MULTIPLE);
  BOOST_TEST(core::parseQuestionType("Boolean") == core::QuestionType::BOOLEAN);
}

BOOST_AUTO_TEST_CASE(unknown_throws)
{
  BOOST_CHECK_THROW(core::parseQuestionType(""), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseQuestionType("single"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(round_trip)
{
  const std::array< core::QuestionType, 3 > VALUES = {core::QuestionType::ANY, core::QuestionType::MULTIPLE,
                                                      core::QuestionType::BOOLEAN};
  for (const core::QuestionType value : VALUES)
  {
    BOOST_TEST(core::parseQuestionType(core::toString(value)) == value);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(enums_category)

BOOST_AUTO_TEST_CASE(conversions_for_every_category)
{
  for (const CategoryRaw &row : CATEGORIES)
  {
    BOOST_TEST(static_cast< int >(row.category) == row.id);
    BOOST_TEST(core::toString(row.category) == row.name);
    BOOST_TEST(core::parseCategory(row.name) == row.category);
    BOOST_TEST(core::parseCategory(std::to_string(row.id)) == row.category);
  }
}

BOOST_AUTO_TEST_CASE(any_keyword)
{
  BOOST_TEST(core::parseCategory("any") == core::Category::ANY);
  BOOST_TEST(core::parseCategory("ANY") == core::Category::ANY);
}

BOOST_AUTO_TEST_CASE(parse_case_insensitive)
{
  BOOST_TEST(core::parseCategory("general knowledge") == core::Category::GENERAL_KNOWLEDGE);
  BOOST_TEST(core::parseCategory("SCIENCE & NATURE") == core::Category::SCIENCE_NATURE);
  BOOST_TEST(core::parseCategory("entertainment: japanese anime & manga") == core::Category::ENTERTAINMENT_ANIME);
}

BOOST_AUTO_TEST_CASE(parse_unknown_throws)
{
  BOOST_CHECK_THROW(core::parseCategory(""), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("cooking"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("Science"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("Entertainment"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("Video Games"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("Computers"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("Science: Math"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("Sport"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("Entertainment: Anime"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("8"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("33"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("-1"), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()
