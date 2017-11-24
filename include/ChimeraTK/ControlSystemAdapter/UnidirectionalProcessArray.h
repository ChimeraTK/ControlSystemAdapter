#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H

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

#include <mtca4u/VersionNumber.h>

#include "ProcessArray.h"
#include "ProcessVariableListener.h"
#include "TimeStampSource.h"
#include "PersistentDataStorage.h"

namespace ChimeraTK {

  /**
   * Implementation of the process array that transports data in a single
   * direction. This implementation is used for both sides (sender and
   * receiver).
   *
   * This class is not thread-safe and should only be used from a single thread.
   */
  template<class T>
  class UnidirectionalProcessArray : public ProcessArray<T> {

  public:

    /**
     * Type alias for a shared pointer to this type.
     */
    typedef boost::shared_ptr<UnidirectionalProcessArray> SharedPtr;

    /**
     * Creates a process array that acts as a receiver. A receiver is
     * intended to work as a tandem with a sender and receives values that
     * have been set to the sender.
     *
     * This constructor creates the buffers and queues that are needed for the
     * send/receive process and are shared with the sender.
     */
    UnidirectionalProcessArray(
        typename ProcessArray<T>::InstanceType instanceType,
        const mtca4u::RegisterPath& name, const std::string &unit,
        const std::string &description, const std::vector<T>& initialValue,
        std::size_t numberOfBuffers);

    /**
     * Creates a process array that acts as a sender. A sender is intended
     * intended to work as a tandem with a receiver and send set values to
     * it. It uses the buffers and queues that have been created by the
     * receiver.
     *
     * If the <code>maySendDestructively</code> flag is <code>true</code>, the
     * {@link ProcessArray::writeDestructively()} method may be used to transfer
     * values without copying but losing them on the sender side.
     *
     * The optional send-notification listener is notified every time the
     * sender's ProcessArray::write() method is called. It can be
     * used to queue a request for the receiver's
     * readNonBlocking() method to be called. The process
     * variable passed to the listener is the receiver and not the sender.
     */
    UnidirectionalProcessArray(
        typename ProcessArray<T>::InstanceType instanceType,
        bool maySendDestructively, TimeStampSource::SharedPtr timeStampSource,
        ProcessVariableListener::SharedPtr sendNotificationListener,
        UnidirectionalProcessArray::SharedPtr receiver);

    TimeStamp getTimeStamp() const override {
      return _currentIndex->timeStamp;
    }

    ChimeraTK::VersionNumber getVersionNumber() const override {
      return _currentIndex->_versionNumber;
    }

    void doReadTransfer() override;

    bool doReadTransferNonBlocking() override;

    bool doReadTransferLatest() override;

    mtca4u::TransferFuture readAsync() override;

    void postRead() override;

    bool write(ChimeraTK::VersionNumber versionNumber={}) override;

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
     * The time-stamp of the sent value is retrieved from the time-stamp source.
     *
     * Throws an exception if this process variable is not a sender or if this
     * process variable does not allow destructive sending.
     */
    bool writeDestructively(ChimeraTK::VersionNumber versionNumber={}) override; /// @todo FIXME this function must be present in TransferElement already!

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
     * The time-stamp source of this process array is ignored and the passed
     * time stamp is used instead.
     *
     * Throws an exception if this process variable is not a sender or if this
     * process variable does not allow destructive sending.
     */
    bool writeDestructively(ChimeraTK::TimeStamp timeStamp, ChimeraTK::VersionNumber versionNumber);

    void setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) override ;

    /** Return a unique ID of this process variable, which will be indentical for the receiver and sender side of the
     *  same variable but different for any other process variable within the same process. The unique ID will not be
     *  persistent accross executions of the process. */
    size_t getUniqueId() const override {
      return reinterpret_cast<size_t>(_sharedState.get());      // use pointer address of the shared state
    }

    bool asyncTransferActive() override {
      return hasActiveFuture;
    }

    void clearAsyncTransferActive() override {
      hasActiveFuture = false;
    }

  private:

