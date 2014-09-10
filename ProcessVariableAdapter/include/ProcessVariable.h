#ifndef MTCA4U_PROCESS_VARIABLE_H
#define MTCA4U_PROCESS_VARIABLE_H

#include <boost/function.hpp>

namespace mtca4u{

/** The ProvessVariable class provides an interface to acces a control system variable holding a simple data type.
 *  It contains two types of accessors (getters and setters). The get() and set() functions
 *  can trigger a callback functions. They are intended to be called if the variable is
 *  updated or read by the control system.
 *  The second set of accessors is intended for use in by the business logic and does not
 *  trigger the callback. This is needed to avoid endless loops and efficient updates.
 *
 *  This class and its implementations are not guaranteed to be thread safe. Thread safety 
 *  has to be implemented in the adapter implementation so the ProcessVariables can be used safey in the 
 *  constol system function callbacks for synchronous and asynchronous update.
 */
template<class T>
class ProcessVariable{
 private:
  /** The copy constructor is intentionally private and not implemented.
   *  The derrived class shall not implement a copy constructor. Like this
   *  it is assured that a compiler error is raised if a copy constructor is
   *  used somewhere.
   */
  ProcessVariable(ProcessVariable<T> const &);

 protected:
  /** A default constructor has to be specified if a copy constructor is specified. But why?
   */
  ProcessVariable(){};

 public:
  /** Register a function which is called when the set() function is executed.
   *  The signature of this function contains the new and the old value and is
   *  executed whenever set() is called, even if the new and the old value are the
   *  same. Like this function can decide whether its body is executed even 
   *  if the value has not changed.
   */
  virtual void setOnSetCallbackFunction( 
     boost::function< void (T const & /*newValue*/, T const & /*oldValue*/) > onSetCallbackFunction)=0;

  /** Register a function which is called when the get() function is executed.
   */
  virtual void setOnGetCallbackFunction( boost::function< T () > onGetCallbackFunction )=0;
  
  /** Clear the callback function for the set() method.
   */  
  virtual void clearOnSetCallbackFunction()=0;

  /** Clear the callback function for the get() method.
   */  
  virtual void clearOnGetCallbackFunction()=0;
  
  /** Assign the content of another process variable of type T to this one.
   *  It only assigns the variable content, but not the callback functions.
   *  This operator behaves like setWithoutCallback() and does not trigger 
   *  the "on set" callback function.
   */
  virtual ProcessVariable<T> & operator=(ProcessVariable<T> const & other)=0;

  /** Assign the content of the template type.
   *  It only assigns the variable content, but not the callback functions.
   *  This operator behaves like setWithoutCallback() and does not trigger 
   *  the "on set" callback function.
   */
  virtual ProcessVariable<T> & operator=(T const & t)=0;
 
  /** Set the content to that of the other process variable.
   *  This method trigger the "on set" callback function.
   */
  virtual void set(ProcessVariable<T> const & other)=0;

  /** Set the content to the content of the template type.
   *  This method trigger the "on set" callback function.
   */  
  virtual void set(T const & t)=0;

  /** Set the content to that of the other process variable.
   *  This method doen not trigger any callback functions.
   */
  virtual void setWithoutCallback(ProcessVariable<T> const & other)=0;
  
  /** Set the content to  to the content of the template type.
   *  This method doen not trigger any callback functions.
   */
  virtual void setWithoutCallback(T const & t)=0;
  
  /** Automatic conversion operator which returns a \b copy of the simple type.
   *  This means you cannot use this for assignment operations. You have to use
   *  the = operator or the setter functions.
   *  FIXME: Can this always be implemented?
   *  \code virtual operator T const & () const=0; \endcode
   *  For the time being we use the copying version as fallback solution.
   */
  virtual operator T () const=0;

  /** Get a copy of T. This method triggers the "on get" callback
   *  function before it returns the value. This might change 
   *  the internal state, which is why this function is not const.
   */
  virtual T get() =0;

  /** Get a copy of T without triggering a callback function.
   */
  virtual T getWithoutCallback() const=0;

  /** Every class with virtual functions should have a virtual destructor.
   */
  virtual ~ProcessVariable(){};
};

}//namespace mtca4u

#endif// MTCA4U_PROCESS_VARIABLE_H
