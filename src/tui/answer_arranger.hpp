#ifndef ANSWER_ARRANGER_HPP
#define ANSWER_ARRANGER_HPP

#include <random>
#include <string>
#include <vector>

#include "../core/question.hpp"

namespace quiz
{
  namespace tui
  {
    class IAnswerArranger
    {
    public:
      virtual ~IAnswerArranger() = default;
      virtual std::vector< std::string > arrange(const core::Question &question) = 0;
    };

    class RandomAnswerArranger: public IAnswerArranger
    {
    public:
      RandomAnswerArranger();
      explicit RandomAnswerArranger(unsigned int seed);
      std::vector< std::string > arrange(const core::Question &question) override;

    private:
      std::mt19937 generator_;
    };
  }
}

#endif
