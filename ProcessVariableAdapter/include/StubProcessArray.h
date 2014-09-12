#ifndef MTCA4U_STUB_PROCESS_ARRAY_H
#define MTCA4U_STUB_PROCESS_ARRAY_H

#include "ProcessArray.h"

namespace mtca4u{

template<class T>
  class StubProcessArrayTest;

/** Implementation of the ProcessArray for the control system stub.
 *  It holds a std::vector as internal container and uses its iterators.
 */
template<class T>
  class StubProcessArray : public ProcessArray<T>{
 protected:
  std::vector<T> _container; ///< The instance of the container, a vector in this case.

  /** The function which is executed when set() is called
   */
  boost::function< void (ProcessArray<T> const & /*newValue*/) > _onSetCallbackFunction;

  /** The function which is executed when get() is called
   */  boost::function< void (ProcessArray<T> & /*toBeFilled*/) > _onGetCallbackFunction;  

  /** The constructors are private and can only be created by the factory.
   */
  StubProcessArray(size_t arraySize) : _container(arraySize){}

  /** The test is friend to allow testing of the constructors.*/
  friend class StubProcessArrayTest<T>;
  /** The factory is friend because someone has to be able to construct.*/
  friend class StubProcessArrayFactory;
 public:

//  void setOnSetCallbackFunction( 
//	boost::function< void (ProcessArray<T> const & /*newValue*/) > onSetCallbackFunction){
//    _onSetCallbackFunction = onSetCallbackFunction;
//  }
//  
//  void setOnGetCallbackFunction( boost::function<  void (ProcessArray<T> & /*toBeFilled*/) > onGetCallbackFunction){
//    _onGetCallbackFunction = onGetCallbackFunction;    
//  }
//
//  void clearOnSetCallbackFunction(){
//    _onSetCallbackFunction.clear();
//  }
//
//  void clearOnGetCallbackFunction(){
//    _onGetCallbackFunction.clear();
//  }
//  
//  /** Implementation of the copy assigmment operator for the StubProcessArray.
//   *  It is needed to prevent a default copy assignment operator from being created, which would also copy
//   *  the callback functions (although it never should be used because the calling code should not know
//   *  that this is a StubProcessArray and always use operator=(ProcessArray<T> const & other).
//   */
//  /*  In addition it's faster because it directly uses the vector's assignment operator avoids
//   *  calling iterating the incomming array.
//   */
//  StubProcessArray<T> & operator=(StubProcessArray<T> const & other){
//    if(&other==this){
//      return (*this);
//    }
//
//    setWithoutCallback(other._container);
//    return (*this);
//  }
//
//  StubProcessArray<T> & operator=(ProcessArray<T> const & other){
//    setWithoutCallback(other);
//    return (*this);
//  }
//
//  StubProcessArray<T> & operator=(std::vector<T> const & v){
//    setWithoutCallback(v);
//    return *this;
//  }
//
//  void setWithoutCallback(ProcessArray<T> const & other){
//    // fixme: check type for StubProcessArray and use more efficient function in this case
//    if (other.size() > _container.size()){
//      throw std::out_of_range("Assigned ProcessArray is too large.");
//    }
//    std::copy(other.cbegin(), other.cend(), _container.begin());
//  }
//
//  void setWithoutCallback(T const & t){
//    if (v.size() > _container.size()){
//      throw std::out_of_range("Assigned vector is too large.");
//    }
//    // Only asign the template class, but not the callback functions.
//    _container=v;
//  }
//
//  void set(ProcessArray<T> const & other){
//    StubProcessArray<T>::set( other.getWithoutCallback() );
//  }
//
//  void set(T const & t){
//    T oldValue = _t;
//    _t = t;
//    if (_onSetCallbackFunction){
//      _onSetCallbackFunction(t,oldValue);
//    }
//  }
//
//  operator T () const {
//    return _t;
//  }
//
//  T get(){
//    if (_onGetCallbackFunction){
//      _t=_onGetCallbackFunction();
//    }
//    return _t;
//  }
//
//  T getWithoutCallback() const{
//    return _t;
//  }
//

  typename mtca4u::ProcessArray<T>::iterator begin(){
    return typename mtca4u::ProcessArray<T>::iterator(_container.begin());
  }

  typename mtca4u::ProcessArray<T>::iterator end(){
    return typename mtca4u::ProcessArray<T>::iterator(_container.end());
  }

  typename mtca4u::ProcessArray<T>::const_iterator begin() const{
    return typename mtca4u::ProcessArray<T>::const_iterator(_container.begin());
  }

  typename mtca4u::ProcessArray<T>::const_iterator end() const{
    return typename mtca4u::ProcessArray<T>::const_iterator(_container.end());
  }

  typename mtca4u::ProcessArray<T>::const_iterator cbegin() const{
    return typename mtca4u::ProcessArray<T>::const_iterator(_container.begin());
  }

  typename mtca4u::ProcessArray<T>::const_iterator cend() const{
    return typename mtca4u::ProcessArray<T>::const_iterator(_container.end());
  }

  typename mtca4u::ProcessArray<T>::reverse_iterator rbegin(){
    return typename mtca4u::ProcessArray<T>::reverse_iterator(_container.rbegin());
  }

  typename mtca4u::ProcessArray<T>::reverse_iterator rend(){
    return typename mtca4u::ProcessArray<T>::reverse_iterator(_container.rend());
  }

  typename mtca4u::ProcessArray<T>::const_reverse_iterator rbegin() const{
    return typename mtca4u::ProcessArray<T>::const_reverse_iterator(_container.rbegin());
  }

  typename mtca4u::ProcessArray<T>::const_reverse_iterator rend() const{
    return typename mtca4u::ProcessArray<T>::const_reverse_iterator(_container.rend());
  }

  typename mtca4u::ProcessArray<T>::const_reverse_iterator crbegin() const{
    return typename mtca4u::ProcessArray<T>::const_reverse_iterator(_container.rbegin());
  }

  typename mtca4u::ProcessArray<T>::const_reverse_iterator crend() const{
    return typename mtca4u::ProcessArray<T>::const_reverse_iterator(_container.rend());
  }

};

}//namespace mtca4u

#endif// MTCA4U_STUB_PROCESS_ARRAY_H
