#include <sstream>
#include <algorithm>
#include <iostream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <boost/lambda/lambda.hpp>

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
//  void testOnSetCallbackFunction();
//  void testOnGetCallbackFunction();
//  void testSetters();
//  void testSetWithoutCallback();
  void testIterators();
  void testRandomAccess();
  void testFrontBack();
  void testFill();
  void testAssignment();

 private:
//  unsigned int _callbackCounter;
//  unsigned int _callbackWithEqualValuesCounter;
//  unsigned int _getCounter;
  mtca4u::StubProcessArray<T> _stubProcessArray;
  mtca4u::ProcessArray<T> & _processArray;
  mtca4u::ProcessArray<T> const & _constArray; // a const reference to _processArray
  std::vector<T> _referenceVector;
  //  void set(T const & newValue, T const & oldValue);
  //  T get();

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
//    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testOnSetCallbackFunction,
//				processArrayTest ) );
//    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testOnGetCallbackFunction,
//				processArrayTest ) );
//    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testSetters,
//				processArrayTest ) );
//    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testSetWithoutCallback,
//				processArrayTest ) );
//    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testEquals,
//				processArrayTest ) );
//    add( BOOST_CLASS_TEST_CASE( &StubProcessArrayTest<T>::testConversionOperator,
//				processArrayTest ) );

  }
};

template <class T>
StubProcessArrayTest<T>::StubProcessArrayTest() 
  : _stubProcessArray(N_ELEMENTS), _processArray(_stubProcessArray), _constArray(_stubProcessArray),
    _referenceVector(N_ELEMENTS) 
{}

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
  
