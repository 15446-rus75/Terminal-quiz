#ifndef IDATABASE_HPP
#define IDATABASE_HPP

#include <memory>
#include <string>

namespace quiz
{
  namespace db
  {
    class IConnection
    {
    public:
      virtual ~IConnection() = default;
      virtual bool isConnected() const = 0;
      virtual void beginTransaction() = 0;
      virtual void commit() = 0;
      virtual void rollback() = 0;
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
