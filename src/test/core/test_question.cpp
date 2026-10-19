#include <string>
#include <vector>

#include "core/question.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;

namespace
{
  core::Question makeQuestion(const std::string &correct, const std::vector< std::string > &incorrect)
  {
    core::Question question;
    question.id = 1;
    question.category = core::Category::GENERAL_KNOWLEDGE;
    question.difficulty = core::Difficulty::EASY;
    question.type = core::QuestionType::MULTIPLE;
    question.text = "Question?";
    question.correct_answer = correct;
    question.incorrect_answers = incorrect;
    return question;
  }
}

BOOST_AUTO_TEST_SUITE(question_all_answers)

BOOST_AUTO_TEST_CASE(correct_answer_goes_first)
{
  const core::Question QUESTION = makeQuestion("Paris", {"London", "Berlin", "Madrid"});
  const std::vector< std::string > EXPECTED = {"Paris", "London", "Berlin", "Madrid"};

  BOOST_TEST(QUESTION.allAnswers() == EXPECTED, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(boolean_question)
{
  const core::Question QUESTION = makeQuestion("True", {"False"});
  const std::vector< std::string > EXPECTED = {"True", "False"};

  BOOST_TEST(QUESTION.allAnswers() == EXPECTED, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(no_incorrect_answers)
{
  const core::Question QUESTION = makeQuestion("Only", {});

  BOOST_TEST(QUESTION.allAnswers().size() == 1U);
  BOOST_TEST(QUESTION.allAnswers().front() == "Only");
}

BOOST_AUTO_TEST_CASE(does_not_modify_question)
{
  const core::Question QUESTION = makeQuestion("A", {"B", "C"});

  QUESTION.allAnswers();

  BOOST_TEST(QUESTION.correct_answer == "A");
  BOOST_TEST(QUESTION.incorrect_answers.size() == 2U);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(question_is_correct_answer)

BOOST_AUTO_TEST_CASE(exact_match)
{
  const core::Question QUESTION = makeQuestion("Paris", {"London"});

  BOOST_TEST(QUESTION.isCorrectAnswer("Paris"));
}

BOOST_AUTO_TEST_CASE(case_insensitive)
{
  const core::Question QUESTION = makeQuestion("True", {"False"});

  BOOST_TEST(QUESTION.isCorrectAnswer("true"));
  BOOST_TEST(QUESTION.isCorrectAnswer("TRUE"));
  BOOST_TEST(QUESTION.isCorrectAnswer("tRuE"));
}

BOOST_AUTO_TEST_CASE(wrong_answer)
{
  const core::Question QUESTION = makeQuestion("True", {"False"});

  BOOST_TEST(!QUESTION.isCorrectAnswer("false"));
  BOOST_TEST(!QUESTION.isCorrectAnswer(""));
  BOOST_TEST(!QUESTION.isCorrectAnswer("tru"));
}

BOOST_AUTO_TEST_CASE(incorrect_answer_is_not_accepted)
{
  const core::Question QUESTION = makeQuestion("Paris", {"London", "Berlin"});

  BOOST_TEST(!QUESTION.isCorrectAnswer("London"));
  BOOST_TEST(!QUESTION.isCorrectAnswer("berlin"));
}

BOOST_AUTO_TEST_CASE(whitespace_is_not_trimmed)
{
  const core::Question QUESTION = makeQuestion("True", {"False"});

  BOOST_TEST(!QUESTION.isCorrectAnswer(" true"));
  BOOST_TEST(!QUESTION.isCorrectAnswer("true "));
}

BOOST_AUTO_TEST_SUITE_END()
