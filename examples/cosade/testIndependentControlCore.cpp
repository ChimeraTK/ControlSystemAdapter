// test the IndependentControlCore without DOOCS

#define BOOST_TEST_MODULE IndependentControlCoreTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include <ChimeraTK/ControlSystemAdapter/ControlSystemSynchronizationUtility.h>

#include "IndependentControlCore.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE( IndependentControlCoreTestSuite )

BOOST_AUTO_TEST_CASE( independentControlCoreTest ){
  std::pair<boost::shared_ptr<ControlSystemPVManager>,
	    boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
  boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
  boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

  IndependentControlCore controlCore(devManager);

  ControlSystemSynchronizationUtility syncUtil(csManager);

  ProcessScalar<int>::SharedPtr targetVoltage 
    = csManager->getProcessScalar<int>("TARGET_VOLTAGE");
  ProcessScalar<int>::SharedPtr monitorVoltage 
    = csManager->getProcessScalar<int>("MONITOR_VOLTAGE");

  // start with -1 for both voltages
  *targetVoltage = -1;
  *monitorVoltage = -1;

  // the other side has a timeout of 100 ms, so we wait 1 second to be safe,
  // but we look every 10 ms (not too frequent, but only with 10 % of additional latency)
  //syncUtil.waitForNotifications(1000000, 10000);
  //Note: as waitForNotification does not work as expected, the functionality is
  // done manually for just the one variable we are receiving.
  for (size_t i=0; i<  100; ++i){
    boost::this_thread::sleep_for( boost::chrono::milliseconds(10) );
    if (monitorVoltage->receive()){
      break;
    }
  }

  // the target voltage must not have changed
  BOOST_CHECK(  *targetVoltage == -1 );
  // the monitor voltage has to be 0 now, like in the hardware
  BOOST_CHECK(  *monitorVoltage == 0 );

  *targetVoltage = 42;
  targetVoltage->send();

  // Wait until we read something back and check that the expected value is there.
  // This is not nevessarily the case on the first read, so we put a limit to 10.
  // Note that the number of actual receive attempty is higher, because we 
  // read every 10 ms while the other side is sending every 100 ms.
  for (size_t receiveCounter=0; receiveCounter<  10; /*empty*/){
    boost::this_thread::sleep_for( boost::chrono::milliseconds(10) );
    if (monitorVoltage->receive()){
      ++receiveCounter;
      if (*monitorVoltage == 42){
	break;
      }
    }
  }

  BOOST_CHECK(*monitorVoltage == 42);
  
}

BOOST_AUTO_TEST_SUITE_END()
