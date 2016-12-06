#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_DEVICE_PV_MANAGER_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_DEVICE_PV_MANAGER_H

namespace ChimeraTK {
  class DevicePVManager;
}

#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "PVManager.h"
#include "SynchronizationDirection.h"

namespace ChimeraTK {

  /**
   * Manager for process variables on the device side. Provides methods for
   * creating new process variables, retrieving existing ones, and synchronizing
   * process variable values with the control system.
   *
   * This class is expected to be only used from a single device thread. It
   * interacts with an instance of {@link ControlSystemPVManager}, however
   * these interactions are implemented in a thread-safe manner, so code using
   * this class does not have to deal with concurrency issues unless there is
   * more than one device thread.
   *
   * The creation of process variables is not thread safe. Therefore, all
   * process variables should be created before the control system starts using
   * the corresponding {@link ControlSystemPVManager}.
   */
  class DevicePVManager {
    friend std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > createPVManager();

  private:
    /**
     * Private constructor. Construction should be done through the
     * {@link createPVManager()} function.
     */
    DevicePVManager(boost::shared_ptr<PVManager> pvManager);

    /**
     * Disable copy-construction.
     */
    DevicePVManager(const DevicePVManager&);

    /**
     * Disable copy-assignment.
     */
    DevicePVManager& operator=(const DevicePVManager&);

  public:
    /**
     * Shared pointer to this type.
     */
    typedef boost::shared_ptr<DevicePVManager> SharedPtr;

    /**
     * Creates a new process array and registers it with the PV manager.
     * Creating a process array with a name that is already used for a different
     * process scalar or array is an error and causes an
     * \c std::invalid_argument exception to be thrown.
     *
     * If the <code>maySendDestructively</code> flag is <code>true</code> (it is
     * <code>false</code> by default), the <code>sendDestructively()</code>
     * method may be used to transfer values without copying but losing them on
     * the sender side.
     *
     * The number of buffers (the minimum and default value is two) is the max.
     * number of values that can be queued in the transfer queue. Specifying a
     * larger number make loss of data less likely but increases the memory
     * footprint.
     *
     * Two process variables are created: one for the control system and one for
     * the device library. The one that is returned is the one that should be
     * used by the device library.
     */
    template<class T>
    typename ProcessArray<T>::SharedPtr createProcessArray(
        SynchronizationDirection synchronizationDirection,
        const std::string& processVariableName, std::size_t size,
        const std::string& unit = mtca4u::TransferElement::unitNotSet, const std::string& description = "",
        T initialValue = 0, bool maySendDestructively = false, std::size_t numberOfBuffers = 2);

    /**
     * Creates a new process array and registers it with the PV manager.
     * Creating a process array with a name that is already used for a different
     * process scalar or array is an error and causes an
     * \c std::invalid_argument exception to be thrown.
     *
     * The array's size is set to the number of elements stored in the vector
     * provided for initialization and all elements are initialized with the
     * values provided by this vector.
     *
     * If the <code>maySendDestructively</code> flag is <code>true</code> (it is
     * <code>false</code> by default), the <code>sendDestructively()</code>
     * method may be used to transfer values without copying but losing them on
     * the sender side.
     *
     * The number of buffers (the minimum and default value is two) is the max.
     * number of values that can be queued in the transfer queue. Specifying a
     * larger number make loss of data less likely but increases the memory
     * footprint.
     *
     * Two process variables are created: one for the control system and one for
     * the device library. The one that is returned is the one that should be
     * used by the device library.
     */
    template<class T>
    typename ProcessArray<T>::SharedPtr createProcessArray(
        SynchronizationDirection synchronizationDirection,
        const std::string& processVariableName, const std::vector<T>& initialValue,
        const std::string& unit = mtca4u::TransferElement::unitNotSet, const std::string& description = "",
        bool maySendDestructively = false, std::size_t numberOfBuffers = 2);

    /**
     * Returns a reference to a process array that has been created earlier
     * using the
     * {@link createProcessArray(SynchronizationDirection, const std::string&, std::size_t, const std::string&, const std::string&, T, bool, std::size_t)}
     * or
     * {@link createProcessArray(SynchronizationDirection, const std::string&, const std::vector<T>&, const std::string&, const std::string&, bool, std::size_t)}
     * method. Returns a pointer to <code>null</code> if there is no process
     * scalar or array with the specified name. Throws a bad_cast exception if
     * there is a process scalar or array with the specified name but its type
     * does not match.
     */
    template<class T> typename ProcessArray<T>::SharedPtr getProcessArray(
        const std::string& processVariableName) const;

