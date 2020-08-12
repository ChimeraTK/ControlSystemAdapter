// Define a name for the test module.
#define BOOST_TEST_MODULE DeviceSynchronizationUtilityTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include <list>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>

#include "ControlSystemPVManager.h"
#include "DevicePVManager.h"
#include "DeviceSynchronizationUtility.h"

#include "CountingProcessVariableListener.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(DeviceSynchronizationUtilityTestSuite)

// Define one test after another. Each one needs a unique name.
BOOST_AUTO_TEST_CASE(testSendReceiveList) {
  std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
      createPVManager();
  boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
  boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

  ProcessArray<int32_t>::SharedPtr devIntOut =
      devManager->createProcessArray<int32_t>(SynchronizationDirection::controlSystemToDevice, "intOut", 1);
  ProcessArray<int32_t>::SharedPtr csIntOut = csManager->getProcessArray<int32_t>("intOut");
  ProcessArray<std::string>::SharedPtr devStringOut =
      devManager->createProcessArray<std::string>(SynchronizationDirection::controlSystemToDevice, "stringOut", 1);
  ProcessArray<std::string>::SharedPtr csStringOut = csManager->getProcessArray<std::string>("stringOut");
  ProcessArray<int32_t>::SharedPtr devIntIn =
      devManager->createProcessArray<int32_t>(SynchronizationDirection::deviceToControlSystem, "intIn", 1);
  ProcessArray<int32_t>::SharedPtr csIntIn = csManager->getProcessArray<int32_t>("intIn");
  ProcessArray<std::string>::SharedPtr devStringIn =
      devManager->createProcessArray<std::string>(SynchronizationDirection::deviceToControlSystem, "stringIn", 1);
  ProcessArray<std::string>::SharedPtr csStringIn = csManager->getProcessArray<std::string>("stringIn");

  DeviceSynchronizationUtility syncUtil(devManager);

  // Test all operations with zero elements. This should have no effect at
  // all, so this is just to test that the code does not throw an exception.
  ProcessVariable::SharedPtr emptyRawArray[1];
  syncUtil.receive(emptyRawArray, emptyRawArray);
  syncUtil.send(emptyRawArray, emptyRawArray);
  std::vector<ProcessVariable::SharedPtr> emptyVector;
  syncUtil.receive(emptyVector);
  syncUtil.send(emptyVector);
  std::list<ProcessVariable::SharedPtr> emptyList;
  syncUtil.receive(emptyList);
  syncUtil.send(emptyList);

  // Next we test it with a single element.
  ProcessVariable::SharedPtr singletonRawArray[1];
  singletonRawArray[0] = devIntOut;
  csIntOut->accessData(0) = 42;
  csIntOut->write();
  syncUtil.receive(singletonRawArray, singletonRawArray + 1);
  BOOST_CHECK(devIntOut->accessData(0) == 42);
  singletonRawArray[0] = devIntIn;
  devIntIn->accessData(0) = 43;
  syncUtil.send(singletonRawArray, singletonRawArray + 1);
  csIntIn->readNonBlocking();
  BOOST_CHECK(csIntIn->accessData(0) == 43);
  std::vector<ProcessVariable::SharedPtr> singletonVector(1);
  singletonVector[0] = devIntOut;
  csIntOut->accessData(0) = 44;
  csIntOut->write();
  syncUtil.receive(singletonVector);
  BOOST_CHECK(devIntOut->accessData(0) == 44);
  singletonVector[0] = devIntIn;
  devIntIn->accessData(0) = 45;
  syncUtil.send(singletonVector);
  csIntIn->readNonBlocking();
  BOOST_CHECK(csIntIn->accessData(0) == 45);
  std::list<ProcessVariable::SharedPtr> singletonList;
  singletonList.push_back(devIntOut);
  csIntOut->accessData(0) = 46;
  csIntOut->write();
  syncUtil.receive(singletonList);
  BOOST_CHECK(devIntOut->accessData(0) == 46);
  singletonList.clear();
  singletonList.push_back(devIntIn);
  devIntIn->accessData(0) = 47;
  syncUtil.send(singletonList);
  csIntIn->readNonBlocking();
  BOOST_CHECK(csIntIn->accessData(0) == 47);

  // Finally, we test with two elements.
  ProcessVariable::SharedPtr doubleRawArray[2];
  doubleRawArray[0] = devIntOut;
  doubleRawArray[1] = devStringOut;
  csIntOut->accessData(0) = 48;
  csStringOut->accessData(0) = "fourtynine";
  csIntOut->write();
  csStringOut->write();
  syncUtil.receive(doubleRawArray, doubleRawArray + 2);
  BOOST_CHECK(devIntOut->accessData(0) == 48);
  BOOST_CHECK(devStringOut->accessData(0) == "fourtynine");
  doubleRawArray[0] = devIntIn;
  doubleRawArray[1] = devStringIn;
  devIntIn->accessData(0) = 49;
  devStringIn->accessData(0) = "fifty";
  syncUtil.send(doubleRawArray, doubleRawArray + 2);
  csIntIn->readNonBlocking();
  csStringIn->readNonBlocking();
  BOOST_CHECK(csIntIn->accessData(0) == 49);
  BOOST_CHECK(csStringIn->accessData(0) == "fifty");
  std::vector<ProcessVariable::SharedPtr> doubleVector(2);
  doubleVector[0] = devIntOut;
  doubleVector[1] = devStringOut;
  csIntOut->accessData(0) = 51;
  csStringOut->accessData(0) = "fiftytwo";
  csIntOut->write();
  csStringOut->write();
  syncUtil.receive(doubleVector);
  BOOST_CHECK(devIntOut->accessData(0) == 51);
  BOOST_CHECK(devStringOut->accessData(0) == "fiftytwo");
  doubleVector[0] = devIntIn;
  doubleVector[1] = devStringIn;
  devIntIn->accessData(0) = 53;
  devStringIn->accessData(0) = "fiftyfour";
  syncUtil.send(doubleVector);
  csIntIn->readNonBlocking();
  csStringIn->readNonBlocking();
  BOOST_CHECK(csIntIn->accessData(0) == 53);
  BOOST_CHECK(csStringIn->accessData(0) == "fiftyfour");
  std::list<ProcessVariable::SharedPtr> doubleList;
  doubleList.push_back(devIntOut);
  doubleList.push_back(devStringOut);
  csIntOut->accessData(0) = 55;
  csStringOut->accessData(0) = "fiftysix";
  csIntOut->write();
  csStringOut->write();
  syncUtil.receive(doubleList);
  BOOST_CHECK(devIntOut->accessData(0) == 55);
  BOOST_CHECK(devStringOut->accessData(0) == "fiftysix");
  doubleList.clear();
  doubleList.push_back(devIntIn);
  doubleList.push_back(devStringIn);
  devIntIn->accessData(0) = 57;
  devStringIn->accessData(0) = "fiftyeight";
  syncUtil.send(doubleList);
  csIntIn->readNonBlocking();
  csStringIn->readNonBlocking();
  BOOST_CHECK(csIntIn->accessData(0) == 57);
  BOOST_CHECK(csStringIn->accessData(0) == "fiftyeight");
}

