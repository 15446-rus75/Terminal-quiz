#include "applictaion.hpp"

#include <exception>
#include <iostream>
#include <memory>
#include <vector>

#include "../api/open_tdb_client.hpp"
#include "../db/postgres_attempt_repository.hpp"
#include "../db/postgres_custom_test_repository.hpp"
#include "../db/postgres_database.hpp"
#include "../db/postgres_favorite_repository.hpp"
#include "../db/postgres_question_repository.hpp"
#include "../db/postgres_statistics_repository.hpp"
#include "../db/schema_initializer.hpp"
#include "../services/custom_test_service.hpp"
#include "../services/favorite_service.hpp"
#include "../services/history_service.hpp"
#include "../services/question_service.hpp"
#include "../services/quiz_service.hpp"
#include "../services/statistics_service.hpp"
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

namespace db = quiz::db;
namespace api = quiz::api;
namespace service = quiz::service;
namespace tui = quiz::tui;

quiz::app::Application::Application(const AppConfig &config, std::shared_ptr< tui::ITerminal > terminal):
  config_(config), terminal_(std::move(terminal))
{
}

int quiz::app::Application::run()
{
  try
  {
    auto database = std::make_shared< db::PostgresDatabase >(config_.db_host, config_.db_port, config_.db_name,
                                                             config_.db_user, config_.db_password);
    db::SchemaInitializer(database).apply();

    auto question_repository = std::make_shared< db::PostgresQuestionRepository >(database);
    auto attempt_repository = std::make_shared< db::PostgresAttemptRepository >(database);
    auto favorite_repository = std::make_shared< db::PostgresFavoriteRepository >(database);
    auto custom_test_repository = std::make_shared< db::PostgresCustomTestRepository >(database);
    auto statistics_repository = std::make_shared< db::PostgresStatisticsRepository >(database);

    auto open_tdb_client = std::make_shared< api::OpenTdbClient >();

    auto question_service =
      std::make_shared< service::QuestionService >(open_tdb_client, question_repository, config_.cache_capacity);
    auto quiz_service = std::make_shared< service::QuizService >(attempt_repository);
    auto favorite_service = std::make_shared< service::FavoriteService >(favorite_repository);
    auto history_service = std::make_shared< service::HistoryService >(attempt_repository, question_repository);
    auto custom_test_service =
      std::make_shared< service::CustomTestService >(custom_test_repository, question_repository);
    auto statistics_service = std::make_shared< service::StatisticsService >(statistics_repository);

    auto arranger = std::make_shared< tui::RandomAnswerArranger >();
    auto runner =
      std::make_shared< tui::QuizRunner >(terminal_, quiz_service, favorite_service, arranger, config_.user_id);

    std::vector< std::shared_ptr< tui::IScreen > > screens;
    screens.push_back(std::make_shared< tui::QuizScreen >(terminal_, question_service, runner));
    screens.push_back(
      std::make_shared< tui::CustomTestScreen >(terminal_, custom_test_service, question_service, runner,
                                                config_.user_id));
    screens.push_back(std::make_shared< tui::FavoritesScreen >(terminal_, favorite_service, runner, config_.user_id));
    screens.push_back(
      std::make_shared< tui::SearchScreen >(terminal_, question_service, favorite_service, config_.user_id));
    screens.push_back(std::make_shared< tui::HistoryScreen >(terminal_, history_service, config_.user_id));
    screens.push_back(std::make_shared< tui::StatisticsScreen >(terminal_, statistics_service, config_.user_id));

    tui::MainMenu menu(terminal_, screens);
    menu.run();
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "Application failed to start: " << error.what() << std::endl;
    return 1;
  }
}
