#define BOOST_TEST_MODULE PvAcessorsTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ProcessScalarAccessor.h"
#include "ProcessArray.h"

using namespace mtca4u;

BOOST_AUTO_TEST_SUITE( PvAccessorsTestSuite )

BOOST_AUTO_TEST_CASE( testScalarCreation ){
  // These tests are just to check that they are working

  // checking that the default constructor is working
  ProcessScalarAccessor<int> scalarIntAccessor;

  // There are two ways to create. Just run them both:
  auto firstInt = createSimpleProcessScalar<int>("first",42);
  scalarIntAccessor.replace( firstInt );

  BOOST_CHECK( scalarIntAccessor.get() == 42 );
  
  auto secondInt = createSimpleProcessScalar<int>("second",43);
  ProcessScalarAccessor<int> secondIntAccessor(secondInt);

  BOOST_CHECK( secondIntAccessor.get() == 43 );

  scalarIntAccessor.replace( secondIntAccessor );

  BOOST_CHECK( scalarIntAccessor.get() == 43 );
  // both must point to the same impl (shared prt pointing to the same instance
  
  scalarIntAccessor.set(44);
  BOOST_CHECK( secondIntAccessor.get() == 44 );

  auto intArray = createSimpleProcessArray<int>(10, "intArray");
  BOOST_CHECK_THROW( secondIntAccessor.replace(intArray), std::runtime_error) ;

  auto scalarFloat = createSimpleProcessScalar<float>("scalarFloat");
  BOOST_CHECK_THROW( secondIntAccessor.replace(scalarFloat), std::runtime_error) ;
}

BOOST_AUTO_TEST_CASE( testPVAccessor ){
  // testing the base class. 
  
}

BOOST_AUTO_TEST_SUITE_END()
