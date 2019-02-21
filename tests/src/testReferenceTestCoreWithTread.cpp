#define BOOST_TEST_MODULE ReferenceTestApplicationWithThreadTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ControlSystemSynchronizationUtility.h"
#include "ReferenceTestApplication.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

// We are testing the reference application when executed with it's own thread
// running. Synchronisation is done through the atomic
// mainBodyCompletelyExecuted variable.

// This test is only executed for int32_t scalars because it is only testing the
// thread functionality. The rest is done in the {FIXEM: find a better name )
// fullStubTest

BOOST_AUTO_TEST_SUITE(ReferenceTestApplicationWithThreadTestSuite)

BOOST_AUTO_TEST_CASE(testInt32_t) {
  auto pvManagers = createPVManager();
  auto csManager = pvManagers.first;
  auto devManager = pvManagers.second;

  // create the control application with it's own thread running the main loop
  ReferenceTestApplication testApplication;
  testApplication.setPVManager(devManager);
  testApplication.initialise();
  testApplication.run();

  ControlSystemSynchronizationUtility csSyncUtil(csManager);

  auto toDeviceScalar =
      csManager->getProcessArray<int32_t>("INT/TO_DEVICE_SCALAR");
  auto fromDeviceScalar =
      csManager->getProcessArray<int32_t>("INT/FROM_DEVICE_SCALAR");

  int32_t previousReadValue = fromDeviceScalar->accessData(0);

  toDeviceScalar->accessData(0) = previousReadValue + 13;

  csSyncUtil.sendAll();

  // Wait for max 1000 iterations. Each time we wait for the expected execution
  // time. If it is not true after 1000 waiting periods we declare this test as
  // failed.
  int i = 0;
  for (; i < 1000; ++i) {
    csSyncUtil.receiveAll();
    if (fromDeviceScalar->accessData(0) == previousReadValue + 13) {
      break;
    }
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
  }

  BOOST_CHECK_MESSAGE(i < 1000, "Reading the correct value timed out.");
}

BOOST_AUTO_TEST_SUITE_END()
