#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_BIDIRECTIONAL_PROCESS_ARRAY_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_BIDIRECTIONAL_PROCESS_ARRAY_H

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

#include <mtca4u/ExperimentalFeatures.h>

#include "ProcessArray.h"
#include "ProcessVariableListener.h"
#include "TimeStampSource.h"
#include "PersistentDataStorage.h"

namespace ChimeraTK {

  /**
   * Creates a bidirectional synchronized process array. A bidirectional
   * synchronized process array works as a pair of two process arrays, where
   * changes written on either side can be read on the respective other side.
   *
   * The synchronization is implemented in a thread-safe manner, so that the
   * two coupled process arrays can safely be used by different threads without
   * a mutex. However, both process arrays each only support a single thread.
   * This means that access to each process array have to be protected with a
   * mutex if more than one thread wants to access it.
   *
   * The number of buffers specifies how many buffers are allocated for the
   * send / receive mechanism. The minimum number (and default) is two. This
   * number specifies, how many times ProcessArray::write() can be
   * called in a row without losing data when readNonBlocking() is not called in
   * between. The specified number of buffers is used for each direction.
   *
   * The specified time-stamp sources are used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used. The first specified time-stamp
   * source is used when sending values from the first returned process array
   * and the second specified time-stamp source is used when sending values from
   * the second returned process array.
   *
   * The optional send-notification listeners are notified every time one of the
   * process array's ProcessArray::write() method is called. These
   * listeners can be used to queue a request for the receiver's
   * readNonBlocking() method to be called.  The process variable passed to the
   * listener is the receiver and not the sender. The first listener is notified
   * when the first of the returned process arrays is sent (and is passed the
   * second of the returned process arrays). The second listener is notified
   * when the second of the returned process arrays is sent (and is passed the
   * first of the returned process arrays).
   *
   * The specified initial value is used for all the elements of the array.
   *
   * Of the two returned process arrays, only the first one can take an
   * optional persistent data storage. Trying to set a persistent data storage
   * on the first one results in an exception.
   *
   * Note: biderectional process arrays are still experimental, so experimental
   * features must be enable when using this function (see
   * ChimeraTK::ExperimentalFeatures in DeviceAccess).
   */
  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createBidirectionalSynchronizedProcessArray(
      std::size_t size, const mtca4u::RegisterPath & name = "",
      const std::string &unit = "", const std::string &description = "",
      T initialValue = T(), std::size_t numberOfBuffers = 2,
      TimeStampSource::SharedPtr timeStampSource1 =
          TimeStampSource::SharedPtr(),
      TimeStampSource::SharedPtr timeStampSource2 =
          TimeStampSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener1 =
          ProcessVariableListener::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener2 =
          ProcessVariableListener::SharedPtr());

  /**
   * Creates a bidirectional synchronized process array. A bidirectional
   * synchronized process array works as a pair of two process arrays, where
   * changes written on either side can be read on the respective other side.
   *
   * The synchronization is implemented in a thread-safe manner, so that the
   * two coupled process arrays can safely be used by different threads without
   * a mutex. However, both process arrays each only support a single thread.
   * This means that access to each process array have to be protected with a
   * mutex if more than one thread wants to access it.
   *
   * The number of buffers specifies how many buffers are allocated for the
   * send / receive mechanism. The minimum number (and default) is two. This
   * number specifies, how many times ProcessArray::write() can be
   * called in a row without losing data when readNonBlocking() is not called in
   * between. The specified number of buffers is used for each direction.
   *
   * The specified time-stamp source is used for determining the current time
   * when sending a value. The receiver will be updated with this time stamp
   * when receiving the value. If no time-stamp source is specified, the current
   * system-time when the value is sent is used.
   *
   * The optional send-notification listeners are notified every time one of the
   * process array's ProcessArray::write() method is called. These
   * listeners can be used to queue a request for the receiver's
   * readNonBlocking() method to be called.  The process variable passed to the
   * listener is the receiver and not the sender. The first listener is notified
   * when the first of the returned process arrays is sent (and is passed the
   * second of the returned process arrays). The second listener is notified
   * when the second of the returned process arrays is sent (and is passed the
   * first of the returned process arrays).
   *
   * The array's size is set to the number of elements stored in the vector
   * provided for initialization and all elements are initialized with the
   * values provided by this vector.
   *
   * Of the two returned process arrays, only the first one can take an
   * optional persistent data storage. Trying to set a persistent data storage
   * on the first one results in an exception.
   *
   * Note: biderectional process arrays are still experimental, so experimental
   * features must be enable when using this function (see
   * ChimeraTK::ExperimentalFeatures in DeviceAccess).
   */
  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createBidirectionalSynchronizedProcessArray(
      const std::vector<T>& initialValue,
      const mtca4u::RegisterPath & name = "", const std::string &unit = "",
      const std::string &description = "", std::size_t numberOfBuffers = 2,
      TimeStampSource::SharedPtr timeStampSource1 =
          TimeStampSource::SharedPtr(),
      TimeStampSource::SharedPtr timeStampSource2 =
          TimeStampSource::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener1 =
          ProcessVariableListener::SharedPtr(),
      ProcessVariableListener::SharedPtr sendNotificationListener2 =
          ProcessVariableListener::SharedPtr());

