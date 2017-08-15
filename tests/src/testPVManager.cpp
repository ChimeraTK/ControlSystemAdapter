// Define a name for the test module.
#define BOOST_TEST_MODULE PVManagerTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include <vector>

#include <boost/chrono.hpp>
#include <boost/make_shared.hpp>

#include "ControlSystemPVManager.h"
#include "DevicePVManager.h"
#include "SynchronizationDirection.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

using boost::shared_ptr;
using std::list;
using std::map;
using std::pair;
using std::string;
using std::vector;

/**
 * Utility method for receiving a list of process variables.
 */
static void receiveAll(
    list<ProcessVariable::SharedPtr> const & processVariables) {
  for (list<ProcessVariable::SharedPtr>::const_iterator i =
      processVariables.begin(); i != processVariables.end(); ++i) {
    // Receive all pending values so that we can be sure that we have the most
    // up-to-date value.
    while ((*i)->readNonBlocking()) {
      continue;
    }
  }
}

/**
 * Utility method for sending a list of process variables.
 */
static void sendAll(list<ProcessVariable::SharedPtr> const & processVariables) {
  for (list<ProcessVariable::SharedPtr>::const_iterator i =
      processVariables.begin(); i != processVariables.end(); ++i) {
    (*i)->write();
  }
}

template<class T>
static void testCreateProcessVariables(const string& name,
    shared_ptr<DevicePVManager> devManager,
    shared_ptr<ControlSystemPVManager> csManager) {

  shared_ptr<ProcessArray<T> > createdPV = devManager->createProcessArray<T>(
      deviceToControlSystem, name + "In", 1, "kindOfAUnit","any description");
  // Although process variables are/ can be created without a leading '/', the
  // RegisterPath which is used assures that it is there. We have to test with '/'.
  BOOST_CHECK(createdPV->getName() == "/"+name + "In");
  BOOST_CHECK(createdPV->getUnit() == "kindOfAUnit");
  BOOST_CHECK(createdPV->getDescription() == "any description");

  shared_ptr<ProcessArray<T> > devPV = devManager->getProcessArray<T>(
      name + "In");
  BOOST_CHECK(devPV->getName() == "/"+name + "In");
  BOOST_CHECK(devPV->getUnit() == "kindOfAUnit");
  BOOST_CHECK(devPV->getDescription() == "any description");

  shared_ptr<ProcessArray<T> > csPV = csManager->getProcessArray<T>(
      name + "In");
  BOOST_CHECK(csPV->getName() == "/"+name + "In");
  BOOST_CHECK(csPV->getUnit() == "kindOfAUnit");
  BOOST_CHECK(csPV->getDescription() == "any description");

  createdPV = devManager->createProcessArray<T>(controlSystemToDevice,
      name + "Out", 1, "anotherUnit","something");
  BOOST_CHECK(createdPV->getName() == "/"+ name + "Out");
  BOOST_CHECK(createdPV->getUnit() == "anotherUnit");
  BOOST_CHECK(createdPV->getDescription() == "something");

  devPV = devManager->getProcessArray<T>(name + "Out");
  BOOST_CHECK(devPV->getName() == "/" + name + "Out");
  BOOST_CHECK(devPV->getUnit() == "anotherUnit");
  BOOST_CHECK(devPV->getDescription() == "something");

  csPV = csManager->getProcessArray<T>(name + "Out");
  BOOST_CHECK(csPV->getName() == "/" + name + "Out");
  BOOST_CHECK(csPV->getUnit() == "anotherUnit");
  BOOST_CHECK(csPV->getDescription() == "something");

  BOOST_CHECK_THROW( devManager->createProcessArray<T>(
   static_cast<SynchronizationDirection>(-1),name + "ShouldFail", 1),
   std::invalid_argument );

  string arrayName = name + "Array";
  shared_ptr<ProcessArray<T> > createdPA = devManager->createProcessArray<T>(
      deviceToControlSystem, arrayName + "In", 5);
  BOOST_CHECK(createdPA->getName() == "/" + arrayName + "In");
  BOOST_CHECK(createdPA->getUnit() == "n./a.");
  BOOST_CHECK(createdPA->getDescription() == "");
  shared_ptr<ProcessArray<T> > devPA = devManager->getProcessArray<T>(
      arrayName + "In");
  BOOST_CHECK(devPA->getName() == "/" + arrayName + "In");
  shared_ptr<ProcessArray<T> > csPA = csManager->getProcessArray<T>(
      arrayName + "In");
  BOOST_CHECK(csPA->getName() == "/" + arrayName + "In");
  createdPA = devManager->createProcessArray<T>(controlSystemToDevice,
      arrayName + "Out", 5);
  BOOST_CHECK(createdPA->getName() == "/" + arrayName + "Out");
  devPA = devManager->getProcessArray<T>(arrayName + "Out");
  BOOST_CHECK(devPA->getName() == "/" + arrayName + "Out");
  csPA = csManager->getProcessArray<T>(arrayName + "Out");
  BOOST_CHECK(csPA->getName() == "/" + arrayName + "Out");

  BOOST_CHECK_THROW( devManager->createProcessArray<T>(
   static_cast<SynchronizationDirection>(-1),arrayName + "ShouldFail", 5),
   std::invalid_argument );

}

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE( PVManagerTestSuite )

