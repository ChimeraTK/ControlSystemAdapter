// Define a name for the test module.
#define BOOST_TEST_MODULE ControlSystemSynchronizationUtilityTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include <list>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>

#include "ControlSystemPVManager.h"
#include "DevicePVManager.h"
#include "ControlSystemSynchronizationUtility.h"

#include "CountingProcessVariableListener.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE( ControlSystemSynchronizationUtilityTestSuite )

// Define one test after another. Each one needs a unique name.
  BOOST_AUTO_TEST_CASE( testSendReceiveList ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessArray<int32_t>::SharedPtr devIntOut =
        devManager->createProcessArray<int32_t>(controlSystemToDevice,
            "intOut",1);
    ProcessArray<int32_t>::SharedPtr csIntOut = csManager->getProcessArray<
        int32_t>("intOut");
    ProcessArray<float>::SharedPtr devFloatOut =
        devManager->createProcessArray<float>(controlSystemToDevice,
            "floatOut",1);
    ProcessArray<float>::SharedPtr csFloatOut = csManager->getProcessArray<
        float>("floatOut");
    ProcessArray<int32_t>::SharedPtr devIntIn =
        devManager->createProcessArray<int32_t>(deviceToControlSystem,
            "intIn",1);
    ProcessArray<int32_t>::SharedPtr csIntIn = csManager->getProcessArray<
        int32_t>("intIn");
    ProcessArray<float>::SharedPtr devFloatIn =
        devManager->createProcessArray<float>(deviceToControlSystem,
            "floatIn",1);
    ProcessArray<float>::SharedPtr csFloatIn = csManager->getProcessArray<
        float>("floatIn");

    ControlSystemSynchronizationUtility syncUtil(csManager);

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
    singletonRawArray[0] = csIntIn;
    devIntIn->accessData(0) = 42;
    devIntIn->write();
    syncUtil.receive(singletonRawArray, singletonRawArray + 1);
    BOOST_CHECK(csIntIn->accessData(0) == 42);
    singletonRawArray[0] = csIntOut;
    csIntOut->accessData(0) = 43;
    syncUtil.send(singletonRawArray, singletonRawArray + 1);
    devIntOut->readNonBlocking();
    BOOST_CHECK(devIntOut->accessData(0) == 43);
    std::vector<ProcessVariable::SharedPtr> singletonVector(1);
    singletonVector[0] = csIntIn;
    devIntIn->accessData(0) = 44;
    devIntIn->write();
    syncUtil.receive(singletonVector);
    BOOST_CHECK(csIntIn->accessData(0) == 44);
    singletonVector[0] = csIntOut;
    csIntOut->accessData(0) = 45;
    syncUtil.send(singletonVector);
    devIntOut->readNonBlocking();
    BOOST_CHECK(devIntOut->accessData(0) == 45);
    std::list<ProcessVariable::SharedPtr> singletonList;
    singletonList.push_back(csIntIn);
    devIntIn->accessData(0) = 46;
    devIntIn->write();
    syncUtil.receive(singletonList);
    BOOST_CHECK(csIntIn->accessData(0) == 46);
    singletonList.clear();
    singletonList.push_back(csIntOut);
    csIntOut->accessData(0) = 47;
    syncUtil.send(singletonList);
    devIntOut->readNonBlocking();
    BOOST_CHECK(devIntOut->accessData(0) == 47);

    // Finally, we test with two elements.
    ProcessVariable::SharedPtr doubleRawArray[2];
    doubleRawArray[0] = csIntIn;
    doubleRawArray[1] = csFloatIn;
    devIntIn->accessData(0) = 48;
    devFloatIn->accessData(0) = 49.0f;
    devIntIn->write();
    devFloatIn->write();
    syncUtil.receive(doubleRawArray, doubleRawArray + 2);
    BOOST_CHECK(csIntIn->accessData(0) == 48);
    BOOST_CHECK(csFloatIn->accessData(0) == 49.0f);
    doubleRawArray[0] = csIntOut;
    doubleRawArray[1] = csFloatOut;
    csIntOut->accessData(0) = 49;
    csFloatOut->accessData(0) = 50.0f;
    syncUtil.send(doubleRawArray, doubleRawArray + 2);
    devIntOut->readNonBlocking();
    devFloatOut->readNonBlocking();
    BOOST_CHECK(devIntOut->accessData(0) == 49);
    BOOST_CHECK(devFloatOut->accessData(0) == 50.0f);
    std::vector<ProcessVariable::SharedPtr> doubleVector(2);
    doubleVector[0] = csIntIn;
    doubleVector[1] = csFloatIn;
    devIntIn->accessData(0) = 51;
    devFloatIn->accessData(0) = 52.0f;
    devIntIn->write();
    devFloatIn->write();
    syncUtil.receive(doubleVector);
    BOOST_CHECK(csIntIn->accessData(0) == 51);
    BOOST_CHECK(csFloatIn->accessData(0) == 52.0f);
    doubleVector[0] = csIntOut;
    doubleVector[1] = csFloatOut;
    csIntOut->accessData(0) = 53;
    csFloatOut->accessData(0) = 54.0f;
    syncUtil.send(doubleVector);
    devIntOut->readNonBlocking();
    devFloatOut->readNonBlocking();
    BOOST_CHECK(devIntOut->accessData(0) == 53);
    BOOST_CHECK(devFloatOut->accessData(0) == 54.0f);
    std::list<ProcessVariable::SharedPtr> doubleList;
    doubleList.push_back(csIntIn);
    doubleList.push_back(csFloatIn);
    devIntIn->accessData(0) = 55;
    devFloatIn->accessData(0) = 56.0f;
    devIntIn->write();
    devFloatIn->write();
    syncUtil.receive(doubleList);
    BOOST_CHECK(csIntIn->accessData(0) == 55);
    BOOST_CHECK(csFloatIn->accessData(0) == 56.0f);
    doubleList.clear();
    doubleList.push_back(csIntOut);
    doubleList.push_back(csFloatOut);
    csIntOut->accessData(0) = 57;
    csFloatOut->accessData(0) = 58.0f;
    syncUtil.send(doubleList);
    devIntOut->readNonBlocking();
    devFloatOut->readNonBlocking();
    BOOST_CHECK(devIntOut->accessData(0) == 57);
    BOOST_CHECK(devFloatOut->accessData(0) == 58.0f);
  }

  BOOST_AUTO_TEST_CASE( testReceiveAll ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessArray<int32_t>::SharedPtr devIntIn =
        devManager->createProcessArray<int32_t>(deviceToControlSystem,
            "intIn",1);
    ProcessArray<int32_t>::SharedPtr csIntIn = csManager->getProcessArray<
        int32_t>("intIn");

    ControlSystemSynchronizationUtility syncUtil(csManager);

    devIntIn->accessData(0) = 5;
    devIntIn->write();
    syncUtil.receiveAll();
    BOOST_CHECK(csIntIn->accessData(0) == 5);
  }

  BOOST_AUTO_TEST_CASE( testSendAll ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessArray<int32_t>::SharedPtr devIntOut =
        devManager->createProcessArray<int32_t>(controlSystemToDevice,
            "intOut",1);
    ProcessArray<int32_t>::SharedPtr csIntOut = csManager->getProcessArray<
        int32_t>("intOut");
    ProcessArray<float>::SharedPtr devFloatOut =
        devManager->createProcessArray<float>(controlSystemToDevice,
            "floatOut",1);
    ProcessArray<float>::SharedPtr csFloatOut = csManager->getProcessArray<
        float>("floatOut");
    ProcessArray<int32_t>::SharedPtr devIntIn =
        devManager->createProcessArray<int32_t>(deviceToControlSystem,
            "intIn",1);
    ProcessArray<int32_t>::SharedPtr csIntIn = csManager->getProcessArray<
        int32_t>("intIn");

    ControlSystemSynchronizationUtility syncUtil(csManager);

    csIntOut->accessData(0) = 15;
    csFloatOut->accessData(0) = 16.0f;
    syncUtil.sendAll();
    devIntOut->readNonBlocking();
    devFloatOut->readNonBlocking();
    BOOST_CHECK(devIntOut->accessData(0) == 15);
    BOOST_CHECK(devFloatOut->accessData(0) == 16.0f);
  }

  BOOST_AUTO_TEST_CASE( testReceiveNotificationListener ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessArray<int32_t>::SharedPtr devIntIn =
        devManager->createProcessArray<int32_t>(deviceToControlSystem,
            "intIn",1);
    ProcessArray<int32_t>::SharedPtr csIntIn = csManager->getProcessArray<
        int32_t>("intIn");
    ProcessArray<float>::SharedPtr devFloatIn =
        devManager->createProcessArray<float>(deviceToControlSystem,
            "floatIn",1);
    ProcessArray<float>::SharedPtr csFloatIn = csManager->getProcessArray<
        float>("floatIn");

    ControlSystemSynchronizationUtility syncUtil(csManager);

    boost::shared_ptr<CountingProcessVariableListener> receiveNotificationListener =
        boost::make_shared<CountingProcessVariableListener>();

    syncUtil.addReceiveNotificationListener("intIn",
        receiveNotificationListener);
    devIntIn->write();
    devFloatIn->write();
    syncUtil.receiveAll();
    BOOST_CHECK(receiveNotificationListener->count == 1);
    // the pv name has a '/' because it is a register path
    BOOST_CHECK(
        receiveNotificationListener->lastProcessVariable->getName() == "/intIn");

    devIntIn->write();
    std::vector<ProcessVariable::SharedPtr> pvList(1);
    pvList[0] = csIntIn;
    syncUtil.receive(pvList);
    BOOST_CHECK(receiveNotificationListener->count == 2);
    BOOST_CHECK(
        receiveNotificationListener->lastProcessVariable->getName() == "/intIn");

    syncUtil.removeReceiveNotificationListener("intIn");
    devIntIn->write();
    syncUtil.receiveAll();
    BOOST_CHECK(receiveNotificationListener->count == 2);
  }

// After you finished all test you have to end the test suite.
  BOOST_AUTO_TEST_SUITE_END()
