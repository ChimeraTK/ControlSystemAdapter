#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H

#include <vector>
#include <utility>

#include <boost/smart_ptr.hpp>
#include <limits>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

//#include "ProcessVariable.h"
#include <mtca4u/NDRegisterAccessor.h>
#include "ProcessVariableListener.h"
#include "TimeStampSource.h"
#include "VersionNumberSource.h"

namespace ChimeraTK {
  /**
   * Array implementation of the ProcessVariable. This implementation is used
   * for all three use cases (sender, receiver, and stand-alone).
   *
   * This class is not thread-safe and should only be used from a single thread.
   *
   * If no version number source is specified when creating an instance of this
   * class, version number checks are disabled. This means that a receive
   * operation will always proceed, regardless of the version number of the new
   * value. The version number of the process array without a version number
   * source will always stay at zero.
   */
  template<class T>
    class ProcessArray: public mtca4u::NDRegisterAccessor<T> {

  public:

    /**
     * Type alias for a shared pointer to this type.
     */
    typedef boost::shared_ptr<ProcessArray> SharedPtr;

    /**
     * Type of the instance. This defines the behavior (send or receive
     * possible, modifications allowed, etc.). This enum type is private so
     * that only friend functions can construct instances of this class. The
     * constructors themselves have to be public so that boost::make_shared
     * can be used.
     */
    enum InstanceType {
      /**
       * Instance acts on is own.
       */
      STAND_ALONE,

      /**
       * Instance acts as the sender in a sender / receiver pair.
       */
      SENDER,

      /**
       * Instance acts as the receiver in a sender / receiver pair.
       */
      RECEIVER
    };

    /**
     * Creates a process array that works independently. This means that the
     * instance is not synchronized with any other instance and thus the send
     * and receive operations are not supported. However, all other operations
     * can be used like on any process variable.
     */
    ProcessArray(InstanceType instanceType, const std::string& name,
        const std::vector<T>& initialValue) :
            mtca4u::NDRegisterAccessor<T>(name),
	    _instanceType(instanceType), _vectorSize(initialValue.size()),
	    _swappable(true),
	    _buffers(boost::make_shared<std::vector<Buffer> >(1)),
	    _currentIndex(0), _lastSentIndex(0) {
      // It would be better to do the validation before initializing, but this
      // would mean that we would have to initialize twice.
      if (instanceType != STAND_ALONE) {
        throw std::invalid_argument(
            "This constructor may only be used for a stand-alone process scalar.");
      }
      // We have to initialize the buffer by creating a vector of the
      // appropriate size. We have to use swap because scoped_ptr does not
      // allow assignment (it is compatible with C++ 98 which does not have
      // move semantics).
      boost::scoped_ptr<std::vector<T> > v(new std::vector<T>(initialValue));
      (*_buffers)[0].value.swap(v);
    }

