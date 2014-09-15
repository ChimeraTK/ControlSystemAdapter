#include <sstream>
#include <algorithm>
#include <iostream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>

#include "StubProcessArray.h"

namespace mtca4u{

/** A class that exists just to create a different type of ProcessArray for testing.
 */
template<class T>
class AssignTestProcessArray: public mtca4u::StubProcessArray<T>{
public:
  AssignTestProcessArray(size_t arraySize): mtca4u::StubProcessArray<T>(arraySize){}
};

/** The test class for the StubProcessArray.
 *  It is templated to be tested with all data types.
 */
template<class T>
class StubProcessArrayTest
{
 public:
  StubProcessArrayTest();
  static void testConstructors();
  void testIterators();
  void testRandomAccess();
  void testFrontBack();
  void testFill();
  void testAssignment();
  void testSetWithoutCallback();
  void testSet();
  void testGet();

 private:
  unsigned int _setCallbackCounter;
  unsigned int _getCallbackCounter;
  mtca4u::StubProcessArray<T> _stubProcessArray;
  mtca4u::ProcessArray<T> & _processArray;
  mtca4u::ProcessArray<T> const & _constArray; // a const reference to _processArray
  std::vector<T> _referenceVector;
  void set(ProcessArray<T> const & newValue);
  void get(ProcessArray<T> & toBeFilled, T fillValue);

  static size_t const N_ELEMENTS=12;
  static size_t const SOME_NUMBER=42; // just some number to add so the content if not equal to the index
};

/** The boost test suite which executes the StubProcessArrayTest.
 */
template <class T>
class StubProcessArrayTestSuite : public test_suite {
public:
  StubProcessArrayTestSuite(): test_suite("StubProcessArray test suite") {
    boost::shared_ptr< StubProcessArrayTest<T> > 
      processArrayTest( new StubProcessArrayTest<T> );

    add( BOOST_TEST_CASE( &StubProcessArrayTest<T>::testConstructors ) );

    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testIterators,
				processArrayTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testRandomAccess,
				processArrayTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testFrontBack,
				processArrayTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testFill,
				processArrayTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testAssignment,
    				processArrayTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testSetWithoutCallback,
				processArrayTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testSet,
				processArrayTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testGet,
				processArrayTest ) );
  }
};

template <class T>
StubProcessArrayTest<T>::StubProcessArrayTest() 
  :  _setCallbackCounter(0), _getCallbackCounter(0), 
     _stubProcessArray(N_ELEMENTS), _processArray(_stubProcessArray), _constArray(_stubProcessArray),
    _referenceVector(N_ELEMENTS) 
{  // register the callback functions
  _processArray.setOnSetCallbackFunction( 
     boost::bind( &StubProcessArrayTest<T>::set, this, _1) );

  _processArray.setOnGetCallbackFunction( 
     boost::bind( &StubProcessArrayTest<T>::get, this, _1, SOME_NUMBER) );
}

template <class T>
void StubProcessArrayTest<T>::testConstructors(){
  mtca4u::StubProcessArray<T> processArray(N_ELEMENTS);
  BOOST_CHECK( processArray.size() == N_ELEMENTS );
  BOOST_CHECK( processArray.empty() == false );

   mtca4u::StubProcessArray<T> emptyArray(0);
  BOOST_CHECK( emptyArray.size() == 0 );
  BOOST_CHECK( emptyArray.empty() == true );
}

