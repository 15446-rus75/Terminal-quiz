#ifndef CUSTOM_TEST_HPP
#define CUSTOM_TEST_HPP

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace quiz
{
  namespace core
  {
    struct CustomTest
    {
      size_t id;
      size_t user_id;
      std::string name;
      std::chrono::system_clock::time_point created_at;
      std::vector< size_t > question_ids;
    };
  }
}

#endif
