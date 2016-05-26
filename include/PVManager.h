#ifndef MTCA4U_PV_MANAGER_H
#define MTCA4U_PV_MANAGER_H

#include <list>
#include <utility>

#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/unordered_map.hpp>

#include "ProcessArray.h"
#include "ProcessScalar.h"

#include "PVManagerDecl.h"

namespace mtca4u {

  // These declarations should actually be in the respective header files,
  // however this would cause problems with incomplete definitions due to
  // circular dependencies.
  class ControlSystemPVManager;
  class DevicePVManager;

  class PVManager: public boost::enable_shared_from_this<PVManager> {
    /**
     * The {@link createPVManager()} function is used to instantiate a
     * PVManager.
     */
    friend std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > createPVManager();

  private:
    /**
     * Private constructor. An instance of this class should only be created
     * using the {@link #createPVManager()} function.
     */
    PVManager();

    /**
     * Disable copy-construction.
     */
    PVManager(const PVManager&);

    /**
     * Disable copy-assignment.
     */
    PVManager& operator=(const PVManager&);

  public:
    /**
     * Type alias for a pair of shared pointers to {@link ProcessVariable}s.
     */
    typedef std::pair<ProcessVariable::SharedPtr, ProcessVariable::SharedPtr> ProcessVariableSharedPtrPair;

    /**
     * Type alias for the process variable map. Useful for getting related types
     * (e.g. an iterator).
     */
    typedef boost::unordered_map<std::string, ProcessVariableSharedPtrPair> ProcessVariableMap;

    /**
     * Creates a new process scalar for transferring data from the device
     * library to the control system and registers it with the PV manager.
     * Creating a process scalar with a name that is already used for a
     * different process scalar or array is an error and causes an
     * {@link std::invalid_argument} exception to be thrown.
     *
     * The number of buffers (the minimum and default value is one) is the max.
     * number of values that can be queued in the transfer queue. Specifying a
     * larger number make loss of data less likely but increases the memory
     * footprint.
     *
     * Two process variables are created: one for the control system and one for
     * the device library. The pair that is returned has a reference to the
     * instance intended for the control system as its first and a reference to
     * the instance intended for the device library as its second member.
     */
    template<class T>
    std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> createProcessScalarDeviceToControlSystem(
        const std::string& processVariableName, T initialValue = 0,
        std::size_t numberOfBuffers = 1);

    /**
     * Creates a new process scalar for transferring data from the control
     * system to the device library and registers it with the PV manager.
     * Creating a process scalar with a name that is already used for a
     * different process scalar or array is an error and causes an
     * {@link std::invalid_argument} exception to be thrown.
     *
     * The number of buffers (the minimum and default value is one) is the max.
     * number of values that can be queued in the transfer queue. Specifying a
     * larger number make loss of data less likely but increases the memory
     * footprint.
     *
     * Two process variables are created: one for the control system and one for
     * the device library. The pair that is returned has a reference to the
     * instance intended for the control system as its first and a reference to
     * the instance intended for the device library as its second member.
     */
    template<class T>
    std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> createProcessScalarControlSystemToDevice(
        const std::string& processVariableName, T initialValue = 0,
        std::size_t numberOfBuffers = 1);

    /**
     * Creates a new process array for transferring data from the device library
     * to the control system and registers it with the PV manager.
     * Creating a process array with a name that is already used for a different
     * process scalar or array is an error and causes an
     * {@link std::invalid_argument} exception to be thrown.
     *
     * The array's size is set to the number of elements stored in the vector
     * provided for initialization and all elements are initialized with the
     * values provided by this vector.
     *
     * If the <code>swappable</code> flag is <code>true</code> (the default),
     * the control-system PV is marked as swappable and thus the control system
     * can retrieve data by swapping instead of copying. This means that the
     * device library cannot access the data (not even for reading) after
     * sending it.
     *
     * The number of buffers (the minimum and default value is two) is the max.
     * number of values that can be queued in the transfer queue. Specifying a
     * larger number make loss of data less likely but increases the memory
     * footprint.
     *
     * Two process arrays are created: one for the control system and one for
     * the device library. The pair that is returned has a reference to the
     * instance intended for the control system as its first and a reference to
     * the instance intended for the device library as its second member.
     */
    template<class T>
    std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> createProcessArrayDeviceToControlSystem(
        const std::string& processVariableName,
        const std::vector<T>& initialValue, bool swappable = true,
        std::size_t numberOfBuffers = 2);

