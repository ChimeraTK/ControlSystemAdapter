#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H

#include <vector>
#include <utility>
#include <limits>
#include <stdexcept>
#include <typeinfo>
#include <thread>

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

  namespace detail {

    /** Global flag if thread safety check shall performed on each read/write. */
    extern std::atomic<bool> processArrayEnableThreadSafetyCheck;   // std::atomic<bool> defaults to false

  }

  /** Globally enable or disable the thread safety check on each read/write. This will throw an assertion if the
    *  thread id has been changed since the last read/write operation which has been executed with the safety check
    *  enabled. This will only have an effect if debug compiler flags are enabled. */
  void setEnableProcessArrayThreadSafetyCheck(bool enable);

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
        return _localBuffer._timeStamp;
      }

      ChimeraTK::VersionNumber getVersionNumber() const override {
        return _localBuffer._versionNumber;
      }

      void doReadTransfer() override;

      bool doReadTransferNonBlocking() override;

      bool doReadTransferLatest() override;

      mtca4u::TransferFuture doReadTransferAsync() override;

      void doPostRead() override;

      bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber={}) override;

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
        // use pointer address of receiver end of the variable
        if(_receiver) {
          return reinterpret_cast<size_t>(_receiver.get());
        }
        else {
          return reinterpret_cast<size_t>(this);
        }
      }

    private:

      /**
      *  Type for the individual buffers. Each buffer stores a vector, the version number and the time stamp. The type
      *  is swappable by the default implemenation of std::swap since both the move constructor and the move assignment
      *  operator are implemented. This helps to avoid unnecessary memory allocations when transported in a
      *  cppext::future_queue.
      */
      struct Buffer {

        Buffer(const std::vector<T> &initialValue)
        : _value(initialValue) {}

        Buffer(size_t size)
        : _value(size) {}

        Buffer() {}

        Buffer(Buffer &&other)
        : _value(std::move(other._value)), _versionNumber(other._versionNumber), _timeStamp(other._timeStamp) {}

        Buffer& operator=(Buffer &&other) {
          _value = std::move(other._value);
          _versionNumber = other._versionNumber;
          _timeStamp = other._timeStamp;
          return *this;
        }

        /** The actual data contained in this buffer. */
        std::vector<T> _value;

        /** Version number of this data */
        ChimeraTK::VersionNumber _versionNumber;

        /** Time stamp associated with this buffer */
        TimeStamp _timeStamp;

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
      * The state shared between the sender and the receiver
      */
      struct SharedState {

        SharedState(size_t numberOfBuffers, size_t bufferLength)
        : _queue(numberOfBuffers)
        {
          // fill the internal buffers of the queue
          for(size_t i=0; i<numberOfBuffers+1; ++i) {
            Buffer b0(bufferLength);
            Buffer b1(bufferLength);
            _queue.push(std::move(b0));
            _queue.pop(b1);   // here the buffer b1 gets swapped into the queue
          }
        }

        // Create copy of shared state. Sice the future_queue itself already supports sharing and we have nothing else
        // in our shared state, we do not need to store our share state as a pointer but we can "copy" it and the copies
        // will stay linked.
        SharedState(const SharedState &other)
        : _queue(other._queue) {}

        /**
        * Queue of buffers transporting the actual values
        */
        cppext::future_queue<Buffer, cppext::SWAP_DATA> _queue;

      };
      SharedState _sharedState;

      /**
       * Local buffer of this end (receiving or sending) of the process variable
       */
      Buffer _localBuffer;

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
      bool writeInternal(TimeStamp newTimeStamp, VersionNumber newVersionNumber, bool shouldCopy);


      /** Check thread safety. This function is used in various places inside an assert(). */
      bool checkThreadSafety() {

        // Only perform check if enableThreadSafetyCheck is enabled
        if(!detail::processArrayEnableThreadSafetyCheck) return true;

        // If threadSafetyCheckLastId has already been filled, perform the check
        if(threadSafetyCheckInitialised) {
          return ( threadSafetyCheckLastId == std::hash<std::thread::id>()(std::this_thread::get_id()) );
        }
        else {
          // ThreadSafetyCheckLastId not yet filled: fill it and set the flag.
          threadSafetyCheckLastId = std::hash<std::thread::id>()(std::this_thread::get_id());
          threadSafetyCheckInitialised = true;
          return true;
        }
      }

      /** Hash of the last known thread id for the thread safety check */
      std::atomic<size_t> threadSafetyCheckLastId;

      /** Flag whether threadSafetyCheckLastId has been filled already */
      std::atomic<bool> threadSafetyCheckInitialised;     // std::atomic<bool> defaults to false

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
      const std::string &description = "", T initialValue = T(), std::size_t numberOfBuffers = 3,
      bool maySendDestructively = false, TimeStampSource::SharedPtr timeStampSource = TimeStampSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener = ProcessVariableListener::SharedPtr() );

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
   * be used to transfer values without copying but losing them on the senderdoWriteTransfer
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
      const std::string &description = "", std::size_t numberOfBuffers = 3, bool maySendDestructively = false,
      TimeStampSource::SharedPtr timeStampSource = TimeStampSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener = ProcessVariableListener::SharedPtr());

/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T>
  UnidirectionalProcessArray<T>::UnidirectionalProcessArray( typename ProcessArray<T>::InstanceType instanceType,
      const mtca4u::RegisterPath& name, const std::string &unit, const std::string &description,
      const std::vector<T>& initialValue, std::size_t numberOfBuffers )
  : ProcessArray<T>(instanceType, name, unit, description),
    _vectorSize(initialValue.size()),
    _maySendDestructively(false),
    _sharedState(numberOfBuffers, initialValue.size()),
    _localBuffer(initialValue)
  {
    mtca4u::ExperimentalFeatures::enable();
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
    // of buffers is the specified number plus one. This extra buffer is
    // needed internally by the future_queue.
    if (numberOfBuffers > (std::numeric_limits<std::size_t>::max() - 1)) {
      throw std::invalid_argument("The number of buffers is too large.");
    }
  }

