#include "ControlSystemPVManager.h"

#include <utility>

namespace ChimeraTK {

  ControlSystemPVManager::ControlSystemPVManager(boost::shared_ptr<PVManager> pvManager)
  : _pvManager(std::move(std::move(pvManager))) {}

  ProcessVariable::SharedPtr ControlSystemPVManager::getProcessVariable(
      const ChimeraTK::RegisterPath& processVariableName) const {
    auto pv = _pvManager->getProcessVariable(processVariableName).first;
    if(_persistentDataStorage && pv->isWriteable()) {
      pv->setPersistentDataStorage(_persistentDataStorage);
    }
    return pv;
  }

  std::vector<ProcessVariable::SharedPtr> ControlSystemPVManager::getAllProcessVariables() const {
    std::vector<ProcessVariable::SharedPtr> csProcessVariables;
    PVManager::ProcessVariableMap const& processVariables = _pvManager->getAllProcessVariables();
    // We reserve the capacity that we need in order to avoid unnecessary copy
    // operations.
    csProcessVariables.reserve(processVariables.size());
    for(const auto& processVariable : processVariables) {
      auto pv = processVariable.second.first;
      if(_persistentDataStorage && pv->isWriteable()) {
        pv->setPersistentDataStorage(_persistentDataStorage);
      }
      csProcessVariables.push_back(pv);
    }
    return csProcessVariables;
  }

} // namespace ChimeraTK