    /**
     * Creates a new process array for transferring data from the control system
     * to the device library and registers it with the PV manager.
     * Creating a process array with a name that is already used for a different
     * process scalar or array is an error and causes an
     * {@link std::invalid_argument} exception to be thrown.
     *
     * The array's size is set to the number of elements stored in the vector
     * provided for initialization and all elements are initialized with the
     * values provided by this vector.
     *
     * If the <code>swappable</code> flag is <code>true</code> (the default),
     * the device-library PV is marked as swappable and thus the device library
     * can retrieve data by swapping instead of copying. This means that the
     * control system cannot access the data (not even for reading) after
     * sending it.
     *
     * The number of buffers (the minimum and default value is two) is the max.
     * number of values that can be queued in the transfer queue. Specifying a
     * larger number make loss of data less likely but increases the memory
     * footprint.
     *
     * Two process arrays are created: one for the control system and one for
     * the device library. The pair that is returned has a reference to the
     * instance intended for the control system as its first and a reference to
     * the instance intended for the device library as its second member.
     */
    template<class T>
    std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> createProcessArrayControlSystemToDevice(
        const std::string& processVariableName,
        const std::vector<T>& initialValue, bool swappable = true,
        std::size_t numberOfBuffers = 2);

    /**
     * Returns a reference to a process scalar that has been created earlier
     * using one of the <code>createProcessScalar...</code> methods.
     * Returns a pointer to <code>null</code> if there is no process scalar or
     * array with the specified name. Throws a bad_cast exception if there is a
     * process scalar or array with the specified name but its type does not
     * match.
     *
     * A pair of two process variables is returned: The first member of the pair
     * is a reference to the instance intended for the control system and the
     * second member of the pair is a reference to the instance intended for the
     * device library.
     */
    template<class T>
    std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> getProcessScalar(
        const std::string& processVariableName) const;

    /**
     * Returns a reference to a process array that has been created earlier
     * using one of the <code>createProcessArray...</code> methods. Returns a
     * pointer to <code>null</code> if there is no process scalar or array
     * with the specified name. Throws a bad_cast exception if there is a
     * process scalar or array with the specified name but its type does not
     * match.
     *
     * A pair of two process arrays is returned: The first member of the pair is
     * a reference to the instance intended for the control system and the
     * second member of the pair is a reference to the instance intended for the
     * device library.
     */
    template<class T>
    std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> getProcessArray(
        const std::string& processVariableName) const;

    /**
     * Returns a reference to a process scalar or array that has been created earlier
     * using one of the <code>createProcessScalar...</code> or
     * <code>createProcessArray...</code> methods. Returns a pointer to <code>null</code> if there is no process scalar or array
     * with the specified name.
     *
     * A pair of two process variables is returned: The first member of the pair
     * is a reference to the instance intended for the control system and the
     * second member of the pair is a reference to the instance intended for the
     * device library.
     */
    std::pair<ProcessVariable::SharedPtr, ProcessVariable::SharedPtr> getProcessVariable(
        const std::string& processVariableName) const;

    /**
     * Returns the map containing all process variables, using the names as
     * keys and the respective process variables as values.
     * Each value in the map is a pair which has a reference to the process
     * variable instance intended for the control system as its first and a
     * reference to the instance intended for the device library as its second
     * member.
     */
    const ProcessVariableMap& getAllProcessVariables() const;

    /**
     * Returns the next control-system process variable with a pending
     * notification or <code>null</code> if no notification are pending.
     *
     * This method should be periodically called by the control system in order
     * to find process variables that have new data available which should be
     * received.
     *
     * This method is only intended for use by the control-sytem thread.
     */
    ProcessVariable::SharedPtr nextControlSystemNotification();

