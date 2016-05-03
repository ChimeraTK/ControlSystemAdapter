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
    csSyncUtil.receiveAll();
  }
};

BOOST_AUTO_TEST_SUITE( FullStubTestSuite )

BOOST_FIXTURE_TEST_CASE( test_read_scalar, TestCoreFixture){
  // just after creation of the fixture the constants should be available to the control system
  BOOST_CHECK( csManager->getProcessScalar<int8_t>("CHAR/DATA_TYPE_CONSTANT")->get() == -1 );
  BOOST_CHECK( csManager->getProcessScalar<uint8_t>("UCHAR/DATA_TYPE_CONSTANT")->get() == 1 );
  BOOST_CHECK( csManager->getProcessScalar<int16_t>("SHORT/DATA_TYPE_CONSTANT")->get() == -2 );
  BOOST_CHECK( csManager->getProcessScalar<uint16_t>("USHORT/DATA_TYPE_CONSTANT")->get() == 2 );
  BOOST_CHECK( csManager->getProcessScalar<int32_t>("INT/DATA_TYPE_CONSTANT")->get() == -4 );
  BOOST_CHECK( csManager->getProcessScalar<uint32_t>("UINT/DATA_TYPE_CONSTANT")->get() == 4 );
  BOOST_CHECK( csManager->getProcessScalar<float>("FLOAT/DATA_TYPE_CONSTANT")->get() == 1./4 );
  BOOST_CHECK( csManager->getProcessScalar<double>("DOUBLE/DATA_TYPE_CONSTANT")->get() == 1./8 );
}

BOOST_AUTO_TEST_SUITE_END()
