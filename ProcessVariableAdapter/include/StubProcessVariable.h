#ifndef MTCA4U_STUB_PROCESS_VARIABLE_H
#define MTCA4U_STUB_PROCESS_VARIABLE_H

#include "ProcessVariable.h"

namespace mtca4u{

template<class T>
  class StubProcessVariableTest;

/** Implementation of the ProcessVariable for the control system stub.
 *  The contained "control system" variable is just the simple type itself.
 *  The callback fuctionality is fully functional.
 */
template<class T>
  class StubProcessVariable : public ProcessVariable<T>{
 protected:
  T _t; ///< The instance of the simple type

  /** The function which is executed when set() is called
   */
  boost::function< void (T const & /*newValue*/, T const & /*oldValue*/) > _onSetCallbackFunction;

  /** The function which is executed when get() is called
   */ 
  boost::function< T () > _onGetCallbackFunction;  

  /** The constructors are private and can only be created by the factory.
   */
  StubProcessVariable(T const & t=0) : _t(t){}

  /** The test is friend to allow testing of the constructors.*/
  friend class StubProcessVariableTest<T>;
  /** The factory is friend because someone has to be able to construct.*/
  friend class StubProcessVariableFactory;
 public:

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
  
  /** Implementation of the copy assigmment operator for the StubProcessVariable.
   *  It is needed to prevent a default copy assignment operator from being created, which would also copy
   *  the callback functions (although it never should be used because the calling code should not know
   *  that this is a StubProcessVariable and always use operator=(ProcessVariable<T> const & other).
   */
  /*  In addition it's faster because it avoids
   *  calling the virtual getWithoutCallback() function and one copy of the template type.
   */
  StubProcessVariable<T> & operator=(StubProcessVariable<T> const & other){
    _t=other._t;
    return (*this);
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
    // use the assignment operator which resolves the virtual function 
    *this = other;
  }

  void setWithoutCallback(T const & t){
    _t=t;
  }

  void set(ProcessVariable<T> const & other){
    StubProcessVariable<T>::set( other.getWithoutCallback() );
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
