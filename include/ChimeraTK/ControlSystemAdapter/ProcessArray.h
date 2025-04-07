#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H

#include "PersistentDataStorage.h"

#include <ChimeraTK/NDRegisterAccessor.h>
#include <ChimeraTK/VersionNumber.h>

#include <boost/shared_ptr.hpp>

#include <limits>
#include <stdexcept>
#include <typeinfo>
#include <utility>
#include <vector>

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
    using SharedPtr = boost::shared_ptr<ProcessArray>;

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
    ProcessArray(InstanceType instanceType, const ChimeraTK::RegisterPath& name, const std::string& unit,
        const std::string& description, const AccessModeFlags& flags);

    ~ProcessArray() override;

    [[nodiscard]] bool isReadable() const override {
      return _instanceType == RECEIVER || _instanceType == SENDER_RECEIVER;
    }

    [[nodiscard]] bool isWriteable() const override {
      return _instanceType == SENDER || _instanceType == SENDER_RECEIVER;
    }

    [[nodiscard]] bool isReadOnly() const override { return !isWriteable(); }

    /** Return a unique ID of this process variable, which will be indentical for
     * the receiver and sender side of the same variable but different for any
     * other process variable within the same process. The unique ID will not be
     *  persistent accross executions of the process. */
    [[nodiscard]] virtual size_t getUniqueId() const = 0;

    [[nodiscard]] const std::type_info& getValueType() const override { return typeid(T); }

    [[nodiscard]] bool mayReplaceOther(const boost::shared_ptr<const ChimeraTK::TransferElement>&) const override {
      return false; // never true as we shall return false if instance is the same
    }

    std::vector<boost::shared_ptr<ChimeraTK::TransferElement>> getHardwareAccessingElements() override {
      return {boost::enable_shared_from_this<ChimeraTK::TransferElement>::shared_from_this()};
    }

    std::list<boost::shared_ptr<ChimeraTK::TransferElement>> getInternalElements() override { return {}; }

    void replaceTransferElement(boost::shared_ptr<ChimeraTK::TransferElement>) override {
      // You can't replace anything here. Just do nothing.
    }

   protected:
    /**
     * Type this instance is representing.
     */
    InstanceType _instanceType;
  };

  /********************************************************************************************************************/
  /*** Implementations of member functions below this line ************************************************************/
  /********************************************************************************************************************/

  template<class T>
  ProcessArray<T>::ProcessArray(InstanceType instanceType, const ChimeraTK::RegisterPath& name, const std::string& unit,
      const std::string& description, const AccessModeFlags& flags)
  : ChimeraTK::NDRegisterAccessor<T>(name, flags, unit, description), _instanceType(instanceType) {
    flags.checkForUnknownFlags({AccessMode::wait_for_new_data});
  }

  /********************************************************************************************************************/

  template<class T>
  ProcessArray<T>::~ProcessArray() = default;

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
