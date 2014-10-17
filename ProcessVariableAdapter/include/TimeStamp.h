#ifndef MTCA4U_TIMESTAMP_H
#define MTCA4U_TIMESTAMP_H

namespace mtca4u{
  
  /** The MTCA4U time stamp consists of the unix time stamp (number of seconds since 1.1.1970).
   *  The unsigned 32 bit value will have an overflow on Sun, 7 February 2106.
   *  If you need higher resolution than once per second, the nanoseconds field can be used.
   *  In addition two 32 bit user indices are provided, which can hold for instance 
   *  a run and an event number.
   */ 
  struct TimeStamp{
    /** The constructor can set all parameters. Only the seconds have to be specified, all other
     *  values default to 0.
     */
    inline TimeStamp(uint32_t seconds_, uint32_t nanoSeconds_=0,
		     uint32_t index0_=0, uint32_t index1_=0)
      : seconds(seconds_), nanoSeconds(nanoSeconds_),
	index0(index0_), index1(index1_){}
    uint32_t seconds; ///< Unix time in seconds
    uint32_t nanoSeconds; ///< Nano seconds should give enough resolution
    uint32_t index0; ///< An index to hold a unique number, for instance an event number.
    uint32_t index1; ///< Another index to hold a unique number, for instance a run number.

    inline bool operator==(TimeStamp const & right){
      return ( (seconds==right.seconds) &&
	       (nanoSeconds==right.nanoSeconds) &&
	       (index0==right.index0)&&
	       (index1==right.index1) );	       
    }
  };
}

#endif// MTCA4U_TIMESTAMP_H
