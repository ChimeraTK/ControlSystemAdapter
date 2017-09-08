#define BOOST_TEST_MODULE TypeChangingDecoratorTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "TypeChangingDecorator.h"
using namespace ChimeraTK;

#include <mtca4u/Device.h>

bool test_close(double a, double b, double tolerance = 0.0001){
  return (std::fabs(a - b) < tolerance );
}

BOOST_AUTO_TEST_SUITE( TypeChangingDecoratorTestSuite )

template<class T, class IMPL_T>
void testDecorator(double startReadValue, T expectedReadValue, T startWriteValue, double expectedWriteValue){
  mtca4u::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto scalar = d.getScalarRegisterAccessor<IMPL_T>("/SOME/SCALAR");
  auto anotherScalarAccessor = d.getScalarRegisterAccessor<double>("/SOME/SCALAR");
  
  auto ndAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor< IMPL_T > >(scalar.getHighLevelImplElement());
  TypeChangingDecorator<T, IMPL_T> decoratedScalar(ndAccessor);

  BOOST_REQUIRE( decoratedScalar.getNumberOfChannels()==1);
  BOOST_REQUIRE( decoratedScalar.getNumberOfSamples()==1);
  
  anotherScalarAccessor = startReadValue;
  anotherScalarAccessor.write();
  // check that the values are different at start so we know the test is sensitive
  BOOST_CHECK( !test_close(decoratedScalar.accessData(0), expectedReadValue) );
  decoratedScalar.read();
  // internal precision of the register is 16 fractional bits fixed point
  BOOST_CHECK( test_close(decoratedScalar.accessData(0), expectedReadValue) );

  decoratedScalar.accessData(0) = startWriteValue;
  decoratedScalar.write();
  anotherScalarAccessor.read();
  BOOST_CHECK( test_close(anotherScalarAccessor, expectedWriteValue) );
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
  testDecorator<int, std::string>(101, 101, 102, 102);
}

BOOST_AUTO_TEST_SUITE_END()