  /**
   * Implementation of the process array that transports data in both
   * directions.
   *
   * This class is not thread-safe and should only be used from a single thread.
   *
   * Note: biderectional process arrays are still experimental, so experimental
   * features must be enable when using this class (see
   * ChimeraTK::ExperimentalFeatures in DeviceAccess).
   */
  template<class T>
  class BidirectionalProcessArray : public ProcessArray<T> {

  public:

    /**
     * Type alias for a shared pointer to this type.
     */
    typedef boost::shared_ptr<BidirectionalProcessArray> SharedPtr;

    /* The two createBidirectionalSynchronizedProcessArray functions have to be
       friends so that they can access the private _partner field. */
    friend std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> createBidirectionalSynchronizedProcessArray<>(
        const std::vector<T>&, const mtca4u::RegisterPath &,
        const std::string &, const std::string &, std::size_t,
        TimeStampSource::SharedPtr, TimeStampSource::SharedPtr,
        ProcessVariableListener::SharedPtr, ProcessVariableListener::SharedPtr);
    friend std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> createBidirectionalSynchronizedProcessArray<>(
        std::size_t, const mtca4u::RegisterPath &, const std::string &,
        const std::string &, T, std::size_t, TimeStampSource::SharedPtr,
        TimeStampSource::SharedPtr, ProcessVariableListener::SharedPtr,
        ProcessVariableListener::SharedPtr);

    /**
     * Creates a bidirectional process array that uses the passed process arrays
     * for sending and receiving data. This constructor should not be used
     * directly. Instead, one should use one of the
     * createBidirectionalSynchronizedProcessArray functions.
     */
    BidirectionalProcessArray(std::size_t uniqueId,
        const mtca4u::RegisterPath& name, const std::string &unit,
        const std::string &description, bool allowPersistentDataStorage,
        typename ProcessArray<T>::SharedPtr receiver,
        typename ProcessArray<T>::SharedPtr sender,
        TimeStampSource::SharedPtr timeStampSource,
        ProcessVariableListener::SharedPtr sendNotificationListener,
        TimeStamp initialTimeStamp, VersionNumber initialVersionNumber);

    TimeStamp getTimeStamp() const override {
      return _timeStamp;
    }

    VersionNumber getVersionNumber() const override {
      return _versionNumber;
    }

    void doReadTransfer() override;

    bool doReadTransferNonBlocking() override;

    bool doReadTransferLatest() override;

    mtca4u::TransferFuture readAsync() override;

    void postRead() override;

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
     * Throws an exception if this process variable is not a sender or if this
     * process variable does not allow destructive sending.
     */
    bool writeDestructively(ChimeraTK::VersionNumber versionNumber={}) override; /// @todo FIXME this function must be present in TransferElement already!

    void setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) override ;

    /**
     * Returns a unique ID of this process variable, which will be indentical
     * for the receiver and sender side of the same variable but different for
     * any other process variable within the same process. The unique ID will
     * not be  persistent across executions of the process. */
    size_t getUniqueId() const override {
      return _uniqueId;
    }

    bool asyncTransferActive() override {
      return _receiver->asyncTransferActive();
    }

    void clearAsyncTransferActive() override {
      _receiver->clearAsyncTransferActive();
    }

