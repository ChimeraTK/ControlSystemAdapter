#define BOOST_TEST_MODULE TypeChangingDecoratorTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "TypeChangingDecorator.h"
using namespace ChimeraTK;

#include <mtca4u/Device.h>

BOOST_AUTO_TEST_SUITE( TypeChangingDecoratorTestSuite )

BOOST_AUTO_TEST_CASE( testCreation ){
  mtca4u::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto scalar = d.getScalarRegisterAccessor<int>("/SOME/SCALAR");
  auto anotherScalarAccessor = d.getScalarRegisterAccessor<int>("/SOME/SCALAR");

  // don't change the type to start with, use int,int 
  auto ndAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor< int> >(scalar.getHighLevelImplElement());
  TypeChangingDecorator<int, int> decoratedScalar(ndAccessor);
  
  BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()


