#define BOOST_TEST_MODULE TypeChangingDecoratorTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "TypeChangingDecorator.h"
using namespace ChimeraTK;

#include <mtca4u/Device.h>

BOOST_AUTO_TEST_SUITE( TypeChangingDecoratorTestSuite )

template<class T, class IMPL_T>
void testDecorator(IMPL_T startReadValue, T expectedReadValue, T startWriteValue, IMPL_T exprectedWriteValue){
  mtca4u::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto scalar = d.getScalarRegisterAccessor<IMPL_T>("/SOME/SCALAR");
  auto anotherScalarAccessor = d.getScalarRegisterAccessor<IMPL_T>("/SOME/SCALAR");
  
  auto ndAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor< IMPL_T > >(scalar.getHighLevelImplElement());
  TypeChangingDecorator<T, IMPL_T> decoratedScalar(ndAccessor);

  BOOST_REQUIRE( decoratedScalar.getNumberOfChannels()==1);
  BOOST_REQUIRE( decoratedScalar.getNumberOfSamples()==1);
  
  anotherScalarAccessor = startReadValue;
  anotherScalarAccessor.write();
  BOOST_CHECK(scalar == 0);
  BOOST_CHECK(decoratedScalar.accessData(0) == 0);
  decoratedScalar.read();
  BOOST_CHECK( decoratedScalar.accessData(0) == expectedReadValue);

  decoratedScalar.accessData(0) = startWriteValue;
  decoratedScalar.write();
  anotherScalarAccessor.read();
  BOOST_CHECK( anotherScalarAccessor == exprectedWriteValue );
}

BOOST_AUTO_TEST_CASE( testAllDecorators ){
  testDecorator<int, int8_t>(12,12, 22, 22);
  testDecorator<int, uint8_t>(13,13, 23, 23);
  testDecorator<int, int16_t>(14,14, 24, 24);
  testDecorator<int, uint16_t>(15,15, 25, 25);
  testDecorator<int, int32_t>(16,16, 26, 26);
  testDecorator<int, uint32_t>(17,17, 27, 27);
  // only use values which habe an exact representation in float/fixed point
  testDecorator<int, float>(18.5,19, 28, 28);
  testDecorator<int, double>(19.5,20, 29, 29);
  //  testDecorator<int, string>();
}

BOOST_AUTO_TEST_SUITE_END()


