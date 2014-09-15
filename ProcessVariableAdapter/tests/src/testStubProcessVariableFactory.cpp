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

  // check that name conflicts with arrays work
  boost::shared_ptr<mtca4u::ProcessArray<int> > intArray = 
    stubProcessVariableFactory.getProcessArray<int>("intArray", 42);
  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessVariable<int>("intArray"),  boost::bad_any_cast );
}

// the same tests for getting arrays
BOOST_AUTO_TEST_CASE( testGetProcessArray ){
  mtca4u::StubProcessVariableFactory stubProcessVariableFactory;

  boost::shared_ptr<mtca4u::ProcessArray<int> > firstIntArray = 
    stubProcessVariableFactory.getProcessArray<int>("firstIntArray", 42);
  firstIntArray->fill(7);

  boost::shared_ptr<mtca4u::ProcessArray<int> > secondIntArray = 
     stubProcessVariableFactory.getProcessArray<int>("firstIntArray", 42);
 
  boost::shared_ptr<mtca4u::ProcessArray<int> > thirdIntArray = 
    stubProcessVariableFactory.getProcessArray<int>("thirdIntArray", 42);
  
  thirdIntArray->fill(5);
  
  // make sure the second integer array is the same instance as the first array, but the third is not
  for (mtca4u::ProcessArray<int>::const_iterator it = firstIntArray->cbegin();
       it != firstIntArray->cend(); ++it){
    BOOST_CHECK(*it == 7);
  }
  for (mtca4u::ProcessArray<int>::const_iterator it = secondIntArray->cbegin();
       it != secondIntArray->cend(); ++it){
    BOOST_CHECK(*it == 7);
  }

  // change the second and check the first
  secondIntArray->fill(43);
  for (mtca4u::ProcessArray<int>::const_iterator it = firstIntArray->cbegin();
       it != firstIntArray->cend(); ++it){
    BOOST_CHECK(*it == 43);
  }
  for (mtca4u::ProcessArray<int>::const_iterator it = thirdIntArray->cbegin();
       it != thirdIntArray->cend(); ++it){
    BOOST_CHECK(*it == 5);
  }

  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessArray<uint64_t>("unknownType", 12), std::bad_typeid );

  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessArray<double>("firstIntArray", 42), boost::bad_any_cast );

  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessArray<int>("firstIntArray", 41), std::length_error );

  // check that name conflicts with variables work
  boost::shared_ptr<mtca4u::ProcessVariable<int> > firstInt = 
    stubProcessVariableFactory.getProcessVariable<int>("firstInt");
  BOOST_CHECK_THROW( stubProcessVariableFactory.getProcessArray<int>("firstInt", 42), boost::bad_any_cast );
}



// This tests the createProcessVariable class, checks that all supported types can be created
BOOST_AUTO_TEST_CASE( testAllTypes ){
  mtca4u::StubProcessVariableFactory stubProcessVariableFactory;

  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<int32_t>("theInt") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<uint32_t>("theUnsignedInt") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<int16_t>("theShort") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<uint16_t>("theUnsignedShort") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<int8_t>("theChar") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<uint8_t>("theUnsignedChar") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<float_t>("theFloat") );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessVariable<double_t>("theDouble") );

  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<int32_t>("intArray", 42) );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<uint32_t>("uintArray", 42) );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<int16_t>("shortArray", 42) );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<uint16_t>("ushortArray", 42) );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<int8_t>("charArray", 42) );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<uint8_t>("ucharArray", 42) );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<float_t>("floatArray", 42) );
  BOOST_CHECK_NO_THROW( stubProcessVariableFactory.getProcessArray<double_t>("double", 42) );

}



BOOST_AUTO_TEST_SUITE_END()

