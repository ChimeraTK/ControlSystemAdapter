#include <sstream>

#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "ProcessVariable.h"

template<class T>
class ProcessVariableTest
{
 public:
  ProcessVariableTest();
  static void testConstructors();
  void testAssignment();
  void testCallbackFunction();
  void testSetters();
  void testSetWithoutCallback();
  void testEquals();
  void testConversionOperator();

 private:
  unsigned int _callbackCounter;
  mtca4u::ProcessVariable<T> _processT;
  T _t;
  void set(T const &);
};

template <class T>
class ProcessVariableTestSuite : public test_suite {
public:
  ProcessVariableTestSuite(): test_suite("ProcessVariable test suite") {
    boost::shared_ptr< ProcessVariableTest<T> > 
      processVariableTest( new ProcessVariableTest<T> );

    add( BOOST_TEST_CASE( &ProcessVariableTest<T>::testConstructors ) );

    add( BOOST_CLASS_TEST_CASE( &ProcessVariableTest<T>::testAssignment,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &ProcessVariableTest<T>::testCallbackFunction,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &ProcessVariableTest<T>::testSetters,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &ProcessVariableTest<T>::testSetWithoutCallback,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &ProcessVariableTest<T>::testEquals,
				processVariableTest ) );
    add( BOOST_CLASS_TEST_CASE( &ProcessVariableTest<T>::testConversionOperator,
				processVariableTest ) );

  }
};

// Although the compiler complains that argc and argv are not used they 
// cannot be commented out. This somehow changes the signature and linking fails.
test_suite*
init_unit_test_suite( int argc, char* argv[] )
{
  framework::master_test_suite().p_name.value = "ProcessVariable test suite";

  framework::master_test_suite().add( new ProcessVariableTestSuite<int> );
  //  framework::master_test_suite().add( new ProcessVariableTestSuite<double>);

  return NULL;
}

template <class T>
ProcessVariableTest<T>::ProcessVariableTest() 
  : _callbackCounter(0), _processT(0), _t(0){
}

template <class T>
void ProcessVariableTest<T>::testConstructors(){
  mtca4u::ProcessVariable<T> processT1;

  mtca4u::ProcessVariable<T> processT2(2);
  BOOST_CHECK( processT2 == 2 );
  
  // copy constructor is private, object cannot be copied
  //  mtca4u::ProcessVariable<T> processT3( processT2 );
  //  BOOST_CHECK( processT3 == 2 );
}

template <class T>
void ProcessVariableTest<T>::testAssignment(){
  _processT = 3;
  BOOST_CHECK( _processT == 3 );
  
  mtca4u::ProcessVariable<T> processT2(2);
  _processT = processT2;
  BOOST_CHECK( _processT == 2 );
}

template <class T>
void ProcessVariableTest<T>::set(T const & t){
  _t = t;
  ++_callbackCounter;
}

template <class T>
void ProcessVariableTest<T>::testCallbackFunction(){
  _processT.setOnChangeCallbackFunction( boost::bind( &ProcessVariableTest<T>::set,
						      this, _1 ) );
  _processT = 5;
  BOOST_CHECK( _t == 5 );
  BOOST_CHECK( _callbackCounter == 1 );

  _processT.clearOnChangeCallbackFunction();
  _processT = 6;
  BOOST_CHECK( _t == 5 );
  BOOST_CHECK( _callbackCounter == 1 );

  // turn on the callback again, we use it to check the other operators
  _processT.setOnChangeCallbackFunction( boost::bind( &ProcessVariableTest<T>::set,
						      this, _1 ) );  

  // test self assignment and same value assignment. Callback must not be triggered
  _processT = 6;
  BOOST_CHECK( _t == 5 );
  BOOST_CHECK( _callbackCounter == 1 );
  
  _processT = _processT;
  BOOST_CHECK( _t == 5 );
  BOOST_CHECK( _callbackCounter == 1 );
}

template <class T>
void ProcessVariableTest<T>::testSetters(){
  _processT.set( 7 );
  BOOST_CHECK( _processT == 7 );
  BOOST_CHECK( _t == 7 );
  BOOST_CHECK( _callbackCounter == 2 );

  mtca4u::ProcessVariable<T> processT1(8);
  _processT.set( processT1 );
  BOOST_CHECK( _processT == 8 );
  BOOST_CHECK( _t == 8 );
  BOOST_CHECK( _callbackCounter == 3 );
}

template <class T>
void ProcessVariableTest<T>::testSetWithoutCallback(){
  _processT.setWithoutCallback( 9 );
  BOOST_CHECK( _processT == 9 );
  BOOST_CHECK( _t == 8 );
  BOOST_CHECK( _callbackCounter == 3 );

  mtca4u::ProcessVariable<T> processT1(10);
  _processT.setWithoutCallback( processT1 );
  BOOST_CHECK( _processT == 10 );
  BOOST_CHECK( _t == 8 );
  BOOST_CHECK( _callbackCounter == 3 );
  
  // test self asignment, nothing should happen
   _processT.setWithoutCallback( _processT );
  BOOST_CHECK( _processT == 10 );
  BOOST_CHECK( _t == 8 );
  BOOST_CHECK( _callbackCounter == 3 );
}

template <class T>
void ProcessVariableTest<T>::testEquals(){
  // we are using this all along. It uses the implicit conversion to T
  _processT = 11;
  BOOST_CHECK( _processT == 11 );
 
  // we can even compare two processvariables like this. Note that only the
  // content of the member T is compared, not the callback functions.
  mtca4u::ProcessVariable<T> processT1(12);
  _processT.set( processT1 );
  BOOST_CHECK( _processT ==  processT1 );
}

template <class T>
void ProcessVariableTest<T>::testConversionOperator(){
  int a = _processT;
  BOOST_CHECK( a == 12 );
  BOOST_CHECK( a == _processT );
  mtca4u::ProcessVariable<T> processT1(4);
  BOOST_CHECK( _processT * processT1 == 48 );
  BOOST_CHECK( 2.5 * processT1 == 10. );
  _processT = _processT/3;
  // compare to double. Implicit use if comparison (int, double)
  BOOST_CHECK( _processT == 4. );
  BOOST_CHECK( _t == 4. );

  // This does not and should not compile. The implicit conversion returns a const
  // reference so the callback is not bypassed. And there is no *= operator defined.
  //  _processT *= 2;
}


