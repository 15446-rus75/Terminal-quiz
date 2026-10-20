#include "logger.hpp"

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
  std::mutex &loggerMutex()
  {
    static std::mutex mutex;
    return mutex;
  }

  std::string_view levelName(quiz::util::LogLevel level)
  {
    switch (level)
    {
    case quiz::util::LogLevel::DEBUG:
      return "DEBUG";
    case quiz::util::LogLevel::INFO:
      return "INFO";
    case quiz::util::LogLevel::WARNING:
      return "WARNING";
    case quiz::util::LogLevel::ERROR:
      return "ERROR";
    }
    throw std::invalid_argument("Unknown log level");
  }
}

quiz::util::LogLevel quiz::util::Logger::current_level_ = quiz::util::LogLevel::INFO;

void quiz::util::Logger::debug(const std::string &message)
{
  log(LogLevel::DEBUG, message);
}

void quiz::util::Logger::info(const std::string &message)
{
  log(LogLevel::INFO, message);
}

void quiz::util::Logger::warning(const std::string &message)
{
  log(LogLevel::WARNING, message);
}

void quiz::util::Logger::error(const std::string &message)
{
  log(LogLevel::ERROR, message);
}

void quiz::util::Logger::setLevel(LogLevel level)
{
  levelName(level);
  const std::lock_guard< std::mutex > LOCK(loggerMutex());
  current_level_ = level;
}

void quiz::util::Logger::log(LogLevel level, const std::string &message)
{
  const std::lock_guard< std::mutex > LOCK(loggerMutex());
  if (level >= current_level_)
  {
    std::clog << '[' << levelName(level) << "] " << message << '\n';
  }
}
