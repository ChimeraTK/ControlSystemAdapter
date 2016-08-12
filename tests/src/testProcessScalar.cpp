#include <sstream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <ProcessScalar.h>

#include <CountingProcessVariableListener.h>
#include <CountingTimeStampSource.h>

namespace ChimeraTK {
  /**
   * The test class for the ProcessScalar.
   * It is templated to be tested with all data types.
   */
  template<class T>
  struct ProcessScalarTest {
    static void testConstructors();
    static void testAssignment();
    static void testConversion();
    static void testGet();
    static void testSet();
    static void testSendNotification();
    static void testTimeStampSource();
    static void testSynchronization();
  };

  /**
   * The boost test suite which executes the ProcessScalarTest.
   */
  template<class T>
  class ProcessScalarTestSuite: public test_suite {
  public:
    ProcessScalarTestSuite() :
        test_suite("ProcessScalar test suite") {
      add(BOOST_TEST_CASE(&ProcessScalarTest<T>::testConstructors));
      add(BOOST_TEST_CASE(&ProcessScalarTest<T>::testAssignment));
      add(BOOST_TEST_CASE(&ProcessScalarTest<T>::testConversion));
      add(BOOST_TEST_CASE(&ProcessScalarTest<T>::testSet));
      add(BOOST_TEST_CASE(&ProcessScalarTest<T>::testSendNotification));
      add(BOOST_TEST_CASE(&ProcessScalarTest<T>::testTimeStampSource));
      add(BOOST_TEST_CASE(&ProcessScalarTest<T>::testSynchronization));
    }
  };

  template<class T>
  void ProcessScalarTest<T>::testConstructors() {
    typename ProcessScalar<T>::SharedPtr simpleScalar =
        createSimpleProcessScalar<T>();
    // The name should be empty and the value should have been initialized with
    // zero.
    BOOST_CHECK(simpleScalar->getName() == "");
    BOOST_CHECK(simpleScalar->get() == 0);
    BOOST_CHECK(!simpleScalar->isReceiver());
    BOOST_CHECK(!simpleScalar->isSender());
    // Now we test the constructor with non-default parameters.
    simpleScalar = createSimpleProcessScalar<T>("test", 42);
    BOOST_CHECK(simpleScalar->getName() == "test");
    BOOST_CHECK(simpleScalar->get() == 42);

    // Now we repeat the tests but for a sender / receiver pair. We do not test
    // the notification listener and time-stamp source because there is no
    // simple test for them and they are tested in a different method.
    typename std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessScalar<T>();
    typename ProcessScalar<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessScalar<T>::SharedPtr receiver = senderReceiver.second;
    BOOST_CHECK(sender->getName() == "");
    BOOST_CHECK(sender->get() == 0);
    BOOST_CHECK(receiver->getName() == "");
    BOOST_CHECK(receiver->get() == 0);
    BOOST_CHECK(!sender->isReceiver());
    BOOST_CHECK(sender->isSender());
    BOOST_CHECK(receiver->isReceiver());
    BOOST_CHECK(!receiver->isSender());
    senderReceiver = createSynchronizedProcessScalar<T>("test", 42);
    sender = senderReceiver.first;
    receiver = senderReceiver.second;
    BOOST_CHECK(sender->getName() == "test");
    BOOST_CHECK(sender->get() == 42);
    BOOST_CHECK(receiver->getName() == "test");
    BOOST_CHECK(receiver->get() == 42);
  }

  template<class T>
  void ProcessScalarTest<T>::testAssignment() {
    // It is sufficient to run the test for the simple variant because the
    // variants do not differ in their assignment operator and set method
    // implementation.
    typename ProcessScalar<T>::SharedPtr scalar1 =
        createSimpleProcessScalar<T>();
    typename ProcessScalar<T>::SharedPtr scalar2 = createSimpleProcessScalar<T>(
        "", 42);
    // Test the assignment of another process scalar.
    (*scalar1) = (*scalar2);
    BOOST_CHECK(scalar1->get() == 42);
    // Test the assignment of a raw number.
    (*scalar1) = 23;
    BOOST_CHECK(scalar1->get() == 23);
  }

  template<class T>
  void ProcessScalarTest<T>::testConversion() {
    // It is sufficient to run the test for the simple variant because the
    // variants do not differ in their conversion operator implementation.
    T number = 42;
    typename ProcessScalar<T>::SharedPtr simpleScalar =
        createSimpleProcessScalar<T>("", number);
    T otherNumber = (*simpleScalar);
    BOOST_CHECK(number == otherNumber);
  }