    /**
     * Type for the individual buffers. Each buffer stores a vector (wrapped in a Boost scoped pointer) and a time
     * stamp. It extends the TransferFuture::Data type (which provides the version number) so it can be transported
     * by a TransferFuture.
     */
    struct Buffer : public TransferFuture::Data {

      /**
       * Default constructor. Has to be defined explicitly because we delete our copy constructor.
       */
      Buffer()
      : TransferFuture::Data({})
      {}

      /**
       * Delete all copy and move constructors and assignment operators. In C++11, elements of a std::vector no longer
       * have to be copy-constructable if the size of the vector is fixed and specified in the constructor.
       */
      Buffer(const Buffer &other) = delete;
      Buffer& operator=(const Buffer &other) = delete;
      Buffer(Buffer &&other) = delete;
      Buffer& operator=(Buffer &&other) = delete;

      /** The actual data contained in this buffer. */
      std::vector<T> value;

      /** Flag whether this buffer currently contains valid data. This flag is used only for the atomic triple buffer
       *  and not set for buffers on the queues (since the queue the buffer is in already tells whether it contains
       *  valid data or not). */
      bool _isValid{false};

      /**
       * Extra version number used to sort the values within the process variable (so this version is not globally
       * valid). This number is needed in addition to the globally valid _versionNumber defined in
       * TransferFuture::Data, because the globally valid version numebr can be controlled by the application and
       * values with an older version number might be written to this process variable later. That later data with
       * the older version number shall still be considered as last data e.g. in the context of readLatest(), i.e.
       * whatever has been written to the process variable last should be read with readLatest(), even if it has
       * the older version number.
       */
      uint64_t _internalVersionNumber;

      /** Time stamp associated with this buffer */
      TimeStamp timeStamp;

    };

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
     * Last internal version number written. See Buffer::_internalVersionNumber for more details.
     */
    uint64_t _lastInternalVersionNumber{0};

    /**
     * The state shared between the sender and the receiver
     */
    struct SharedState {

      SharedState(size_t numberOfBuffers)
      : _buffers(numberOfBuffers + 2 + 3),    // two additional buffers owned by sender and receiver, 3 more additional
                                              // buffers for the atomic triple buffer
        _tripleBufferIndex(&(_buffers[3])),
        _fullBufferQueue(numberOfBuffers+1),
        _emptyBufferQueue(numberOfBuffers)
      {}

      /**
      * Buffers that hold the actual values.
      */
      std::vector<Buffer> _buffers;

      /**
       * Special atomic triple buffer to be filled when the queue runs over
       * (i.e. all normal buffers are full). After construction, buffer 2 is
       * owned by the sender, 3 by the shared state and 4 by the receiver. A
       * buffer is considered containing valid data if its _isValid flag is
       * set. The buffer's default constructor sets _isValid to false.
       */
      std::atomic<Buffer*> _tripleBufferIndex;

      /**
      * Queue holding the indices of the full buffers. Those are the buffers
      * that have been sent but not yet received. We can use an spsc_queue for
      * this queue because it is only filled by the sending process array and
      * only consumed by the receiving process array. In fact, we have to use
      * an spsc_queue because the standard boost::lockfree::queue cannot hold
      * futures (as they are not trivially assignable / destructable).
      */
      boost::lockfree::spsc_queue< TransferFuture::PlainFutureType > _fullBufferQueue;

      /**
      * Queue holding the empty buffers. Those are the buffers that have been
      * returned to the sender by the receiver.
      */
      boost::lockfree::spsc_queue<Buffer*> _emptyBufferQueue;

      /**
      * The promise corresponding to the unfulfilled future in the _fullBufferQueue.
      * This promise is basically only used by the sender.
      */
      TransferFuture::PromiseType _notificationPromise;

    };
    boost::shared_ptr<SharedState> _sharedState;

    /**
     * Index into the _buffers array that is currently owned by this instance
     * and used for read and (possibly) write operations.
     */
    Buffer *_currentIndex;

    /**
     * Index into the _buffers array that is currently owned by this instance.
     * The _tripleBufferIndex is different from the _currentIndex because it is
     * only used as a fallback when a value cannot be transferred through the
     * _fullBufferQueue because the _emptyBufferQueue is empty.
     */
    Buffer *_tripleBufferIndex;