    /**
     * Creates a process array that acts as a receiver. A receiver is
     * intended to work as a tandem with a sender and receives values that
     * have been set to the sender.
     *
     * This constructor creates the buffers and queues that are needed for the
     * send/receive process and are shared with the sender.
     *
     * If the <code>swappable</code> option is true, the swap operation is
     * allowed. The <code>swappable</code> flag must be the same for the
     * sender and receiver.
     */
    ProcessArray(InstanceType instanceType, const std::string& name,
        const std::vector<T>& initialValue, std::size_t numberOfBuffers,
        bool swappable, VersionNumberSource::SharedPtr versionNumberSource) :
	mtca4u::NDRegisterAccessor<T>(name), _instanceType(instanceType), _vectorSize(
            initialValue.size()), _swappable(swappable), _buffers(
            boost::make_shared<std::vector<Buffer> >(numberOfBuffers + 2)), _fullBufferQueue(
            boost::make_shared<boost::lockfree::queue<std::size_t> >(
                numberOfBuffers)), _emptyBufferQueue(
            boost::make_shared<boost::lockfree::spsc_queue<std::size_t> >(
                numberOfBuffers)), _currentIndex(0), _lastSentIndex(0), _versionNumberSource(
            versionNumberSource) {
      // It would be better to do the validation before initializing, but this
      // would mean that we would have to initialize twice.
      if (instanceType != RECEIVER) {
        throw std::invalid_argument(
            "This constructor may only be used for a receiver process scalar.");
      }
      // We need at least two buffers for the queue (so four buffers in total)
      // in order to guarantee that we never have to block.
      if (numberOfBuffers < 2) {
        throw std::invalid_argument(
            "The number of buffers must be at least two.");
      }
      // We have to limit the number of buffers because we cannot allocate
      // more buffers than can be represented by size_t and the total number
      // of buffers is the specified number plus two. These extra two buffers
      // are needed because the sender and the receiver each hold one buffer
      // at all times and thus only the rest remains for the queue. We use
      // this definition of the number of buffers to keep it consistent with
      // what ProcessScalarImpl uses.
      if (numberOfBuffers > (std::numeric_limits<std::size_t>::max() - 2)) {
        throw std::invalid_argument("The number of buffers is too large.");
      }
      // We have to initialize the buffers by creating vectors of the
      // appropriate size. We have to use swap because scoped_ptr does not
      // allow assignment (it is compatible with C++ 98 which does not have
      // move semantics).
      for (typename std::vector<Buffer>::iterator i = _buffers->begin();
          i != _buffers->end(); ++i) {
        boost::scoped_ptr<std::vector<T> > v(new std::vector<T>(initialValue));
        i->value.swap(v);
      }
      // The buffer with the index 0 is assigned to the receiver and the
      // buffer with the index 1 is assigned to the sender. All buffers have
      // to be added to the empty-buffer queue.
      for (std::size_t i = 2; i < _buffers->size(); ++i) {
        _emptyBufferQueue->push(i);
      }
    }

    /**
     * Creates a process array that acts as a sender. A sender is intended
     * intended to work as a tandem with a receiver and send set values to
     * it. It uses the buffers and queues that have been created by the
     * receiver.
     *
     * If the <code>swappable</code> option is true, the swap operation is
     * allowed on the receiver. The <code>swappable</code> flag must be the
     * same for the sender and receiver. Allowing swapping has the side effect
     * that a value cannot be read after having been sent.
     *
     * The optional send-notification listener is notified every time the
     * sender's {@link ProcessArray::send()} method is called. It can be
     * used to queue a request for the receiver's
     * {@link ProcessArray::receive()} method to be called. The process
     * variable passed to the listener is the receiver and not the sender.
     */
    ProcessArray(InstanceType instanceType, bool swappable,
        TimeStampSource::SharedPtr timeStampSource,
        VersionNumberSource::SharedPtr versionNumberSource,
        ProcessVariableListener::SharedPtr sendNotificationListener,
        ProcessArray::SharedPtr receiver) :
	    mtca4u::NDRegisterAccessor<T>(receiver->getName()),
	    _instanceType(instanceType), _vectorSize(receiver->_vectorSize),
	    _swappable(swappable), _buffers(
            receiver->_buffers), _fullBufferQueue(receiver->_fullBufferQueue), _emptyBufferQueue(
            receiver->_emptyBufferQueue), _currentIndex(1), _lastSentIndex(1), _receiver(
            receiver), _timeStampSource(timeStampSource), _versionNumberSource(
            versionNumberSource), _sendNotificationListener(
            sendNotificationListener) {
      // It would be better to do the validation before initializing, but this
      // would mean that we would have to initialize twice.
      if (instanceType != SENDER) {
        throw std::invalid_argument(
            "This constructor may only be used for a sender process scalar.");
      }
      if (!receiver) {
        throw std::invalid_argument(
            "The pointer to the receiver must not be null.");
      }
      if (receiver->_instanceType != RECEIVER) {
        throw std::invalid_argument(
            "The pointer to the receiver must point to an instance that is actually a receiver.");
      }
    }

    /**
     * FIXME: I think we don't want this. The base class is no copy/assignable anyway.
     *
     * Assignment operator for another ProcessArray of the same template type.
     * It can be of a different implementation, though. The size of the assigned
     * array must be smaller than or equal to the target size.
     * This behaves excactly like the set(...) method.
     */
    ProcessArray<T> & operator=(ProcessArray<T> const & other) {
      set(other);
      return (*this);
    }

