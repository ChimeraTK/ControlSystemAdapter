#define BOOST_TEST_MODULE FullStubTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include <ChimeraTK/ReadAnyGroup.h>
#include <ChimeraTK/TransferGroup.h>

#include "ReferenceTestApplication.h"
#include "ControlSystemPVManager.h"
#include "DevicePVManager.h"
#include "toDouble.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

struct TestApplicationFixture {
  std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers;
  boost::shared_ptr<ControlSystemPVManager> csManager;
  boost::shared_ptr<DevicePVManager> devManager;

  ReferenceTestApplication testApplication;

  ReadAnyGroup readAnyGroup;

  TestApplicationFixture() : pvManagers(createPVManager()), csManager(pvManagers.first), devManager(pvManagers.second) {
    std::cout << "this is TestApplicationFixture():" << std::endl;
    testApplication.setPVManager(devManager);
    testApplication.initialise();
    for(auto pv : csManager->getAllProcessVariables()) {
      if(pv->isReadable()) readAnyGroup.add(pv);
    }
    readAnyGroup.finalise();
    testApplication.run();
    ReferenceTestApplication::initialiseManualLoopControl();
    while(readAnyGroup.readAnyNonBlocking().isValid()) {
      continue;
    }
  }
  ~TestApplicationFixture() {
    std::cout << "this is ~TestApplicationFixture():" << std::endl;
    ReferenceTestApplication::releaseManualLoopControl();
  }

  template<class UserType>
  void typedWriteScalarTest(std::string typeNamePrefix) {
    auto toDeviceScalar = csManager->getProcessArray<UserType>(typeNamePrefix + "/TO_DEVICE_SCALAR");
    auto fromDeviceScalar = csManager->getProcessArray<UserType>(typeNamePrefix + "/FROM_DEVICE_SCALAR");

    // check that the test is actually sensitive to writing. We could add 13 to a numeric value, but what is
    // sting + 13? So we just check that there was something different before.
    BOOST_CHECK( fromDeviceScalar->accessData(0) != toType<UserType>(13) );

    toDeviceScalar->accessData(0) = toType<UserType>(13);

    for(auto pv : csManager->getAllProcessVariables()) {
      if(pv->isWriteable()) pv->write();
    }
    ReferenceTestApplication::runMainLoopOnce();
    while(readAnyGroup.readAnyNonBlocking().isValid()) continue;

    BOOST_CHECK_EQUAL(fromDeviceScalar->accessData(0), toType<UserType>(13));
  }

  template<class UserType>
  void typedReadArrayTest(std::string typeNamePrefix) {
    UserType typeConstant = csManager->getProcessArray<UserType>(typeNamePrefix + "/DATA_TYPE_CONSTANT")->accessData(0);
    double typeIdentiyingDouble = toDouble(typeConstant);
    
    auto inputArray = csManager->getProcessArray<UserType>(typeNamePrefix + "/CONSTANT_ARRAY");
    BOOST_REQUIRE(inputArray);
    for(size_t i = 0; i < inputArray->accessChannel(0).size(); ++i) {
      std::stringstream errorMessage;
      errorMessage << "check failed: " << typeNamePrefix + "/CONSTANT_ARRAY[" << i
                   << "] = " << inputArray->accessChannel(0)[i] << ", expected " << toType<UserType>(typeIdentiyingDouble * i * i) << std::endl;
      BOOST_CHECK_MESSAGE(
          toType<UserType>(typeIdentiyingDouble * i * i) == inputArray->accessChannel(0)[i], errorMessage.str());
    }
  }

  template<class UserType>
  void typedWriteArrayTest(std::string typeNamePrefix) {
    auto toDeviceArray = csManager->getProcessArray<UserType>(typeNamePrefix + "/TO_DEVICE_ARRAY");
    auto fromDeviceArray = csManager->getProcessArray<UserType>(typeNamePrefix + "/FROM_DEVICE_ARRAY");
    BOOST_REQUIRE(toDeviceArray);
    BOOST_REQUIRE(fromDeviceArray);

    // first check that there are all zeros/default constructed in (startup condition)
    for(auto& t : toDeviceArray->accessChannel(0)) {
      BOOST_CHECK_EQUAL(t, UserType());
    }

    for(size_t i = 0; i < toDeviceArray->accessChannel(0).size(); ++i) {
      toDeviceArray->accessChannel(0)[i] = toType<UserType>(13 + i);
    }

    for(auto pv : csManager->getAllProcessVariables()) {
      if(pv->isWriteable()) pv->write();
    }
    ReferenceTestApplication::runMainLoopOnce();
    while(readAnyGroup.readAnyNonBlocking().isValid()) continue;

    for(size_t i = 0; i < fromDeviceArray->accessChannel(0).size(); ++i) {
      BOOST_CHECK(fromDeviceArray->accessChannel(0)[i] == toType<UserType>(13 + i));
    }
  }
};

