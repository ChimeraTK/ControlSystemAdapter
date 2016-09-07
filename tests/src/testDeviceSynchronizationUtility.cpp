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
BOOST_AUTO_TEST_SUITE( DeviceSynchronizationUtilityTestSuite )

// Define one test after another. Each one needs a unique name.
  BOOST_AUTO_TEST_CASE( testSendReceiveList ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessScalar<int32_t>::SharedPtr devIntOut =
        devManager->createProcessScalar<int32_t>(controlSystemToDevice,
            "intOut");
    ProcessScalar<int32_t>::SharedPtr csIntOut = csManager->getProcessScalar<
        int32_t>("intOut");
    ProcessScalar<float>::SharedPtr devFloatOut =
        devManager->createProcessScalar<float>(controlSystemToDevice,
            "floatOut");
    ProcessScalar<float>::SharedPtr csFloatOut = csManager->getProcessScalar<
        float>("floatOut");
    ProcessScalar<int32_t>::SharedPtr devIntIn =
        devManager->createProcessScalar<int32_t>(deviceToControlSystem,
            "intIn");
    ProcessScalar<int32_t>::SharedPtr csIntIn = csManager->getProcessScalar<
        int32_t>("intIn");
    ProcessScalar<float>::SharedPtr devFloatIn =
        devManager->createProcessScalar<float>(deviceToControlSystem,
            "floatIn");
    ProcessScalar<float>::SharedPtr csFloatIn = csManager->getProcessScalar<
        float>("floatIn");

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
    *csIntOut = 42;
    csIntOut->write();
    syncUtil.receive(singletonRawArray, singletonRawArray + 1);
    BOOST_CHECK(*devIntOut == 42);
    singletonRawArray[0] = devIntIn;
    *devIntIn = 43;
    syncUtil.send(singletonRawArray, singletonRawArray + 1);
    csIntIn->readNonBlocking();
    BOOST_CHECK(*csIntIn == 43);
    std::vector<ProcessVariable::SharedPtr> singletonVector(1);
    singletonVector[0] = devIntOut;
    *csIntOut = 44;
    csIntOut->write();
    syncUtil.receive(singletonVector);
    BOOST_CHECK(*devIntOut == 44);
    singletonVector[0] = devIntIn;
    *devIntIn = 45;
    syncUtil.send(singletonVector);
    csIntIn->readNonBlocking();
    BOOST_CHECK(*csIntIn == 45);
    std::list<ProcessVariable::SharedPtr> singletonList;
    singletonList.push_back(devIntOut);
    *csIntOut = 46;
    csIntOut->write();
    syncUtil.receive(singletonList);
    BOOST_CHECK(*devIntOut == 46);
    singletonList.clear();
    singletonList.push_back(devIntIn);
    *devIntIn = 47;
    syncUtil.send(singletonList);
    csIntIn->readNonBlocking();
    BOOST_CHECK(*csIntIn == 47);

    // Finally, we test with two elements.
    ProcessVariable::SharedPtr doubleRawArray[2];
    doubleRawArray[0] = devIntOut;
    doubleRawArray[1] = devFloatOut;
    *csIntOut = 48;
    *csFloatOut = 49.0f;
    csIntOut->write();
    csFloatOut->write();
    syncUtil.receive(doubleRawArray, doubleRawArray + 2);
    BOOST_CHECK(*devIntOut == 48);
    BOOST_CHECK(*devFloatOut == 49.0f);
    doubleRawArray[0] = devIntIn;
    doubleRawArray[1] = devFloatIn;
    *devIntIn = 49;
    *devFloatIn = 50.0f;
    syncUtil.send(doubleRawArray, doubleRawArray + 2);
    csIntIn->readNonBlocking();
    csFloatIn->readNonBlocking();
    BOOST_CHECK(*csIntIn == 49);
    BOOST_CHECK(*csFloatIn == 50.0f);
    std::vector<ProcessVariable::SharedPtr> doubleVector(2);
    doubleVector[0] = devIntOut;
    doubleVector[1] = devFloatOut;
    *csIntOut = 51;
    *csFloatOut = 52.0f;
    csIntOut->write();
    csFloatOut->write();
    syncUtil.receive(doubleVector);
    BOOST_CHECK(*devIntOut == 51);
    BOOST_CHECK(*devFloatOut == 52.0f);
    doubleVector[0] = devIntIn;
    doubleVector[1] = devFloatIn;
    *devIntIn = 53;
    *devFloatIn = 54.0f;
    syncUtil.send(doubleVector);
    csIntIn->readNonBlocking();
    csFloatIn->readNonBlocking();
    BOOST_CHECK(*csIntIn == 53);
    BOOST_CHECK(*csFloatIn == 54.0f);
    std::list<ProcessVariable::SharedPtr> doubleList;
    doubleList.push_back(devIntOut);
    doubleList.push_back(devFloatOut);
    *csIntOut = 55;
    *csFloatOut = 56.0f;
    csIntOut->write();
    csFloatOut->write();
    syncUtil.receive(doubleList);
    BOOST_CHECK(*devIntOut == 55);
    BOOST_CHECK(*devFloatOut == 56.0f);
    doubleList.clear();
    doubleList.push_back(devIntIn);
    doubleList.push_back(devFloatIn);
    *devIntIn = 57;
    *devFloatIn = 58.0f;
    syncUtil.send(doubleList);
    csIntIn->readNonBlocking();
    csFloatIn->readNonBlocking();
    BOOST_CHECK(*csIntIn == 57);
    BOOST_CHECK(*csFloatIn == 58.0f);
  }

  BOOST_AUTO_TEST_CASE( testReceiveAll ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessScalar<int32_t>::SharedPtr devIntOut =
        devManager->createProcessScalar<int32_t>(controlSystemToDevice,
            "intOut");
    ProcessScalar<int32_t>::SharedPtr csIntOut = csManager->getProcessScalar<
        int32_t>("intOut");

    DeviceSynchronizationUtility syncUtil(devManager);

    *csIntOut = 5;
    csIntOut->write();
    syncUtil.receiveAll();
    BOOST_CHECK(*devIntOut == 5);
  }

  BOOST_AUTO_TEST_CASE( testSendAll ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessScalar<int32_t>::SharedPtr devIntIn =
        devManager->createProcessScalar<int32_t>(deviceToControlSystem,
            "intIn");
    ProcessScalar<int32_t>::SharedPtr csIntIn = csManager->getProcessScalar<
        int32_t>("intIn");
    ProcessScalar<float>::SharedPtr devFloatIn =
        devManager->createProcessScalar<float>(deviceToControlSystem,
            "floatIn");
    ProcessScalar<float>::SharedPtr csFloatIn = csManager->getProcessScalar<
        float>("floatIn");
    ProcessScalar<int32_t>::SharedPtr devIntOut =
        devManager->createProcessScalar<int32_t>(controlSystemToDevice,
            "intOut");
    ProcessScalar<int32_t>::SharedPtr csIntOut = csManager->getProcessScalar<
        int32_t>("intOut");

    DeviceSynchronizationUtility syncUtil(devManager);

    *devIntIn = 15;
    *devFloatIn = 16.0f;
    syncUtil.sendAll();
    csIntIn->readNonBlocking();
    csFloatIn->readNonBlocking();
    BOOST_CHECK(*csIntIn == 15);
    BOOST_CHECK(*csFloatIn == 16.0f);
  }

  BOOST_AUTO_TEST_CASE( testReceiveNotificationListener ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessScalar<int32_t>::SharedPtr devIntOut =
        devManager->createProcessScalar<int32_t>(controlSystemToDevice,
            "intOut");
    ProcessScalar<int32_t>::SharedPtr csIntOut = csManager->getProcessScalar<
        int32_t>("intOut");
    ProcessScalar<float>::SharedPtr devFloatOut =
        devManager->createProcessScalar<float>(controlSystemToDevice,
            "floatOut");
    ProcessScalar<float>::SharedPtr csFloatOut = csManager->getProcessScalar<
        float>("floatOut");

    DeviceSynchronizationUtility syncUtil(devManager);

    boost::shared_ptr<CountingProcessVariableListener> receiveNotificationListener =
        boost::make_shared<CountingProcessVariableListener>();

    syncUtil.addReceiveNotificationListener("intOut",
        receiveNotificationListener);
    csIntOut->write();
    csFloatOut->write();
    syncUtil.receiveAll();
    BOOST_CHECK(receiveNotificationListener->count == 1);
    BOOST_CHECK(
        receiveNotificationListener->lastProcessVariable->getName()
            == "intOut");

    csIntOut->write();
    std::vector<ProcessVariable::SharedPtr> pvList(1);
    pvList[0] = devIntOut;
    syncUtil.receive(pvList);
    BOOST_CHECK(receiveNotificationListener->count == 2);
    BOOST_CHECK(
        receiveNotificationListener->lastProcessVariable->getName()
            == "intOut");

    syncUtil.removeReceiveNotificationListener("intOut");
    csIntOut->write();
    syncUtil.receiveAll();
    BOOST_CHECK(receiveNotificationListener->count == 2);
  }

// After you finished all test you have to end the test suite.
  BOOST_AUTO_TEST_SUITE_END()
