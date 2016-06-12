#ifndef MTCA4U_PROCESS_ARRAY_ACCESSOR_H
#define MTCA4U_PROCESS_ARRAY_ACCESSOR_H

#include "ProcessArray.h"
#include "ProcessVariableAccessor.h"

namespace mtca4u{
  template<class T>
    class NonModifiableProcessArrayAccessor: public ProcessVariableAccessor{

  public:
  NonModifiableProcessArrayAccessor(typename ProcessArray<T>::SharedPtr const & processArray =
			typename ProcessArray<T>::SharedPtr())
    : ProcessVariableAccessor(processArray){
      //      _processArray( boost::static_pointer_cast< ProcessArray<T> >( _impl ) ){      
    }

    std::vector<T> const & get() const{
      return boost::static_pointer_cast< ProcessArray<T> >(_impl)->get();
    }

    std::vector<T> const & getConst() const{
      return boost::static_pointer_cast< ProcessArray<T> >(_impl)->getConst();
    }
    
  };

  template<class T>
    class ProcessArrayAccessor: public NonModifiableProcessArrayAccessor<T>{

  public:
  ProcessArrayAccessor(typename ProcessArray<T>::SharedPtr const & processArray =
			typename ProcessArray<T>::SharedPtr())
    : NonModifiableProcessArrayAccessor<T>(processArray){
      //      _processArray( boost::static_pointer_cast< ProcessArray<T> >( _impl ) ){      
    }

    std::vector<T> & get(){
      return boost::static_pointer_cast< ProcessArray<T> >(ProcessVariableAccessor::_impl)->get();
    }
    
  };

} //namespace mtca4u

#endif // MTCA4U_PROCESS_ARRAY_ACCESSOR_H