    /**
     * Pointer to the receiver associated with this sender. This field is only
     * used if this process variable represents a sender.
     */
    boost::shared_ptr<UnidirectionalProcessArray> _receiver;

    /**
     * Time-stamp source used to update the time-stamp when sending a value.
     */
    boost::shared_ptr<TimeStampSource> _timeStampSource;

    /**
     * Listener that is notified when the process variable is sent.
     */
    boost::shared_ptr<ProcessVariableListener> _sendNotificationListener;

    /**
     * Persistent data storage which needs to be informed when the process variable is sent.
     */
    boost::shared_ptr<PersistentDataStorage> _persistentDataStorage;

    /**
     * The last future returned by readAsync
     */
    mtca4u::TransferFuture activeFuture;

    /**
     * Flag whether acviteFuture is valid
     */
    bool hasActiveFuture{false};

    /**
     * Variable ID for the persistent data storage
     */
    size_t _persistentDataStorageID{0};

    /**
     * Internal implementation of the various {@code send} methods. All these
     * methods basically do the same and only differ in whether the data in the
     * sent array is copied to the buffer used as the new current buffer and
     * which version number is used.
     *
     * The return value will be true, if older data was overwritten during the send operation, or otherwise false.
     */
    bool writeInternal(TimeStamp newTimeStamp, VersionNumber newVersionNumber,
        bool shouldCopy);

  };

/*********************************************************************************************************************/
/*** Non-member functions below this line ****************************************************************************/
/*********************************************************************************************************************/

  /**
   * Creates a synchronized process array. A synchronized process array works
   * as a pair of two process arrays, where one process array acts as a sender
   * and the other one acts as a receiver.
   *
   * The sender allows full read-write access. Changes that have been made to
   * the sender can be sent to the receiver through the
   * ProcessArray::write() method. The receiver can be updated with these
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
   * number specifies, how many times ProcessArray::write() can be called
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
   * ProcessArray::write() method is called. It can be used to queue a
   * request for the receiver's readNonBlocking() method to be
   * called.  The process variable passed to the listener is the receiver and
   * not the sender.
   *
   * The specified initial value is used for all the elements of the array.
   */
  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const mtca4u::RegisterPath & name = "", const std::string &unit = "",
      const std::string &description = "", T initialValue = T(), std::size_t numberOfBuffers = 2,
      bool maySendDestructively = false,
      TimeStampSource::SharedPtr timeStampSource = TimeStampSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener =
          ProcessVariableListener::SharedPtr());

  /**
   * Creates a synchronized process array. A synchronized process array works
   * as a pair of two process arrays, where one process array acts as a sender
   * and the other one acts as a receiver.
   *
   * The sender allows full read-write access. Changes that have been made to
   * the sender can be sent to the receiver through the
   * ProcessArray::write() method. The receiver can be updated with these
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
   * number specifies, how many times ProcessArray::write() can be called
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
   * ProcessArray::write() method is called. It can be used to queue a
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
      const std::vector<T>& initialValue, const mtca4u::RegisterPath & name = "", const std::string &unit = "",
      const std::string &description = "", std::size_t numberOfBuffers = 2, bool maySendDestructively = false,
      TimeStampSource::SharedPtr timeStampSource = TimeStampSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener =
          ProcessVariableListener::SharedPtr());

/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T>
  UnidirectionalProcessArray<T>::UnidirectionalProcessArray(
      typename ProcessArray<T>::InstanceType instanceType,
      const mtca4u::RegisterPath& name, const std::string &unit,
      const std::string &description, const std::vector<T>& initialValue,
      std::size_t numberOfBuffers) :
      ProcessArray<T>(instanceType, name, unit, description), _vectorSize(
          initialValue.size()), _maySendDestructively(false), _sharedState(
          boost::make_shared<SharedState>(numberOfBuffers)), _currentIndex(
          &(_sharedState->_buffers[0])), _tripleBufferIndex(
          &(_sharedState->_buffers[4]))
  {
    // allocate and initialise buffer of the base class
    mtca4u::NDRegisterAccessor<T>::buffer_2D.resize(1);
    mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = initialValue;
    // It would be better to do the validation before initializing, but this
    // would mean that we would have to initialize twice.
    if (!this->isReadable()) {
      throw std::invalid_argument("This constructor may only be used for a receiver process variable.");
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
    if (numberOfBuffers > (std::numeric_limits<std::size_t>::max() - (2+3))) {
      throw std::invalid_argument("The number of buffers is too large.");
    }
    // We have to initialize the buffers by copying in the initial vectors.
    for (auto &i : _sharedState->_buffers) {
      i.value = initialValue;
    }
    // The buffer with the index 0 is assigned to the receiver and the buffer with the index 1 is assigned to the
    // sender. Indices 2-4 are assigned to the atomic triple buffer (for detailed assignment see definition of
    // atomic triple buffer in the shared state). All other buffers have to be added to the empty-buffer queue.
    for (std::size_t i = 2+3; i < _sharedState->_buffers.size(); ++i) {
      _sharedState->_emptyBufferQueue.push(&(_sharedState->_buffers[i]));
    }
  }

/*********************************************************************************************************************/

  template<class T>
  UnidirectionalProcessArray<T>::UnidirectionalProcessArray(
      typename ProcessArray<T>::InstanceType instanceType,
      bool maySendDestructively, TimeStampSource::SharedPtr timeStampSource,
      ProcessVariableListener::SharedPtr sendNotificationListener,
      UnidirectionalProcessArray::SharedPtr receiver) :
      ProcessArray<T>(instanceType, receiver->getName(), receiver->getUnit(),
          receiver->getDescription()), _vectorSize(receiver->_vectorSize), _maySendDestructively(
          maySendDestructively), _sharedState(receiver->_sharedState), _currentIndex(
          &(_sharedState->_buffers[1])), _tripleBufferIndex(
          &(_sharedState->_buffers[2])), _receiver(receiver), _timeStampSource(
          timeStampSource), _sendNotificationListener(sendNotificationListener)
  {
    // It would be better to do the validation before initializing, but this
    // would mean that we would have to initialize twice.
    if (!this->isWriteable()) {
      throw std::invalid_argument("This constructor may only be used for a sender process variable.");
    }
    if (!receiver) {
      throw std::invalid_argument("The pointer to the receiver must not be null.");
    }
    if (!receiver->isReadable()) {
      throw std::invalid_argument(
          "The pointer to the receiver must point to an instance that is actually a receiver.");
    }
    // allocate and initialise buffer of the base class
    mtca4u::NDRegisterAccessor<T>::buffer_2D.resize(1);
    mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = receiver->buffer_2D[0];
    // put future into the notification queue
    _sharedState->_fullBufferQueue.push(_sharedState->_notificationPromise.get_future());
  }

