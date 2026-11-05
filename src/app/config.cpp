#include "config.hpp"

#include <cctype>
#include <cstdlib>
#include <string>

namespace
{
  std::string envOr(const char *name, const std::string &fallback)
  {
    const char *value = std::getenv(name);
    if (value == nullptr || *value == '\0')
    {
      return fallback;
    }
    return value;
  }

  std::string toLower(std::string text)
  {
    for (char &ch : text)
    {
      ch = static_cast< char >(std::tolower(static_cast< unsigned char >(ch)));
    }
    return text;
  }

  unsigned short parsePort(const std::string &text, unsigned short fallback)
  {
    try
    {
      const long PARSED = std::stol(text);
      if (PARSED <= 0 || PARSED > 65535)
      {
        return fallback;
      }
      return static_cast< unsigned short >(PARSED);
    }
    catch (...)
    {
      return fallback;
    }
  }

  std::size_t parseSize(const std::string &text, std::size_t fallback)
  {
    try
    {
      const long long PARSED = std::stoll(text);
      if (PARSED < 0)
      {
        return fallback;
      }
      return static_cast< std::size_t >(PARSED);
    }
    catch (...)
    {
      return fallback;
    }
  }

  bool parseBool(const std::string &text, bool fallback)
  {
    const std::string LOWER = toLower(text);
    if (LOWER == "1" || LOWER == "true" || LOWER == "yes" || LOWER == "on")
    {
      return true;
    }
    if (LOWER == "0" || LOWER == "false" || LOWER == "no" || LOWER == "off")
    {
      return false;
    }
    return fallback;
  }

  quiz::util::LogLevel parseLogLevel(const std::string &text, quiz::util::LogLevel fallback)
  {
    const std::string LOWER = toLower(text);
    if (LOWER == "debug")
    {
      return quiz::util::LogLevel::DEBUG;
    }
    if (LOWER == "info")
    {
      return quiz::util::LogLevel::INFO;
    }
    if (LOWER == "warning" || LOWER == "warn")
    {
      return quiz::util::LogLevel::WARNING;
    }
    if (LOWER == "error")
    {
      return quiz::util::LogLevel::ERROR;
    }
    return fallback;
  }
}

quiz::app::AppConfig quiz::app::defaultConfig()
{
  AppConfig config;
  config.db_host = "localhost";
  config.db_port = 5432;
  config.db_name = "quiz";
  config.db_user = "quiz";
  config.db_password = "quiz";
  config.cache_capacity = 100;
  config.user_id = 1;
  config.use_colors = true;
  config.log_level = util::LogLevel::INFO;
  return config;
}

quiz::app::AppConfig quiz::app::loadConfigFromEnvironment()
{
  AppConfig config = defaultConfig();
  config.db_host = envOr("QUIZ_DB_HOST", config.db_host);
  config.db_port = parsePort(envOr("QUIZ_DB_PORT", std::to_string(config.db_port)), config.db_port);
  config.db_name = envOr("QUIZ_DB_NAME", config.db_name);
  config.db_user = envOr("QUIZ_DB_USER", config.db_user);
  config.db_password = envOr("QUIZ_DB_PASSWORD", config.db_password);
  config.cache_capacity =
    parseSize(envOr("QUIZ_CACHE_CAPACITY", std::to_string(config.cache_capacity)), config.cache_capacity);
  config.user_id = parseSize(envOr("QUIZ_USER_ID", std::to_string(config.user_id)), config.user_id);
  config.use_colors = parseBool(envOr("QUIZ_USE_COLORS", config.use_colors ? "true" : "false"), config.use_colors);
  config.log_level = parseLogLevel(envOr("QUIZ_LOG_LEVEL", "info"), config.log_level);
  return config;
}
