#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_DEVICE_PV_MANAGER_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_DEVICE_PV_MANAGER_H

namespace ChimeraTK {
  class DevicePVManager;
} // namespace ChimeraTK

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
    friend std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> createPVManager();

   private:
    /**
     * Private constructor. Construction should be done through the
     * {@link createPVManager()} function.
     */
    explicit DevicePVManager(boost::shared_ptr<PVManager> pvManager);

    /**
     * Disable copy-construction.
     */
    DevicePVManager(const DevicePVManager&) = delete;

    /**
     * Disable copy-assignment.
     */
    DevicePVManager& operator=(const DevicePVManager&) = delete;

   public:
    /**
     * Shared pointer to this type.
     */
    using SharedPtr = boost::shared_ptr<DevicePVManager>;

    /**
     * Creates a new process array and registers it with the PV manager.
     * Creating a process array with a name that is already used for a different
     * process scalar or array is an error and causes an
     * \c ChimeraTK::logic_error exception to be thrown.
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
    typename ProcessArray<T>::SharedPtr createProcessArray(SynchronizationDirection synchronizationDirection,
        const ChimeraTK::RegisterPath& processVariableName, std::size_t size,
        const std::string& unit = ChimeraTK::TransferElement::unitNotSet, const std::string& description = "",
        T initialValue = T(), std::size_t numberOfBuffers = 3,
        const AccessModeFlags& flags = {AccessMode::wait_for_new_data});

    /**
     * Creates a new process array and registers it with the PV manager.
     * Creating a process array with a name that is already used for a different
     * process scalar or array is an error and causes an
     * \c ChimeraTK::logic_error exception to be thrown.
     *
     * The array's size is set to the number of elements stored in the vector
     * provided for initialization and all elements are initialized with the
     * values provided by this vector.
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
    typename ProcessArray<T>::SharedPtr createProcessArray(SynchronizationDirection synchronizationDirection,
        const ChimeraTK::RegisterPath& processVariableName, const std::vector<T>& initialValue,
        const std::string& unit = ChimeraTK::TransferElement::unitNotSet, const std::string& description = "",
        std::size_t numberOfBuffers = 3, const AccessModeFlags& flags = {AccessMode::wait_for_new_data});

    /**
     * Returns a reference to a process array that has been created earlier
     * using the
     * createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, std::size_t, const std::string&, const
     * std::string&, T, bool, std::size_t, const AccessModeFlags&) or
     * createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, const std::vector<T>&, const std::string&, const
     * std::string&, bool, std::size_t, const AccessModeFlags&) method. 
     * Throws a logic_error if there is no process scalar or array with
     * the specified name. Also throws logic_error if there is a process
     * scalar or array with the specified name but its type does not match.
     */
    template<class T>
    typename ProcessArray<T>::SharedPtr getProcessArray(const ChimeraTK::RegisterPath& processVariableName) const;

    /**
     * Returns a reference to a process scalar or array that has been created
     * earlier using the
     * createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, std::size_t, const std::string&, const
     * std::string&, T, bool, std::size_t, const AccessModeFlags&), or
     * createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, const std::vector<T>&, const std::string&, const
     * std::string&, bool, std::size_t, const AccessModeFlags&) method. 
     * Throws a logic_error if there is no process scalar or array with
     * the specified name. Also throws logic_error if there is a process
     * scalar or array with the specified name but its type does not match.
     */
    /* (intentionally not doxygen)
     * FIXME: It these should be links, but Doxygen can't resolve then, so we
     * leave it normal to avoid non-working links:
     *
     * The {@link getProcessScalar(const ChimeraTK::RegisterPath&)} and
     * {@link getProcessArray(const ChimeraTK::RegisterPath&)} methods should be
     * preferred
     */
    /** The \c getProcessScalar(const ChimeraTK::RegisterPath&) and
     * \c getProcessArray(const ChimeraTK::RegisterPath&) methods should be
     * preferred if the type of the process variable is known at compile time.
     */
    [[nodiscard]] ProcessVariable::SharedPtr getProcessVariable(
        const ChimeraTK::RegisterPath& processVariableName) const;

    /**
     * Checks whether a process scalar or array with the specified name exists.
     */
    [[nodiscard]] bool hasProcessVariable(ChimeraTK::RegisterPath const& processVariableName) const {
      return _pvManager->hasProcessVariable(processVariableName);
    }

    /**
     * Returns a vector containing all process variables that are registered
     * with this PV manager.
     */
    [[nodiscard]] std::vector<ProcessVariable::SharedPtr> getAllProcessVariables() const;

   private:
    /**
     * Reference to the {@link PVManager} backing this facade for the device
     * library.
     */
    boost::shared_ptr<PVManager> _pvManager;
  };

  template<class T>
  typename ProcessArray<T>::SharedPtr DevicePVManager::createProcessArray(
      SynchronizationDirection synchronizationDirection, const ChimeraTK::RegisterPath& processVariableName,
      std::size_t size, const std::string& unit, const std::string& description, T initialValue,
      std::size_t numberOfBuffers, const AccessModeFlags& flags) {
    switch(synchronizationDirection) {
      case SynchronizationDirection::controlSystemToDevice:
        return _pvManager
            ->createProcessArrayControlSystemToDevice<T>(
                processVariableName, std::vector<T>(size, initialValue), unit, description, numberOfBuffers, flags)
            .second;
      case SynchronizationDirection::deviceToControlSystem:
        return _pvManager
            ->createProcessArrayDeviceToControlSystem<T>(
                processVariableName, std::vector<T>(size, initialValue), unit, description, numberOfBuffers, flags)
            .second;
      case SynchronizationDirection::bidirectional:
        return _pvManager
            ->createBidirectionalProcessArray<T>(
                processVariableName, std::vector<T>(size, initialValue), unit, description, numberOfBuffers)
            .second;
    }
    std::cerr << "unrecoverable error: invalid syncrhonization direction in DevicePVManager::createProcessArray()"
              << std::endl;
    std::terminate(); // terminate here so the compiler does not complain release mode where an assertion is ignored.
  }

  template<class T>
  typename ProcessArray<T>::SharedPtr DevicePVManager::createProcessArray(
      SynchronizationDirection synchronizationDirection, const ChimeraTK::RegisterPath& processVariableName,
      const std::vector<T>& initialValue, const std::string& unit, const std::string& description,
      std::size_t numberOfBuffers, const AccessModeFlags& flags) {
    switch(synchronizationDirection) {
      case SynchronizationDirection::controlSystemToDevice:
        return _pvManager
            ->createProcessArrayControlSystemToDevice<T>(
                processVariableName, initialValue, unit, description, numberOfBuffers, {}, {}, flags)
            .second;
      case SynchronizationDirection::deviceToControlSystem:
        return _pvManager
            ->createProcessArrayDeviceToControlSystem<T>(
                processVariableName, initialValue, unit, description, numberOfBuffers, {}, {}, flags)
            .second;
      case SynchronizationDirection::bidirectional:
        return _pvManager
            ->createBidirectionalProcessArray<T>(processVariableName, initialValue, unit, description, numberOfBuffers)
            .second;
    }
    assert(false); // one of the switch cases should have returned
  }

  template<class T>
  typename ProcessArray<T>::SharedPtr DevicePVManager::getProcessArray(
      const ChimeraTK::RegisterPath& processVariableName) const {
    return _pvManager->getProcessArray<T>(processVariableName).second;
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_DEVICE_PV_MANAGER_H
