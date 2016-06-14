#ifndef MTCA4U_PROCESS_ARRAY_IMPL_H
#define MTCA4U_PROCESS_ARRAY_IMPL_H

#include <limits>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "ProcessArray.h"
#include "ProcessVariableListener.h"

namespace mtca4u {
  namespace impl {

    /**
     * Implementation of the ProcessArray. This implementation is used for all
     * three use cases (sender, receiver, and stand-alone).
     */
    template<class T>
    class ProcessArrayImpl: public ProcessArray<T> {

//      // Allow the factory functions to create instances of this class.
//      friend typename ProcessArray<T>::SharedPtr mtca4u::createSimpleProcessArray<
//          T>(std::size_t, const std::string &, T);
//
//      friend typename ProcessArray<T>::SharedPtr mtca4u::createSimpleProcessArray<
//          T>(const std::vector<T>&, const std::string &);
//
//      friend typename std::pair<typename ProcessArray<T>::SharedPtr,
//          typename ProcessArray<T>::SharedPtr> mtca4u::createSynchronizedProcessArray<
//          T>(std::size_t, const std::string &, T, bool, std::size_t,
//          boost::shared_ptr<TimeStampSource>,
//          boost::shared_ptr<ProcessVariableListener>);
//
//      friend typename std::pair<typename ProcessArray<T>::SharedPtr,
//          typename ProcessArray<T>::SharedPtr> mtca4u::createSynchronizedProcessArray<
//          T>(const std::vector<T>&, const std::string &, bool, std::size_t,
//          boost::shared_ptr<TimeStampSource>,
//          boost::shared_ptr<ProcessVariableListener>);

    public:
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

