#include "postgres_database.hpp"

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "idatabase.hpp"
#if __has_include(<libpq-fe.h>)
#include <libpq-fe.h>
#else
#include <postgresql/libpq-fe.h>
#endif

namespace
{
  constexpr size_t MAX_PORT = 65535;
  constexpr int CONNECT_TIMEOUT_SECONDS = 5;
  constexpr size_t MAX_POOL_SIZE = 4;
  constexpr size_t MAX_PREPARED_STATEMENTS = 64;
  constexpr std::chrono::seconds POOL_WAIT_TIMEOUT(5);

  std::string quoteConnectionValue(const std::string &value)
  {
    std::string result = "'";
    for (const char CH : value)
    {
      if (CH == '\0')
      {
        throw std::invalid_argument("Connection parameter contains a null byte");
      }
      if (CH == '\'' || CH == '\\')
      {
        result.push_back('\\');
      }
      result.push_back(CH);
    }
    result.push_back('\'');
    return result;
  }
  struct ConnectionDeleter
  {
    void operator()(PGconn *connection) const;
  };
  void ConnectionDeleter::operator()(PGconn *connection) const
  {
    PQfinish(connection);
  }
  struct ResultDeleter
  {
    void operator()(PGresult *result) const;
  };
  void ResultDeleter::operator()(PGresult *result) const
  {
    PQclear(result);
  }
  using OwnedResult = std::unique_ptr< PGresult, ResultDeleter >;

  void checkResult(PGconn *connection, const OwnedResult &rows)
  {
    if (!rows)
    {
      throw std::runtime_error("PostgreSQL query failed: " + std::string(PQerrorMessage(connection)));
    }
    const ExecStatusType STATUS = PQresultStatus(rows.get());
    if (STATUS != PGRES_TUPLES_OK && STATUS != PGRES_COMMAND_OK)
    {
      throw std::runtime_error("PostgreSQL query failed: " + std::string(PQresultErrorMessage(rows.get())));
    }
  }
  quiz::db::ResultSet readResult(PGconn *connection, const OwnedResult &rows)
  {
    checkResult(connection, rows);
    quiz::db::ResultSet result;
    const int ROW_COUNT = PQntuples(rows.get());
    const int COLUMN_COUNT = PQnfields(rows.get());
    result.reserve(static_cast< size_t >(ROW_COUNT));
    for (int row_index = 0; row_index < ROW_COUNT; ++row_index)
    {
      quiz::db::Row row;
      row.reserve(static_cast< size_t >(COLUMN_COUNT));
      for (int column = 0; column < COLUMN_COUNT; ++column)
      {
        if (PQgetisnull(rows.get(), row_index, column) != 0)
        {
          throw std::runtime_error("SQL NULL cannot be represented by ResultSet; use an explicit SQL conversion");
        }
        row.emplace_back(PQgetvalue(rows.get(), row_index, column),
                         static_cast< size_t >(PQgetlength(rows.get(), row_index, column)));
      }
      result.push_back(std::move(row));
    }
    return result;
  }

  class PostgresConnection: public quiz::db::IConnection
  {
  public:
    explicit PostgresConnection(const std::string &connection_string);
    bool isConnected() const override;
    void beginTransaction() override;
    void commit() override;
    void rollback() override;
    quiz::db::ResultSet execute(const std::string &sql, const std::vector< std::string > &params) override;
    bool resetForReuse() noexcept;

