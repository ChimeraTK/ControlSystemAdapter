#include <exception>

#include "DevicePVManager.h"

namespace ChimeraTK {

  DevicePVManager::DevicePVManager(boost::shared_ptr<PVManager> pvManager) : _pvManager(pvManager) {}

  ProcessVariable::SharedPtr DevicePVManager::getProcessVariable(
      const ChimeraTK::RegisterPath& processVariableName) const {
    if(!_pvManager->hasProcessArray(processVariableName)) {
      return nullptr;
    }
    return _pvManager->getProcessVariable(processVariableName).second;
  }

  std::vector<ProcessVariable::SharedPtr> DevicePVManager::getAllProcessVariables() const {
    std::vector<ProcessVariable::SharedPtr> devProcessVariables;
    PVManager::ProcessVariableMap const& processVariables = _pvManager->getAllProcessVariables();
    // We reserve the capacity that we need in order to avoid unnecessary copy
    // operations.
    devProcessVariables.reserve(processVariables.size());
    for(PVManager::ProcessVariableMap::const_iterator i = processVariables.begin(); i != processVariables.end(); i++) {
      devProcessVariables.push_back(i->second.second);
    }
    return devProcessVariables;
  }

} // namespace ChimeraTK