//  mtca4u::StubProcessArray<T> processT2(2);
//  _processT = processT2;
//  BOOST_CHECK( _processT == 2 );
//
//  // test self assigmnent, nothing should happen
//  _processT = _processT;
//  BOOST_CHECK( _processT == 2 );
//
//  mtca4u::ProcessArray<T> * processArrayPointer = &processT2;
//  processT2 = 17;
//  _processT = *processArrayPointer;
//  BOOST_CHECK( _processT == 17 );
//}
//
//template <class T>
//void StubProcessArrayTest<T>::set(T const & newValue, T const & oldValue){
//  if (newValue == oldValue){
//    ++_callbackWithEqualValuesCounter;
//  }
//  _t = newValue;
//  ++_callbackCounter;
//}
//
//template <class T>
//void StubProcessArrayTest<T>::testOnSetCallbackFunction(){
//  _processT.setOnSetCallbackFunction( boost::bind( &StubProcessArrayTest<T>::set,
//						   this, _1, _2 ) );
//  _processT.set(5);
//  BOOST_CHECK( _t == 5 );
//  BOOST_CHECK( _callbackCounter == 1 );
//  BOOST_CHECK( _callbackWithEqualValuesCounter == 0 );
//
//  _processT.clearOnSetCallbackFunction();
//  _processT.set(6);
//  BOOST_CHECK( _t == 5 );
//  BOOST_CHECK( _callbackCounter == 1 );
//  BOOST_CHECK( _callbackWithEqualValuesCounter == 0 );
//
//  // turn on the callback again, we use it to check the other operators
//  _processT.setOnSetCallbackFunction( boost::bind( &StubProcessArrayTest<T>::set,
//						   this, _1, _2 ) );  
//
//  // test same value assignment. Callback must be triggered
//  _processT.set(6);
//  BOOST_CHECK( _t == 6 );
//  BOOST_CHECK( _callbackCounter == 2 );
//  BOOST_CHECK( _callbackWithEqualValuesCounter == 1 );
//
//  // test self assignment. Callback should be triggered even now
//  _processT.set(_processT);
//  BOOST_CHECK( _t == 6 );
//  BOOST_CHECK( _callbackCounter == 3 );
//  BOOST_CHECK( _callbackWithEqualValuesCounter == 2 );
//}
//
//template <class T>
//T StubProcessArrayTest<T>::get(){
//  ++_getCounter;
//  return _t;
//}
//
//template <class T>
//void StubProcessArrayTest<T>::testOnGetCallbackFunction(){
//  _processT.setOnGetCallbackFunction( boost::bind( &StubProcessArrayTest<T>::get, this ) );
//  
//  BOOST_CHECK( _processT == 6 );
//  _t = 19;
//  BOOST_CHECK( _processT.get() == 19 );
//  BOOST_CHECK( _processT == 19 );
//  BOOST_CHECK( _getCounter == 1 );
//
//  _t = 17;
//
//  _processT.clearOnGetCallbackFunction();
//  BOOST_CHECK( _processT.get() == 19 );
//  BOOST_CHECK( _processT == 19 );
//  BOOST_CHECK( _getCounter == 1 );
//}
//
//template <class T>
//void StubProcessArrayTest<T>::testSetters(){
//  _processT.set( 7 );
//  BOOST_CHECK( _processT == 7 );
//  BOOST_CHECK( _t == 7 );
//  BOOST_CHECK( _callbackCounter == 4 );
//
//  mtca4u::StubProcessArray<T> processT1(8);
//  _processT.set( processT1 );
//  BOOST_CHECK( _processT == 8 );
//  BOOST_CHECK( _t == 8 );
//  BOOST_CHECK( _callbackCounter == 5 );
//
//  processT1=88;
//  mtca4u::ProcessArray<T> * processArrayPointer = &processT1;
//  _processT.set( *processArrayPointer );
//  BOOST_CHECK( _processT == 88 );
//  BOOST_CHECK( _t == 88 );
//  BOOST_CHECK( _callbackCounter == 6 );
//  
// 
//}
//
//template <class T>
//void StubProcessArrayTest<T>::testSetWithoutCallback(){
//  _processT.setWithoutCallback( 9 );
//  BOOST_CHECK( _processT == 9 );
//  BOOST_CHECK( _t == 88 );
//  BOOST_CHECK( _callbackCounter == 6 );
//
//  mtca4u::StubProcessArray<T> processT1(10);
//  _processT.setWithoutCallback( processT1 );
//  BOOST_CHECK( _processT == 10 );
//  BOOST_CHECK( _t == 88 );
//  BOOST_CHECK( _callbackCounter == 6 );
//  
//  // test self asignment, nothing should happen
//   _processT.setWithoutCallback( _processT );
//  BOOST_CHECK( _processT == 10 );
//  BOOST_CHECK( _t == 88 );
//  BOOST_CHECK( _callbackCounter == 6 );
//}
//
//template <class T>
//void StubProcessArrayTest<T>::testEquals(){
//  // we are using this all along. It uses the implicit conversion to T
//  _processT = 11;
//  BOOST_CHECK( _processT == 11 );
// 
//  // we can even compare two processarrays like this. Note that only the
//  // content of the member T is compared, not the callback functions.
//  mtca4u::StubProcessArray<T> processT1(N_ELEMENTS);
//  _processT.set( processT1 );
//  BOOST_CHECK( _processT ==  processT1 );
//}
//
//template <class T>
//void StubProcessArrayTest<T>::testConversionOperator(){
//  T a = _processT;
//  BOOST_CHECK( a == 12 );
//  BOOST_CHECK( a == _processT );
//  mtca4u::StubProcessArray<T> processT1(4);
//  BOOST_CHECK( _processT * processT1 == 48 );
//  BOOST_CHECK( 2.5 * processT1 == 10. );
//  _processT.set(_processT/3);
//  // compare to double. Implicit use if comparison (int, double)
//  BOOST_CHECK( _processT == 4. );
//  BOOST_CHECK( _t == 4. );
//
//  // This does not and should not compile. The implicit conversion returns a const
//  // reference so the callback is not bypassed. And there is no *= operator defined.
//  //  _processT *= 2;
//}

}//namespace mtca4u

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "StubProcessArray test suite";

  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<int32_t> );
//  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<uint32_t> );
//  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<int16_t> );
//  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<uint16_t> );
//  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<int8_t> );
//  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<uint8_t> );
//  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<double>);
//  framework::master_test_suite().add( new mtca4u::StubProcessArrayTestSuite<float>);

  return NULL;
}

