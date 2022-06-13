#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PV_MANAGER_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PV_MANAGER_H

#include <list>
#include <utility>

#include <boost/atomic.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <map>

#include <ChimeraTK/RegisterPath.h>

#include "BidirectionalProcessArray.h"
#include "PVManagerDecl.h"
#include "UnidirectionalProcessArray.h"
#include "ProcessVariable.h"

namespace ChimeraTK {

  // These declarations should actually be in the respective header files,
  // however this would cause problems with incomplete definitions due to
  // circular dependencies.
  class ControlSystemPVManager;
  class DevicePVManager;

  class PVManager : public boost::enable_shared_from_this<PVManager> {
    /**
     * The {@link createPVManager()} function is used to instantiate a
     * PVManager.
     */
    friend std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> createPVManager();

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
     * @todo FIXME: This was an unordered_map of strings, but it was changed to
     * register path. The register path does not have a hash, thus we fall back to
     * std::map. Write a std::hash<RegisterPath> if you cant to improve the
     * performance.
     */
    typedef std::map<ChimeraTK::RegisterPath, ProcessVariableSharedPtrPair> ProcessVariableMap;

    /**
     * Creates a new process array for transferring data between the device
     * library and the control system in both directions and registers it with
     * the PV manager. Creating a process array with a name that is already used
     * for a different process array is an error and causes an
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
     * Two process arrays are created: one for the control system and one for
     * the device library. The pair that is returned has a reference to the
     * instance intended for the control system as its first and a reference to
     * the instance intended for the device library as its second member.
     */
    template<class T>
    std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> createBidirectionalProcessArray(
        ChimeraTK::RegisterPath const& processVariableName, const std::vector<T>& initialValue,
        const std::string& unit = ChimeraTK::TransferElement::unitNotSet, const std::string& description = "",
        std::size_t numberOfBuffers = 2);

    /**
     * Creates a new process array for transferring data from the device library
     * to the control system and registers it with the PV manager.
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
     * Two process arrays are created: one for the control system and one for
     * the device library. The pair that is returned has a reference to the
     * instance intended for the control system as its first and a reference to
     * the instance intended for the device library as its second member.
     */
    template<class T>
    std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr>
        createProcessArrayDeviceToControlSystem(ChimeraTK::RegisterPath const& processVariableName,
            const std::vector<T>& initialValue, const std::string& unit = ChimeraTK::TransferElement::unitNotSet,
            const std::string& description = "", std::size_t numberOfBuffers = 3,
            const AccessModeFlags& flags = {AccessMode::wait_for_new_data});

    /**
     * Creates a new process array for transferring data from the control system
     * to the device library and registers it with the PV manager.
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
     * Two process arrays are created: one for the control system and one for
     * the device library. The pair that is returned has a reference to the
     * instance intended for the control system as its first and a reference to
     * the instance intended for the device library as its second member.
     */
    template<class T>
    std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr>
        createProcessArrayControlSystemToDevice(ChimeraTK::RegisterPath const& processVariableName,
            const std::vector<T>& initialValue, const std::string& unit = ChimeraTK::TransferElement::unitNotSet,
            const std::string& description = "", std::size_t numberOfBuffers = 3,
            const AccessModeFlags& flags = {AccessMode::wait_for_new_data});

    /**
     * Returns a reference to a process array that has been created earlier
     * using one of the <code>createProcessArray...</code> methods. 
     * Throws a logic_error if there is a process scalar or array with the specified name but its type does not match.
     * A pair of two process arrays is returned: The first member of the pair is
     * a reference to the instance intended for the control system and the
     * second member of the pair is a reference to the instance intended for the
     * device library.
     */
    template<class T>
    std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> getProcessArray(
        ChimeraTK::RegisterPath const& processVariableName) const;

    /**
     * Checks whether a process scalar or array with the specified name exists.
     */
    bool hasProcessArray(ChimeraTK::RegisterPath const& processVariableName) const;

    /**
     * Returns a reference to a process scalar or array that has been created
     * earlier using one of the <code>createProcessScalar...</code> or
     * <code>createProcessArray...</code> methods. Throws ChimeraTK::logic_error
     * if there is no process scalar or array with the specified name.
     *
     * A pair of two process variables is returned: The first member of the pair
     * is a reference to the instance intended for the control system and the
     * second member of the pair is a reference to the instance intended for the
     * device library.
     */
    std::pair<ProcessVariable::SharedPtr, ProcessVariable::SharedPtr> getProcessVariable(
        ChimeraTK::RegisterPath const& processVariableName) const;

