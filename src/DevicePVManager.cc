#include "DevicePVManager.h"

#include <exception>
#include <utility>

namespace ChimeraTK {

  DevicePVManager::DevicePVManager(boost::shared_ptr<PVManager> pvManager)
  : _pvManager(std::move(std::move(pvManager))) {}

  ProcessVariable::SharedPtr DevicePVManager::getProcessVariable(
      const ChimeraTK::RegisterPath& processVariableName) const {
    return _pvManager->getProcessVariable(processVariableName).second;
  }

  std::vector<ProcessVariable::SharedPtr> DevicePVManager::getAllProcessVariables() const {
    std::vector<ProcessVariable::SharedPtr> devProcessVariables;
    PVManager::ProcessVariableMap const& processVariables = _pvManager->getAllProcessVariables();
    // We reserve the capacity that we need in order to avoid unnecessary copy
    // operations.
    devProcessVariables.reserve(processVariables.size());
    for(const auto& processVariable : processVariables) {
      devProcessVariables.push_back(processVariable.second.second);
    }
    return devProcessVariables;
  }

} // namespace ChimeraTK