  private:

    /**
     * Flag indicating whether this process array may be associated with a
     * persistent data storage. Usually, on the control-system side of a process
     * array should have a persistent data storage. This is critical for
     * bi-directional process arrays because we cannot have both sides compete
     * for updating the persistent data storage.
     */
    bool _allowPersistentDataStorage;

    /**
     * Other process array that belongs to the pair of process array. Due to the
     * obvious circular dependency, this field is not initialized by the
     * constructor. Instead, it is set by
     * createBidirectionalSynchronizedProcessArray after constructing both
     * instances of a pair. For the same reason, we have to use a weak pointer
     * instead of a shared pointer.
     */
    typename boost::weak_ptr<ProcessArray<T>> _partner;

    /**
     * Persistent data storage which needs to be informed when the process
     * variable is sent.
     */
    boost::shared_ptr<PersistentDataStorage> _persistentDataStorage;

    /**
     * Variable ID for the persistent data storage.
     */
    size_t _persistentDataStorageID{0};

    /**
     * Process array from which we receive values. When this process array is
     * read, we actually read from the receiver.
     */
    typename ProcessArray<T>::SharedPtr _receiver;

    /**
     * Process array to which send values. When this process array is written,
     * we actually write to the sender.
     */
    typename UnidirectionalProcessArray<T>::SharedPtr _sender;

    /**
     * Listener that is notified when this process array is sent.
     */
    ProcessVariableListener::SharedPtr _sendNotificationListener;

    /**
     * Current time stamp. Depending on whether the last operation was a read or
     * write, the time stamp from the receiver or the sender is supposed to be
     * used. This is why we store a separate copy of the time stamp.
     */
    TimeStamp _timeStamp;

    /**
     * Time stamp source used when this process array is written.
     */
    TimeStampSource::SharedPtr _timeStampSource;

    /**
     * Unique ID identifying the pair to which this process array belongs.
     */
    std::size_t _uniqueId;

    /**
     * Current version number. Depending on whether the last operation was a
     * read or write, the version number from the receiver or sender is supposed
     * to be used. This is why we store a separate copy of the version number.
     */
    VersionNumber _versionNumber;

  };


/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T>
  BidirectionalProcessArray<T>::BidirectionalProcessArray(std::size_t uniqueId,
      const mtca4u::RegisterPath &name, const std::string &unit,
      const std::string &description, bool allowPersistentDataStorage,
      typename ProcessArray<T>::SharedPtr receiver,
      typename ProcessArray<T>::SharedPtr sender,
      TimeStampSource::SharedPtr timeStampSource,
      ProcessVariableListener::SharedPtr sendNotificationListener,
      TimeStamp initialTimeStamp, VersionNumber initialVersionNumber)
  : ProcessArray<T>(ProcessArray<T>::SENDER_RECEIVER, name, unit, description),
    _allowPersistentDataStorage(allowPersistentDataStorage),
    _receiver(receiver),
    _sender(boost::dynamic_pointer_cast<UnidirectionalProcessArray<T>>(sender)),
    _sendNotificationListener(sendNotificationListener),
    _timeStamp(initialTimeStamp),
    _timeStampSource(timeStampSource),
    _uniqueId(uniqueId),
    _versionNumber(initialVersionNumber)
  {
    ChimeraTK::ExperimentalFeatures::check("BidirectionalProcessArray");
    // If the passed sender was not null but the class variable is, the dynamic
    // cast failed.
    if (sender && !_sender) {
      throw std::runtime_error(
          "The passed sender must be an instance of UnidirectionalProcessArray.");
    }
    if (!receiver->isReadable()) {
      throw std::runtime_error("The passed receiver must be readable.");
    }
    if (!sender->isWriteable()) {
      throw std::runtime_error("The passed sender must be writable.");
    }
    // Allocate and initialize the buffer of the base class we copy the value
    // from the receiver because the calling code should already have take care
    // of initializing that value.
    mtca4u::NDRegisterAccessor<T>::buffer_2D.resize(1);
    mtca4u::NDRegisterAccessor<T>::buffer_2D[0] = receiver->accessChannel(0);
  }

/*********************************************************************************************************************/

  template<class T>
  void BidirectionalProcessArray<T>::doReadTransfer() {
    _receiver->doReadTransfer();
  }

