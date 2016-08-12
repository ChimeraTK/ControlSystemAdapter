#ifndef MTCA4U_COUNTING_TIME_STAMP_SOURCE_H
#define MTCA4U_COUNTING_TIME_STAMP_SOURCE_H

#include <TimeStampSource.h>

namespace ChimeraTK {

  /**
   * Simple time-stamp source that returns a time stamp that has all fields
   * except the index0 field set to zero. The index0 field is initialized with
   * a number that is incremented with each invocation.
   */
  struct CountingTimeStampSource: public TimeStampSource {

    int count;

    CountingTimeStampSource() :
        count(0) {
    }

    TimeStamp getCurrentTimeStamp() {
      return TimeStamp(0, 0, count++, 0);
    }

  };

} // namespace ChimeraTK

#endif // MTCA4U_COUNTING_TIME_STAMP_SOURCE_H
