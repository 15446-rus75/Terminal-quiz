#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "util/logger.hpp"
#include <boost/test/unit_test.hpp>

namespace
{
  class CaptureLog
  {
  public:
    CaptureLog(): previous_buffer_(std::clog.rdbuf(output_.rdbuf()))
    {
      quiz::util::Logger::setLevel(quiz::util::LogLevel::INFO);
    }

    ~CaptureLog()
    {
      quiz::util::Logger::setLevel(quiz::util::LogLevel::INFO);
      std::clog.rdbuf(previous_buffer_);
    }

    std::string text() const
    {
      return output_.str();
    }

  private:
    std::ostringstream output_;
    std::streambuf *previous_buffer_;
  };
}

using quiz::util::Logger;
using quiz::util::LogLevel;

BOOST_FIXTURE_TEST_SUITE(logger, CaptureLog)

BOOST_AUTO_TEST_CASE(info_threshold_filters_debug)
{
  Logger::debug("hidden");
  Logger::info("information");
  Logger::warning("warning");
  Logger::error("error");
  BOOST_TEST(text() == "[INFO] information\n[WARNING] warning\n[ERROR] error\n");
}

BOOST_AUTO_TEST_CASE(debug_threshold_includes_all_levels)
{
  Logger::setLevel(LogLevel::DEBUG);
  Logger::debug("d");
  Logger::info("i");
  Logger::warning("w");
  Logger::error("e");
  BOOST_TEST(text() == "[DEBUG] d\n[INFO] i\n[WARNING] w\n[ERROR] e\n");
}

BOOST_AUTO_TEST_CASE(warning_and_error_thresholds)
{
  Logger::setLevel(LogLevel::WARNING);
  Logger::info("hidden");
  Logger::warning("visible");
  Logger::setLevel(LogLevel::ERROR);
  Logger::warning("hidden too");
  Logger::error("visible too");
  BOOST_TEST(text() == "[WARNING] visible\n[ERROR] visible too\n");
}

BOOST_AUTO_TEST_CASE(invalid_level_does_not_change_threshold)
{
  BOOST_CHECK_THROW(Logger::setLevel(static_cast< LogLevel >(999)), std::invalid_argument);
  Logger::debug("hidden");
  Logger::info("still info");
  BOOST_TEST(text() == "[INFO] still info\n");
}

BOOST_AUTO_TEST_CASE(empty_and_unicode_messages)
{
  Logger::info("");
  Logger::error("\xD0\x96");
  BOOST_TEST(text() == "[INFO] \n[ERROR] \xD0\x96\n");
}

BOOST_AUTO_TEST_CASE(concurrent_messages_do_not_interleave)
{
  constexpr size_t THREAD_COUNT = 4;
  constexpr size_t MESSAGE_COUNT = 100;
  std::vector< std::thread > threads;
  for (size_t i = 0; i < THREAD_COUNT; ++i)
  {
    threads.emplace_back(
      []()
      {
        for (size_t j = 0; j < MESSAGE_COUNT; ++j)
        {
          Logger::info("complete message");
        }
      });
  }
  for (auto &thread : threads)
  {
    thread.join();
  }
  std::istringstream lines(text());
  std::string line;
  size_t count = 0;
  while (std::getline(lines, line))
  {
    BOOST_TEST(line == "[INFO] complete message");
    ++count;
  }
  BOOST_TEST(count == THREAD_COUNT * MESSAGE_COUNT);
}

BOOST_AUTO_TEST_SUITE_END()
