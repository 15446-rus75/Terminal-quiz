#ifndef FORMAT_HPP
#define FORMAT_HPP

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace quiz
{
  namespace tui
  {
    std::vector< std::string > wrapText(const std::string &text, size_t width);
    std::string padRight(const std::string &text, size_t width);
    std::string formatAccuracy(double accuracy);
    std::string formatTimePoint(std::chrono::system_clock::time_point time);
    std::string formatDuration(std::chrono::seconds duration);
  }
}

#endif