/*********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::doReadTransfer() {

    // Obtain future and wait until transfer is complete. Do not yet call postRead(), so do not call
    // TransferFuture::wait().
    readAsync().getBoostFuture().wait();
    boost::this_thread::interruption_point();

  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::doReadTransferNonBlocking() {
    bool ret = readAsync().hasNewData();
    return ret;
  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::doReadTransferLatest() {

    // As long as there is more than one valid element on the queue, discard it.
    // Due to our implementation there is always one unfulfilled future in the queue, so
    // we must pop until there are two elements left in order not to flush out the newest valid value.
    auto theFuture = readAsync().getBoostFuture();
    while(    theFuture.wait_for(boost::chrono::duration<int, boost::centi>(0)) != boost::future_status::timeout
           && _sharedState->_fullBufferQueue.read_available() > 2                                                ) {
      // discard data by moving the buffer to the empty buffer queue
      TransferFuture::Data *discardedBuffer = theFuture.get();
      _sharedState->_fullBufferQueue.pop();
      hasActiveFuture = false;
      _sharedState->_emptyBufferQueue.push(static_cast<Buffer*>(discardedBuffer));    // static cast is ok, we never put something else into the queue
      theFuture = readAsync().getBoostFuture();
    }
    // Check whether data is present in the atomic triple buffer. If it is newer, we also need to discard the last
    // valid element in the queue. If it is older, we need to discard the data in the triple buffer.
    // First update the triple buffer if it is not valid on our side
    if(!(_tripleBufferIndex->_isValid)) {
      _tripleBufferIndex = _sharedState->_tripleBufferIndex.exchange(_tripleBufferIndex);
    }
    // if it now contains a valid element, check if it is newer
    if(_tripleBufferIndex->_isValid) {
      uint64_t tripleBufferVersion = _tripleBufferIndex->_internalVersionNumber;
      uint64_t queueVersion = static_cast<Buffer*>(theFuture.get())->_internalVersionNumber;
      if(tripleBufferVersion > queueVersion) {
        TransferFuture::Data *discardedBuffer = theFuture.get();
        _sharedState->_fullBufferQueue.pop();
        _sharedState->_emptyBufferQueue.push(static_cast<Buffer*>(discardedBuffer));
      }
      else {
        _tripleBufferIndex->_isValid = false;
      }
    }
    return this->doReadTransferNonBlocking();
  }

/*********************************************************************************************************************/

  template<class T>
  mtca4u::TransferFuture UnidirectionalProcessArray<T>::readAsync() {

    // If we already have a still-active future, we can return it directly. It is not possible that this future is
    // blocking and there is valid data to be read in the atomic triple buffer.
    // This is only a preformance optimisation, we would generate the same future again.
    if(hasActiveFuture) return activeFuture;

    if (!this->isReadable()) {
      throw std::logic_error("Receive operation is only allowed for a receiver process variable.");
    }

    // Obtain future from queue but do not pop it yet, since we need to determine first whether to use it nor not.
    // The future will only be popped from the queue in postRead(). This makes sure that subsequent calls to this
    // function even if the future has not yet been used still result in the correct behaviour.
    assert(_sharedState->_fullBufferQueue.read_available() > 0);   // guaranteed by our logic
    TransferFuture::PlainFutureType future = _sharedState->_fullBufferQueue.front();

    // if the future blocks, check for data in the tripple buffer
    auto status = future.wait_for(boost::chrono::duration<int, boost::centi>(0));
    if(status == boost::future_status::timeout) {
      // if local buffer of triple buffer is invalid, swap with shared buffer first
      if(!(_tripleBufferIndex->_isValid)) {
        _tripleBufferIndex = _sharedState->_tripleBufferIndex.exchange(_tripleBufferIndex);
      }
      // if now the local buffer contains valid data, use that data first
      if(_tripleBufferIndex->_isValid) {
        future = boost::make_ready_future(static_cast<TransferFuture::Data*>(_tripleBufferIndex)).share();
      }
    }

    // return the future
    activeFuture = TransferFuture(future, this);
    hasActiveFuture = true;
    return activeFuture;

  }

