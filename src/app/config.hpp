#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <cstddef>
#include <string>

#include "../util/logger.hpp"

namespace quiz
{
  namespace app
  {
    struct AppConfig
    {
      std::string db_host;
      unsigned short db_port;
      std::string db_name;
      std::string db_user;
      std::string db_password;
      size_t cache_capacity;
      size_t user_id;
      bool use_colors;
      util::LogLevel log_level;
    };

    AppConfig defaultConfig();
    AppConfig loadConfigFromEnvironment();
  }
}

#endif