    public:
      /**
       * Creates a process array that works independently. This means that the
       * instance is not synchronized with any other instance and thus the send
       * and receive operations are not supported. However, all other operations
       * can be used like on any process variable.
       */
      ProcessArrayImpl(InstanceType instanceType, const std::string& name,
          const std::vector<T>& initialValue) :
          ProcessArray<T>(name), _instanceType(instanceType), _vectorSize(
              initialValue.size()), _swappable(true), _buffers(
              boost::make_shared<std::vector<Buffer> >(1)), _currentIndex(0), _lastSentIndex(
              0) {
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
      ProcessArrayImpl(InstanceType instanceType, const std::string& name,
          const std::vector<T>& initialValue, std::size_t numberOfBuffers,
          bool swappable) :
          ProcessArray<T>(name), _instanceType(instanceType), _vectorSize(
              initialValue.size()), _swappable(swappable), _buffers(
              boost::make_shared<std::vector<Buffer> >(numberOfBuffers + 2)), _fullBufferQueue(
              boost::make_shared<boost::lockfree::queue<std::size_t> >(
                  numberOfBuffers)), _emptyBufferQueue(
              boost::make_shared<boost::lockfree::spsc_queue<std::size_t> >(
                  numberOfBuffers)), _currentIndex(0), _lastSentIndex(0) {
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
          boost::scoped_ptr<std::vector<T> > v(
              new std::vector<T>(initialValue));
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
       * sender's {@link ProcessArrayImpl::send()} method is called. It can be
       * used to queue a request for the receiver's
       * {@link ProcessArrayImpl::receive()} method to be called. The process
       * variable passed to the listener is the receiver and not the sender.
       */
      ProcessArrayImpl(InstanceType instanceType, bool swappable,
          boost::shared_ptr<TimeStampSource> timeStampSource,
          boost::shared_ptr<ProcessVariableListener> sendNotificationListener,
          boost::shared_ptr<ProcessArrayImpl> receiver) :
          ProcessArray<T>(receiver->getName()), _instanceType(instanceType), _vectorSize(
              receiver->_vectorSize), _swappable(swappable), _buffers(
              receiver->_buffers), _fullBufferQueue(receiver->_fullBufferQueue), _emptyBufferQueue(
              receiver->_emptyBufferQueue), _currentIndex(1), _lastSentIndex(1), _receiver(
              receiver), _timeStampSource(timeStampSource), _sendNotificationListener(
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
       * Assignment operator. This behaves excactly like the set(...) method.
       */
      ProcessArrayImpl<T> & operator=(ProcessArrayImpl<T> const & other) {
        set(other.getConst());
        return (*this);
      }

      /**
       * Assignment operator. This behaves exactly like the set(...) method.
       */
      ProcessArrayImpl<T> & operator=(ProcessArray<T> const & other) {
        set(other.getConst());
        return (*this);
      }

      /**
       * Assignment operator. This behaves exactly like the set(...) method.
       */
      ProcessArrayImpl<T> & operator=(std::vector<T> const & v) {
        set(v);
        return *this;
      }

      operator std::vector<T>() const {
        return getConst();
      }

      void set(std::vector<T> const & v) {
        get() = v;
      }

      void set(ProcessArray<T> const & other) {
        set(other.getConst());
      }

      void swap(boost::scoped_ptr<std::vector<T> > & otherVector) {
        if (!isSwappable()) {
          throw std::logic_error(
              "Swap is not supported by this process array.");
        }
        if (otherVector->size() != get().size()) {
          throw std::runtime_error("Vector sizes do not match");
        }
        (*_buffers)[_currentIndex].value.swap(otherVector);
      }

      std::vector<T> & get() {
        if (_instanceType == RECEIVER && !_swappable) {
          throw std::logic_error(
              "Attempt to modify a read-only process variable.");
        }
        return *(((*_buffers)[_currentIndex]).value);
      }

      std::vector<T> const & get() const {
        return getConst();
      }

      std::vector<T> const & getConst() const {
        return *(((*_buffers)[_currentIndex]).value);
      }

      std::vector<T> const & getLastSent() const {
        if (_instanceType != SENDER || _swappable) {
          throw std::logic_error(
              "The last sent value is only available for a sender that is associated with a receiver that does not allow swapping.");
        }
        return *(((*_buffers)[_lastSentIndex]).value);
      }

      bool isReceiver() const {
        return _instanceType == RECEIVER;
      }

      bool isSender() const {
        return _instanceType == SENDER;
      }

      TimeStamp getTimeStamp() const {
        return ((*_buffers)[_currentIndex]).timeStamp;
      }

      bool isSwappable() const {
        // Senders and stand-alone instances are always swappable. For
        // receivers, it depends on the swappable flag.
        if (_instanceType == RECEIVER) {
          return _swappable;
        } else {
          return true;
        }
      }

      bool receive() {
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
          _emptyBufferQueue->push(_currentIndex);
          _currentIndex = nextIndex;
          return true;
        } else {
          return false;
        }
      }

      bool send() {
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
        std::size_t nextIndex;
        bool foundEmptyBuffer;
        if (_emptyBufferQueue->pop(nextIndex)) {
          foundEmptyBuffer = true;
          _fullBufferQueue->push(_currentIndex);
        } else {
          // We can still send, but we will lose older data.
          if (_fullBufferQueue->pop(nextIndex)) {
            foundEmptyBuffer = false;
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
            foundEmptyBuffer = true;
            _fullBufferQueue->push(_currentIndex);
          }
        }
        _lastSentIndex = _currentIndex;
        _currentIndex = nextIndex;
        if (_sendNotificationListener) {
          _sendNotificationListener->notify(_receiver);
        }
        return foundEmptyBuffer;
      }

    private:

      /**
       * Type for the individual buffers. Each buffer stores a vector (wrapped
       * in a Boost scoped pointer) and a time stamp.
       */
      struct Buffer {

        TimeStamp timeStamp;
        boost::scoped_ptr<std::vector<T> > value;

        /**
         * Default constructor. Has to be defined explicitly because we have an
         * non-default copy constructor.
         */
        Buffer() {
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
                other.value ? new std::vector<T>(*(other.value)) : 0) {
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
      boost::shared_ptr<ProcessArrayImpl> _receiver;

      /**
       * Time-stamp source used to update the time-stamp when sending a value.
       */
      boost::shared_ptr<TimeStampSource> _timeStampSource;

      /**
       * Listener that is notified when the process variable is sent.
       */
      boost::shared_ptr<ProcessVariableListener> _sendNotificationListener;

    };

  } // namespace impl
} // namespace mtca4u

#endif // MTCA4U_PROCESS_ARRAY_IMPL_H
