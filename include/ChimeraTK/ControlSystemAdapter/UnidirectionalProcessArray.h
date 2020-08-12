#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H

#include <limits>
#include <stdexcept>
#include <thread>
#include <typeinfo>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <ChimeraTK/VersionNumber.h>

#include "PersistentDataStorage.h"
#include "ProcessArray.h"

namespace ChimeraTK {

  namespace detail {

    /** Global flag if thread safety check shall performed on each read/write. */
    extern std::atomic<bool> processArrayEnableThreadSafetyCheck; // std::atomic<bool> defaults to false

  } // namespace detail

  /** Globally enable or disable the thread safety check on each read/write. This
   * will throw an assertion if the thread id has been changed since the last
   * read/write operation which has been executed with the safety check enabled.
   * This will only have an effect if debug compiler flags are enabled. */
  void setEnableProcessArrayThreadSafetyCheck(bool enable);

  /**
   * Implementation of the process array that transports data in a single
   * direction. This implementation is used for both sides (sender and
   * receiver).
   *
   * @note while this class is mostly following the [TransferElement
   * specification](https://chimeratk.github.io/DeviceAccess/master/spec__transfer_element.html)
   * it deviates in its behaviour at one point. When created without
   * AccessMode::wait_for_new_data, all read operations will block until an initial
   * value was seen. This follows the behaviour specified in the [initial value propagation
   * specification](https://chimeratk.github.io/ApplicationCore/master/spec_initial_value_propagation.html)
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
    UnidirectionalProcessArray(typename ProcessArray<T>::InstanceType instanceType, const ChimeraTK::RegisterPath& name,
        const std::string& unit, const std::string& description, const std::vector<T>& initialValue,
        std::size_t numberOfBuffers, const AccessModeFlags& flags);

    /**
     * Creates a process array that acts as a sender. A sender is intended
     * intended to work as a tandem with a receiver and send set values to
     * it. It uses the buffers and queues that have been created by the
     * receiver.
     */
    UnidirectionalProcessArray(typename ProcessArray<T>::InstanceType instanceType,
        UnidirectionalProcessArray::SharedPtr receiver, const AccessModeFlags& flags);

    void doReadTransferSynchronously() override;

    void doPostRead(ChimeraTK::TransferType type, bool hasNewData) override;

    void doPreRead(ChimeraTK::TransferType type) override;

    void doPreWrite(ChimeraTK::TransferType type, VersionNumber versionNumber) override;

    void doPostWrite(ChimeraTK::TransferType type, VersionNumber versionNumber) override; // Workaround

