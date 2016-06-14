#ifndef MTCA4U_PROCESS_ARRAY_H
#define MTCA4U_PROCESS_ARRAY_H

#include <vector>
#include <utility>

#include <boost/smart_ptr.hpp>

#include "ProcessVariable.h"
#include "ProcessVariableListener.h"

namespace mtca4u {
  /**
   * Interface implemented by all process arrays.
   *
   * Instances implementing this interface are typically not thread-safe and
   * should only be used from a single thread.
   */
  template<class T>
  class ProcessArrayHowItLookedLike: public ProcessVariable {
  public:
    /**
     * Type alias for a shared pointer to this type.
     */
    typedef boost::shared_ptr<ProcessArrayHowItLookedLike> SharedPtr;

    /**
     * Assignment operator for another ProcessArray of the same template type.
     * It can be of a different implementation, though. The size of the assigned
     * array must be smaller than or equal to the target size.
     */
    // we don't want this behavious anyway.
    //ProcessArray<T> & operator=(ProcessArray<T> const & other) {
    //  this->set(other);
    //  return *this;
    //}

    /**
     * Assignment operator for a std::vector of the template data type.
     * The size of the assigned array must be smaller than or equal to the
     * target size.
     */
//    ProcessArray<T> & operator=(std::vector<T> const & other) {
//      this->set(other);
//      return *this;
//    }

    /**
     * Returns a reference to the vector that represents the current value of
     * this process array.
     *
     * If this instance of the process array must not be modified (because it is a receiver and does not allow swapping), this
     * method throws an exception. In this case, you should use the
     * {@link #getConst()} method instead.
     *
     * The reference returned by this method becomes invalid when a receive,
     * send, or swap operation is performed on this process variable. Use of an
     * invalid reference results in undefined behavior.
     */
    virtual std::vector<T> & get() =0;

    /**
     * Returns a constant reference to the vector that represents the current
     * value of this process array. This version of the get method is used if
     * this process array is const.
     *
     * The reference returned by this method becomes invalid when a receive,
     * send, or swap operation is performed on this process variable. Use of an
     * invalid reference results in undefined behavior.
     */
    virtual std::vector<T> const & get() const =0;

    /**
     * Returns a constant reference to the the vector that represents the
     * current value of this process array. This method should be preferred for
     * receiver process variables because it works even if the process array
     * does not allow swapping and thus the {@link #get()} method cannot be
     * used.
     *
     * The reference returned by this method becomes invalid when a receive,
     * send, or swap operation is performed on this process variable. Use of an
     * invalid reference results in undefined behavior.
     */
    virtual std::vector<T> const & getConst() const =0;

    /**
     * Returns a constant reference to the vector that represents the value that
     * has been sent with the last sent operation.
     *
     * This method may only be used on a process variable that is a sender and
     * if swapping is not allowed for the respective receiver. In all other
     * cases, calling this method results in an exception.
     *
     * While the reference returned by this method stays valid even after a
     * receive, send, or swap operation is performed on this process variable,
     * it is recommended to not retain this reference because it might point
     * to a different vector than expected.
     */
    virtual std::vector<T> const & getLastSent() const =0;

    /**
     * Updates this process variable's value with the other process variable's
     * value. The other process variable's number of elements must match this
     * process variable's number of elements.
     *
     * If this instance of the process array must not be modified (because it is
     * a receiver and does not allow swapping), this method throws an exception.
     */
    //    virtual void set(ProcessArray<T> const & other) =0;

    /**
     * Updates this process variable's value with the elements from the
     * specified vector. The vector's number of elements must match this process
     * variable's number of elements.
     *
     * If this instance of the process array must not be modified (because it is
     * a receiver and does not allow swapping), this method throws an exception.
     */
    virtual void set(std::vector<T> const & other) =0;

    const std::type_info& getValueType() const {
      return typeid(T);
    }

    bool isArray() const {
      return true;
    }

    /**
     * Tells whether this array supports swapping. If <code>true</code>, the
     * <code>swap</code> method can be used to swap the vector backing this
     * array with a different one, thus avoiding copying during synchronization.
     * If <code>false</code>, calling <code>swap</code> results in an exception.
     * Swapping (and other modifications) are always supported on the sender
     * side but might not be supported on the receiver side.
     */
    virtual bool isSwappable() const =0;

