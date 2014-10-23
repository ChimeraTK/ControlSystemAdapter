#ifndef MTCA4U_STUB_CONTROL_SYSTEM_ADAPTER
#define MTCA4U_STUB_CONTROL_SYSTEM_ADAPTER

#include "ControlSystemAdapter.h"

namespace mtca4u{

  /** The stub implementation of the ControlSystemAdapter.
   *  \fixme It uses a mutex to ensure thread safety. But how to test it?
   */
  class StubControlSystemAdapter : public ControlSystemAdapter{
  public:

    void registerPeriodicSyncFunction( 	boost::function< void( TimeStamp const & ) > );
    void clearPeriodicSyncFunction();
    /** The stub also has a function to execute the PeriodicSyncFunction, to 
     *  be used by tests or demonstator software. This does not necessarily have to be 
     *  preiodic for testing purposes.
     */
    void executePeriodicSyncFunction( TimeStamp const & );

    void registerTriggeredSyncFunction( boost::function< void( TimeStamp const & ) > );
    void clearTriggeredSyncFunction();
    /** The stub also has a function to execute the TriggeredSyncFunction, to 
     *  be used by tests or demonstator software.
     */
    void executeTriggeredSyncFunction( TimeStamp const & );
   
    void executeUserSyncFunction( boost::function< void() > );

  private:
    boost::function< void( TimeStamp const & ) > _periodicSyncFunction;
    boost::function< void( TimeStamp const & ) > _triggeredSyncFunction;
  };

}//namespace mtca4u

#endif// MTCA4U_CONTROL_SYSTEM_ADAPTER
