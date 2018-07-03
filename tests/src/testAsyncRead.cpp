#include <algorithm>
#include <thread>
#include <atomic>

#include <boost/thread.hpp>
#include <boost/test/included/unit_test.hpp>

#include <ChimeraTK/Device.h>
#include <ChimeraTK/ExperimentalFeatures.h>
#include <ChimeraTK/ReadAnyGroup.h>

#include "ProcessArray.h"


using namespace boost::unit_test_framework;
using namespace ChimeraTK;
using namespace ChimeraTK;

/**********************************************************************************************************************/
class AsyncReadTest {
  public:

    /// test normal asychronous read
    void testAsyncRead();

    /// test the readAny() function on ProcessArrays
    void testReadAny();

    /// test mixing asynchronous with synchronous reads
    void testMixedRead();

};

/**********************************************************************************************************************/

class  AsyncReadTestSuite : public test_suite {
  public:
    AsyncReadTestSuite() : test_suite("Async read test suite") {
      boost::shared_ptr<AsyncReadTest> asyncReadTest( new AsyncReadTest );

      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testAsyncRead, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testReadAny, asyncReadTest ) );
      add( BOOST_CLASS_TEST_CASE( &AsyncReadTest::testMixedRead, asyncReadTest ) );
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
  auto accessor = ChimeraTK::ScalarRegisterAccessor<int32_t>(receiver);
  auto senderAccessor = ChimeraTK::ScalarRegisterAccessor<int32_t>(sender);

  // simple reading through readAsync without actual need
  TransferFuture future;
  senderAccessor = 5;
  senderAccessor.write();
  future = accessor.readAsync();
  future.wait();
  BOOST_CHECK( accessor == 5 );

  senderAccessor = 6;
  senderAccessor.write();
  future = accessor.readAsync();
  future.wait();
  BOOST_CHECK( accessor == 6 );

  // check that future's wait() function won't return before the read is complete
  for(int i=0; i<5; ++i) {
    senderAccessor = 42+i;
    future = accessor.readAsync();
    std::atomic<bool> flag;
    flag = false;
    std::thread thread([&future, &flag] { future.wait(); flag = true; });
    usleep(100000);
    BOOST_CHECK(flag == false);
    senderAccessor.write();
    thread.join();
    BOOST_CHECK( accessor == 42+i );
  }

  // check that obtaining the same future multiple times works properly
  senderAccessor = 666;
  for(int i=0; i<5; ++i) {
    future = accessor.readAsync();
    BOOST_CHECK( accessor == 46 );    // still the old value from the last test part
  }
  senderAccessor.write();
  future.wait();
  BOOST_CHECK( accessor == 666 );

  // now try another asynchronous transfer
  senderAccessor = 999;
  future = accessor.readAsync();
  std::atomic<bool> flag;
  flag = false;
  std::thread thread([&future, &flag] { future.wait(); flag = true; });
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
  auto a1 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver1.second);
  auto s1 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver1.first);
  auto senderReceiver2 = createSynchronizedProcessArray<int32_t>(1, "Test2");
  auto a2 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver2.second);
  auto s2 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver2.first);
  auto senderReceiver3 = createSynchronizedProcessArray<int32_t>(1, "Test3");
  auto a3 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver3.second);
  auto s3 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver3.first);
  auto senderReceiver4 = createSynchronizedProcessArray<int32_t>(1, "Test4");
  auto a4 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver4.second);
  auto s4 = ChimeraTK::ScalarRegisterAccessor<int32_t>(senderReceiver4.first);

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

  // create ReadAnyGroup
  ChimeraTK::ReadAnyGroup group({a1,a2,a3,a4});

  // variable Test1
  {
    // launch the readAny in a background thread
    std::atomic<bool> flag{false};
    std::thread thread([&group,&flag] { group.waitAny(); flag = true; });

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
    std::thread thread([&group,&flag] { group.waitAny(); flag = true; });

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
    std::thread thread([&group,&flag] { group.waitAny(); flag = true; });

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
    std::thread thread([&group,&flag] { group.waitAny(); flag = true; });

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
    std::thread thread([&group,&flag] { group.waitAny(); flag = true; });

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
    std::thread thread([&group,&flag] { group.waitAny(); flag = true; });

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
    std::thread thread([&group,&flag] { group.waitAny(); flag = true; });

    // check that it doesn't return too soon
    usleep(100000);
    BOOST_CHECK(flag == false);

    // write register and check that readAny() completes
    s3 = 122;
    s3.write();
    thread.join();
    BOOST_CHECK( a3 == 122 );
  }

  // Test1 and then Test2 (order should be guaranteed) - this time write first to check if order is properly kept
  {
    s1 = 666;
    s1.write();
    s2 = 777;
    s2.write();
    // no point to use a thread here
    auto r = group.waitAny();
    BOOST_CHECK(a1.getId() == r);
    BOOST_CHECK(a1 == 666);
    BOOST_CHECK(a2 == 123);

    r = group.waitAny();
    BOOST_CHECK(a2.getId() == r);
    BOOST_CHECK(a1 == 666);
    BOOST_CHECK(a2 == 777);

    BOOST_CHECK(a1.readNonBlocking() == false);
    BOOST_CHECK(a2.readNonBlocking() == false);
    BOOST_CHECK(a3.readNonBlocking() == false);
    BOOST_CHECK(a4.readNonBlocking() == false);
  }

  // In order: Test4, Test2, Test3 and Test1
  {
    s4 = 111;
    s4.write();
    s2 = 222;
    s2.write();
    s3 = 333;
    s3.write();
    s1 = 444;
    s1.write();

    // no point to use a thread here
    auto r = group.waitAny();
    BOOST_CHECK(a4.getId() == r);
    BOOST_CHECK(a1 == 666);
    BOOST_CHECK(a2 == 777);
    BOOST_CHECK(a3 == 122);
    BOOST_CHECK(a4 == 111);

    r = group.waitAny();
    BOOST_CHECK(a2.getId() == r);
    BOOST_CHECK(a1 == 666);
    BOOST_CHECK(a2 == 222);
    BOOST_CHECK(a3 == 122);
    BOOST_CHECK(a4 == 111);

    r = group.waitAny();
    BOOST_CHECK(a3.getId() == r);
    BOOST_CHECK(a1 == 666);
    BOOST_CHECK(a2 == 222);
    BOOST_CHECK(a3 == 333);
    BOOST_CHECK(a4 == 111);

    r = group.waitAny();
    BOOST_CHECK(a1.getId() == r);
    BOOST_CHECK(a1 == 444);
    BOOST_CHECK(a2 == 222);
    BOOST_CHECK(a3 == 333);
    BOOST_CHECK(a4 == 111);

    BOOST_CHECK(a1.readNonBlocking() == false);
    BOOST_CHECK(a2.readNonBlocking() == false);
    BOOST_CHECK(a3.readNonBlocking() == false);
    BOOST_CHECK(a4.readNonBlocking() == false);

  }

}