    /**
     * Swaps the vector backing this process array with a different vector.
     * This method may only be called by the peer control-system process array
     * when synchronizing with this process array and only if
     * <code>isSwappable()</code> returns <code>true</code>. Otherwise, this
     * method throws an <code>std::logic_error</code>.
     *
     * The <code>boost::scoped_ptr</code> passed must not be <code>null</code>
     * and must point to a vector that has the same size as the vector it is
     * swapped with.
     */
    virtual void swap(boost::scoped_ptr<std::vector<T> >& otherVector) =0;

  protected:
    /**
     * Creates a process array with the specified name.
     */
//    ProcessArray(const std::string& name = std::string()) :
//        ProcessVariable(name) {
//    }
//
//    /**
//     * Protected destructor. Instances should not be destroyed through
//     * pointers to this base type.
//     */
//    virtual ~ProcessArray() {
//    }

  };
}

// ProcessArrayImpl.h must be included after the class definition and the
// template function declaration, because it depends on it.
#include "ProcessArrayImpl.h"

namespace mtca4u {

  // temporary typedef
  template<class T>
  using ProcessArray = impl::ProcessArrayImpl<T>;

  /**
   * Creates a simple process array. A simple process array just works on its
   * own and does not implement a synchronization mechanism. Apart from this,
   * it is fully functional, so that you get, set, and swap values.
   *
   * The specified initial value is used for all the elements of the array.
   */
  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(std::size_t size,
      const std::string & name = "", T initialValue = 0);

  /**
   * Creates a simple process array. A simple process array just works on its
   * own and does not implement a synchronization mechanism. Apart from this,
   * it is fully functional, so that you get, set, and swap values.
   *
   * The array's size is set to the number of elements stored in the vector
   * provided for initialization and all elements are initialized with the
   * values provided by this vector.
   */
  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(
      const std::vector<T>& initialValue, const std::string & name = "");

  /**
   * Creates a synchronized process array. A synchronized process array works
   * as a pair of two process arrays, where one process array acts as a sender
   * and the other one acts as a receiver.
   *
   * The sender allows full read-write access. Changes that have been made to
   * the sender can be sent to the receiver through the
   * {@link ProcessArray::send()} method. The receiver can be updated with these
   * changes by calling its {@link ProcessArray::receive()} method.
   *
   * The synchronization is implemented in a thread-safe manner, so that the
   * sender and the receiver can safely be used by different threads without
   * a mutex. However, both the sender and receiver each only support a single
   * thread. This means that the sender or the receiver have to be protected
   * with a mutex if more than one thread wants to access either of them.
   *
   * If the <code>swappable</code> flag is <code>true</code> (the default), the
   * receiver supports full read-write access (including swapping). If it is
   * <code>false</code>, only read-only access is supported. On the other hand,
   * setting this flag to <code>false</code> has the advantage that the sender's
   * {@link ProcessArray::getLastSent()} method can be used.
   *
   * The number of buffers specifies how many buffers are allocated for the
   * send / receive mechanism. The minimum number (and default) is two. This
   * number specifies, how many times {@link ProcessArray::send()} can be called
   * in a row without losing data when {@link ProcessArray::receive()} is not
   * called in between.
   *
   * The specified time-stamp source is used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used.
   *
   * The optional send-notification listener is notified every time the sender's
   * {@link ProcessArray::send()} method is called. It can be used to queue a
   * request for the receiver's {@link ProcessArray::receive()} method to be
   * called.  The process variable passed to the listener is the receiver and
   * not the sender.
   *
   * The specified initial value is used for all the elements of the array.
   */
  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const std::string & name = "", T initialValue = 0,
      bool swappable = true, std::size_t numberOfBuffers = 2,
      boost::shared_ptr<TimeStampSource> timeStampSource = boost::shared_ptr<
          TimeStampSource>(),
      boost::shared_ptr<ProcessVariableListener> sendNotificationListener =
          boost::shared_ptr<ProcessVariableListener>());

  /**
   * Creates a synchronized process array. A synchronized process array works
   * as a pair of two process arrays, where one process array acts as a sender
   * and the other one acts as a receiver.
   *
   * The sender allows full read-write access. Changes that have been made to
   * the sender can be sent to the receiver through the
   * {@link ProcessArray::send()} method. The receiver can be updated with these
   * changes by calling its {@link ProcessArray::receive()} method.
   *
   * The synchronization is implemented in a thread-safe manner, so that the
   * sender and the receiver can safely be used by different threads without
   * a mutex. However, both the sender and receiver each only support a single
   * thread. This means that the sender or the receiver have to be protected
   * with a mutex if more than one thread wants to access either of them.
   *
   * If the <code>swappable</code> flag is <code>true</code> (the default), the
   * receiver supports full read-write access (including swapping). If it is
   * <code>false</code>, only read-only access is supported. On the other hand,
   * setting this flag to <code>false</code> has the advantage that the sender's
   * {@link ProcessArray::getLastSent()} method can be used.
   *
   * The number of buffers specifies how many buffers are allocated for the
   * send / receive mechanism. The minimum number (and default) is two. This
   * number specifies, how many times {@link ProcessArray::send()} can be called
   * in a row without losing data when {@link ProcessArray::receive()} is not
   * called in between.
   *
   * The specified time-stamp source is used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used.
   *
   * The optional send-notification listener is notified every time the sender's
   * {@link ProcessArray::send()} method is called. It can be used to queue a
   * request for the receiver's {@link ProcessArray::receive()} method to be
   * called.  The process variable passed to the listener is the receiver and
   * not the sender.
   *
   * The array's size is set to the number of elements stored in the vector
   * provided for initialization and all elements are initialized with the
   * values provided by this vector.
   */
  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      const std::vector<T>& initialValue, const std::string & name = "",
      bool swappable = true, std::size_t numberOfBuffers = 2,
      boost::shared_ptr<TimeStampSource> timeStampSource = boost::shared_ptr<
          TimeStampSource>(),
      boost::shared_ptr<ProcessVariableListener> sendNotificationListener =
          boost::shared_ptr<ProcessVariableListener>());

} // namespace mtca4u

