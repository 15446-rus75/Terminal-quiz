#include "history_screen.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/attempt.hpp"
#include "../services/history_service.hpp"
#include "input_parsing.hpp"
#include "prompter.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace
{
  constexpr size_t RECENT_ATTEMPTS_LIMIT = 20;
}

quiz::tui::HistoryScreen::HistoryScreen(std::shared_ptr< ITerminal > terminal,
                                        std::shared_ptr< service::HistoryService > history_service, size_t user_id):
  renderer_(terminal), prompter_(std::move(terminal)), history_service_(std::move(history_service)), user_id_(user_id)
{
  if (history_service_ == nullptr)
  {
    throw std::invalid_argument("History service must not be null");
  }
}

std::string quiz::tui::HistoryScreen::title() const
{
  return "History";
}

void quiz::tui::HistoryScreen::run()
{
  renderer_.showTitle(title());
  const std::vector< core::Attempt > ATTEMPTS = history_service_->recent(user_id_, RECENT_ATTEMPTS_LIMIT);
  renderer_.showAttemptList(ATTEMPTS);
  if (!ATTEMPTS.empty())
  {
    showDetails(ATTEMPTS);
  }
}

void quiz::tui::HistoryScreen::showDetails(const std::vector< core::Attempt > &attempts)
{
  const NumberRange RANGE{.min = 1, .max = attempts.size()};
  while (true)
  {
    const std::optional< size_t > NUMBER = prompter_.askNumber("Attempt to view (/back to return)", RANGE);
    if (!NUMBER.has_value())
    {
      return;
    }

    const std::optional< service::AttemptDetails > DETAILS = history_service_->details(attempts[*NUMBER - 1].id);
    if (!DETAILS.has_value())
    {
      renderer_.showError("Attempt not found.");
      continue;
    }
    renderer_.showAttemptDetails(*DETAILS);
  }
}
