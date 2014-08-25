#define BOOST_TEST_MODULE StubProcessVariableFactoryTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "StubProcessVariableFactory.h"

BOOST_AUTO_TEST_SUITE( StubProcessVariableFactoryTestSuite )

BOOST_AUTO_TEST_CASE( testGetProcessVariable ){
  mtca4u::StubProcessVariableFactory stubProcessVariableFactory;

  boost::shared_ptr<mtca4u::ProcessVariable<int> > firstInteger = 
    stubProcessVariableFactory.getProcessVariable<int>("firstInteger");

  firstInteger->set(42);

  boost::shared_ptr<mtca4u::ProcessVariable<int> > secondInteger = 
     stubProcessVariableFactory.getProcessVariable<int>("firstInteger");
 
  // make sure this is the same instance as the first integer
  BOOST_CHECK(*secondInteger == 42);
  secondInteger->set(43);
  BOOST_CHECK(*firstInteger == 43);

  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessVariable<uint64_t>("tooLong"), std::bad_typeid );

  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessVariable<double>("firstInteger"), boost::bad_any_cast );
}


BOOST_AUTO_TEST_SUITE_END()

