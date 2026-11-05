#include <boost/test/unit_test.hpp>
#include "tui/format.hpp"
#include <chrono>

BOOST_AUTO_TEST_SUITE(format_tests)

BOOST_AUTO_TEST_CASE(wrap_text_basic)
{
  const auto lines = quiz::tui::wrapText("hello world", 20);
  BOOST_CHECK_EQUAL(lines.size(), 1);
  BOOST_CHECK_EQUAL(lines[0], "hello world");
}

BOOST_AUTO_TEST_CASE(wrap_text_wraps_long_line)
{
  const auto lines = quiz::tui::wrapText("hello world this is a long line", 15);
  BOOST_CHECK_EQUAL(lines.size(), 3);
  BOOST_CHECK_EQUAL(lines[0], "hello world");
  BOOST_CHECK_EQUAL(lines[1], "this is a long");
  BOOST_CHECK_EQUAL(lines[2], "line");
}

BOOST_AUTO_TEST_CASE(wrap_text_preserves_newlines)
{
  const auto lines = quiz::tui::wrapText("line1\nline2\nline3", 80);
  BOOST_CHECK_EQUAL(lines.size(), 3);
  BOOST_CHECK_EQUAL(lines[0], "line1");
  BOOST_CHECK_EQUAL(lines[1], "line2");
  BOOST_CHECK_EQUAL(lines[2], "line3");
}

BOOST_AUTO_TEST_CASE(wrap_text_empty_string)
{
  const auto lines = quiz::tui::wrapText("", 80);
  BOOST_CHECK(lines.empty());
}

BOOST_AUTO_TEST_CASE(wrap_text_word_longer_than_width)
{
  const auto lines = quiz::tui::wrapText("superlongword short", 10);
  BOOST_CHECK_EQUAL(lines.size(), 2);
  BOOST_CHECK_EQUAL(lines[0], "superlongword");
  BOOST_CHECK_EQUAL(lines[1], "short");
}

BOOST_AUTO_TEST_CASE(pad_right_basic)
{
  BOOST_CHECK_EQUAL(quiz::tui::padRight("hello", 10), "hello     ");
}

BOOST_AUTO_TEST_CASE(pad_right_exact_width)
{
  BOOST_CHECK_EQUAL(quiz::tui::padRight("hello", 5), "hello");
}

BOOST_AUTO_TEST_CASE(pad_right_no_padding_needed)
{
  BOOST_CHECK_EQUAL(quiz::tui::padRight("hello world", 5), "hello world");
}

BOOST_AUTO_TEST_CASE(pad_right_empty_string)
{
  BOOST_CHECK_EQUAL(quiz::tui::padRight("", 5), "     ");
}

BOOST_AUTO_TEST_CASE(format_accuracy_zero)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatAccuracy(0.0), "0.0%");
}

BOOST_AUTO_TEST_CASE(format_accuracy_half)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatAccuracy(0.5), "50.0%");
}

BOOST_AUTO_TEST_CASE(format_accuracy_full)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatAccuracy(1.0), "100.0%");
}

BOOST_AUTO_TEST_CASE(format_accuracy_fractional)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatAccuracy(1.0 / 3.0), "33.3%");
}

BOOST_AUTO_TEST_CASE(format_time_point_epoch)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatTimePoint(std::chrono::system_clock::time_point{}), "-");
}

BOOST_AUTO_TEST_CASE(format_time_point_valid)
{
  const auto time = std::chrono::system_clock::time_point(std::chrono::microseconds(1700000000123456));
  const std::string result = quiz::tui::formatTimePoint(time);
  BOOST_CHECK(!result.empty());
  BOOST_CHECK(result != "-");
}

BOOST_AUTO_TEST_CASE(format_duration_zero)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatDuration(std::chrono::seconds(0)), "0s");
}

BOOST_AUTO_TEST_CASE(format_duration_seconds_only)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatDuration(std::chrono::seconds(45)), "45s");
}

BOOST_AUTO_TEST_CASE(format_duration_minutes_and_seconds)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatDuration(std::chrono::seconds(90)), "1m 30s");
}

BOOST_AUTO_TEST_CASE(format_duration_hours_minutes_seconds)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatDuration(std::chrono::seconds(3661)), "1h 01m 01s");
}

BOOST_AUTO_TEST_CASE(format_duration_negative_clamped_to_zero)
{
  BOOST_CHECK_EQUAL(quiz::tui::formatDuration(std::chrono::seconds(-10)), "0s");
}

BOOST_AUTO_TEST_SUITE_END()