template <class T>
void StubProcessArrayTest<T>::testIterators(){
  T i =SOME_NUMBER;
  for (typename mtca4u::ProcessArray<T>::iterator it= _processArray.begin();
       it != _processArray.end(); ++it){
    *it=i++;
  }

  // use the fact that we re friend and can directly access the underlying vector to verify
  for (size_t j = 0; j < N_ELEMENTS; ++j){
    BOOST_CHECK( _stubProcessArray._container[j] == static_cast<T>(j+SOME_NUMBER) );
  }

  //constant array with begin and end
  i=SOME_NUMBER;
  for (typename mtca4u::ProcessArray<T>::const_iterator it= _constArray.begin();
       it != _constArray.end(); ++it){
    BOOST_CHECK( *it == i++ );    
  }
  
  //non-constant array with const_iterator
  i=SOME_NUMBER;
  for (typename mtca4u::ProcessArray<T>::const_iterator it= _processArray.cbegin();
       it != _processArray.cend(); ++it){
    BOOST_CHECK( *it == i++ );    
  }

  // check that the iterators run with std algorithms
  std::sort( _processArray.rbegin(), _processArray.rend() );

  // again use the container to check on the modifications
  i=SOME_NUMBER+N_ELEMENTS;
  for (size_t j = 0; j < _stubProcessArray._container.size(); ++j){
    BOOST_CHECK( _stubProcessArray._container[j] == static_cast<T>(--i) );
  }
 
  //constant array with rbegin and rend
  i=SOME_NUMBER;
  for (typename mtca4u::ProcessArray<T>::const_reverse_iterator it= _constArray.rbegin();
       it != _constArray.rend(); ++it){
    BOOST_CHECK( *it == i++ );    
  }
  
  //non-constant array with const_iterator
  i=SOME_NUMBER;
  for (typename mtca4u::ProcessArray<T>::const_reverse_iterator it= _processArray.crbegin();
       it != _processArray.crend(); ++it){
    BOOST_CHECK( *it == i++ );    
  }
}

template <class T>
void StubProcessArrayTest<T>::testRandomAccess(){
  T i = SOME_NUMBER;
  // set via [] operator
  for (size_t j = 0; j < _processArray.size(); ++j){
    _processArray[j] = static_cast<T>(i++);
  }

  // check the container for verification
  i = SOME_NUMBER;
  for (size_t j = 0; j < _stubProcessArray._container.size(); ++j){
    BOOST_CHECK( _stubProcessArray._container[j] = static_cast<T>(i++) );
  }

  i = SOME_NUMBER;
  // check via const [] operator
  for (size_t j = 0; j < _processArray.size(); ++j){
    BOOST_CHECK( _constArray[j] == static_cast<T>(i++) );
  }  

  // now the same with at()
  // setting:
  i = SOME_NUMBER + _processArray.size();
    for (size_t j = 0; j < _processArray.size(); ++j){
      _processArray.at(j) = static_cast<T>(--i);
  }

  // check the container for verification
  i = SOME_NUMBER + _processArray.size();
  for (size_t j = 0; j < _stubProcessArray._container.size(); ++j){
    BOOST_CHECK( _stubProcessArray._container[j] = static_cast<T>(--i) );
  }

  i = SOME_NUMBER + _processArray.size();
  // check via const [] operator
  for (size_t j = 0; j < _processArray.size(); ++j){
    BOOST_CHECK( _constArray[j] == static_cast<T>(--i) );
  }  

  // check the out of range check
  BOOST_CHECK_THROW( _processArray.at(N_ELEMENTS), std::out_of_range);
  BOOST_CHECK_THROW( _constArray.at(N_ELEMENTS), std::out_of_range);
}

template <class T>
void StubProcessArrayTest<T>::testFrontBack(){
  BOOST_CHECK( _constArray.front() == static_cast<T>(SOME_NUMBER + N_ELEMENTS -1) );
  ++_processArray.front();
  // use the container to verify
  BOOST_CHECK( _stubProcessArray._container.front() == static_cast<T>(SOME_NUMBER + N_ELEMENTS) );

  BOOST_CHECK( _constArray.back() == static_cast<T>(SOME_NUMBER) );
  --_processArray.back();
  // use the container to verify
  BOOST_CHECK( _stubProcessArray._container.back() == static_cast<T>(SOME_NUMBER -1) );
}

