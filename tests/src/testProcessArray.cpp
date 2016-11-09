#include <algorithm>
#include <stdexcept>

#include <boost/test/included/unit_test.hpp>

#include "ProcessArray.h"

#include "CountingProcessVariableListener.h"
#include "CountingTimeStampSource.h"

using namespace boost::unit_test_framework;

namespace ChimeraTK {

  /**
   * The test class for the ProcessArray.
   * It is templated to be tested with all data types.
   */
  template<class T>
  class ProcessArrayTest {
  public:
    static void testConstructors();
    static void testAssignment();
    static void testGet();
    static void testSet();
    static void testSwap();
    static void testSendNotification();
    static void testTimeStampSource();
    static void testSynchronization();
    static void testVersionNumbers();

  private:
    static size_t const N_ELEMENTS = 12;

  };

  // Just some number to add so the content is not equal to the index. We
  // declare and define this constant outside of the ProcessArrayTest class, so
  // that it can be used with references.
  static size_t const SOME_NUMBER = 42;

  /**
   * The boost test suite which executes the ProcessArrayTest.
   */
  template<class T>
  class ProcessArrayTestSuite: public test_suite {
  public:
    ProcessArrayTestSuite() :
        test_suite("ProcessArray test suite") {
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testConstructors));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testAssignment));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testGet));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testSet));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testSwap));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testSendNotification));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testTimeStampSource));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testSynchronization));
      add(BOOST_TEST_CASE(&ProcessArrayTest<T>::testVersionNumbers));
    }
  };

  template<class T>
  void ProcessArrayTest<T>::testConstructors() {
    typename ProcessArray<T>::SharedPtr simpleArray =
        createSimpleProcessArray<T>(N_ELEMENTS);
    // The name should be empty and all elements should have been initialized
    // with zeroes.
    BOOST_CHECK(simpleArray->getName() == "");
    for (typename std::vector<T>::iterator i = simpleArray->get().begin();
        i != simpleArray->get().end(); ++i) {
      BOOST_CHECK(*i == 0);
    }
    BOOST_CHECK(simpleArray->get().size() == N_ELEMENTS);
    // Now we test the constructor with non-default parameters.
    simpleArray = createSimpleProcessArray<T>(N_ELEMENTS, "test", SOME_NUMBER);
    BOOST_CHECK(simpleArray->getName() == "test");
    for (typename std::vector<T>::iterator i = simpleArray->get().begin();
        i != simpleArray->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    BOOST_CHECK(simpleArray->get().size() == N_ELEMENTS);
    BOOST_CHECK(!simpleArray->isReadable());
    BOOST_CHECK(!simpleArray->isWriteable());
    // Now we test the constructor that takes a whole reference vector.
    std::vector<T> referenceVector;
    referenceVector.push_back(0);
    referenceVector.push_back(1);
    referenceVector.push_back(2);
    referenceVector.push_back(3);
    simpleArray = createSimpleProcessArray<T>(referenceVector, "test");
    BOOST_CHECK(simpleArray->getName() == "test");
    BOOST_CHECK(simpleArray->get().size() == 4);
    BOOST_CHECK(
        std::equal(simpleArray->get().begin(), simpleArray->get().end(),
            referenceVector.begin()));
    BOOST_CHECK(!simpleArray->isReadable());
    BOOST_CHECK(!simpleArray->isWriteable());

    // Now we repeat the tests but for a sender / receiver pair. We do not test
    // the notification listener and time-stamp source because there is no
    // simple test for them and they are tested in a different method.
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    BOOST_CHECK(sender->getName() == "");
    for (typename std::vector<T>::iterator i = sender->get().begin();
        i != sender->get().end(); ++i) {
      BOOST_CHECK(*i == 0);
    }
    BOOST_CHECK(sender->get().size() == N_ELEMENTS);
    BOOST_CHECK(receiver->getName() == "");
    for (typename std::vector<T>::iterator i = receiver->get().begin();
        i != receiver->get().end(); ++i) {
      BOOST_CHECK(*i == 0);
    }
    BOOST_CHECK(receiver->get().size() == N_ELEMENTS);
    BOOST_CHECK(!sender->isReadable());
    BOOST_CHECK(sender->isWriteable());
    BOOST_CHECK(receiver->isReadable());
    BOOST_CHECK(!receiver->isWriteable());
    senderReceiver = createSynchronizedProcessArray<T>(N_ELEMENTS, "test",
        SOME_NUMBER, 5);
    sender = senderReceiver.first;
    receiver = senderReceiver.second;
    BOOST_CHECK(sender->getName() == "test");
    for (typename std::vector<T>::iterator i = sender->get().begin();
        i != sender->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    BOOST_CHECK(sender->get().size() == N_ELEMENTS);
    BOOST_CHECK(receiver->getName() == "test");
    for (typename std::vector<T>::const_iterator i = receiver->get().begin();
        i != receiver->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    BOOST_CHECK(receiver->get().size() == N_ELEMENTS);
    senderReceiver = createSynchronizedProcessArray<T>(referenceVector, "test",
        5, false);
    sender = senderReceiver.first;
    receiver = senderReceiver.second;
    BOOST_CHECK(sender->getName() == "test");
    BOOST_CHECK(sender->get().size() == 4);
    BOOST_CHECK(
        std::equal(sender->get().begin(), sender->get().end(),
            referenceVector.begin()));
    BOOST_CHECK(receiver->getName() == "test");
    BOOST_CHECK(receiver->get().size() == 4);
    BOOST_CHECK(
        std::equal(receiver->get().begin(), receiver->get().end(),
            referenceVector.begin()));
  }

  template<class T>
  void ProcessArrayTest<T>::testAssignment() {
    typename ProcessArray<T>::SharedPtr array1 = createSimpleProcessArray<T>(
        N_ELEMENTS);
    typename ProcessArray<T>::SharedPtr array2 = createSimpleProcessArray<T>(
        N_ELEMENTS, "", SOME_NUMBER);
    // Test the assignment of another process array.
    (*array1) = (*array2);
    for (typename std::vector<T>::iterator i = array1->get().begin();
        i != array1->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    // Test the assignment of a vector.
    std::vector<T> v(N_ELEMENTS, SOME_NUMBER + 1);
    (*array1) = v;
    for (typename std::vector<T>::iterator i = array1->get().begin();
        i != array1->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER + 1);
    }
    // For the synchronized variant we only test that the assignment operator
    // does not throw if it should not. As the implementation is the same as
    // for the simple array, there is no need to check the actual values.
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    (*sender) = (*array1);
    (*receiver) = (*array1);
    (*sender) = v;
    (*receiver) = v;
  }

  template<class T>
  void ProcessArrayTest<T>::testGet() {
    typename ProcessArray<T>::SharedPtr simpleArray =
        createSimpleProcessArray<T>(N_ELEMENTS, "", SOME_NUMBER);
    typename std::vector<T> & v = simpleArray->get();
    typename std::vector<T> const & cv = simpleArray->get();
    for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    for (typename std::vector<T>::const_iterator i = cv.begin(); i != cv.end();
        ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    // If we have a pointer to a const array, we should get a reference to a
    // const vector.
    typename boost::shared_ptr<ProcessArray<T> const> constSimpleArray =
        simpleArray;
    typename std::vector<T> const & cv2 = constSimpleArray->get();
    for (typename std::vector<T>::const_iterator i = cv2.begin();
        i != cv2.end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    // Next we run the tests for a synchronized array that supports swapping. We
    // only test that the getter methods do not throw an exception, as we can
    // assume that the returned reference is correct if it was correct for the
    // simple array.
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    sender->get();
    receiver->get();
  }

  template<class T>
  void ProcessArrayTest<T>::testSet() {
    typename ProcessArray<T>::SharedPtr array1 = createSimpleProcessArray<T>(
        N_ELEMENTS);
    typename ProcessArray<T>::SharedPtr array2 = createSimpleProcessArray<T>(
        N_ELEMENTS, "", SOME_NUMBER);
    // Test the assignment of another process array.
    array1->set(*array2);
    for (typename std::vector<T>::iterator i = array1->get().begin();
        i != array1->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    // Test the assignment of a vector.
    std::vector<T> v(N_ELEMENTS, SOME_NUMBER + 1);
    array1->set(v);
    for (typename std::vector<T>::iterator i = array1->get().begin();
        i != array1->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER + 1);
    }
    // For the synchronized variant we only test that the set method does not
    // throw if it should not. As the implementation is the same as for the
    // simple array, there is no need to check the actual values.
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    sender->set(*array1);
    receiver->set(*array1);
    sender->set(v);
    receiver->set(v);
  }

  template<class T>
  void ProcessArrayTest<T>::testSwap() {
    typename ProcessArray<T>::SharedPtr simpleArray =
        createSimpleProcessArray<T>(N_ELEMENTS);
    typename std::vector<T> v(N_ELEMENTS, SOME_NUMBER);
    simpleArray->swap(v);
    for (typename std::vector<T>::iterator i = simpleArray->get().begin();
        i != simpleArray->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i) {
      BOOST_CHECK(*i == 0);
    }
    // For the synchronized variant we only test that the swap method does not
    // throw if it should not. As the implementation is the same as for the
    // simple array, there is no need to check the actual values.
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    sender->swap(v);
    receiver->swap(v);
  }

  template<class T>
  void ProcessArrayTest<T>::testSendNotification() {
    boost::shared_ptr<CountingProcessVariableListener> sendNotificationListener(
        boost::make_shared<CountingProcessVariableListener>());
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS, "", 0, 2, false,
            TimeStampSource::SharedPtr(), VersionNumberSource::SharedPtr(),
            sendNotificationListener);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    BOOST_CHECK(sendNotificationListener->count == 0);
    sender->write();
    BOOST_CHECK(sendNotificationListener->count == 1);
    BOOST_CHECK(sendNotificationListener->lastProcessVariable == receiver);
    sender->write();
    BOOST_CHECK(sendNotificationListener->count == 2);
    BOOST_CHECK(sendNotificationListener->lastProcessVariable == receiver);
  }

  template<class T>
  void ProcessArrayTest<T>::testTimeStampSource() {
    TimeStampSource::SharedPtr timeStampSource(
        boost::make_shared<CountingTimeStampSource>());
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS, "", 0, 2, false,
            timeStampSource);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    sender->write();
    receiver->readNonBlocking();
    BOOST_CHECK(receiver->getTimeStamp().index0 == 0);
    sender->write();
    receiver->readNonBlocking();
    BOOST_CHECK(receiver->getTimeStamp().index0 == 1);
    sender->write();
    receiver->readNonBlocking();
    BOOST_CHECK(receiver->getTimeStamp().index0 == 2);
  }

  template<class T>
  void ProcessArrayTest<T>::testSynchronization() {
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS, "", 0, 2, true);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    // If we send two values consecutively, they both should be received because
    // the number of buffers is two.
    sender->get().assign(N_ELEMENTS, SOME_NUMBER);
    sender->writeDestructively();
    sender->get().assign(N_ELEMENTS, SOME_NUMBER + 1);
    sender->writeDestructively();
    BOOST_CHECK(receiver->readNonBlocking());
    for (typename std::vector<T>::iterator i = receiver->get().begin();
        i != receiver->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER);
    }
    BOOST_CHECK(receiver->readNonBlocking());
    for (typename std::vector<T>::iterator i = receiver->get().begin();
        i != receiver->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER + 1);
    }
    // We have received all values, so no more values should be available.
    BOOST_CHECK(!receiver->readNonBlocking());
    // Now we try to send three values consecutively. This should result in the
    // first value being dropped.
    sender->get().assign(N_ELEMENTS, SOME_NUMBER + 2);
    sender->writeDestructively();
    sender->get().assign(N_ELEMENTS, SOME_NUMBER + 3);
    sender->writeDestructively();
    sender->get().assign(N_ELEMENTS, SOME_NUMBER + 4);
    sender->writeDestructively();
    BOOST_CHECK(receiver->readNonBlocking());
    for (typename std::vector<T>::iterator i = receiver->get().begin();
        i != receiver->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER + 3);
    }
    BOOST_CHECK(receiver->readNonBlocking());
    for (typename std::vector<T>::iterator i = receiver->get().begin();
        i != receiver->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER + 4);
    }
    // We have received all values, so no more values should be available.
    BOOST_CHECK(!receiver->readNonBlocking());
    // When we send non-destructively, the value should also be preserved on the
    // sender side.
    sender->get().assign(N_ELEMENTS, SOME_NUMBER + 5);
    sender->write();
    BOOST_CHECK(receiver->readNonBlocking());
    for (typename std::vector<T>::iterator i = receiver->get().begin();
        i != receiver->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER + 5);
    }
    for (typename std::vector<T>::iterator i = sender->get().begin();
        i != sender->get().end(); ++i) {
      BOOST_CHECK(*i == SOME_NUMBER + 5);
    }
    // Calling writeDestructively() on a sender that has not the corresponding
    // flag set should result in an exception.
    senderReceiver =
            createSynchronizedProcessArray<T>(N_ELEMENTS, "", 0, 2, false);
    sender = senderReceiver.first;
    receiver = senderReceiver.second;
    BOOST_CHECK_THROW(sender->writeDestructively(), std::runtime_error);
  }

  template<class T>
  void ProcessArrayTest<T>::testVersionNumbers() {
    VersionNumberSource::SharedPtr versionNumberSource = boost::make_shared<
        VersionNumberSource>();
    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessArray<T>(N_ELEMENTS, "", 0, 2, true,
            TimeStampSource::SharedPtr(), versionNumberSource);
    typename ProcessArray<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessArray<T>::SharedPtr receiver = senderReceiver.second;
    // Initially, the version number should be zero.
    BOOST_CHECK(sender->getVersionNumber() == 0);
    BOOST_CHECK(receiver->getVersionNumber() == 0);
    // After sending destructively and receiving a value, the version number on
    // the receiver should be greater.
    VersionNumber initialVersionNumber = receiver->getVersionNumber();
    sender->get()[0] = 1;
    sender->writeDestructively();
    BOOST_CHECK(receiver->readNonBlocking());
    VersionNumber versionNumber = receiver->getVersionNumber();
    BOOST_CHECK(versionNumber > initialVersionNumber);
    BOOST_CHECK(receiver->get()[0] == 1);
    // When we send again specifying the same version number, there should be no
    // update of the receiver.
    sender->get()[0] = 2;
    sender->writeDestructively(versionNumber);
    BOOST_CHECK(!receiver->readNonBlocking());
    BOOST_CHECK(versionNumber == receiver->getVersionNumber());
    BOOST_CHECK(receiver->get()[0] == 1);
    // When we explicitly use a greater version number, the receiver should be
    // updated again.
    sender->get()[0] = 3;
    sender->writeDestructively(versionNumberSource->nextVersionNumber());
    BOOST_CHECK(receiver->readNonBlocking());
    BOOST_CHECK(receiver->getVersionNumber() > versionNumber);
    BOOST_CHECK(receiver->get()[0] == 3);
    // When we send non-destructively, the version number on the sender and the
    // receiver should match after sending and receiving.
    versionNumber = receiver->getVersionNumber();
    sender->get()[0] = 4;
    sender->write();
    BOOST_CHECK(receiver->readNonBlocking());
    BOOST_CHECK(receiver->getVersionNumber() > versionNumber);
    BOOST_CHECK(receiver->getVersionNumber() == sender->getVersionNumber());
    BOOST_CHECK(receiver->get()[0] == 4);
  }

}  //namespace ChimeraTK

test_suite*
init_unit_test_suite(int /*argc*/, char* /*argv*/[]) {
  framework::master_test_suite().p_name.value = "ProcessArray test suite";

  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<int32_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<uint32_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<int16_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<uint16_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<int8_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<uint8_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<double>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessArrayTestSuite<float>);

  return NULL;
}

