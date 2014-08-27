#ifndef MTCA4U_STUB_PROCESS_VARIABLE_FACTORY
#define MTCA4U_STUB_PROCESS_VARIABLE_FACTORY

#include <boost/any.hpp>

#include "StubProcessVariable.h"
#include "ProcessVariableFactory.h"

namespace mtca4u{
  
  /** Implementation of the ProcessVariableFactory for the control system stub.
   *  The retruned ProcessVariables are of the type StubProcessVariable.
   */
  class StubProcessVariableFactory : public ProcessVariableFactory{

  protected:
    /** Creates StubProcessVariables of the following simple data types:
     *  \li uint32_t
     *  \li int32_t 
     *  \li uint16_t
     *  \li int16_t 
     *  \li uint8_t 
     *  \li int8_t
     *  \li float  
     *  \li double
     *
     *  \throw std::bad_typeid is thrown if the type is not of the supported types.
     */
    boost::any createProcessVariable( std::type_info  const & variableType, std::string name );
  };

}// namespace mtca4u

#endif// MTCA4U_STUB_PROCESS_VARIABLE_FACTORY
