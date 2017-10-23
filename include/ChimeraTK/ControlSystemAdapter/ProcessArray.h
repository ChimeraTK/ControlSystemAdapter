#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H

#include <vector>
#include <utility>
#include <limits>
#include <stdexcept>
#include <typeinfo>

#include <boost/smart_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/future.hpp>

#include <mtca4u/NDRegisterAccessor.h>
#include <mtca4u/VersionNumber.h>

#include "ProcessVariableListener.h"
#include "TimeStampSource.h"
#include "PersistentDataStorage.h"

namespace ChimeraTK {
  
  /**
   * Array version of the ProcessVariable. This class mainly exists for
   * historical reasons: Originally, there were different implementations for
   * scalars and arrays. Now, only arrays exist, so all ProcessVariables are
   * ProcessArrays.
   *
   * This class is not thread-safe and should only be used from a single thread.
   *
   * TODO This comment might not belong here:
   * If no version number source is specified when creating an instance of this
   * class, version number checks are disabled. This means that a receive
   * operation will always proceed, regardless of the version number of the new
   * value. The version number of the process array without a version number
   * source will always stay at zero.
   */
  template<class T>
  class ProcessArray : public mtca4u::NDRegisterAccessor<T> {

  public:

    /**
     * Type alias for a shared pointer to this type.
     */
    typedef boost::shared_ptr<ProcessArray> SharedPtr;

    /**
     * Type of the instance. This defines the behavior (send or receive
     * possible, modifications allowed, etc.). This enum type is private so
     * that only friend functions can construct instances of this class. The
     * constructors themselves have to be public so that boost::make_shared
     * can be used.
     */
    enum InstanceType {

      /**
       * Instance acts as the sender in a sender / receiver pair.
       */
      SENDER,

      /**
       * Instance acts as the receiver in a sender / receiver pair.
       */
      RECEIVER,

      /**
       * Instance acts as both a sender and a receiver. This is only true for
       * a process array that is part of a bidirectional pair.
       */
      SENDER_RECEIVER

    };

    /**
     * Creates a process array of the specified type.
     */
    ProcessArray(InstanceType instanceType, const mtca4u::RegisterPath& name,
        const std::string &unit, const std::string &description);

    virtual ~ProcessArray();

    bool isReadable() const override {
      return _instanceType == RECEIVER || _instanceType == SENDER_RECEIVER;
    }

    bool isWriteable() const override {
      return _instanceType == SENDER || _instanceType == SENDER_RECEIVER;
    }

    bool isReadOnly() const override {
      return !isWriteable();
    }
  
    /**
     * Sends the current value to the receiver. Returns <code>true</code> if an
     * empty buffer was available and <code>false</code> if no empty buffer was
     * available and thus a previously sent value has been dropped in order to
     * send the current value.
     *
     * The specified version number is passed to the receiver. If the receiver
     * has a value with a version number greater than or equal to the specified
     * version number, it silently discards this update.
     *
     * This version of the send operation moves the current value from the
     * sender to the receiver without copying it. This means that after calling
     * this method, the sender's value, time stamp, and version number are
     * undefined. Therefore, this method must only be used if this process
     * variable is not read (on the sender side) after sending it.
     *
     * Throws an exception if this process variable is not a sender or if this
     * process variable does not allow destructive sending.
     */
    virtual bool writeDestructively(ChimeraTK::VersionNumber versionNumber={}) = 0; /// @todo FIXME this function must be present in TransferElement already!

    const std::type_info& getValueType() const override {
      return typeid(T);
    }

    bool isArray() const override {
      return true;
    }

    bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const override {
      // only true if the very instance of the transfer element is the same
      return e.get() == this;
    }

    std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
      return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() };
    }
    
    void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {
      // You can't replace anything here. Just do nothing.
    }

  private:

    /**
     * Type this instance is representing.
     */
    InstanceType _instanceType;

  };

/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T>
  ProcessArray<T>::ProcessArray(InstanceType instanceType,
      const mtca4u::RegisterPath& name, const std::string &unit,
      const std::string &description) :
      mtca4u::NDRegisterAccessor<T>(name, unit, description), _instanceType(
          instanceType) {
  }

/*********************************************************************************************************************/

  template<class T>
  ProcessArray<T>::~ProcessArray() {
  }

} // namespace ChimeraTK

// We include the UnidirectionalProcessArray.h header so that code using the
// create... functions does not have to do so explicitly. This include must be
// after the declaration of ProcessArray because the code in
// UnidirectionalProcessArray.h depends on it.
#include "UnidirectionalProcessArray.h"

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
