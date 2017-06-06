#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H

#include <vector>
#include <utility>
#include <limits>
#include <stdexcept>
#include <typeinfo>

#include <boost/smart_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/thread/future.hpp>

#include <mtca4u/NDRegisterAccessor.h>

#include "ProcessVariableListener.h"
#include "TimeStampSource.h"
#include "VersionNumberSource.h"
#include "PersistentDataStorage.h"

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
  class ProcessArray : public mtca4u::NDRegisterAccessor<T> {

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
     * 
     * TODO @todo Is this really a wanted feature? I see no benefit in it...
     */
    ProcessArray(InstanceType instanceType, const std::string &name, const std::string &unit,
        const std::string &description, const std::vector<T> &initialValue)
    : mtca4u::NDRegisterAccessor<T>(name, unit, description),
      _instanceType(instanceType),
      _vectorSize(initialValue.size()),
      _maySendDestructively(true),
      _sharedState(boost::make_shared<SharedState>(1)),
      _currentIndex(0)
    {
      // It would be better to do the validation before initializing, but this
      // would mean that we would have to initialize twice.
      if (instanceType != STAND_ALONE) {
        throw std::invalid_argument(
            "This constructor may only be used for a stand-alone process scalar.");
      }
      // allocate and initialise buffer of the base class
      mtca4u::NDRegisterAccessor<T>::buffer_2D.resize(1);
      mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = initialValue;
    }

    /**
     * Creates a process array that acts as a receiver. A receiver is
     * intended to work as a tandem with a sender and receives values that
     * have been set to the sender.
     *
     * This constructor creates the buffers and queues that are needed for the
     * send/receive process and are shared with the sender.
     */
    ProcessArray(InstanceType instanceType, const std::string& name, const std::string &unit,
        const std::string &description, const std::vector<T>& initialValue, std::size_t numberOfBuffers,
        VersionNumberSource::SharedPtr versionNumberSource)
    : mtca4u::NDRegisterAccessor<T>(name, unit, description),
      _instanceType(instanceType),
      _vectorSize(initialValue.size()),
      _maySendDestructively(false),
      _sharedState(boost::make_shared<SharedState>(numberOfBuffers)),
      _currentIndex(0),
      _versionNumberSource(versionNumberSource)
    {
      // allocate and initialise buffer of the base class
      mtca4u::NDRegisterAccessor<T>::buffer_2D.resize(1);
      mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = initialValue;
      // It would be better to do the validation before initializing, but this
      // would mean that we would have to initialize twice.
      if (instanceType != RECEIVER) {
        throw std::invalid_argument("This constructor may only be used for a receiver process scalar.");
      }
      // We need at least two buffers for the queue (so four buffers in total)
      // in order to guarantee that we never have to block.
      if (numberOfBuffers < 2) {
        throw std::invalid_argument("The number of buffers must be at least two.");
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
      // We have to initialize the buffers by copying in the initial vectors.
      for (auto &i : _sharedState->_buffers) {
        i.value = initialValue;
      }
      // The buffer with the index 0 is assigned to the receiver and the
      // buffer with the index 1 is assigned to the sender. All buffers have
      // to be added to the empty-buffer queue.
      for (std::size_t i = 2; i < _sharedState->_buffers.size(); ++i) {
        _sharedState->_emptyBufferQueue.push(i);
      }
    }

    /**
     * Creates a process array that acts as a sender. A sender is intended
     * intended to work as a tandem with a receiver and send set values to
     * it. It uses the buffers and queues that have been created by the
     * receiver.
     *
     * If the <code>maySendDestructively</code> flag is <code>true</code>, the
     * {@link ProcessArray::writeDestructively()} method may be used to transfer values without
     * copying but losing them on the sender side.
     *
     * The optional send-notification listener is notified every time the
     * sender's {@link ProcessArray::write()} method is called. It can be
     * used to queue a request for the receiver's
     * readNonBlocking() method to be called. The process
     * variable passed to the listener is the receiver and not the sender.
     */
    ProcessArray(InstanceType instanceType, bool maySendDestructively,
        TimeStampSource::SharedPtr timeStampSource,
        VersionNumberSource::SharedPtr versionNumberSource,
        ProcessVariableListener::SharedPtr sendNotificationListener,
        ProcessArray::SharedPtr receiver)
    : mtca4u::NDRegisterAccessor<T>(receiver->getName(), receiver->getUnit(), receiver->getDescription()),
      _instanceType(instanceType),
      _vectorSize(receiver->_vectorSize),
      _maySendDestructively(maySendDestructively),
      _sharedState(receiver->_sharedState),
      _currentIndex(1),
      _receiver(receiver),
      _timeStampSource(timeStampSource),
      _versionNumberSource(versionNumberSource),
      _sendNotificationListener(sendNotificationListener)
    {
      // It would be better to do the validation before initializing, but this
      // would mean that we would have to initialize twice.
      if (instanceType != SENDER) {
        throw std::invalid_argument("This constructor may only be used for a sender process scalar.");
      }
      if (!receiver) {
        throw std::invalid_argument("The pointer to the receiver must not be null.");
      }
      if (receiver->_instanceType != RECEIVER) {
        throw std::invalid_argument(
            "The pointer to the receiver must point to an instance that is actually a receiver.");
      }
      // allocate and initialise buffer of the base class
      mtca4u::NDRegisterAccessor<T>::buffer_2D.resize(1);
      mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = receiver->buffer_2D[0];
      // put future into the notification queue
      _sharedState->_notificationQueue.push(_sharedState->_notificationPromise.get_future());
    }

    ~ProcessArray() {
      // if this is a receiver, shutdown a potentially running readAsync
      if(_instanceType == RECEIVER) {
        if(this->readAsyncThread.joinable()) {
          this->readAsyncThread.interrupt();
          try {
            _sharedState->_notificationPromise.set_value();
          }
          catch(boost::promise_already_satisfied) {
            // ignore -> already notified
          }
          this->readAsyncThread.join();
        }
      }
    }

    /**
     * FIXME: I think we don't want this. The base class is no copy/assignable anyway.
     *
     * Assignment operator for another ProcessArray of the same template type.
     * It can be of a different implementation, though. The size of the assigned
     * array must be smaller than or equal to the target size.
     * This behaves excactly like the set(...) method.
     * 
     * @todo THIS FUNCTION IS DEPRECTED
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
     * 
     * @todo THIS FUNCTION IS DEPRECTED, use ScalarRegisterAccessor::operator=() instead 
     */
    ProcessArray<T> & operator=(std::vector<T> const & v) {
      set(v);
      return *this;
    }

    /*
     * @todo THIS FUNCTION IS DEPRECTED
     */
    operator std::vector<T>() const {
      return get();
    }

    /**
     * Updates this process variable's value with the elements from the
     * specified vector. The vector's number of elements must match this process
     * variable's number of elements.
     * 
     * @todo THIS FUNCTION IS DEPRECTED, use accessData() instead
     */
    void set(std::vector<T> const & v) {
      get() = v;
    }

    /**
     * Updates this process variable's value with the other process variable's
     * value. The other process variable's number of elements must match this
     * process variable's number of elements.
     * 
     * @todo THIS FUNCTION IS DEPRECTED, use accessData() instead
     */
    void set(ProcessArray<T> const & other) {
      set(other.get());
    }

    /**
     * Swaps the vector backing this process array with a different vector.
     *
     * The <code>boost::scoped_ptr</code> passed must not be <code>null</code>
     * and must point to a vector that has the same size as the vector it is
     * swapped with.
     * 
     * @todo THIS FUNCTION IS DEPRECTED, use accessData()[0].swap() instead
     */
    void swap(std::vector<T> & otherVector) {
      if (otherVector.size() != get().size()) {
        throw std::runtime_error("Vector sizes do not match");
      }
      get().swap(otherVector);
    }

    /**
     * Returns a reference to the vector that represents the current value of
     * this process array.
     *
     * The reference returned by this method becomes invalid when a receive,
     * send, or swap operation is performed on this process variable. Use of an
     * invalid reference results in undefined behavior.
     * 
     * @todo THIS FUNCTION IS DEPRECTED, use accessData() instead
     */
    std::vector<T> & get() {
      return mtca4u::NDRegisterAccessor<T>::buffer_2D[0];
    }

    /**
     * Returns a constant reference to the vector that represents the current
     * value of this process array. This version of the get method is used if
     * this process array is const.
     *
     * The reference returned by this method becomes invalid when a receive,
     * send, or swap operation is performed on this process variable. Use of an
     * invalid reference results in undefined behavior.
     * 
     * @todo THIS FUNCTION IS DEPRECTED, use accessData() instead
     */
    std::vector<T> const & get() const {
      return mtca4u::NDRegisterAccessor<T>::buffer_2D[0];
    }

    bool isReadable() const override {
      return _instanceType == RECEIVER;
    }

    bool isWriteable() const override {
      return _instanceType == SENDER;
    }

    bool isReadOnly() const override {
      return !isWriteable();
    }
  
    TimeStamp getTimeStamp() const override {
      return (_sharedState->_buffers[_currentIndex]).timeStamp;
    }

    /**
     * Returns the version number that is associated with the current value.
     * This is the version number that was received with the readNonBlocking()
     * operation that received the respective value or the last {@link write()}
     * operation. If the last send operation was destructive, the version number
     * (like the current value) is undefined.
     *
     * The version number is used to resolve conflicting updates. When an update
     * is received using the readNonBlocking() method, it is only used if its
     * value has a version number that is greater than the version number of the
     * current value. Initially, each process variable has a version number of
     * zero.
     *
     * When this process variable has not been initialized with a version number
     * source, its version number always stays at zero and the version-number
     * logic is disabled.
     */
    VersionNumber getVersionNumber() const { /// @todo FIXME this function must be present in TransferElement already!
      // On the other hand, this should not matter too much because the current
      // value will be in an undefined state anyway and thus we might not care.
      return (_sharedState->_buffers[_currentIndex]).versionNumber;
    }

    void doReadTransfer() override {
      // If previously a TransferFuture has been requested by readAsync(), first make sure that that future is
      // already fulfilled. Otherwise we might run out of futures on the notification queue.
      if(mtca4u::TransferElement::hasActiveFuture) mtca4u::TransferElement::activeFuture.wait();
      // Obtain futures from the notification queue and wait on them until we receive data. We start with checking
      // for data before obtaining a future from the notification queue, since this is faster and the notification
      // queue is shorter than the data queue.
      while(!doReadTransferNonBlocking()) {
        boost::shared_future<void> future;
        bool atLeastOneFuturePresent = _sharedState->_notificationQueue.pop(future);
        assert(atLeastOneFuturePresent);  // if this is not true, there is something wrong with the algorithm
        (void)atLeastOneFuturePresent;    // prevent warning in case asserts are disabled
        future.wait();
        boost::this_thread::interruption_point();
      }
    }

    bool doReadTransferNonBlocking() override {
      if (_instanceType != RECEIVER) {
        throw std::logic_error("Receive operation is only allowed for a receiver process variable.");
      }
      // If previously a TransferFuture has been requested by readAsync(), first make sure that that future is
      // already fulfilled. Otherwise we might run out of futures on the notification queue.
      if(mtca4u::TransferElement::hasActiveFuture) {
        return mtca4u::TransferElement::activeFuture.hasNewData();
      }
      // We have to check that the vector that we currently own still has the
      // right size. Otherwise, the code using the sender might get into
      // trouble when it suddenly experiences a vector of the wrong size.
      if (_sharedState->_buffers[_currentIndex].value.size() != _vectorSize) {
        throw std::runtime_error("Cannot run receive operation because the size of the vector belonging to the current"
                                 " buffer has been modified.");
      }
      std::size_t nextIndex;
      if (_sharedState->_fullBufferQueue.pop(nextIndex)) {
        // We only use the incoming update if it has a higher version number
        // than the current value. This check is disabled when there is no
        // version number source.
        if (!_versionNumberSource || (_sharedState->_buffers[nextIndex]).versionNumber > getVersionNumber()) {
          _sharedState->_emptyBufferQueue.push(_currentIndex);
          _currentIndex = nextIndex;
          return true;
        } else {
          _sharedState->_emptyBufferQueue.push(nextIndex);
          return false;
        }
      } else {
        return false;
      }
    }

    bool doReadTransferLatest() override {
      bool hasRead = false;
      while(doReadTransferNonBlocking()) hasRead = true;
      return hasRead;
    }

    mtca4u::TransferFuture& readAsync() override {
      if (_instanceType != RECEIVER) {
        throw std::logic_error("Receive operation is only allowed for a receiver process variable.");
      }
      ChimeraTK::ExperimentalFeatures::check("asynchronous read");
      if(mtca4u::TransferElement::hasActiveFuture) return mtca4u::TransferElement::activeFuture;  // the last future given out by this fuction is still active
      
      // check if we already have data
      boost::promise<void> promise;
      boost::shared_future<void> readyFuture = promise.get_future().share();
      promise.set_value();
      if(doReadTransferNonBlocking()) {
        mtca4u::TransferElement::activeFuture = mtca4u::TransferFuture(readyFuture, static_cast<mtca4u::TransferElement*>(this));
        mtca4u::TransferElement::hasActiveFuture = true;
        _asyncReadTransferNeeded = false;
        return mtca4u::TransferElement::activeFuture;
      }
      else {
        boost::shared_future<void> future;
        // obtain future from queue
        bool atLeastOneFuturePresent = _sharedState->_notificationQueue.pop(future);
        assert(atLeastOneFuturePresent);  // if this is not true, there is something wrong with the algorithm
        (void)atLeastOneFuturePresent;    // prevent warning in case asserts are disabled
        // if this future is already fulfilled, obtain the next one
        if(future.is_ready()) {
          // before we can obtain the next future, we must check for new data once more
          if(doReadTransferNonBlocking()) {
            mtca4u::TransferElement::activeFuture = mtca4u::TransferFuture(readyFuture, static_cast<mtca4u::TransferElement*>(this));
            mtca4u::TransferElement::hasActiveFuture = true;
           _asyncReadTransferNeeded = false;
            return mtca4u::TransferElement::activeFuture;
          }
          atLeastOneFuturePresent = _sharedState->_notificationQueue.pop(future);
          assert(atLeastOneFuturePresent);
        }
        // return the future
        mtca4u::TransferElement::activeFuture = mtca4u::TransferFuture(future, static_cast<mtca4u::TransferElement*>(this));
        mtca4u::TransferElement::hasActiveFuture = true;
        _asyncReadTransferNeeded = true;
        return mtca4u::TransferElement::activeFuture;
      }
    }
    
    void postRead() override {
      // if this is called inside mtca4u::TransferFuture::wait(), we may still need to obtain the new data from the queue
      if(_asyncReadTransferNeeded) {
        _asyncReadTransferNeeded = false;
        mtca4u::TransferElement::hasActiveFuture = false;  // prevent doReadTransferNonBlocking() from checking the future only
        bool hasNewDataAfterFuture = doReadTransferNonBlocking();
        if(!hasNewDataAfterFuture) {
          // we did not receive new data despite being notified about new data...
          // this either means there is a bug in the ControlSystemAdapter, or the application has called postRead() from
          // a different thread while mtca4u::TransferFuture::wait() is still waiting. This is in principle violating
          // the interface definition, but can be safe under certain circumstances (e.g. in ApplicationCore's testable
          // mode, when a stall is detected and debugging information should be printed). Thus we do not raise an
          // assertion but throw an exception which can be caught when this is known to be safe.
          throw std::logic_error("postRead() called despite no new data has arrived. Are you reading the same "
                                 "ProcessArray from different threads? Variable name: "+this->getName());
        }
      }
      // swap data out of the queue buffer
      mtca4u::NDRegisterAccessor<T>::buffer_2D[0].swap( (_sharedState->_buffers[_currentIndex]).value );
    }

    bool write() override {
      VersionNumber newVersionNumber;
      if (_versionNumberSource) {
        newVersionNumber = _versionNumberSource->nextVersionNumber();
      } else {
        newVersionNumber = 0;
      }
      return write(newVersionNumber);
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
    bool write(VersionNumber newVersionNumber) { /// @todo FIXME this function must be present in TransferElement already! ???
      return writeInternal(newVersionNumber, true);
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
     * This version of the send operation moves the current value from the
     * sender to the receiver without copying it. This means that after calling
     * this method, the sender's value, time stamp, and version number are
     * undefined. Therefore, this method must only be used if this process
     * variable is not read (on the sender side) after sending it.
     *
     * Throws an exception if this process variable is not a sender or if this
     * process variable does not allow destructive sending.
     */
    bool writeDestructively() { /// @todo FIXME this function must be present in TransferElement already!
      VersionNumber newVersionNumber;
      if (_versionNumberSource) {
        newVersionNumber = _versionNumberSource->nextVersionNumber();
      } else {
        newVersionNumber = 0;
      }
      return writeDestructively(newVersionNumber);
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
     * This version of the send operation moves the current value from the
     * sender to the receiver without copying it. This means that after calling
     * this method, the sender's value, time stamp, and version number are
     * undefined. Therefore, this method must only be used if this process
     * variable is not read (on the sender side) after sending it.
     *
     * Throws an exception if this process variable is not a sender or if this
     * process variable does not allow destructive sending.
     */
    bool writeDestructively(VersionNumber newVersionNumber) { /// @todo FIXME this function must be present in TransferElement already!
      if (!_maySendDestructively) {
        throw std::runtime_error(
            "This process variable must not be sent destructively because the corresponding flag has not been set.");
      }
      return writeInternal(newVersionNumber, false);
    }

    const std::type_info& getValueType() const override {
      return typeid(T);
    }

    bool isArray() const override {
      return true;
    }

    bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const override {
      // only true if the very instance of the transfer element is the same
      return e.get() == this;
    }

    std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
      return { boost::enable_shared_from_this<mtca4u::TransferElement>::shared_from_this() };
    }
    
    void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement>) override {
      // You can't replace anything here. Just do nothing.
    }
    
    void setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) override {
      if(!isWriteable()) return;
      bool sendInitialValue = false;
      if(!_persistentDataStorage) sendInitialValue = true;
      _persistentDataStorage = storage;
      _persistentDataStorageID = _persistentDataStorage->registerVariable<T>(mtca4u::TransferElement::getName(),
                                    mtca4u::NDRegisterAccessor<T>::getNumberOfSamples());
      if(sendInitialValue) {
         mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = _persistentDataStorage->retrieveValue<T>(_persistentDataStorageID);
         write();
      }
    }

    /** Return a unique ID of this process variable, which will be indentical for the receiver and sender side of the
     *  same variable but different for any other process variable within the same process. The unique ID will not be
     *  persistent accross executions of the process. */
    size_t getUniqueId() const {
      return reinterpret_cast<size_t>(_sharedState.get());      // use pointer address of the shared state
    }

  private:

    /**
     * Type for the individual buffers. Each buffer stores a vector (wrapped
     * in a Boost scoped pointer) and a time stamp.
     */
    struct Buffer {

      TimeStamp timeStamp;
      std::vector<T> value;
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
     * Flag indicating whether the {@code sendDestructively} methods may be used
     * on this process array.
     */
    bool _maySendDestructively;
                        
    /**
     * The state shared between the sender and the receiver
     */
    struct SharedState {

      SharedState(size_t numberOfBuffers)
      : _buffers(numberOfBuffers + 2),
        _fullBufferQueue(numberOfBuffers),
        _emptyBufferQueue(numberOfBuffers)
      {}

      /**
      * Buffers that hold the actual values.
      */
      std::vector<Buffer> _buffers;

      /**
      * Queue holding the indices of the full buffers. Those are the buffers
      * that have been sent but not yet received. We do not use an spsc_queue
      * for this queue, because might want to take elements from the sending
      * thread, so there are two threads which might consume elements and thus
      * an spsc_queue is not safe.
      */
      boost::lockfree::queue<std::size_t> _fullBufferQueue;

      /**
      * Queue holding the empty buffers. Those are the buffers that have been
      * returned to the sender by the receiver.
      */
      boost::lockfree::spsc_queue<std::size_t> _emptyBufferQueue;

      /**
      * Queue holding futures for notification. The receiver can obtain a future from this queue when the
      * _fullBufferQueue is empty and wait on it until new data has been sent.
      * 
      * It is sufficient to have a fixed queue length of 2, since it is only important to guarantee the presence of
      * at least one unfulfilled future in the queue.
      * 
      * We are using a shared future, since the spsc_queue expects a copy-constructable data type. Since the data
      * type void is not particularly expensive to copy, this should have no performance impact.
      */
      boost::lockfree::spsc_queue< boost::shared_future<void>, boost::lockfree::capacity<2> > _notificationQueue;
    
      /**
      * The promise corresponding to the unfulfilled future in the _notificationQueue.
      * This promise is basically only used by the sender. Only in the destructor of the receiver it is used to
      * shutdown properly, thus it must be in the shared state.
      */
      boost::promise<void> _notificationPromise;

    };
    boost::shared_ptr<SharedState> _sharedState;

    /**
     * Index into the _buffers array that is currently owned by this instance
     * and used for read and (possibly) write operations.
     */
    std::size_t _currentIndex;

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
    
    /**
     * Persistent data storage which needs to be informed when the process variable is sent.
     */
    boost::shared_ptr<PersistentDataStorage> _persistentDataStorage;

    /**
     * Variable ID for the persistent data storage
     */
    size_t _persistentDataStorageID{0};
    
    /**
     * Flag set in readAsync() whether doReadTransferNonBlocking() must or must not be called in postRead() after the
     * future go tready.
     */
    bool _asyncReadTransferNeeded{false};

    /**
     * Internal implementation of the various {@code send} methods. All these
     * methods basically do the same and only differ in whether the data in the
     * sent array is copied to the buffer used as the new current buffer and
     * which version number is used.
     * 
     * The return value will be true, if older data was overwritten during the send operation, or otherwise false.
     */
    bool writeInternal(VersionNumber newVersionNumber, bool shouldCopy) {
      if (_instanceType != SENDER) {
        throw std::logic_error(
            "Send operation is only allowed for a sender process variable. Variable name: "+this->getName());
      }
      // We have to check that the vector that we currently own still has the
      // right size. Otherwise, the code using the receiver might get into
      // trouble when it suddenly experiences a vector of the wrong size.
      if (mtca4u::NDRegisterAccessor<T>::buffer_2D[0].size() != _vectorSize) {
        throw std::runtime_error(
            "Cannot run receive operation because the size of the vector belonging to the current buffer has been "
            "modified. Variable name: "+this->getName());
      }
      // First update the persistent data storage, if any was associated. This cannot be done after sending, since the
      // value might no longer be available within this instance.
      if (_persistentDataStorage) {
        _persistentDataStorage->updateValue(_persistentDataStorageID, mtca4u::NDRegisterAccessor<T>::buffer_2D[0]);
      }
      
      // Before sending the value, we have to update the associated
      // time-stamp.
      TimeStamp newTimeStamp =
          _timeStampSource ?
              _timeStampSource->getCurrentTimeStamp() :
              TimeStamp::currentTime();
      (_sharedState->_buffers[_currentIndex]).timeStamp = newTimeStamp;
      (_sharedState->_buffers[_currentIndex]).versionNumber = newVersionNumber;
      if (shouldCopy) {
        _sharedState->_buffers[_currentIndex].value = mtca4u::NDRegisterAccessor<T>::buffer_2D[0];
      }
      else {
        _sharedState->_buffers[_currentIndex].value.swap( mtca4u::NDRegisterAccessor<T>::buffer_2D[0] );
      }
      std::size_t nextIndex;
      bool lostData = false;
      if (_sharedState->_emptyBufferQueue.pop(nextIndex)) {
        _sharedState->_fullBufferQueue.push(_currentIndex);
      } else {
        // We can still send, but we will lose older data.
        if (_sharedState->_fullBufferQueue.pop(nextIndex)) {
          _sharedState->_fullBufferQueue.push(_currentIndex);
          lostData = true;
        } else {
          // It is possible, that we did not find an empty buffer but before
          // we could get a full buffer, the receiver processed all full
          // buffers. In this case, the queue of empty buffers cannot be empty
          // any longer because we have at least four buffers.
          if (!_sharedState->_emptyBufferQueue.pop(nextIndex)) {
            // This should never happen and is just an assertion for extra
            // safety.
            throw std::runtime_error(
                "Assertion that empty-buffer queue has at least one element failed.");
          }
          _sharedState->_fullBufferQueue.push(_currentIndex);
        }
      }
      if (shouldCopy) {
        _sharedState->_buffers[nextIndex].timeStamp = newTimeStamp;
        _sharedState->_buffers[nextIndex].versionNumber = newVersionNumber;
      }
      _currentIndex = nextIndex;
      if (_sendNotificationListener) {
        _sendNotificationListener->notify(_receiver);
      }
      // Notify the receiver through the notification queue. It must be guaranteed that there is at least one
      // unfulfilled future in the notification queue at all times, so we try pushing a new one first. If this
      // failes, there is already one unfulfilled and one fulfilled future in the queue, so we are done.
      boost::promise<void> nextPromise;
      if(_sharedState->_notificationQueue.push(nextPromise.get_future())) {
        try {
          _sharedState->_notificationPromise.set_value();
        }
        catch(boost::promise_already_satisfied) {
          // ignore -> this may happen only during shutdown
        }
        _sharedState->_notificationPromise = std::move(nextPromise);
      }

      return lostData;
    }
  };

/*********************************************************************************************************************/
/*** Non-member functions below this line ****************************************************************************/
/*********************************************************************************************************************/

  /**
   * Creates a simple process array. A simple process array just works on its
   * own and does not implement a synchronization mechanism. Apart from this,
   * it is fully functional, so that you get, set, and swap values.
   *
   * The specified initial value is used for all the elements of the array.
   */
  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(std::size_t size, const std::string &name = "",
      const std::string &unit = "", const std::string &description = "", T initialValue = 0);

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
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(const std::vector<T>& initialValue,
    const std::string & name = "", const std::string &unit = "", const std::string &description = "");

  /**
   * Creates a synchronized process array. A synchronized process array works
   * as a pair of two process arrays, where one process array acts as a sender
   * and the other one acts as a receiver.
   *
   * The sender allows full read-write access. Changes that have been made to
   * the sender can be sent to the receiver through the
   * {@link ProcessArray::write()} method. The receiver can be updated with these
   * changes by calling its readNonBlocking() method.
   *
   * The synchronization is implemented in a thread-safe manner, so that the
   * sender and the receiver can safely be used by different threads without
   * a mutex. However, both the sender and receiver each only support a single
   * thread. This means that the sender or the receiver have to be protected
   * with a mutex if more than one thread wants to access either of them.
   *
   * The number of buffers specifies how many buffers are allocated for the
   * send / receive mechanism. The minimum number (and default) is two. This
   * number specifies, how many times {@link ProcessArray::write()} can be called
   * in a row without losing data when readNonBlocking() is not
   * called in between.
   *
   * If the <code>maySendDestructively</code> flag is <code>true</code> (it is
   * <code>false</code> by default), the {@link ProcessArray::writeDestructively()} method may
   * be used to transfer values without copying but losing them on the sender
   * side.
   *
   * The specified time-stamp source is used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used.
   *
   * The optional send-notification listener is notified every time the sender's
   * {@link ProcessArray::write()} method is called. It can be used to queue a
   * request for the receiver's readNonBlocking() method to be
   * called.  The process variable passed to the listener is the receiver and
   * not the sender.
   *
   * The specified initial value is used for all the elements of the array.
   */
  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const std::string & name = "", const std::string &unit = "",
      const std::string &description = "", T initialValue = 0, std::size_t numberOfBuffers = 2,
      bool maySendDestructively = false,
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
   * {@link ProcessArray::write()} method. The receiver can be updated with these
   * changes by calling its readNonBlocking() method.
   *
   * The synchronization is implemented in a thread-safe manner, so that the
   * sender and the receiver can safely be used by different threads without
   * a mutex. However, both the sender and receiver each only support a single
   * thread. This means that the sender or the receiver have to be protected
   * with a mutex if more than one thread wants to access either of them.
   *
   * The number of buffers specifies how many buffers are allocated for the
   * send / receive mechanism. The minimum number (and default) is two. This
   * number specifies, how many times {@link ProcessArray::write()} can be called
   * in a row without losing data when readNonBlocking() is not
   * called in between.
   *
   * If the <code>maySendDestructively</code> flag is <code>true</code> (it is
   * <code>false</code> by default), the {@link ProcessArray::writeDestructively()} method may
   * be used to transfer values without copying but losing them on the sender
   * side.
   *
   * The specified time-stamp source is used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used.
   *
   * The optional send-notification listener is notified every time the sender's
   * {@link ProcessArray::write()} method is called. It can be used to queue a
   * request for the receiver's readNonBlocking() method to be
   * called.  The process variable passed to the listener is the receiver and
   * not the sender.
   *
   * The array's size is set to the number of elements stored in the vector
   * provided for initialization and all elements are initialized with the
   * values provided by this vector.
   */
  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      const std::vector<T>& initialValue, const std::string & name = "", const std::string &unit = "",
      const std::string &description = "", std::size_t numberOfBuffers = 2, bool maySendDestructively = false,
      TimeStampSource::SharedPtr timeStampSource = TimeStampSource::SharedPtr(),
      VersionNumberSource::SharedPtr versionNumberSource =
          VersionNumberSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener =
          ProcessVariableListener::SharedPtr());

/*********************************************************************************************************************/
/*** Implementations below this line *********************************************************************************/
/*********************************************************************************************************************/

  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(std::size_t size, const std::string & name,
      const std::string &unit, const std::string &description, T initialValue) {
    return boost::make_shared<ProcessArray<T> >(ProcessArray<T>::STAND_ALONE,
        name, unit, description, std::vector<T>(size, initialValue));
  }

/*********************************************************************************************************************/

  template<class T>
  typename ProcessArray<T>::SharedPtr createSimpleProcessArray(
      const std::vector<T>& initialValue, const std::string & name, const std::string &unit, const std::string &description) {
    return boost::make_shared<ProcessArray<T> >(ProcessArray<T>::STAND_ALONE,
        name, unit, description, initialValue);
  }

/*********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const std::string & name, const std::string &unit, const std::string &description, T initialValue,
      std::size_t numberOfBuffers, bool maySendDestructively,
      TimeStampSource::SharedPtr timeStampSource,
      VersionNumberSource::SharedPtr versionNumberSource,
      ProcessVariableListener::SharedPtr sendNotificationListener) {
    typename boost::shared_ptr<ProcessArray<T> > receiver = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::RECEIVER, name, unit, description, 
        std::vector<T>(size, initialValue), numberOfBuffers,
        versionNumberSource);
    typename ProcessArray<T>::SharedPtr sender = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::SENDER, maySendDestructively,
        timeStampSource, versionNumberSource, sendNotificationListener,
        receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

/*********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      const std::vector<T>& initialValue, const std::string & name, const std::string &unit, const std::string &description,
      std::size_t numberOfBuffers, bool maySendDestructively,
      TimeStampSource::SharedPtr timeStampSource,
      VersionNumberSource::SharedPtr versionNumberSource,
      ProcessVariableListener::SharedPtr sendNotificationListener) {
    typename boost::shared_ptr<ProcessArray<T> > receiver = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::RECEIVER, name, unit, description, initialValue,
        numberOfBuffers, versionNumberSource);
    typename ProcessArray<T>::SharedPtr sender = boost::make_shared<
        ProcessArray<T> >(ProcessArray<T>::SENDER, maySendDestructively,
        timeStampSource, versionNumberSource, sendNotificationListener,
        receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_ARRAY_H
