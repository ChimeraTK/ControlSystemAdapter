#define BOOST_TEST_MODULE PvAcessorsTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ProcessScalarAccessor.h"
#include "ProcessArray.h"
#include "ManualTimeStampSource.h" 

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

  // test copy construction (automatically created, but it's a feature we want
  ProcessScalarAccessor<int> thirdIntAccessor( secondIntAccessor );
  secondIntAccessor.set( 46 );
  BOOST_CHECK( thirdIntAccessor.get() == 46 );
}

BOOST_AUTO_TEST_CASE( testPVAccessor ){
  // testing the base class. We are mainly testing sending and receiving, so
  // we need a pair
  auto myTimeStampSource = boost::make_shared<ManualTimeStampSource>();
  TimeStamp testTimeStamp;
  testTimeStamp.seconds = 123;
  myTimeStampSource->setTimeStamp(testTimeStamp);

  auto senderReceiverPair = createSynchronizedProcessScalar<int>("anInt",
    0 /*start value*/, 1 /* nBuffers */, myTimeStampSource );

  ProcessScalarAccessor<int> scalarSender(senderReceiverPair.first);
  ProcessScalarAccessor<int> scalarReceiver(senderReceiverPair.second);

  ProcessVariableAccessor & sender = scalarSender;
  ProcessVariableAccessor & receiver = scalarReceiver;

  BOOST_CHECK( sender.getName() == "anInt" );
  BOOST_CHECK( sender.getValueType() == typeid(int) );
  BOOST_CHECK( sender.isArray() == false );

  BOOST_CHECK( sender.isReceiver() == false );
  BOOST_CHECK( sender.isSender() == true );
  BOOST_CHECK( receiver.isReceiver() == true );
  BOOST_CHECK( receiver.isSender() == false );

  BOOST_CHECK( !(sender.getTimeStamp() == testTimeStamp) );
  BOOST_CHECK( scalarReceiver != 2 );
  scalarSender.set(2); 
  sender.send();
  BOOST_CHECK( sender.getTimeStamp() == testTimeStamp );
  receiver.receive();
  BOOST_CHECK( receiver.getTimeStamp() == testTimeStamp );
  BOOST_CHECK( scalarReceiver == 2 );

}

BOOST_AUTO_TEST_CASE( testScalarOperations ){
  // test getters, setters  and operators
  auto scalarInt = createSimpleProcessScalar<int>("scalarInt",42);
  ProcessScalarAccessor<int> scalarIntAccessor(scalarInt);

  // implicit conversion
  int i;
  i = scalarIntAccessor;
  BOOST_CHECK( i == 42 );

  // assigment
  i = 43;
  scalarIntAccessor = i;
  // we test with the underlying impl
  BOOST_CHECK( scalarInt->get() == 43 );
  // now we can also test the get function
  BOOST_CHECK( scalarIntAccessor.get() == 43 );

  auto anotherScalarInt = createSimpleProcessScalar<int>("anotherScalarInt",44);
  scalarIntAccessor.set(*anotherScalarInt);
  BOOST_CHECK( scalarIntAccessor.get() == 44 );
  scalarIntAccessor.set( 45 );
  BOOST_CHECK( scalarIntAccessor.get() == 45 );

  ProcessScalarAccessor<int> anotherScalarIntAccessor(anotherScalarInt);
  anotherScalarIntAccessor.set(46);
  scalarIntAccessor.set( anotherScalarIntAccessor );
  BOOST_CHECK( scalarIntAccessor.get() == 46 );
  
}


BOOST_AUTO_TEST_SUITE_END()