/*********************************************************************************************************************/

  template<class T>
  UnidirectionalProcessArray<T>::UnidirectionalProcessArray( typename ProcessArray<T>::InstanceType instanceType,
      bool maySendDestructively, TimeStampSource::SharedPtr timeStampSource,
      ProcessVariableListener::SharedPtr sendNotificationListener, UnidirectionalProcessArray::SharedPtr receiver )
  : ProcessArray<T>(instanceType, receiver->getName(), receiver->getUnit(), receiver->getDescription()),
    _vectorSize(receiver->_vectorSize),
    _maySendDestructively(maySendDestructively),
    _sharedState(receiver->_sharedState),
    _localBuffer(receiver->_localBuffer._value),
    _receiver(receiver),
    _timeStampSource(timeStampSource),
    _sendNotificationListener(sendNotificationListener)
  {
    mtca4u::ExperimentalFeatures::enable();
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
  }

/*********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::doReadTransfer() {
    _sharedState._queue.pop_wait(_localBuffer);
    boost::this_thread::interruption_point();                         /// @todo probably redundant

  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::doReadTransferNonBlocking() {
    return _sharedState._queue.pop(_localBuffer);
  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::doReadTransferLatest() {

    if (!this->isReadable()) {
      throw std::logic_error("Receive operation is only allowed for a receiver process variable.");
    }

    // flag if at least one of the pops was successfull
    bool receivedData = false;

    // pop elements from the queue until it is empty
    while(_sharedState._queue.pop(_localBuffer)) receivedData = true;

    // return if we got new data
    return receivedData;
  }

/*********************************************************************************************************************/

  template<class T>
  mtca4u::TransferFuture UnidirectionalProcessArray<T>::doReadTransferAsync() {

    if (!this->isReadable()) {
      throw std::logic_error("Receive operation is only allowed for a receiver process variable.");
    }

    // return the future
    return TransferFuture(_sharedState._queue.template then<void>([this] (Buffer &b) {
      std::swap( _localBuffer, b );
    }, std::launch::deferred), this);

  }

/*********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::doPostRead() {
    assert(checkThreadSafety());

    // We have to check that the vector that we currently own still has the
    // right size. Otherwise, the code using the sender might get into
    // trouble when it suddenly experiences a vector of the wrong size.
    assert(mtca4u::NDRegisterAccessor<T>::buffer_2D[0].size() == _localBuffer._value.size());

    // swap data out of the queue buffer
    mtca4u::NDRegisterAccessor<T>::buffer_2D[0].swap( _localBuffer._value );

  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
    auto timestamp = _timeStampSource ? _timeStampSource->getCurrentTimeStamp() : TimeStamp::currentTime();
    return writeInternal(timestamp, versionNumber, true);
  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::writeDestructively(ChimeraTK::VersionNumber versionNumber) {
    if(!_maySendDestructively) {
      throw std::runtime_error("This process variable must not be sent destructively because the corresponding flag "
                               "has not been set.");
    }
    auto timestamp = _timeStampSource ? _timeStampSource->getCurrentTimeStamp() : TimeStamp::currentTime();
    return writeInternal(timestamp, versionNumber, false);
  }

  /*********************************************************************************************************************/

    template<class T>
    bool UnidirectionalProcessArray<T>::writeDestructively(ChimeraTK::TimeStamp timeStamp,
                                                           ChimeraTK::VersionNumber versionNumber) {
      if(!_maySendDestructively) {
        throw std::runtime_error("This process variable must not be sent destructively because the corresponding flag "
                                 "has not been set.");
      }
      return writeInternal(timeStamp, versionNumber, false);
    }

/*********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) {
    if(!this->isWriteable()) return;
    bool sendInitialValue = false;
    if(!_persistentDataStorage) sendInitialValue = true;
    _persistentDataStorage = storage;
    _persistentDataStorageID = _persistentDataStorage->registerVariable<T>(mtca4u::TransferElement::getName(),
                                  mtca4u::NDRegisterAccessor<T>::getNumberOfSamples());
    if(sendInitialValue) {
      if( _persistentDataStorage->retrieveValue<T>(_persistentDataStorageID).size() ==
          mtca4u::NDRegisterAccessor<T>::buffer_2D[0].size()                           ) {
        mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = _persistentDataStorage->retrieveValue<T>(_persistentDataStorageID);
        doWriteTransfer();
      }
    }
  }

/*********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::writeInternal(TimeStamp newTimeStamp, VersionNumber newVersionNumber,
                                                    bool shouldCopy) {

    // thread safety check, if enabled (only active with debug flags enabled)
    assert(checkThreadSafety());

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
    _localBuffer._timeStamp = newTimeStamp;

    // set the version numbers
    _localBuffer._versionNumber = newVersionNumber;

    // set the data by copying or swapping
    assert(_localBuffer._value.size() == mtca4u::NDRegisterAccessor<T>::buffer_2D[0].size() );
    if(shouldCopy) {
      _localBuffer._value = mtca4u::NDRegisterAccessor<T>::buffer_2D[0];
    }
    else {
      _localBuffer._value.swap( mtca4u::NDRegisterAccessor<T>::buffer_2D[0] );
    }

    // send the data to the queue
    bool lostData = _sharedState._queue.push_overwrite(std::move(_localBuffer));

    // change current index to the new empty buffer
    if(shouldCopy) {
      _localBuffer._timeStamp = newTimeStamp;
      _localBuffer._versionNumber = newVersionNumber;
    }

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