/*********************************************************************************************************************/

  template<class T>
  bool BidirectionalProcessArray<T>::doReadTransferNonBlocking() {
    return _receiver->doReadTransferNonBlocking();
  }

/*********************************************************************************************************************/

  template<class T>
  bool BidirectionalProcessArray<T>::doReadTransferLatest() {
    return _receiver->doReadTransferLatest();
  }

/*********************************************************************************************************************/

  template<class T>
  mtca4u::TransferFuture BidirectionalProcessArray<T>::readAsync() {
    return {_receiver->readAsync(), this};
  }

/*********************************************************************************************************************/

  template<class T>
  void BidirectionalProcessArray<T>::postRead() {
    _receiver->postRead();
    // We only update the current value (stored in the sender) when the version
    // number of the data that we received is greater than the current version
    // number. This ensures that old updates (that might arrive late due to the
    // asynchronous nature of the transfer logic) do not overwrite newer values
    // and also helps to ensure that we do not get a feedback loop where two (or
    // more) bidirectional process variables "play ping-pong" (see issue #2 for
    // the full discussion).
    if (_receiver->getVersionNumber() > _versionNumber) {
      this->accessChannel(0).swap(_receiver->accessChannel(0));
      // After receiving, our new time stamp and version number are the ones
      // that we got from the receiver.
      _timeStamp = _receiver->getTimeStamp();
      _versionNumber = _receiver->getVersionNumber();
      // If we have a persistent data-storage, we have to update it. We have to
      // do this because a (new) value received from the other side should be
      // treated like a value sent by this side.
      if (_persistentDataStorage) {
        _persistentDataStorage->updateValue(_persistentDataStorageID,
            _sender->accessChannel(0));
      }
    }
  }

/*********************************************************************************************************************/

  template<class T>
  bool BidirectionalProcessArray<T>::doWriteTransfer(
      ChimeraTK::VersionNumber versionNumber) {
    // We have to copy our current value to the sender. We cannot swap it
    // because this would mean that we would lose the current value.
    _sender->accessChannel(0) = this->accessChannel(0);
    // We need a new time stamp.
    TimeStamp newTimeStamp =
        _timeStampSource ?
            _timeStampSource->getCurrentTimeStamp() : TimeStamp::currentTime();
    // We already copied the value, so the sender does not have to copy the
    // value again.
    bool lostData = _sender->writeDestructively(newTimeStamp, versionNumber);
    // After sending the new value, our current time stamp and version number
    // are the one from the sender.
    _timeStamp = newTimeStamp;
    _versionNumber = versionNumber;
    // If we have a persistent data-storage, we have to update it.
    if (_persistentDataStorage) {
      _persistentDataStorage->updateValue(_persistentDataStorageID,
          _sender->accessChannel(0));
    }
    if (_sendNotificationListener) {
      typename ProcessArray<T>::SharedPtr partner = _partner.lock();
      if (partner) {
        _sendNotificationListener->notify(partner);
      }
    }
    return lostData;
  }

/*********************************************************************************************************************/

  template<class T>
  bool BidirectionalProcessArray<T>::writeDestructively(
      ChimeraTK::VersionNumber) {
    throw std::runtime_error(
        "This process variable must not be sent destructively because it is a bidirectional process variable.");
  }

/*********************************************************************************************************************/

  template<class T>
  void BidirectionalProcessArray<T>::setPersistentDataStorage(
      boost::shared_ptr<PersistentDataStorage> storage) {
    if (_allowPersistentDataStorage) {
      throw std::logic_error(
          "This device side of a process array must not be associated with a persistent data storage.");
    }
    bool sendInitialValue = false;
    if (!_persistentDataStorage) {
      sendInitialValue = true;
    }
    _persistentDataStorage = storage;
    _persistentDataStorageID = _persistentDataStorage->registerVariable<T>(
        mtca4u::TransferElement::getName(),
        mtca4u::NDRegisterAccessor<T>::getNumberOfSamples());
    if (sendInitialValue) {
      mtca4u::NDRegisterAccessor<T>::buffer_2D[0] =
          _persistentDataStorage->retrieveValue<T>(_persistentDataStorageID);
      doWriteTransfer();
    }
  }

