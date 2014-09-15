#include "StubProcessVariableFactory.h"
#include "StubProcessVariable.h"
#include "StubProcessArray.h"

#include "stdint.h"

#define TYPED_STUB_PROCESS_VARIABLE( T )\
  if(variableType == typeid(T) ){\
    return boost::shared_ptr< ProcessVariable<T> >( new StubProcessVariable<T> ); \
  }else

#define TYPED_STUB_PROCESS_ARRAY( T, SIZE )\
  if(variableType == typeid(T) ){\
    return boost::shared_ptr< ProcessArray<T> >( new StubProcessArray<T>(SIZE) ); \
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

  boost::any StubProcessVariableFactory::createProcessArray( std::type_info const & variableType, 
							     std::string /*name*/,
							     size_t arraySize){
    TYPED_STUB_PROCESS_ARRAY( uint32_t, arraySize )
    TYPED_STUB_PROCESS_ARRAY( int32_t, arraySize )
    TYPED_STUB_PROCESS_ARRAY( uint16_t, arraySize )
    TYPED_STUB_PROCESS_ARRAY( int16_t, arraySize )
    TYPED_STUB_PROCESS_ARRAY( uint8_t, arraySize )
    TYPED_STUB_PROCESS_ARRAY( int8_t, arraySize )
    TYPED_STUB_PROCESS_ARRAY( float, arraySize )
    TYPED_STUB_PROCESS_ARRAY( double, arraySize )
    {
      throw std::bad_typeid();
    }
  }

}// namespace mtca4u
