#define BOOST_TEST_MODULE StubProcessVariableFactoryTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "StubProcessVariableFactory.h"

BOOST_AUTO_TEST_SUITE( StubProcessVariableFactoryTestSuite )

// This test checks the functionality of the ProcessVariableFactory, i.e. 
// insertion into the map, resolution of the any_cast, getting the same object if the same name is used.
// It need not be copied for all other factory types to be implemented.
BOOST_AUTO_TEST_CASE( testGetProcessVariable ){
  mtca4u::StubProcessVariableFactory stubProcessVariableFactory;

  boost::shared_ptr<mtca4u::ProcessVariable<int> > firstInteger = 
    stubProcessVariableFactory.getProcessVariable<int>("firstInteger");

  firstInteger->set(42);

  boost::shared_ptr<mtca4u::ProcessVariable<int> > secondInteger = 
     stubProcessVariableFactory.getProcessVariable<int>("firstInteger");
 
  boost::shared_ptr<mtca4u::ProcessVariable<int> > thirdInteger = 
     stubProcessVariableFactory.getProcessVariable<int>("thirdInteger");
  
  thirdInteger->set(55);
  
  // make sure the second integer is the same instance as the first integer, but the third is not
  BOOST_CHECK(*secondInteger == 42);
  secondInteger->set(43);
  BOOST_CHECK(*firstInteger == 43);
  BOOST_CHECK(*thirdInteger == 55);

  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessVariable<uint64_t>("tooLong"), std::bad_typeid );

  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessVariable<double>("firstInteger"), boost::bad_any_cast );
}

// This tests the createProcessVariable class, checks that all supported types can be created
BOOST_AUTO_TEST_CASE( testAllTypes ){
  mtca4u::StubProcessVariableFactory stubProcessVariableFactory;

  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<int32_t>("theInt") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<uint32_t>("theUnsignedInt") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<int16_t>("theShort") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<uint16_t>("theUnsignedShort") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<int8_t>("theChar") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<uint8_t>("theUnisgnedInt") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<float_t>("theFloat") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<double_t>("theDouble") );

}

BOOST_AUTO_TEST_SUITE_END()

