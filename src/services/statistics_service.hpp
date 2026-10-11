#ifndef STATISTICS_SERVICE_HPP
#define STATISTICS_SERVICE_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include "../core/statistics.hpp"
#include "../db/i_statistics_repository.hpp"

namespace quiz
{
  namespace service
  {
    class StatisticsService
    {
    public:
      explicit StatisticsService(std::shared_ptr< db::IStatisticsRepository > statistics_repository);
      core::OverallStats getOverall(size_t user_id);
      std::vector< core::CategoryStats > getByCategory(size_t user_id);
      std::vector< core::DifficultyStats > getByDifficulty(size_t user_id);

    private:
      std::shared_ptr< db::IStatisticsRepository > statistics_repository_;
    };
  }
}

#endif
