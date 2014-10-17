#ifndef MTCA4U_CONTROL_SYSTEM_ADAPTER
#define MTCA4U_CONTROL_SYSTEM_ADAPTER

#include <boost/function.hpp>
#include "TimeStamp.h"

namespace mtca4u{

  /** The ControlSystemAdapter allows to register callback functions
   *  which are executed to synchronise ProcessVariables between 
   *  the business logic and the control system.
   *  
   *  There are two types of callbacks.
   *  \li Functions that are triggered by the control system layer.
   *  \li Functions that are triggered by the business logic.
   *
   *  The main reason why the synchronisation functions have to be 
   *  executed as callbacks is thread safety. The adapter guarantees that
   *  is is safe to access the ProcessVariables inside the call back functions.
   *
   *  The functions which are triggered by the control system give
   *  a time stamp to the user code. Their execution is sheduled by the
   *  control system layer. There is one function for periodic sychronisation,
   *  and one function which is intended to be executed on external software
   *  triggers which are send by the control system. The implementation  details
   *  when the functions are executed depend on the adapter implementation.
   *
   *  The functions which are triggered by the business logic have no parameters.
   *  They are executed by calling the executeUserSyncFunction1() and 
   *  executeUserSyncFunction2() functions
   *  of the ControlSystemAdapter instead of calling them directly. The reason is 
   *  that some control systems require to hold a global lock while accessing 
   *  process variables, but this is implementation dependent. The adapter takes
   *  care of these implementation details and makes sure that it is (thread)
   *  safe to access the process variables inside the syncronisation callback function.
   */
  class ControlSystemAdapter{
  public:

    /** Register a function to be executed periodically by the control system.
     */
    virtual void registerPeriodicSyncFunction( 
	boost::function< void( TimeStamp const & ) > ) = 0;

    /** Clear the synchronisation function for periodic updates.
     */
    virtual void clearPeriodicSyncFunction()=0;

    /** Register a function to be executed by the control system upon 
     *  a signal or software trigger.
     */
    virtual void registerTriggeredSyncFunction( 
	boost::function< void( TimeStamp const & ) > ) = 0;
					      
    /** Clear the synchronisation function for periodic updates.
     */
    virtual void clearTriggeredSyncFunction()=0;
   
    /** Register a function which is triggered by the user logic.
     */
    virtual void registerUserSyncFunction1( boost::function< void() > ) = 0;

    /** Clear the first user sync function.
     */
    virtual void clearUserSyncFunction1()=0;

    /** Execute the first user sync function.
     */
    virtual void executeUserSyncFunction1()=0;

    /** Register a second function which is triggered by the user logic.
     */
    virtual void registerUserSyncFunction2( boost::function< void() > ) = 0;


    /** Clear the second user sync function.
     */
    virtual void clearUserSyncFunction2()=0;


    /** Execute the second user sync function.
     */
    virtual void executeUserSyncFunction2()=0;
  };

}//namespace mtca4u

#endif// MTCA4U_CONTROL_SYSTEM_ADAPTER