/*********************************************************************************************************************/
/*** Implementations of non-member functions below this line *********************************************************/
/*********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createBidirectionalSynchronizedProcessArray(
      std::size_t size, const mtca4u::RegisterPath & name,
      const std::string &unit, const std::string &description, T initialValue,
      std::size_t numberOfBuffers, TimeStampSource::SharedPtr timeStampSource1,
      TimeStampSource::SharedPtr timeStampSource2,
      ProcessVariableListener::SharedPtr sendNotificationListener1,
      ProcessVariableListener::SharedPtr sendNotificationListener2) {
    auto senderReceiver1 = createSynchronizedProcessArray(size, name, unit,
        description, initialValue, numberOfBuffers, true);
    auto senderReceiver2 = createSynchronizedProcessArray(size, name, unit,
        description, initialValue, numberOfBuffers, true);
    // We create a default-constructed time-stamp and version number because we
    // want to be sure that initially both sides have the same time stamp and
    // version number.
    TimeStamp timeStamp;
    VersionNumber versionNumber;
    // The unique ID has to be the same for both process arrays that belong to
    // the pair, but it has to be different from the one used by all other
    // process arrays. We simply use the unique ID used by the second
    // sender / receiver pair.
    std::size_t uniqueId = senderReceiver2.first->getUniqueId();
    typename boost::shared_ptr<BidirectionalProcessArray<T> > pv1 =
        boost::make_shared<BidirectionalProcessArray<T> >(uniqueId, name, unit,
            description, true, senderReceiver2.second, senderReceiver1.first,
            timeStampSource1, sendNotificationListener1, timeStamp,
            versionNumber);
    typename boost::shared_ptr<BidirectionalProcessArray<T> > pv2 =
        boost::make_shared<BidirectionalProcessArray<T> >(uniqueId, name, unit,
            description, false, senderReceiver1.second, senderReceiver2.first,
            timeStampSource2, sendNotificationListener2, timeStamp,
            versionNumber);
    pv1->_partner = pv2;
    pv2->_partner = pv1;
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(pv1, pv2);
  }

/*********************************************************************************************************************/

  template<class T>
  typename std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> createBidirectionalSynchronizedProcessArray(
      const std::vector<T>& initialValue, const mtca4u::RegisterPath & name,
      const std::string &unit, const std::string &description,
      std::size_t numberOfBuffers, TimeStampSource::SharedPtr timeStampSource1,
      TimeStampSource::SharedPtr timeStampSource2,
      ProcessVariableListener::SharedPtr sendNotificationListener1,
      ProcessVariableListener::SharedPtr sendNotificationListener2) {
    auto senderReceiver1 = createSynchronizedProcessArray(initialValue, name,
        unit, description, numberOfBuffers, true, timeStampSource1);
    auto senderReceiver2 = createSynchronizedProcessArray(initialValue, name,
        unit, description, numberOfBuffers, true, timeStampSource2);
    // We create a default-constructed time-stamp and version number because we
    // want to be sure that initially both sides have the same time stamp and
    // version number.
    TimeStamp timeStamp;
    VersionNumber versionNumber;
    // The unique ID has to be the same for both process arrays that belong to
    // the pair, but it has to be different from the one used by all other
    // process arrays. We simply use the unique ID used by the second
    // sender / receiver pair.
    std::size_t uniqueId = senderReceiver2.first->getUniqueId();
    typename boost::shared_ptr<BidirectionalProcessArray<T> > pv1 =
        boost::make_shared<BidirectionalProcessArray<T> >(uniqueId, name, unit,
            description, true, senderReceiver2.second, senderReceiver1.first,
            timeStampSource1, sendNotificationListener1, timeStamp,
            versionNumber);
    typename boost::shared_ptr<BidirectionalProcessArray<T> > pv2 =
        boost::make_shared<BidirectionalProcessArray<T> >(uniqueId, name, unit,
            description, false, senderReceiver1.second, senderReceiver2.first,
            timeStampSource2, sendNotificationListener2, timeStamp,
            versionNumber);
    pv1->_partner = pv2;
    pv2->_partner = pv1;
    return std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr>(pv1, pv2);
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_BIDIRECTIONAL_PROCESS_ARRAY_H