BOOST_AUTO_TEST_CASE(testReceiveAll) {
  std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
      createPVManager();
  boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
  boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

  ProcessArray<int32_t>::SharedPtr devIntOut =
      devManager->createProcessArray<int32_t>(SynchronizationDirection::controlSystemToDevice, "intOut", 1);
  ProcessArray<int32_t>::SharedPtr csIntOut = csManager->getProcessArray<int32_t>("intOut");

  DeviceSynchronizationUtility syncUtil(devManager);

  csIntOut->accessData(0) = 5;
  csIntOut->write();
  syncUtil.receiveAll();
  BOOST_CHECK(devIntOut->accessData(0) == 5);
}

BOOST_AUTO_TEST_CASE(testSendAll) {
  std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
      createPVManager();
  boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
  boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

  ProcessArray<int32_t>::SharedPtr devIntIn =
      devManager->createProcessArray<int32_t>(SynchronizationDirection::deviceToControlSystem, "intIn", 1);
  ProcessArray<int32_t>::SharedPtr csIntIn = csManager->getProcessArray<int32_t>("intIn");
  ProcessArray<std::string>::SharedPtr devStringIn =
      devManager->createProcessArray<std::string>(SynchronizationDirection::deviceToControlSystem, "stringIn", 1);
  ProcessArray<std::string>::SharedPtr csStringIn = csManager->getProcessArray<std::string>("stringIn");
  ProcessArray<int32_t>::SharedPtr devIntOut =
      devManager->createProcessArray<int32_t>(SynchronizationDirection::controlSystemToDevice, "intOut", 1);
  ProcessArray<int32_t>::SharedPtr csIntOut = csManager->getProcessArray<int32_t>("intOut");

  DeviceSynchronizationUtility syncUtil(devManager);

  devIntIn->accessData(0) = 15;
  devStringIn->accessData(0) = "sixteen";
  syncUtil.sendAll();
  csIntIn->readNonBlocking();
  csStringIn->readNonBlocking();
  BOOST_CHECK(csIntIn->accessData(0) == 15);
  BOOST_CHECK(csStringIn->accessData(0) == "sixteen");
}

BOOST_AUTO_TEST_CASE(testReceiveNotificationListener) {
  std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> pvManagers =
      createPVManager();
  boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
  boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

  ProcessArray<int32_t>::SharedPtr devIntOut =
      devManager->createProcessArray<int32_t>(SynchronizationDirection::controlSystemToDevice, "intOut", 1);
  ProcessArray<int32_t>::SharedPtr csIntOut = csManager->getProcessArray<int32_t>("intOut");
  ProcessArray<std::string>::SharedPtr devStringOut =
      devManager->createProcessArray<std::string>(SynchronizationDirection::controlSystemToDevice, "stringOut", 1);
  ProcessArray<std::string>::SharedPtr csStringOut = csManager->getProcessArray<std::string>("stringOut");

  DeviceSynchronizationUtility syncUtil(devManager);

  boost::shared_ptr<CountingProcessVariableListener> receiveNotificationListener =
      boost::make_shared<CountingProcessVariableListener>();

  syncUtil.addReceiveNotificationListener("intOut", receiveNotificationListener);
  csIntOut->write();
  csStringOut->write();
  syncUtil.receiveAll();
  BOOST_CHECK(receiveNotificationListener->count == 1);
  BOOST_CHECK(receiveNotificationListener->lastProcessVariable->getName() == "/intOut");

  csIntOut->write();
  std::vector<ProcessVariable::SharedPtr> pvList(1);
  pvList[0] = devIntOut;
  syncUtil.receive(pvList);
  BOOST_CHECK(receiveNotificationListener->count == 2);
  BOOST_CHECK(receiveNotificationListener->lastProcessVariable->getName() == "/intOut");

  syncUtil.removeReceiveNotificationListener("intOut");
  csIntOut->write();
  syncUtil.receiveAll();
  BOOST_CHECK(receiveNotificationListener->count == 2);
}

// After you finished all test you have to end the test suite.
BOOST_AUTO_TEST_SUITE_END()