    /**
     * Assignment operator for a std::vector of the template data type.
     * The size of the assigned array must be smaller than or equal to the
     * target size.
     * This behaves exactly like the set(...) method.
     */
    ProcessArray<T> & operator=(std::vector<T> const & v) {
      set(v);
      return *this;
    }

    operator std::vector<T>() const {
      return getConst();
    }

    /**
     * Updates this process variable's value with the elements from the
     * specified vector. The vector's number of elements must match this process
     * variable's number of elements.
     *
     * If this instance of the process array must not be modified (because it is
     * a receiver and does not allow swapping), this method throws an exception.
     *
     * FIXME: The description says it is throwing, but the implementation does not
     * do it. Throwing will be obsolete in future because it is planned to have
     * receivers always swappable.
     */
    void set(std::vector<T> const & v) {
      get() = v;
    }

    /**
     * Updates this process variable's value with the other process variable's
     * value. The other process variable's number of elements must match this
     * process variable's number of elements.
     *
     * If this instance of the process array must not be modified (because it is
     * a receiver and does not allow swapping), this method throws an exception.
     *
     * FIXME: The description says it is throwing, but the implementation does not
     * do it. Throwing will be obsolete in future because it is planned to have
     * receivers always swappable.
     */
    void set(ProcessArray<T> const & other) {
      set(other.getConst());
    }

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
    void swap(boost::scoped_ptr<std::vector<T> > & otherVector) {
      if (!isSwappable()) {
        throw std::logic_error("Swap is not supported by this process array.");
      }
      if (otherVector->size() != get().size()) {
        throw std::runtime_error("Vector sizes do not match");
      }
      (*_buffers)[_currentIndex].value.swap(otherVector);
    }

    /**
     * Returns a reference to the vector that represents the current value of
     * this process array.
     *
     * If this instance of the process array must not be modified
     * (because it is a receiver and does not allow swapping), this
     * method throws an exception. In this case, you should use the
     * {@link #getConst()} method instead.
     *
     * The reference returned by this method becomes invalid when a receive,
     * send, or swap operation is performed on this process variable. Use of an
     * invalid reference results in undefined behavior.
     */
    std::vector<T> & get() {
      if (_instanceType == RECEIVER && !_swappable) {
        throw std::logic_error(
            "Attempt to modify a read-only process variable.");
      }
      return *(((*_buffers)[_currentIndex]).value);
    }

    /**
     * Returns a constant reference to the vector that represents the current
     * value of this process array. This version of the get method is used if
     * this process array is const.
     *
     * The reference returned by this method becomes invalid when a receive,
     * send, or swap operation is performed on this process variable. Use of an
     * invalid reference results in undefined behavior.
     */
    std::vector<T> const & get() const {
      return getConst();
    }

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
    std::vector<T> const & getConst() const {
      return *(((*_buffers)[_currentIndex]).value);
    }

    /**
     * @deprecated
     * It is planned that receivers are always swappable.
     *
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
    std::vector<T> const & getLastSent() const {
      if (_instanceType != SENDER || _swappable) {
        throw std::logic_error(
            "The last sent value is only available for a sender that is associated with a receiver that does not allow swapping.");
      }
      return *(((*_buffers)[_lastSentIndex]).value);
    }

    virtual bool isReadable() const {
      return _instanceType == RECEIVER;
    }

    virtual bool isWriteable() const {
      return _instanceType == SENDER;
    }

    virtual bool isReadOnly() const {
      return !isWriteable();
    }
  
    TimeStamp getTimeStamp() const {
      return ((*_buffers)[_currentIndex]).timeStamp;
    }

    virtual void read(){
      throw std::logic_error("Blocking read is not supported by process array.");
    }

    /**
     * Returns the version number that is associated with the current value.
     * This is the version number that was received with the {@link receive()}
     * operation that received the respective value. Typically, this will be the
     * last receive operation. If this process variable has just been sent, the
     * value and thus the version number is undefined (it can be any value /
     * version-number combination that has been sent at an earlier point in
     * time).
     *
     * The version number is used to resolve conflicting updates. When an update
     * is received using the {@link receive()} method, it is only used if its
     * value has a version number that is greater than the version number of the
     * current value. Initially, each process variable has a version number of
     * zero.
     *
     * When this process variable has not been initialized with a version number
     * source, its version number always stays at zero and the version-number
     * logic is disabled.
     */
    VersionNumber getVersionNumber() const {
      // On the other hand, this should not matter too much because the current
      // value will be in an undefined state anyway and thus we might not care.
      return ((*_buffers)[_currentIndex]).versionNumber;
    }

