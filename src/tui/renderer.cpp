#include "renderer.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../core/attempt.hpp"
#include "../core/custom_test.hpp"
#include "../core/enums.hpp"
#include "../core/question.hpp"
#include "../core/statistics.hpp"
#include "../services/history_service.hpp"
#include "../services/quiz_service.hpp"
#include "format.hpp"
#include "terminal.hpp"

namespace
{
  constexpr size_t MIN_TEXT_WIDTH = 20;
  constexpr size_t MAX_RULE_WIDTH = 60;
  constexpr size_t ATTEMPT_SCORE_WIDTH = 9;
  constexpr size_t ATTEMPT_ACCURACY_WIDTH = 7;
  constexpr size_t ATTEMPTS_COLUMN_WIDTH = 8;
  constexpr size_t CORRECT_COLUMN_WIDTH = 9;
  constexpr const char *UNFINISHED = "unfinished";

  struct StatsRow
  {
    std::string label;
    size_t attempts;
    size_t correct;
    size_t questions;
    double accuracy;
  };

  struct RowBuilder
  {
    StatsRow operator()(const quiz::core::CategoryStats &stats) const
    {
      return StatsRow{.label = quiz::core::toString(stats.category),
                      .attempts = stats.total_attempts,
                      .correct = stats.total_correct,
                      .questions = stats.total_questions,
                      .accuracy = stats.accuracy};
    }

    StatsRow operator()(const quiz::core::DifficultyStats &stats) const
    {
      return StatsRow{.label = capitalize(quiz::core::toString(stats.difficulty)),
                      .attempts = stats.total_attempts,
                      .correct = stats.total_correct,
                      .questions = stats.total_questions,
                      .accuracy = stats.accuracy};
    }

    static std::string capitalize(std::string text)
    {
      if (!text.empty())
      {
        text.front() = static_cast< char >(std::toupper(static_cast< unsigned char >(text.front())));
      }
      return text;
    }
  };

  struct LabelLength
  {
    size_t operator()(const StatsRow &row) const
    {
      return row.label.size();
    }
  };

  std::string capitalize(const std::string &text)
  {
    return RowBuilder::capitalize(text);
  }

  size_t textWidth(size_t total, size_t indent)
  {
    return total > indent + MIN_TEXT_WIDTH ? total - indent : MIN_TEXT_WIDTH;
  }

  std::string itemPrefix(size_t number, size_t count)
  {
    const size_t DIGITS = std::to_string(count).size();
    return "  " + quiz::tui::padRight(std::to_string(number) + ".", DIGITS + 1) + " ";
  }

  std::vector< std::string > prefixedLines(const std::string &text, const std::string &prefix, size_t total_width)
  {
    const std::vector< std::string > WRAPPED = quiz::tui::wrapText(text, textWidth(total_width, prefix.size()));
    const std::string CONTINUATION(prefix.size(), ' ');

    std::vector< std::string > lines;
    lines.reserve(WRAPPED.size());
    for (size_t i = 0; i < WRAPPED.size(); ++i)
    {
      lines.push_back((i == 0 ? prefix : CONTINUATION) + WRAPPED[i]);
    }
    return lines;
  }

  std::string questionMeta(const quiz::core::Question &question)
  {
    return quiz::core::toString(question.category) + " | " + capitalize(quiz::core::toString(question.difficulty)) +
           " | " + capitalize(quiz::core::toString(question.type));
  }

  double ratio(size_t part, size_t total)
  {
    return total == 0 ? 0.0 : static_cast< double >(part) / static_cast< double >(total);
  }

  std::string scoreText(size_t score, size_t total)
  {
    return std::to_string(score) + "/" + std::to_string(total);
  }

  std::string attemptDuration(const quiz::core::Attempt &attempt)
  {
    if (attempt.finished_at == std::chrono::system_clock::time_point{})
    {
      return UNFINISHED;
    }
    return quiz::tui::formatDuration(
      std::chrono::duration_cast< std::chrono::seconds >(attempt.finished_at - attempt.started_at));
  }

  std::vector< std::string > tableLines(const std::string &first_title, const std::vector< StatsRow > &rows)
  {
    std::vector< size_t > lengths;
    lengths.reserve(rows.size() + 1);
    lengths.push_back(first_title.size());
    std::ranges::transform(rows, std::back_inserter(lengths), LabelLength{});
    const size_t LABEL_WIDTH = *std::ranges::max_element(lengths);

    std::vector< std::string > lines;
    lines.push_back(quiz::tui::padRight(first_title, LABEL_WIDTH) + "  " +
                    quiz::tui::padRight("Attempts", ATTEMPTS_COLUMN_WIDTH) + "  " +
                    quiz::tui::padRight("Correct", CORRECT_COLUMN_WIDTH) + "  Accuracy");
    for (const StatsRow &row : rows)
    {
      lines.push_back(quiz::tui::padRight(row.label, LABEL_WIDTH) + "  " +
                      quiz::tui::padRight(std::to_string(row.attempts), ATTEMPTS_COLUMN_WIDTH) + "  " +
                      quiz::tui::padRight(scoreText(row.correct, row.questions), CORRECT_COLUMN_WIDTH) + "  " +
                      quiz::tui::formatAccuracy(row.accuracy));
    }
    return lines;
  }
}

