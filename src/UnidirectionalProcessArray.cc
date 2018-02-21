#include "ProcessArray.h"

namespace ChimeraTK {
  namespace detail {
    std::atomic<bool> processArrayEnableThreadSafetyCheck;
  }
  void setEnableProcessArrayThreadSafetyCheck(bool enable) {
    detail::processArrayEnableThreadSafetyCheck = enable;
  }
}