    /**
     * Returns the next device-library process variable with a pending
     * notification or <code>null</code> if no notification are pending.
     *
     * This method should be periodically called by the device library in order
     * to find process variables that have new data available which should be
     * received.
     *
     * This method is only intended for use by the device-library thread.
     */
    ProcessVariable::SharedPtr nextDeviceNotification();

    /**
     * Tells whether the automatic reference time-stamp mode is enabled (the
     * default is <code>true</code>). In this mode, the method
     * getReferenceTimeStamp() simply returns the current system time. If the
     * automatic mode is disabled (this method returns <code>false</code>), the
     * getReferenceTimeStamp() method returns the value set through
     * setReferenceTimeStamp(const TimeStamp&).
     *
     * This method is only intended for use by the device-library thread.
     */
    bool isAutomaticReferenceTimeStampMode() const;

    /**
     * Enables or disables the automatic reference time-stamp mode, if the
     * parameter is <code>true</code> or <code>false</code>. In automatic mode,
     * the method getReferenceTimeStamp() simply returns the current system
     * time. In manual mode, getReferenceTimeStamp() returns the value set
     * through setReferenceTimeStamp(const TimeStamp&).
     *
     * This method is only intended for use by the device-library thread.
     */
    void setAutomaticReferenceTimeStampMode(
        bool automaticReferenceTimeStampMode);

    /**
     * Returns the current reference time-stamp. This is the time-stamp that is
     * associated with a process variable's value, when its value is changed.
     * In automatic mode (isAutomaticReferenceTimeStampMode() returns
     * <code>true</code>), the current system time is returned. In manual mode
     * (isAutomaticReferenceTimeStamp() returns <code>false</code>), the time
     * stamp set through setReferenceTimeStamp(const TimeStamp&) is returned.
     *
     * This method is only intended for use by the device-library thread.
     */
    TimeStamp getReferenceTimeStamp() const;

    /**
     * Updates the reference time-stamp. This means that the next call to
     * getReferenceTimeStamp() will return the passed time stamp. Calling this
     * method implicitly disables the automatic reference time-stamp mode.
     *
     * This method is only intended for use by the device-library thread.
     */
    void setReferenceTimeStamp(const TimeStamp& referenceTimeStamp);

  private:
    /**
     * Internal time-stamp source implementation.
     */
    class TimeStampSourceImpl: public TimeStampSource {

    private:
      // We use a weak reference in order to avoid circular references.
      boost::weak_ptr<PVManager> _pvManager;

    public:
      TimeStampSourceImpl(boost::shared_ptr<PVManager> pvManager) :
          _pvManager(pvManager) {
      }

      TimeStamp getCurrentTimeStamp() {
        boost::shared_ptr<PVManager> pvManager = _pvManager.lock();
        // We can only use the PV manager if it is not null. However, if it has
        // been destroyed, the process variables that refer to this time-stamp
        // source are not fully function anyway.
        if (pvManager) {
          return pvManager->getReferenceTimeStamp();
        } else {
          return TimeStamp::currentTime();
        }
      }

    };

    /**
     * Send notification listener implementation for device-library process
     * variables.
     */
    class DeviceSendNotificationListenerImpl: public ProcessVariableListener {

    private:
      // We use weak references in order to avoid circular references.
      boost::weak_ptr<PVManager> _pvManager;
      boost::weak_ptr<ProcessVariable> _controlSystemProcessVariable;
      boost::atomic<bool> _notificationPending;

    public:
      DeviceSendNotificationListenerImpl(boost::shared_ptr<PVManager> pvManager) :
          _pvManager(pvManager), _notificationPending(false) {
      }

