#include "test_support.hpp"
#include "tui/prompter.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(prompter_tests)

BOOST_AUTO_TEST_CASE(ask_text_returns_valid_input)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"hello"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askText("Enter text");

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(*result, "hello");
}

BOOST_AUTO_TEST_CASE(ask_text_rejects_empty_input)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"", "  ", "valid"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askText("Enter text");

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(*result, "valid");
}

BOOST_AUTO_TEST_CASE(ask_text_back_command)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askText("Enter text");

  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(ask_number_valid_input)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"5"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askNumber("Enter number", {1, 10});

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(*result, 5);
}

BOOST_AUTO_TEST_CASE(ask_number_rejects_invalid)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"abc", "15", "7"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askNumber("Enter number", {1, 10});

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(*result, 7);
}

BOOST_AUTO_TEST_CASE(ask_choice_returns_index)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"2"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askChoice("Choose", {"Option 1", "Option 2", "Option 3"});

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(*result, 1);
}

BOOST_AUTO_TEST_CASE(ask_choice_throws_on_empty_options)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Prompter prompter(terminal);

  BOOST_CHECK_THROW(prompter.askChoice("Choose", {}), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(ask_yes_no_yes_input)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"y"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askYesNo("Confirm?", false);

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(*result);
}

BOOST_AUTO_TEST_CASE(ask_yes_no_no_input)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"n"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askYesNo("Confirm?", true);

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(!*result);
}

BOOST_AUTO_TEST_CASE(ask_yes_no_default_on_empty)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({""});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askYesNo("Confirm?", true);

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(*result);
}

BOOST_AUTO_TEST_CASE(ask_selection_single_number)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"3"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askSelection("Select", 10);

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->size(), 1);
  BOOST_CHECK_EQUAL((*result)[0], 3);
}

BOOST_AUTO_TEST_CASE(ask_selection_range)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"2-5"});

  quiz::tui::Prompter prompter(terminal);
  const auto result = prompter.askSelection("Select", 10);

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->size(), 4);
}

BOOST_AUTO_TEST_SUITE_END()
