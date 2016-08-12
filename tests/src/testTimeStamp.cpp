// Define a name for the test module.
#define BOOST_TEST_MODULE TimeStampTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <boost/thread.hpp>

#include <ChimeraTK/ControlSystemAdapter/TimeStamp.h>

using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE( TimeStampTestSuite )

// Define one test after another. Each one needs a unique name.
  BOOST_AUTO_TEST_CASE( testConstructor ) {
    TimeStamp timeStamp(42);
    BOOST_CHECK(timeStamp.seconds == 42);
    BOOST_CHECK(timeStamp.nanoSeconds == 0);
    BOOST_CHECK(timeStamp.index0 == 0);
    BOOST_CHECK(timeStamp.index1 == 0);

    TimeStamp timeStamp2(43, 44, 45, 46);
    BOOST_CHECK(timeStamp2.seconds == 43);
    BOOST_CHECK(timeStamp2.nanoSeconds == 44);
    BOOST_CHECK(timeStamp2.index0 == 45);
    BOOST_CHECK(timeStamp2.index1 == 46);
  }

  BOOST_AUTO_TEST_CASE( testEqualsOperator ) {
    TimeStamp reference(3, 4, 5, 6);
    TimeStamp sameAsReference(3, 4, 5, 6);
    TimeStamp differentSeconds(9, 4, 5, 6);
    TimeStamp differentNanoSeconds(3, 9, 5, 6);
    TimeStamp differentIndex0(3, 4, 9, 6);
    TimeStamp differentIndex1(3, 4, 5, 9);

    BOOST_CHECK(reference == sameAsReference);
    BOOST_CHECK(!(reference == differentSeconds));
    BOOST_CHECK(!(reference == differentNanoSeconds));
    BOOST_CHECK(!(reference == differentIndex0));
    BOOST_CHECK(!(reference == differentIndex1));
  }

  BOOST_AUTO_TEST_CASE( testCurrentTime ) {
    TimeStamp currentTime1(TimeStamp::currentTime());
    // We sleep slightly more than a second, this should ensure that the time
    // changes even on systems that do not have a high precision timer.
    boost::this_thread::sleep_for(boost::chrono::milliseconds(1100));
    TimeStamp currentTime2(TimeStamp::currentTime());
    BOOST_CHECK(currentTime2.seconds > currentTime1.seconds);
  }

// After you finished all test you have to end the test suite.
  BOOST_AUTO_TEST_SUITE_END()
