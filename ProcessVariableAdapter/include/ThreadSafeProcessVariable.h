#ifndef MTCA4U_THREAD_SAFE_PROCESS_VARIABLE_H
#define MTCA4U_THREAD_SAFE_PROCESS_VARIABLE_H

#include <boost/thread/mutex.hpp>
#include <boost/function.hpp>

namespace mtca4u{

  /** This is only a non-operational draft!!!
   */
template<class T>
class ThreadSafeProcessVariable{
 protected:
  T _t;
  boost::mutex _mutex;
  boost::function< void (t) > _onChangeCallbackFunction;

 private:
  /** FIXME: How should the copy constructor behave?:
   *  \li Copy everything, incl. the callback function
   *  \li Copy only the data content and start with an empty callback function
   *  \li No copying allowed (private, intentionally not implemented)
   */
  ThreadSafeProcessVariable( ThreadSafeProcessVariable const & );

 public:
  ThreadSafeProcessVariable(){}
  ThreadSafeProcessVariable(T const & t) : _t(t){}

  /** Provide a function which is called when the variable has changed.
   *  Note the signature intentionally is 'void (T)' and not 'void (T const &)' to 
   *  avoid variable lifetime problems.
   */
  setOnChangeCallbackFunction( boost::function< void (T) > onChangeCallbackFunction);
  
  clearOnChangeCallbackFunction();
  
  ThreadSafeProcessVariable<T> & operator=(const ThreadSafeProcessVariable<T> & other){
    // avoid self asignment
    if (this == &other){
      return *this;
    }

    boost::mutex::scoped_lock lock(_mutex);
    if( _t == other._t ){
      return *this;
    }

    // Only asign the template class, but not the callback function.
    _t = other._t;
 
    // Execute the callback function instead. 
    // Unfortunately this has to be done while holding the mutex
    // to ensure the right state ends up at the client.
    if (setOnChangeCallbackFunction){
      _onChangeCallbackFunction;
    }
  }

  void set(ThreadSafeProcessVariable<T> const & other){
    *this = other;
  }

  void set(T const & t){
    *this = t;
  }

  void setWithoutCallback(const ThreadSafeProcessVariable<T> & other){
    // avoid self asignment
    if (this == &other){
      return;
    }

    boost::mutex::scoped_lock lock(_mutex);
    if( _t == other._t ){
      return;
    }

    // Only asign the template class, but not the callback function.
    _t = other._t;
  }

  bool operator==(T const & t) const {
    boost::mutex::scoped_lock lock(_mutex);
    return (_t==t);
  }
  
};

}//namespace mtca4u

#endif// MTCA4U_THREAD_SAFE_PROCESS_VARIABLE_H