template <class T>
void StubProcessArrayTest<T>::testFill(){
  _processArray.fill(SOME_NUMBER);
  for (typename mtca4u::ProcessArray<T>::const_iterator it= _constArray.cbegin();
       it != _constArray.cend(); ++it){
    BOOST_CHECK( *it == static_cast<T>(SOME_NUMBER) );    
  }
}

template <class T>
void StubProcessArrayTest<T>::testAssignment(){
  // prepare the reference container
  T i = SOME_NUMBER + 3;
  for (typename std::vector<T>::iterator it = _referenceVector.begin();
       it != _referenceVector.end(); ++it){
    *it = i++;
  }

  _processArray = _referenceVector;

  BOOST_CHECK( std::equal( _processArray.begin(),  _processArray.end(), _referenceVector.begin()) );

  mtca4u::StubProcessArray<T> stubReferenceArray(N_ELEMENTS);
  stubReferenceArray=_referenceVector;
  // reverse the array so something different is written, the directly to the StubProcessArray
  std::sort( stubReferenceArray.rbegin(), stubReferenceArray.rend() );

  _stubProcessArray = stubReferenceArray;
  // use rbegin of the reference vector. We did not reverse it.
  BOOST_CHECK( std::equal( _processArray.begin(),  _processArray.end(), _referenceVector.rbegin()) );

  // test self assignment to check if code coverage goes up
  _stubProcessArray = _stubProcessArray;

  // reverse again (to correct order) and test with the processArray assigment operator.
  std::sort( stubReferenceArray.begin(), stubReferenceArray.end() );
  _processArray = stubReferenceArray;
  BOOST_CHECK( std::equal( _processArray.begin(),  _processArray.end(), _referenceVector.begin()) );

  // and yet another test, this time with a different implementation of ProcessArray
  AssignTestProcessArray<T> assignTestProcessArray(N_ELEMENTS);
  assignTestProcessArray.setWithoutCallback(_referenceVector); // FIXME: why does the assignment operator not work?
  // we need to reverse again for the rest 
  std::sort(  assignTestProcessArray.rbegin(),  assignTestProcessArray.rend() );
  _processArray = assignTestProcessArray;
  BOOST_CHECK( std::equal( _processArray.begin(),  _processArray.end(), _referenceVector.rbegin()) );
  
  AssignTestProcessArray<T> tooLargeAssignTestArray(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray = tooLargeAssignTestArray, std::out_of_range );

  StubProcessArray<T> tooLargeStubArray(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray = tooLargeStubArray, std::out_of_range );
  BOOST_CHECK_THROW( _stubProcessArray = tooLargeStubArray, std::out_of_range );

  std::vector<T> tooLargeVector(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray = tooLargeVector, std::out_of_range );
}
  
template <class T>
void StubProcessArrayTest<T>::testSetWithoutCallback(){
  // fill something known so we can check that it changed
  _processArray.fill(5);

  std::vector<T> referenceVector(N_ELEMENTS, 6);

  _processArray.setWithoutCallback(referenceVector);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 6 );
  }

  StubProcessArray<T> referenceStubArray(N_ELEMENTS);
  referenceStubArray.fill(7);

  _processArray.setWithoutCallback(referenceStubArray);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 7 );
  }

  ProcessArray<T> & referenceArray = referenceStubArray;
  referenceArray.fill(8);
  _processArray.setWithoutCallback(referenceArray);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 8 );
  }
  
  AssignTestProcessArray<T> assignTestProcessArray(N_ELEMENTS);
  assignTestProcessArray.fill(9);
   _processArray.setWithoutCallback(assignTestProcessArray);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 9 );
  } 

  AssignTestProcessArray<T> tooLargeAssignTestArray(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray.setWithoutCallback( tooLargeAssignTestArray ), std::out_of_range );

  StubProcessArray<T> tooLargeStubArray(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray.setWithoutCallback( tooLargeStubArray ), std::out_of_range );
  BOOST_CHECK_THROW( _stubProcessArray.setWithoutCallback( tooLargeStubArray ), std::out_of_range );

  std::vector<T> tooLargeVector(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray.setWithoutCallback( tooLargeVector ), std::out_of_range ); 
}

