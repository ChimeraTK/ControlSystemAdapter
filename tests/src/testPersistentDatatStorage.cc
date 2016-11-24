// Define a name for the test module.
#define BOOST_TEST_MODULE PersistentDataStorageTest

// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include <ChimeraTK/ControlSystemAdapter/PersistentDataStorage.h>
#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>

using namespace ChimeraTK;

// a simple test application, just to fullfill the requirement of its existence
class TestApplication : public ApplicationBase {
  public:
    using ApplicationBase::ApplicationBase;
    ~TestApplication() { shutdown(); }
    void initialise() {};
    void run() {};
};

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE( PersistentDataStorageTestSuite )

/**
 *  @todo FIXME No exception tests are yet done. They should be implemented asap!
 */


// Define one test after another. Each one needs a unique name.
  BOOST_AUTO_TEST_CASE( testStoreAndRetrieve ) {

    TestApplication app{"myTestApplication"};
    
    // remove persistency file from previous test run
    boost::filesystem::remove("myTestApplication.persist");

    // create PersistentDataStorage for the first time and fill it with two variables
    {
      PersistentDataStorage storage;

      // register integer variable MyVar1 with 10 array elements and fill it
      std::vector<int32_t> myVar1(10);
      int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
      for(int i=0; i<10; ++i) myVar1[i] = 3*i;
      storage.updateValue(id1, myVar1);

      // register floating-point variable /some/path.with.dots/to/MyVar2 with 100 array elements and fill it
      std::vector<double> myVar2(100);
      int id2 = storage.registerVariable<double>("/some/path.with.dots/to/MyVar2", 100);
      for(int i=0; i<100; ++i) myVar2[i] = -120 + 7*i;
      storage.updateValue(id2, myVar2);

      // check if both variables are properly stored
      auto myVar1stored = storage.retrieveValue<int32_t>(id1);
      for(int i=0; i<10; ++i) BOOST_CHECK( myVar1stored[i] == 3*i );
      auto myVar2stored = storage.retrieveValue<double>(id2);
      for(int i=0; i<100; ++i) BOOST_CHECK_CLOSE( myVar2stored[i], -120 + 7*i, 0.0001 );
    }
    // the first PersistentDataStorage is destroyed at this point, in its destructor the file is written
    
    // create another PersistentDataStorage and read the previously stored variables from it
    {
      PersistentDataStorage storage;
      
      // check MyVar1
      std::vector<int32_t> myVar1(10);
      int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
      myVar1 = storage.retrieveValue<int32_t>(id1);
      for(int i=0; i<10; ++i) BOOST_CHECK( myVar1[i] == 3*i );

      // check /some/path.with.dots/to/MyVar2
      std::vector<double> myVar2(100);
      int id2 = storage.registerVariable<double>("/some/path.with.dots/to/MyVar2", 100);
      myVar2 = storage.retrieveValue<double>(id2);
      for(int i=0; i<100; ++i) BOOST_CHECK_CLOSE( myVar2[i], -120 + 7*i, 0.0001 );
      
      // modify some elements of myVar1
      myVar1[7] = 42;
      myVar1[3] = 120;
      storage.updateValue(id1, myVar1);
    }
    
    // create another PersistentDataStorage and read the previously stored variables from it
    {
      PersistentDataStorage storage;
      
      // check MyVar1
      std::vector<int32_t> myVar1(10);
      int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
      myVar1 = storage.retrieveValue<int32_t>(id1);
      for(int i=0; i<10; ++i) {
        if(i == 3) {
          BOOST_CHECK( myVar1[i] == 120 );
        }
        else if(i == 7) {
          BOOST_CHECK( myVar1[i] == 42 );
        }
        else {
          BOOST_CHECK( myVar1[i] == 3*i );
        }
      }

      // check /some/path.with.dots/to/MyVar2
      std::vector<double> myVar2(100);
      int id2 = storage.registerVariable<double>("/some/path.with.dots/to/MyVar2", 100);
      myVar2 = storage.retrieveValue<double>(id2);
      for(int i=0; i<100; ++i) BOOST_CHECK_CLOSE( myVar2[i], -120 + 7*i, 0.0001 );
      
      // modify some elements of myVar1
      myVar1[7] = 42;
      myVar1[3] = 120;
      storage.updateValue(id1, myVar1);
    }
    
  }

// After you finished all test you have to end the test suite.
BOOST_AUTO_TEST_SUITE_END()