    /**
     * Returns the version number associated with the value that has been sent
     * with the last sent operation.
     *
     * This method may only be used on a process variable that is a sender and
     * if swapping is not allowed for the respective receiver. In all other
     * cases, calling this method results in an exception.
     *
     * @deprecated
     * This method is going to be removed together with the
     * {@link getLastSent()} and {@link isSwappable()} methods.
     */
    VersionNumber getLastSentVersionNumber() const {
      // TODO Typically, there should be no reason to check the version number
      // of the last sent value, but we still have this method for symmetry
      // reasons (because we also have a getLastSent() method).
      // We should really consider getting rid of both methods and allow for an
      // (optional) copy when sending instead of disabling swapping.
      if (_instanceType != SENDER || _swappable) {
        throw std::logic_error(
            "The last sent version number is only available for a sender that is associated with a receiver that does not allow swapping.");
      }
      return ((*_buffers)[_lastSentIndex]).versionNumber;
    }

    /**
     * Tells whether this array supports swapping. If <code>true</code>, the
     * <code>swap</code> method can be used to swap the vector backing this
     * array with a different one, thus avoiding copying during synchronization.
     * If <code>false</code>, calling <code>swap</code> results in an exception.
     * Swapping (and other modifications) are always supported on the sender
     * side but might not be supported on the receiver side.
     */
    bool isSwappable() const {
      // Senders and stand-alone instances are always swappable. For
      // receivers, it depends on the swappable flag.
      if (_instanceType == RECEIVER) {
        return _swappable;
      } else {
        return true;
      }
    }

    bool readNonBlocking() {
      if (_instanceType != RECEIVER) {
        throw std::logic_error(
            "Receive operation is only allowed for a receiver process variable.");
      }
      // We have to check that the vector that we currently own still has the
      // right size. Otherwise, the code using the sender might get into
      // trouble when it suddenly experiences a vector of the wrong size.
      if ((*_buffers)[_currentIndex].value->size() != _vectorSize) {
        throw std::runtime_error(
            "Cannot run receive operation because the size of the vector belonging to the current buffer has been modified.");
      }
      std::size_t nextIndex;
      if (_fullBufferQueue->pop(nextIndex)) {
        // We only use the incoming update if it has a higher version number
        // than the current value. This check is disabled when there is no
        // version number source.
        if (!_versionNumberSource
            || ((*_buffers)[nextIndex]).versionNumber > getVersionNumber()) {
          _emptyBufferQueue->push(_currentIndex);
          _currentIndex = nextIndex;
          return true;
        } else {
          _emptyBufferQueue->push(nextIndex);
          return false;
        }
      } else {
        return false;
      }
    }

    /**
     * Sends the current value to the receiver. Returns <code>true</code> if an
     * empty buffer was available and <code>false</code> if no empty buffer was
     * available and thus a previously sent value has been dropped in order to
     * send the current value.
     *
     * If this process variable has a version-number source, a new version
     * number is retrieved from this source and used for the value being sent to
     * the receiver. Otherwise, a version number of zero is used.
     *
     * Throws an exception if this process variable is not a sender.
     */
    void write() {
      VersionNumber newVersionNumber;
      if (_versionNumberSource) {
        newVersionNumber = _versionNumberSource->nextVersionNumber();
      } else {
        newVersionNumber = 0;
      }
      write(newVersionNumber);
    }

