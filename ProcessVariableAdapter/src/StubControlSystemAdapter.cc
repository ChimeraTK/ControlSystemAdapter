#include "StubControlSystemAdapter.h"

#include <stdexcept>

namespace mtca4u{
  typedef std::runtime_error NotImplementedException;

  void StubControlSystemAdapter::registerPeriodicSyncFunction(
    boost::function< void( TimeStamp const & ) > syncFunction){
    _periodicSyncFunction = syncFunction;
  }

  void StubControlSystemAdapter::clearPeriodicSyncFunction(){
    _periodicSyncFunction.clear();
  }

  void StubControlSystemAdapter::executePeriodicSyncFunction( TimeStamp const & timeStamp){
    if(_periodicSyncFunction){
      _periodicSyncFunction(timeStamp);
    }
  }

  void StubControlSystemAdapter::registerTriggeredSyncFunction(
    boost::function< void( TimeStamp const & ) > syncFunction){
    _triggeredSyncFunction = syncFunction;
  }

  void StubControlSystemAdapter::clearTriggeredSyncFunction(){
    _triggeredSyncFunction.clear();
  }

  void StubControlSystemAdapter::executeTriggeredSyncFunction( TimeStamp const & timeStamp ){
    if(_triggeredSyncFunction){
      _triggeredSyncFunction(timeStamp);
    }
  }

  void StubControlSystemAdapter::executeUserSyncFunction( boost::function< void() > syncFunction){
    if(syncFunction){
      syncFunction();
    }
  }

}//namespace mtca4u
