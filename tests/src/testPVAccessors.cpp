#define BOOST_TEST_MODULE PvAcessorsTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ProcessScalarAccessor.h"

using namespace mtca4u;

BOOST_AUTO_TEST_SUITE( PvAccessorsTestSuite )

BOOST_AUTO_TEST_CASE( testScalarCreation ){
  // checking that the default constructor is working
  ProcessScalarAccessor<int> scalarIntAccessor;

}

BOOST_AUTO_TEST_SUITE_END()