    /**
     * Sends the current value to the receiver. Returns <code>true</code> if an
     * empty buffer was available and <code>false</code> if no empty buffer was
     * available and thus a previously sent value has been dropped in order to
     * send the current value.
     *
     * The specified version number is passed to the receiver. If the receiver
     * has a value with a version number greater than or equal to the specified
     * version number, it silently discards this update.
     *
     * Throws an exception if this process variable is not a sender.
     */
    void write(VersionNumber newVersionNumber) {
      if (_instanceType != SENDER) {
        throw std::logic_error(
            "Send operation is only allowed for a sender process variable.");
      }
      // We have to check that the vector that we currently own still has the
      // right size. Otherwise, the code using the receiver might get into
      // trouble when it suddenly experiences a vector of the wrong size.
      if ((*_buffers)[_currentIndex].value->size() != _vectorSize) {
        throw std::runtime_error(
            "Cannot run receive operation because the size of the vector belonging to the current buffer has been modified.");
      }
      // Before sending the value, we have to update the associated
      // time-stamp.
      ((*_buffers)[_currentIndex]).timeStamp =
          _timeStampSource ?
              _timeStampSource->getCurrentTimeStamp() :
              TimeStamp::currentTime();
      ((*_buffers)[_currentIndex]).versionNumber = newVersionNumber;
      std::size_t nextIndex;
      if (_emptyBufferQueue->pop(nextIndex)) {
        _fullBufferQueue->push(_currentIndex);
      } else {
        // We can still send, but we will lose older data.
        if (_fullBufferQueue->pop(nextIndex)) {
          _fullBufferQueue->push(_currentIndex);
        } else {
          // It is possible, that we did not find an empty buffer but before
          // we could get a full buffer, the receiver processed all full
          // buffers. In this case, the queue of empty buffers cannot be empty
          // any longer because we have at least four buffers.
          if (!_emptyBufferQueue->pop(nextIndex)) {
            // This should never happen and is just an assertion for extra
            // safety.
            throw std::runtime_error(
                "Assertion that empty-buffer queue has at least one element failed.");
          }
          _fullBufferQueue->push(_currentIndex);
        }
      }
      _lastSentIndex = _currentIndex;
      _currentIndex = nextIndex;
      if (_sendNotificationListener) {
        _sendNotificationListener->notify(_receiver);
      }
    }

    const std::type_info& getValueType() const {
      return typeid(T);
    }

    bool isArray() const {
      return true;
    }

    virtual bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const{
      // only true if the very instance of the transfer element is the same
      return e.get() == this;
    }

    virtual std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements(){
      return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() };
    }
    
