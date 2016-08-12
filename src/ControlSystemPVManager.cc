#include "ControlSystemPVManager.h"

namespace ChimeraTK {

  ControlSystemPVManager::ControlSystemPVManager(
      boost::shared_ptr<PVManager> pvManager) :
      _pvManager(pvManager) {
  }

  ProcessVariable::SharedPtr ControlSystemPVManager::getProcessVariable(
      const std::string & processVariableName) const {
    return _pvManager->getProcessVariable(processVariableName).first;
  }

  std::vector<ProcessVariable::SharedPtr> ControlSystemPVManager::getAllProcessVariables() const {
    std::vector<ProcessVariable::SharedPtr> csProcessVariables;
    PVManager::ProcessVariableMap const & processVariables =
        _pvManager->getAllProcessVariables();
    // We reserve the capacity that we need in order to avoid unnecessary copy
    // operations.
    csProcessVariables.reserve(processVariables.size());
    for (PVManager::ProcessVariableMap::const_iterator i =
        processVariables.begin(); i != processVariables.end(); i++) {
      csProcessVariables.push_back(i->second.first);
    }
    return csProcessVariables;
  }

  ProcessVariable::SharedPtr ControlSystemPVManager::nextNotification() {
    return _pvManager->nextControlSystemNotification();
  }

}
