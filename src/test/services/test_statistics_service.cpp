#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

#include "core/statistics.hpp"
#include "db/istatistics_repository.hpp"
#include "services/statistics_service.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;
namespace db = quiz::db;
namespace service = quiz::service;

namespace
{
  class MockStatisticsRepository: public db::IStatisticsRepository
  {
  public:
    core::OverallStats getOverall(size_t user_id) override
    {
      core::OverallStats stats;
      stats.total_attempts = (user_id == 1) ? 5 : 0;
      return stats;
    }

    std::vector< core::CategoryStats > getByCategory(size_t user_id) override
    {
      if (user_id == 1)
      {
        core::CategoryStats c;
        c.category = core::Category::GENERAL_KNOWLEDGE;
        return {c};
      }
      return {};
    }

    std::vector< core::DifficultyStats > getByDifficulty(size_t user_id) override
    {
      if (user_id == 1)
      {
        core::DifficultyStats d;
        d.difficulty = core::Difficulty::EASY;
        return {d};
      }
      return {};
    }
  };
}

BOOST_AUTO_TEST_SUITE(statistics_service_constructor)

BOOST_AUTO_TEST_CASE(null_repository_throws)
{
  BOOST_CHECK_THROW(service::StatisticsService(nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(statistics_service_queries)

BOOST_AUTO_TEST_CASE(get_overall_stats)
{
  auto repo = std::make_shared< MockStatisticsRepository >();
  service::StatisticsService service(repo);

  const auto STATS = service.getOverall(1);
  BOOST_TEST(STATS.total_attempts == 5U);
}

BOOST_AUTO_TEST_CASE(get_category_stats)
{
  auto repo = std::make_shared< MockStatisticsRepository >();
  service::StatisticsService service(repo);

  const auto STATS = service.getByCategory(1);
  BOOST_TEST(STATS.size() == 1U);
}

BOOST_AUTO_TEST_CASE(get_difficulty_stats)
{
  auto repo = std::make_shared< MockStatisticsRepository >();
  service::StatisticsService service(repo);

  const auto STATS = service.getByDifficulty(1);
  BOOST_TEST(STATS.size() == 1U);
}

BOOST_AUTO_TEST_SUITE_END()