    virtual void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>){
      // You can't replace anything here. Just do nothing.
    }
  private:

    /**
     * Type for the individual buffers. Each buffer stores a vector (wrapped
     * in a Boost scoped pointer) and a time stamp.
     */
    struct Buffer {

      TimeStamp timeStamp;
      boost::scoped_ptr<std::vector<T> > value;
      VersionNumber versionNumber;

      /**
       * Default constructor. Has to be defined explicitly because we have an
       * non-default copy constructor.
       */
      Buffer() :
          versionNumber(0) {
      }

      /**
       * Copy constructor. Some STL implementations need the copy constructor
       * while initializing the vector that stores the buffers: They do not
       * default construct each element but only default construct the first
       * element and then copy it. In this case, this copy constructor will
       * work perfectly fine. It would even work if the value pointer was
       * already initialized with a vector, however we might have a serious
       * performance problem if that happened. Therefore, we use an assertion
       * to check that the copy constructor is never used once the buffer has
       * been initialized with a vector.
       */
      Buffer(Buffer const & other) :
          timeStamp(other.timeStamp), value(
              other.value ? new std::vector<T>(*(other.value)) : 0), versionNumber(
              other.versionNumber) {
        assert(!(other.value));
      }

    };

    /**
     * Type this instance is representing.
     */
    InstanceType _instanceType;

    /**
     * Number of elements that each vector (and thus this array) has.
     */
    std::size_t _vectorSize;

    /**
     * Flag indicating the swapping behavior. For a receiver, it indicates
     * whether swapping is allowed. For a sender, it indicates whether
     * swapping is allowed on the corresponding receiver. For a stand-alone
     * instance, it does not have any meaning.
     */
    bool _swappable;

    /**
     * Buffers that hold the actual values.
     */
    boost::shared_ptr<std::vector<Buffer> > _buffers;

    /**
     * Queue holding the indices of the full buffers. Those are the buffers
     * that have been sent but not yet received. We do not use an spsc_queue
     * for this queue, because might want to take elements from the sending
     * thread, so there are two threads which might consume elements and thus
     * an spsc_queue is not safe.
     */
    boost::shared_ptr<boost::lockfree::queue<std::size_t> > _fullBufferQueue;

    /**
     * Queue holding the empty buffers. Those are the buffers that have been
     * returned to the sender by the receiver.
     */
    boost::shared_ptr<boost::lockfree::spsc_queue<std::size_t> > _emptyBufferQueue;

    /**
     * Index into the _buffers array that is currently owned by this instance
     * and used for read and (possibly) write operations.
     */
    std::size_t _currentIndex;

    /**
     * Index of the buffer that has been sent last. This index is only
     * meaningful if this is a sender and swapping is not allowed on the
     * receiver.
     */
    std::size_t _lastSentIndex;

    /**
     * Pointer to the receiver associated with this sender. This field is only
     * used if this process variable represents a sender.
     */
    boost::shared_ptr<ProcessArray> _receiver;

    /**
     * Time-stamp source used to update the time-stamp when sending a value.
     */
    boost::shared_ptr<TimeStampSource> _timeStampSource;

    /**
     * Version number source used for getting the next version number when one
     * of the {@code set} methods is called without specifying a version number.
     */
    boost::shared_ptr<VersionNumberSource> _versionNumberSource;

    /**
     * Listener that is notified when the process variable is sent.
     */
    boost::shared_ptr<ProcessVariableListener> _sendNotificationListener;

  };

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
      TimeStampSource::SharedPtr timeStampSource = TimeStampSource::SharedPtr(),
      VersionNumberSource::SharedPtr versionNumberSource =
          VersionNumberSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener =
          ProcessVariableListener::SharedPtr());

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
      TimeStampSource::SharedPtr timeStampSource = TimeStampSource::SharedPtr(),
      VersionNumberSource::SharedPtr versionNumberSource =
          VersionNumberSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener =
          ProcessVariableListener::SharedPtr());

  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(std::size_t size,
      const std::string & name, T initialValue) {
    return boost::make_shared<ProcessArray<T> >(ProcessArray<T>::STAND_ALONE,
        name, std::vector<T>(size, initialValue));
  }

  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(
      const std::vector<T>& initialValue, const std::string & name) {
    return boost::make_shared<ProcessArray<T> >(ProcessArray<T>::STAND_ALONE,
        name, initialValue);
  }

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const std::string & name, T initialValue,
      bool swappable, std::size_t numberOfBuffers,
      TimeStampSource::SharedPtr timeStampSource,
      VersionNumberSource::SharedPtr versionNumberSource,
      ProcessVariableListener::SharedPtr sendNotificationListener) {
    typename boost::shared_ptr<ProcessArray<T> > receiver = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::RECEIVER, name,
        std::vector<T>(size, initialValue), numberOfBuffers, swappable,
        versionNumberSource);
    typename ProcessArray<T>::SharedPtr sender = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::SENDER, swappable, timeStampSource,
        versionNumberSource, sendNotificationListener, receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      const std::vector<T>& initialValue, const std::string & name,
      bool swappable, std::size_t numberOfBuffers,
      TimeStampSource::SharedPtr timeStampSource,
      VersionNumberSource::SharedPtr versionNumberSource,
      ProcessVariableListener::SharedPtr sendNotificationListener) {
    typename boost::shared_ptr<ProcessArray<T> > receiver = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::RECEIVER, name, initialValue,
        numberOfBuffers, swappable, versionNumberSource);
    typename ProcessArray<T>::SharedPtr sender = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::SENDER, swappable, timeStampSource,
        versionNumberSource, sendNotificationListener, receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
