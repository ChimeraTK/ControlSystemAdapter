#ifndef MTCA4U_PROCESS_VARIABLE_H
#define MTCA4U_PROCESS_VARIABLE_H

#include <boost/function.hpp>

namespace mtca4u{

template<class T>
class ProcessVariable{
 protected:
  T _t;
  boost::function< void (T const &) > _onChangeCallbackFunction;

 private:
  /** FIXME: How should the copy constructor behave?:
   *  \li Copy everything, incl. the callback function
   *  \li Copy only the data content and start with an empty callback function
   *  \li No copying allowed (private, intentionally not implemented)
   */
  ProcessVariable( ProcessVariable const & );

 public:
  ProcessVariable(){}
  ProcessVariable(T const & t) : _t(t){}

  /** Provide a function which is called when the variable has changed.
   *  Note the signature intentionally is 'void (T)' and not 'void (T const &)' to 
   *  avoid variable lifetime problems.
   */
  void setOnChangeCallbackFunction( boost::function< void (T const &) > onChangeCallbackFunction){
    _onChangeCallbackFunction = onChangeCallbackFunction;
  }
  
  void clearOnChangeCallbackFunction(){
    _onChangeCallbackFunction.clear();
  }
  
  ProcessVariable<T> & operator=(ProcessVariable<T> const & other){
    // avoid self asignment
    if (this == &other){
      return *this;
    }

    // use the type T assignment to avoid code duplication
    return (*this = other._t);
  }

  ProcessVariable<T> & operator=(T const & t){
    // do not trigger the callback if nothing has changed
    if( _t == t ){
      return *this;
    }

    // Only asign the template class, but not the callback function.
    _t = t;
 
    // Execute the callback function instead. 
    if (_onChangeCallbackFunction){
      _onChangeCallbackFunction(_t);
    }

    return *this;
  }

  /** Convenience function which can be stored as a function pointer 
   *  (in contrast to the asignment operator).
   */
  void set(ProcessVariable<T> const & other){
    *this = other;
  }

  /** Not strictly needed but faster than set(ProcessVariable<T> const & other) 
   *  called with T because there is no contructor involved.
   */
  void set(T const & t){
    // use the asignment operator which also does the callback and checks for change
    *this = t;
  }

  /** Asign the content variable without triggering the callback function.
   */
  void setWithoutCallback(ProcessVariable<T> const & other){
    // avoid self asignment
    if (this == &other){
      return;
    }

    _t = other._t;
  }

  /** Asign a value without triggering the callback function.
   */
  void setWithoutCallback(T const & t){
    _t = t;
  }

  operator T const & () const {
    return _t;
  }

};

}//namespace mtca4u

#endif// MTCA4U_PROCESS_VARIABLE_H
