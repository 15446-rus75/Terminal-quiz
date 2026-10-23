#ifndef SCHEMA_INITIALIZER_HPP
#define SCHEMA_INITIALIZER_HPP

#include <memory>

#include "idatabase.hpp"

namespace quiz
{
  namespace db
  {

    class SchemaInitializer
    {
    public:
      explicit SchemaInitializer(std::shared_ptr< IDatabase > database);
      void apply();

    private:
      std::shared_ptr< IDatabase > database_;
    };
  }
}

#endif
