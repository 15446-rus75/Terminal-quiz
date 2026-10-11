#ifndef POSTGRES_DATABASE_HPP
#define POSTGRES_DATABASE_HPP

#include <memory>
#include <string>

#include "idatabase.hpp"

namespace quiz
{
  namespace db
  {
    class PostgresDatabase: public IDatabase
    {
    public:
      PostgresDatabase(const std::string &host, std::size_t port, const std::string &dbname, const std::string &user,
                       const std::string &password);
      ~PostgresDatabase() override;
      std::shared_ptr< IConnection > getConnection() override;

    private:
      std::string connection_string_;
    };
  }
}

#endif
