#ifndef POSTGRES_STATISTICS_REPOSITORY_HPP
#define POSTGRES_STATISTICS_REPOSITORY_HPP

#include "idatabase.hpp"
#include "istatistics_repository.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace quiz
{
  namespace db
  {
    class PostgresStatisticsRepository: public IStatisticsRepository
    {
    public:
      explicit PostgresStatisticsRepository(std::shared_ptr< IDatabase > database);
      core::OverallStats getOverall(size_t user_id) override;
      std::vector< core::CategoryStats > getByCategory(size_t user_id) override;
      std::vector< core::DifficultyStats > getByDifficulty(size_t user_id) override;

    private:
      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
