#ifndef ICUSTOM_TEST_REPOSITORY_HPP
#define ICUSTOM_TEST_REPOSITORY_HPP

#include "../core/custom_test.hpp"
#include <cstdint>
#include <optional>
#include <vector>

namespace quiz
{
  namespace db
  {
    class ICustomTestRepository
    {
    public:
      virtual ~ICustomTestRepository() = default;
      virtual size_t save(const core::CustomTest &test) = 0;
      virtual std::optional< core::CustomTest > findById(size_t id) = 0;
      virtual std::vector< core::CustomTest > findByUserId(size_t user_id) = 0;
      virtual void remove(size_t id) = 0;
    };
  }
}

#endif
