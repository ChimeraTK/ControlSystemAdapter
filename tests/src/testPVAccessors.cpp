#define BOOST_TEST_MODULE PvAcessorsTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

#include "ProcessScalarAccessor.h"
#include "ProcessArrayAccessor.h"
#include "ManualTimeStampSource.h" 

using namespace mtca4u;

BOOST_AUTO_TEST_SUITE( PvAccessorsTestSuite )

BOOST_AUTO_TEST_CASE( testScalarCreation ){
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

BOOST_AUTO_TEST_CASE( testArrayCreation ){
  // checking that the default constructor is working
  ProcessArrayAccessor<int> arrayIntAccessor;

  // There are two ways to create. Just run them both:
  auto firstIntArray = createSimpleProcessArray<int>(10,"first", 42);
  arrayIntAccessor.replace( firstIntArray );

  BOOST_CHECK( arrayIntAccessor.get().size() == 10 );
  BOOST_CHECK( arrayIntAccessor.get()[0] == 42 );
  
  auto secondIntArray = createSimpleProcessArray<int>(11,"second",43);
  ProcessArrayAccessor<int> secondIntArrayAccessor(secondIntArray);

  BOOST_CHECK( secondIntArrayAccessor.get().size() == 11 );
  BOOST_CHECK( secondIntArrayAccessor.get()[0] == 43 );

  arrayIntAccessor.replace( secondIntArrayAccessor );

  BOOST_CHECK( arrayIntAccessor.get().size() == 11 );
  BOOST_CHECK( arrayIntAccessor.get()[0] == 43 );
  // both must point to the same impl (shared prt pointing to the same instance
  
  for (auto & i : arrayIntAccessor.get()){
    i = 44;
  } 
  BOOST_CHECK( secondIntArrayAccessor.get()[0] == 44 );

  auto scalarInt = createSimpleProcessScalar<int>("scalarInt");
  BOOST_CHECK_THROW( secondIntArrayAccessor.replace(scalarInt), std::runtime_error) ;

  auto arrayFloat = createSimpleProcessArray<float>(10, "arrayFloat");
  BOOST_CHECK_THROW( secondIntArrayAccessor.replace(arrayFloat), std::runtime_error) ;

  // test copy construction (automatically created, but it's a feature we want
  ProcessArrayAccessor<int> thirdIntArrayAccessor( secondIntArrayAccessor );
  for (auto & i : secondIntArrayAccessor.get()){
    i = 46;
  } 
  BOOST_CHECK( thirdIntArrayAccessor.get()[0] == 46 );
}

BOOST_AUTO_TEST_CASE( testArrayOperations ){
  auto firstIntArray = createSimpleProcessArray<int>(10,"first", 42);
  ProcessArrayAccessor<int> arrayIntAccessor( firstIntArray );
  
  // get the underlying vector to prepare it
  int i = 45;
  for ( auto & c : arrayIntAccessor.get() ){
    c = i++;
  }

  // test the [] operator
  for (int j = 0; static_cast<unsigned int>(j) < arrayIntAccessor.size(); ++j){
    BOOST_CHECK( arrayIntAccessor[j] == 45+j );
  }

  // test the accessors. Start with modifiying the array.
  // NOTE: We are not really testing the accessors themselves, but that we get the
  // right one, so that we don't accidentally return the reverst iterator instead of
  // the forward one.
  for ( auto & c : arrayIntAccessor ){
    c++;
 }
 // Check with the [] operator, wich we already verified
  for (int j = 0; static_cast<unsigned int>(j) < arrayIntAccessor.size(); ++j){
    BOOST_CHECK( arrayIntAccessor[j] == 46+j );
  }

  // now recheck with the const iterator. Use a const object to check that it's really const.
  ProcessArrayAccessor<int> const & constArrayIntAccessor = arrayIntAccessor;
  i = 46;
  for (auto it = constArrayIntAccessor.cbegin(); it < constArrayIntAccessor.cend(); ++it ){
    BOOST_CHECK( (*it) == i++ );
  }

  // also check the const begin and end iterators
  i = 46;
  for (auto it = constArrayIntAccessor.begin(); it < constArrayIntAccessor.end(); ++it ){
    BOOST_CHECK( (*it) == i++ );
  }

  // also try the const get methods
  i = 46;
  for ( auto & c : constArrayIntAccessor.get() ){
    BOOST_CHECK( c == i++ );
  }
  i = 46;
  for ( auto & c : constArrayIntAccessor.getConst() ){
    BOOST_CHECK( c == i++ );
  }

  // Now test the reverse iterators
  // Modify with the writeable one
  i =78;
  for (auto it = arrayIntAccessor.rbegin(); it <  arrayIntAccessor.rend(); ++it){
    (*it) = i++;
  }

  // check with normal, already testes iterators
  i = 87;
  for (auto const & c : arrayIntAccessor){
    BOOST_CHECK( c == i-- );
  }

  // check with const reverse iterators
  i = 78;
  for (auto it = constArrayIntAccessor.rbegin();  it < constArrayIntAccessor.rend(); ++it){
    BOOST_CHECK( (*it) == i++ );
  }
  
  i = 78;
  for (auto it = constArrayIntAccessor.crbegin();  it < constArrayIntAccessor.crend(); ++it){
    BOOST_CHECK( (*it) == i++ );
  }

  // test swapping with a vector
  std::vector<int> v(10,42);
  arrayIntAccessor.swap(v);
  for (auto & c : constArrayIntAccessor ){
    BOOST_CHECK( c == 42 );
  }
  i= 87;
  for (auto const & c : v){
    BOOST_CHECK( c == i-- );
  }
 
  // set using the direct pointer
  int * ptr = arrayIntAccessor.data();
  for (size_t j = 0; j < arrayIntAccessor.size(); ++j){
    ptr[j] = 13 + j;
  }

  // check with iterator (already verified)
  i = 13;
  for (auto & c : constArrayIntAccessor ){
    BOOST_CHECK( c == i++ );
  }

  int const * cptr = constArrayIntAccessor.data();
  for (size_t j = 0; j < constArrayIntAccessor.size(); ++j){
    BOOST_CHECK( cptr[j] == static_cast<int>(13 + j) );
  }
  
  
}

BOOST_AUTO_TEST_SUITE_END()
