#include <list>

#include "ControlSystemPVManager.h"
#include "DevicePVManager.h"
#include "PVManager.h"

namespace ChimeraTK {

  using boost::atomic;
  using boost::shared_ptr;
  using std::list;

  PVManager::PVManager() {
    /// @todo FIXME I keep this part of the code commented out, although it is not
    /// the best programming style. This performance optimisation has been turned
    /// off when changing the map key from string to RegisterPath, which does not
    /// have a hash function.
    ///  We had to change to std::map, which does not have the load factor. To
    ///  re-enable the performance optimisation write a std::hash for
    ///  RegisterPath.
    /*
    // We choose the load factor very small. This will increase the memory
    // footprint, however because of frequent lookups we want to get the
    // performance as close to O(1) as reasonably possible.
    _processVariables.max_load_factor(0.5);
    */
  }

  std::pair<ProcessVariable::SharedPtr, ProcessVariable::SharedPtr> PVManager::getProcessVariable(
      ChimeraTK::RegisterPath const& processVariableName) const {
    ProcessVariableMap::const_iterator i = _processVariables.find(processVariableName);
    if(i != _processVariables.end()) {
      return i->second;
    }
    else {
      throw ChimeraTK::logic_error("ChimeraTK::ControlSystemAdapter: Error in "
                                   "PVManager. Unknown process variable '" +
          (processVariableName) + "'");
    }
  }

  const PVManager::ProcessVariableMap& PVManager::getAllProcessVariables() const { return _processVariables; }

  std::pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager>> createPVManager() {
    // We cannot use boost::make_shared here, because we are using private
    // constructors.
    shared_ptr<PVManager> pvManager(new PVManager());
    shared_ptr<ControlSystemPVManager> controlSystemPVManager(new ControlSystemPVManager(pvManager));
    shared_ptr<DevicePVManager> devicePVManager(new DevicePVManager(pvManager));

    return std::make_pair(controlSystemPVManager, devicePVManager);
  }

} // namespace ChimeraTK
