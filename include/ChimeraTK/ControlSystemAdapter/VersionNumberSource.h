#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_VERSION_NUMBER_SOURCE_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_VERSION_NUMBER_SOURCE_H

#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>

#include "VersionNumber.h"

namespace ChimeraTK {

  /**
   * Source generating version numbers. Version numbers are used to resolve
   * competing updates that are applied to the same process variable. For
   * example, it they can help in breaking an infinite update loop that might
   * occur when two process variables are related and update each other.
   *
   * Each version number returned by the same source is unique, but different
   * sources may return the same version numbers.
   */
  class VersionNumberSource {

  public:
    /**
     * Shared pointer to this type.
     */
    typedef boost::shared_ptr<VersionNumberSource> SharedPtr;

    /**
     * Returns the next version number. The next version number is determined in
     * an atomic way, so that it is guaranteed that this method never returns
     * the same version number twice (unless the counter overflows, which is
     * very unlikely). The first version number returned by this method is one.
     * The version number that is returned is guaranteed to be greater than the
     * version numbers returned for earlier calls to this method. This method
     * may safely be called by any thread without any synchronization.
     */
    inline VersionNumber nextVersionNumber() {
      VersionNumber currentVersionNumber = _lastReturnedVersionNumber.load(
          boost::memory_order_acquire);
      VersionNumber nextVersionNumber = currentVersionNumber + 1;
      while (!_lastReturnedVersionNumber.compare_exchange_weak(
          currentVersionNumber, nextVersionNumber, boost::memory_order_acq_rel,
          boost::memory_order_acquire)) {
        nextVersionNumber = currentVersionNumber + 1;
      }
      return nextVersionNumber;
    }

  private:
    /**
     * Last version number that was returned by a call to
     * {@link nextVersionNumber()}.
     */
    boost::atomic<VersionNumber> _lastReturnedVersionNumber;

  };

}

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_VERSION_NUMBER_SOURCE_H
