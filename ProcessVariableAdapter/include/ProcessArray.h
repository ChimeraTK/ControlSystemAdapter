#ifndef MTCA4U_PROCESS_ARRAY_H
#define MTCA4U_PROCESS_ARRAY_H

#include <vector>
#include <iterator>
#include <boost/range.hpp>
#include <boost/range/detail/any_iterator.hpp>
#include <boost/function.hpp>

namespace mtca4u{
  
  /** The ProcessArray implements the abstract control system access for array type variables.
   *  Appart from the callback functions it provides an interface similar to the C++ std::array,
   *  (not std::vector, wich allows resizing). The size is defined in the constructor and cannot
   *  be changed afterwards.
   *
   *  The getters, which are present in the ProcessVariable, have been replaced by
   *  the array specific functions like front(), back(), iterators and the [] operator.
   *  Note that these do not trigger the onGetCallbackFunction. The business logic has to trigger
   *  it manually. For the adapter a mechanism must be found to trigger this function when
   *  the variable is being requested by the control system, for example by implementing
   *  a  function like \code CONTROL_SYSTEM_ARRAY_TYPE const & get() const; \endcode
   *  in the concrete implementation, if required. Also get and set functions without callback for the
   *  control system type might be required.
   *  \code
   *  void set(CONTROL_SYSTEM_ARRAY_TYPE const &);
   *  void setWithoutCallback(CONTROL_SYSTEM_ARRAY_TYPE const &);
   *  CONTROL_SYSTEM_ARRAY_TYPE const & getWithoutCallback() const;
   *  \endcode
   *  But these are details which will change from implementation to implementation.
   * 
   *  The iterators use the boost::any_iterator for type erasure, so the calling code is agnostic to the 
   *  actual implementation. "Despite the underlying any_iterator being the fastest available implementation,
   *  the performance overhead of any_range is still appreciable due to the cost of virtual function calls
   *  required to implement increment, decrement, advance, equal etc."
   *  (Taken from the boost::any_range documentation.) This overhead seems acceptable for filling a control
   *  system variable, because the overhead/latency for network communication etc. is probably higher.
   *
   *  For internal calculations of the business logic it is recommended to use a std::vector a container.
   *  Althogh this class allows dynamic resizing at run time, which is not wanted for real-time applications,
   *  the reserve() function allows to pre-allocate the memory and access times are constant if the
   *  capacity is not exceeded. This is much more flexible than compile time sizes as required by
   *  std::array for instance.
   *  The set() functions and assignment operators are provided for std::vector<T> and ProcessArray<T>.
   */
  template<class T>
  class ProcessArray{
  private:
    /** The copy constructor is intentionally private and not implemented.
     *  The derrived class shall not implement a copy constructor. Like this
     *  it is assured that a compiler error is raised if a copy constructor is
     *  used somewhere.
     */
    ProcessArray(ProcessArray<T> const &);

  protected:
    /** A default constructor has to be specified if a copy constructor is specified. But why?
     */
    ProcessArray(){};

  public:
    typedef boost::range_detail::any_iterator<T, boost::random_access_traversal_tag, T &, std::ptrdiff_t > iterator;
    typedef boost::range_detail::any_iterator<T const, boost::random_access_traversal_tag, T const &, std::ptrdiff_t> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
//
//    /** Register a function which is called when the set() function is executed.
//     *  In contrast to the ProcessVariable, the signature of this function only 
//     *  contains the new value in order to avoid unnecessary copying of the array content.
//     */
//    virtual void setOnSetCallbackFunction( 
//      boost::function< void (ProcessArray<T> const & /*newValue*/) > onSetCallbackFunction)=0;
//
//    /** Register a function which is called when the get() function is executed.
//     *  The argument is a reference to the array to be filled, which eventually will be *this in the
//     *  calling code inside the ProcessArray implementation.
//     */
//    virtual void setOnGetCallbackFunction( 
//      boost::function< void (ProcessArray<T> & /*toBeFilled*/) > onGetCallbackFunction )=0;
//    
//    /** Clear the callback function for the set() method.
//     */  
//    virtual void clearOnSetCallbackFunction()=0;
//    
//    /** Clear the callback function for the get() method.
//     */  
//    virtual void clearOnGetCallbackFunction()=0;

