#include "statistics_service.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../core/statistics.hpp"
#include "../db/istatistics_repository.hpp"

quiz::service::StatisticsService::StatisticsService(std::shared_ptr< db::IStatisticsRepository > statistics_repository):
  statistics_repository_(std::move(statistics_repository))
{
  if (statistics_repository_ == nullptr)
  {
    throw std::invalid_argument("Statistics repository must not be null");
  }
}

quiz::core::OverallStats quiz::service::StatisticsService::getOverall(size_t user_id)
{
  return statistics_repository_->getOverall(user_id);
}

std::vector< quiz::core::CategoryStats > quiz::service::StatisticsService::getByCategory(size_t user_id)
{
  return statistics_repository_->getByCategory(user_id);
}

std::vector< quiz::core::DifficultyStats > quiz::service::StatisticsService::getByDifficulty(size_t user_id)
{
  return statistics_repository_->getByDifficulty(user_id);
}
