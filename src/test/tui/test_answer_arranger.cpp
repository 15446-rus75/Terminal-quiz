#include <boost/test/unit_test.hpp>
#include "tui/answer_arranger.hpp"
#include "core/question.hpp"
#include <set>

BOOST_AUTO_TEST_SUITE(answer_arranger_tests)

BOOST_AUTO_TEST_CASE(random_arranger_includes_all_answers)
{
  quiz::core::Question question;
  question.type = quiz::core::QuestionType::MULTIPLE;
  question.correct_answer = "correct";
  question.incorrect_answers = {"wrong1", "wrong2", "wrong3"};

  quiz::tui::RandomAnswerArranger arranger(42);
  const auto answers = arranger.arrange(question);

  BOOST_CHECK_EQUAL(answers.size(), 4);
  std::set< std::string > answer_set(answers.begin(), answers.end());
  BOOST_CHECK(answer_set.count("correct") > 0);
  BOOST_CHECK(answer_set.count("wrong1") > 0);
  BOOST_CHECK(answer_set.count("wrong2") > 0);
  BOOST_CHECK(answer_set.count("wrong3") > 0);
}

BOOST_AUTO_TEST_CASE(random_arranger_deterministic_with_seed)
{
  quiz::core::Question question;
  question.type = quiz::core::QuestionType::MULTIPLE;
  question.correct_answer = "correct";
  question.incorrect_answers = {"wrong1", "wrong2", "wrong3"};

  quiz::tui::RandomAnswerArranger arranger1(42);
  quiz::tui::RandomAnswerArranger arranger2(42);

  const auto answers1 = arranger1.arrange(question);
  const auto answers2 = arranger2.arrange(question);

  BOOST_CHECK_EQUAL_COLLECTIONS(answers1.begin(), answers1.end(), answers2.begin(), answers2.end());
}

BOOST_AUTO_TEST_CASE(random_arranger_different_seeds_different_order)
{
  quiz::core::Question question;
  question.type = quiz::core::QuestionType::MULTIPLE;
  question.correct_answer = "correct";
  question.incorrect_answers = {"wrong1", "wrong2", "wrong3", "wrong4"};

  quiz::tui::RandomAnswerArranger arranger1(1);
  quiz::tui::RandomAnswerArranger arranger2(2);

  const auto answers1 = arranger1.arrange(question);
  const auto answers2 = arranger2.arrange(question);

  bool different = false;
  for (size_t i = 0; i < answers1.size(); ++i)
  {
    if (answers1[i] != answers2[i])
    {
      different = true;
      break;
    }
  }
  BOOST_CHECK(different);
}

BOOST_AUTO_TEST_CASE(boolean_question_always_sorted)
{
  quiz::core::Question question;
  question.type = quiz::core::QuestionType::BOOLEAN;
  question.correct_answer = "False";
  question.incorrect_answers = {"True"};

  quiz::tui::RandomAnswerArranger arranger(42);
  const auto answers = arranger.arrange(question);

  BOOST_CHECK_EQUAL(answers.size(), 2);
  BOOST_CHECK_EQUAL(answers[0], "True");
  BOOST_CHECK_EQUAL(answers[1], "False");
}

BOOST_AUTO_TEST_CASE(boolean_question_sorted_descending)
{
  quiz::core::Question question;
  question.type = quiz::core::QuestionType::BOOLEAN;
  question.correct_answer = "True";
  question.incorrect_answers = {"False"};

  quiz::tui::RandomAnswerArranger arranger(123);
  const auto answers = arranger.arrange(question);

  BOOST_CHECK_EQUAL(answers.size(), 2);
  BOOST_CHECK_EQUAL(answers[0], "True");
  BOOST_CHECK_EQUAL(answers[1], "False");
}

BOOST_AUTO_TEST_SUITE_END()
