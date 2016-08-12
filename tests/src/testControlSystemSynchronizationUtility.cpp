// Define a name for the test module.
#define BOOST_TEST_MODULE ControlSystemSynchronizationUtilityTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include <list>
#include <vector>

#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>

#include <ControlSystemPVManager.h>
#include <DevicePVManager.h>
#include <ControlSystemSynchronizationUtility.h>

#include <CountingProcessVariableListener.h>

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
    *devIntIn = 42;
    devIntIn->send();
    syncUtil.receive(singletonRawArray, singletonRawArray + 1);
    BOOST_CHECK(*csIntIn == 42);
    singletonRawArray[0] = csIntOut;
    *csIntOut = 43;
    syncUtil.send(singletonRawArray, singletonRawArray + 1);
    devIntOut->receive();
    BOOST_CHECK(*devIntOut == 43);
    std::vector<ProcessVariable::SharedPtr> singletonVector(1);
    singletonVector[0] = csIntIn;
    *devIntIn = 44;
    devIntIn->send();
    syncUtil.receive(singletonVector);
    BOOST_CHECK(*csIntIn == 44);
    singletonVector[0] = csIntOut;
    *csIntOut = 45;
    syncUtil.send(singletonVector);
    devIntOut->receive();
    BOOST_CHECK(*devIntOut == 45);
    std::list<ProcessVariable::SharedPtr> singletonList;
    singletonList.push_back(csIntIn);
    *devIntIn = 46;
    devIntIn->send();
    syncUtil.receive(singletonList);
    BOOST_CHECK(*csIntIn == 46);
    singletonList.clear();
    singletonList.push_back(csIntOut);
    *csIntOut = 47;
    syncUtil.send(singletonList);
    devIntOut->receive();
    BOOST_CHECK(*devIntOut == 47);

    // Finally, we test with two elements.
    ProcessVariable::SharedPtr doubleRawArray[2];
    doubleRawArray[0] = csIntIn;
    doubleRawArray[1] = csFloatIn;
    *devIntIn = 48;
    *devFloatIn = 49.0f;
    devIntIn->send();
    devFloatIn->send();
    syncUtil.receive(doubleRawArray, doubleRawArray + 2);
    BOOST_CHECK(*csIntIn == 48);
    BOOST_CHECK(*csFloatIn == 49.0f);
    doubleRawArray[0] = csIntOut;
    doubleRawArray[1] = csFloatOut;
    *csIntOut = 49;
    *csFloatOut = 50.0f;
    syncUtil.send(doubleRawArray, doubleRawArray + 2);
    devIntOut->receive();
    devFloatOut->receive();
    BOOST_CHECK(*devIntOut == 49);
    BOOST_CHECK(*devFloatOut == 50.0f);
    std::vector<ProcessVariable::SharedPtr> doubleVector(2);
    doubleVector[0] = csIntIn;
    doubleVector[1] = csFloatIn;
    *devIntIn = 51;
    *devFloatIn = 52.0f;
    devIntIn->send();
    devFloatIn->send();
    syncUtil.receive(doubleVector);
    BOOST_CHECK(*csIntIn == 51);
    BOOST_CHECK(*csFloatIn == 52.0f);
    doubleVector[0] = csIntOut;
    doubleVector[1] = csFloatOut;
    *csIntOut = 53;
    *csFloatOut = 54.0f;
    syncUtil.send(doubleVector);
    devIntOut->receive();
    devFloatOut->receive();
    BOOST_CHECK(*devIntOut == 53);
    BOOST_CHECK(*devFloatOut == 54.0f);
    std::list<ProcessVariable::SharedPtr> doubleList;
    doubleList.push_back(csIntIn);
    doubleList.push_back(csFloatIn);
    *devIntIn = 55;
    *devFloatIn = 56.0f;
    devIntIn->send();
    devFloatIn->send();
    syncUtil.receive(doubleList);
    BOOST_CHECK(*csIntIn == 55);
    BOOST_CHECK(*csFloatIn == 56.0f);
    doubleList.clear();
    doubleList.push_back(csIntOut);
    doubleList.push_back(csFloatOut);
    *csIntOut = 57;
    *csFloatOut = 58.0f;
    syncUtil.send(doubleList);
    devIntOut->receive();
    devFloatOut->receive();
    BOOST_CHECK(*devIntOut == 57);
    BOOST_CHECK(*devFloatOut == 58.0f);
  }

  BOOST_AUTO_TEST_CASE( testReceiveAll ) {
    std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > pvManagers = createPVManager();
    boost::shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    boost::shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessScalar<int32_t>::SharedPtr devIntIn =
        devManager->createProcessScalar<int32_t>(deviceToControlSystem,
            "intIn");
    ProcessScalar<int32_t>::SharedPtr csIntIn = csManager->getProcessScalar<
        int32_t>("intIn");

    ControlSystemSynchronizationUtility syncUtil(csManager);

    *devIntIn = 5;
    devIntIn->send();
    syncUtil.receiveAll();
    BOOST_CHECK(*csIntIn == 5);
  }

  BOOST_AUTO_TEST_CASE( testSendAll ) {
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

    ControlSystemSynchronizationUtility syncUtil(csManager);

    *csIntOut = 15;
    *csFloatOut = 16.0f;
    syncUtil.sendAll();
    devIntOut->receive();
    devFloatOut->receive();
    BOOST_CHECK(*devIntOut == 15);
    BOOST_CHECK(*devFloatOut == 16.0f);
  }

  BOOST_AUTO_TEST_CASE( testReceiveNotificationListener ) {
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

    ControlSystemSynchronizationUtility syncUtil(csManager);

    boost::shared_ptr<CountingProcessVariableListener> receiveNotificationListener =
        boost::make_shared<CountingProcessVariableListener>();

    syncUtil.addReceiveNotificationListener("intIn",
        receiveNotificationListener);
    devIntIn->send();
    devFloatIn->send();
    syncUtil.receiveAll();
    BOOST_CHECK(receiveNotificationListener->count == 1);
    BOOST_CHECK(
        receiveNotificationListener->lastProcessVariable->getName() == "intIn");

    devIntIn->send();
    std::vector<ProcessVariable::SharedPtr> pvList(1);
    pvList[0] = csIntIn;
    syncUtil.receive(pvList);
    BOOST_CHECK(receiveNotificationListener->count == 2);
    BOOST_CHECK(
        receiveNotificationListener->lastProcessVariable->getName() == "intIn");

    syncUtil.removeReceiveNotificationListener("intIn");
    devIntIn->send();
    syncUtil.receiveAll();
    BOOST_CHECK(receiveNotificationListener->count == 2);
  }

// After you finished all test you have to end the test suite.
  BOOST_AUTO_TEST_SUITE_END()
