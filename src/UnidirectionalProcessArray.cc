#include "ProcessArray.h"

namespace ChimeraTK {
  namespace detail {
    std::atomic<bool> processArrayEnableThreadSafetyCheck;
  } // namespace detail
  void setEnableProcessArrayThreadSafetyCheck(bool enable) {
    detail::processArrayEnableThreadSafetyCheck = enable;
  }
} // namespace ChimeraTK