      void notify(ProcessVariable::SharedPtr controlSystemProcessVariable) {
        bool expectedNotificationPending = false;
        if (_notificationPending.compare_exchange_strong(
            expectedNotificationPending, true, boost::memory_order_acq_rel)) {
          boost::shared_ptr<PVManager> pvManager = _pvManager.lock();
          // We can only use the PV manager if it is not null. However,
          // it should typically exist as long as the process variables exist.
          if (pvManager) {
            // Changing this variable without a mutex should be safe because it
            // should only happen the first time this method is called
            // (otherwise the pointer should already be initialized) and the
            // change is synchronized indirectly by writing to the notification
            // queue.
            if (_controlSystemProcessVariable.expired()) {
              _controlSystemProcessVariable = controlSystemProcessVariable;
            }
            pvManager->_controlSystemNotificationQueue.push(this);
          }
        }
      }

      inline void resetNotificationPending() {
        _notificationPending.store(false, boost::memory_order_release);
      }

      inline ProcessVariable::SharedPtr getControlSystemProcessVariable() {
        return _controlSystemProcessVariable.lock();
      }

    };

    /**
     * Send notification listener implementation for control-system process
     * variables.
     */
    class ControlSystemSendNotificationListenerImpl: public ProcessVariableListener {

    private:
      // We use weak references in order to avoid circular references.
      boost::weak_ptr<PVManager> _pvManager;
      boost::weak_ptr<ProcessVariable> _deviceProcessVariable;
      boost::atomic<bool> _notificationPending;

    public:
      ControlSystemSendNotificationListenerImpl(
          boost::shared_ptr<PVManager> pvManager) :
          _pvManager(pvManager), _notificationPending(false) {
      }

      void notify(ProcessVariable::SharedPtr deviceProcessVariable) {
        bool expectedNotificationPending = false;
        if (_notificationPending.compare_exchange_strong(
            expectedNotificationPending, true, boost::memory_order_acq_rel)) {
          boost::shared_ptr<PVManager> pvManager = _pvManager.lock();
          // We can only use the PV manager if it is not null. However,
          // it should typically exist as long as the process variables exist.
          if (pvManager) {
            // Changing this variable without a mutex should be safe because it
            // should only happen the first time this method is called
            // (otherwise the pointer should already be initialized) and the
            // change is synchronized indirectly by writing to the notification
            // queue.
            if (_deviceProcessVariable.expired()) {
              _deviceProcessVariable = deviceProcessVariable;
            }
            pvManager->_deviceNotificationQueue.push(this);
          }
        }
      }

      inline void resetNotificationPending() {
        _notificationPending.store(false, boost::memory_order_release);
      }

      inline ProcessVariable::SharedPtr getDeviceProcessVariable() {
        return _deviceProcessVariable.lock();
      }

    };

    /**
     * Map storing the process variables.
     */
    ProcessVariableMap _processVariables;

    /**
     * Queue for notifications from the device library to the control system.
     *
     * This queue is filled by the device library and processed by the control
     * system. The queue stores plain pointers to the notification listeners
     * the lock-free queue can only store PODs. This is safe because a process
     * variable (and thus the notification listener) exists as long as the PV
     * manager exists and the pointer never leaves the PV manager.
     *
     * We cannot use an spsc_queue because the spsc_queue does not allow
     * resizing once it has been created, however we need to increase the size
     * of this queue if a process variable is added.
     */
    boost::lockfree::queue<DeviceSendNotificationListenerImpl*> _controlSystemNotificationQueue;

    /**
     * Queue for notifications from the control system to the device library.
     *
     * This queue is filled by the control system and processed by the device
     * library. The queue stores plain pointers to the notification listeners
     * the lock-free queue can only store PODs. This is safe because a process
     * variable (and thus the notification listener) exists as long as the PV
     * manager exists and the pointer never leaves the PV manager.
     *
     * We cannot use an spsc_queue because the spsc_queue does not allow
     * resizing once it has been created, however we need to increase the size
     * of this queue if a process variable is added.
     */
    boost::lockfree::queue<ControlSystemSendNotificationListenerImpl*> _deviceNotificationQueue;

    /**
     * Flag indicating whether the automatic reference time-stamp mode is
     * enabled.
     */
    bool _automaticReferenceTimeStampMode;

