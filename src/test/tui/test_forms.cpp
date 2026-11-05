#include <boost/test/unit_test.hpp>
#include "tui/forms.hpp"
#include "test_support.hpp"

BOOST_AUTO_TEST_SUITE(forms_tests)

BOOST_AUTO_TEST_CASE(ask_quiz_parameters_valid_input)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"5", "1", "1", "1"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askQuizParameters();

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->amount, 5);
  BOOST_CHECK(result->category == quiz::core::Category::ANY);
  BOOST_CHECK(result->difficulty == quiz::core::Difficulty::ANY);
  BOOST_CHECK(result->type == quiz::core::QuestionType::ANY);
}

BOOST_AUTO_TEST_CASE(ask_quiz_parameters_back_on_amount)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askQuizParameters();

  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(ask_quiz_parameters_back_on_category)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"5", "/back"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askQuizParameters();

  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(ask_search_parameters_valid_input)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"query", "1", "1", "10"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askSearchParameters();

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK_EQUAL(result->query, "query");
  BOOST_CHECK_EQUAL(result->limit, 10);
}

BOOST_AUTO_TEST_CASE(ask_search_parameters_back_on_query)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"/back"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askSearchParameters();

  BOOST_CHECK(!result.has_value());
}

BOOST_AUTO_TEST_CASE(ask_category_returns_selected)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"2"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askCategory();

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(*result == quiz::core::Category::GENERAL_KNOWLEDGE);
}

BOOST_AUTO_TEST_CASE(ask_difficulty_returns_selected)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"2"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askDifficulty();

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(*result == quiz::core::Difficulty::EASY);
}

BOOST_AUTO_TEST_CASE(ask_question_type_returns_selected)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  terminal->setInput({"2"});

  quiz::tui::Forms forms(terminal);
  const auto result = forms.askQuestionType();

  BOOST_REQUIRE(result.has_value());
  BOOST_CHECK(*result == quiz::core::QuestionType::MULTIPLE);
}

BOOST_AUTO_TEST_SUITE_END()
