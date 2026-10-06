#include "ControlSystemPVManager.h"

#include <utility>

namespace ChimeraTK {

  ControlSystemPVManager::ControlSystemPVManager(boost::shared_ptr<PVManager> pvManager)
  : _pvManager(std::move(std::move(pvManager))) {}

  ProcessVariable::SharedPtr ControlSystemPVManager::getProcessVariable(
      const ChimeraTK::RegisterPath& processVariableName) const {
    return _pvManager->getProcessVariable(processVariableName).first;
  }

  std::vector<ProcessVariable::SharedPtr> ControlSystemPVManager::getAllProcessVariables() const {
    std::vector<ProcessVariable::SharedPtr> csProcessVariables;
    PVManager::ProcessVariableMap const& processVariables = _pvManager->getAllProcessVariables();
    // We reserve the capacity that we need in order to avoid unnecessary copy
    // operations.
    csProcessVariables.reserve(processVariables.size());
    for(const auto& processVariable : processVariables) {
      csProcessVariables.push_back(processVariable.second.first);
    }
    return csProcessVariables;
  }

  void ControlSystemPVManager::setPersistentDataStorage(const std::vector<ProcessVariable::SharedPtr>& exclude) const {
    PVManager::ProcessVariableMap const& processVariables = _pvManager->getAllProcessVariables();
    for(const auto& processVariable : processVariables) {
      auto pv = processVariable.second.first;
      if(_persistentDataStorage && pv->isWriteable()) {
        if(std::find(exclude.begin(), exclude.end(), pv) == exclude.end()) {
          pv->setPersistentDataStorage(_persistentDataStorage);
        }
      }
    }
  }

} // namespace ChimeraTK
