// Define a name for the test module.
#define BOOST_TEST_MODULE UnidirectionalProcessArrayTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <algorithm>
#include <stdexcept>
#include <thread>

#include "UnidirectionalProcessArray.h"

#include "toType.h"

#include <boost/mpl/list.hpp>
#include <boost/thread.hpp>

typedef boost::mpl::list<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double,
    std::string>
    test_types;

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
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3, {});
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  BOOST_CHECK(sender->getName() == "/");
  //sender has default-constructed elements
  for(typename std::vector<T>::iterator i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, T());
  }
  BOOST_CHECK(sender->accessChannel(0).size() == N_ELEMENTS);
  BOOST_CHECK(receiver->getName() == "/");
  //sender has default-constructed elements
  for(typename std::vector<T>::iterator i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, T());
  }
  BOOST_CHECK(receiver->accessChannel(0).size() == N_ELEMENTS);
  BOOST_CHECK(!sender->isReadable());
  BOOST_CHECK(sender->isWriteable());
  BOOST_CHECK(receiver->isReadable());
  BOOST_CHECK(!receiver->isWriteable());
  senderReceiver = createSynchronizedProcessArray<T>(N_ELEMENTS, "test", "", "", toType<T>(SOME_NUMBER), 5, {});
  sender = senderReceiver.first;
  receiver = senderReceiver.second;
  BOOST_CHECK(sender->getName() == "/test");
  for(typename std::vector<T>::iterator i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  BOOST_CHECK(sender->accessChannel(0).size() == N_ELEMENTS);
  BOOST_CHECK(receiver->getName() == "/test");
  for(typename std::vector<T>::const_iterator i = receiver->accessChannel(0).begin();
      i != receiver->accessChannel(0).end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
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

BOOST_AUTO_TEST_CASE_TEMPLATE(testGet, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", toType<T>(SOME_NUMBER), 3, {});
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  sender->accessChannel(0);
  receiver->accessChannel(0);
  typename std::vector<T>& v = sender->accessChannel(0);
  typename std::vector<T> const& cv = sender->accessChannel(0);
  for(typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  for(typename std::vector<T>::const_iterator i = cv.begin(); i != cv.end(); ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testSet, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3, {});
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;

  // Test the assignment of a vector.
  std::vector<T> v(N_ELEMENTS, toType<T>(SOME_NUMBER + 1));
  sender->accessChannel(0) = v;
  receiver->accessChannel(0) = v;
  for(typename std::vector<T>::iterator i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 1));
  }
  for(typename std::vector<T>::iterator i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 1));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testSwap, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3, {});
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;

  // Test swapping with a vector (contains default constructed elements)
  typename std::vector<T> v(N_ELEMENTS, toType<T>(SOME_NUMBER));
  sender->accessChannel(0).swap(v);
  for(typename std::vector<T>::iterator i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  for(typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i) {
    BOOST_CHECK_EQUAL(*i, T());
  }
  sender->accessChannel(0).swap(v);
  receiver->accessChannel(0).swap(v);
  for(typename std::vector<T>::iterator i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER));
  }
  for(typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i) {
    BOOST_CHECK_EQUAL(*i, T());
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testSynchronization, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3, {});
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

  // Without wait_for_new_data, we immediately get the last vaule
  BOOST_CHECK(receiver->readNonBlocking());
  BOOST_CHECK_EQUAL(receiver->accessChannel(0).size(), N_ELEMENTS);
  for(typename std::vector<T>::iterator i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 2));
  }

  // No matter what, we should always get true where
  BOOST_CHECK(receiver->readNonBlocking());

  // Same as before but this time with more values
  for(int i = 0; i < 10; ++i) {
    sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + i));
    sender->writeDestructively();
  }
  BOOST_CHECK(receiver->readNonBlocking());
  for(typename std::vector<T>::iterator i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 9));
  }
  // No matter what, we should always get true where
  BOOST_CHECK(receiver->readNonBlocking());

  // When we send non-destructively, the value should also be preserved on the
  // sender side.
  sender->accessChannel(0).assign(N_ELEMENTS, toType<T>(SOME_NUMBER + 5));
  sender->write();
  BOOST_CHECK(receiver->readNonBlocking());
  for(typename std::vector<T>::iterator i = receiver->accessChannel(0).begin(); i != receiver->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 5));
  }
  for(typename std::vector<T>::iterator i = sender->accessChannel(0).begin(); i != sender->accessChannel(0).end();
      ++i) {
    BOOST_CHECK_EQUAL(*i, toType<T>(SOME_NUMBER + 5));
  }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testVersionNumbers, T, test_types) {
  auto senderReceiver = createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3, {});
  typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
  typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
  // After sending destructively and receiving a value, the version number on
  // the receiver should be greater.
  VersionNumber initialVersionNumber = receiver->getVersionNumber();
  sender->accessChannel(0)[0] = toType<T>(1);
  sender->writeDestructively();
  receiver->read();
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

  // No matter what, we should always get true where
  BOOST_CHECK(receiver->readNonBlocking());
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
}


BOOST_AUTO_TEST_CASE_TEMPLATE(testReadLatest, T, test_types) {
  auto senderReceiver = createSynchronizedProcessArray<T>(1, "", "", "", T(), 3, {});
  auto sender = senderReceiver.first;
  auto receiver = senderReceiver.second;

  // readLatest with only one element in the queue should read that element
  sender->accessData(0) = toType<T>(42);
  sender->write();
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(42));

  // readLatest with no element in the queue will always return true and not change
  // the value
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(42));

  // readLatest with two elements (queue is full) should return the second
  // element
  sender->accessData(0) = toType<T>(66);
  sender->write();
  sender->accessData(0) = toType<T>(77);
  sender->write();
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(77));
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(77));

  // readLatest with three elements should return the third element
  sender->accessData(0) = toType<T>(33);
  sender->write();
  sender->accessData(0) = toType<T>(44);
  sender->write();
  sender->accessData(0) = toType<T>(55);
  sender->write();
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(55));

  // readLatest with many elements should still return the last element
  for(int i = 0; i < 20; ++i) {
    sender->accessData(0) = toType<T>(i);
    sender->write();
  }
  BOOST_CHECK(receiver->readLatest());
  BOOST_CHECK_EQUAL(receiver->accessData(0), toType<T>(19));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(testValidiy, T, test_types) {
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> senderReceiver =
      createSynchronizedProcessArray<T>(N_ELEMENTS, "", "", "", T(), 3, {});
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
