#define BOOST_TEST_MODULE TypeChangingDecoratorTest

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "TypeChangingDecorator.h"
using namespace ChimeraTK;

#include <mtca4u/Device.h>

// always test close for floating point values. Don't rely on exact binary representations
bool test_close(double a, double b, double tolerance = 0.0001){
  return (std::fabs(a - b) < tolerance );
}

BOOST_AUTO_TEST_SUITE( TypeChangingDecoratorTestSuite )

// the startReadValue and expectedWriteValue are from the register in the dummy device, which is 32 bit fixed point singed with 16 fractional bits, thus we talk to it as double from the test
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
  testDecorator<int, float>(18.5,19, 28, 28);
  testDecorator<int, float>(18.4,18, 28, 28);
  testDecorator<int, double>(19.5,20, 29, 29);
  testDecorator<int, double>(19.4,19, 29, 29);
  testDecorator<int, std::string>(101, 101, 102, 102);

  testDecorator<float, int8_t>(112,112, -122.4, -122);
  testDecorator<float, int8_t>(112,112, -122.5, -123);
  testDecorator<float, uint8_t>(113,113, 123.4, 123);
  testDecorator<float, uint8_t>(113,113, 123.5, 124);
  testDecorator<float, int16_t>(114,114, -124.4, -124);
  testDecorator<float, int16_t>(114,114, -124.5, -125);
  testDecorator<float, uint16_t>(115,115, 125.4, 125);
  testDecorator<float, uint16_t>(115,115, 125.5, 126);
  testDecorator<float, int32_t>(116,116, -126.4, -126);
  testDecorator<float, int32_t>(116,116, -126.5, -127);
  testDecorator<float, uint32_t>(117,117, 127.4, 127);
  testDecorator<float, uint32_t>(117,117, 127.5, 128);
  testDecorator<float, float>(118.5,118.5, 128.6, 128.6);
  testDecorator<float, double>(119.5,119.5, 129.6, 129.6);
  testDecorator<float, std::string>(101.1, 101.1, 102.2, 102.2);
  testDecorator<double, std::string>(201.1, 201.1, 202.2, 202.2);
  
}

BOOST_AUTO_TEST_SUITE_END()


