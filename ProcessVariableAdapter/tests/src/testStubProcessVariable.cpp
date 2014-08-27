#include <sstream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "StubProcessVariable.h"

namespace mtca4u{
/** The test class for the StubProcessVariable.
 *  It is templated to be tested with all data types.
 */
template<class T>
class StubProcessVariableTest
{
 public:
  StubProcessVariableTest();
  static void testConstructors();
  void testAssignment();
  void testOnSetCallbackFunction();
  void testOnGetCallbackFunction();
  void testSetters();
  void testSetWithoutCallback();
  void testEquals();
  void testConversionOperator();

 private:
  unsigned int _callbackCounter;
  unsigned int _callbackWithEqualValuesCounter;
  unsigned int _getCounter;
  mtca4u::StubProcessVariable<T> _processT;
  T _t;
  void set(T const & newValue, T const & oldValue);
  T get();
};

/** The boost test suite which executes the StubProcessVariableTest.
 */
template <class T>
class StubProcessVariableTestSuite : public test_suite {
public:
  StubProcessVariableTestSuite(): test_suite("StubProcessVariable test suite") {
    boost::shared_ptr< StubProcessVariableTest<T> > 
      processVariableTest( new StubProcessVariableTest<T> );

    add( BOOST_TEST_CASE( &StubProcessVariableTest<T>::testConstructors ) );

    add( BOOST_CLASS_TEST_CASE( &StubProcessVariableTest<T>::testAssignment,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessVariableTest<T>::testOnSetCallbackFunction,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessVariableTest<T>::testOnGetCallbackFunction,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessVariableTest<T>::testSetters,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessVariableTest<T>::testSetWithoutCallback,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessVariableTest<T>::testEquals,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &StubProcessVariableTest<T>::testConversionOperator,
				processVariableTest ) );

  }
};

template <class T>
StubProcessVariableTest<T>::StubProcessVariableTest() 
  : _callbackCounter(0),  _callbackWithEqualValuesCounter(0), _getCounter(0),
    _processT(0), _t(0){
}

template <class T>
void StubProcessVariableTest<T>::testConstructors(){
  mtca4u::StubProcessVariable<T> processT1;

  mtca4u::StubProcessVariable<T> processT2(2);
  BOOST_CHECK( processT2 == 2 );
  
  // copy constructor is private, object cannot be copied
  //mtca4u::StubProcessVariable<T> processT3( processT2 );
  //  BOOST_CHECK( processT3 == 2 );
}

template <class T>
void StubProcessVariableTest<T>::testAssignment(){
  _processT = 3;
  BOOST_CHECK( _processT == 3 );
  
  mtca4u::StubProcessVariable<T> processT2(2);
  _processT = processT2;
  BOOST_CHECK( _processT == 2 );

  // test self assigmnent, nothing should happen
  _processT = _processT;
  BOOST_CHECK( _processT == 2 );

  mtca4u::ProcessVariable<T> * processVariablePointer = &processT2;
  processT2 = 17;
  _processT = *processVariablePointer;
  BOOST_CHECK( _processT == 17 );
}

template <class T>
void StubProcessVariableTest<T>::set(T const & newValue, T const & oldValue){
  if (newValue == oldValue){
    ++_callbackWithEqualValuesCounter;
  }
  _t = newValue;
  ++_callbackCounter;
}

template <class T>
void StubProcessVariableTest<T>::testOnSetCallbackFunction(){
  _processT.setOnSetCallbackFunction( boost::bind( &StubProcessVariableTest<T>::set,
						   this, _1, _2 ) );
  _processT.set(5);
  BOOST_CHECK( _t == 5 );
  BOOST_CHECK( _callbackCounter == 1 );
  BOOST_CHECK( _callbackWithEqualValuesCounter == 0 );

  _processT.clearOnSetCallbackFunction();
  _processT.set(6);
  BOOST_CHECK( _t == 5 );
  BOOST_CHECK( _callbackCounter == 1 );
  BOOST_CHECK( _callbackWithEqualValuesCounter == 0 );

  // turn on the callback again, we use it to check the other operators
  _processT.setOnSetCallbackFunction( boost::bind( &StubProcessVariableTest<T>::set,
						   this, _1, _2 ) );  

  // test same value assignment. Callback must be triggered
  _processT.set(6);
  BOOST_CHECK( _t == 6 );
  BOOST_CHECK( _callbackCounter == 2 );
  BOOST_CHECK( _callbackWithEqualValuesCounter == 1 );

  // test self assignment. Callback should be triggered even now
  _processT.set(_processT);
  BOOST_CHECK( _t == 6 );
  BOOST_CHECK( _callbackCounter == 3 );
  BOOST_CHECK( _callbackWithEqualValuesCounter == 2 );
}

template <class T>
T StubProcessVariableTest<T>::get(){
  ++_getCounter;
  return _t;
}

template <class T>
void StubProcessVariableTest<T>::testOnGetCallbackFunction(){
  _processT.setOnGetCallbackFunction( boost::bind( &StubProcessVariableTest<T>::get, this ) );
  
  BOOST_CHECK( _processT == 6 );
  _t = 19;
  BOOST_CHECK( _processT.get() == 19 );
  BOOST_CHECK( _processT == 19 );
  BOOST_CHECK( _getCounter == 1 );

  _t = 17;

  _processT.clearOnGetCallbackFunction();
  BOOST_CHECK( _processT.get() == 19 );
  BOOST_CHECK( _processT == 19 );
  BOOST_CHECK( _getCounter == 1 );
}

template <class T>
void StubProcessVariableTest<T>::testSetters(){
  _processT.set( 7 );
  BOOST_CHECK( _processT == 7 );
  BOOST_CHECK( _t == 7 );
  BOOST_CHECK( _callbackCounter == 4 );

  mtca4u::StubProcessVariable<T> processT1(8);
  _processT.set( processT1 );
  BOOST_CHECK( _processT == 8 );
  BOOST_CHECK( _t == 8 );
  BOOST_CHECK( _callbackCounter == 5 );

  processT1=88;
  mtca4u::ProcessVariable<T> * processVariablePointer = &processT1;
  _processT.set( *processVariablePointer );
  BOOST_CHECK( _processT == 88 );
  BOOST_CHECK( _t == 88 );
  BOOST_CHECK( _callbackCounter == 6 );
  
 
}

template <class T>
void StubProcessVariableTest<T>::testSetWithoutCallback(){
  _processT.setWithoutCallback( 9 );
  BOOST_CHECK( _processT == 9 );
  BOOST_CHECK( _t == 88 );
  BOOST_CHECK( _callbackCounter == 6 );

  mtca4u::StubProcessVariable<T> processT1(10);
  _processT.setWithoutCallback( processT1 );
  BOOST_CHECK( _processT == 10 );
  BOOST_CHECK( _t == 88 );
  BOOST_CHECK( _callbackCounter == 6 );
  
  // test self asignment, nothing should happen
   _processT.setWithoutCallback( _processT );
  BOOST_CHECK( _processT == 10 );
  BOOST_CHECK( _t == 88 );
  BOOST_CHECK( _callbackCounter == 6 );
}

template <class T>
void StubProcessVariableTest<T>::testEquals(){
  // we are using this all along. It uses the implicit conversion to T
  _processT = 11;
  BOOST_CHECK( _processT == 11 );
 
  // we can even compare two processvariables like this. Note that only the
  // content of the member T is compared, not the callback functions.
  mtca4u::StubProcessVariable<T> processT1(12);
  _processT.set( processT1 );
  BOOST_CHECK( _processT ==  processT1 );
}

template <class T>
void StubProcessVariableTest<T>::testConversionOperator(){
  T a = _processT;
  BOOST_CHECK( a == 12 );
  BOOST_CHECK( a == _processT );
  mtca4u::StubProcessVariable<T> processT1(4);
  BOOST_CHECK( _processT * processT1 == 48 );
  BOOST_CHECK( 2.5 * processT1 == 10. );
  _processT.set(_processT/3);
  // compare to double. Implicit use if comparison (int, double)
  BOOST_CHECK( _processT == 4. );
  BOOST_CHECK( _t == 4. );

  // This does not and should not compile. The implicit conversion returns a const
  // reference so the callback is not bypassed. And there is no *= operator defined.
  //  _processT *= 2;
}

}//namespace mtca4u

test_suite*
init_unit_test_suite( int /*argc*/, char* /*argv*/ [] )
{
  framework::master_test_suite().p_name.value = "StubProcessVariable test suite";

  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<int32_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<uint32_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<int16_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<uint16_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<int8_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<uint8_t> );
  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<double>);
  framework::master_test_suite().add( new mtca4u::StubProcessVariableTestSuite<float>);

  return NULL;
}