BOOST_AUTO_TEST_SUITE(FullStubTestSuite)

BOOST_FIXTURE_TEST_CASE(test_read_scalar, TestApplicationFixture) {
  // just after creation of the fixture the constants should be available to the
  // control system
  BOOST_CHECK_EQUAL(csManager->getProcessArray<int8_t>("CHAR/DATA_TYPE_CONSTANT")->accessChannel(0)[0], -1);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<uint8_t>("UCHAR/DATA_TYPE_CONSTANT")->accessChannel(0)[0], 1);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<int16_t>("SHORT/DATA_TYPE_CONSTANT")->accessChannel(0)[0], -2);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<uint16_t>("USHORT/DATA_TYPE_CONSTANT")->accessChannel(0)[0], 2);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<int32_t>("INT/DATA_TYPE_CONSTANT")->accessChannel(0)[0], -4);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<uint32_t>("UINT/DATA_TYPE_CONSTANT")->accessChannel(0)[0], 4);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<int64_t>("LONG/DATA_TYPE_CONSTANT")->accessChannel(0)[0], -8);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<uint64_t>("ULONG/DATA_TYPE_CONSTANT")->accessChannel(0)[0], 8);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<float>("FLOAT/DATA_TYPE_CONSTANT")->accessChannel(0)[0], 1. / 4);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<double>("DOUBLE/DATA_TYPE_CONSTANT")->accessChannel(0)[0], 1. / 8);
  BOOST_CHECK_EQUAL(csManager->getProcessArray<std::string>("STRING/DATA_TYPE_CONSTANT")->accessChannel(0)[0], std::to_string(42.));
}

BOOST_FIXTURE_TEST_CASE(test_write_scalar, TestApplicationFixture) {
  typedWriteScalarTest<int8_t>("CHAR");
  typedWriteScalarTest<uint8_t>("UCHAR");
  typedWriteScalarTest<int16_t>("SHORT");
  typedWriteScalarTest<uint16_t>("USHORT");
  typedWriteScalarTest<int32_t>("INT");
  typedWriteScalarTest<uint32_t>("UINT");
  typedWriteScalarTest<int64_t>("LONG");
  typedWriteScalarTest<uint64_t>("ULONG");
  typedWriteScalarTest<float>("FLOAT");
  typedWriteScalarTest<double>("DOUBLE");
  typedWriteScalarTest<std::string>("STRING");
}

BOOST_FIXTURE_TEST_CASE(test_read_array, TestApplicationFixture) {
  typedReadArrayTest<int8_t>("CHAR");
  typedReadArrayTest<uint8_t>("UCHAR");
  typedReadArrayTest<int16_t>("SHORT");
  typedReadArrayTest<uint16_t>("USHORT");
  typedReadArrayTest<int32_t>("INT");
  typedReadArrayTest<uint32_t>("UINT");
  typedReadArrayTest<int64_t>("LONG");
  typedReadArrayTest<uint64_t>("ULONG");
  typedReadArrayTest<float>("FLOAT");
  typedReadArrayTest<double>("DOUBLE");
  typedReadArrayTest<std::string>("STRING");
}

BOOST_FIXTURE_TEST_CASE(test_write_array, TestApplicationFixture) {
  typedWriteArrayTest<int8_t>("CHAR");
  typedWriteArrayTest<uint8_t>("UCHAR");
  typedWriteArrayTest<int16_t>("SHORT");
  typedWriteArrayTest<uint16_t>("USHORT");
  typedWriteArrayTest<int32_t>("INT");
  typedWriteArrayTest<uint32_t>("UINT");
  typedWriteArrayTest<int64_t>("LONG");
  typedWriteArrayTest<uint64_t>("ULONG");
  typedWriteArrayTest<float>("FLOAT");
  typedWriteArrayTest<double>("DOUBLE");
  typedWriteArrayTest<std::string>("STRING");
}

BOOST_AUTO_TEST_SUITE_END()
