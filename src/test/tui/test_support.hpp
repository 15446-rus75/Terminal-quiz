#ifndef TEST_SUPPORT_HPP
#define TEST_SUPPORT_HPP

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "tui/terminal.hpp"
#include "db/iattempt_repository.hpp"
#include "db/ifavorite_repository.hpp"
#include "db/iquestion_repository.hpp"
#include "services/favorite_service.hpp"
#include "services/quiz_service.hpp"


namespace test
{
  class FakeTerminal: public quiz::tui::ITerminal
  {
  public:
    std::optional< std::string > readLine(const std::string &prompt) override
    {
      output_ += prompt;
      if (input_lines_.empty())
      {
        return std::nullopt;
      }
      std::string line = input_lines_.front();
      input_lines_.erase(input_lines_.begin());
      return line;
    }

    void write(const std::string &text, quiz::tui::TextStyle style) override
    {
      output_ += text;
      styles_.push_back(style);
    }

    void clear() override
    {
      cleared_ = true;
    }

    size_t width() const override
    {
      return width_;
    }

    void setInput(const std::vector< std::string > &lines)
    {
      input_lines_ = lines;
    }

    std::string getOutput() const
    {
      return output_;
    }

    bool wasCleared() const
    {
      return cleared_;
    }

    void setWidth(size_t width)
    {
      width_ = width;
    }

  private:
    std::vector< std::string > input_lines_;
    std::string output_;
    std::vector< quiz::tui::TextStyle > styles_;
    bool cleared_ = false;
    size_t width_ = 80;
  };

  class MockAttemptRepository: public quiz::db::IAttemptRepository
  {
  public:
    size_t save(const quiz::core::Attempt &attempt) override
    {
      attempts_.push_back(attempt);
      attempts_.back().id = next_id_++;
      return attempts_.back().id;
    }

    void saveAnswers(size_t attempt_id, const std::vector< quiz::core::AttemptAnswer > &answers) override
    {
      for (auto &att : attempts_)
      {
        if (att.id == attempt_id)
        {
          att.answers = answers;
          return;
        }
      }
    }

    void finish(size_t attempt_id, size_t score, std::chrono::system_clock::time_point finished_at) override
    {
      for (auto &att : attempts_)
      {
        if (att.id == attempt_id)
        {
          att.score = score;
          att.finished_at = finished_at;
          return;
        }
      }
    }

    std::optional< quiz::core::Attempt > findById(size_t id) override
    {
      for (const auto &att : attempts_)
      {
        if (att.id == id)
        {
          return att;
        }
      }
      return std::nullopt;
    }

    std::vector< quiz::core::Attempt > findByUserId(size_t, size_t) override
    {
      return attempts_;
    }

    std::vector< quiz::core::Attempt > attempts_;

  private:
    size_t next_id_ = 1;
  };

  class MockFavoriteRepository: public quiz::db::IFavoriteRepository
  {
  public:
    void add(size_t user_id, size_t question_id) override
    {
      if (!exists(user_id, question_id))
      {
        favorites_.push_back({user_id, question_id});
      }
    }

    void remove(size_t user_id, size_t question_id) override
    {
      std::erase_if(favorites_, [user_id, question_id](const auto &pair)
                    { return pair.first == user_id && pair.second == question_id; });
    }

    std::vector< quiz::core::Question > findByUserId(size_t user_id) override
    {
      std::vector< quiz::core::Question > result;
      for (const auto &pair : favorites_)
      {
        if (pair.first == user_id)
        {
          quiz::core::Question q;
          q.id = pair.second;
          q.category = quiz::core::Category::GENERAL_KNOWLEDGE;
          q.difficulty = quiz::core::Difficulty::EASY;
          q.type = quiz::core::QuestionType::MULTIPLE;
          q.text = "Favorite question " + std::to_string(pair.second);
          q.correct_answer = "A";
          result.push_back(q);
        }
      }
      return result;
    }

    bool exists(size_t user_id, size_t question_id) override
    {
      for (const auto &pair : favorites_)
      {
        if (pair.first == user_id && pair.second == question_id)
        {
          return true;
        }
      }
      return false;
    }

  private:
    std::vector< std::pair< size_t, size_t > > favorites_;
  };

  class MockQuestionRepository: public quiz::db::IQuestionRepository
  {
  public:
    size_t save(const quiz::core::Question &) override
    {
      return 0;
    }

    std::vector< size_t > saveBatch(const std::vector< quiz::core::Question > &) override
    {
      return {};
    }

    std::optional< quiz::core::Question > findById(size_t id) override
    {
      for (const auto &q : questions_)
      {
        if (q.id == id)
        {
          return q;
        }
      }
      return std::nullopt;
    }

    std::vector< quiz::core::Question > findByFilter(quiz::core::Category, quiz::core::Difficulty, size_t) override
    {
      return questions_;
    }

    std::vector< quiz::core::Question > searchByText(const std::string &, quiz::core::Category,
                                                      quiz::core::Difficulty, size_t) override
    {
      return questions_;
    }

    size_t count() override
    {
      return questions_.size();
    }

    void setQuestions(const std::vector< quiz::core::Question > &questions)
    {
      questions_ = questions;
    }

  private:
    std::vector< quiz::core::Question > questions_;
  };
}

#endif
