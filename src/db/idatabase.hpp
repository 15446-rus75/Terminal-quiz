#ifndef IDATABASE_HPP
#define IDATABASE_HPP

#include <memory>
#include <string>
#include <vector>

namespace quiz
{
  namespace db
  {
    using Row = std::vector< std::string >;
    using ResultSet = std::vector< Row >;

    class IConnection
    {
    public:
      virtual ~IConnection() = default;
      virtual bool isConnected() const = 0;
      virtual void beginTransaction() = 0;
      virtual void commit() = 0;
      virtual void rollback() = 0;
      virtual ResultSet execute(const std::string &sql, const std::vector< std::string > &params) = 0;
    };

    class IDatabase
    {
    public:
      virtual ~IDatabase() = default;
      virtual std::shared_ptr< IConnection > getConnection() = 0;
    };
  }
}

#endif
