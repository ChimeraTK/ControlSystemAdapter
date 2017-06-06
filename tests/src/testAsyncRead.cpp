#include <algorithm>
#include <thread>
#include <atomic>

#include <boost/thread.hpp>
#include <boost/test/included/unit_test.hpp>

#include <mtca4u/Device.h>
#include <mtca4u/ExperimentalFeatures.h>

#include "ProcessArray.h"


using namespace boost::unit_test_framework;
using namespace ChimeraTK;
using namespace mtca4u;

/**********************************************************************************************************************/
class AsyncReadTest {
  public:

    /// test normal asychronous read
    void testAsyncRead();

    /// test the TransferElement::readAny() function on ProcessArrays
    void testReadAny();

};

/**********************************************************************************************************************/
class  AsyncReadTestSuite : public test_suite {
  public:
    AsyncReadTestSuite() : test_suite("Async read test suite") {
      boost::shared_ptr<AsyncReadTest> asyncReadTest( new AsyncReadTest );

      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testAsyncRead, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testReadAny, asyncReadTest ) );
    }};

/**********************************************************************************************************************/
test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  ChimeraTK::ExperimentalFeatures::enable();
  
  framework::master_test_suite().p_name.value = "Async read test suite";
  framework::master_test_suite().add(new AsyncReadTestSuite);

  return NULL;
}

/**********************************************************************************************************************/
void AsyncReadTest::testAsyncRead() {
  std::cout << "testAsyncRead" << std::endl;

  auto senderReceiver = createSynchronizedProcessArray<int32_t>(1, "Test");
  auto sender = senderReceiver.first;
  auto receiver = senderReceiver.second;

  // obtain register accessor with integral type
  auto accessor = mtca4u::ScalarRegisterAccessor<int32_t>(receiver);
  auto senderAccessor = mtca4u::ScalarRegisterAccessor<int32_t>(sender);
  
  // simple reading through readAsync without actual need
  TransferFuture *future;
  senderAccessor = 5;
  senderAccessor.write();
  future = &(accessor.readAsync());
  future->wait();
  BOOST_CHECK( accessor == 5 );
  
  senderAccessor = 6;
  senderAccessor.write();
  future = &(accessor.readAsync());
  future->wait();
  BOOST_CHECK( accessor == 6 );

  // check that future's wait() function won't return before the read is complete
  for(int i=0; i<5; ++i) {
    senderAccessor = 42+i;
    future = &(accessor.readAsync());
    std::atomic<bool> flag;
    flag = false;
    std::thread thread([&future, &flag] { future->wait(); flag = true; });
    usleep(100000);
    BOOST_CHECK(flag == false);
    senderAccessor.write();
    thread.join();
    BOOST_CHECK( accessor == 42+i );
  }
  
  // check that obtaining the same future multiple times works properly
  senderAccessor = 666;
  for(int i=0; i<5; ++i) {
    future = &(accessor.readAsync());
    BOOST_CHECK( accessor == 46 );    // still the old value from the last test part
  }
  senderAccessor.write();
  future->wait();
  BOOST_CHECK( accessor == 666 );
  
  // now try another asynchronous transfer
  senderAccessor = 999;
  future = &(accessor.readAsync());
  std::atomic<bool> flag;
  flag = false;
  std::thread thread([&future, &flag] { future->wait(); flag = true; });
  usleep(100000);
  BOOST_CHECK(flag == false);
  senderAccessor.write();
  thread.join();
  BOOST_CHECK( accessor == 999 );

}

/**********************************************************************************************************************/

void AsyncReadTest::testReadAny() {
  std::cout << "testReadAny" << std::endl;

  // obtain accessors for process variables
  auto senderReceiver1 = createSynchronizedProcessArray<int32_t>(1, "Test1");
  auto a1 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver1.second);
  auto s1 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver1.first);
  auto senderReceiver2 = createSynchronizedProcessArray<int32_t>(1, "Test2");
  auto a2 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver2.second);
  auto s2 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver2.first);
  auto senderReceiver3 = createSynchronizedProcessArray<int32_t>(1, "Test3");
  auto a3 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver3.second);
  auto s3 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver3.first);
  auto senderReceiver4 = createSynchronizedProcessArray<int32_t>(1, "Test4");
  auto a4 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver4.second);
  auto s4 = mtca4u::ScalarRegisterAccessor<int32_t>(senderReceiver4.first);

  // initialise the buffers of the accessors
  a1 = 1;
  a2 = 2;
  a3 = 3;
  a4 = 4;

  // initialise the dummy registers
  s1 = 42;
  s2 = 123;
  s3 = 120;
  s4 = 345;

  // variable Test1
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);
    
    // write register and check that readAny() completes
    s1.write();
    thread.join();
    BOOST_CHECK( a1 == 42 );
  }

  // variable Test3
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);
    
    // write register and check that readAny() completes
    s3.write();
    thread.join();
    BOOST_CHECK( a3 == 120 );
  }

  // variable Test3 again
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);
    
    // write register and check that readAny() completes
    s3 = 121;
    s3.write();
    thread.join();
    BOOST_CHECK( a3 == 121 );
  }

  // variable Test2
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);
    
    // write register and check that readAny() completes
    s2.write();
    thread.join();
    BOOST_CHECK( a2 == 123 );
  }

  // variable Test4
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);
    
    // write register and check that readAny() completes
    s4.write();
    thread.join();
    BOOST_CHECK( a4 == 345 );
  }

  // variable Test4 again
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);
    
    // write register and check that readAny() completes
    s4.write();
    thread.join();
    BOOST_CHECK( a4 == 345 );
  }

  // variable Test3 a 3rd time
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&a1,&a2,&a3,&a4,&flag] { TransferElement::readAny({a1,a2,a3,a4}); flag = true; });
    
    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);
    
    // write register and check that readAny() completes
    s3 = 122;
    s3.write();
    thread.join();
    BOOST_CHECK( a3 == 122 );
  }
  
}
