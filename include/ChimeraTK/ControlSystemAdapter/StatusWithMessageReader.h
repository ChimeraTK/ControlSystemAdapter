#pragma once

#include <ChimeraTK/ForwardDeclarations.h>
#include <ChimeraTK/RegisterPath.h>
#include <ChimeraTK/ScalarRegisterAccessor.h>
#include <ChimeraTK/DataConsistencyGroup.h>

#include "StatusAccessorBase.h"

namespace ChimeraTK {

  /// only for code deduplication, not for direct use. Used by StatusWithMessageReader and StatusWithMessageInput
  template<class Derived>
  class StatusWithMessageReaderBase {
    Derived* derived() { return static_cast<Derived*>(this); };

   public:
    /// uses DataConsistencyGroup to determine wether status+string update is ready
    /// returns true if updatedId belongs to inputs and inputs are in consistent state
    bool update(TransferElementID updatedId) {
      // we wait until first call to update, with initialzation of our DataConsistencyGroup.
      // The reason is, at construction time Accessors are not ready.
      if(!_consistencyGroupInitialized) {
        _consistencyGroup.add(derived()->_status);
        if(hasMessageSource) _consistencyGroup.add(derived()->_message);
        _consistencyGroupInitialized = true;
      }

      // return false if updatedId does not belong to our variables
      if(updatedId != derived()->_status.getId() && (!hasMessageSource || updatedId != derived()->_message.getId())) {
        return false;
      }
      bool isConsistent = _consistencyGroup.update(updatedId);
      if(!isConsistent) {
        if(!_updated)
          // actually this should never happen..
          std::cout << "WARNING: Data loss when updating status code and message for " << derived()->_status.getName()
                    << std::endl;
        _updated = false;
        return isConsistent;
      }
      _updated = true;
      return isConsistent;
    }

    std::string getMessage() {
      if(hasMessageSource) {
        return derived()->_message;
      }
      else {
        int statusCode = derived()->_status;
        auto err = StatusAccessorBase::Status(statusCode);
        std::string statusString = StatusAccessorBase::statusToString(err);
        return _statusNameLong + " switched to " + statusString;
      }
    }
    bool hasMessageSource;

   protected:
    DataConsistencyGroup _consistencyGroup;
    bool _consistencyGroupInitialized = false;
    bool _updated = true; // for detecting data losses
    /// this should provide a name useful for identification in status message, e.g. fully qualified name
    std::string _statusNameLong;
  };

  /**
   * This is for consistent readout of StatusWithMessage - DeviceAccess version.
   *  It can be instantiated wih or without message string. 
   *  If instantiated without message, the message is generated automatically from the status.
   */
  struct StatusWithMessageReader : StatusWithMessageReaderBase<StatusWithMessageReader> {
    ChimeraTK::ScalarRegisterAccessor<int32_t> _status;
    ChimeraTK::ScalarRegisterAccessor<std::string> _message;

    StatusWithMessageReader(
        ChimeraTK::ScalarRegisterAccessor<int32_t> status, ChimeraTK::ScalarRegisterAccessor<std::string> message)
    : _status(status), _message(message) {
      hasMessageSource = true;
      _statusNameLong = _status.getName();
      // as a later improvement, it would make sense to set _statusNameLong as
      // the name mapped to the actual control system, here and also in StatusWithMessageInput
    }
    StatusWithMessageReader(ChimeraTK::ScalarRegisterAccessor<int32_t> status) : _status(status) {
      hasMessageSource = false;
      _statusNameLong = _status.getName();
    }
    void setMessageSource(ChimeraTK::ScalarRegisterAccessor<std::string> message) {
      _message.replace(message);
      hasMessageSource = true;
    }
  };
} /* namespace ChimeraTK */
