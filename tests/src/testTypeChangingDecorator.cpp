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
  auto anotherImplTAccessor = d.getScalarRegisterAccessor<IMPL_T>("/SOME/SCALAR");
  
  auto ndAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor< IMPL_T > >(scalar.getHighLevelImplElement());
  TypeChangingRangeCheckingDecorator<T, IMPL_T> decoratedScalar(ndAccessor);

  BOOST_REQUIRE( decoratedScalar.getNumberOfChannels()==1);
  BOOST_REQUIRE( decoratedScalar.getNumberOfSamples()==1);

  BOOST_CHECK( decoratedScalar.getName() == "/SOME/SCALAR" );
  
  BOOST_CHECK( decoratedScalar.isReadable() );
  BOOST_CHECK( decoratedScalar.isWriteable() );
  BOOST_CHECK( !decoratedScalar.isReadOnly() );
  
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

  // repeat the read / write tests with all different functions

  // 1. The way a transfer group would call it:
  anotherScalarAccessor = startReadValue+1;
  anotherScalarAccessor.write();
  for (auto & hwAccessor : decoratedScalar.getHardwareAccessingElements()){
    hwAccessor->read();
  }
  // still nothing has changed on the user buffer
  BOOST_CHECK( test_close(decoratedScalar.accessData(0), startWriteValue) ); 
  decoratedScalar.postRead();
  BOOST_CHECK( test_close(decoratedScalar.accessData(0), expectedReadValue+1) );   

  decoratedScalar.accessData(0) = startWriteValue + 1 ;
  decoratedScalar.preWrite();
  // nothing changed on the device yet
  anotherScalarAccessor.read();
  BOOST_CHECK( test_close(anotherScalarAccessor, startReadValue+1) );
  for (auto & hwAccessor : decoratedScalar.getHardwareAccessingElements()){
    hwAccessor->write();
  }
  decoratedScalar.postWrite();

  anotherScalarAccessor.read();
  BOOST_CHECK( test_close(anotherScalarAccessor, expectedWriteValue+1) );

  // FIXME: We cannot test that the decorator is relaying doReadTransfer, doReadTransferLatest and
  // do readTransferLatest correctly with the dummy backend because they all point to the same
  // implementation. This we intentionally do not call them to indicate them uncovered.
  // We would have to use the control system adapter implementations with the queues to test it.

  BOOST_CHECK( decoratedScalar.isSameRegister( boost::dynamic_pointer_cast<const mtca4u::TransferElement>(anotherImplTAccessor.getHighLevelImplElement()) ) );

  // OK, I give up. I would have to repeat all tests ever written for a decorator, incl. transfer group, persistentDataStorage and everything. All I can do is leave the stuff intentionally uncovered so a reviewer can find the places.
}