// Define one test after another. Each one needs a unique name.
  BOOST_AUTO_TEST_CASE( testConstructor ) {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();
  }

  BOOST_AUTO_TEST_CASE( testCreatePVs ) {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    testCreateProcessVariables<int8_t>("int8", devManager, csManager);
    testCreateProcessVariables<uint8_t>("uint8", devManager, csManager);
    testCreateProcessVariables<int16_t>("int16", devManager, csManager);
    testCreateProcessVariables<uint16_t>("uint16", devManager, csManager);
    testCreateProcessVariables<int32_t>("int32", devManager, csManager);
    testCreateProcessVariables<uint32_t>("uint32", devManager, csManager);
    testCreateProcessVariables<float>("float", devManager, csManager);
    testCreateProcessVariables<double>("double", devManager, csManager);
  }

  BOOST_AUTO_TEST_CASE( testDoublePVName ) {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    devManager->createProcessArray<double>(deviceToControlSystem, "double", 1);
    // We expect a bad_argument exception to be thrown because the specified
    // PV name is already used.
    BOOST_CHECK_THROW(
        devManager->createProcessArray<float>(controlSystemToDevice, "double", 1),
        std::invalid_argument);
    BOOST_CHECK_THROW(
        devManager->createProcessArray<double>(deviceToControlSystem,
            "double", 1), std::invalid_argument);
  }

  BOOST_AUTO_TEST_CASE( testNonExistentPVName ) {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    BOOST_CHECK(!devManager->getProcessArray<double>("foo"));
    BOOST_CHECK(!csManager->getProcessArray<double>("foo"));
  }

  BOOST_AUTO_TEST_CASE( testInvalidCast ) {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    devManager->createProcessArray<double>(deviceToControlSystem, "double", 1);
    devManager->createProcessArray<float>(controlSystemToDevice, "floatArray", 10);
    // We expect a bad_cast exception to be thrown because the specified
    // PV name points to a PV of a different type.
    BOOST_CHECK_THROW(devManager->getProcessArray<float>("double"),
        std::bad_cast);
    BOOST_CHECK_THROW(csManager->getProcessArray<float>("double"),
        std::bad_cast);
    BOOST_CHECK_THROW(devManager->getProcessArray<double>("floatArray"),
        std::bad_cast);
    BOOST_CHECK_THROW(csManager->getProcessArray<double>("floatArray"),
        std::bad_cast);
  }

  template<class T>
  static void checkControlSystemPVMap(vector<T> processVariables) {
    bool foundDouble = false, foundInt32 = false, foundFloatArray = false;
    for (typename vector<T>::const_iterator i = processVariables.begin();
        i != processVariables.end(); i++) {
      T pv = *i;
      const std::string& name = pv->getName();
      if (name == "/double") {
        BOOST_CHECK(pv->getValueType() == typeid(double));
        auto pvc = boost::dynamic_pointer_cast<ProcessArray<double>, typename T::element_type>(pv);
        BOOST_CHECK(pvc->isArray() == true);
        BOOST_CHECK(pvc->getNumberOfSamples() == 1);
        BOOST_CHECK(pvc->getNumberOfChannels() == 1);
        foundDouble = true;
      } else if (name == "/int32") {
        BOOST_CHECK(pv->getValueType() == typeid(int32_t));
        auto pvc = boost::dynamic_pointer_cast<ProcessArray<int32_t>, typename T::element_type>(pv);
        BOOST_CHECK(pvc->isArray() == true);
        BOOST_CHECK(pvc->getNumberOfSamples() == 1);
        BOOST_CHECK(pvc->getNumberOfChannels() == 1);
        foundInt32 = true;
      } else if (name == "/floatArray") {
        BOOST_CHECK(pv->getValueType() == typeid(float));
        auto pvc = boost::dynamic_pointer_cast<ProcessArray<float>, typename T::element_type>(pv);
        BOOST_CHECK(pvc->isArray() == true);
        BOOST_CHECK(pvc->getNumberOfSamples() == 10);
        BOOST_CHECK(pvc->getNumberOfChannels() == 1);
        foundFloatArray = true;
      } else {
        BOOST_FAIL(
            "Iterator returned a process variable that has not been created.");
      }
    }
    BOOST_CHECK(foundDouble && foundInt32 && foundFloatArray);
  }

  template<class T>
  static void checkDevicePVMap(vector<T> processVariables) {
    bool foundDouble = false, foundInt32 = false, foundFloatArray = false;
    for (typename vector<T>::const_iterator i = processVariables.begin();
        i != processVariables.end(); i++) {
      T pv = *i;
      const std::string& name = pv->getName();
      if (name == "/double") {
        BOOST_CHECK(pv->getValueType() == typeid(double));
        auto pvc = boost::dynamic_pointer_cast<ProcessArray<double>, typename T::element_type>(pv);
        BOOST_CHECK(pvc->isArray() == true);
        BOOST_CHECK(pvc->getNumberOfSamples() == 1);
        BOOST_CHECK(pvc->getNumberOfChannels() == 1);
        foundDouble = true;
      } else if (name == "/int32") {
        BOOST_CHECK(pv->getValueType() == typeid(int32_t));
        auto pvc = boost::dynamic_pointer_cast<ProcessArray<int32_t>, typename T::element_type>(pv);
        BOOST_CHECK(pvc->isArray() == true);
        BOOST_CHECK(pvc->getNumberOfSamples() == 1);
        BOOST_CHECK(pvc->getNumberOfChannels() == 1);
        foundInt32 = true;
      } else if (name == "/floatArray") {
        BOOST_CHECK(pv->isArray() == true);
        boost::dynamic_pointer_cast<ProcessArray<float>, typename T::element_type>(pv);
        auto pvc = boost::dynamic_pointer_cast<ProcessArray<float>, typename T::element_type>(pv);
        BOOST_CHECK(pvc->isArray() == true);
        BOOST_CHECK(pvc->getNumberOfSamples() == 10);
        BOOST_CHECK(pvc->getNumberOfChannels() == 1);
        foundFloatArray = true;
      } else {
        BOOST_FAIL(
            "Iterator returned a process variable that has not been created.");
      }
    }
    BOOST_CHECK(foundDouble && foundInt32 && foundFloatArray);
  }

  BOOST_AUTO_TEST_CASE( testAllPVIterator ) {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    devManager->createProcessArray<double>(controlSystemToDevice, "double", 1);
    devManager->createProcessArray<int32_t>(deviceToControlSystem, "int32", 1);
    devManager->createProcessArray<float>(deviceToControlSystem, "floatArray", 10);

    vector<ProcessVariable::SharedPtr> csProcessVariables(
        csManager->getAllProcessVariables());
    checkControlSystemPVMap(csProcessVariables);
    vector<ProcessVariable::SharedPtr> devProcessVariables(
        devManager->getAllProcessVariables());
    checkDevicePVMap(devProcessVariables);
  }

  struct TestDeviceCallable {
    shared_ptr<DevicePVManager> pvManager;

    void operator()() {
      ProcessArray<int32_t>::SharedPtr int32In = pvManager->getProcessArray<
          int32_t>("int32In");
      ProcessArray<int32_t>::SharedPtr int32Out = pvManager->getProcessArray<
          int32_t>("int32Out");
      ProcessArray<float>::SharedPtr floatArrayIn = pvManager->getProcessArray<
          float>("floatArrayIn");
      ProcessArray<float>::SharedPtr floatArrayOut = pvManager->getProcessArray<
          float>("floatArrayOut");
      ProcessArray<int8_t>::SharedPtr stopDeviceThread =
          pvManager->getProcessArray<int8_t>("stopDeviceThread");

      int32In->accessData(0) = 0;
      int32Out->accessData(0) = 0;
      floatArrayIn->accessChannel(0) = vector<float>(10, 0.0);
      floatArrayOut->accessChannel(0) = vector<float>(10, 0.0);
      stopDeviceThread->accessData(0) = 0;

      while (stopDeviceThread->accessData(0) == 0) {
        int32In->accessChannel(0) = int32Out->accessChannel(0);
        floatArrayIn->accessChannel(0) = floatArrayOut->accessChannel(0);
        int32In->write();
        floatArrayIn->write();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        stopDeviceThread->readNonBlocking();
        int32Out->readNonBlocking();
        floatArrayOut->readNonBlocking();
      }
    }
  };

  static shared_ptr<ControlSystemPVManager> initTestDeviceLib() {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    devManager->createProcessArray<int32_t>(deviceToControlSystem, "int32In", 1);
    devManager->createProcessArray<int32_t>(controlSystemToDevice, "int32Out", 1);
    devManager->createProcessArray<float>(deviceToControlSystem, "floatArrayIn", 10);
    devManager->createProcessArray<float>(controlSystemToDevice, "floatArrayOut", 10);
    devManager->createProcessArray<int8_t>(controlSystemToDevice, "stopDeviceThread", 1);

    TestDeviceCallable callable;
    callable.pvManager = devManager;

    // Start device thread.
    boost::thread deviceThread(callable);

    return csManager;
  }

  BOOST_AUTO_TEST_CASE( testSynchronization ) {
    shared_ptr<ControlSystemPVManager> pvManager = initTestDeviceLib();

    ProcessArray<int32_t>::SharedPtr int32In = pvManager->getProcessArray<
        int32_t>("int32In");
    ProcessArray<int32_t>::SharedPtr int32Out = pvManager->getProcessArray<
        int32_t>("int32Out");
    ProcessArray<float>::SharedPtr floatArrayIn = pvManager->getProcessArray<
        float>("floatArrayIn");
    ProcessArray<float>::SharedPtr floatArrayOut = pvManager->getProcessArray<
        float>("floatArrayOut");
    ProcessArray<int8_t>::SharedPtr stopDeviceThread =
        pvManager->getProcessArray<int8_t>("stopDeviceThread");

    list<ProcessVariable::SharedPtr> inboundProcessVariables;
    inboundProcessVariables.push_back(int32In);
    inboundProcessVariables.push_back(floatArrayIn);

    list<ProcessVariable::SharedPtr> outboundProcessVariables;
    outboundProcessVariables.push_back(int32Out);
    outboundProcessVariables.push_back(floatArrayOut);
    outboundProcessVariables.push_back(stopDeviceThread);

    int32Out->accessData(0) = 55;
    floatArrayOut->accessChannel(0).at(0) = 1.0f;
    floatArrayOut->accessChannel(0).at(1) = 2.0f;
    floatArrayOut->accessChannel(0).at(2) = -1.3f;

    // Send the values, wait a moment for the other thread to send the updates
    // and then receive the new values.
    sendAll(outboundProcessVariables);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(150));
    receiveAll(inboundProcessVariables);

    BOOST_CHECK(int32In->accessData(0) == 55);
    BOOST_CHECK(floatArrayIn->accessChannel(0).at(0) == 1.0f);
    BOOST_CHECK(floatArrayIn->accessChannel(0).at(1) == 2.0f);
    BOOST_CHECK(floatArrayIn->accessChannel(0).at(2) == -1.3f);

    int32Out->accessData(0) = -300;
    floatArrayOut->accessChannel(0).at(0) = 15.0f;
    floatArrayOut->accessChannel(0).at(1) = -7.2f;
    floatArrayOut->accessChannel(0).at(9) = 120.0f;

    // Send the values, wait a moment for the other thread to send the updates
    // and then receive the new values.
    sendAll(outboundProcessVariables);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(150));
    receiveAll(inboundProcessVariables);

    BOOST_CHECK(int32In->accessData(0) == -300);
    BOOST_CHECK(floatArrayIn->accessChannel(0).at(0) == 15.0f);
    BOOST_CHECK(floatArrayIn->accessChannel(0).at(1) == -7.2f);
    BOOST_CHECK(floatArrayIn->accessChannel(0).at(9) == 120.0f);

    stopDeviceThread->accessData(0) = 1;
    stopDeviceThread->write();
  }

  struct TestDeviceCallable2 {
    shared_ptr<DevicePVManager> pvManager;

    void operator()() {
      ProcessArray<int32_t>::SharedPtr int32In = pvManager->getProcessArray<
          int32_t>("int32In");
      ProcessArray<double>::SharedPtr doubleIn = pvManager->getProcessArray<
          double>("doubleIn");
      ProcessArray<int8_t>::SharedPtr stopDeviceThread =
          pvManager->getProcessArray<int8_t>("stopDeviceThread");

      int i = 0;
      while (stopDeviceThread->accessData(0) == 0) {
        if (i == 0) {
          // Flood notifications for first process variable before sending a
          // notification for a second one. This tests the mechanism that
          // prevents the queue from being filled with notifications for a
          // single process variable.
          while (i < 50000) {
            int32In->accessData(0) = i;
            ++i;
            int32In->write();
          }
          doubleIn->accessData(0) = 2.0;
          doubleIn->write();
        } else {
          // After the flood test, we send notifications in regular intervals
          // to ensure that the reset mechanism works (multiple notifications
          // can be sent if they are collected).
          int32In->accessData(0) = 55;
          int32In->write();
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        stopDeviceThread->readNonBlocking();
      }
    }
  };

  static shared_ptr<ControlSystemPVManager> initTestDeviceLib2() {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    devManager->createProcessArray<int32_t>(deviceToControlSystem, "int32In", 1);
    devManager->createProcessArray<double>(deviceToControlSystem, "doubleIn", 1);
    devManager->createProcessArray<int8_t>(controlSystemToDevice, "stopDeviceThread", 1);

    TestDeviceCallable2 callable;
    callable.pvManager = devManager;

    // Start device thread.
    boost::thread deviceThread(callable);

    return csManager;
  }

  BOOST_AUTO_TEST_CASE( testNotificationToControlSystem ) {
    shared_ptr<ControlSystemPVManager> pvManager = initTestDeviceLib2();

    ProcessArray<int32_t>::SharedPtr int32In = pvManager->getProcessArray<
        int32_t>("int32In");
    ProcessArray<double>::SharedPtr doubleIn = pvManager->getProcessArray<
        double>("doubleIn");
    ProcessArray<int8_t>::SharedPtr stopDeviceThread =
        pvManager->getProcessArray<int8_t>("stopDeviceThread");

    int int32NotificationCount = 0;
    int doubleNotificationCount = 0;
    boost::chrono::system_clock::time_point start =
        boost::chrono::system_clock::now();
    boost::chrono::seconds maxWait(10);
    while ((int32NotificationCount < 5 || doubleNotificationCount < 1)
        && boost::chrono::system_clock::now() - start < maxWait) {
      ProcessVariable::SharedPtr pv = pvManager->nextNotification();
      if (pv) {
        const std::string& name = pv->getName();
        if (name == "/int32In") {
          ++int32NotificationCount;
        } else if (name == "/doubleIn") {
          ++doubleNotificationCount;
        } else {
          BOOST_FAIL("Unexpected notification.");
        }
      } else {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
      }
    }
    BOOST_CHECK(int32NotificationCount >= 5);
    BOOST_CHECK(doubleNotificationCount >= 1);

    stopDeviceThread->accessData(0) = 1;
    stopDeviceThread->write();
  }

  struct TestDeviceCallable3 {
    shared_ptr<DevicePVManager> pvManager;

    void operator()() {
      ProcessArray<int32_t>::SharedPtr int32In = pvManager->getProcessArray<
          int32_t>("int32Out");
      ProcessArray<double>::SharedPtr doubleIn = pvManager->getProcessArray<
          double>("doubleOut");
      ProcessArray<int8_t>::SharedPtr stopControlSystemThread =
          pvManager->getProcessArray<int8_t>("stopControlSystemThread");

      int int32NotificationCount = 0;
      int doubleNotificationCount = 0;
      boost::chrono::system_clock::time_point start =
          boost::chrono::system_clock::now();
      boost::chrono::seconds maxWait(10);
      while ((int32NotificationCount < 5 || doubleNotificationCount < 1)
          && boost::chrono::system_clock::now() - start < maxWait) {
        ProcessVariable::SharedPtr pv = pvManager->nextNotification();
        if (pv) {
          const std::string& name = pv->getName();
          if (name == "/int32Out") {
            ++int32NotificationCount;
          } else if (name == "/doubleOut") {
            ++doubleNotificationCount;
          } else {
            BOOST_FAIL("Unexpected notification.");
          }
        } else {
          boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        }
      }
      BOOST_CHECK(int32NotificationCount >= 5);
      BOOST_CHECK(doubleNotificationCount >= 1);

      stopControlSystemThread->accessData(0) = 1;
      stopControlSystemThread->write();
    }
  };

  static shared_ptr<ControlSystemPVManager> initTestDeviceLib3() {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    devManager->createProcessArray<int32_t>(controlSystemToDevice, "int32Out", 1);
    devManager->createProcessArray<double>(controlSystemToDevice, "doubleOut", 1);
    devManager->createProcessArray<int8_t>(deviceToControlSystem, "stopControlSystemThread", 1);

    TestDeviceCallable3 callable;
    callable.pvManager = devManager;

    // Start device thread.
    boost::thread deviceThread(callable);

    return csManager;
  }

  BOOST_AUTO_TEST_CASE( testNotificationToDevice ) {
    shared_ptr<ControlSystemPVManager> pvManager = initTestDeviceLib3();

    ProcessArray<int32_t>::SharedPtr int32Out = pvManager->getProcessArray<
        int32_t>("int32Out");
    ProcessArray<double>::SharedPtr doubleOut = pvManager->getProcessArray<
        double>("doubleOut");
    ProcessArray<int8_t>::SharedPtr stopControlSystemThread =
        pvManager->getProcessArray<int8_t>("stopControlSystemThread");

    int i = 0;
    while (stopControlSystemThread->accessData(0) == 0) {
      if (i == 0) {
        // Flood notifications for first process variable before sending a
        // notification for a second one. This tests the mechanism that
        // prevents the queue from being filled with notifications for a
        // single process variable.
        while (i < 50000) {
          int32Out->accessData(0) = i;
          ++i;
          int32Out->write();
        }
        doubleOut->accessData(0) = 2.0;
        doubleOut->write();
      } else {
        // After the flood test, we send notifications in regular intervals
        // to ensure that the reset mechanism works (multiple notifications
        // can be sent if they are collected).
        int32Out->accessData(0) = 55;
        int32Out->write();
      }
      boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
      stopControlSystemThread->readNonBlocking();
    }
  }

  struct TestDeviceCallable4 {
    shared_ptr<DevicePVManager> pvManager;

    void operator()() {
      ProcessArray<uint32_t>::SharedPtr intA = pvManager->getProcessArray<
          uint32_t>("intA");
      ProcessArray<uint32_t>::SharedPtr intB = pvManager->getProcessArray<
          uint32_t>("intB");
      ProcessArray<uint32_t>::SharedPtr index0 = pvManager->getProcessArray<
          uint32_t>("index0");
      ProcessArray<int8_t>::SharedPtr stopDeviceThread =
          pvManager->getProcessArray<int8_t>("stopDeviceThread");

      uint32_t nextIndexNumber = 0;

      // By default, the PV manager should be in automatic time-stamp mode.
      BOOST_CHECK(pvManager->isAutomaticReferenceTimeStampMode());

      pvManager->setReferenceTimeStamp(TimeStamp::currentTime(nextIndexNumber));
      index0->accessData(0) = nextIndexNumber;
      ++nextIndexNumber;

      // Setting a reference time-stamp should implicitly disable the automatic
      // mode.
      BOOST_CHECK(!pvManager->isAutomaticReferenceTimeStampMode());

      intA->accessData(0) = 0;
      intB->accessData(0) = 0;

      while (stopDeviceThread->accessData(0) == 0) {
        // On each iteration, we set the current time-stamp. This should ensure
        // that all PVs receive the same time-stamp.
        pvManager->setReferenceTimeStamp(
            TimeStamp::currentTime(nextIndexNumber));
        index0->accessData(0) = nextIndexNumber;
        ++nextIndexNumber;

        intA->accessData(0) = intA->accessData(0) + 2;
        intB->accessData(0) = intB->accessData(0) + 2;

        index0->write();
        intA->write();
        intB->write();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
        stopDeviceThread->readNonBlocking();
      }
    }
  };

  static shared_ptr<ControlSystemPVManager> initTestDeviceLib4() {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    devManager->createProcessArray<uint32_t>(deviceToControlSystem, "intA", 1);
    devManager->createProcessArray<uint32_t>(deviceToControlSystem, "intB", 1);
    devManager->createProcessArray<uint32_t>(deviceToControlSystem, "index0", 1);
    devManager->createProcessArray<int8_t>(controlSystemToDevice, "stopDeviceThread", 1);

    TestDeviceCallable4 callable;
    callable.pvManager = devManager;

    // Start device thread.
    boost::thread deviceThread(callable);

    return csManager;
  }

  BOOST_AUTO_TEST_CASE( testTimeStamps ) {
    shared_ptr<ControlSystemPVManager> pvManager = initTestDeviceLib4();

    ProcessArray<uint32_t>::SharedPtr intA = pvManager->getProcessArray<
        uint32_t>("intA");
    ProcessArray<uint32_t>::SharedPtr intB = pvManager->getProcessArray<
        uint32_t>("intB");
    ProcessArray<uint32_t>::SharedPtr index0 = pvManager->getProcessArray<
        uint32_t>("index0");
    ProcessArray<int8_t>::SharedPtr stopDeviceThread =
        pvManager->getProcessArray<int8_t>("stopDeviceThread");

    list<ProcessVariable::SharedPtr> inboundProcessVariables;
    inboundProcessVariables.push_back(intA);
    inboundProcessVariables.push_back(intB);
    inboundProcessVariables.push_back(index0);

    while (index0->accessData(0) < 150) {
      uint32_t i0 = index0->accessData(0);
      uint32_t a = intA->accessData(0);
      uint32_t b = intB->accessData(0);
      TimeStamp i0TS = index0->getTimeStamp();
      TimeStamp aTS = intA->getTimeStamp();
      TimeStamp bTS = intB->getTimeStamp();
      // If the numbers match, the time stamps should match and vice versa.
      BOOST_CHECK((a == b) == (aTS == bTS));
      // The time-stamp of index0 should always have exactly the same number for
      // index0.
      BOOST_CHECK(i0 == i0TS.index0);
      // If intA has a smaller index than intB, it should also be smaller than
      // intB (actually two times the index difference smaller) and vice versa.
      if (a < b) {
        BOOST_CHECK(aTS.index0 < bTS.index0);
        BOOST_CHECK(b == (a + (bTS.index0 - aTS.index0) * 2));
      } else {
        BOOST_CHECK(aTS.index0 >= bTS.index0);
        BOOST_CHECK(a == (b + (aTS.index0 - bTS.index0) * 2));
      }

      // Sleep some time in order to avoid 100 % CPU load.
      boost::this_thread::sleep_for(boost::chrono::milliseconds(1));

      // Synchronize before starting next iteration.
      receiveAll(inboundProcessVariables);
    }

    stopDeviceThread->accessData(0) = 1;
    stopDeviceThread->write();
  }

  BOOST_AUTO_TEST_CASE( automaticTimeStampMode ) {
    pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > pvManagers =
        createPVManager();

    shared_ptr<ControlSystemPVManager> csManager = pvManagers.first;
    shared_ptr<DevicePVManager> devManager = pvManagers.second;

    ProcessArray<int32_t>::SharedPtr intAdev = devManager->createProcessArray<
        int32_t>(deviceToControlSystem, "intA", 1);
    ProcessArray<int32_t>::SharedPtr intBdev = devManager->createProcessArray<
        int32_t>(deviceToControlSystem, "intB", 1, "kindOfAUnit", "any description");
    ProcessArray<int32_t>::SharedPtr intAcs = csManager->getProcessArray<
        int32_t>("intA");
    ProcessArray<int32_t>::SharedPtr intBcs = csManager->getProcessArray<
        int32_t>("intB");

    // By default, the PV manager should be in automatic time-stamp mode.
    BOOST_CHECK(devManager->isAutomaticReferenceTimeStampMode());

    intAdev->write();

    // We sleep slightly more than a second, this should ensure that the time
    // changes even on systems that do not have a high precision timer.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1100));

    intBdev->write();

    // intB should have a time stamp that is greater than the time stamp of
    // intA.
    intAcs->readNonBlocking();
    intBcs->readNonBlocking();
    BOOST_CHECK(
        intAcs->getTimeStamp().seconds < intBcs->getTimeStamp().seconds);
  }

  // After you finished all test you have to end the test suite.
  BOOST_AUTO_TEST_SUITE_END()
