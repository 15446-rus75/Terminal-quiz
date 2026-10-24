#include "postgres_database.hpp"

#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
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

  quiz::db::ResultSet executeQuery(PGconn *connection, const std::string &sql, const std::vector< std::string > &params)
  {
    if (sql.find('\0') != std::string::npos)
    {
      throw std::invalid_argument("SQL contains a null byte");
    }
    if (params.size() > static_cast< size_t >(std::numeric_limits< int >::max()))
    {
      throw std::invalid_argument("Too many query parameters");
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
    const std::unique_ptr< PGresult, ResultDeleter > ROWS(
      PQexecParams(connection, sql.c_str(), static_cast< int >(values.size()), nullptr,
                   values.empty() ? nullptr : values.data(), nullptr, nullptr, 0));
    if (!ROWS)
    {
      throw std::runtime_error("PostgreSQL query failed: " + std::string(PQerrorMessage(connection)));
    }
    const ExecStatusType STATUS = PQresultStatus(ROWS.get());
    if (STATUS != PGRES_TUPLES_OK && STATUS != PGRES_COMMAND_OK)
    {
      throw std::runtime_error("PostgreSQL query failed: " + std::string(PQresultErrorMessage(ROWS.get())));
    }
    quiz::db::ResultSet result;
    const int ROW_COUNT = PQntuples(ROWS.get());
    const int COLUMN_COUNT = PQnfields(ROWS.get());
    result.reserve(static_cast< size_t >(ROW_COUNT));
    for (int row_index = 0; row_index < ROW_COUNT; ++row_index)
    {
      quiz::db::Row row;
      row.reserve(static_cast< size_t >(COLUMN_COUNT));
      for (int column = 0; column < COLUMN_COUNT; ++column)
      {
        if (PQgetisnull(ROWS.get(), row_index, column) != 0)
        {
          throw std::runtime_error("SQL NULL cannot be represented by ResultSet; use an explicit SQL conversion");
        }
        row.emplace_back(PQgetvalue(ROWS.get(), row_index, column),
                         static_cast< size_t >(PQgetlength(ROWS.get(), row_index, column)));
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

  private:
    std::unique_ptr< PGconn, ConnectionDeleter > connection_;
    bool transaction_;
  };

  PostgresConnection::PostgresConnection(const std::string &connection_string):
    connection_(PQconnectdb(connection_string.c_str())), transaction_(false)
  {
    if (!connection_)
    {
      throw std::runtime_error("Failed to allocate PostgreSQL connection");
    }
    if (PQstatus(connection_.get()) != CONNECTION_OK || PQsetClientEncoding(connection_.get(), "UTF8") != 0)
    {
      throw std::runtime_error("PostgreSQL connection failed: " + std::string(PQerrorMessage(connection_.get())));
    }
  }

  bool PostgresConnection::isConnected() const
  {
    return connection_ && PQstatus(connection_.get()) == CONNECTION_OK;
  }

  void PostgresConnection::beginTransaction()
  {
    if (!isConnected())
    {
      throw std::runtime_error("PostgreSQL connection is closed");
    }
    if (transaction_)
    {
      throw std::logic_error("A transaction is already active");
    }
    executeQuery(connection_.get(), "BEGIN", {});
    transaction_ = true;
  }

  void PostgresConnection::commit()
  {
    if (!transaction_)
    {
      throw std::logic_error("No active transaction to commit");
    }
    if (PQtransactionStatus(connection_.get()) == PQTRANS_INERROR)
    {
      throw std::logic_error("Failed transaction must be rolled back");
    }
    executeQuery(connection_.get(), "COMMIT", {});
    transaction_ = false;
  }

  void PostgresConnection::rollback()
  {
    if (transaction_)
    {
      executeQuery(connection_.get(), "ROLLBACK", {});
      transaction_ = false;
    }
  }

  quiz::db::ResultSet PostgresConnection::execute(const std::string &sql, const std::vector< std::string > &params)
  {
    if (transaction_)
    {
      return executeQuery(connection_.get(), sql, params);
    }
    beginTransaction();
    try
    {
      quiz::db::ResultSet result = executeQuery(connection_.get(), sql, params);
      commit();
      return result;
    }
    catch (...)
    {
      try
      {
        rollback();
      }
      catch (...)
      {
        connection_.reset();
        transaction_ = false;
      }
      throw;
    }
  }
}

quiz::db::PostgresDatabase::PostgresDatabase(const std::string &host, size_t port, const std::string &dbname,
                                             const std::string &user, const std::string &password)
{
  if (port == 0 || port > MAX_PORT)
  {
    throw std::invalid_argument("PostgreSQL port is out of range");
  }
  connection_string_ = "host=" + quoteConnectionValue(host) + " port=" + quoteConnectionValue(std::to_string(port)) +
                       " dbname=" + quoteConnectionValue(dbname) + " user=" + quoteConnectionValue(user) +
                       " password=" + quoteConnectionValue(password) +
                       " connect_timeout=" + std::to_string(CONNECT_TIMEOUT_SECONDS);
}

quiz::db::PostgresDatabase::~PostgresDatabase() = default;

std::shared_ptr< quiz::db::IConnection > quiz::db::PostgresDatabase::getConnection()
{
  return std::make_shared< PostgresConnection >(connection_string_);
}
