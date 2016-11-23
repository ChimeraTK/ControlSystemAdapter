// Define a name for the test module.
#define BOOST_TEST_MODULE PersistentDataStorageTest

// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <boost/thread.hpp>

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

// Define one test after another. Each one needs a unique name.
  BOOST_AUTO_TEST_CASE( testStoreAndRetrieve ) {

    TestApplication app{"myTestApplication"};
    
    {
      PersistentDataStorage storage;
      
      std::vector<int32_t> myVar1(10);
      int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
      for(int i=0; i<10; ++i) myVar1[i] = 3*i;
      storage.updateValue(id1, myVar1);

      auto myVar1stored = storage.retrieveValue<int32_t>(id1);
      for(int i=0; i<10; ++i) BOOST_CHECK( myVar1stored[i] == 3*i );
    }
    
    {
      PersistentDataStorage storage;
      
      std::vector<int32_t> myVar1(10);
      int id1 = storage.registerVariable<int32_t>("MyVar1", 10);
      myVar1 = storage.retrieveValue<int32_t>(id1);
      for(int i=0; i<10; ++i) BOOST_CHECK( myVar1[i] == 3*i );
    }
    
    
    
  }

// After you finished all test you have to end the test suite.
BOOST_AUTO_TEST_SUITE_END()
