#define BOOST_TEST_MODULE FullStubTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ControlSystemSynchronizationUtility.h"
#include "ReferenceTestApplication.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

struct TestApplicationFixture {
  std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers;
  boost::shared_ptr<ControlSystemPVManager> csManager;
  boost::shared_ptr<DevicePVManager> devManager;

  ReferenceTestApplication testApplication;

  ControlSystemSynchronizationUtility csSyncUtil;

  TestApplicationFixture()
  : pvManagers(createPVManager()), csManager(pvManagers.first), devManager(pvManagers.second), csSyncUtil(csManager) {
    std::cout << "this is TestApplicationFixture():" << std::endl;
    testApplication.setPVManager(devManager);
    testApplication.initialise();
    testApplication.run();
    ReferenceTestApplication::initialiseManualLoopControl();
    csSyncUtil.receiveAll();
  }
  ~TestApplicationFixture() {
    std::cout << "this is ~TestApplicationFixture():" << std::endl;
    ReferenceTestApplication::releaseManualLoopControl();
  }

  template<class UserType>
  void typedWriteScalarTest(std::string typeNamePrefix) {
    auto toDeviceScalar = csManager->getProcessArray<UserType>(typeNamePrefix + "/TO_DEVICE_SCALAR");
    auto fromDeviceScalar = csManager->getProcessArray<UserType>(typeNamePrefix + "/FROM_DEVICE_SCALAR");

    UserType previousReadValue = fromDeviceScalar->accessData(0);

    toDeviceScalar->accessData(0) = previousReadValue + 13;

    csSyncUtil.sendAll();
    ReferenceTestApplication::runMainLoopOnce();
    csSyncUtil.receiveAll();

    BOOST_CHECK(fromDeviceScalar->accessData(0) == previousReadValue + 13);
  }

  template<class UserType>
  void typedReadArrayTest(std::string typeNamePrefix) {
    UserType typeConstant = csManager->getProcessArray<UserType>(typeNamePrefix + "/DATA_TYPE_CONSTANT")->accessData(0);

    auto inputArray = csManager->getProcessArray<UserType>(typeNamePrefix + "/CONSTANT_ARRAY");
    BOOST_REQUIRE(inputArray);
    for(size_t i = 0; i < inputArray->accessChannel(0).size(); ++i) {
      std::stringstream errorMessage;
      errorMessage << "check failed: " << typeNamePrefix + "/CONSTANT_ARRAY[" << i
                   << "] = " << inputArray->accessChannel(0)[i] << ", expected " << typeConstant * i * i << std::endl;
      BOOST_CHECK_MESSAGE(
          static_cast<UserType>(typeConstant * i * i) == inputArray->accessChannel(0)[i], errorMessage.str());
    }
  }

  template<class UserType>
  void typedWriteArrayTest(std::string typeNamePrefix) {
    auto toDeviceArray = csManager->getProcessArray<UserType>(typeNamePrefix + "/TO_DEVICE_ARRAY");
    auto fromDeviceArray = csManager->getProcessArray<UserType>(typeNamePrefix + "/FROM_DEVICE_ARRAY");
    BOOST_REQUIRE(toDeviceArray);
    BOOST_REQUIRE(fromDeviceArray);

    // first check that there are all zeros in (startup condition)
    for(auto& t : toDeviceArray->accessChannel(0)) {
      BOOST_CHECK(t == 0);
    }

    for(size_t i = 0; i < toDeviceArray->accessChannel(0).size(); ++i) {
      toDeviceArray->accessChannel(0)[i] = 13 + i;
    }

    csSyncUtil.sendAll();
    ReferenceTestApplication::runMainLoopOnce();
    csSyncUtil.receiveAll();

    for(size_t i = 0; i < fromDeviceArray->accessChannel(0).size(); ++i) {
      BOOST_CHECK(fromDeviceArray->accessChannel(0)[i] == static_cast<UserType>(13 + i));
    }
  }
};

BOOST_AUTO_TEST_SUITE(FullStubTestSuite)

BOOST_FIXTURE_TEST_CASE(test_read_scalar, TestApplicationFixture) {
  // just after creation of the fixture the constants should be available to the
  // control system
  BOOST_CHECK(csManager->getProcessArray<int8_t>("CHAR/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == -1);
  BOOST_CHECK(csManager->getProcessArray<uint8_t>("UCHAR/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == 1);
  BOOST_CHECK(csManager->getProcessArray<int16_t>("SHORT/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == -2);
  BOOST_CHECK(csManager->getProcessArray<uint16_t>("USHORT/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == 2);
  BOOST_CHECK(csManager->getProcessArray<int32_t>("INT/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == -4);
  BOOST_CHECK(csManager->getProcessArray<uint32_t>("UINT/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == 4);
  BOOST_CHECK(csManager->getProcessArray<float>("FLOAT/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == 1. / 4);
  BOOST_CHECK(csManager->getProcessArray<double>("DOUBLE/DATA_TYPE_CONSTANT")->accessChannel(0)[0] == 1. / 8);
}

BOOST_FIXTURE_TEST_CASE(test_write_scalar, TestApplicationFixture) {
  typedWriteScalarTest<int8_t>("CHAR");
  typedWriteScalarTest<uint8_t>("UCHAR");
  typedWriteScalarTest<int16_t>("SHORT");
  typedWriteScalarTest<uint16_t>("USHORT");
  typedWriteScalarTest<int32_t>("INT");
  typedWriteScalarTest<uint32_t>("UINT");
  typedWriteScalarTest<float>("FLOAT");
  typedWriteScalarTest<double>("DOUBLE");
}

BOOST_FIXTURE_TEST_CASE(test_read_array, TestApplicationFixture) {
  typedReadArrayTest<int8_t>("CHAR");
  typedReadArrayTest<uint8_t>("UCHAR");
  typedReadArrayTest<int16_t>("SHORT");
  typedReadArrayTest<uint16_t>("USHORT");
  typedReadArrayTest<int32_t>("INT");
  typedReadArrayTest<uint32_t>("UINT");
  typedReadArrayTest<float>("FLOAT");
  typedReadArrayTest<double>("DOUBLE");
}

BOOST_FIXTURE_TEST_CASE(test_write_array, TestApplicationFixture) {
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
