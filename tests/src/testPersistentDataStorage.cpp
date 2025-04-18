#define BOOST_TEST_MODULE PersistentDataStorageTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>
#include <ChimeraTK/ControlSystemAdapter/ControlSystemPVManager.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>
#include <ChimeraTK/ControlSystemAdapter/PersistentDataStorage.h>

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

using namespace ChimeraTK;

// define empty test application to fullfill the requirement of having an instance
class MyTestApplication : public ApplicationBase {
 public:
  using ApplicationBase::ApplicationBase;
  ~MyTestApplication() override { shutdown(); }
  void initialise() override {}
  void run() override {}
};

BOOST_AUTO_TEST_SUITE(PersistentDataStorageTestSuite)

/**
 *  @todo FIXME No exception tests are yet done. They should be implemented asap!
 */
// Test storing data in first instance of PersistentDataStorage and retrieveing it in a second instance
BOOST_AUTO_TEST_CASE(testStoreAndRetrieve) {
  // remove persistency file from previous test run
  boost::filesystem::remove("myTestApplication.persist");

  // create PersistentDataStorage for the first time and fill it with two variables
  {
    PersistentDataStorage storage{"myTestApplication"};

    // register integer variable MyVar1 with 10 array elements and fill it
    std::vector<int32_t> myVar1(10);
    int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
    for(int i = 0; i < 10; ++i) {
      myVar1[i] = 3 * i;
    }
    storage.updateValue(id1, myVar1);

    // register floating-point variable /some/path.with.dots/to/MyVar2 with 100 array elements and fill it
    std::vector<double> myVar2(100);
    int id2 = storage.registerVariable<double>("/some/path.with.dots/to/MyVar2", 100);
    for(int i = 0; i < 100; ++i) {
      myVar2[i] = -120 + 7 * i;
    }
    storage.updateValue(id2, myVar2);

    // register Boolean variable /bool/MyVar3 with 4 elements and fill it
    std::vector<Boolean> myVar3 = {true, false, true, false};
    int id3 = storage.registerVariable<Boolean>("/bool/MyVar3", 4);
    storage.updateValue(id3, myVar3);

    // check if both variables are properly stored
    auto myVar1stored = storage.retrieveValue<int32_t>(id1);
    for(int i = 0; i < 10; ++i) {
      BOOST_CHECK(myVar1stored[i] == 3 * i);
    }
    auto myVar2stored = storage.retrieveValue<double>(id2);
    for(int i = 0; i < 100; ++i) {
      BOOST_CHECK_CLOSE(myVar2stored[i], -120 + 7 * i, 0.0001);
    }
    auto myVar3stored = storage.retrieveValue<Boolean>(id3);
    BOOST_CHECK_EQUAL_COLLECTIONS(myVar3.begin(), myVar3.end(), myVar3stored.begin(), myVar3stored.end());
  }
  // the first PersistentDataStorage is destroyed at this point, in its destructor the file is written

  // create another PersistentDataStorage and read the previously stored variables from it
  {
    PersistentDataStorage storage{"myTestApplication"};

    // check MyVar1
    std::vector<int32_t> myVar1(10);
    int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
    myVar1 = storage.retrieveValue<int32_t>(id1);
    for(int i = 0; i < 10; ++i) {
      BOOST_CHECK(myVar1[i] == 3 * i);
    }

    // check /some/path.with.dots/to/MyVar2
    std::vector<double> myVar2(100);
    int id2 = storage.registerVariable<double>("/some/path.with.dots/to/MyVar2", 100);
    myVar2 = storage.retrieveValue<double>(id2);
    for(int i = 0; i < 100; ++i) {
      BOOST_CHECK_CLOSE(myVar2[i], -120 + 7 * i, 0.0001);
    }

    // check /bool/MyVar3
    std::vector<Boolean> myVar3(4);
    int id3 = storage.registerVariable<Boolean>("/bool/MyVar3", 4);
    myVar3 = storage.retrieveValue<Boolean>(id3);
    std::vector<Boolean> test = {true, false, true, false};
    BOOST_CHECK_EQUAL_COLLECTIONS(myVar3.begin(), myVar3.end(), test.begin(), test.end());

    // modify some elements of myVar1
    myVar1[7] = 42;
    myVar1[3] = 120;
    storage.updateValue(id1, myVar1);
  }

  // create another PersistentDataStorage and read the previously stored variables from it
  {
    PersistentDataStorage storage{"myTestApplication"};

    // check MyVar1
    std::vector<int32_t> myVar1(10);
    int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
    myVar1 = storage.retrieveValue<int32_t>(id1);
    for(int i = 0; i < 10; ++i) {
      if(i == 3) {
        BOOST_CHECK(myVar1[i] == 120);
      }
      else if(i == 7) {
        BOOST_CHECK(myVar1[i] == 42);
      }
      else {
        BOOST_CHECK(myVar1[i] == 3 * i);
      }
    }

    // check /some/path.with.dots/to/MyVar2
    std::vector<double> myVar2(100);
    int id2 = storage.registerVariable<double>("/some/path.with.dots/to/MyVar2", 100);
    myVar2 = storage.retrieveValue<double>(id2);
    for(int i = 0; i < 100; ++i) {
      BOOST_CHECK_CLOSE(myVar2[i], -120 + 7 * i, 0.0001);
    }

    // modify some elements of myVar1
    myVar1[7] = 42;
    myVar1[3] = 120;
    storage.updateValue(id1, myVar1);
  }
}

