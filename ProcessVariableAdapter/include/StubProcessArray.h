#ifndef MTCA4U_STUB_PROCESS_ARRAY_H
#define MTCA4U_STUB_PROCESS_ARRAY_H

#include "ProcessArray.h"
#include <algorithm>
#include <typeinfo>

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
  size_t _nValidElements; ///< The information how many of the elements are valid.

  /** The function which is executed when set() is called
   */
  boost::function< void (ProcessArray<T> const & /*newValue*/) > _onSetCallbackFunction;

  /** The function which is executed when get() is called
   */  boost::function< void (ProcessArray<T> & /*toBeFilled*/) > _onGetCallbackFunction;  

  /** The constructors are private and can only be created by the factory.
   */
  StubProcessArray(size_t arraySize) : _container(arraySize), _nValidElements(arraySize){}

  /** The test is friend to allow testing of the constructors.*/
  friend class StubProcessArrayTest<T>;
  /** The factory is friend because someone has to be able to construct.*/
  friend class StubProcessVariableFactory;
 public:

  void setOnSetCallbackFunction( 
    boost::function< void (ProcessArray<T> const & /*newValue*/) > onSetCallbackFunction){
      _onSetCallbackFunction = onSetCallbackFunction;
  }
  
  void setOnGetCallbackFunction( boost::function<  void (ProcessArray<T> & /*toBeFilled*/) > onGetCallbackFunction){
    _onGetCallbackFunction = onGetCallbackFunction;    
  }

  void clearOnSetCallbackFunction(){
    _onSetCallbackFunction.clear();
  }

  void clearOnGetCallbackFunction(){
    _onGetCallbackFunction.clear();
  }
  
  /** Implementation of the copy assigmment operator for the StubProcessArray.
   *  It is needed to prevent a default copy assignment operator from being created, which would also copy
   *  the callback functions (although it never should be used because the calling code should not know
   *  that this is a StubProcessArray and always use operator=(ProcessArray<T> const & other).
   */
  /*  In addition it's faster because it directly uses the vector's assignment operator avoids
   *  calling iterating the incomming array.
   */
  StubProcessArray<T> & operator=(StubProcessArray<T> const & other){
    if(&other==this){
      return (*this);
    }

    setWithoutCallback(other._container);
    return (*this);
  }

  StubProcessArray<T> & operator=(ProcessArray<T> const & other){
    setWithoutCallback(other);
    return (*this);
  }

  StubProcessArray<T> & operator=(std::vector<T> const & v){
    setWithoutCallback(v);
    return *this;
  }

  void setWithoutCallback(ProcessArray<T> const & other){
    if ( typeid(other) == typeid(StubProcessArray<T>) ){
      // use static cast. We alredy did the type check.
      StubProcessArray<T> const * stubProcessArray = static_cast< StubProcessArray<T> const * >( & other );
      setWithoutCallback( stubProcessArray->_container );
    }else{
      if (other.size() != _container.size()){
	throw std::out_of_range("Assigned ProcessArray is too large.");
      }
      std::copy(other.cbegin(), other.cend(), _container.begin());
    }
  }

  void setWithoutCallback(std::vector<T> const & v){
    if (v.size() != _container.size()){
      throw std::out_of_range("Assigned vector is too large.");
    }
    // Only asign the template class, but not the callback functions.
    _container=v;
  }
  
  void set(ProcessArray<T> const & other){
    StubProcessArray<T>::setWithoutCallback( other );
    if (_onSetCallbackFunction){
      _onSetCallbackFunction(*this);
    }
  }

  void set(std::vector<T> const & v){
    StubProcessArray<T>::setWithoutCallback( v );
    if (_onSetCallbackFunction){
      _onSetCallbackFunction(*this);
    }
  }

  /** Implementation dependent getter for use in the adapter.
   *  It return a std::vector, as it is the internal representation of the
   *  control system stub.
   */
  std::vector<T> const & get(){
     if (_onGetCallbackFunction){
      _onGetCallbackFunction(*this);
     }
     return _container;
  }

  virtual T & operator[](size_t index){
    return _container[index];
  }

  virtual T const & operator[](size_t index) const{
    return _container[index];    
  }

  virtual T & at(size_t index){
    return _container.at(index);
  }

  virtual T const & at(size_t index) const{
    return _container.at(index);
  }

  virtual size_t size() const{
    return _container.size();
  }

  virtual T & front(){
    return _container.front();
  }

  virtual T const & front() const{
    return _container.front();
  }
  
  virtual T & back(){
    return _container.back();
  }

  virtual T const & back() const{
    return _container.back();
  }

  virtual bool empty() const{
    return _container.empty();
  }

  virtual void fill(T const & t){
    std::fill(_container.begin(), _container.end(), t);
  }  

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