    bool doWriteTransfer(ChimeraTK::VersionNumber versionNumber) override;

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
    bool doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) override;

    void setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) override;

    /** Return a unique ID of this process variable, which will be indentical for
     * the receiver and sender side of the same variable but different for any
     * other process variable within the same process. The unique ID will not be
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

    void interrupt() override { TransferElement::interrupt_impl(_sharedState._queue); }

   private:
    /**
     *  Type for the individual buffers. Each buffer stores a vector, the version
     * number and the time stamp. The type is swappable by the default
     * implemenation of std::swap since both the move constructor and the move
     * assignment operator are implemented. This helps to avoid unnecessary memory
     * allocations when transported in a cppext::future_queue.
     */
    struct Buffer {
      Buffer(const std::vector<T>& initialValue) : _value(initialValue) {}

      Buffer(size_t size) : _value(size) {}

      Buffer() {}

      Buffer(Buffer&& other)
      : _value(std::move(other._value)), _versionNumber(other._versionNumber), _dataValidity(other._dataValidity) {}

      Buffer& operator=(Buffer&& other) {
        _value = std::move(other._value);
        _versionNumber = other._versionNumber;
        _dataValidity = other._dataValidity;
        return *this;
      }

      /** The actual data contained in this buffer. */
      std::vector<T> _value;

      /** Version number of this data */
      ChimeraTK::VersionNumber _versionNumber{nullptr};

      /** Whether or not the data in the bufer is considered valid */
      ChimeraTK::DataValidity _dataValidity{ChimeraTK::DataValidity::ok};
    };

    /**
     * Number of elements that each vector (and thus this array) has.
     */
    std::size_t _vectorSize;

    /**
     * The state shared between the sender and the receiver
     */
    struct SharedState {
      SharedState(size_t numberOfBuffers, size_t bufferLength) : _queue(numberOfBuffers) {
        // fill the internal buffers of the queue
        for(size_t i = 0; i < numberOfBuffers + 1; ++i) {
          Buffer b0(bufferLength);
          Buffer b1(bufferLength);
          _queue.push(std::move(b0));
          _queue.pop(b1); // here the buffer b1 gets swapped into the queue
        }
      }

      // Create copy of shared state. Sice the future_queue itself already
      // supports sharing and we have nothing else in our shared state, we do not
      // need to store our share state as a pointer but we can "copy" it and the
      // copies will stay linked.
      SharedState(const SharedState& other) : _queue(other._queue) {}

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
     * Workaround: Introduce this intermedate buffer due to failing testUnified, using content of buffer if
     * writeDestructively. This conflicts with spec: "Applications still are not allowed
     * to use the content of the application buffer after writeDestructively()."
     * In writeInternal ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0] was exchanged with _intermedateBuffer.
     */
    std::vector<T> _intermedateBuffer;

    /**
     * Pointer to the receiver associated with this sender. This field is only
     * used if this process variable represents a sender.
     */
    boost::shared_ptr<UnidirectionalProcessArray> _receiver;

    /**
     * Persistent data storage which needs to be informed when the process
     * variable is sent.
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
     * The return value will be true, if older data was overwritten during the
     * send operation, or otherwise false.
     */
    bool writeInternal(VersionNumber newVersionNumber, bool shouldCopy);

    /** Check thread safety. This function is used in various places inside an
     * assert(). */
    bool checkThreadSafety() {
      // Only perform check if enableThreadSafetyCheck is enabled
      if(!detail::processArrayEnableThreadSafetyCheck) return true;

      // If threadSafetyCheckLastId has already been filled, perform the check
      if(threadSafetyCheckInitialised) {
        return (threadSafetyCheckLastId == std::hash<std::thread::id>()(std::this_thread::get_id()));
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
    std::atomic<bool> threadSafetyCheckInitialised; // std::atomic<bool> defaults to false

    template<typename U>
    friend std::pair<typename ProcessArray<U>::SharedPtr, typename ProcessArray<U>::SharedPtr>
        createSynchronizedProcessArray(std::size_t size, const ChimeraTK::RegisterPath& name, const std::string& unit,
            const std::string& description, U initialValue, std::size_t numberOfBuffers, const AccessModeFlags& flags);

    template<typename U>
    friend std::pair<typename ProcessArray<U>::SharedPtr, typename ProcessArray<U>::SharedPtr>
        createSynchronizedProcessArray(const std::vector<U>& initialValue, const ChimeraTK::RegisterPath& name,
            const std::string& unit, const std::string& description, std::size_t numberOfBuffers,
            const AccessModeFlags& flags);

    template<typename U>
    friend class BidirectionalProcessArray;
  };

  /********************************************************************************************************************/
  /*** Non-member functions below this line ***************************************************************************/
  /********************************************************************************************************************/

  /**
   * Creates a synchronized process array. A synchronized process array works
   * as a pair of two process arrays, where the pair's first acts as a sender
   * and the second one acts as a receiver.
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
   * The specified time-stamp source is used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used.
   *
   * The specified initial value is used for all the elements of the array.
   */
  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      std::size_t size, const ChimeraTK::RegisterPath& name = "", const std::string& unit = "",
      const std::string& description = "", T initialValue = T(), std::size_t numberOfBuffers = 3,
      const AccessModeFlags& flags = {AccessMode::wait_for_new_data});

  /**
   * Creates a synchronized process array. A synchronized process array works
   * as a pair of two process arrays, where pair's first acts as a sender
   * and the second one acts as a receiver.
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
   * The specified time-stamp source is used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used.
   *
   * The array's size is set to the number of elements stored in the vector
   * provided for initialization and all elements are initialized with the
   * values provided by this vector.
   */
  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> createSynchronizedProcessArray(
      const std::vector<T>& initialValue, const ChimeraTK::RegisterPath& name = "", const std::string& unit = "",
      const std::string& description = "", std::size_t numberOfBuffers = 3,
      const AccessModeFlags& flags = {AccessMode::wait_for_new_data});

  /********************************************************************************************************************/
  /*** Implementations of member functions below this line ************************************************************/
  /********************************************************************************************************************/

  template<class T>
  UnidirectionalProcessArray<T>::UnidirectionalProcessArray(typename ProcessArray<T>::InstanceType instanceType,
      const ChimeraTK::RegisterPath& name, const std::string& unit, const std::string& description,
      const std::vector<T>& initialValue, std::size_t numberOfBuffers, const AccessModeFlags& flags)
  : ProcessArray<T>(instanceType, name, unit, description, flags), _vectorSize(initialValue.size()),
    _sharedState(numberOfBuffers, initialValue.size()), _localBuffer(initialValue) {
    TransferElement::_readQueue = _sharedState._queue.template then<void>(
        [this](Buffer& buf) { std::swap(_localBuffer, buf); }, std::launch::deferred);
    // allocate and initialise buffer of the base class
    ChimeraTK::NDRegisterAccessor<T>::buffer_2D.resize(1);
    ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0] = initialValue;
    // Workaround
    _intermedateBuffer.resize( ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].size() );
    // It would be better to do the validation before initializing, but this
    // would mean that we would have to initialize twice.
    if(!this->isReadable()) {
      throw ChimeraTK::logic_error("This constructor may only be used for a receiver process variable.");
    }
    // We need at least two buffers for the queue (so four buffers in total)
    // in order to guarantee that we never have to block.
    if(numberOfBuffers < 2) {
      throw ChimeraTK::logic_error("The number of buffers must be at least two.");
    }
    // We have to limit the number of buffers because we cannot allocate
    // more buffers than can be represented by size_t and the total number
    // of buffers is the specified number plus one. This extra buffer is
    // needed internally by the future_queue.
    if(numberOfBuffers > (std::numeric_limits<std::size_t>::max() - 1)) {
      throw ChimeraTK::logic_error("The number of buffers is too large.");
    }
  }

  /********************************************************************************************************************/

  template<class T>
  UnidirectionalProcessArray<T>::UnidirectionalProcessArray(typename ProcessArray<T>::InstanceType instanceType,
      UnidirectionalProcessArray::SharedPtr receiver, const AccessModeFlags& flags)
  : ProcessArray<T>(instanceType, receiver->getName(), receiver->getUnit(), receiver->getDescription(), flags),
    _vectorSize(receiver->_vectorSize), _sharedState(receiver->_sharedState),
    _localBuffer(receiver->_localBuffer._value), _receiver(receiver) {
    // It would be better to do the validation before initializing, but this
    // would mean that we would have to initialize twice.
    if(!this->isWriteable()) {
      throw ChimeraTK::logic_error("This constructor may only be used for a sender process variable.");
    }
    if(!receiver) {
      throw ChimeraTK::logic_error("The pointer to the receiver must not be null.");
    }
    if(!receiver->isReadable()) {
      throw ChimeraTK::logic_error("The pointer to the receiver must point to an "
                                   "instance that is actually a receiver.");
    }
    // allocate and initialise buffer of the base class
    ChimeraTK::NDRegisterAccessor<T>::buffer_2D.resize(1);
    ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0] = receiver->buffer_2D[0];
    // Workaround
    _intermedateBuffer.resize( ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].size() );
  }

  /********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::doPreRead(ChimeraTK::TransferType) {
    if(!this->isReadable()) {
      throw ChimeraTK::logic_error("Receive operation is only allowed for a receiver process variable.");
    }
  }

  /********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::doPreWrite(ChimeraTK::TransferType, VersionNumber) {
    if(!this->isWriteable()) {
      throw ChimeraTK::logic_error("Send operation is only allowed for a sender process variable.");
    }

    // We have to check that the vector that we currently own still has the
    // right size. Otherwise, the code using the receiver might get into
    // trouble when it suddenly experiences a vector of the wrong size.
    if(ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].size() != _vectorSize) {
      throw ChimeraTK::logic_error("Cannot run receive operation because the size of the vector belonging "
                                   "to the current buffer has been modified. Variable name: " +
          this->getName());
    }
    // Workaround
    assert(_intermedateBuffer.size() == ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].size());
    _intermedateBuffer.swap(ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0]);
  }

  /********************************************************************************************************************/
  // Workaround
  template<class T>
  void UnidirectionalProcessArray<T>::doPostWrite(ChimeraTK::TransferType type, VersionNumber) {
    if (type == ChimeraTK::TransferType::write) {
      assert(ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].size() == _intermedateBuffer.size());
      ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].swap(_intermedateBuffer);
    }
  }

  /********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::doReadTransferSynchronously() {
    assert(this->isReadable());

    // If without wait_for_new_data, make sure that there is an initial value
    // TODO: Link spec element
    if(TransferElement::getVersionNumber() == VersionNumber{nullptr}) {
      _sharedState._queue.pop_wait(_localBuffer);
    }

    // Empty queue, equivalent of readLatest()
    while(_sharedState._queue.pop(_localBuffer)) {
    }
  }

  /********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::doPostRead(ChimeraTK::TransferType, bool hasNewData) {
    assert(checkThreadSafety());
    if(hasNewData) {
      // We have to check that the vector that we currently own still has the
      // right size. Otherwise, the code using the sender might get into
      // trouble when it suddenly experiences a vector of the wrong size.
      assert(ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].size() == _localBuffer._value.size());

      if(this->_accessModeFlags.has(AccessMode::wait_for_new_data)) {
        // swap data out of the local buffer into the user buffer
        ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].swap(_localBuffer._value);
      }
      else {
        // We have to mimic synchronous mode. Here we have to copy here because there might be multiple reads, and
        // the reading code is allowed to swap out the user buffer, and has to get the correct value on the second read.
        ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0] = _localBuffer._value;
      }
      TransferElement::_versionNumber = _localBuffer._versionNumber;
      TransferElement::_dataValidity = _localBuffer._dataValidity;
    }
  }

  /********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::doWriteTransfer(ChimeraTK::VersionNumber versionNumber) {
    return writeInternal(versionNumber, true);
  }

  /********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::doWriteTransferDestructively(ChimeraTK::VersionNumber versionNumber) {
    return writeInternal(versionNumber, false);
  }

  /********************************************************************************************************************/

  template<class T>
  void UnidirectionalProcessArray<T>::setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) {
    if(!this->isWriteable()) return;
    bool sendInitialValue = false;
    if(!_persistentDataStorage) sendInitialValue = true;
    _persistentDataStorage = storage;
    _persistentDataStorageID = _persistentDataStorage->registerVariable<T>(
        ChimeraTK::TransferElement::getName(), ChimeraTK::NDRegisterAccessor<T>::getNumberOfSamples());
    if(sendInitialValue) {
      if(_persistentDataStorage->retrieveValue<T>(_persistentDataStorageID).size() ==
          ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0].size()) {
        ChimeraTK::NDRegisterAccessor<T>::buffer_2D[0] =
            _persistentDataStorage->retrieveValue<T>(_persistentDataStorageID);
      }
      this->write();
    }
  }

  /********************************************************************************************************************/

  template<class T>
  bool UnidirectionalProcessArray<T>::writeInternal(VersionNumber newVersionNumber, bool shouldCopy) {
    // thread safety check, if enabled (only active with debug flags enabled)
    assert(checkThreadSafety());

    assert(this->isWriteable());

    // First update the persistent data storage, if any was associated. This
    // cannot be done after sending, since the value might no longer be available
    // within this instance.
    if(_persistentDataStorage) {
      _persistentDataStorage->updateValue(_persistentDataStorageID, _intermedateBuffer);
    }

    // Set time stamp and version number
    _localBuffer._versionNumber = newVersionNumber;
    _localBuffer._dataValidity = TransferElement::dataValidity();

    // set the data by copying or swapping
    assert(_localBuffer._value.size() == _intermedateBuffer.size());
    if(shouldCopy) {
      _localBuffer._value = _intermedateBuffer;
    }
    else {
      _localBuffer._value.swap(_intermedateBuffer);
    }

    // send the data to the queue
    bool dataNotLost = _sharedState._queue.push_overwrite(std::move(_localBuffer));

    // if receiver does not have wait_for_new_data, do not return whether data has been lost (because conceptionally it
    // hasn't)
    if(!_receiver->getAccessModeFlags().has(AccessMode::wait_for_new_data)) {
      return false;
    }

    return !dataNotLost;
  }

  /********************************************************************************************************************/
  /*** Implementations of non-member functions below this line ********************************************************/
  /********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr>
      createSynchronizedProcessArray(std::size_t size, const ChimeraTK::RegisterPath& name, const std::string& unit,
          const std::string& description, T initialValue, std::size_t numberOfBuffers, const AccessModeFlags& flags) {
    auto receiver = boost::make_shared<UnidirectionalProcessArray<T>>(
        ProcessArray<T>::RECEIVER, name, unit, description, std::vector<T>(size, initialValue), numberOfBuffers, flags);
    auto sender = boost::make_shared<UnidirectionalProcessArray<T>>(ProcessArray<T>::SENDER, receiver, flags);

    // Receiving end has initially no valid data. Since we keep the sender at "ok", this will be overwritten once the
    // first real data arrives.
    receiver->_dataValidity = DataValidity::faulty;

    return {sender, receiver};
  }

  /********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr>
      createSynchronizedProcessArray(const std::vector<T>& initialValue, const ChimeraTK::RegisterPath& name,
          const std::string& unit, const std::string& description, std::size_t numberOfBuffers,
          const AccessModeFlags& flags) {
    auto receiver = boost::make_shared<UnidirectionalProcessArray<T>>(
        ProcessArray<T>::RECEIVER, name, unit, description, initialValue, numberOfBuffers, flags);
    auto sender = boost::make_shared<UnidirectionalProcessArray<T>>(ProcessArray<T>::SENDER, receiver, flags);

    // Receiving end has initially no valid data. Since we keep the sender at "ok", this will be overwritten once the
    // first real data arrives.
    receiver->_dataValidity = DataValidity::faulty;

    return {sender, receiver};
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_UNIDIRECTIONAL_PROCESS_ARRAY_H