BOOST_AUTO_TEST_CASE( testAllDecoratorConversions ){
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

BOOST_AUTO_TEST_CASE( testLoops ){
  // Test loops for numeric data types. One type is enough because it's template code
  // We don't have to test the string specialisations, arrays of strings are not allowed
  mtca4u::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto twoD = d.getTwoDRegisterAccessor<double>("/SOME/TWO_D");
  auto anotherAccessor = d.getTwoDRegisterAccessor<double>("/SOME/TWO_D");

  anotherAccessor[0][0] = 100.0;
  anotherAccessor[0][1] = 101.1;
  anotherAccessor[1][0] = 110.0;
  anotherAccessor[1][1] = 111.6;
  anotherAccessor[2][0] = 120.0;
  anotherAccessor[2][1] = 121.6;
  anotherAccessor.write();

  // device under test (dut)
  auto impl = boost::dynamic_pointer_cast<NDRegisterAccessor< double > >(twoD.getHighLevelImplElement());
  TypeChangingRangeCheckingDecorator<int, double> dut( impl );

  BOOST_REQUIRE( dut.getNumberOfChannels()==3);
  BOOST_REQUIRE( dut.getNumberOfSamples()==2);

  dut.read();

  BOOST_CHECK( dut.accessData(0,0) == 100);
  BOOST_CHECK( dut.accessData(0,1) == 101);
  BOOST_CHECK( dut.accessData(1,0) == 110);
  BOOST_CHECK( dut.accessData(1,1) == 112);
  BOOST_CHECK( dut.accessData(2,0) == 120);
  BOOST_CHECK( dut.accessData(2,1) == 122);

  dut.accessData(0,0) = 200;
  dut.accessData(0,1) = 201;
  dut.accessData(1,0) = 210;
  dut.accessData(1,1) = 212;
  dut.accessData(2,0) = 220;
  dut.accessData(2,1) = 222;
  dut.write();

  anotherAccessor.read();

  BOOST_CHECK( test_close( anotherAccessor[0][0], 200 ));
  BOOST_CHECK( test_close( anotherAccessor[0][1], 201 ));
  BOOST_CHECK( test_close( anotherAccessor[1][0], 210 ));
  BOOST_CHECK( test_close( anotherAccessor[1][1], 212 ));
  BOOST_CHECK( test_close( anotherAccessor[2][0], 220 ));
  BOOST_CHECK( test_close( anotherAccessor[2][1], 222 ));
}

#define CHECK_THROW_PRINT( command , exception_type)           \
try{ \
    command;\
    BOOST_ERROR( std::string(# command) + " did not throw as excepted." ); \
  }catch( exception_type & e){\
  std::cout << "** For manually checking the exeption message of " << # command <<":\n"\
            << "   " << e.what() << std::endl;                              \
  }\


BOOST_AUTO_TEST_CASE( testRangeChecks ){
  // Just a few tests where the type changing decorator with conversion should throw
  // where the direct cast decorator changes the interpretation
  mtca4u::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto myInt = d.getScalarRegisterAccessor<int32_t>("/SOME/INT");
  auto myIntDummy = d.getScalarRegisterAccessor<int32_t>("/SOME/INT"); // the second accessor for the test
  auto myUInt = d.getScalarRegisterAccessor<uint32_t>("/SOME/UINT");
  auto myUIntDummy = d.getScalarRegisterAccessor<uint32_t>("/SOME/UINT");
  
  auto intNDAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor< int32_t > >(myInt.getHighLevelImplElement());
  TypeChangingRangeCheckingDecorator<uint32_t, int32_t> u2i(intNDAccessor);
  TypeChangingDirectCastDecorator<uint32_t, int32_t> directU2i(intNDAccessor); // don't try this at home: putting the same NDAccessor into different decorators can cause trouble
  
  auto uintNDAccessor = boost::dynamic_pointer_cast<NDRegisterAccessor< uint32_t > >(myUInt.getHighLevelImplElement());
  TypeChangingRangeCheckingDecorator<int32_t, uint32_t> i2u(uintNDAccessor);
  TypeChangingDirectCastDecorator<int32_t, uint32_t> directI2u(uintNDAccessor); // don't try this at home: putting the same NDAccessor into different decorators can cause trouble

  // the bit content is the same, but the interpretation is different.
  myIntDummy = 0xFFFFFFFF;
  myIntDummy.write();
  myUIntDummy = 0xFFFFFFFF;
  myUIntDummy.write();
  
  CHECK_THROW_PRINT( u2i.read(), boost::numeric::negative_overflow );
  CHECK_THROW_PRINT( i2u.read(), boost::numeric::positive_overflow );
  BOOST_CHECK_NO_THROW( directI2u.read() );
  BOOST_CHECK_NO_THROW( directU2i.read() );
  BOOST_CHECK( directU2i.accessData(0) == 0xFFFFFFFF );
  BOOST_CHECK( directI2u.accessData(0) == -1 );
  
  i2u.accessData(0) = 0xFFFFFFFE;
  u2i.accessData(0) = 0xFFFFFFFE;

  CHECK_THROW_PRINT( i2u.write(), boost::numeric::negative_overflow );
  CHECK_THROW_PRINT( u2i.write(), boost::numeric::positive_overflow );
}

BOOST_AUTO_TEST_CASE( testFactory ){
  mtca4u::Device d;
  d.open("sdm://./dummy=decoratorTest.map");
  auto scalar = d.getScalarRegisterAccessor<double>("/SOME/SCALAR");
  mtca4u::TransferElement & transferElement = scalar;

  auto decoratedScalar = getDecorator<int>(transferElement);
  BOOST_CHECK( decoratedScalar );
  // factory must detect the type of scalar and create the correct decorator
  auto castedScalar = boost::dynamic_pointer_cast< TypeChangingRangeCheckingDecorator< int, double> >( decoratedScalar) ;
  BOOST_CHECK( castedScalar );

  auto decoratedDirectConvertingScalar = getDecorator<int>(transferElement, DecoratorType::C_style_conversion);
  BOOST_CHECK( decoratedDirectConvertingScalar );

  auto castedDCScalar = boost::dynamic_pointer_cast< TypeChangingDirectCastDecorator< int, double> >( decoratedDirectConvertingScalar) ;
  BOOST_CHECK( castedDCScalar );
  
}

BOOST_AUTO_TEST_SUITE_END()


