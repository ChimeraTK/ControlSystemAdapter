#ifndef MTCA4U_STUB_PROCESS_VARIABLE_FACTORY
#define MTCA4U_STUB_PROCESS_VARIABLE_FACTORY

#include <boost/any.hpp>

#include "StubProcessVariable.h"
#include "ProcessVariableFactory.h"

namespace mtca4u{
  
  class StubProcessVariableFactory : public ProcessVariableFactory{

  protected:
    boost::any createProcessVariable( std::type_info  const & variableType, std::string name );
  };

}// namespace mtca4u

#endif// MTCA4U_STUB_PROCESS_VARIABLE_FACTORY
