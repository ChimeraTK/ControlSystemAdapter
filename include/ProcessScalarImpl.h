#ifndef MTCA4U_PROCESS_SCALAR_IMPL_H
#define MTCA4U_PROCESS_SCALAR_IMPL_H

#include <limits>
#include <stdexcept>
#include <typeinfo>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

#include "ProcessScalar.h"
#include "ProcessVariableListener.h"

namespace mtca4u {
  namespace impl {

    /**
     * Implementation of the ProcessScalar. This implementation is used for all
     * three use cases (sender, receiver, and stand-alone).
     */
    template<class T>
    class ProcessScalarImpl: public ProcessScalar<T> {

      // Allow the factory functions to create instances of this class.
      friend typename ProcessScalar<T>::SharedPtr createSimpleProcessScalar<T>(
          const std::string &, T);

      friend typename std::pair<typename ProcessScalar<T>::SharedPtr,
          typename ProcessScalar<T>::SharedPtr> createSynchronizedProcessScalar<
          T>(const std::string &, T, std::size_t,
          boost::shared_ptr<TimeStampSource>,
          boost::shared_ptr<ProcessVariableListener>);

    private:
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
       * Creates a process scalar that works independently. This means that the
       * instance is not synchronized with any other instance and thus the send
       * and receive operations are not supported. However, all other operations
       * can be used like on any process variable.
       */
      ProcessScalarImpl(InstanceType instanceType, const std::string& name,
          T initialValue) :
          ProcessScalar<T>(name), _instanceType(instanceType), _value(
              initialValue) {
        // It would be better to do the validation before initializing, but this
        // would mean that we would have to initialize twice.
        if (instanceType != STAND_ALONE) {
          throw std::invalid_argument(
              "This constructor may only be used for a stand-alone process scalar.");
        }
      }

      /**
       * Creates a process scalar that acts as a receiver. A receiver is
       * intended to work as a tandem with a sender and receives values that
       * have been set to the sender.
       *
       * This constructor creates the buffers and queues that are needed for the
       * send/receive process and are shared with the sender.
       */
      ProcessScalarImpl(InstanceType instanceType, const std::string& name,
          T initialValue, std::size_t numberOfBuffers) :
          ProcessScalar<T>(name), _instanceType(instanceType), _value(
              initialValue), _bufferQueue(
              boost::make_shared<boost::lockfree::queue<Buffer> >(
                  numberOfBuffers)) {
        // It would be better to do the validation before initializing, but this
        // would mean that we would have to initialize twice.
        if (instanceType != RECEIVER) {
          throw std::invalid_argument(
              "This constructor may only be used for a receiver process scalar.");
        }
        if (numberOfBuffers < 1) {
          throw std::invalid_argument(
              "The number of buffers must be at least one.");
        }
      }

      /**
       * Creates a process scalar that acts as a sender. A sender is intended
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
       * sender's {@link ProcessScalarImpl::send()} method is called. It can be
       * used to queue a request for the receiver's
       * {@link ProcessScalarImpl::receive()} method to be called. The process
       * variable passed to the listener is the receiver and not the sender.
       */
      ProcessScalarImpl(InstanceType instanceType,
          boost::shared_ptr<TimeStampSource> timeStampSource,
          boost::shared_ptr<ProcessVariableListener> sendNotificationListener,
          boost::shared_ptr<ProcessScalarImpl> receiver) :
          ProcessScalar<T>(receiver->getName()), _instanceType(instanceType), _timeStamp(
              receiver->_timeStamp), _value(receiver->_value), _bufferQueue(
              receiver->_bufferQueue), _receiver(receiver), _timeStampSource(
              timeStampSource), _sendNotificationListener(
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
      ProcessScalarImpl<T> & operator=(ProcessScalarImpl<T> const & other) {
        set(other.get());
        return (*this);
      }

      /**
       * Assignment operator. This behaves exactly like the set(...) method.
       */
      ProcessScalarImpl<T> & operator=(ProcessScalar<T> const & other) {
        set(other.get());
        return (*this);
      }

      /**
       * Assignment operator. This behaves exactly like the set(...) method.
       */
      ProcessScalarImpl<T> & operator=(T const & t) {
        set(t);
        return *this;
      }

      operator T() const {
        return get();
      }

      void set(T const & t) {
        _value = t;
      }

      void set(ProcessScalar<T> const & other) {
        set(other.get());
      }

      T get() const {
        return _value;
      }

      bool isReceiver() const {
        return _instanceType == RECEIVER;
      }

      bool isSender() const {
        return _instanceType == SENDER;
      }

      TimeStamp getTimeStamp() const {
        return _timeStamp;
      }

      bool receive() {
        if (_instanceType != RECEIVER) {
          throw std::logic_error(
              "Receive operation is only allowed for a receiver process variable.");
        }
        Buffer nextBuffer;
        if (_bufferQueue->pop(nextBuffer)) {
          _timeStamp = nextBuffer.timeStamp;
          _value = nextBuffer.value;
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
        // Before sending the value, we have to update our time stamp.
        _timeStamp =
            _timeStampSource ?
                _timeStampSource->getCurrentTimeStamp() :
                TimeStamp::currentTime();
        Buffer nextBuffer;
        nextBuffer.timeStamp = _timeStamp;
        nextBuffer.value = _value;
        bool foundEmptyBuffer;
        if (_bufferQueue->bounded_push(nextBuffer)) {
          foundEmptyBuffer = true;
        } else {
          // We are not interested in the old value, but we have to provided
          // a reference.
          Buffer oldBuffer;
          // If we remove an element, pop returns true, otherwise the receiving
          // thread just took the last element and we can continue without
          // losing data.
          foundEmptyBuffer = !_bufferQueue->pop(oldBuffer);
          // Now we can be sure that the push is successful.
          _bufferQueue->bounded_push(nextBuffer);
        }
        if (_sendNotificationListener) {
          _sendNotificationListener->notify(_receiver);
        }
        return foundEmptyBuffer;
      }

    private:
      /**
       * Type for the individual buffers. Each buffer stores a value and a time
       * stamp.
       */
      struct Buffer {
        TimeStamp timeStamp;
        T value;
      };

      /**
       * Type this instance is representing.
       */
      InstanceType _instanceType;

      /**
       * Time stamp of the current value.
       */
      TimeStamp _timeStamp;

      /**
       * Current value.
       */
      T _value;

      /**
       * Queue holding the values that have been sent but not received yet.
       * We do not use an spsc_queue for this queue, because might want to take
       * elements from the sending thread, so there are two threads which might
       * consume elements and thus an spsc_queue is not safe.
       */
      boost::shared_ptr<boost::lockfree::queue<Buffer> > _bufferQueue;

      /**
       * Pointer to the receiver associated with this sender. This field is only
       * used if this process variable represents a sender.
       */
      boost::shared_ptr<ProcessScalarImpl<T> > _receiver;

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
} //namespace mtca4u

#endif // MTCA4U_PROCESS_SCALAR_IMPL_H
