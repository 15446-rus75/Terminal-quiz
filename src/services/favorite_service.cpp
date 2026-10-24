#include "favorite_service.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "../core/question.hpp"
#include "../db/ifavorite_repository.hpp"

quiz::service::FavoriteService::FavoriteService(std::shared_ptr< db::IFavoriteRepository > favorite_repository):
  favorite_repository_(std::move(favorite_repository))
{
  if (favorite_repository_ == nullptr)
  {
    throw std::invalid_argument("Favorite repository must not be null");
  }
}

void quiz::service::FavoriteService::add(size_t user_id, size_t question_id)
{
  if (!favorite_repository_->exists(user_id, question_id))
  {
    favorite_repository_->add(user_id, question_id);
  }
}

void quiz::service::FavoriteService::remove(size_t user_id, size_t question_id)
{
  favorite_repository_->remove(user_id, question_id);
}

std::vector< quiz::core::Question > quiz::service::FavoriteService::getAll(size_t user_id)
{
  return favorite_repository_->findByUserId(user_id);
}

bool quiz::service::FavoriteService::isFavorite(size_t user_id, size_t question_id)
{
  return favorite_repository_->exists(user_id, question_id);
}