// test integration in PVManager
BOOST_AUTO_TEST_CASE(testUsageInPVManager) {
  // remove persistency file from previous test run
  boost::filesystem::remove("myTestApplication.persist");

  // first application instance: initialise the variables with some values and store them in the persistency file
  {
    // create instance of test application
    MyTestApplication myTestApplication{"myTestApplication"};

    // create PV managers
    auto pvManagers = createPVManager();
    auto csManager = pvManagers.first;
    auto devManager = pvManagers.second;

    // create some variables, incl. bi-directional
    devManager->createProcessArray<uint16_t>(SynchronizationDirection::controlSystemToDevice, "SomeCsToDevVar", 7);
    devManager->createProcessArray<float>(SynchronizationDirection::controlSystemToDevice, "AnotherCsToDevVar", 42);
    devManager->createProcessArray<int32_t>(SynchronizationDirection::deviceToControlSystem, "SomeDevToCsVar", 7);
    devManager->createProcessArray<uint32_t>(SynchronizationDirection::bidirectional, "SomeBidirectionalVar", 7);

    // enable persist data storage
    csManager->enablePersistentDataStorage();

    // obtain the process variables, send some values to the variables
    auto v1 = csManager->getProcessArray<uint16_t>("SomeCsToDevVar");
    for(int i = 0; i < 7; ++i) {
      v1->accessChannel(0)[i] = i * 17;
    }
    v1->write();

    auto v2 = csManager->getProcessArray<float>("AnotherCsToDevVar");
    for(int i = 0; i < 42; ++i) {
      v2->accessChannel(0)[i] = i * 3.1415 * 1e12;
    }
    v2->write();

    auto v3 = devManager->getProcessArray<int32_t>("SomeDevToCsVar"); // this one won't get stored
    for(int i = 0; i < 7; ++i) {
      v3->accessChannel(0)[i] = 9 * i + 666;
    }
    v3->write();

    auto v4 = csManager->getProcessArray<uint32_t>("SomeBidirectionalVar");
    for(uint32_t i = 0; i < 7; ++i) {
      v4->accessChannel(0)[i] = i + 123;
    }
    v4->write();
  }

  // second application instance: check if stored values are properly retrieved
  {
    // create instance of test application
    MyTestApplication myTestApplication{"myTestApplication"};

    // create PV managers
    auto pvManagers = createPVManager();
    auto csManager = pvManagers.first;
    auto devManager = pvManagers.second;

    // create some variables
    devManager->createProcessArray<uint16_t>(SynchronizationDirection::controlSystemToDevice, "SomeCsToDevVar", 7);
    devManager->createProcessArray<float>(SynchronizationDirection::controlSystemToDevice, "AnotherCsToDevVar", 42);
    devManager->createProcessArray<int32_t>(SynchronizationDirection::deviceToControlSystem, "SomeDevToCsVar", 7);
    devManager->createProcessArray<uint32_t>(SynchronizationDirection::bidirectional, "SomeBidirectionalVar", 7);

    // enable persist data storage
    csManager->enablePersistentDataStorage();

    // obtain all variables from the manager to initialise them with the persistent data storage
    csManager->getAllProcessVariables();

    // obtain the process variables, send some values to the variables
    auto v1 = devManager->getProcessArray<uint16_t>("SomeCsToDevVar");
    v1->readNonBlocking();
    for(int i = 0; i < 7; ++i) {
      BOOST_CHECK(v1->accessChannel(0)[i] == i * 17);
    }

    auto v2 = devManager->getProcessArray<float>("AnotherCsToDevVar");
    v2->readNonBlocking();
    for(int i = 0; i < 42; ++i) {
      BOOST_CHECK_CLOSE(v2->accessChannel(0)[i], i * 3.1415 * 1e30, 2e23);
    }

    auto v3 = csManager->getProcessArray<int32_t>("SomeDevToCsVar"); // this one won't get stored
    v3->readNonBlocking();
    for(int i = 0; i < 7; ++i) {
      BOOST_CHECK(v3->accessChannel(0)[i] == 0);
    }

    auto v4dev = devManager->getProcessArray<uint32_t>("SomeBidirectionalVar");
    v4dev->readLatest();
    for(uint32_t i = 0; i < 7; ++i) {
      BOOST_CHECK_EQUAL(v4dev->accessChannel(0)[i], i + 123);
    }

    // now test that also writing from the device goes to the persistency
    for(uint32_t i = 0; i < 7; ++i) {
      v4dev->accessChannel(0)[i] = i * 12;
    }
    v4dev->write();

    // data is only persisted once it arrived in the control system. So we have to read it from the control system side.
    auto v4cs = csManager->getProcessArray<uint32_t>("SomeBidirectionalVar");
    v4cs->readLatest();
  }

  // check in another instance that also the data written from the device side has been persisted.
  {
    // create instance of test application
    MyTestApplication myTestApplication{"myTestApplication"};

    // create PV managers
    auto pvManagers = createPVManager();
    auto csManager = pvManagers.first;
    auto devManager = pvManagers.second;

    // create some variables
    devManager->createProcessArray<uint32_t>(SynchronizationDirection::bidirectional, "SomeBidirectionalVar", 7);

    // enable persist data storage
    csManager->enablePersistentDataStorage();

    // obtain all variables from the manager to initialise them with the persistent data storage
    csManager->getAllProcessVariables();

    auto v4cs = csManager->getProcessArray<uint32_t>("SomeBidirectionalVar");
    auto v4dev = devManager->getProcessArray<uint32_t>("SomeBidirectionalVar");
    v4cs->readNonBlocking();
    v4dev->readNonBlocking();
    for(uint32_t i = 0; i < 7; ++i) {
      BOOST_CHECK_EQUAL(v4cs->accessChannel(0)[i], i * 12);
      BOOST_CHECK_EQUAL(v4dev->accessChannel(0)[i], i * 12);
    }
  }
}

