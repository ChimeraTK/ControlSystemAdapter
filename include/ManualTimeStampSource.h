#ifndef MTCA4U_MANUAL_TIME_STAMP_SOURCE_H
#define MTCA4U_MANUAL_TIME_STAMP_SOURCE_H

namespace ChimeraTK {

  /** The code manually has to set a time stamp, which is then disctributed
   *  to the connected PVs.
   */
  class ManualTimeStampSource : public TimeStampSource{
  public:
    void setTimeStamp( TimeStamp timeStamp ){
      _timeStamp = timeStamp;
    }

    TimeStamp getCurrentTimeStamp(){
      return _timeStamp;
    }

  protected:
    TimeStamp _timeStamp;
  };
}

#endif // MTCA4U_MANUAL_TIME_STAMP_SOURCE_H