    /**
     * Current reference time-stamp. This time stamp is only used if the
     * automatic reference time-stamp mode is disabled.
     */
    TimeStamp _referenceTimeStamp;

  };

  /**
   * Creates a PV manager and returns a pair containing references to its
   * {@link ControlSystemPVManager} and {@link DevicePVManager} interfaces.
   * The {@link DevicePVManager} is only intended for use by the device-library
   * thread and the {@link ControlSystemPVManager} is only intended for use by
   * the control-system thread.
   */
  std::pair<boost::shared_ptr<ControlSystemPVManager>,
      boost::shared_ptr<DevicePVManager> > createPVManager();

  template<class T>
  std::pair<typename ProcessScalar<T>::SharedPtr,
      typename ProcessScalar<T>::SharedPtr> PVManager::createProcessScalarDeviceToControlSystem(
      const std::string& processVariableName, T initialValue,
      std::size_t numberOfBuffers) {
    if (_processVariables.find(processVariableName)
        != _processVariables.end()) {
      throw std::invalid_argument(
          "Process variable with name " + processVariableName
              + " already exists.");
    }

    boost::shared_ptr<TimeStampSource> timeStampSource = boost::make_shared<
        TimeStampSourceImpl>(shared_from_this());
    boost::shared_ptr<ProcessVariableListener> sendNotificationListener =
        boost::make_shared<DeviceSendNotificationListenerImpl>(
            shared_from_this());

    typename std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> processVariables =
        createSynchronizedProcessScalar<T>(processVariableName, initialValue,
            numberOfBuffers, timeStampSource, sendNotificationListener);

    _processVariables.insert(
        std::make_pair(processVariableName,
            std::make_pair(processVariables.second, processVariables.first)));

    // Increase notification queue size by one to make space for this process
    // variable. We can use the unsafe variant because calls to this method have
    // to be synchronized anyway.
    _controlSystemNotificationQueue.reserve_unsafe(1);

    return std::make_pair(processVariables.second, processVariables.first);
  }

