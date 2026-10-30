#include <algorithm>
#include <chrono>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "db/schema_initializer.hpp"
#include "test_support.hpp"
#include <boost/test/unit_test.hpp>

using namespace quiz;
using namespace dbtest;
BOOST_FIXTURE_TEST_SUITE(repositories, Fixture)

BOOST_AUTO_TEST_CASE(schema_is_idempotent_and_has_trigram_index)
{
  questions.save(question());
  db::SchemaInitializer(db).apply();
  db::SchemaInitializer(db).apply();
  BOOST_CHECK_EQUAL(questions.count(), 1);
  const auto TABLES = db->getConnection()->execute(
    "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema='public' AND table_name IN "
    "('questions','question_incorrect_answers','favorites','custom_tests','custom_test_questions','attempts','attempt_"
    "answers')",
    {});
  BOOST_CHECK_EQUAL(TABLES.at(0).at(0), "7");
  BOOST_CHECK_EQUAL(
    db->getConnection()->execute("SELECT COUNT(*) FROM pg_extension WHERE extname='pg_trgm'", {}).at(0).at(0), "1");
  const auto INDEX =
    db->getConnection()->execute("SELECT indexdef FROM pg_indexes WHERE indexname='questions_text_trgm_idx'", {});
  BOOST_REQUIRE_EQUAL(INDEX.size(), 1);
  BOOST_CHECK(INDEX[0][0].find("gin_trgm_ops") != std::string::npos);
}
BOOST_AUTO_TEST_CASE(empty_repositories_return_empty_results)
{
  BOOST_CHECK_EQUAL(questions.count(), 0);
  BOOST_CHECK(!questions.findById(MISSING));
  BOOST_CHECK(!attempts.findById(MISSING));
  BOOST_CHECK(!tests.findById(MISSING));
  BOOST_CHECK(favorites.findByUserId(USER).empty());
  BOOST_CHECK(attempts.findByUserId(USER, 10).empty());
  BOOST_CHECK(tests.findByUserId(USER).empty());
}
BOOST_AUTO_TEST_CASE(question_round_trip_preserves_unicode_and_answer_order)
{
  auto value = question("Русский вопрос O'Reilly \\ 100%_");
  value.correct_answer = "Ответ ' \\";
  value.incorrect_answers = {"", "два", "два", "one"};
  const size_t ID = questions.save(value);
  const auto LOADED = questions.findById(ID);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK_EQUAL(LOADED->id, ID);
  checkQuestion(*LOADED, value);
}
BOOST_AUTO_TEST_CASE(update_replaces_all_question_answers)
{
  auto value = question();
  value.id = questions.save(value);
  value.text = "updated";
  value.incorrect_answers = {"replacement"};
  BOOST_CHECK_EQUAL(questions.save(value), value.id);
  auto loaded = questions.findById(value.id);
  BOOST_REQUIRE(loaded.has_value());
  checkQuestion(*loaded, value);
  value.incorrect_answers.clear();
  questions.save(value);
  loaded = questions.findById(value.id);
  BOOST_REQUIRE(loaded.has_value());
  BOOST_CHECK(loaded->incorrect_answers.empty());
  BOOST_CHECK_EQUAL(questions.count(), 1);
}
BOOST_AUTO_TEST_CASE(nonexistent_question_update_is_not_insert)
{
  auto value = question();
  value.id = MISSING;
  BOOST_CHECK_THROW(questions.save(value), std::out_of_range);
  BOOST_CHECK_EQUAL(questions.count(), 0);
}
BOOST_AUTO_TEST_CASE(filters_limits_and_order_are_deterministic)
{
  auto first = question("first");
  auto second = question("second");
  auto third = question("third");
  second.category = core::Category::ENTERTAINMENT_MUSIC;
  second.difficulty = core::Difficulty::HARD;
  third.difficulty = core::Difficulty::MEDIUM;
  const auto IDS = questions.saveBatch({first, second, third});
  BOOST_REQUIRE_EQUAL(IDS.size(), 3);
  BOOST_CHECK_EQUAL(questions.findByFilter(core::Category::ANY, core::Difficulty::ANY, 10).size(), 3);
  BOOST_CHECK_EQUAL(questions.findByFilter(core::Category::SCIENCE_COMPUTERS, core::Difficulty::ANY, 10).size(), 2);
  BOOST_CHECK_EQUAL(questions.findByFilter(core::Category::ANY, core::Difficulty::HARD, 10).size(), 1);
  BOOST_CHECK(questions.findByFilter(core::Category::ANY, core::Difficulty::ANY, 0).empty());
  const auto ONE = questions.findByFilter(core::Category::ANY, core::Difficulty::ANY, 1);
  BOOST_REQUIRE_EQUAL(ONE.size(), 1);
  BOOST_CHECK_EQUAL(ONE.front().id, IDS.front());
}
BOOST_AUTO_TEST_CASE(search_uses_case_insensitive_literal_substrings)
{
  questions.saveBatch({question("Alice's 100%_code\\ UTF-8 Ж"), question("100Xycode"), question("other")});
  for (const auto &query : {"ALICE'S", "%", "_", "\\", "Ж"})
  {
    BOOST_TEST_CONTEXT(query)
    {
      BOOST_CHECK_EQUAL(questions.searchByText(query, core::Category::ANY, core::Difficulty::ANY, 10).size(), 1);
    }
  }
  BOOST_CHECK(
    questions.searchByText("' OR TRUE; DROP TABLE questions;--", core::Category::ANY, core::Difficulty::ANY, 10)
      .empty());
  BOOST_CHECK_EQUAL(questions.searchByText("", core::Category::ANY, core::Difficulty::ANY, 10).size(), 3);
  BOOST_CHECK(questions.searchByText("", core::Category::ANY, core::Difficulty::ANY, 0).empty());
  BOOST_CHECK(questions.searchByText("Alice", core::Category::ANY, core::Difficulty::HARD, 10).empty());
  BOOST_CHECK_EQUAL(questions.count(), 3);
}
BOOST_AUTO_TEST_CASE(empty_batch_is_noop)
{
  BOOST_CHECK(questions.saveBatch({}).empty());
  BOOST_CHECK_EQUAL(questions.count(), 0);
}
BOOST_AUTO_TEST_CASE(large_batch_crosses_chunks_and_preserves_id_mapping)
{
  std::vector< core::Question > values;
  for (size_t index = 0; index < 257; ++index)
  {
    values.push_back(question("batch " + std::to_string(index)));
  }
  auto counted = std::make_shared< CountedDatabase >(db);
  db::PostgresQuestionRepository repository(counted);
  const auto IDS = repository.saveBatch(values);
  BOOST_REQUIRE_EQUAL(IDS.size(), values.size());
  BOOST_CHECK_EQUAL(std::set< size_t >(IDS.begin(), IDS.end()).size(), IDS.size());
  // Regression guard against one INSERT/answer query per input record, allowing
  // different batch sizes and future implementations with fewer SQL calls.
  BOOST_CHECK_LT(counted->calls->queries, 30);
  BOOST_CHECK_EQUAL(counted->calls->begins, 1);
  BOOST_CHECK_EQUAL(counted->calls->commits, 1);
  const auto LOADED = questions.findByFilter(core::Category::ANY, core::Difficulty::ANY, values.size());
  BOOST_REQUIRE_EQUAL(LOADED.size(), values.size());
  for (size_t index = 0; index < values.size(); ++index)
  {
    BOOST_TEST_CONTEXT("index=" << index)
    {
      BOOST_CHECK_EQUAL(LOADED[index].id, IDS[index]);
      checkQuestion(LOADED[index], values[index]);
    }
  }
}
BOOST_AUTO_TEST_CASE(mixed_batch_updates_existing_ids_in_input_order)
{
  auto existing = question("old");
  existing.id = questions.save(existing);
  auto later = existing;
  later.text = "last update";
  existing.text = "first update";
  const auto IDS = questions.saveBatch({question("new1"), existing, question("new2"), later});
  BOOST_REQUIRE_EQUAL(IDS.size(), 4);
  BOOST_CHECK_EQUAL(IDS[1], existing.id);
  BOOST_CHECK_EQUAL(IDS[3], existing.id);
  const auto LOADED = questions.findById(existing.id);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK_EQUAL(LOADED->text, "last update");
  BOOST_CHECK_EQUAL(questions.count(), 3);
}
BOOST_AUTO_TEST_CASE(late_batch_failure_rolls_back_earlier_chunks_and_updates)
{
  auto original = question("original");
  original.id = questions.save(original);
  auto changed = original;
  changed.text = "must roll back";
  std::vector< core::Question > values(257, question());
  values.insert(values.begin(), changed);
  values.back().incorrect_answers = {std::string("x\0y", 3)};
  BOOST_CHECK_THROW(questions.saveBatch(values), std::exception);
  BOOST_CHECK_EQUAL(questions.count(), 1);
  const auto LOADED = questions.findById(original.id);
  BOOST_REQUIRE(LOADED.has_value());
  checkQuestion(*LOADED, original);
}
BOOST_AUTO_TEST_CASE(child_insert_failure_rolls_back_single_question)
{
  auto value = question();
  value.incorrect_answers = {std::string("x\0y", 3)};
  BOOST_CHECK_THROW(questions.save(value), std::invalid_argument);
  BOOST_CHECK_EQUAL(questions.count(), 0);
}
BOOST_AUTO_TEST_CASE(invalid_enums_and_oversized_ids_are_rejected)
{
  auto value = question();
  value.category = core::Category::ANY;
  BOOST_CHECK_THROW(questions.save(value), std::invalid_argument);
  BOOST_CHECK_THROW(questions.findByFilter(static_cast< core::Category >(999), core::Difficulty::ANY, 10),
                    std::exception);
  if (std::numeric_limits< size_t >::max() > static_cast< unsigned long long >(std::numeric_limits< long long >::max()))
  {
    BOOST_CHECK_THROW(questions.findById(std::numeric_limits< size_t >::max()), std::out_of_range);
  }
}
BOOST_AUTO_TEST_CASE(question_reads_batch_children_without_explicit_transaction)
{
  questions.saveBatch(std::vector< core::Question >(20, question()));
  auto counted = std::make_shared< CountedDatabase >(db);
  db::PostgresQuestionRepository repository(counted);
  const auto VALUES = repository.findByFilter(core::Category::ANY, core::Difficulty::ANY, 100);
  BOOST_REQUIRE_EQUAL(VALUES.size(), 20);
  BOOST_CHECK_LE(counted->calls->queries, 2);
  BOOST_CHECK_EQUAL(counted->calls->begins, 0);
  BOOST_CHECK_EQUAL(counted->calls->commits, 0);
}
BOOST_AUTO_TEST_CASE(favorites_are_idempotent_and_isolated_by_user)
{
  const auto IDS = questions.saveBatch({question("1"), question("2")});
  favorites.add(USER, IDS[1]);
  favorites.add(USER, IDS[0]);
  favorites.add(USER, IDS[0]);
  favorites.add(OTHER_USER, IDS[0]);
  BOOST_CHECK(favorites.exists(USER, IDS[0]));
  BOOST_CHECK(!favorites.exists(OTHER_USER, IDS[1]));
  const auto VALUES = favorites.findByUserId(USER);
  BOOST_REQUIRE_EQUAL(VALUES.size(), 2);
  BOOST_CHECK_EQUAL(VALUES[0].id, IDS[0]);
  checkQuestion(VALUES[0], question("1"));
  favorites.remove(USER, IDS[0]);
  favorites.remove(USER, IDS[0]);
  BOOST_CHECK(!favorites.exists(USER, IDS[0]));
  BOOST_CHECK(favorites.exists(OTHER_USER, IDS[0]));
}
BOOST_AUTO_TEST_CASE(favorites_reject_missing_question_and_cascade_on_delete)
{
  BOOST_CHECK_THROW(favorites.add(USER, MISSING), std::runtime_error);
  const size_t ID = questions.save(question());
  favorites.add(USER, ID);
  deleteQuestion(ID);
  BOOST_CHECK(!favorites.exists(USER, ID));
  BOOST_CHECK(favorites.findByUserId(USER).empty());
  deleteQuestion(ID);
}
BOOST_AUTO_TEST_CASE(custom_tests_preserve_repeated_questions_and_timestamp)
{
  const auto IDS = questions.saveBatch({question("1"), question("2")});
  const auto VALUE = customTest({IDS[1], IDS[0], IDS[1]});
  const size_t ID = tests.save(VALUE);
  const auto LOADED = tests.findById(ID);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK(LOADED->question_ids == VALUE.question_ids);
  BOOST_CHECK_EQUAL(LOADED->name, VALUE.name);
  BOOST_CHECK(LOADED->created_at == VALUE.created_at);
  BOOST_CHECK_EQUAL(LOADED->user_id, USER);
}
BOOST_AUTO_TEST_CASE(custom_test_update_replaces_links_and_remove_keeps_questions)
{
  const size_t QID = questions.save(question());
  auto value = customTest({QID, QID});
  value.id = tests.save(value);
  value.question_ids.clear();
  value.name = "empty";
  BOOST_CHECK_EQUAL(tests.save(value), value.id);
  const auto LOADED = tests.findById(value.id);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK(LOADED->question_ids.empty());
  tests.remove(value.id);
  tests.remove(value.id);
  BOOST_CHECK(!tests.findById(value.id));
  BOOST_CHECK_EQUAL(questions.count(), 1);
}
BOOST_AUTO_TEST_CASE(custom_test_ordering_and_user_filter)
{
  auto first = customTest();
  const size_t FIRST = tests.save(first);
  const size_t SECOND = tests.save(first);
  first.user_id = OTHER_USER;
  tests.save(first);
  const auto VALUES = tests.findByUserId(USER);
  BOOST_REQUIRE_EQUAL(VALUES.size(), 2);
  BOOST_CHECK_EQUAL(VALUES[0].id, SECOND);
  BOOST_CHECK_EQUAL(VALUES[1].id, FIRST);
}
BOOST_AUTO_TEST_CASE(custom_test_bad_foreign_key_rolls_back_update)
{
  const size_t QID = questions.save(question());
  auto value = customTest({QID});
  value.id = tests.save(value);
  value.question_ids = {QID, MISSING};
  value.name = "changed";
  BOOST_CHECK_THROW(tests.save(value), std::runtime_error);
  const auto LOADED = tests.findById(value.id);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK_EQUAL(LOADED->name, customTest().name);
  BOOST_CHECK(LOADED->question_ids == std::vector< size_t >{QID});
  value.id = MISSING;
  BOOST_CHECK_THROW(tests.save(value), std::out_of_range);
}
BOOST_AUTO_TEST_CASE(question_delete_is_restricted_by_custom_test)
{
  const size_t QID = questions.save(question());
  const size_t TID = tests.save(customTest({QID}));
  BOOST_CHECK_THROW(deleteQuestion(QID), std::runtime_error);
  BOOST_CHECK(questions.findById(QID).has_value());
  tests.remove(TID);
  BOOST_CHECK_NO_THROW(deleteQuestion(QID));
}
BOOST_AUTO_TEST_CASE(custom_test_reads_batch_links)
{
  const size_t QID = questions.save(question());
  for (size_t index = 0; index < 12; ++index)
  {
    tests.save(customTest({QID, QID}));
  }
  auto counted = std::make_shared< CountedDatabase >(db);
  db::PostgresCustomTestRepository repository(counted);
  const auto VALUES = repository.findByUserId(USER);
  BOOST_REQUIRE_EQUAL(VALUES.size(), 12);
  for (const auto &value : VALUES)
  {
    BOOST_CHECK_EQUAL(value.question_ids.size(), 2);
  }
  BOOST_CHECK_LE(counted->calls->queries, 2);
  BOOST_CHECK_EQUAL(counted->calls->begins, 0);
}
BOOST_AUTO_TEST_CASE(attempt_round_trip_preserves_microseconds_and_answer_positions)
{
  const size_t QID = questions.save(question());
  auto value = attempt();
  value.answers = {{0, 0, QID, "wrong", false, 1}, {0, 0, QID, "Верно ' \\ ", true, 0}};
  const size_t ID = attempts.save(value);
  const auto LOADED = attempts.findById(ID);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK(LOADED->started_at == value.started_at);
  BOOST_CHECK(LOADED->finished_at == value.finished_at);
  BOOST_CHECK_EQUAL(LOADED->score, 1);
  BOOST_REQUIRE_EQUAL(LOADED->answers.size(), 2);
  BOOST_CHECK_EQUAL(LOADED->answers[0].position, 0);
  BOOST_CHECK(LOADED->answers[0].is_correct);
  BOOST_CHECK_EQUAL(LOADED->answers[0].attempt_id, ID);
  BOOST_CHECK_GT(LOADED->answers[0].id, 0);
  BOOST_CHECK_EQUAL(LOADED->answers[0].user_answer, value.answers[1].user_answer);
}
BOOST_AUTO_TEST_CASE(unfinished_attempt_can_be_finished)
{
  auto value = attempt(USER, 0, 2, false);
  const size_t ID = attempts.save(value);
  auto loaded = attempts.findById(ID);
  BOOST_REQUIRE(loaded.has_value());
  BOOST_CHECK(loaded->finished_at == value.finished_at);
  const auto FINISH = value.started_at + std::chrono::microseconds(1234567);
  attempts.finish(ID, 2, FINISH);
  loaded = attempts.findById(ID);
  BOOST_REQUIRE(loaded.has_value());
  BOOST_CHECK_EQUAL(loaded->score, 2);
  BOOST_CHECK(loaded->finished_at == FINISH);
}
BOOST_AUTO_TEST_CASE(invalid_finish_does_not_change_attempt)
{
  const auto VALUE = attempt(USER, 0, 2, false);
  const size_t ID = attempts.save(VALUE);
  BOOST_CHECK_THROW(attempts.finish(ID, 1, {}), std::invalid_argument);
  BOOST_CHECK_THROW(attempts.finish(ID, 3, VALUE.started_at), std::runtime_error);
  BOOST_CHECK_THROW(attempts.finish(ID, 1, VALUE.started_at - std::chrono::seconds(1)), std::runtime_error);
  BOOST_CHECK_THROW(attempts.finish(MISSING, 1, VALUE.started_at), std::out_of_range);
  const auto LOADED = attempts.findById(ID);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK_EQUAL(LOADED->score, 0);
}
BOOST_AUTO_TEST_CASE(attempt_update_preserves_id_and_replaces_answers)
{
  const size_t QID = questions.save(question());
  auto value = attempt();
  value.id = attempts.save(value);
  value.score = 2;
  value.answers = {{0, value.id, QID, "yes", true, 0}};
  BOOST_CHECK_EQUAL(attempts.save(value), value.id);
  const auto LOADED = attempts.findById(value.id);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_CHECK_EQUAL(LOADED->score, 2);
  BOOST_CHECK_EQUAL(LOADED->answers.size(), 1);
  value.id = MISSING;
  value.answers.clear();
  BOOST_CHECK_THROW(attempts.save(value), std::out_of_range);
}
BOOST_AUTO_TEST_CASE(save_answers_replaces_and_clears_existing_answers)
{
  const size_t QID = questions.save(question());
  const size_t ID = attempts.save(attempt());
  attempts.saveAnswers(ID, {{0, ID, QID, "first", true, 0}});
  attempts.saveAnswers(ID, {{0, 0, QID, "replacement", false, 3}});
  auto loaded = attempts.findById(ID);
  BOOST_REQUIRE(loaded.has_value());
  BOOST_REQUIRE_EQUAL(loaded->answers.size(), 1);
  BOOST_CHECK_EQUAL(loaded->answers[0].user_answer, "replacement");
  BOOST_CHECK_EQUAL(loaded->answers[0].position, 3);
  attempts.saveAnswers(ID, {});
  loaded = attempts.findById(ID);
  BOOST_REQUIRE(loaded.has_value());
  BOOST_CHECK(loaded->answers.empty());
  BOOST_CHECK_THROW(attempts.saveAnswers(MISSING, {}), std::out_of_range);
}
BOOST_AUTO_TEST_CASE(bad_answer_replacement_rolls_back_delete_and_partial_insert)
{
  const size_t QID = questions.save(question());
  const size_t ID = attempts.save(attempt());
  attempts.saveAnswers(ID, {{0, ID, QID, "original", true, 0}});
  BOOST_CHECK_THROW(attempts.saveAnswers(ID, {{0, 0, QID, "ok", true, 0}, {0, 0, MISSING, "bad", false, 1}}),
                    std::runtime_error);
  BOOST_CHECK_THROW(attempts.saveAnswers(ID, {{0, ID + 1, QID, "wrong owner", false, 0}}), std::invalid_argument);
  BOOST_CHECK_THROW(attempts.saveAnswers(ID, {{0, 0, QID, "a", true, 0}, {0, 0, QID, "b", false, 0}}),
                    std::runtime_error);
  const auto LOADED = attempts.findById(ID);
  BOOST_REQUIRE(LOADED.has_value());
  BOOST_REQUIRE_EQUAL(LOADED->answers.size(), 1);
  BOOST_CHECK_EQUAL(LOADED->answers[0].user_answer, "original");
}
BOOST_AUTO_TEST_CASE(failed_attempt_insert_is_atomic)
{
  auto value = attempt();
  value.answers = {{0, 0, MISSING, "bad", false, 0}};
  BOOST_CHECK_THROW(attempts.save(value), std::runtime_error);
  BOOST_CHECK(attempts.findByUserId(USER, 10).empty());
  value = attempt(USER, 3, 2);
  BOOST_CHECK_THROW(attempts.save(value), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(attempt_history_is_ordered_filtered_and_limited)
{
  auto value = attempt();
  const size_t FIRST = attempts.save(value);
  const size_t SECOND = attempts.save(value);
  value.started_at += std::chrono::seconds(1);
  const size_t THIRD = attempts.save(value);
  value.user_id = OTHER_USER;
  attempts.save(value);
  const auto VALUES = attempts.findByUserId(USER, 2);
  BOOST_REQUIRE_EQUAL(VALUES.size(), 2);
  BOOST_CHECK_EQUAL(VALUES[0].id, THIRD);
  BOOST_CHECK_EQUAL(VALUES[1].id, SECOND);
  BOOST_CHECK_NE(FIRST, SECOND);
  BOOST_CHECK(attempts.findByUserId(USER, 0).empty());
}
BOOST_AUTO_TEST_CASE(attempt_reads_batch_answers)
{
  const size_t QID = questions.save(question());
  auto value = attempt();
  value.answers = {{0, 0, QID, "a", true, 0}};
  for (size_t index = 0; index < 12; ++index)
  {
    attempts.save(value);
  }
  auto counted = std::make_shared< CountedDatabase >(db);
  db::PostgresAttemptRepository repository(counted);
  const auto VALUES = repository.findByUserId(USER, 20);
  BOOST_REQUIRE_EQUAL(VALUES.size(), 12);
  for (const auto &item : VALUES)
  {
    BOOST_CHECK_EQUAL(item.answers.size(), 1);
  }
  BOOST_CHECK_LE(counted->calls->queries, 2);
  BOOST_CHECK_EQUAL(counted->calls->begins, 0);
  BOOST_CHECK_EQUAL(counted->calls->commits, 0);
}
BOOST_AUTO_TEST_CASE(question_delete_is_restricted_by_attempt_history)
{
  const size_t QID = questions.save(question());
  auto value = attempt();
  value.answers = {{0, 0, QID, "a", true, 0}};
  attempts.save(value);
  BOOST_CHECK_THROW(deleteQuestion(QID), std::runtime_error);
  BOOST_CHECK(questions.findById(QID).has_value());
}
BOOST_AUTO_TEST_CASE(empty_statistics_are_zero)
{
  const auto VALUE = statistics.getOverall(USER);
  BOOST_CHECK_EQUAL(VALUE.total_attempts, 0);
  BOOST_CHECK_EQUAL(VALUE.total_questions, 0);
  BOOST_CHECK_EQUAL(VALUE.total_correct, 0);
  BOOST_CHECK_SMALL(VALUE.overall_accuracy, 1e-10);
  BOOST_CHECK(statistics.getByCategory(USER).empty());
  BOOST_CHECK(statistics.getByDifficulty(USER).empty());
}
BOOST_AUTO_TEST_CASE(overall_statistics_are_weighted_and_exclude_unfinished_and_other_users)
{
  attempts.save(attempt(USER, 1, 1));
  attempts.save(attempt(USER, 1, 3));
  attempts.save(attempt(USER, 10, 10, false));
  attempts.save(attempt(OTHER_USER, 7, 7));
  const auto VALUE = statistics.getOverall(USER);
  BOOST_CHECK_EQUAL(VALUE.total_attempts, 2);
  BOOST_CHECK_EQUAL(VALUE.total_questions, 4);
  BOOST_CHECK_EQUAL(VALUE.total_correct, 2);
  BOOST_CHECK_CLOSE(VALUE.overall_accuracy, 50.0, 1e-8);
}
BOOST_AUTO_TEST_CASE(grouped_statistics_count_attempts_once_per_group)
{
  auto music = question("music");
  music.category = core::Category::ENTERTAINMENT_MUSIC;
  music.difficulty = core::Difficulty::HARD;
  const auto IDS = questions.saveBatch({question("one"), question("two"), music});
  auto value = attempt(USER, 2, 3);
  value.answers = {{0, 0, IDS[0], "a", true, 0}, {0, 0, IDS[1], "b", false, 1}, {0, 0, IDS[2], "c", true, 2}};
  attempts.save(value);
  value.finished_at = {};
  attempts.save(value);
  value = attempt(OTHER_USER);
  attempts.save(value);
  const auto CATEGORIES = statistics.getByCategory(USER);
  BOOST_REQUIRE_EQUAL(CATEGORIES.size(), 2);
  BOOST_CHECK(CATEGORIES[0].category == core::Category::ENTERTAINMENT_MUSIC);
  BOOST_CHECK_EQUAL(CATEGORIES[1].total_attempts, 1);
  BOOST_CHECK_EQUAL(CATEGORIES[1].total_questions, 2);
  BOOST_CHECK_EQUAL(CATEGORIES[1].total_correct, 1);
  BOOST_CHECK_CLOSE(CATEGORIES[1].accuracy, 50.0, 1e-8);
  const auto DIFFICULTIES = statistics.getByDifficulty(USER);
  BOOST_REQUIRE_EQUAL(DIFFICULTIES.size(), 2);
  BOOST_CHECK(DIFFICULTIES[0].difficulty == core::Difficulty::EASY);
  BOOST_CHECK_CLOSE(DIFFICULTIES[1].accuracy, 100.0, 1e-8);
}
BOOST_AUTO_TEST_CASE(zero_question_finished_attempt_has_zero_accuracy)
{
  attempts.save(attempt(USER, 0, 0));
  const auto VALUE = statistics.getOverall(USER);
  BOOST_CHECK_EQUAL(VALUE.total_attempts, 1);
  BOOST_CHECK_SMALL(VALUE.overall_accuracy, 1e-10);
}
BOOST_AUTO_TEST_SUITE_END()