  private:
    std::unique_ptr< PGconn, ConnectionDeleter > connection_;
    bool transaction_;
    std::unordered_map< std::string, std::string > statements_;
    size_t next_statement_;
  };
  PostgresConnection::PostgresConnection(const std::string &connection_string):
    connection_(PQconnectdb(connection_string.c_str())), transaction_(false), next_statement_(0)
  {
    if (!connection_)
    {
      throw std::runtime_error("Failed to allocate PostgreSQL connection");
    }
    if (PQstatus(connection_.get()) != CONNECTION_OK || PQsetClientEncoding(connection_.get(), "UTF8") != 0)
    {
      throw std::runtime_error("PostgreSQL connection failed: " + std::string(PQerrorMessage(connection_.get())));
    }
    statements_.reserve(MAX_PREPARED_STATEMENTS);
  }
  bool PostgresConnection::isConnected() const
  {
    return connection_ && PQstatus(connection_.get()) == CONNECTION_OK;
  }
  void PostgresConnection::beginTransaction()
  {
    if (transaction_ || (isConnected() && PQtransactionStatus(connection_.get()) != PQTRANS_IDLE))
    {
      throw std::logic_error("A transaction is already active");
    }
    execute("BEGIN", {});
    transaction_ = true;
  }
  void PostgresConnection::commit()
  {
    if (!transaction_)
    {
      throw std::logic_error("No active transaction to commit");
    }
    if (!isConnected() || PQtransactionStatus(connection_.get()) == PQTRANS_INERROR)
    {
      throw std::logic_error("Failed transaction must be rolled back");
    }
    execute("COMMIT", {});
    transaction_ = false;
  }
  void PostgresConnection::rollback()
  {
    if (transaction_ || (isConnected() && PQtransactionStatus(connection_.get()) != PQTRANS_IDLE))
    {
      execute("ROLLBACK", {});
      transaction_ = false;
    }
  }
  quiz::db::ResultSet PostgresConnection::execute(const std::string &sql, const std::vector< std::string > &params)
  {
    if (!isConnected())
    {
      throw std::runtime_error("PostgreSQL connection is closed");
    }
    if (sql.find('\0') != std::string::npos || params.size() > static_cast< size_t >(std::numeric_limits< int >::max()))
    {
      throw std::invalid_argument("Invalid SQL or parameter count");
    }
    std::vector< const char * > values;
    values.reserve(params.size());
    for (const auto &parameter : params)
    {
      if (parameter.find('\0') != std::string::npos)
      {
        throw std::invalid_argument("Query parameter contains a null byte");
      }
      values.push_back(parameter.c_str());
    }
    if (values.empty())
    {
      const OwnedResult ROWS(PQexec(connection_.get(), sql.c_str()));
      return readResult(connection_.get(), ROWS);
    }
    auto found = statements_.find(sql);
    if (found == statements_.end() && statements_.size() < MAX_PREPARED_STATEMENTS)
    {
      const std::string NAME = "terminal_quiz_" + std::to_string(next_statement_++);
      const auto INSERTED = statements_.emplace(sql, NAME);
      try
      {
        const OwnedResult PREPARED(
          PQprepare(connection_.get(), NAME.c_str(), sql.c_str(), static_cast< int >(values.size()), nullptr));
        checkResult(connection_.get(), PREPARED);
      }
      catch (...)
      {
        statements_.erase(INSERTED.first);
        throw;
      }
      found = INSERTED.first;
    }
    const OwnedResult ROWS(found == statements_.end()
                             ? PQexecParams(connection_.get(), sql.c_str(), static_cast< int >(values.size()), nullptr,
                                            values.data(), nullptr, nullptr, 0)
                             : PQexecPrepared(connection_.get(), found->second.c_str(),
                                              static_cast< int >(values.size()), values.data(), nullptr, nullptr, 0));
    return readResult(connection_.get(), ROWS);
  }
  bool PostgresConnection::resetForReuse() noexcept
  {
    try
    {
      if (!isConnected())
      {
        throw std::runtime_error("Disconnected pooled connection");
      }
      rollback();
      if (PQtransactionStatus(connection_.get()) != PQTRANS_IDLE)
      {
        throw std::runtime_error("Pooled connection is not idle");
      }
      return true;
    }
    catch (...)
    {
      connection_.reset();
      statements_.clear();
      transaction_ = false;
      return false;
    }
  }
}

class quiz::db::PostgresDatabase::PoolState: public std::enable_shared_from_this< PoolState >
{
public:
  explicit PoolState(std::string connection_string);
  std::shared_ptr< IConnection > acquire();
  void release(IConnection *raw) noexcept;

private:
  class Available
  {
  public:
    explicit Available(PoolState &pool);
    bool operator()() const;