  template<class T>
  void ProcessScalarTest<T>::testSet() {
    // It is sufficient to run the test for the simple variant because the
    // variants do not differ in their assignment operator and set method
    // implementation.
    typename ProcessScalar<T>::SharedPtr scalar1 =
        createSimpleProcessScalar<T>();
    typename ProcessScalar<T>::SharedPtr scalar2 = createSimpleProcessScalar<T>(
        "", 42);
    // Test the assignment of another process scalar.
    scalar1->set(*scalar2);
    BOOST_CHECK(scalar1->get() == 42);
    // Test the assignment of a raw number.
    scalar1->set(23);
    BOOST_CHECK(scalar1->get() == 23);
  }

  template<class T>
  void ProcessScalarTest<T>::testSendNotification() {
    boost::shared_ptr<CountingProcessVariableListener> sendNotificationListener(
        boost::make_shared<CountingProcessVariableListener>());
    typename std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessScalar<T>("", 0, 1,
            TimeStampSource::SharedPtr(), sendNotificationListener);
    typename ProcessScalar<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessScalar<T>::SharedPtr receiver = senderReceiver.second;
    BOOST_CHECK(sendNotificationListener->count == 0);
    sender->send();
    BOOST_CHECK(sendNotificationListener->count == 1);
    BOOST_CHECK(sendNotificationListener->lastProcessVariable == receiver);
    sender->send();
    BOOST_CHECK(sendNotificationListener->count == 2);
    BOOST_CHECK(sendNotificationListener->lastProcessVariable == receiver);
  }

  template<class T>
  void ProcessScalarTest<T>::testTimeStampSource() {
    TimeStampSource::SharedPtr timeStampSource(
        boost::make_shared<CountingTimeStampSource>());
    typename std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessScalar<T>("", 0, 1, timeStampSource);
    typename ProcessScalar<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessScalar<T>::SharedPtr receiver = senderReceiver.second;
    sender->send();
    receiver->receive();
    BOOST_CHECK(receiver->getTimeStamp().index0 == 0);
    sender->send();
    receiver->receive();
    BOOST_CHECK(receiver->getTimeStamp().index0 == 1);
    sender->send();
    receiver->receive();
    BOOST_CHECK(receiver->getTimeStamp().index0 == 2);
  }

  template<class T>
  void ProcessScalarTest<T>::testSynchronization() {
    typename std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> senderReceiver =
        createSynchronizedProcessScalar<T>();
    typename ProcessScalar<T>::SharedPtr sender = senderReceiver.first;
    typename ProcessScalar<T>::SharedPtr receiver = senderReceiver.second;
    *sender = 42;
    BOOST_CHECK(sender->send());
    BOOST_CHECK(receiver->receive());
    BOOST_CHECK(*receiver == 42);
    // No more values should be available.
    BOOST_CHECK(!receiver->receive());
    *sender = 43;
    BOOST_CHECK(sender->send());
    *sender = 44;
    // When sending this value, we should lose the previous value.
    BOOST_CHECK(!sender->send());
    BOOST_CHECK(receiver->receive());
    BOOST_CHECK(*receiver == 44);
    BOOST_CHECK(!receiver->receive());
    // Now we create a sender/receiver pair with a slight larger queue and run
    // the test again to check that we can transfer more data without losing
    // values.
    senderReceiver = createSynchronizedProcessScalar<T>("", 0, 2);
    sender = senderReceiver.first;
    receiver = senderReceiver.second;
    *sender = 42;
    BOOST_CHECK(sender->send());
    *sender = 43;
    BOOST_CHECK(sender->send());
    BOOST_CHECK(receiver->receive());
    BOOST_CHECK(*receiver == 42);
    BOOST_CHECK(receiver->receive());
    BOOST_CHECK(*receiver == 43);
    // No more values should be available.
    BOOST_CHECK(!receiver->receive());
    BOOST_CHECK(*receiver == 43);
    *sender = 44;
    BOOST_CHECK(sender->send());
    *sender = 45;
    BOOST_CHECK(sender->send());
    *sender = 46;
    // When sending this value, we should lose the value we sent first.
    BOOST_CHECK(!sender->send());
    BOOST_CHECK(receiver->receive());
    BOOST_CHECK(*receiver == 45);
    BOOST_CHECK(receiver->receive());
    BOOST_CHECK(*receiver == 46);
    // No more values should be available.
    BOOST_CHECK(!receiver->receive());
    BOOST_CHECK(*receiver == 46);
  }

}  //namespace ChimeraTK

test_suite*
init_unit_test_suite(int /*argc*/, char* /*argv*/[]) {
  framework::master_test_suite().p_name.value = "ProcessScalar test suite";

  framework::master_test_suite().add(
      new ChimeraTK::ProcessScalarTestSuite<int32_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessScalarTestSuite<uint32_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessScalarTestSuite<int16_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessScalarTestSuite<uint16_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessScalarTestSuite<int8_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessScalarTestSuite<uint8_t>);
  framework::master_test_suite().add(
      new ChimeraTK::ProcessScalarTestSuite<double>);
  framework::master_test_suite().add(new ChimeraTK::ProcessScalarTestSuite<float>);

  return NULL;
}