quiz::tui::Renderer::Renderer(std::shared_ptr< ITerminal > terminal): terminal_(std::move(terminal))
{
  if (terminal_ == nullptr)
  {
    throw std::invalid_argument("Terminal must not be null");
  }
}

void quiz::tui::Renderer::showTitle(const std::string &title)
{
  writeLine("", TextStyle::PLAIN);
  writeLine(title, TextStyle::TITLE);
  writeLine(std::string(std::min(terminal_->width(), MAX_RULE_WIDTH), '-'), TextStyle::MUTED);
}

void quiz::tui::Renderer::showMenu(const std::vector< std::string > &items, const std::string &exit_label)
{
  for (size_t i = 0; i < items.size(); ++i)
  {
    writeLine("  " + std::to_string(i + 1) + ". " + items[i], TextStyle::PLAIN);
  }
  writeLine("  0. " + exit_label, TextStyle::MUTED);
}

void quiz::tui::Renderer::showInfo(const std::string &message)
{
  writeLine(message, TextStyle::PLAIN);
}

void quiz::tui::Renderer::showSuccess(const std::string &message)
{
  writeLine(message, TextStyle::SUCCESS);
}

void quiz::tui::Renderer::showError(const std::string &message)
{
  writeLine(message, TextStyle::ERROR);
}

void quiz::tui::Renderer::showHelp(const std::vector< std::string > &lines)
{
  writeLine("", TextStyle::PLAIN);
  writeLine("Help", TextStyle::TITLE);
  for (const std::string &line : lines)
  {
    writeLine(line, TextStyle::PLAIN);
  }
}

void quiz::tui::Renderer::showQuestion(const QuestionPosition &position, const core::Question &question,
                                       const std::vector< std::string > &options)
{
  const size_t WIDTH = terminal_->width();
  writeLine("", TextStyle::PLAIN);
  writeLine("Question " + std::to_string(position.number) + " of " + std::to_string(position.total), TextStyle::TITLE);
  writeLine(questionMeta(question), TextStyle::MUTED);
  writeLine("", TextStyle::PLAIN);
  for (const std::string &line : prefixedLines(question.text, "", WIDTH))
  {
    writeLine(line, TextStyle::PLAIN);
  }
  writeLine("", TextStyle::PLAIN);
  for (size_t i = 0; i < options.size(); ++i)
  {
    for (const std::string &line : prefixedLines(options[i], itemPrefix(i + 1, options.size()), WIDTH))
    {
      writeLine(line, TextStyle::PLAIN);
    }
  }
}

void quiz::tui::Renderer::showAnswerFeedback(bool is_correct, const core::Question &question)
{
  if (is_correct)
  {
    writeLine("Correct!", TextStyle::SUCCESS);
    return;
  }
  for (const std::string &line :
       prefixedLines("Wrong. Correct answer: " + question.correct_answer, "", terminal_->width()))
  {
    writeLine(line, TextStyle::ERROR);
  }
}

void quiz::tui::Renderer::showQuizResult(const service::QuizResult &result)
{
  writeLine("", TextStyle::PLAIN);
  writeLine("Quiz finished", TextStyle::TITLE);
  writeLine("Score:    " + scoreText(result.score, result.total), TextStyle::PLAIN);
  writeLine("Accuracy: " + formatAccuracy(result.accuracy), TextStyle::PLAIN);
  if (result.total > 0 && result.score == result.total)
  {
    writeLine("Perfect score!", TextStyle::SUCCESS);
  }
}

void quiz::tui::Renderer::showQuestionList(const std::vector< core::Question > &questions)
{
  if (questions.empty())
  {
    showInfo("No questions found.");
    return;
  }

  const size_t WIDTH = terminal_->width();
  for (size_t i = 0; i < questions.size(); ++i)
  {
    const std::string PREFIX = itemPrefix(i + 1, questions.size());
    for (const std::string &line : prefixedLines(questions[i].text, PREFIX, WIDTH))
    {
      writeLine(line, TextStyle::PLAIN);
    }
    writeLine(std::string(PREFIX.size(), ' ') + questionMeta(questions[i]), TextStyle::MUTED);
  }
}

void quiz::tui::Renderer::showCustomTests(const std::vector< core::CustomTest > &tests)
{
  if (tests.empty())
  {
    showInfo("You have no custom tests yet.");
    return;
  }

  const size_t WIDTH = terminal_->width();
  for (size_t i = 0; i < tests.size(); ++i)
  {
    const std::string PREFIX = itemPrefix(i + 1, tests.size());
    for (const std::string &line : prefixedLines(tests[i].name, PREFIX, WIDTH))
    {
      writeLine(line, TextStyle::PLAIN);
    }
    writeLine(std::string(PREFIX.size(), ' ') + std::to_string(tests[i].question_ids.size()) +
                " question(s), created " + formatTimePoint(tests[i].created_at),
              TextStyle::MUTED);
  }
}

