#ifndef MTCA4U_PROCESS_VARIABLE_ACCESSOR_H
#define MTCA4U_PROCESS_VARIABLE_ACCESSOR_H

#include "ProcessVariable.h"

namespace mtca4u {

  class ProcessVariableAccessor{
  public:
    /**
     * Returns the name that identifies the process variable.
     */
    const std::string& getName() const{
      return _impl->getName();
    }

    /**
     * Returns the {@link std::type_info} for the value type of this process
     * variable or array. This can be used to determine the type of a process
     * variable at runtime.
     */
    const std::type_info& getValueType() const{
      return _impl->getValueType();
    }

    /**
     * Returns <code>true</code> if the implementation is an instance of
     * {@link ProcessArray<T>} and <code>false</code> if this object is an
     * instance of {@link ProcessScalar<T>}.
     */
    bool isArray() const{
      return _impl->isArray();
    }

    /**
     * Returns <code>true</code> if this object represents the receiver in a
     * sender / receiver pair, <code>false</code> otherwise.
     */
    virtual bool isReceiver() const{
      return _impl->isReceiver();
    }

    /**
     * Returns <code>true</code> if this object represents the sender in a
     * sender / receiver pair, <code>false</code> otherwise.
     */
    bool isSender() const{
      return _impl->isSender();
    }

    /**
     * Returns the time stamp associated with the current value of the process
     * variable. Typically, this is the time when the value was updated.
     */
    TimeStamp getTimeStamp() const{
      return _impl->getTimeStamp();
    }

    /**
     * Receives a new value from the sender if this process variable is a
     * receiver. Returns <code>true</code> if a new value has been received and
     * <code>false</code> if there is no new value available.
     *
     * Throws an exception if this process variable is not a receiver.
     */
    bool receive(){
      return _impl->receive();
    }

    /**
     * Sends the current value to the receiver if this process variable is a
     * sender. Returns <code>true</code> if an empty buffer was available and
     * <code>false</code> if no empty buffer was available and thus a previously
     * sent value has been dropped in order to send the current value.
     *
     * Throws an exception if this process variable is not a sender.
     */
    bool send(){
      return _impl->send();
    }

    /**
     * Replaces the implementation that is used under the hood. Use this function instead of 
     * a assignment operator. The assignment operator was turned off to avoid confusion with
     * assigning the content of the other process variable, which is accessed by set.
     */
    void replace(ProcessVariableAccessor const & other){
      replaceImpl( other._impl );
    }

 private:
    
    /** The replace impl to avoid code duplication. Requires impl to be valid.
     */
    void replaceImpl(ProcessVariable::SharedPtr const & otherImpl){
      // current safety check
      // FIXME: Do we want this in future when the accessors childen
      // should work with all kinds of impl? Just one impl?
      if ( (_impl->isArray() != otherImpl->isArray() ) ||
	   (_impl->getValueType() != otherImpl->getValueType() ) ){
	throw std::runtime_error("ProcessVariableAccessors can only be replaced by accessors of the same type");
      }
      _impl = otherImpl;
    }

    /** 
     * Replace function which direktly takes a ProcessVariable (sharedPtr). Can be used
     * to initalise an accessor which has been created with the default constuctor of the child
     * (empty shared ptr).
     */
    void replace(ProcessVariable::SharedPtr const & impl){
      if (_impl){ // my current impl exists. replace with safety check
	replaceImpl( impl );
      }else{ // I don't have an impl yet, just assign it.
      _impl = impl;
      }
    }
    
  protected:
    ProcessVariable::SharedPtr _impl;

    /**
     */
    ProcessVariableAccessor(ProcessVariable::SharedPtr const & impl)
      : _impl(impl){      
      }

  private:
    /**
     * The assignment operator is intentionally turned off. Also derrieved classes cannot 
     * access it. It intentionally is not implemented. Use replace() instead to replace the impl,
     * and set() to replace the content.
     */
    ProcessVariableAccessor & operator=(ProcessVariableAccessor const & other);    
  };

} //namespace mtca4u

#endif // MTCA4U_PROCESS_VARIABLE_ACCESSOR_H
