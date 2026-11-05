#include "core/attempt.hpp"
#include "core/question.hpp"
#include "test_support.hpp"
#include "tui/renderer.hpp"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(renderer_tests)

BOOST_AUTO_TEST_CASE(renderer_throws_on_null_terminal)
{
  BOOST_CHECK_THROW(quiz::tui::Renderer(nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(show_title_outputs_title_and_rule)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  renderer.showTitle("Test Title");

  const std::string output = terminal->getOutput();
  BOOST_CHECK(output.find("Test Title") != std::string::npos);
  BOOST_CHECK(output.find("---") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_menu_lists_items)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  renderer.showMenu({"Item 1", "Item 2", "Item 3"}, "Exit");

  const std::string output = terminal->getOutput();
  BOOST_CHECK(output.find("1. Item 1") != std::string::npos);
  BOOST_CHECK(output.find("2. Item 2") != std::string::npos);
  BOOST_CHECK(output.find("3. Item 3") != std::string::npos);
  BOOST_CHECK(output.find("0. Exit") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_info_outputs_message)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  renderer.showInfo("Test message");

  BOOST_CHECK(terminal->getOutput().find("Test message") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_error_outputs_message)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  renderer.showError("Error message");

  BOOST_CHECK(terminal->getOutput().find("Error message") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_success_outputs_message)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  renderer.showSuccess("Success message");

  BOOST_CHECK(terminal->getOutput().find("Success message") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_question_outputs_question_and_options)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  quiz::core::Question question;
  question.text = "What is 2+2?";
  question.category = quiz::core::Category::GENERAL_KNOWLEDGE;
  question.difficulty = quiz::core::Difficulty::EASY;
  question.type = quiz::core::QuestionType::MULTIPLE;

  std::vector< std::string > options = {"3", "4", "5", "6"};

  renderer.showQuestion({.number = 1, .total = 10}, question, options);

  const std::string output = terminal->getOutput();
  BOOST_CHECK(output.find("Question 1 of 10") != std::string::npos);
  BOOST_CHECK(output.find("What is 2+2?") != std::string::npos);
  BOOST_CHECK(output.find("3") != std::string::npos);
  BOOST_CHECK(output.find("4") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_answer_feedback_correct)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  quiz::core::Question question;
  question.correct_answer = "Paris";

  renderer.showAnswerFeedback(true, question);

  BOOST_CHECK(terminal->getOutput().find("Correct!") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_answer_feedback_incorrect)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  quiz::core::Question question;
  question.correct_answer = "Paris";

  renderer.showAnswerFeedback(false, question);

  const std::string output = terminal->getOutput();
  BOOST_CHECK(output.find("Wrong") != std::string::npos);
  BOOST_CHECK(output.find("Paris") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_quiz_result_outputs_score)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  quiz::service::QuizResult result{.attempt_id = 1, .score = 8, .total = 10, .accuracy = 0.8};

  renderer.showQuizResult(result);

  const std::string output = terminal->getOutput();
  BOOST_CHECK(output.find("8/10") != std::string::npos);
  BOOST_CHECK(output.find("80.0%") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_quiz_result_perfect_score)
{
  auto terminal = std::make_shared< test::FakeTerminal >();
  quiz::tui::Renderer renderer(terminal);

  quiz::service::QuizResult result{.attempt_id = 1, .score = 10, .total = 10, .accuracy = 1.0};

  renderer.showQuizResult(result);

  BOOST_CHECK(terminal->getOutput().find("Perfect score!") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
