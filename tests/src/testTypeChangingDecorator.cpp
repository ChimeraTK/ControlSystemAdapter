#define BOOST_TEST_MODULE TypeChangingDecoratorTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "TypeChangingDecorator.h"
#include <mtca4u/Device.h>

BOOST_AUTO_TEST_SUITE( TypeChangingDecoratorTestSuite )

BOOST_AUTO_TEST_CASE( testCreation ){
  mtca4u::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto scalar = d.getScalarRegisterAccessor<int>("/SOME/SCALAR");
  
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()


