// Define a name for the test module.
#define BOOST_TEST_MODULE BidirectionalProcessArrayTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include <tuple>

#include "CountingProcessVariableListener.h"
#include "BidirectionalProcessArray.h"

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

using boost::make_shared;
using std::pair;
using std::tie;

// We only test an array of doubles because the logic is actually the same for
// all data types and the low-level stuff is already tested for all data types
// when testing the UnidirectionalProcessArray.
using DoubleArray = ProcessArray<double>;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE( BidirectionalProcessArrayTestSuite )

  // Test that a delayed incoming update with an older version does not
  // overwrite a newer version.
  BOOST_AUTO_TEST_CASE( testConflictingUpdates ) {
    ChimeraTK::ExperimentalFeatures::enable();

    DoubleArray::SharedPtr pv1, pv2;
    double initialValue = 3.5;
    tie(pv1, pv2) = createBidirectionalSynchronizedProcessArray(1, "", "", "",
        initialValue);
    // Initially, both sides should have the initial value.
    BOOST_CHECK(pv1->accessData(0) == initialValue);
    BOOST_CHECK(pv2->accessData(0) == initialValue);
    // Now we write to pv1. After that, pv1 should have been updated, but pv2
    // should still have its old value.
    double newValue1 = -2.1;
    pv1->accessData(0) = newValue1;
    pv1->write();
    BOOST_CHECK(pv1->accessData(0) == newValue1);
    BOOST_CHECK(pv2->accessData(0) == initialValue);
    // Now we write to pv2. We do this before reading pv2. As a result, both
    // pv1 and pv2 should have their respective new values.
    double newValue2 = 1.8;
    pv2->accessData(0) = newValue2;
    pv2->write();
    BOOST_CHECK(pv1->accessData(0) == newValue1);
    BOOST_CHECK(pv2->accessData(0) == newValue2);
    // Now we read pv2. As the incoming update is older than the current value,
    // it should be discarded.
    pv2->readNonBlocking();
    BOOST_CHECK(pv1->accessData(0) == newValue1);
    BOOST_CHECK(pv2->accessData(0) == newValue2);
    // Now we read pv1. The incoming update should overwrite the current value.
    pv1->readNonBlocking();
    BOOST_CHECK(pv1->accessData(0) == newValue2);
    BOOST_CHECK(pv2->accessData(0) == newValue2);
    // Now we write another value to pv2, but before reading it on pv1, we write
    // a new value there.
    double newValue3 = 25.0;
    pv2->accessData(0) = newValue3;
    pv2->write();
    BOOST_CHECK(pv1->accessData(0) == newValue2);
    BOOST_CHECK(pv2->accessData(0) == newValue3);
    double newValue4 = 100.0;
    pv1->accessData(0) = newValue4;
    pv1->write();
    BOOST_CHECK(pv1->accessData(0) == newValue4);
    BOOST_CHECK(pv2->accessData(0) == newValue3);
    // Now we read pv1. The incoming update should be discarded.
    pv1->readNonBlocking();
    BOOST_CHECK(pv1->accessData(0) == newValue4);
    BOOST_CHECK(pv2->accessData(0) == newValue3);
    // Now we read pv2. The incoming update should succeed.
    pv2->readNonBlocking();
    BOOST_CHECK(pv1->accessData(0) == newValue4);
    BOOST_CHECK(pv2->accessData(0) == newValue4);
  }

  // Test that passing on values (e.g. to other ApplicationCore modules) and sending back corrected values works as
  // expected
  BOOST_AUTO_TEST_CASE( testPassingOnWithCorrection ) {
    ChimeraTK::ExperimentalFeatures::enable();

    // Two pairs of PVs, "s" is for sending and "r" for receiving end (assuming a "favoured" direction - this is only
    // for clarification of the test scenario). Values read from Ar are passed on to Bs, after limiting the value
    // to be >= 0. Values read from Br are multiplied with a factor of -2 and sent back.
    DoubleArray::SharedPtr As, Ar, Bs, Br;
    double initialValue = 0.5;
    tie(As, Ar) = createBidirectionalSynchronizedProcessArray(1, "", "", "", initialValue);
    tie(Bs, Br) = createBidirectionalSynchronizedProcessArray(1, "", "", "", initialValue);
    // Initially, all endssides should have the initial value.
    BOOST_CHECK_CLOSE(As->accessData(0), initialValue, 0.001);
    BOOST_CHECK_CLOSE(Ar->accessData(0), initialValue, 0.001);
    BOOST_CHECK_CLOSE(Bs->accessData(0), initialValue, 0.001);
    BOOST_CHECK_CLOSE(Br->accessData(0), initialValue, 0.001);
    // A new value is written to As and received by Ar
    As->accessData(0) = 42.0;
    As->write();
    Ar->read();
    BOOST_CHECK_CLOSE(Ar->accessData(0), 42.0, 0.001);
    // Value is limited to be >= 0 and hence written back to Ar. It is also passed on to Bs.
    Ar->accessData(0) = 1.0;
    Bs->accessData(0) = 1.0;
    Ar->write(Ar->getVersionNumber());
    Bs->write(Ar->getVersionNumber());
    // As and Br receive the limited value.
    As->read();
    BOOST_CHECK_CLOSE(As->accessData(0), 1.0, 0.001);
    Br->read();
    BOOST_CHECK_CLOSE(Br->accessData(0), 1.0, 0.001);
    // The value at Br is multiplied by -2 and written back
    Br->accessData(0) = -2.0;
    Br->write(Br->getVersionNumber());
    // Bs receives the multipled value, which is limited again to be >= 0 and hence written again to Bs. It ls also
    // passed back on to Ar
    Bs->read();
    BOOST_CHECK_CLOSE(Bs->accessData(0), -2.0, 0.001);
    Bs->accessData(0) = 0.0;
    Ar->accessData(0) = 0.0;
    Ar->write(Bs->getVersionNumber());
    Bs->write(Bs->getVersionNumber());
    // Both As and Br receive the multiplied and limited value.
    As->read();
    BOOST_CHECK_CLOSE(As->accessData(0), 0.0, 0.001);
    Br->read();
    BOOST_CHECK_CLOSE(Br->accessData(0), 0.0, 0.001);
  }

  // Test that send notification listeners are called correctly.
  BOOST_AUTO_TEST_CASE( testListeners ) {
    ChimeraTK::ExperimentalFeatures::enable();

    DoubleArray::SharedPtr pv1, pv2;
    auto listener1 = make_shared<CountingProcessVariableListener>();
    auto listener2 = make_shared<CountingProcessVariableListener>();
    tie(pv1, pv2) = createBidirectionalSynchronizedProcessArray(1, "", "", "",
        0.0, 2, listener1, listener2);
    // Initially, the two listeners should not have received any notifications.
    BOOST_CHECK(listener1->count == 0);
    BOOST_CHECK(!listener1->lastProcessVariable);
    BOOST_CHECK(listener2->count == 0);
    BOOST_CHECK(!listener2->lastProcessVariable);
    // Now we write pv1. This should lead to listener1 being notified that pv2
    // is now ready to be read.
    pv1->write();
    BOOST_CHECK(listener1->count == 1);
    BOOST_CHECK(listener1->lastProcessVariable == pv2);
    BOOST_CHECK(listener2->count == 0);
    BOOST_CHECK(!listener2->lastProcessVariable);
    // Now we write pv2. This should lead to listener2 being notified that pv1
    // is now ready to be read.
    pv2->readNonBlocking();
    pv2->write();
    BOOST_CHECK(listener1->count == 1);
    BOOST_CHECK(listener1->lastProcessVariable == pv2);
    BOOST_CHECK(listener2->count == 1);
    BOOST_CHECK(listener2->lastProcessVariable == pv1);
    // We reset the pointers stored in the listeners in order to avoid a cyclic
    // dependency.
    listener1->lastProcessVariable.reset();
    listener2->lastProcessVariable.reset();
  }

  // Test that the data-transfer mechanism works.
  BOOST_AUTO_TEST_CASE( testSync ) {
    ChimeraTK::ExperimentalFeatures::enable();

    DoubleArray::SharedPtr pv1, pv2;
    double initialValue = 2.0;
    tie(pv1, pv2) = createBidirectionalSynchronizedProcessArray(1, "", "", "",
        initialValue, 2);
    // Both sides should be initialized to zero.
    BOOST_CHECK(pv1->accessData(0) == initialValue);
    BOOST_CHECK(pv2->accessData(0) == initialValue);
    // Both sides should have the same time stamp and version number.
    BOOST_CHECK(pv1->getVersionNumber() == pv2->getVersionNumber());
    // We save the time stamp and version number so that we can reuse it in
    // later checks.
    auto initialVersionNumber = pv1->getVersionNumber();
    // Nothing has been written yet, so calling read() should not have any
    // effects.
    pv1->readNonBlocking();
    pv2->readNonBlocking();
    BOOST_CHECK(pv1->accessData(0) == initialValue);
    BOOST_CHECK(pv1->getVersionNumber() == initialVersionNumber);
    BOOST_CHECK(pv2->accessData(0) == initialValue);
    BOOST_CHECK(pv2->getVersionNumber() == initialVersionNumber);
    // Now we write on pv1.
    double newValue1 = 5.0;
    pv1->accessData(0) = newValue1;
    pv1->write();
    // After writing, the time stamp and version number should have increased on
    // pv1, but pv2 should not have changed.
    BOOST_CHECK(pv1->accessData(0) == newValue1);
    BOOST_CHECK(pv1->getVersionNumber() > initialVersionNumber);
    auto newVersionNumber1 = pv1->getVersionNumber();
    BOOST_CHECK(pv2->accessData(0) == initialValue);
    BOOST_CHECK(pv2->getVersionNumber() == initialVersionNumber);
    // Now we read pv2. After this, the two PVs should match again.
    pv2->readNonBlocking();
    BOOST_CHECK(pv1->accessData(0) == newValue1);
    BOOST_CHECK(pv1->getVersionNumber() == newVersionNumber1);
    BOOST_CHECK(pv2->accessData(0) == newValue1);
    BOOST_CHECK(pv2->getVersionNumber() == newVersionNumber1);
    // Now, we write on pv2.
    double newValue2 = 42.0;
    pv2->accessData(0) = newValue2;
    pv2->write();
    // After writing, the time stamp and version number should have increased on
    // pv2, but pv1 should not have changed.
    BOOST_CHECK(pv2->accessData(0) == newValue2);
    BOOST_CHECK(pv2->getVersionNumber() > newVersionNumber1);
    auto newVersionNumber2 = pv2->getVersionNumber();
    BOOST_CHECK(pv1->accessData(0) == newValue1);
    BOOST_CHECK(pv1->getVersionNumber() == newVersionNumber1);
    // Now we read pv1. After this, the two PVs should match again.
    pv1->readNonBlocking();
    BOOST_CHECK(pv1->accessData(0) == newValue2);
    BOOST_CHECK(pv1->getVersionNumber() == newVersionNumber2);
    BOOST_CHECK(pv2->accessData(0) == newValue2);
    BOOST_CHECK(pv2->getVersionNumber() == newVersionNumber2);
  }

  // Test that time-stamp source are used correctly.
  BOOST_AUTO_TEST_CASE( testTimeStampSources ) {
    ChimeraTK::ExperimentalFeatures::enable();

    DoubleArray::SharedPtr pv1, pv2;
    tie(pv1, pv2) = createBidirectionalSynchronizedProcessArray(1, "", "", "",
        0.0, 2);
    // After writing pv1, timestampSource1 should have a count of 1.
    pv1->write();
    pv2->readNonBlocking();
    // After writing pv2, timestampSource2 should also have a count of 1.
    pv2->write();
    pv1->readNonBlocking();
  }

  // After you finished all test you have to end the test suite.
  BOOST_AUTO_TEST_SUITE_END()
