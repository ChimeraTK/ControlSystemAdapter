#ifndef MTCA4U_STUB_PROCESS_VARIABLE_H
#define MTCA4U_STUB_PROCESS_VARIABLE_H

#include "ProcessVariable.h"

namespace mtca4u{

template<class T>
  class StubProcessVariable : public ProcessVariable<T>{
 protected:
  T _t;
  boost::function< void (T const & /*newValue*/, T const & /*newValue*/) > _onSetCallbackFunction;
  boost::function< T () > _onGetCallbackFunction;  

 public:
  StubProcessVariable(){}
  StubProcessVariable(T const & t) : _t(t){}

  void setOnSetCallbackFunction( 
	boost::function< void (T const & /*newValue*/, T const & /*oldValue*/) > onSetCallbackFunction){
    _onSetCallbackFunction = onSetCallbackFunction;
  }
  
  void setOnGetCallbackFunction( boost::function< T () > onGetCallbackFunction){
    _onGetCallbackFunction = onGetCallbackFunction;    
  }

  void clearOnSetCallbackFunction(){
    _onSetCallbackFunction.clear();
  }

  void clearOnGetCallbackFunction(){
    _onGetCallbackFunction.clear();
  }
  
  StubProcessVariable<T> & operator=(StubProcessVariable<T> const & other){
    // avoid self asignment
    if (this == &other){
      return *this;
    }

    // use the type T assignment to avoid code duplication
    return (*this = other._t);
  }

  StubProcessVariable<T> & operator=(ProcessVariable<T> const & other){
    _t=other.getWithoutCallback();
    return (*this);
  }

  StubProcessVariable<T> & operator=(T const & t){
    // Only asign the template class, but not the callback functions.
    _t = t;

    return *this;
  }

  void setWithoutCallback(ProcessVariable<T> const & other){
    *this = other;
  }

  void setWithoutCallback(T const & t){
    // use the asignment operator which also checks for change
    *this = t;
  }

  void set(StubProcessVariable<T> const & other){
    // avoid self asignment
    if (this == &other){
      return;
    }

    set( other._t );
  }

  void set(ProcessVariable<T> const & other){
    set( other.getWithoutCallback() );
  }

  void set(T const & t){
    T oldValue = _t;
    _t = t;
    if (_onSetCallbackFunction){
      _onSetCallbackFunction(t,oldValue);
    }
  }

  operator T () const {
    return _t;
  }

  T get(){
    if (_onGetCallbackFunction){
      _t=_onGetCallbackFunction();
    }
    return _t;
  }

  T getWithoutCallback() const{
    return _t;
  }


};

}//namespace mtca4u

#endif// MTCA4U_STUB_PROCESS_VARIABLE_H
