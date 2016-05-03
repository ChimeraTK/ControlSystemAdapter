#define BOOST_TEST_MODULE FullStubTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ControlSystemSynchronizationUtility.h"
#include "IndependentTestCore.h"

using namespace boost::unit_test_framework;
using namespace mtca4u;

struct TestCoreFixture{
  std::pair<boost::shared_ptr<ControlSystemPVManager>,
	    boost::shared_ptr<DevicePVManager> > pvManagers;
  boost::shared_ptr<ControlSystemPVManager> csManager;
  boost::shared_ptr<DevicePVManager> devManager;
  
  IndependentTestCore testCore;

  ControlSystemSynchronizationUtility csSyncUtil;

  TestCoreFixture() : 
    pvManagers( createPVManager() ),
    csManager( pvManagers.first ),
    devManager( pvManagers.second ),
    testCore(devManager),
    csSyncUtil(csManager){
  }
};

BOOST_AUTO_TEST_SUITE( FullStubTestSuite )

BOOST_FIXTURE_TEST_CASE( test_something, TestCoreFixture){
}

BOOST_AUTO_TEST_SUITE_END()
