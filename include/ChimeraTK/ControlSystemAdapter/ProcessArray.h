#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H

#include <vector>
#include <utility>
#include <limits>
#include <stdexcept>
#include <typeinfo>

#include <boost/shared_ptr.hpp>

#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/VersionNumber.h>

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
   */
  template<class T>
  class ProcessArray : public ChimeraTK::NDRegisterAccessor<T> {

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
    ProcessArray(InstanceType instanceType, const ChimeraTK::RegisterPath& name,
        const std::string &unit, const std::string &description, const AccessModeFlags &flags);

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

    /** Return a unique ID of this process variable, which will be indentical for the receiver and sender side of the
     *  same variable but different for any other process variable within the same process. The unique ID will not be
     *  persistent accross executions of the process. */
    virtual size_t getUniqueId() const = 0;

    const std::type_info& getValueType() const override {
      return typeid(T);
    }

    bool isArray() const override {
      return true;
    }

    bool mayReplaceOther(const boost::shared_ptr<const ChimeraTK::TransferElement>&) const override {
      return false;  // never true as we shall return false if instance is the same
    }

    std::vector<boost::shared_ptr<ChimeraTK::TransferElement> > getHardwareAccessingElements() override {
      return { boost::enable_shared_from_this<ChimeraTK::TransferElement>::shared_from_this() };
    }

    std::list<boost::shared_ptr<ChimeraTK::TransferElement>> getInternalElements() override {
      return {};
    }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement>) override {
      // You can't replace anything here. Just do nothing.
    }

    AccessModeFlags getAccessModeFlags() const override {
      return _flags;
    }

  protected:

    /**
     * Type this instance is representing.
     */
    InstanceType _instanceType;

    /// AccessModeFlags of this ProcessArray
    AccessModeFlags _flags;

  };

/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T>
  ProcessArray<T>::ProcessArray(InstanceType instanceType, const ChimeraTK::RegisterPath& name, const std::string &unit,
                                const std::string &description, const AccessModeFlags &flags)
    : ChimeraTK::NDRegisterAccessor<T>(name, unit, description),
      _instanceType(instanceType),
      _flags(flags)
  {}

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
// The BidirectionalProcessArray depends on the UnidirectionalProcessArray, so
// its header has to be included second.
#include "BidirectionalProcessArray.h"

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
