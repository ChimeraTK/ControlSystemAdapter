#include "StubProcessVariableFactory.h"
#include "stdint.h"

#define TYPED_STUB_PROCESS_VARIABLE( T	)\
  if(variableType == typeid(T) ){\
    return boost::shared_ptr< ProcessVariable<T> >( new StubProcessVariable<T> ); \
  }else

namespace mtca4u{

  boost::any StubProcessVariableFactory::createProcessVariable( std::type_info const & variableType, 
								std::string /*name*/ ){
    TYPED_STUB_PROCESS_VARIABLE( uint32_t )
    TYPED_STUB_PROCESS_VARIABLE( int32_t )
    TYPED_STUB_PROCESS_VARIABLE( uint16_t )
    TYPED_STUB_PROCESS_VARIABLE( int16_t )
    TYPED_STUB_PROCESS_VARIABLE( uint8_t )
    TYPED_STUB_PROCESS_VARIABLE( int8_t )
    TYPED_STUB_PROCESS_VARIABLE( float )
    TYPED_STUB_PROCESS_VARIABLE( double )
    {
      throw std::bad_typeid();
    }
  }

}// namespace mtca4u
