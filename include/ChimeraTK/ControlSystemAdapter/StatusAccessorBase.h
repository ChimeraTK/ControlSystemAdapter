#pragma once

#include <cstdint>
#include <string>
#include <assert.h>

namespace ChimeraTK {

  /** Base class - used to avoid code duplication in StatusOutput, StatusPushInput and StatusPollInput. */
  struct StatusAccessorBase {
    /**
     *  These are the states which can be reported.
     *  
     *  Note: The values are exposed to the control system and hence are part of the public interface!
     */
    enum class Status : int32_t { OK = 0, FAULT = 1, OFF = 2, WARNING = 3 };

    static std::string statusToString(Status statusCode) {
      std::string statusString;
      switch(statusCode) {
        case StatusAccessorBase::Status::OK:
          statusString = "OK";
          break;
        case StatusAccessorBase::Status::FAULT:
          statusString = "FAULT";
          break;
        case StatusAccessorBase::Status::OFF:
          statusString = "OFF";
          break;
        case StatusAccessorBase::Status::WARNING:
          statusString = "WARNING";
          break;
        default:
          assert(false);
      }
      return statusString;
    }
  };

} // namespace ChimeraTK
