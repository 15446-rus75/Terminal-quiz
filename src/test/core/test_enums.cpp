#define BOOST_TEST_MODULE core_tests
#include <boost/test/unit_test.hpp>
#include <array>
#include <stdexcept>
#include <string>
#include "core/enums.hpp"

namespace core = quiz::core;

BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::Difficulty)
BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::QuestionType)
BOOST_TEST_DONT_PRINT_LOG_VALUE(quiz::core::Category)

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

BOOST_AUTO_TEST_CASE(category_names_match_open_tdb)
{
  BOOST_TEST(core::toString(core::Category::ANY) == "Any Category");
  BOOST_TEST(core::toString(core::Category::GENERAL_KNOWLEDGE) == "General Knowledge");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_BOOKS) == "Entertainment: Books");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_FILM) == "Entertainment: Film");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_MUSIC) == "Entertainment: Music");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_MUSICALS_THEATRES) == "Entertainment: Musicals & Theatres");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_TELEVISION) == "Entertainment: Television");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_VIDEO_GAMES) == "Entertainment: Video Games");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_BOARD_GAMES) == "Entertainment: Board Games");
  BOOST_TEST(core::toString(core::Category::SCIENCE_NATURE) == "Science & Nature");
  BOOST_TEST(core::toString(core::Category::SCIENCE_COMPUTERS) == "Science: Computers");
  BOOST_TEST(core::toString(core::Category::SCIENCE_MATHEMATICS) == "Science: Mathematics");
  BOOST_TEST(core::toString(core::Category::MYTHOLOGY) == "Mythology");
  BOOST_TEST(core::toString(core::Category::SPORTS) == "Sports");
  BOOST_TEST(core::toString(core::Category::GEOGRAPHY) == "Geography");
  BOOST_TEST(core::toString(core::Category::HISTORY) == "History");
  BOOST_TEST(core::toString(core::Category::POLITICS) == "Politics");
  BOOST_TEST(core::toString(core::Category::ART) == "Art");
  BOOST_TEST(core::toString(core::Category::CELEBRITIES) == "Celebrities");
  BOOST_TEST(core::toString(core::Category::ANIMALS) == "Animals");
  BOOST_TEST(core::toString(core::Category::VEHICLES) == "Vehicles");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_COMICS) == "Entertainment: Comics");
  BOOST_TEST(core::toString(core::Category::SCIENCE_GADGETS) == "Science: Gadgets");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_ANIME) == "Entertainment: Japanese Anime & Manga");
  BOOST_TEST(core::toString(core::Category::ENTERTAINMENT_CARTOON_ANIMATIONS) == "Entertainment: Cartoon & Animations");
}

BOOST_AUTO_TEST_CASE(category_unknown_throws)
{
  BOOST_CHECK_THROW(core::toString(static_cast< core::Category >(1)), std::invalid_argument);
  BOOST_CHECK_THROW(core::toString(static_cast< core::Category >(33)), std::invalid_argument);
  BOOST_CHECK_THROW(core::toString(static_cast< core::Category >(-1)), std::invalid_argument);
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
  const std::array< core::Difficulty, 4 > VALUES = { core::Difficulty::ANY, core::Difficulty::EASY,
      core::Difficulty::MEDIUM, core::Difficulty::HARD };
  for (const core::Difficulty value: VALUES)
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
  const std::array< core::QuestionType, 3 > VALUES = { core::QuestionType::ANY, core::QuestionType::MULTIPLE,
      core::QuestionType::BOOLEAN };
  for (const core::QuestionType value: VALUES)
  {
    BOOST_TEST(core::parseQuestionType(core::toString(value)) == value);
  }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(enums_parse_category)

BOOST_AUTO_TEST_CASE(any_keyword)
{
  BOOST_TEST(core::parseCategory("any") == core::Category::ANY);
  BOOST_TEST(core::parseCategory("ANY") == core::Category::ANY);
  BOOST_TEST(core::parseCategory("0") == core::Category::ANY);
}

BOOST_AUTO_TEST_CASE(by_name_case_insensitive)
{
  BOOST_TEST(core::parseCategory("General Knowledge") == core::Category::GENERAL_KNOWLEDGE);
  BOOST_TEST(core::parseCategory("general knowledge") == core::Category::GENERAL_KNOWLEDGE);
  BOOST_TEST(core::parseCategory("SCIENCE & NATURE") == core::Category::SCIENCE_NATURE);
  BOOST_TEST(core::parseCategory("entertainment: japanese anime & manga") == core::Category::ENTERTAINMENT_ANIME);
}

BOOST_AUTO_TEST_CASE(by_numeric_id)
{
  BOOST_TEST(core::parseCategory("9") == core::Category::GENERAL_KNOWLEDGE);
  BOOST_TEST(core::parseCategory("17") == core::Category::SCIENCE_NATURE);
  BOOST_TEST(core::parseCategory("32") == core::Category::ENTERTAINMENT_CARTOON_ANIMATIONS);
}

BOOST_AUTO_TEST_CASE(unknown_throws)
{
  BOOST_CHECK_THROW(core::parseCategory(""), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("cooking"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("1"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("33"), std::invalid_argument);
  BOOST_CHECK_THROW(core::parseCategory("-1"), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(round_trip_for_every_category)
{
  const std::array< core::Category, 25 > VALUES = { core::Category::ANY,
      core::Category::GENERAL_KNOWLEDGE,
      core::Category::ENTERTAINMENT_BOOKS,
      core::Category::ENTERTAINMENT_FILM,
      core::Category::ENTERTAINMENT_MUSIC,
      core::Category::ENTERTAINMENT_MUSICALS_THEATRES,
      core::Category::ENTERTAINMENT_TELEVISION,
      core::Category::ENTERTAINMENT_VIDEO_GAMES,
      core::Category::ENTERTAINMENT_BOARD_GAMES,
      core::Category::SCIENCE_NATURE,
      core::Category::SCIENCE_COMPUTERS,
      core::Category::SCIENCE_MATHEMATICS,
      core::Category::MYTHOLOGY,
      core::Category::SPORTS,
      core::Category::GEOGRAPHY,
      core::Category::HISTORY,
      core::Category::POLITICS,
      core::Category::ART,
      core::Category::CELEBRITIES,
      core::Category::ANIMALS,
      core::Category::VEHICLES,
      core::Category::ENTERTAINMENT_COMICS,
      core::Category::SCIENCE_GADGETS,
      core::Category::ENTERTAINMENT_ANIME,
      core::Category::ENTERTAINMENT_CARTOON_ANIMATIONS };
  for (const core::Category value: VALUES)
  {
    BOOST_TEST(core::parseCategory(core::toString(value)) == value);
    BOOST_TEST(core::parseCategory(std::to_string(static_cast< int >(value))) == value);
  }
}

BOOST_AUTO_TEST_SUITE_END()