/**********************************************************************************************************************/

void AsyncReadTest::testMixedRead() {
  std::cout << "testMixedRead" << std::endl;

  auto senderReceiver = createSynchronizedProcessArray<int32_t>(1, "someName");
  auto sender = senderReceiver.first;
  auto receiver = senderReceiver.second;

  // obtain register accessor with integral type
  auto accessor = ChimeraTK::ScalarRegisterAccessor<int32_t>(receiver);
  auto senderAccessor = ChimeraTK::ScalarRegisterAccessor<int32_t>(sender);

  TransferFuture future;

  // plain readAsync and read mixed one after each other
  senderAccessor = 5;
  senderAccessor.write();
  future = accessor.readAsync();
  future.wait();
  BOOST_CHECK( accessor == 5 );

  senderAccessor = 6;
  senderAccessor.write();
  accessor.read();
  BOOST_CHECK( accessor == 6 );

  senderAccessor = 7;
  senderAccessor.write();
  future = accessor.readAsync();
  future.wait();
  BOOST_CHECK( accessor == 7 );

  senderAccessor = 8;
  senderAccessor.write();
  accessor.read();
  BOOST_CHECK( accessor == 8 );

  future = accessor.readAsync();
  senderAccessor = 9;
  senderAccessor.write();
  future.wait();
  BOOST_CHECK( accessor == 9 );

  senderAccessor = 10;
  senderAccessor.write();
  accessor.read();
  BOOST_CHECK( accessor == 10 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

  // mixing with read() when future stays unfulfilled
  future = accessor.readAsync();
  usleep(10000);
  BOOST_CHECK(future.hasNewData() == false);

  senderAccessor = 11;
  senderAccessor.write();
  usleep(10000);

  accessor.read();
  BOOST_CHECK( accessor == 11 );
  senderAccessor = 12;
  senderAccessor.write();
  accessor.read();
  BOOST_CHECK( accessor == 12 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

  // mixing with read() when future stays unfulfilled, different order
  future = accessor.readAsync();
  usleep(10000);
  BOOST_CHECK(future.hasNewData() == false);

  senderAccessor = 13;
  senderAccessor.write();
  senderAccessor = 14;
  senderAccessor.write();
  usleep(10000);

  accessor.read();
  BOOST_CHECK( accessor == 13 );

  accessor.read();
  BOOST_CHECK( accessor == 14 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

  // readAsync with data already present, then discard the future
  senderAccessor = 15;
  senderAccessor.write();
  future = accessor.readAsync();
  usleep(10000);
  accessor.read();
  BOOST_CHECK( accessor == 15 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

  // readAsync with data already present, then discard the future, with more data
  senderAccessor = 16;
  senderAccessor.write();
  senderAccessor = 17;
  senderAccessor.write();
  future = accessor.readAsync();
  usleep(10000);
  accessor.read();
  BOOST_CHECK( accessor == 16 );
  accessor.read();
  BOOST_CHECK( accessor == 17 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

  // mixing with readNonBlocking() when future stays unfulfilled
  future = accessor.readAsync();
  usleep(10000);
  BOOST_CHECK(future.hasNewData() == false);

  senderAccessor = 18;
  senderAccessor.write();
  senderAccessor = 19;
  senderAccessor.write();
  usleep(10000);

  BOOST_CHECK( accessor.readNonBlocking() == true );
  BOOST_CHECK( accessor == 18 );

  BOOST_CHECK( accessor.readNonBlocking() == true );
  BOOST_CHECK( accessor == 19 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

  // mixing with readNonBlocking() when future stays unfulfilled and a readNonBlocking() also is unsuccessfull
  future = accessor.readAsync();
  usleep(10000);
  BOOST_CHECK(future.hasNewData() == false);

  BOOST_CHECK( accessor.readNonBlocking() == false );

  senderAccessor = 20;
  senderAccessor.write();
  senderAccessor = 21;
  senderAccessor.write();
  usleep(10000);

  BOOST_CHECK( accessor.readNonBlocking() == true );
  BOOST_CHECK( accessor == 20 );

  BOOST_CHECK( accessor.readNonBlocking() == true );
  BOOST_CHECK( accessor == 21 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

  // readAsync with data already present, then discard the future - readNonBlocking() version
  senderAccessor = 22;
  senderAccessor.write();
  future = accessor.readAsync();
  usleep(10000);
  BOOST_CHECK( accessor.readNonBlocking() == true );
  BOOST_CHECK( accessor == 22 );

  BOOST_CHECK( accessor.readNonBlocking() == false );

}
