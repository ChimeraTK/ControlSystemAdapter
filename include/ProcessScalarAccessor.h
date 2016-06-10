#ifndef MTCA4U_PROCESS_SCALAR_ACCESSOR_H
#define MTCA4U_PROCESS_SCALAR_ACCESSOR_H

#include "ProcessScalar.h"

namespace mtca4u {

  template<class T>
    class ProcessScalarAccessor: public ProcessVariableAccessor{
  public:
    ProcessScalarAccessor(ProcessScalar<T>::SharedPtr & processScalar)
      : _processScalar(processScalar){      
    }

   /**
    * Assign the content of the template type. This operator behaves like
    * set(T).
    */
    ProcessScalarAccessor<T> & operator=(T const & t) {
      processScalar->set(t);
      return *this;
    }

    /**
     * Set the value of this process variable to the one of the other process
     * variable.
     */
    void set(ProcessScalar<T> const & other){
      processScalar->set(other);
    }

    /**
     * Set the value of this process variable to the specified one.
     */
    void set(T const & t){
       processScalar->set(t);     
    }

    /**
     * Automatic conversion operator which returns a \b copy of this process
     * variable's value. As no reference is returned, this cannot be used for
     * assignment.
     */
    operator T() const{
      return processScalar->get();
    }

    /**
     * Returns a \b copy of this process variable's value. As no reference is
     * returned, this cannot be used for assignment.
     */
    virtual T get() const{
      return processScalar->get();
    }
};

}

#endif // MTCA4U_PROCESS_SCALAR_ACCESSOR_H
