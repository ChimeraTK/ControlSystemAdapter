// Define a name for the test module.
#define BOOST_TEST_MODULE UnidirectionalProcessArrayTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "toType.h"
#include "UnidirectionalProcessArray.h"

#include <boost/mpl/list.hpp>
#include <boost/thread.hpp>

#include <algorithm>
#include <stdexcept>
#include <thread>

using test_types = boost::mpl::list<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float,
    double, std::string>;

using namespace ChimeraTK;

static size_t const N_ELEMENTS = 12;

// Just some number to add so the content is not equal to the index. We
// declare and define this constant outside of the ProcessArrayTest class, so
// that it can be used with references.
static size_t const SOME_NUMBER = 42;

BOOST_AUTO_TEST_CASE_TEMPLATE(testConstructors, T, test_types) {
  std::vector<T> referenceVector;
  referenceVector.push_back(toType<T>(0));
  referenceVector.push_back(toType<T>(1));
  referenceVector.push_back(toType<T>(2));
  referenceVector.push_back(toType<T>(3));

  // Now we repeat the tests but for a sender / receiver pair. We do not test
  // the notification listener and time-stamp source because there is no
  // simple test for them and they are tested in a different method.
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS);
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  BOOST_CHECK(sender->getName() == "/");
  //sender has default-constructed elements
  for(const auto& val : accessChannel(0)) {
    BOOST_CHECK_EQUAL(val, T());
  }
  BOOST_CHECK(sender->accessChannel(0).size() == N_ELEMENTS);
  BOOST_CHECK(receiver->getName() == "/");
  //sender has default-constructed elements
  for(const auto& val : accessChannel(0)) {
    BOOST_CHECK_EQUAL(val, T());
  }
  BOOST_CHECK(receiver->accessChannel(0).size() == N_ELEMENTS);
  BOOST_CHECK(!sender->isReadable());
  BOOST_CHECK(sender->isWriteable());
  BOOST_CHECK(receiver->isReadable());
  BOOST_CHECK(!receiver->isWriteable());
  senderReceiver = createSynchronizedProcessArray<T>(N_ELEMENTS, "test", "", "", toType<T>(SOME_NUMBER), 5);
  sender = senderReceiver.first;
  receiver = senderReceiver.second;
  BOOST_CHECK(sender->getName() == "/test");
  for(const auto& val : accessChannel(0)) {
    BOOST_CHECK_EQUAL(val, toType<T>(SOME_NUMBER));
  }
  BOOST_CHECK(sender->accessChannel(0).size() == N_ELEMENTS);
  BOOST_CHECK(receiver->getName() == "/test");
  for(const auto& val : accessChannel(0)) {
    BOOST_CHECK_EQUAL(val, toType<T>(SOME_NUMBER));
  }
  BOOST_CHECK(receiver->accessChannel(0).size() == N_ELEMENTS);
  senderReceiver = createSynchronizedProcessArray<T>(referenceVector, "test", "", "", 5);
  sender = senderReceiver.first;
  receiver = senderReceiver.second;
  BOOST_CHECK(sender->getName() == "/test");
  BOOST_CHECK(sender->accessChannel(0).size() == 4);
  BOOST_CHECK(std::equal(sender->accessChannel(0).begin(), sender->accessChannel(0).end(), referenceVector.begin()));
  BOOST_CHECK(receiver->getName() == "/test");
  BOOST_CHECK(receiver->accessChannel(0).size() == 4);
  BOOST_CHECK(
      std::equal(receiver->accessChannel(0).begin(), receiver->accessChannel(0).end(), referenceVector.begin()));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testDoubleRead, T, test_types) {
  // Check whether consecutive reads on a UniDirectionalProcessArray with no wait_for_new_data works
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(1, "", "", "", toType<T>(SOME_NUMBER), 3, {});
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  sender->accessChannel(0)[0] = toType<T>(SOME_NUMBER + 1);
  sender->write();

  receiver->read();
  BOOST_CHECK_EQUAL(receiver->accessChannel(0)[0], toType<T>(SOME_NUMBER + 1));
  BOOST_CHECK(receiver->getVersionNumber() == sender->getVersionNumber());

  receiver->read();
  BOOST_CHECK_EQUAL(receiver->accessChannel(0)[0], toType<T>(SOME_NUMBER + 1));
  BOOST_CHECK(receiver->getVersionNumber() == sender->getVersionNumber());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testInterrupt, T, test_types) {
  // Expected behaviour
  auto senderReceiver = createSynchronizedProcessArray<T>(N_ELEMENTS);
  auto receiver = senderReceiver.second;

  auto t = std::thread([&receiver]() { BOOST_CHECK_THROW(receiver->read(), boost::thread_interrupted); });
  receiver->interrupt();
  t.join();

  // Calling interrupt without wait_for_new_data throws logic_error
  senderReceiver = createSynchronizedProcessArray<T>(1, "", "", "", toType<T>(SOME_NUMBER), 3, {});
  receiver = senderReceiver.second;

  BOOST_CHECK_THROW(receiver->interrupt(), ChimeraTK::logic_error);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testGet, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", toType<T>(SOME_NUMBER));
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  sender->accessChannel(0);
  receiver->accessChannel(0);
  typename std::vector<T>& v = sender->accessChannel(0);
  typename std::vector<T> const& cv = sender->accessChannel(0);
  for(const auto& val : v) {
    BOOST_CHECK_EQUAL(val, toType<T>(SOME_NUMBER));
  }
  for(const auto& val cv) {
    BOOST_CHECK_EQUAL(val, toType<T>(SOME_NUMBER));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testSet, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS);
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;

  // Test the assignment of a vector.
  std::vector<T> v(N_ELEMENTS, toType<T>(SOME_NUMBER + 1));
  sender->accessChannel(0) = v;
  receiver->accessChannel(0) = v;
  for(auto i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 1));
  }
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 1));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testSwap, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS);
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;

  // Test swapping with a vector (contains default constructed elements)
  typename std::vector<T> v(N_ELEMENTS, toType<T>(SOME_NUMBER));
  sender->accessChannel(0).swap(v);
  for(auto i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  for(auto i = v.begin(); i != v.end(); ++i) {
    BOOST_CHECK_EQUAL(*i, T());
  }
  sender->accessChannel(0).swap(v);
  receiver->accessChannel(0).swap(v);
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  for(auto i = v.begin(); i != v.end(); ++i) {
    BOOST_CHECK_EQUAL(*i, T());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testSynchronization, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3);
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  // If we send three values consecutively, they all should be received because
  // the queue length is two and there is the additional atomic triple buffer
  // holding a third value
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER));
  sender->writeDestructively();
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 1));
  sender->writeDestructively();
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 2));
  sender->writeDestructively();
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK_EQUAL(receiver->accessChannel(0).size(), N_ELEMENTS);
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK(receiver->accessChannel(0).size() == N_ELEMENTS);
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 1));
  }
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK(receiver->accessChannel(0).size() == N_ELEMENTS);
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 2));
  }
  // We have received all values, so no more values should be available.
  BOOST_CHECK(!receiver->readNonBlocking());

  // Now we try to send four values consecutively. This should result in the
  // last but one value being dropped. The latest value should be preserved,
  // since it is in the atomic triple buffer
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 3));
  sender->writeDestructively();
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 4));
  sender->writeDestructively();
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 5));
  sender->writeDestructively();
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 6));
  sender->writeDestructively();
  BOOST_CHECK(receiver->readNonBlocking());
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 3));
  }
  BOOST_CHECK(receiver->readNonBlocking());
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 4));
  }
  BOOST_CHECK(receiver->readNonBlocking());
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 6));
  }
  // We have received all values, so no more values should be available.
  BOOST_CHECK(!receiver->readNonBlocking());

  // Same as before but this time with more values
  for(int i = 0; i < 10; ++i) {
    sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + i));
    sender->writeDestructively();
  }
  BOOST_CHECK(receiver->readNonBlocking());
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  BOOST_CHECK(receiver->readNonBlocking());
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 1));
  }
  BOOST_CHECK(receiver->readNonBlocking());
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 9));
  }
  // We have received all values, so no more values should be available.
  BOOST_CHECK(!receiver->readNonBlocking());

  // When we send non-destructively, the value should also be preserved on the
  // sender side.
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 5));
  sender->write();
  BOOST_CHECK(receiver->readNonBlocking());
  for(auto i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 5));
  }
  for(auto i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 5));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testVersionNumbers, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3);
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  // After sending destructively and receiving a value, the version number on
  // the receiver should be greater.
  VersionNumber initialVersionNumber = receiver->getVersionNumber();
  sender->accessChannel(0)[0] = toType<T>(1);
  sender->writeDestructively();
  BOOST_CHECK(receiver->readNonBlocking());
  VersionNumber versionNumber = receiver->getVersionNumber();
  BOOST_CHECK(versionNumber > initialVersionNumber);
  BOOST_CHECK_EQUAL(receiver->accessChannel(0)[0], toType<T>(1));
  // When we send again specifying the same version number, there should still
  // be the update of the receiver.
  sender->accessChannel(0)[0] = toType<T>(2);
  sender->writeDestructively(versionNumber);
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK(versionNumber == receiver->getVersionNumber());
  BOOST_CHECK_EQUAL(receiver->accessChannel(0)[0], toType<T>(2));
  // When we explicitly use a greater version number, the receiver should be
  // updated again.
  sender->accessChannel(0)[0] = toType<T>(3);
  sender->writeDestructively();
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK(receiver->getVersionNumber() > versionNumber);
  BOOST_CHECK_EQUAL(receiver->accessChannel(0)[0], toType<T>(3));
  // Even when sending again an update with an older version number, we should
  // receive an exception
  sender->accessChannel(0)[0] = toType<T>(4);
  try {
    sender->writeDestructively(versionNumber);
    BOOST_ERROR("Exception expected.");
  }
  catch(ChimeraTK::logic_error&) {
  }
  BOOST_CHECK(receiver->readNonBlocking() == false);
  BOOST_CHECK(receiver->getVersionNumber() > versionNumber);
  BOOST_CHECK_EQUAL(receiver->accessChannel(0)[0], toType<T>(3));
  // When we send non-destructively, the version number on the sender and the
  // receiver should match after sending and receiving.
  versionNumber = receiver->getVersionNumber();
  sender->accessChannel(0)[0] = toType<T>(5);
  sender->write();
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK(receiver->getVersionNumber() > versionNumber);
  BOOST_CHECK(receiver->getVersionNumber() == sender->getVersionNumber());
  BOOST_CHECK_EQUAL(receiver->accessChannel(0)[0], toType<T>(5));

  // provoke buffer overrun, read until the queue is empty (but triple buffer
  // still has value) and put a new element into the queue
  for(int i = 0; i < 10; ++i) {
    sender->accessData(0) = toType<T>(33 + i);
    sender->write();
  }
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(33));
  BOOST_CHECK(receiver->readNonBlocking()); // after this line the buffer should be empty
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(34));
  sender->accessData(0) = toType<T>(12);
  sender->write();
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(42));
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(12));
  BOOST_CHECK(!(receiver->readNonBlocking()));

  // Test if VersionNumber in only updated, if there is new data
  versionNumber = receiver->getVersionNumber();
  sender->accessChannel(0)[0] = toType<T>(5);
  sender->write();
  BOOST_CHECK(receiver->readNonBlocking());
  VersionNumber lastVersionNumber = versionNumber;
  versionNumber = receiver->getVersionNumber();
  BOOST_CHECK(versionNumber > lastVersionNumber);

  lastVersionNumber = versionNumber;
  BOOST_CHECK(!(receiver->readNonBlocking()));
  BOOST_CHECK(lastVersionNumber == receiver->getVersionNumber());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testBlockingRead, T, test_types) {
  auto senderReceiver = createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3);
  auto sender = senderReceiver.first;
  auto receiver = senderReceiver.second;

  // blocking read should just return the value if it is already there
  sender->accessData(0) = toType<T>(42);
  sender->write();
  receiver->read();
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(42));

  // start blocking read first in the background and send then the data
  {
    std::thread backgroundTask([&receiver] { receiver->read(); });
    usleep(200000);
    sender->accessData(0) = toType<T>(43);
    sender->write();
    backgroundTask.join();
    BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(43));
  }

  // same with multiple transfers (not more than the queue length to avoid race
  // conditions)
  {
    std::thread backgroundTask([&receiver] {
      for(int i = 0; i < 2; ++i) {
        receiver->read();
        BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(2 + i));
      }
    });
    for(int i = 0; i < 2; ++i) {
      usleep(10000);
      sender->accessData(0) = toType<T>(2 + i);
      sender->write();
    }
    backgroundTask.join();
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testReadLatest, T, test_types) {
  auto senderReceiver = createSynchronizedProcessArray<T>(1);
  auto sender = senderReceiver.first;
  auto receiver = senderReceiver.second;

  // readLatest with only one element in the queue should read that element
  sender->accessData(0) = toType<T>(42);
  sender->write();
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(42));

  // readLatest with no element in the queue will return false and not change
  // the value
  BOOST_CHECK(!receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(42));

  // readLatest with two elements (queue is full) should return the second
  // element
  sender->accessData(0) = toType<T>(66);
  sender->write();
  sender->accessData(0) = toType<T>(77);
  sender->write();
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(77));
  BOOST_CHECK(!receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(77));

  // readLatest with three elements (queue is full and atomic buffer is filled)
  // should return the third element
  sender->accessData(0) = toType<T>(33);
  sender->write();
  sender->accessData(0) = toType<T>(44);
  sender->write();
  sender->accessData(0) = toType<T>(55);
  sender->write();
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(55));

  // readLatest with 20 elements (certain queue overflow) should still return
  // the last element
  for(int i = 0; i < 20; ++i) {
    sender->accessData(0) = toType<T>(i);
    sender->write();
  }
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(19));
  BOOST_CHECK(!(receiver->readNonBlocking()));

  // redo last step to make sure no buffers are lost when having a queue
  // overflow
  for(int i = 0; i < 20; ++i) {
    sender->accessData(0) = toType<T>(100 + i);
    sender->write();
  }
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(119));
  BOOST_CHECK(!(receiver->readNonBlocking()));

  // provoke buffer overrun, read until the queue is empty (but triple buffer
  // still has value) and put a new element into the queue - which should then
  // be read by readLatest
  for(int i = 0; i < 10; ++i) {
    sender->accessData(0) = toType<T>(33 + i);
    sender->write();
  }
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(33));
  BOOST_CHECK(receiver->readNonBlocking()); // after this line the queue should be empty
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(34));
  sender->accessData(0) = toType<T>(12);
  sender->write();
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(12));
  BOOST_CHECK(!(receiver->readLatest()));

  // provoke that the future obtained in the very beginning of
  // doReadTransferLatest() is pointing to the buffer on the triple buffer
  for(int i = 0; i < 10; ++i) {
    sender->accessData(0) = toType<T>(11 + i);
    sender->write();
  }
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(11));
  BOOST_CHECK(receiver->readNonBlocking()); // after this line the queue should be empty
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(12));
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(20));
  BOOST_CHECK(!(receiver->readLatest()));
  for(int i = 0; i < 100; ++i) { // check that all buffers are still properly working
    for(int k = 0; k < 3; ++k) {
      sender->accessData(0) = toType<T>(10 * i + k);
      sender->write();
    }
    for(int k = 0; k < 3; ++k) {
      BOOST_CHECK(receiver->readNonBlocking());
      BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(10 * i + k));
    }
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testValidiy, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3);
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;

  /* Check that the initial state is "ok" on sender and "faulty" on receiver side */
  BOOST_CHECK(sender->dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(receiver->dataValidity() == ChimeraTK::DataValidity::faulty);

  /* A single write will transport the ok flag to the receiver side */
  sender->write();
  receiver->read();
  BOOST_CHECK(sender->dataValidity() == ChimeraTK::DataValidity::ok);
  BOOST_CHECK(receiver->dataValidity() == ChimeraTK::DataValidity::ok);

  /* Check that the fault state is transported correctly from sender to receiver */
  sender->setDataValidity(ChimeraTK::DataValidity::faulty);
  sender->write();
  receiver->read();
  BOOST_CHECK(receiver->dataValidity() == ChimeraTK::DataValidity::faulty);

  /* Check that intermediate fault states are dropped if readLatest() is used */
  sender->write();
  for(int k = 0; k < 10; ++k) {
    sender->accessData(0) = toType<T>(k);
    sender->write();
  }
  sender->setDataValidity(ChimeraTK::DataValidity::ok);
  sender->write();
  receiver->readLatest();
  BOOST_CHECK(receiver->dataValidity() == ChimeraTK::DataValidity::ok);

  // Checking that you cannot set the DataValidity on a read-only ProcessArray
  // has been removed. This test is done through an assertion in the TransferElement base class
  // and does not belong into the ProcessArray test any more.
}
