#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_VARIABLE_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_VARIABLE_H

namespace ChimeraTK {
  class ProcessVariable;
}

#include <string>
#include <typeinfo>

#include <boost/smart_ptr.hpp>

#include "TimeStampSource.h"

#include "ProcessVariableDecl.h"

namespace ChimeraTK {

  /**
   * Base class for all {@link ProcessScalar}s and {@link ProcessArray}s.
   * This class provides methods that are independent of the actual type or
   * implementation of a process variable.
   */
  class ProcessVariable: public boost::enable_shared_from_this<ProcessVariable> {
  public:
    /**
     * Type alias for a shared pointer to a process variable.
     */
    typedef boost::shared_ptr<ProcessVariable> SharedPtr;

    /**
     * Returns the name that identifies the process variable.
     */
    const std::string& getName() const;

    /**
     * Returns the \c std::type_info for the value type of this process
     * variable or array. This can be used to determine the type of a process
     * variable at runtime.
     */
    virtual const std::type_info& getValueType() const =0;

    /**
     * Returns <code>true</code> if this object is an instance of
     * {@link ProcessArray}\<T\> and <code>false</code> if this object is an
     * instance of {@link ProcessScalar}\<T\>.
     */
    virtual bool isArray() const =0;

    /**
     * Returns <code>true</code> if this object represents the receiver in a
     * sender / receiver pair, <code>false</code> otherwise.
     */
    virtual bool isReceiver() const =0;

    /**
     * Returns <code>true</code> if this object represents the sender in a
     * sender / receiver pair, <code>false</code> otherwise.
     */
    virtual bool isSender() const =0;

    /**
     * Returns the time stamp associated with the current value of the process
     * variable. Typically, this is the time when the value was updated.
     */
    virtual TimeStamp getTimeStamp() const =0;

    /**
     * Receives a new value from the sender if this process variable is a
     * receiver. Returns <code>true</code> if a new value has been received and
     * <code>false</code> if there is no new value available.
     *
     * Throws an exception if this process variable is not a receiver.
     */
    virtual bool receive() =0;

    /**
     * Sends the current value to the receiver if this process variable is a
     * sender. Returns <code>true</code> if an empty buffer was available and
     * <code>false</code> if no empty buffer was available and thus a previously
     * sent value has been dropped in order to send the current value.
     *
     * Throws an exception if this process variable is not a sender.
     */
    virtual bool send() =0;

  protected:
    /**
     * Creates a process variable with the specified name.
     */
    ProcessVariable(const std::string& name = std::string());

    /**
     * Virtual destructor. The destructor is protected so that an instance
     * cannot be destroyed through a pointer to this interface.
     */
    virtual ~ProcessVariable();

  private:
    /**
     * Identifier uniquely identifying the process variable.
     */
    std::string _name;

    // Theoretically, the copy constructor and copy assignment operator should
    // not be generated if a destructor is defined. However, some compilers do
    // not enforce this. By making the constructor and operator private, we
    // ensure that a process variable is never copied accidentally. This also
    // applies to derived classes (the automatically generated implementations
    // need the implementation in the parent class), so that it is sufficient
    // if we do it once here.
    ProcessVariable(const ProcessVariable&);
    ProcessVariable& operator=(const ProcessVariable&);

  };

}

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_VARIABLE_H
