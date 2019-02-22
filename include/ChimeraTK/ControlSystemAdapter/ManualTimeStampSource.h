#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_MANUAL_TIME_STAMP_SOURCE_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_MANUAL_TIME_STAMP_SOURCE_H

namespace ChimeraTK {

  /** The code manually has to set a time stamp, which is then disctributed
   *  to the connected PVs.
   */
  class ManualTimeStampSource : public TimeStampSource {
   public:
    void setTimeStamp(TimeStamp timeStamp) { _timeStamp = timeStamp; }

    TimeStamp getCurrentTimeStamp() { return _timeStamp; }

   protected:
    TimeStamp _timeStamp;
  };
} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_MANUAL_TIME_STAMP_SOURCE_H
