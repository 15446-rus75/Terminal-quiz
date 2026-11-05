#include "applictaion.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "../api/open_tdb_client.hpp"
#include "../db/postgres_attempt_repository.hpp"
#include "../db/postgres_custom_test_repository.hpp"
#include "../db/postgres_database.hpp"
#include "../db/postgres_favorite_repository.hpp"
#include "../db/postgres_question_repository.hpp"
#include "../db/postgres_statistics_repository.hpp"
#include "../db/schema_initializer.hpp"
#include "../service/custom_test_service.hpp"
#include "../service/favorite_service.hpp"
#include "../service/history_service.hpp"
#include "../service/question_service.hpp"
#include "../service/quiz_service.hpp"
#include "../service/statistics_service.hpp"
#include "../tui/answer_arranger.hpp"
#include "../tui/custom_test_screen.hpp"
#include "../tui/favorites_screen.hpp"
#include "../tui/history_screen.hpp"
#include "../tui/main_menu.hpp"
#include "../tui/quiz_runner.hpp"
#include "../tui/quiz_screen.hpp"
#include "../tui/screen.hpp"
#include "../tui/search_screen.hpp"
#include "../tui/statistics_screen.hpp"
#include "../tui/terminal.hpp"
#include "config.hpp"

quiz::app::Application::Application(const AppConfig &config, std::shared_ptr< quiz::tui::ITerminal > terminal):
  config_(config), terminal_(std::move(terminal))
{
}

int quiz::app::Application::run()
{
  try
  {
    auto database = std::make_shared< quiz::db::PostgresDatabase >(
      config_.db_host, config_.db_port, config_.db_name, config_.db_user, config_.db_password);
    quiz::db::SchemaInitializer(database).apply();

    auto question_repository = std::make_shared< quiz::db::PostgresQuestionRepository >(database);
    auto attempt_repository = std::make_shared< quiz::db::PostgresAttemptRepository >(database);
    auto favorite_repository = std::make_shared< quiz::db::PostgresFavoriteRepository >(database);
    auto custom_test_repository = std::make_shared< quiz::db::PostgresCustomTestRepository >(database);
    auto statistics_repository = std::make_shared< quiz::db::PostgresStatisticsRepository >(database);

    auto open_tdb_client = std::make_shared< quiz::api::OpenTdbClient >();

    auto question_service = std::make_shared< quiz::service::QuestionService >(open_tdb_client, question_repository,
                                                                               config_.cache_capacity);
    auto quiz_service = std::make_shared< quiz::service::QuizService >(attempt_repository);
    auto favorite_service = std::make_shared< quiz::service::FavoriteService >(favorite_repository);
    auto history_service =
      std::make_shared< quiz::service::HistoryService >(attempt_repository, question_repository);
    auto custom_test_service =
      std::make_shared< quiz::service::CustomTestService >(custom_test_repository, question_repository);
    auto statistics_service = std::make_shared< quiz::service::StatisticsService >(statistics_repository);

    auto arranger = std::make_shared< quiz::tui::RandomAnswerArranger >();
    auto runner = std::make_shared< quiz::tui::QuizRunner >(terminal_, quiz_service, favorite_service, arranger,
                                                            config_.user_id);

    std::vector< std::shared_ptr< quiz::tui::IScreen > > screens;
    screens.push_back(std::make_shared< quiz::tui::QuizScreen >(terminal_, question_service, runner));
    screens.push_back(std::make_shared< quiz::tui::CustomTestScreen >(terminal_, custom_test_service,
                                                                      question_service, runner, config_.user_id));
    screens.push_back(
      std::make_shared< quiz::tui::FavoritesScreen >(terminal_, favorite_service, runner, config_.user_id));
    screens.push_back(std::make_shared< quiz::tui::SearchScreen >(terminal_, question_service, favorite_service,
                                                                  config_.user_id));
    screens.push_back(std::make_shared< quiz::tui::HistoryScreen >(terminal_, history_service, config_.user_id));
    screens.push_back(
      std::make_shared< quiz::tui::StatisticsScreen >(terminal_, statistics_service, config_.user_id));

    quiz::tui::MainMenu menu(terminal_, screens);
    menu.run();
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "Application failed to start: " << error.what() << std::endl;
    return 1;
  }
}
