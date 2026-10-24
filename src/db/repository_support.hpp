#ifndef REPOSITORY_SUPPORT_HPP
#define REPOSITORY_SUPPORT_HPP

#include <chrono>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/question.hpp"
#include "idatabase.hpp"

namespace quiz
{
  namespace db
  {
    namespace detail
    {
      inline constexpr char QUESTION_COLUMNS[] = "id, category, difficulty, question_type, text, correct_answer";
      inline constexpr char ATTEMPT_COLUMNS[] =
        "id, user_id, score, total_questions, "
        "(EXTRACT(EPOCH FROM started_at) * 1000000)::bigint::text, "
        "COALESCE((EXTRACT(EPOCH FROM finished_at) * 1000000)::bigint::text, '')";

      class Transaction
      {
      public:
        explicit Transaction(const std::shared_ptr< IDatabase > &database);
        Transaction(const Transaction &) = delete;
        Transaction &operator=(const Transaction &) = delete;
        Transaction(Transaction &&) = delete;
        Transaction &operator=(Transaction &&) = delete;
        ~Transaction();
        IConnection &connection();
        void commit();

      private:
        std::shared_ptr< IConnection > connection_;
        bool active_;
      };

      void requireDatabase(const std::shared_ptr< IDatabase > &database);
      std::string number(size_t value);
      size_t readSize(const std::string &text);
      void requireColumns(const Row &row, size_t count);
      std::string timeParameter(std::chrono::system_clock::time_point time);
      std::chrono::system_clock::time_point readTime(const std::string &text);
      std::string timestampExpression(const std::string &parameter);
      std::string literalSearchPattern(const std::string &query);
      void validateQuestion(const core::Question &question);
      void validateFilter(core::Category category, core::Difficulty difficulty);
      size_t saveQuestion(IConnection &connection, const core::Question &question);
      std::vector< core::Question > readQuestions(IConnection &connection, const ResultSet &rows);
      void replaceAnswers(IConnection &connection, size_t attempt_id,
                          const std::vector< core::AttemptAnswer > &answers);
      std::vector< core::Attempt > readAttempts(IConnection &connection, const ResultSet &rows);
      double accuracy(size_t correct, size_t total);
    }
  }
}

#endif
