#include <list>

#include "PVManager.h"
#include "ControlSystemPVManager.h"
#include "DevicePVManager.h"

namespace ChimeraTK {

  using boost::atomic;
  using boost::shared_ptr;
  using std::list;

  PVManager::PVManager() {}

  std::pair<ProcessVariable::SharedPtr, ProcessVariable::SharedPtr> PVManager::getProcessVariable(
      ChimeraTK::RegisterPath const & processVariableName) const {
    ProcessVariableMap::const_iterator i = _processVariables.find(
        processVariableName);
    if (i != _processVariables.end()) {
      return i->second;
    } else {
      throw ChimeraTK::logic_error("ChimeraTK::ControlSystemAdapter: Error in PVManager. Unknown process variable '"+(processVariableName)+"'");
    }
  }

  const PVManager::ProcessVariableMap& PVManager::getAllProcessVariables() const {
    return _processVariables;
  }

  std::pair<shared_ptr<ControlSystemPVManager>, shared_ptr<DevicePVManager> > createPVManager() {
    // We cannot use boost::make_shared here, because we are using private
    // constructors.
    shared_ptr<PVManager> pvManager(new PVManager());
    shared_ptr<ControlSystemPVManager> controlSystemPVManager(
        new ControlSystemPVManager(pvManager));
    shared_ptr<DevicePVManager> devicePVManager(new DevicePVManager(pvManager));

    return std::make_pair(controlSystemPVManager, devicePVManager);
  }

}