    /**
     * Returns the map containing all process variables, using the names as
     * keys and the respective process variables as values.
     * Each value in the map is a pair which has a reference to the process
     * variable instance intended for the control system as its first and a
     * reference to the instance intended for the device library as its second
     * member.
     */
    const ProcessVariableMap& getAllProcessVariables() const;

   private:
    /**
     * Map storing the process variables.
     */
    ProcessVariableMap _processVariables;
  };

  /**
   * Creates a PV manager and returns a pair containing references to its
   * {@link ControlSystemPVManager} and {@link DevicePVManager} interfaces.
   * The {@link DevicePVManager} is only intended for use by the device-library
   * thread and the {@link ControlSystemPVManager} is only intended for use by
   * the control-system thread.
   */
  std::pair<boost::shared_ptr<ControlSystemPVManager>, boost::shared_ptr<DevicePVManager>> createPVManager();

  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> PVManager::
      createBidirectionalProcessArray(ChimeraTK::RegisterPath const& processVariableName,
          const std::vector<T>& initialValue, const std::string& unit, const std::string& description,
          std::size_t numberOfBuffers) {
    if(_processVariables.find(processVariableName) != _processVariables.end()) {
      throw ChimeraTK::logic_error("Process variable with name " + processVariableName + " already exists.");
    }

    typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> processVariables =
        createBidirectionalSynchronizedProcessArray<T>(
            initialValue, processVariableName, unit, description, numberOfBuffers);

    _processVariables.insert(
        std::make_pair(processVariableName, std::make_pair(processVariables.first, processVariables.second)));

    return std::make_pair(processVariables.first, processVariables.second);
  }

  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> PVManager::
      createProcessArrayDeviceToControlSystem(ChimeraTK::RegisterPath const& processVariableName,
          const std::vector<T>& initialValue, const std::string& unit, const std::string& description,
          std::size_t numberOfBuffers, const AccessModeFlags& flags) {
    if(_processVariables.find(processVariableName) != _processVariables.end()) {
      throw ChimeraTK::logic_error("Process variable with name " + processVariableName + " already exists.");
    }

    typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> processVariables =
        createSynchronizedProcessArray<T>(initialValue, processVariableName, unit, description, numberOfBuffers, flags);

    _processVariables.insert(
        std::make_pair(processVariableName, std::make_pair(processVariables.second, processVariables.first)));

    return std::make_pair(processVariables.second, processVariables.first);
  }

  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> PVManager::
      createProcessArrayControlSystemToDevice(ChimeraTK::RegisterPath const& processVariableName,
          const std::vector<T>& initialValue, const std::string& unit, const std::string& description,
          std::size_t numberOfBuffers, const AccessModeFlags& flags) {
    if(_processVariables.find(processVariableName) != _processVariables.end()) {
      throw ChimeraTK::logic_error("Process variable with name " + processVariableName + " already exists.");
    }

    typename std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> processVariables =
        createSynchronizedProcessArray<T>(initialValue, processVariableName, unit, description, numberOfBuffers, flags);

    _processVariables.insert(
        std::make_pair(processVariableName, std::make_pair(processVariables.first, processVariables.second)));

    return std::make_pair(processVariables.first, processVariables.second);
  }

  template<class T>
  std::pair<typename ProcessArray<T>::SharedPtr, typename ProcessArray<T>::SharedPtr> PVManager::getProcessArray(
      ChimeraTK::RegisterPath const& processVariableName) const {
    ProcessVariableSharedPtrPair processVariable = getProcessVariable(processVariableName);
    typename ProcessArray<T>::SharedPtr csPV =
        boost::dynamic_pointer_cast<ProcessArray<T>, ProcessVariable>(processVariable.first);
    typename ProcessArray<T>::SharedPtr devPV =
        boost::dynamic_pointer_cast<ProcessArray<T>, ProcessVariable>(processVariable.second);
    if(!csPV || !devPV) {
      throw ChimeraTK::logic_error("PVManager::getProcessArray() called for variable '" + processVariableName +
          "' with type " + typeid(T).name() + " which is not the original type " +
          processVariable.first->getValueType().name() + " of this process variable.");
    }
    return std::make_pair(csPV, devPV);
  }

  inline bool PVManager::hasProcessArray(ChimeraTK::RegisterPath const& processVariableName) const {
    ProcessVariableMap::const_iterator i = _processVariables.find(processVariableName);
    return (i != _processVariables.end());
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PV_MANAGER_H