    /**
     * Returns a reference to a process scalar or array that has been created earlier
     * using the
     * {@link createProcessArray(SynchronizationDirection, const std::string&, std::size_t, const std::string&, const std::string&, T, bool, std::size_t)},
     * or
     * {@link createProcessArray(SynchronizationDirection, const std::string&, const std::vector<T>&, const std::string&, const std::string&, bool, std::size_t)}
     * method. Returns a pointer to <code>null</code> if there is no process
     * scalar or array with the specified name.
     */
    /* (intentionally not doxygen)
     * FIXME: It these should be links, but Doxygen can't resolve then, so we leave it normal to avoid
     * non-working links:
     *
     * The {@link getProcessScalar(const std::string&)} and
     * {@link getProcessArray(const std:.string&)} methods should be preferred
     */
    /** The \c getProcessScalar(const std::string&) and
     * \c getProcessArray(const std:.string&) methods should be preferred
     * if the type of the process variable is known at compile time.
     */
    ProcessVariable::SharedPtr getProcessVariable(
        const std::string& processVariableName) const;

    /**
     * Returns a vector containing all process variables that are registered
     * with this PV manager.
     */
    std::vector<ProcessVariable::SharedPtr> getAllProcessVariables() const;

    /**
     * Returns the next process variable with a pending notification or
     * <code>null</code> if no notifications are pending. This method should be
     * periodically called by the device library in order to find process
     * variables that have updated by the control system and thus have new
     * values available to the device library.
     *
     * Please note that for a single process variable, only one notification
     * might be pending even if there are multiple updates available. Therefore,
     * when processing a notification, you should try receiving new values for
     * the process variable until no new values are available. On the other
     * hand, it is also possible that a notification is pending even so there
     * are no new values available. In this case, you should simply ignore the
     * notification.
     */
    ProcessVariable::SharedPtr nextNotification();

    /**
     * Tells whether the automatic reference time-stamp mode is enabled (the
     * default is <code>true</code>). In this mode, the method
     * getReferenceTimeStamp() simply returns the current system time. If the
     * automatic mode is disabled (this method returns <code>false</code>), the
     * getReferenceTimeStamp() method returns the value set through
     * setReferenceTimeStamp(const TimeStamp&).
     */
    bool isAutomaticReferenceTimeStampMode() const;

    /**
     * Enables or disables the automatic reference time-stamp mode, if the
     * parameter is <code>true</code> or <code>false</code>. In automatic mode,
     * the method getReferenceTimeStamp() simply returns the current system
     * time. In manual mode, getReferenceTimeStamp() returns the value set
     * through setReferenceTimeStamp(const TimeStamp&).
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
     */
    TimeStamp getReferenceTimeStamp() const;

    /**
     * Updates the reference time-stamp. This means that the next call to
     * getReferenceTimeStamp() will return the passed time stamp. Calling this
     * method implicitly disables the automatic reference time-stamp mode.
     */
    void setReferenceTimeStamp(const TimeStamp& referenceTimeStamp);

  private:
    /**
     * Reference to the {@link PVManager} backing this facade for the device
     * library.
     */
    boost::shared_ptr<PVManager> _pvManager;

  };

  template<class T>
  typename ProcessArray<T>::SharedPtr DevicePVManager::createProcessArray(
      SynchronizationDirection synchronizationDirection,
      const std::string& processVariableName, std::size_t size,
      const std::string& unit, const std::string& description, T initialValue,
      bool maySendDestructively, std::size_t numberOfBuffers) {
    switch (synchronizationDirection) {
    case controlSystemToDevice:
      return _pvManager->createProcessArrayControlSystemToDevice<T>(
          processVariableName, std::vector<T>(size, initialValue), unit, description,
          maySendDestructively, numberOfBuffers).second;
    case deviceToControlSystem:
      return _pvManager->createProcessArrayDeviceToControlSystem<T>(
          processVariableName, std::vector<T>(size, initialValue), unit, description,
          maySendDestructively, numberOfBuffers).second;
    default:
      throw std::invalid_argument("invalid SynchronizationDirection");
    }
  }

  template<class T>
  typename ProcessArray<T>::SharedPtr DevicePVManager::createProcessArray(
      SynchronizationDirection synchronizationDirection,
      const std::string& processVariableName, const std::vector<T>& initialValue,
      const std::string& unit, const std::string& description,
      bool maySendDestructively, std::size_t numberOfBuffers) {
    switch (synchronizationDirection) {
    case controlSystemToDevice:
      return _pvManager->createProcessArrayControlSystemToDevice<T>(
          processVariableName, initialValue, unit, description, maySendDestructively,
          numberOfBuffers).second;
    case deviceToControlSystem:
      return _pvManager->createProcessArrayDeviceToControlSystem<T>(
          processVariableName, initialValue, unit, description, maySendDestructively,
          numberOfBuffers).second;
    default:
      throw std::invalid_argument("invalid SynchronizationDirection");
    }
  }

  template<class T>
  typename ProcessArray<T>::SharedPtr DevicePVManager::getProcessArray(
      const std::string& processVariableName) const {
    return _pvManager->getProcessArray<T>(processVariableName).second;
  }

}

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_DEVICE_PV_MANAGER_H
