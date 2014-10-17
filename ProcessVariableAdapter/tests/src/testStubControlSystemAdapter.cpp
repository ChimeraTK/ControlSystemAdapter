#include <sstream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "StubControlSystemAdapter.h"
using namespace mtca4u;

/** Test class for the control system adapter stub.
 *  \li Fixme: it does not test the thread safety.
 *  \li Fixme: can this be done as a control system independent test?
 */
class StubControlSystemAdapterTest
{
 public:
  void testPeriodicSyncFunction();
  void testTriggeredSyncFunction();
  StubControlSystemAdapterTest();

 private:
  StubControlSystemAdapter _controlSystemAdapter;
  unsigned int _callbackCounter;
  TimeStamp _lastCallbacksTimeStamp;

  void increaseCounter(TimeStamp const & timeStamp);
};

class StubControlSystemAdapterTestSuite : public test_suite {
public:
  StubControlSystemAdapterTestSuite(): test_suite("StubControlSystemAdapter test suite") {
    // create an instance of the test class
    boost::shared_ptr<StubControlSystemAdapterTest> stubControlSystemAdapterTest(
      new StubControlSystemAdapterTest );

    add( BOOST_CLASS_TEST_CASE( &StubControlSystemAdapterTest::testPeriodicSyncFunction,
				stubControlSystemAdapterTest ));
    add( BOOST_CLASS_TEST_CASE( &StubControlSystemAdapterTest::testTriggeredSyncFunction,
				stubControlSystemAdapterTest ));
  }
};

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "StubControlSystemAdapter test suite";
  return new StubControlSystemAdapterTestSuite;
}

void StubControlSystemAdapterTest::testPeriodicSyncFunction(){
  _controlSystemAdapter.registerPeriodicSyncFunction( 
     boost::bind( &StubControlSystemAdapterTest::increaseCounter, this, _1 ) );
  TimeStamp referenceTimeStamp( 123, 0, 42 );
  _controlSystemAdapter.executePeriodicSyncFunction( referenceTimeStamp );
  BOOST_CHECK( _callbackCounter == 1 );
  BOOST_CHECK( _lastCallbacksTimeStamp == referenceTimeStamp );

  _controlSystemAdapter.clearPeriodicSyncFunction();
  _controlSystemAdapter.executePeriodicSyncFunction( TimeStamp(2,3,4,5) );
  BOOST_CHECK( _callbackCounter == 1 );
  BOOST_CHECK( _lastCallbacksTimeStamp == referenceTimeStamp );  
}

void StubControlSystemAdapterTest::testTriggeredSyncFunction(){
  _controlSystemAdapter.registerTriggeredSyncFunction( 
     boost::bind( &StubControlSystemAdapterTest::increaseCounter, this, _1 ) );
  TimeStamp referenceTimeStamp( 321, 20, 242 );
  _controlSystemAdapter.executeTriggeredSyncFunction( referenceTimeStamp );
  BOOST_CHECK( _callbackCounter == 2 );
  BOOST_CHECK( _lastCallbacksTimeStamp == referenceTimeStamp );

  _controlSystemAdapter.clearTriggeredSyncFunction();
  _controlSystemAdapter.executeTriggeredSyncFunction( TimeStamp(2,3,4,5) );
  BOOST_CHECK( _callbackCounter == 2 );
  BOOST_CHECK( _lastCallbacksTimeStamp == referenceTimeStamp );  
}

StubControlSystemAdapterTest::StubControlSystemAdapterTest()
  : _callbackCounter(0), _lastCallbacksTimeStamp(0)
{}

void StubControlSystemAdapterTest::increaseCounter(TimeStamp const & timeStamp){
  ++_callbackCounter;
  _lastCallbacksTimeStamp = timeStamp;
}

