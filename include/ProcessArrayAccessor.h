#ifndef MTCA4U_PROCESS_ARRAY_ACCESSOR_H
#define MTCA4U_PROCESS_ARRAY_ACCESSOR_H

#include "ProcessArray.h"
#include "ProcessVariableAccessor.h"

namespace mtca4u{
  template<class UserType>
    class ProcessArrayAccessor: public ProcessVariableAccessor{

  public:
  ProcessArrayAccessor(typename ProcessArray<UserType>::SharedPtr const & processArray =
			typename ProcessArray<UserType>::SharedPtr())
    : ProcessVariableAccessor(processArray){
      //      _processArray( boost::static_pointer_cast< ProcessArray<UserType> >( _impl ) ){      
    }

    std::vector<UserType> & get(){
      return boost::static_pointer_cast< ProcessArray<UserType> >(_impl)->get();
    }
    std::vector<UserType> const & get() const{
      return boost::static_pointer_cast< ProcessArray<UserType> >(_impl)->getConst();
    }
    std::vector<UserType> const & getConst() const{
      return boost::static_pointer_cast< ProcessArray<UserType> >(_impl)->getConst();
    }
    
    UserType& operator[](unsigned int element) {
      return get()[element];
    }

    /** Access data with std::vector-like iterators */
    typedef typename std::vector<UserType>::iterator iterator;
    typedef typename std::vector<UserType>::const_iterator const_iterator;
    typedef typename std::vector<UserType>::reverse_iterator reverse_iterator;
    typedef typename std::vector<UserType>::const_reverse_iterator const_reverse_iterator;

    iterator begin() { return get().begin(); }
    const_iterator begin() const { return getConst().cbegin(); }
    const_iterator cbegin() const { return getConst().cbegin(); }
    iterator end() { return get().end(); }
    const_iterator end() const { return getConst().cend(); }
    const_iterator cend() const { return getConst().cend(); }
    reverse_iterator rbegin() { return get().rbegin(); }
    const_reverse_iterator rbegin() const { return getConst().crbegin(); }
    const_reverse_iterator crbegin() const { return getConst().crbegin(); }
    reverse_iterator rend() { return get().rend(); }
    const_reverse_iterator rend() const { return getConst().crend(); }
    const_reverse_iterator crend() const { return getConst().crend(); }

    size_t size() const{ return get().size(); }

    /* Swap content of (cooked) buffer with std::vector */
    void swap(std::vector<UserType> &x) {
      get().swap(x);
    }

    UserType* data() {
      return get().data();
    }
    UserType const * data() const{
      return get().data();
    }

  
  };

} //namespace mtca4u

#endif // MTCA4U_PROCESS_ARRAY_ACCESSOR_H