template <class T>
void StubProcessArrayTest<T>::testSet(){
  // fill something known so we can check that it changed
  _processArray.fill(5);

  // even though a lot has been executed, no callback must have been executed until now.
  BOOST_CHECK( _setCallbackCounter == 0 );

  std::vector<T> referenceVector(N_ELEMENTS, 6);
  _processArray.set(referenceVector);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 6 );
  }
  BOOST_CHECK( _setCallbackCounter == 1 );

  StubProcessArray<T> referenceStubArray(N_ELEMENTS);
  referenceStubArray.fill(7);

  _processArray.set(referenceStubArray);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 7 );
  }
  BOOST_CHECK( _setCallbackCounter == 2 );

  // unregister the callback function and try again
  _processArray.clearOnSetCallbackFunction();

  referenceStubArray.fill(8);
  _processArray.set(referenceStubArray);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 8 );
  }
  BOOST_CHECK( _setCallbackCounter == 2 );
  
  std::fill(referenceVector.begin(), referenceVector.end(), 9);
   _processArray.set(referenceVector);
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == 9 );
  }
  BOOST_CHECK( _setCallbackCounter == 2 );
 
  StubProcessArray<T> tooLargeStubArray(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray.setWithoutCallback( tooLargeStubArray ), std::out_of_range );
  std::vector<T> tooLargeVector(N_ELEMENTS+1);
  BOOST_CHECK_THROW( _processArray.setWithoutCallback( tooLargeVector ), std::out_of_range ); 
}

template <class T>
void StubProcessArrayTest<T>::set(ProcessArray<T> const & ){
  ++_setCallbackCounter;
}

template <class T>
void StubProcessArrayTest<T>::get(ProcessArray<T> & toBeFilled, T fillValue ){
  toBeFilled.fill(fillValue);
  ++_getCallbackCounter;  
}

template <class T>
void StubProcessArrayTest<T>::testGet(){
  // fill something known so we can check that it changed
  _processArray.fill(5);

  // even though a lot has been executed, no callback must have been executed until now.
  BOOST_CHECK( _getCallbackCounter == 0 );

  // only the StubProcessArray has a get function
  std::vector<T> const & theGetResult = _stubProcessArray.get();
  BOOST_CHECK( _getCallbackCounter == 1 );
  // the get should have changed everything to SOME_NUMBER
  for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
    BOOST_CHECK( *it == static_cast<T>(SOME_NUMBER) );
  }
  // the get should have changed everything to SOME_NUMBER
  for( typename std::vector<T>::const_iterator it = theGetResult.begin();
       it !=  theGetResult.end(); ++it ){
    BOOST_CHECK( *it == static_cast<T>(SOME_NUMBER) );
  }

  // clear the getter callback function
  _processArray.clearOnGetCallbackFunction();
  _processArray.fill(5);
  
  std::vector<T> const & anotherGetResult = _stubProcessArray.get();
  BOOST_CHECK( _getCallbackCounter == 1 );
  // everything still has to be 5
   for( typename ProcessArray<T>::const_iterator it = _processArray.cbegin();
       it !=  _processArray.cend(); ++it ){
     BOOST_CHECK( *it == 5 );
  }
  for( typename std::vector<T>::const_iterator it = anotherGetResult.begin();
       it !=  anotherGetResult.end(); ++it ){
    BOOST_CHECK( *it == 5 );
  }
}

}//namespace mtca4u

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "StubProcessArray test suite";

  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<int32_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<uint32_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<int16_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<uint16_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<int8_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<uint8_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<double>);
  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<float>);

  return NULL;
}

