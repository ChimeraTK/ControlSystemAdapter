#ifndef MTCA4U_CONTROL_SYSTEM_PV_MANAGER_H
#define MTCA4U_CONTROL_SYSTEM_PV_MANAGER_H

#include <list>
#include <map>
#include <string>

#include <boost/shared_ptr.hpp>

#include "PVManager.h"

namespace mtca4u {

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
   * {@link DevicePVManager}, however these interactions are implemented in a
   * thread-safe manner, so parallel access to the {@link DevicePVManager} by a
   * device threads is not an issue.
   */
  class ControlSystemPVManager {
    friend std::pair<boost::shared_ptr<ControlSystemPVManager>,
        boost::shared_ptr<DevicePVManager> > createPVManager();

  private:
    /**
     * Private constructor. Construction should be done through the
     * {@link createPVManager()} function.
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
     * Returns a reference to a process scalar that has been created earlier
     * using the
     * {@link DevicePVManager::createProcessScalar(SynchronizationDirection, const std::string&, T, std::size_t)}
     * method.
     * Returns a pointer to <code>null</code> if there is no process variable or
     * array with the specified name. Throws a bad_cast exception if there is a
     * process variable or array with the specified name but its type does not
     * match.
     */
    template<class T>
    typename ProcessScalar<T>::SharedPtr getProcessScalar(
        const std::string& processVariableName) const;

    /**
     * Returns a reference to a process array that has been created earlier
     * using the
     * {@link DevicePVManager::createProcessArray(SynchronizationDirection, const std::string&, std::size_t, T, bool, std::size_t)}
     * or
     * {@link DevicePVManager::createProcessArray(SynchronizationDirection, const std::string&, const std::vector<T>&, T, bool, std::size_t)}
     * method. Returns a pointer to <code>null</code> if there is no process
     * scalar or array with the specified name. Throws a bad_cast exception if
     * there is a process scalar or array with the specified name but its type
     * does not match.
     */
    template<class T>
    typename ProcessArray<T>::SharedPtr getProcessArray(
        const std::string& processVariableName) const;

    /**
     * Returns a reference to a process scalar or array that has been created earlier
     * using the
     * {@link DevicePVManager::createProcessScalar(SynchronizationDirection, const std::string&, T, std::size_t)},
     * {@link DevicePVManager::createProcessArray(SynchronizationDirection, const std::string&, std::size_t, T, bool, std::size_t)},
     * or
     * {@link DevicePVManager::createProcessArray(SynchronizationDirection, const std::string&, const std::vector<T>&, T, bool, std::size_t)}
     * method. Returns a pointer to <code>null</code> if there is no process
     * scalar or array with the specified name.
     *
     * The {@link getProcessScalar(const std::string&)} and
     * {@link getProcessArray(const std::string&)} methods should be preferred
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
     * periodically called by the control system in order to find process
     * variables that have updated by the device library and thus have new
     * values available to the control system.
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

  private:
    /**
     * Reference to the {@link PVManager} backing this facade for the control
     * system.
     */
    boost::shared_ptr<PVManager> _pvManager;

  };

  template<class T>
  typename ProcessScalar<T>::SharedPtr ControlSystemPVManager::getProcessScalar(
      const std::string& processVariableName) const {
    return _pvManager->getProcessScalar<T>(processVariableName).first;
  }

  template<class T>
  typename ProcessArray<T>::SharedPtr ControlSystemPVManager::getProcessArray(
      const std::string& processVariableName) const {
    return _pvManager->getProcessArray<T>(processVariableName).first;
  }

}

#endif // MTCA4U_CONTROL_SYSTEM_PV_MANAGER_H
