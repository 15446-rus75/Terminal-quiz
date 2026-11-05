#include <boost/test/unit_test.hpp>
#include "tui/input_parsing.hpp"

BOOST_AUTO_TEST_SUITE(input_parsing_tests)

BOOST_AUTO_TEST_CASE(parse_command_help_aliases)
{
  BOOST_CHECK(quiz::tui::parseCommand("/help") == quiz::tui::Command::HELP);
  BOOST_CHECK(quiz::tui::parseCommand("/h") == quiz::tui::Command::HELP);
  BOOST_CHECK(quiz::tui::parseCommand("?") == quiz::tui::Command::HELP);
}

BOOST_AUTO_TEST_CASE(parse_command_back_aliases)
{
  BOOST_CHECK(quiz::tui::parseCommand("/back") == quiz::tui::Command::BACK);
  BOOST_CHECK(quiz::tui::parseCommand("/b") == quiz::tui::Command::BACK);
}

BOOST_AUTO_TEST_CASE(parse_command_quit_aliases)
{
  BOOST_CHECK(quiz::tui::parseCommand("/quit") == quiz::tui::Command::QUIT);
  BOOST_CHECK(quiz::tui::parseCommand("/q") == quiz::tui::Command::QUIT);
}

BOOST_AUTO_TEST_CASE(parse_command_case_insensitive)
{
  BOOST_CHECK(quiz::tui::parseCommand("/HELP") == quiz::tui::Command::HELP);
  BOOST_CHECK(quiz::tui::parseCommand("/Help") == quiz::tui::Command::HELP);
  BOOST_CHECK(quiz::tui::parseCommand("/BACK") == quiz::tui::Command::BACK);
}

BOOST_AUTO_TEST_CASE(parse_command_trims_whitespace)
{
  BOOST_CHECK(quiz::tui::parseCommand("  /help  ") == quiz::tui::Command::HELP);
  BOOST_CHECK(quiz::tui::parseCommand("\t/back\t") == quiz::tui::Command::BACK);
}

BOOST_AUTO_TEST_CASE(parse_command_unknown)
{
  BOOST_CHECK(quiz::tui::parseCommand("hello") == quiz::tui::Command::NONE);
  BOOST_CHECK(quiz::tui::parseCommand("/unknown") == quiz::tui::Command::NONE);
  BOOST_CHECK(quiz::tui::parseCommand("") == quiz::tui::Command::NONE);
}

BOOST_AUTO_TEST_CASE(parse_number_valid)
{
  const quiz::tui::NumberRange range{1, 10};
  const auto result1 = quiz::tui::parseNumber("5", range);
  BOOST_REQUIRE(result1.has_value());
  BOOST_CHECK_EQUAL(*result1, 5);
  
  const auto result2 = quiz::tui::parseNumber("1", range);
  BOOST_REQUIRE(result2.has_value());
  BOOST_CHECK_EQUAL(*result2, 1);
  
  const auto result3 = quiz::tui::parseNumber("10", range);
  BOOST_REQUIRE(result3.has_value());
  BOOST_CHECK_EQUAL(*result3, 10);
}

BOOST_AUTO_TEST_CASE(parse_number_out_of_range)
{
  const quiz::tui::NumberRange range{1, 10};
  BOOST_CHECK(!quiz::tui::parseNumber("0", range).has_value());
  BOOST_CHECK(!quiz::tui::parseNumber("11", range).has_value());
}

BOOST_AUTO_TEST_CASE(parse_number_invalid_text)
{
  const quiz::tui::NumberRange range{1, 10};
  BOOST_CHECK(!quiz::tui::parseNumber("abc", range).has_value());
  BOOST_CHECK(!quiz::tui::parseNumber("", range).has_value());
  BOOST_CHECK(!quiz::tui::parseNumber("5x", range).has_value());
}

BOOST_AUTO_TEST_CASE(parse_number_trims_whitespace)
{
  const quiz::tui::NumberRange range{1, 10};
  const auto result = quiz::tui::parseNumber("  5  ", range);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(*result, 5);
}

BOOST_AUTO_TEST_CASE(parse_selection_single_number)
{
  const auto result = quiz::tui::parseSelection("3", 10);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->size(), 1);
  BOOST_CHECK_EQUAL((*result)[0], 3);
}

BOOST_AUTO_TEST_CASE(parse_selection_comma_separated)
{
  const auto result = quiz::tui::parseSelection("1,3,5", 10);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->size(), 3);
  BOOST_CHECK_EQUAL((*result)[0], 1);
  BOOST_CHECK_EQUAL((*result)[1], 3);
  BOOST_CHECK_EQUAL((*result)[2], 5);
}

BOOST_AUTO_TEST_CASE(parse_selection_range)
{
  const auto result = quiz::tui::parseSelection("2-5", 10);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->size(), 4);
  BOOST_CHECK_EQUAL((*result)[0], 2);
  BOOST_CHECK_EQUAL((*result)[1], 3);
  BOOST_CHECK_EQUAL((*result)[2], 4);
  BOOST_CHECK_EQUAL((*result)[3], 5);
}

BOOST_AUTO_TEST_CASE(parse_selection_mixed)
{
  const auto result = quiz::tui::parseSelection("1,3-5,7", 10);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->size(), 5);
  BOOST_CHECK_EQUAL((*result)[0], 1);
  BOOST_CHECK_EQUAL((*result)[1], 3);
  BOOST_CHECK_EQUAL((*result)[2], 4);
  BOOST_CHECK_EQUAL((*result)[3], 5);
  BOOST_CHECK_EQUAL((*result)[4], 7);
}

BOOST_AUTO_TEST_CASE(parse_selection_deduplication_and_sorting)
{
  const auto result = quiz::tui::parseSelection("5,3,1,3,5", 10);
  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->size(), 3);
  BOOST_CHECK_EQUAL((*result)[0], 1);
  BOOST_CHECK_EQUAL((*result)[1], 3);
  BOOST_CHECK_EQUAL((*result)[2], 5);
}

BOOST_AUTO_TEST_CASE(parse_selection_empty)
{
  BOOST_CHECK(!quiz::tui::parseSelection("", 10).has_value());
  BOOST_CHECK(!quiz::tui::parseSelection(",,,", 10).has_value());
}

BOOST_AUTO_TEST_CASE(parse_selection_invalid_range)
{
  BOOST_CHECK(!quiz::tui::parseSelection("5-2", 10).has_value());
}

BOOST_AUTO_TEST_CASE(parse_selection_out_of_bounds)
{
  BOOST_CHECK(!quiz::tui::parseSelection("0", 10).has_value());
  BOOST_CHECK(!quiz::tui::parseSelection("11", 10).has_value());
}

BOOST_AUTO_TEST_SUITE_END()
