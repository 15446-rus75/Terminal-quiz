#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

namespace quiz
{
  namespace util
  {
    enum class LogLevel : std::uint8_t
    {
      DEBUG = 0,
      INFO = 1,
      WARNING = 2,
      ERROR = 3
    };

    class Logger
    {
    public:
      static void debug(const std::string &message);
      static void info(const std::string &message);
      static void warning(const std::string &message);
      static void error(const std::string &message);
      static void setLevel(LogLevel level);

    private:
      static void log(LogLevel level, const std::string &message);

      static LogLevel current_level_;
    };
  }
}

#endif
