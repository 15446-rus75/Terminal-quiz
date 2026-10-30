#include <cstddef>
#include <memory>
#include <stdexcept>
#include <vector>

#include "core/question.hpp"
#include "db/ifavorite_repository.hpp"
#include "services/favorite_service.hpp"
#include <boost/test/unit_test.hpp>

namespace core = quiz::core;
namespace db = quiz::db;
namespace service = quiz::service;

namespace
{
  class MockFavoriteRepository: public db::IFavoriteRepository
  {
  public:
    void add(size_t user_id, size_t question_id) override
    {
      favorites.push_back({user_id, question_id});
    }

    void remove(size_t user_id, size_t question_id) override
    {
      std::erase_if(favorites, [user_id, question_id](const auto &pair) {
        return pair.first == user_id && pair.second == question_id;
      });
    }

    std::vector< core::Question > findByUserId(size_t user_id) override
    {
      std::vector< core::Question > result;
      for (const auto &pair: favorites)
      {
        if (pair.first == user_id)
        {
          core::Question q;
          q.id = pair.second;
          result.push_back(q);
        }
      }
      return result;
    }

    bool exists(size_t user_id, size_t question_id) override
    {
      for (const auto &pair: favorites)
      {
        if (pair.first == user_id && pair.second == question_id)
        {
          return true;
        }
      }
      return false;
    }

    std::vector< std::pair< size_t, size_t > > favorites;
  };
}

BOOST_AUTO_TEST_SUITE(favorite_service_constructor)

BOOST_AUTO_TEST_CASE(null_repository_throws)
{
  BOOST_CHECK_THROW(service::FavoriteService(nullptr), std::invalid_argument);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(favorite_service_operations)

BOOST_AUTO_TEST_CASE(add_and_check_is_favorite)
{
  auto repo = std::make_shared< MockFavoriteRepository >();
  service::FavoriteService service(repo);

  BOOST_TEST(!service.isFavorite(1, 100));
  service.add(1, 100);
  BOOST_TEST(service.isFavorite(1, 100));
}

BOOST_AUTO_TEST_CASE(remove_favorite)
{
  auto repo = std::make_shared< MockFavoriteRepository >();
  service::FavoriteService service(repo);

  service.add(1, 100);
  service.remove(1, 100);
  BOOST_TEST(!service.isFavorite(1, 100));
}

BOOST_AUTO_TEST_CASE(get_all_favorites)
{
  auto repo = std::make_shared< MockFavoriteRepository >();
  service::FavoriteService service(repo);

  service.add(1, 100);
  service.add(1, 200);
  service.add(2, 300);

  const auto USER1_FAVS = service.getAll(1);
  BOOST_TEST(USER1_FAVS.size() == 2U);
  BOOST_TEST(USER1_FAVS[0].id == 100U);
  BOOST_TEST(USER1_FAVS[1].id == 200U);
}

BOOST_AUTO_TEST_SUITE_END()
