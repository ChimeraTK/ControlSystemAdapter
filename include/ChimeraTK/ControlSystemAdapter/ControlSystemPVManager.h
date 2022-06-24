#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_CONTROL_SYSTEM_PV_MANAGER_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_CONTROL_SYSTEM_PV_MANAGER_H

#include <list>
#include <map>

#include <boost/shared_ptr.hpp>

#include "ApplicationBase.h"
#include "PVManager.h"

namespace ChimeraTK {

  /**
   * Manager for process variables on the control-system side. Provides methods
   * for retrieving process variables that have been created by the device side
   * and synchronizing those variables with the device library.
   *
   * This class is expected to be only used by control-system threads. If the
   * control system has multiple threads, these threads have to be synchronized
   * so that only one thread uses the PV manager at a time.
   *
   * Each instance of this class interacts with an associated instance of
   * DevicePVManager, however these interactions are implemented in a
   * thread-safe manner, so parallel access to the DevicePVManager by a
   * device threads is not an issue.
   */
  class ControlSystemPVManager {
    friend std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> createPVManager();

   private:
    /**
     * Private constructor. Construction should be done through the
     * createPVManager() function.
     */
    ControlSystemPVManager(boost::shared_ptr<PVManager> pvManager);

    /**
     * Disable copy-construction.
     */
    ControlSystemPVManager(const ControlSystemPVManager&);

    /**
     * Disable copy-assignment.
     */
    ControlSystemPVManager& operator=(const ControlSystemPVManager&);

   public:
    /**
     * Shared pointer to this type.
     */
    typedef boost::shared_ptr<ControlSystemPVManager> SharedPtr;

    /**
     * Returns a reference to a process array that has been created earlier
     * using the
     * DevicePVManager::createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, std::size_t, const std::string&, const
     * std::string&, T, bool, std::size_t, const AccessModeFlags&) or
     * DevicePVManager::createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, const std::vector<T>&, const std::string&, const
     * std::string&, bool, std::size_t, const AccessModeFlags&) method. 
     * Throws logic_error if there is no process scalar or array with
     * the specified name. Also throws a logic_error if there is a process
     * scalar or array with the specified name but its type does not match.
     */
    template<class T>
    typename ProcessArray<T>::SharedPtr getProcessArray(const ChimeraTK::RegisterPath& processVariableName) const;

    /**
     * Returns a reference to a process scalar or array that has been created
     * earlier using the
     * DevicePVManager::createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, std::size_t, const std::string&, const
     * std::string&, T, bool, std::size_t, const AccessModeFlags&), or
     * DevicePVManager::createProcessArray(SynchronizationDirection, const
     * ChimeraTK::RegisterPath&, const std::vector<T>&, const std::string&, const
     * std::string&, bool, std::size_t, const AccessModeFlags&) method. Throws
     * a ChimeraTK::logic_error if the process variable does not exist.
     */
    /* (intentionally not doxygen)
     * FIXME: It these should be links, but Doxygen can't resolve then, so we
     * leave it normal to avoid non-working links:
     *
     * The getProcessArray(const ChimeraTK::RegisterPath&) methods should
     * be preferred
     */
    /** The \c getProcessScalar(const ChimeraTK::RegisterPath&) and
     * \c getProcessArray(const ChimeraTK::RegisterPath&) methods should be
     * preferred if the type of the process variable is known at compile time.
     */
    ProcessVariable::SharedPtr getProcessVariable(const ChimeraTK::RegisterPath& processVariableName) const;

    /**
     * Checks whether a process scalar or array with the specified name exists.
     */
    bool hasProcessVariable(ChimeraTK::RegisterPath const& processVariableName) const {
      return _pvManager->hasProcessVariable(processVariableName);
    }

    /**
     * Returns a vector containing all process variables that are registered
     * with this PV manager.
     */
    std::vector<ProcessVariable::SharedPtr> getAllProcessVariables() const;

    /**
     * Enable the persistent data storage system provided by the
     * ControlSystemAdapter. This function requires an existing instance of an
     * ApplicationBase class. You can specify the write interval in seconds.
     */
    void enablePersistentDataStorage(unsigned int writeInterval = PersistentDataStorage::DEFAULT_WRITE_INTERVAL) {
      _persistentDataStorage = ApplicationBase::getInstance().getPersistentDataStorage(writeInterval);
    }

   private:
    /**
     * Reference to the PVManager backing this facade for the control
     * system.
     */
    boost::shared_ptr<PVManager> _pvManager;

    /**
     * All process variables should be registered in this persistent data storage.
     * This member is mutable, since it is required in a non-const form inside
     * const functions like getProcessArray(). The change of the persistent data
     * storage object does not represent a state change which is visible to the
     * outside, thus this is a legit use of the mutable qualifier.
     */
    mutable boost::shared_ptr<PersistentDataStorage> _persistentDataStorage;
  };

  template<class T>
  typename ProcessArray<T>::SharedPtr ControlSystemPVManager::getProcessArray(
      const ChimeraTK::RegisterPath& processVariableName) const {
    auto pv = _pvManager->getProcessArray<T>(processVariableName).first;
    if(_persistentDataStorage && pv->isWriteable()) pv->setPersistentDataStorage(_persistentDataStorage);
    return pv;
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_CONTROL_SYSTEM_PV_MANAGER_H
