#define BOOST_TEST_MODULE FullStubTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ControlSystemSynchronizationUtility.h"
#include "IndependentTestCore.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

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
    std::cout << "this is TestCoreFixture():" << std::endl;
    IndependentTestCore::initialiseManualLoopControl();
    csSyncUtil.receiveAll();
  }
  ~TestCoreFixture(){
    std::cout << "this is ~TestCoreFixture():" << std::endl;
    IndependentTestCore::releaseManualLoopControl();
  }

  template<class UserType>
  void typedWriteScalarTest(std::string typeNamePrefix){
    auto toDeviceScalar = csManager->getProcessScalar<UserType>(typeNamePrefix+"/TO_DEVICE_SCALAR");
    auto fromDeviceScalar = csManager->getProcessScalar<UserType>(typeNamePrefix+"/FROM_DEVICE_SCALAR");

    UserType previousReadValue =  *fromDeviceScalar;

    *toDeviceScalar = previousReadValue+13;

    csSyncUtil.sendAll();
    IndependentTestCore::runMainLoopOnce();
    csSyncUtil.receiveAll();
  
    BOOST_CHECK( *fromDeviceScalar == previousReadValue+13 );
  }

  template<class UserType>
  void typedReadArrayTest(std::string typeNamePrefix){
    UserType typeConstant = csManager->getProcessScalar<UserType>(typeNamePrefix+"/DATA_TYPE_CONSTANT")->get();

    auto inputArray = csManager->getProcessArray<UserType>(typeNamePrefix+"/CONSTANT_ARRAY");
    BOOST_REQUIRE(inputArray);
    for (size_t i = 0; i < inputArray->get().size(); ++i){
      std::stringstream errorMessage;
      errorMessage << "check failed: " << typeNamePrefix+"/CONSTANT_ARRAY["
		   << i << "] = "<< inputArray->get()[i] << ", expected " << typeConstant*i*i
		   << std::endl;
      BOOST_CHECK_MESSAGE(static_cast<UserType>(typeConstant*i*i) == inputArray->get()[i],
			  errorMessage.str());
    }
  }
 
  template<class UserType>
  void typedWriteArrayTest(std::string typeNamePrefix){
    auto toDeviceArray = csManager->getProcessArray<UserType>(typeNamePrefix+"/TO_DEVICE_ARRAY");
    auto fromDeviceArray = csManager->getProcessArray<UserType>(typeNamePrefix+"/FROM_DEVICE_ARRAY");
    BOOST_REQUIRE(toDeviceArray);
    BOOST_REQUIRE(fromDeviceArray);

    // first check that there are all zeros in (startup condition)
    for ( auto & t  : toDeviceArray->get() ){
      BOOST_CHECK( t == 0 );
    }

    for (size_t i = 0; i < toDeviceArray->get().size(); ++i){
      toDeviceArray->get()[i] = 13 + i;
    }

    csSyncUtil.sendAll();
    IndependentTestCore::runMainLoopOnce();
    csSyncUtil.receiveAll();

    for (size_t i = 0; i < fromDeviceArray->get().size(); ++i){
      BOOST_CHECK(fromDeviceArray->get()[i] == static_cast<UserType>(13 + i));
    }
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

BOOST_FIXTURE_TEST_CASE( test_write_scalar, TestCoreFixture){
  typedWriteScalarTest<int8_t>("CHAR");
  typedWriteScalarTest<uint8_t>("UCHAR");
  typedWriteScalarTest<int16_t>("SHORT");
  typedWriteScalarTest<uint16_t>("USHORT");
  typedWriteScalarTest<int32_t>("INT");
  typedWriteScalarTest<uint32_t>("UINT");
  typedWriteScalarTest<float>("FLOAT");
  typedWriteScalarTest<double>("DOUBLE");
}

BOOST_FIXTURE_TEST_CASE( test_read_array, TestCoreFixture){
  typedReadArrayTest<int8_t>("CHAR");	 
  typedReadArrayTest<uint8_t>("UCHAR");
  typedReadArrayTest<int16_t>("SHORT");
  typedReadArrayTest<uint16_t>("USHORT");
  typedReadArrayTest<int32_t>("INT");	 
  typedReadArrayTest<uint32_t>("UINT");
  typedReadArrayTest<float>("FLOAT");	 
  typedReadArrayTest<double>("DOUBLE");  
}

BOOST_FIXTURE_TEST_CASE( test_write_array, TestCoreFixture){
  typedWriteArrayTest<int8_t>("CHAR");	 
  typedWriteArrayTest<uint8_t>("UCHAR");
  typedWriteArrayTest<int16_t>("SHORT");
  typedWriteArrayTest<uint16_t>("USHORT");
  typedWriteArrayTest<int32_t>("INT");	 
  typedWriteArrayTest<uint32_t>("UINT");
  typedWriteArrayTest<float>("FLOAT");	 
  typedWriteArrayTest<double>("DOUBLE");  
}

BOOST_AUTO_TEST_SUITE_END()