void quiz::tui::Renderer::showAttemptList(const std::vector< core::Attempt > &attempts)
{
  if (attempts.empty())
  {
    showInfo("No attempts yet.");
    return;
  }

  for (size_t i = 0; i < attempts.size(); ++i)
  {
    const core::Attempt &attempt = attempts[i];
    writeLine(itemPrefix(i + 1, attempts.size()) + formatTimePoint(attempt.started_at) + "  " +
                padRight(scoreText(attempt.score, attempt.total_questions), ATTEMPT_SCORE_WIDTH) + "  " +
                padRight(formatAccuracy(ratio(attempt.score, attempt.total_questions)), ATTEMPT_ACCURACY_WIDTH) + "  " +
                attemptDuration(attempt),
              TextStyle::PLAIN);
  }
}

void quiz::tui::Renderer::showAttemptDetails(const service::AttemptDetails &details)
{
  const core::Attempt &attempt = details.attempt;
  writeLine("", TextStyle::PLAIN);
  writeLine("Attempt #" + std::to_string(attempt.id), TextStyle::TITLE);
  writeLine("Started:  " + formatTimePoint(attempt.started_at), TextStyle::PLAIN);
  writeLine("Duration: " + attemptDuration(attempt), TextStyle::PLAIN);
  writeLine("Score:    " + scoreText(attempt.score, attempt.total_questions) + " (" +
              formatAccuracy(ratio(attempt.score, attempt.total_questions)) + ")",
            TextStyle::PLAIN);

  const size_t COUNT = std::min(attempt.answers.size(), details.questions.size());
  if (COUNT == 0)
  {
    writeLine("", TextStyle::PLAIN);
    showInfo("No answers were recorded for this attempt.");
    return;
  }

  const size_t WIDTH = terminal_->width();
  for (size_t i = 0; i < COUNT; ++i)
  {
    const core::AttemptAnswer &answer = attempt.answers[i];
    const core::Question &question = details.questions[i];
    const std::string PREFIX = itemPrefix(i + 1, COUNT);
    const std::string INDENT(PREFIX.size(), ' ');

    writeLine("", TextStyle::PLAIN);
    for (const std::string &line : prefixedLines(question.text, PREFIX, WIDTH))
    {
      writeLine(line, TextStyle::PLAIN);
    }
    const TextStyle STYLE = answer.is_correct ? TextStyle::SUCCESS : TextStyle::ERROR;
    for (const std::string &line : prefixedLines("Your answer: " + answer.user_answer, INDENT, WIDTH))
    {
      writeLine(line, STYLE);
    }
    if (!answer.is_correct)
    {
      for (const std::string &line : prefixedLines("Correct answer: " + question.correct_answer, INDENT, WIDTH))
      {
        writeLine(line, TextStyle::SUCCESS);
      }
    }
  }
}

void quiz::tui::Renderer::showStatistics(const core::OverallStats &overall,
                                         const std::vector< core::CategoryStats > &by_category,
                                         const std::vector< core::DifficultyStats > &by_difficulty)
{
  if (overall.total_attempts == 0)
  {
    showInfo("No statistics yet. Finish a quiz first.");
    return;
  }

  writeLine("", TextStyle::PLAIN);
  writeLine("Overall", TextStyle::TITLE);
  writeLine("Attempts:  " + std::to_string(overall.total_attempts), TextStyle::PLAIN);
  writeLine("Questions: " + std::to_string(overall.total_questions), TextStyle::PLAIN);
  writeLine("Correct:   " + std::to_string(overall.total_correct) + " (" + formatAccuracy(overall.overall_accuracy) +
              ")",
            TextStyle::PLAIN);

  if (!by_category.empty())
  {
    std::vector< StatsRow > rows;
    rows.reserve(by_category.size());
    std::ranges::transform(by_category, std::back_inserter(rows), RowBuilder{});
    writeLine("", TextStyle::PLAIN);
    writeLine("By category", TextStyle::TITLE);
    for (const std::string &line : tableLines("Category", rows))
    {
      writeLine(line, TextStyle::PLAIN);
    }
  }
  if (!by_difficulty.empty())
  {
    std::vector< StatsRow > rows;
    rows.reserve(by_difficulty.size());
    std::ranges::transform(by_difficulty, std::back_inserter(rows), RowBuilder{});
    writeLine("", TextStyle::PLAIN);
    writeLine("By difficulty", TextStyle::TITLE);
    for (const std::string &line : tableLines("Difficulty", rows))
    {
      writeLine(line, TextStyle::PLAIN);
    }
  }
}

void quiz::tui::Renderer::writeLine(const std::string &text, TextStyle style)
{
  terminal_->write(text + "\n", style);
}
