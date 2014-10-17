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

  void StubControlSystemAdapter::registerUserSyncFunction1(
    boost::function< void() > ){
      throw NotImplementedException("registerUserSyncFunction1 is not implemented yet");
  }

  void StubControlSystemAdapter::clearUserSyncFunction1(){
    throw NotImplementedException("clearUserSyncFunction1 is not implemented yet");
  }

  void StubControlSystemAdapter::executeUserSyncFunction1(){
    throw NotImplementedException("executeUserSyncFunction1 is not implemented yet");
  }
 
  void StubControlSystemAdapter::registerUserSyncFunction2(
    boost::function< void() > ){
      throw NotImplementedException("registerUserSyncFunction2 is not implemented yet");
  }

  void StubControlSystemAdapter::clearUserSyncFunction2(){
    throw NotImplementedException("clearUserSyncFunction2 is not implemented yet");
  }

  void StubControlSystemAdapter::executeUserSyncFunction2(){
    throw NotImplementedException("executeUserSyncFunction2 is not implemented yet");
  }

}//namespace mtca4u