    /** Assignment operator for another ProcessArray of the same template type.
     *  It can be of a different implementation, though. The size of the 
     *  assigned array must be smaller or equal than the target size.
     *  The nValidEntries value is set to the number of assiged values.
     *  It does not trigger the OnSetCallbackFunction.
     */
    virtual ProcessArray<T> & operator=(ProcessArray<T> const & other)=0;

    /** Assignment operator for a std::vector of the template data type.
     *  The size of the assigned array must be smaller or equal than the target size.
     *  The nValidEntries value is set to the number of assiged values.
     *  It does not trigger the OnSetCallbackFunction.
     */
    virtual ProcessArray<T> & operator=(std::vector<T> const & other)=0;
 
//    virtual void set(ProcessArray<T> const & other)=0;
//    virtual void set(std::vector<T> const & other)=0;
//
//    /** Set this ProcessArray from another ProcessArray. Behaves like the according
//     *  assignment operator.
//     */
//    virtual void setWithoutCallback(ProcessArray<T> const & other)=0;
//
//    /** Set the ProcessArray from a std::vector. Behaves like the according
//     *  assignment operator.
//     */
//    virtual void setWithoutCallback(std::vector<T> const & other)=0;
  
    /** Random access operator without range check.
     */
    virtual T & operator[](size_t index)=0;

    /** Constant random access operator without range check.
     */
    virtual T const & operator[](size_t index) const =0;

    /** Random access to the element at a given index with range check.
     *  Throws std::out_of_range if index >= size.
     */
    virtual T & at(size_t index)=0;

    /** Constant random access to the element at a given index with range check.
     *  Throws std::out_of_range if index >= size.
     */
    virtual T const & at(size_t index) const =0;

    /** Return the size of the array.
     */
    virtual size_t size() const =0;

    /** Get access to the first element. Behaviour for an empty array is undefined.
     */
    virtual T & front()=0;

    /** Constant access to the first element. Behaviour for an empty array is undefined.
     */
    virtual T const & front() const =0;

     /** Get access to the last element. Behaviour for an empty array is undefined.
     */
    virtual T & back()=0;

     /** Constant access to the last element. Behaviour for an empty array is undefined.
     */
    virtual T const & back() const =0;

    /** Returns true if size is 0;
     */
    virtual bool empty() const =0;    

    /** Fill all elements of the the array with the same value.
     */
    virtual void fill(T const &)=0;
    
    /** Iterator to the first element.
     */
    virtual iterator begin()=0;
    
    /** Const_iterator to the first element for use in constant arrays.
     */
    virtual const_iterator begin() const =0;

     /** Explicitly request a constant iterator to the first element, even for non-const arrays.
     */
    virtual const_iterator cbegin() const =0;

    /** Iterator which points after the last element.
     */
    virtual iterator end()=0;

    /** Constant end iterator for const arrays.
     */
    virtual const_iterator end() const =0;

     /** Explicitly request a constant iterator to the end, even for non-const arrays.
     */
    virtual const_iterator cend() const =0;

    /** Returns a reverse iterator to the  last element.*/
    virtual reverse_iterator rbegin()=0;

    /** Returns a reverse iterator to the beginning */
    virtual const_reverse_iterator rbegin() const =0;
    
    /**  Explicitly request a  constant reverse iterator to the last element, even for non-const arrays. */
    virtual const_reverse_iterator crbegin() const =0;
   
    /** End iterator for reverse iteration.
     */
    virtual reverse_iterator rend()=0;

    /** Constant revertse end iterator constant arrays.
     */
   virtual const_reverse_iterator rend() const =0;

    /**Explicitly request a constant revertse end iterator, even for non-cost arrays.
     */
    virtual const_reverse_iterator crend() const =0;

    /** Every class with virtual functions should have a virtual destructor.
     */
    virtual ~ProcessArray(){};
  };
  
}// namespace mtca4u

#endif// MTCA4U_PROCESS_ARRAY_H
