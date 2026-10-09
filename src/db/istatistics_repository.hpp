#ifndef ISTATISTICS_REPOSITORY_HPP
#define ISTATISTICS_REPOSITORY_HPP

#include <cstdint>
#include <vector>

#include "../core/statistics.hpp"

namespace quiz
{
  namespace db
  {
    class IStatisticsRepository
    {
    public:
      virtual ~IStatisticsRepository() = default;
      virtual core::OverallStats getOverall(size_t user_id) = 0;
      virtual std::vector< core::CategoryStats > getByCategory(size_t user_id) = 0;
      virtual std::vector< core::DifficultyStats > getByDifficulty(size_t user_id) = 0;
    };
  }
}

#endif