/*********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::postRead() {

    // need to reset the hasActiveFuture flag ourselves, since we internally always use readAsync, but
    // TransferFuture::wait() is not always called then.
    hasActiveFuture = false;

    // We have to check that the vector that we currently own still has the
    // right size. Otherwise, the code using the sender might get into
    // trouble when it suddenly experiences a vector of the wrong size.
    if(_currentIndex->value.size() != _vectorSize) {
      throw std::runtime_error("Cannot run read operation because the size of the vector belonging to the current"
                                " buffer has been modified.");
    }

    // If local buffer of triple buffer is invalid, swap with shared buffer first
    if(!(_tripleBufferIndex->_isValid)) {
      _tripleBufferIndex = _sharedState->_tripleBufferIndex.exchange(_tripleBufferIndex);
    }

    // Determine version number of data in the triple buffer
    bool tripleBufferHasData = _tripleBufferIndex->_isValid;

    // Determine version number of data in the queue
    assert(_sharedState->_fullBufferQueue.read_available() > 0);   // guaranteed by our logic
    TransferFuture::PlainFutureType future = _sharedState->_fullBufferQueue.front();
    auto status = future.wait_for(boost::chrono::duration<int, boost::centi>(0));
    bool queueHasData = (status != boost::future_status::timeout);

    // If postRead() has been called, there should be new data somewhere
    assert(queueHasData || tripleBufferHasData);

    // Determine whether to use the data from the queue or from the triple buffer. If in both places data is present,
    // use the older data (by the version number).
    bool useTripleBuffer;
    if(queueHasData && !tripleBufferHasData) {
      // only data in queue present
      useTripleBuffer = false;
    }
    else if(!queueHasData && tripleBufferHasData) {
      // only data in atomic triple buffer present
      useTripleBuffer = true;
    }
    else {  // queueHasData && tripleBufferHasData -> see the assert above
      uint64_t tripleBufferVersion = _tripleBufferIndex->_internalVersionNumber;
      uint64_t queueVersion = static_cast<Buffer*>(future.get())->_internalVersionNumber;
      if(tripleBufferVersion < queueVersion) {
        useTripleBuffer = true;
      }
      else {
        useTripleBuffer = false;
      }
    }

    // perform the determined action
    if(!useTripleBuffer) {
      // remove future from queue, since we only obtained it through front() so far
      _sharedState->_fullBufferQueue.pop();
      // get buffer from future
      TransferFuture::Data *nextBuffer = future.get();
      // put old buffer back on empty buffer queue and update current index
      _sharedState->_emptyBufferQueue.push(_currentIndex);
      _currentIndex = static_cast<Buffer*>(nextBuffer);   // static_cast should be ok, since we control the queue
    }
    else {
      Buffer *nextBuffer = _tripleBufferIndex;
      // put old buffer back into triple buffer (after invalidating int) and update current index
      _currentIndex->_isValid = false;
      _tripleBufferIndex = _currentIndex;
      _currentIndex = nextBuffer;
    }

    // swap data out of the queue buffer
    mtca4u::NDRegisterAccessor<T>::buffer_2D[0].swap( _currentIndex->value );

  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::write(
      ChimeraTK::VersionNumber versionNumber) {
    return writeInternal(
        _timeStampSource ?
            _timeStampSource->getCurrentTimeStamp() : TimeStamp::currentTime(),
        versionNumber, true);
  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::writeDestructively(
      ChimeraTK::VersionNumber versionNumber) {
    if (!_maySendDestructively) {
      throw std::runtime_error(
          "This process variable must not be sent destructively because the corresponding flag has not been set.");
    }
    return writeInternal(
        _timeStampSource ?
            _timeStampSource->getCurrentTimeStamp() : TimeStamp::currentTime(),
        versionNumber, false);
  }

  /*********************************************************************************************************************/

    template<class T>
    bool UnidirectionalProcessArray<T>::writeDestructively(
        ChimeraTK::TimeStamp timeStamp,
        ChimeraTK::VersionNumber versionNumber) {
      if (!_maySendDestructively) {
        throw std::runtime_error(
            "This process variable must not be sent destructively because the corresponding flag has not been set.");
      }
      return writeInternal(timeStamp, versionNumber,
          false);
    }