  template<class T>
  std::pair<typename ProcessScalar<T>::SharedPtr,
      typename ProcessScalar<T>::SharedPtr> PVManager::createProcessScalarControlSystemToDevice(
      const std::string& processVariableName, T initialValue,
      std::size_t numberOfBuffers) {
    if (_processVariables.find(processVariableName)
        != _processVariables.end()) {
      throw std::invalid_argument(
          "Process variable with name " + processVariableName
              + " already exists.");
    }

    // We do not use a time-stamp source for process variables that are
    // synchronized from the control system to the device.
    boost::shared_ptr<TimeStampSource> timeStampSource;
    boost::shared_ptr<ProcessVariableListener> sendNotificationListener =
        boost::make_shared<ControlSystemSendNotificationListenerImpl>(
            shared_from_this());

    typename std::pair<typename ProcessScalar<T>::SharedPtr,
        typename ProcessScalar<T>::SharedPtr> processVariables =
        createSynchronizedProcessScalar<T>(processVariableName, initialValue,
            numberOfBuffers, timeStampSource, sendNotificationListener);

    _processVariables.insert(
        std::make_pair(processVariableName,
            std::make_pair(processVariables.first, processVariables.second)));

    // Increase notification queue size by one to make space for this process
    // variable. We can use the unsafe variant because calls to this method have
    // to be synchronized anyway.
    _deviceNotificationQueue.reserve_unsafe(1);

    return std::make_pair(processVariables.first, processVariables.second);
  }

  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> PVManager::createProcessArrayDeviceToControlSystem(
      const std::string& processVariableName,
      const std::vector<T>& initialValue, bool swappable,
      std::size_t numberOfBuffers) {
    if (_processVariables.find(processVariableName)
        != _processVariables.end()) {
      throw std::invalid_argument(
          "Process variable with name " + processVariableName
              + " already exists.");
    }

    boost::shared_ptr<TimeStampSource> timeStampSource = boost::make_shared<
        TimeStampSourceImpl>(shared_from_this());
    boost::shared_ptr<ProcessVariableListener> sendNotificationListener =
        boost::make_shared<DeviceSendNotificationListenerImpl>(
            shared_from_this());

    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> processVariables =
        createSynchronizedProcessArray<T>(initialValue, processVariableName,
            swappable, numberOfBuffers, timeStampSource,
            sendNotificationListener);

    _processVariables.insert(
        std::make_pair(processVariableName,
            std::make_pair(processVariables.second, processVariables.first)));

    // Increase notification queue size by one to make space for this process
    // variable. We can use the unsafe variant because calls to this method have
    // to be synchronized anyway.
    _controlSystemNotificationQueue.reserve_unsafe(1);

    return std::make_pair(processVariables.second, processVariables.first);
  }

  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> PVManager::createProcessArrayControlSystemToDevice(
      const std::string& processVariableName,
      const std::vector<T>& initialValue, bool swappable,
      std::size_t numberOfBuffers) {
    if (_processVariables.find(processVariableName)
        != _processVariables.end()) {
      throw std::invalid_argument(
          "Process variable with name " + processVariableName
              + " already exists.");
    }

    // We do not use a time-stamp source for process variables that are
    // synchronized from the control system to the device.
    boost::shared_ptr<TimeStampSource> timeStampSource;
    boost::shared_ptr<ProcessVariableListener> sendNotificationListener =
        boost::make_shared<ControlSystemSendNotificationListenerImpl>(
            shared_from_this());

    typename std::pair<typename ProcessArray<T>::SharedPtr,
        typename ProcessArray<T>::SharedPtr> processVariables =
        createSynchronizedProcessArray<T>(initialValue, processVariableName,
            swappable, numberOfBuffers, timeStampSource,
            sendNotificationListener);

    _processVariables.insert(
        std::make_pair(processVariableName,
            std::make_pair(processVariables.first, processVariables.second)));

    // Increase notification queue size by one to make space for this process
    // variable. We can use the unsafe variant because calls to this method have
    // to be synchronized anyway.
    _deviceNotificationQueue.reserve_unsafe(1);

    return std::make_pair(processVariables.first, processVariables.second);
  }

  template<class T>
  std::pair<typename ProcessScalar<T>::SharedPtr,
      typename ProcessScalar<T>::SharedPtr> PVManager::getProcessScalar(
      const std::string& processVariableName) const {
    ProcessVariableSharedPtrPair processVariable = getProcessVariable(
        processVariableName);
    if (processVariable.first && processVariable.second) {
      typename ProcessScalar<T>::SharedPtr csPV = boost::dynamic_pointer_cast<
          ProcessScalar<T>, ProcessVariable>(processVariable.first);
      typename ProcessScalar<T>::SharedPtr devPV = boost::dynamic_pointer_cast<
          ProcessScalar<T>, ProcessVariable>(processVariable.second);
      if (!csPV || !devPV) {
        throw std::bad_cast();
      }
      return std::make_pair(csPV, devPV);
    } else {
      return std::make_pair(typename ProcessScalar<T>::SharedPtr(),
          typename ProcessScalar<T>::SharedPtr());
    }
  }

  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr,
      typename ProcessArray<T>::SharedPtr> PVManager::getProcessArray(
      const std::string& processVariableName) const {
    ProcessVariableSharedPtrPair processVariable = getProcessVariable(
        processVariableName);
    if (processVariable.first && processVariable.second) {
      typename ProcessArray<T>::SharedPtr csPV = boost::dynamic_pointer_cast<
          ProcessArray<T>, ProcessVariable>(processVariable.first);
      typename ProcessArray<T>::SharedPtr devPV = boost::dynamic_pointer_cast<
          ProcessArray<T>, ProcessVariable>(processVariable.second);
      if (!csPV || !devPV) {
        throw std::bad_cast();
      }
      return std::make_pair(csPV, devPV);
    } else {
      return std::make_pair(typename ProcessArray<T>::SharedPtr(),
          typename ProcessArray<T>::SharedPtr());
    }
  }

}

#endif // MTCA4U_PV_MANAGER_H
