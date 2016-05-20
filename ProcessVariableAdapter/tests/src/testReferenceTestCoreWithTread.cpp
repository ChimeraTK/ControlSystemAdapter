#define BOOST_TEST_MODULE ReferenceTestCoreWithThreadTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ControlSystemSynchronizationUtility.h"
#include "IndependentTestCore.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

// We are testing the reference core when executed with it's own thread running.
// Synchronisation is done through the atomic mainBodyCompletelyExecuted variable.

// This test is only executed for int32_t scalars because it is only testing the thread
// functionality. The rest is done in the {FIXEM: find a better name ) fullStubTest

BOOST_AUTO_TEST_SUITE( ReferenceTestCoreWithThreadTestSuite )

BOOST_AUTO_TEST_CASE( testInt32_t ){
  auto pvManagers =  createPVManager();
  auto csManager = pvManagers.first;
  auto devManager = pvManagers.second;

  // create the control core with it's own thread running the main loop
  IndependentTestCore testCore( devManager, true /*startThread*/);

  ControlSystemSynchronizationUtility csSyncUtil(csManager);

  auto toDeviceScalar = csManager->getProcessScalar<int32_t>("INT/TO_DEVICE_SCALAR");
  auto fromDeviceScalar = csManager->getProcessScalar<int32_t>("INT/FROM_DEVICE_SCALAR");
    
  int32_t previousReadValue =  *fromDeviceScalar;

  *toDeviceScalar = previousReadValue+13;

  csSyncUtil.sendAll();
  
  // Now we have to wait until the thread in  the test core has executed the main loop.
  // For this we set the mainBodyCompletelyExecuted variable to false and wait unti it becomes true.
  IndependentTestCore::mainBodyCompletelyExecuted() = false;

  // Wait for max 1000 iterations. Each time we wait for the expected execution time. If it is not
  // true after 1000 waiting periods we declare this test as failed.
  for (int i = 0; i < 1000 ; ++i){
    if (IndependentTestCore::mainBodyCompletelyExecuted()){
      break;
    }
    boost::this_thread::sleep_for( boost::chrono::milliseconds(100) );
  }

  if (!IndependentTestCore::mainBodyCompletelyExecuted()){
    // We ran into the timeout. The mainBody has not been executed, something went wrong.
    BOOST_ERROR( "Synchronisation with the mainBody of the ReferenceTestCore failed." );
  }

  csSyncUtil.receiveAll();
  
  BOOST_CHECK( *fromDeviceScalar == previousReadValue+13 );

}

BOOST_AUTO_TEST_SUITE_END()
