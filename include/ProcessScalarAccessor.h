#ifndef MTCA4U_PROCESS_SCALAR_ACCESSOR_H
#define MTCA4U_PROCESS_SCALAR_ACCESSOR_H

#include "ProcessScalar.h"
#include "ProcessVariableAccessor.h"

namespace ChimeraTK {

  template<class T>
    class ProcessScalarAccessor: public ProcessVariableAccessor{

    //  protected:
    //    typename ProcessScalar<T>::SharedPtr & _processScalar;

  public:
  ProcessScalarAccessor(typename ProcessScalar<T>::SharedPtr const & processScalar =
			typename ProcessScalar<T>::SharedPtr())
      : ProcessVariableAccessor(processScalar){
      //      _processScalar( boost::static_pointer_cast< ProcessScalar<T> >( _impl ) ){      
    }

   /**
    * Assign the content of the template type. This operator behaves like
    * set(T).
    */
    ProcessScalarAccessor<T> & operator=(T const & t) {
      boost::static_pointer_cast< ProcessScalar<T> >(_impl)->set(t);
      return *this;
    }
	
    /**
     * Set the value of this process variable to the one of the other process
     * variable.
     */
    void set(ProcessScalar<T> const & other){
      boost::static_pointer_cast< ProcessScalar<T> >(_impl)->set(other);
    }

    /**
     * Set the value of this process variable to the one of the other process
     * variable accessor
     */
    void set(ProcessScalarAccessor<T> const & other){
      set(other.get());
    }

    /**
     * Set the value of this process variable to the specified one.
     */
    void set(T const & t){
       boost::static_pointer_cast< ProcessScalar<T> >(_impl)->set(t);     
    }

    /**
     * Automatic conversion operator which returns a \b copy of this process
     * variable's value. As no reference is returned, this cannot be used for
     * assignment.
     */
    operator T() const{
      return boost::static_pointer_cast< ProcessScalar<T> >(_impl)->get();
    }

    /**
     * Returns a \b copy of this process variable's value. As no reference is
     * returned, this cannot be used for assignment.
     */
    T get() const{
      return boost::static_pointer_cast< ProcessScalar<T> >(_impl)->get();
    }

  };

}

#endif // MTCA4U_PROCESS_SCALAR_ACCESSOR_H
