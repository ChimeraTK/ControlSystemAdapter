#ifndef MTCA4U_TIME_STAMP_SOURCE_H
#define MTCA4U_TIME_STAMP_SOURCE_H

#include <boost/shared_ptr.hpp>

#include "TimeStamp.h"

namespace ChimeraTK {

  /**
   * Interface for a time-stamp source. A time-stamp source is used by a process
   * variable to get the current time and update its time-stamp when its value
   * is changed.
   */
  class TimeStampSource {

  public:
    /**
     * Shared pointer to this type.
     */
    typedef boost::shared_ptr<TimeStampSource> SharedPtr;

    /**
     * Returns the current time-stamp. The definition of what the current time-
     * stamp is (e.g. system time, external reference time), depends on the
     * implementation.
     */
    virtual TimeStamp getCurrentTimeStamp() =0;

  protected:
    /**
     * Destructor. The destructor is protected so that class implementing this
     * interface cannot be destroyed through pointers to this interface.
     */
    virtual ~TimeStampSource() {
    }

  };

}

#endif // MTCA4U_TIME_STAMP_SOURCE_H