size_t countLinesInFile(const std::string& filename) {
  std::ifstream inFile(filename);
  return std::count(std::istreambuf_iterator<char>(inFile), std::istreambuf_iterator<char>(), '\n');
}

// test opening .persist files which contain a different variable household
BOOST_AUTO_TEST_CASE(testChangedVariableHousehold) {
  // check with changed data type
  boost::filesystem::remove("myTestApplication.persist");
  boost::filesystem::copy("changedType.persist", "myTestApplication.persist");

  // check if stored values are properly retrieved
  {
    // create instance of test application
    MyTestApplication myTestApplication{"myTestApplication"};

    // create PV managers
    auto pvManagers = createPVManager();
    auto csManager = pvManagers.first;
    auto devManager = pvManagers.second;

    // create some variables
    devManager->createProcessArray<uint16_t>(SynchronizationDirection::controlSystemToDevice, "SomeCsToDevVar", 7);
    devManager->createProcessArray<float>(SynchronizationDirection::controlSystemToDevice, "AnotherCsToDevVar", 42);
    devManager->createProcessArray<int32_t>(SynchronizationDirection::deviceToControlSystem, "SomeDevToCsVar", 7);

    // enable persist data storage
    csManager->enablePersistentDataStorage();

    // obtain all variables from the manager to initialise them with the
    // persistent data storage
    csManager->getAllProcessVariables();

    // obtain the process variables, send some values to the variables
    auto v1 = devManager->getProcessArray<uint16_t>("SomeCsToDevVar");
    v1->readNonBlocking();
    for(int i = 0; i < 7; ++i) {
      BOOST_CHECK_EQUAL(v1->accessChannel(0)[i], 0);
    }

    auto v2 = devManager->getProcessArray<float>("AnotherCsToDevVar");
    v2->readNonBlocking();
    for(int i = 0; i < 42; ++i) {
      BOOST_CHECK_CLOSE(v2->accessChannel(0)[i], 0, 0.00001);
    }

    auto v3 = csManager->getProcessArray<int32_t>("SomeDevToCsVar"); // this one won't get stored
    v3->readNonBlocking();
    for(int i = 0; i < 7; ++i) {
      BOOST_CHECK(v3->accessChannel(0)[i] == 0);
    }
  }

  // check that the number of lines in the file didn't change
  BOOST_CHECK_EQUAL(countLinesInFile("changedType.persist"), countLinesInFile("myTestApplication.persist"));

  // check with changed vector size
  boost::filesystem::remove("myTestApplication.persist");
  boost::filesystem::copy("changedVectorSize.persist", "myTestApplication.persist");

  // check if stored values are properly retrieved
  {
    // create instance of test application
    MyTestApplication myTestApplication{"myTestApplication"};

    // create PV managers
    auto pvManagers = createPVManager();
    auto csManager = pvManagers.first;
    auto devManager = pvManagers.second;

    // create some variables
    devManager->createProcessArray<uint16_t>(SynchronizationDirection::controlSystemToDevice, "SomeCsToDevVar", 7);
    devManager->createProcessArray<float>(SynchronizationDirection::controlSystemToDevice, "AnotherCsToDevVar", 42);
    devManager->createProcessArray<int32_t>(SynchronizationDirection::deviceToControlSystem, "SomeDevToCsVar", 7);

    // enable persist data storage
    csManager->enablePersistentDataStorage();

    // obtain all variables from the manager to initialise them with the
    // persistent data storage
    csManager->getAllProcessVariables();

    // obtain the process variables, send some values to the variables
    auto v1 = devManager->getProcessArray<uint16_t>("SomeCsToDevVar");
    v1->readNonBlocking();
    for(int i = 0; i < 4; ++i) {
      BOOST_CHECK_EQUAL(v1->accessChannel(0)[i], i * 17);
    }
    for(int i = 4; i < 7; ++i) {
      BOOST_CHECK_EQUAL(v1->accessChannel(0)[i], 0);
    }

    auto v2 = devManager->getProcessArray<float>("AnotherCsToDevVar");
    v2->readNonBlocking();
    for(int i = 0; i < 42; ++i) {
      BOOST_CHECK_CLOSE(v2->accessChannel(0)[i], i * 3.1415 * 1e30, 2e23);
    }

    auto v3 = csManager->getProcessArray<int32_t>("SomeDevToCsVar"); // this one won't get stored
    v3->readNonBlocking();
    for(int i = 0; i < 7; ++i) {
      BOOST_CHECK(v3->accessChannel(0)[i] == 0);
    }
  }

  // check that the number of lines in the file changed in the right way
  // (SomeCsToDevVar changed from 4 to 7 elements)
  BOOST_CHECK_EQUAL(countLinesInFile("changedVectorSize.persist") + 2, countLinesInFile("myTestApplication.persist"));

  // check with renamed variable
  boost::filesystem::remove("myTestApplication.persist");
  boost::filesystem::copy("renamedVariable.persist", "myTestApplication.persist");

  // check if stored values are properly retrieved
  {
    // create instance of test application
    MyTestApplication myTestApplication{"myTestApplication"};

    // create PV managers
    auto pvManagers = createPVManager();
    auto csManager = pvManagers.first;
    auto devManager = pvManagers.second;

    // create some variables
    devManager->createProcessArray<uint16_t>(SynchronizationDirection::controlSystemToDevice, "SomeCsToDevVar", 7);
    devManager->createProcessArray<float>(SynchronizationDirection::controlSystemToDevice, "AnotherCsToDevVar", 42);
    devManager->createProcessArray<int32_t>(SynchronizationDirection::deviceToControlSystem, "SomeDevToCsVar", 7);

    // enable persist data storage
    csManager->enablePersistentDataStorage();

    // obtain all variables from the manager to initialise them with the
    // persistent data storage
    csManager->getAllProcessVariables();

    // obtain the process variables, send some values to the variables
    auto v1 = devManager->getProcessArray<uint16_t>("SomeCsToDevVar");
    v1->readNonBlocking();
    for(int i = 0; i < 7; ++i) {
      BOOST_CHECK_EQUAL(v1->accessChannel(0)[i], i * 17);
    }

    auto v2 = devManager->getProcessArray<float>("AnotherCsToDevVar");
    v2->readNonBlocking();
    for(int i = 0; i < 42; ++i) {
      BOOST_CHECK_CLOSE(v2->accessChannel(0)[i], 0, 0.00001);
    }

    auto v3 = csManager->getProcessArray<int32_t>("SomeDevToCsVar"); // this one won't get stored
    v3->readNonBlocking();
    for(int i = 0; i < 7; ++i) {
      BOOST_CHECK(v3->accessChannel(0)[i] == 0);
    }
  }

  // check that the number of lines in the file didn't change
  BOOST_CHECK_EQUAL(countLinesInFile("renamedVariable.persist"), countLinesInFile("myTestApplication.persist"));
}

// After you finished all test you have to end the test suite.
BOOST_AUTO_TEST_SUITE_END()