  private:
    PoolState &pool_;
  };

  class Returner
  {
  public:
    explicit Returner(std::shared_ptr< PoolState > pool);
    void operator()(IConnection *connection) const noexcept;

  private:
    std::shared_ptr< PoolState > pool_;
  };
  std::string connection_string_;
  std::mutex mutex_;
  std::condition_variable available_;
  size_t total_;
  std::vector< std::unique_ptr< PostgresConnection > > idle_;
};
quiz::db::PostgresDatabase::PoolState::PoolState(std::string connection_string):
  connection_string_(std::move(connection_string)), total_(0)
{
  idle_.reserve(MAX_POOL_SIZE);
}

quiz::db::PostgresDatabase::PoolState::Available::Available(PoolState &pool): pool_(pool)
{
}

bool quiz::db::PostgresDatabase::PoolState::Available::operator()() const
{
  return !pool_.idle_.empty() || pool_.total_ < MAX_POOL_SIZE;
}
quiz::db::PostgresDatabase::PoolState::Returner::Returner(std::shared_ptr< PoolState > pool): pool_(std::move(pool))
{
}
void quiz::db::PostgresDatabase::PoolState::Returner::operator()(IConnection *connection) const noexcept
{
  pool_->release(connection);
}
std::shared_ptr< quiz::db::IConnection > quiz::db::PostgresDatabase::PoolState::acquire()
{
  std::unique_ptr< PostgresConnection > connection;
  std::unique_lock< std::mutex > lock(mutex_);
  const auto DEADLINE = std::chrono::steady_clock::now() + POOL_WAIT_TIMEOUT;
  while (!connection)
  {
    if (!idle_.empty())
    {
      connection = std::move(idle_.back());
      idle_.pop_back();
      if (!connection->isConnected())
      {
        connection.reset();
        --total_;
      }
    }
    else if (total_ < MAX_POOL_SIZE)
    {
      ++total_;
      lock.unlock();
      try
      {
        connection = std::make_unique< PostgresConnection >(connection_string_);
      }
      catch (...)
      {
        const std::lock_guard< std::mutex > LOCK(mutex_);
        --total_;
        available_.notify_one();
        throw;
      }
      return std::shared_ptr< IConnection >(connection.release(), Returner(shared_from_this()));
    }
    else if (!available_.wait_until(lock, DEADLINE, Available(*this)))
    {
      throw std::runtime_error("PostgreSQL connection pool wait timed out");
    }
  }
  lock.unlock();
  return std::shared_ptr< IConnection >(connection.release(), Returner(shared_from_this()));
}
void quiz::db::PostgresDatabase::PoolState::release(IConnection *raw) noexcept
{
  std::unique_ptr< PostgresConnection > connection(static_cast< PostgresConnection * >(raw));
  const bool REUSABLE = connection->resetForReuse();
  const std::lock_guard< std::mutex > LOCK(mutex_);
  if (REUSABLE)
  {
    idle_.push_back(std::move(connection));
  }
  else
  {
    --total_;
  }
  available_.notify_one();
}
quiz::db::PostgresDatabase::PostgresDatabase(const std::string &host, size_t port, const std::string &dbname,
                                             const std::string &user, const std::string &password)
{
  if (port == 0 || port > MAX_PORT)
  {
    throw std::invalid_argument("PostgreSQL port is out of range");
  }
  const std::string CONNECTION_STRING =
    "host=" + quoteConnectionValue(host) + " port=" + quoteConnectionValue(std::to_string(port)) +
    " dbname=" + quoteConnectionValue(dbname) + " user=" + quoteConnectionValue(user) +
    " password=" + quoteConnectionValue(password) + " connect_timeout=" + std::to_string(CONNECT_TIMEOUT_SECONDS);
  pool_ = std::make_shared< PoolState >(CONNECTION_STRING);
}
quiz::db::PostgresDatabase::~PostgresDatabase() = default;
std::shared_ptr< quiz::db::IConnection > quiz::db::PostgresDatabase::getConnection()
{
  return pool_->acquire();
}