/*********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::setPersistentDataStorage(
      boost::shared_ptr<PersistentDataStorage> storage) {
    if(!this->isWriteable()) return;
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

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::writeInternal(
      TimeStamp newTimeStamp, VersionNumber newVersionNumber, bool shouldCopy) {
    if(!this->isWriteable()) {
      throw std::logic_error(
          "Send operation is only allowed for a sender process variable. Variable name: "+this->getName());
    }
    // We have to check that the vector that we currently own still has the
    // right size. Otherwise, the code using the receiver might get into
    // trouble when it suddenly experiences a vector of the wrong size.
    if(mtca4u::NDRegisterAccessor<T>::buffer_2D[0].size() != _vectorSize) {
      throw std::runtime_error(
          "Cannot run receive operation because the size of the vector belonging to the current buffer has been "
          "modified. Variable name: "+this->getName());
    }
    // A version should never be send with a version number that is equal to or
    // even less than the last version number used. Such an attempt indicates
    // that there is a problem in the logic attempting the write operation.
    if (newVersionNumber <= getVersionNumber()) {
      throw std::runtime_error(
          "The version number passed to write is less than or equal to the last version number used.");
    }

    // First update the persistent data storage, if any was associated. This cannot be done after sending, since the
    // value might no longer be available within this instance.
    if(_persistentDataStorage) {
      _persistentDataStorage->updateValue(_persistentDataStorageID, mtca4u::NDRegisterAccessor<T>::buffer_2D[0]);
    }

    // Before sending the value, we have to set the associated time-stamp.
    _currentIndex->timeStamp = newTimeStamp;

    // set the version numbers
    _currentIndex->_versionNumber = newVersionNumber;
    _currentIndex->_internalVersionNumber = _lastInternalVersionNumber;
    ++_lastInternalVersionNumber;

    // flag current data as valid
    _currentIndex->_isValid = true;

    // set the data by copying or swapping
    if(shouldCopy) {
      _currentIndex->value = mtca4u::NDRegisterAccessor<T>::buffer_2D[0];
    }
    else {
      _currentIndex->value.swap( mtca4u::NDRegisterAccessor<T>::buffer_2D[0] );
    }
    // send the data to the queue by first obtaining a new empty buffer from the empty buffer queue
    Buffer *nextIndex;
    bool lostData = false;
    if(_sharedState->_emptyBufferQueue.pop(nextIndex)) {
      // Create a new promise first and push its future to the full buffer queue before fulfilling the current promise.
      // This makes sure that always at least one unfulfilled future is available for the receier to wait on.
      TransferFuture::PromiseType nextPromise;
      _sharedState->_fullBufferQueue.push(nextPromise.get_future());
      _sharedState->_notificationPromise.set_value(_currentIndex);
      _sharedState->_notificationPromise = std::move(nextPromise);
    }
    else {
      // No empty buffer available: write to special atomic triple buffer
      nextIndex = _tripleBufferIndex;
      _tripleBufferIndex = _currentIndex;
      _tripleBufferIndex = _sharedState->_tripleBufferIndex.exchange(_tripleBufferIndex);
      // if the buffer swapped out of the shared state contained valid data, we have now lost that data
      if(_tripleBufferIndex->_isValid) {
        lostData = true;
        // mark as invalid so we can always just swap it with the shared state
        _tripleBufferIndex->_isValid = false;
      }
    }

    // change current index to the new empty buffer
    if(shouldCopy) {
      nextIndex->timeStamp = newTimeStamp;
      nextIndex->_versionNumber = newVersionNumber;
    }
    _currentIndex = nextIndex;

    // notify send notification listener, if present
    if(_sendNotificationListener) {
      _sendNotificationListener->notify(_receiver);
    }

    return lostData;
  }

/*********************************************************************************************************************/
/*** Implementations of non-member functions below this line *********************************************************/
/*********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const mtca4u::RegisterPath & name, const std::string &unit, const std::string &description, T initialValue,
      std::size_t numberOfBuffers, bool maySendDestructively,
      TimeStampSource::SharedPtr timeStampSource,
      ProcessVariableListener::SharedPtr sendNotificationListener) {
    typename boost::shared_ptr<UnidirectionalProcessArray<T> > receiver =
        boost::make_shared<UnidirectionalProcessArray<T> >(
            ProcessArray<T>::RECEIVER, name, unit, description,
            std::vector<T>(size, initialValue), numberOfBuffers);
    typename UnidirectionalProcessArray<T>::SharedPtr sender =
        boost::make_shared<UnidirectionalProcessArray<T> >(
            ProcessArray<T>::SENDER, maySendDestructively, timeStampSource,
            sendNotificationListener, receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

/*********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      const std::vector<T>& initialValue, const mtca4u::RegisterPath & name, const std::string &unit, const std::string &description,
      std::size_t numberOfBuffers, bool maySendDestructively,
      TimeStampSource::SharedPtr timeStampSource,
      ProcessVariableListener::SharedPtr sendNotificationListener) {
    typename boost::shared_ptr<UnidirectionalProcessArray<T> > receiver =
        boost::make_shared<UnidirectionalProcessArray<T> >(
            ProcessArray<T>::RECEIVER, name, unit, description, initialValue,
            numberOfBuffers);
    typename UnidirectionalProcessArray<T>::SharedPtr sender =
        boost::make_shared<UnidirectionalProcessArray<T> >(
            ProcessArray<T>::SENDER, maySendDestructively, timeStampSource,
            sendNotificationListener, receiver);
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(sender, receiver);
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H