namespace mtca4u {

  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(std::size_t size,
      const std::string & name, T initialValue) {
    return boost::make_shared<typename impl::ProcessArrayImpl<T> >(
        impl::ProcessArrayImpl<T>::STAND_ALONE, name,
        std::vector<T>(size, initialValue));
  }

  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(
      const std::vector<T>& initialValue, const std::string & name) {
    return boost::make_shared<typename impl::ProcessArrayImpl<T> >(
        impl::ProcessArrayImpl<T>::STAND_ALONE, name, initialValue);
  }

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const std::string & name, T initialValue,
      bool swappable, std::size_t numberOfBuffers,
      boost::shared_ptr<TimeStampSource> timeStampSource,
      boost::shared_ptr<ProcessVariableListener> sendNotificationListener) {
    typename boost::shared_ptr<typename impl::ProcessArrayImpl<T> > receiver =
        boost::make_shared<typename impl::ProcessArrayImpl<T> >(
            impl::ProcessArrayImpl<T>::RECEIVER, name,
            std::vector<T>(size, initialValue), numberOfBuffers, swappable);
    typename ProcessArray<T>::SharedPtr sender = boost::make_shared<
        typename impl::ProcessArrayImpl<T> >(impl::ProcessArrayImpl<T>::SENDER,
        swappable, timeStampSource, sendNotificationListener, receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      const std::vector<T>& initialValue, const std::string & name,
      bool swappable, std::size_t numberOfBuffers,
      boost::shared_ptr<TimeStampSource> timeStampSource,
      boost::shared_ptr<ProcessVariableListener> sendNotificationListener) {
    typename boost::shared_ptr<typename impl::ProcessArrayImpl<T> > receiver =
        boost::make_shared<typename impl::ProcessArrayImpl<T> >(
            impl::ProcessArrayImpl<T>::RECEIVER, name, initialValue,
            numberOfBuffers, swappable);
    typename ProcessArray<T>::SharedPtr sender = boost::make_shared<
        typename impl::ProcessArrayImpl<T> >(impl::ProcessArrayImpl<T>::SENDER,
        swappable, timeStampSource, sendNotificationListener, receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

} // namespace mtca4u

#endif // MTCA4U_PROCESS_ARRAY_H
