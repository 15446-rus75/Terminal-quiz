#ifndef STATISTICS_HPP
#define STATISTICS_HPP

#include "enums.hpp"
#include <cstdint>

namespace quiz
{
  namespace core
  {
    struct CategoryStats
    {
      Category category;
      size_t total_attempts;
      size_t total_correct;
      size_t total_questions;
      double accuracy;
    };

    struct DifficultyStats
    {
      Difficulty difficulty;
      size_t total_attempts;
      size_t total_correct;
      size_t total_questions;
      double accuracy;
    };

    struct OverallStats
    {
      size_t total_attempts;
      size_t total_questions;
      size_t total_correct;
      double overall_accuracy;
    };
  }
}

#endif
