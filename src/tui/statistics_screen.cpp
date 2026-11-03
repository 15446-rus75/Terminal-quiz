#include "statistics_screen.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/statistics.hpp"
#include "../services/statistics_service.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

quiz::tui::StatisticsScreen::StatisticsScreen(std::shared_ptr< ITerminal > terminal,
                                              std::shared_ptr< service::StatisticsService > statistics_service,
                                              size_t user_id):
  renderer_(std::move(terminal)),
  statistics_service_(std::move(statistics_service)),
  user_id_(user_id)
{
  if (statistics_service_ == nullptr)
  {
    throw std::invalid_argument("Statistics service must not be null");
  }
}

std::string quiz::tui::StatisticsScreen::title() const
{
  return "Statistics";
}

void quiz::tui::StatisticsScreen::run()
{
  renderer_.showTitle(title());
  const core::OverallStats OVERALL = statistics_service_->getOverall(user_id_);
  const std::vector< core::CategoryStats > BY_CATEGORY = statistics_service_->getByCategory(user_id_);
  const std::vector< core::DifficultyStats > BY_DIFFICULTY = statistics_service_->getByDifficulty(user_id_);
  renderer_.showStatistics(OVERALL, BY_CATEGORY, BY_DIFFICULTY);
}
