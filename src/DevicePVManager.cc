#include <exception>

#include "DevicePVManager.h"

namespace ChimeraTK {

  DevicePVManager::DevicePVManager(boost::shared_ptr<PVManager> pvManager) :
      _pvManager(pvManager) {
  }

  ProcessVariable::SharedPtr DevicePVManager::getProcessVariable(
      const ChimeraTK::RegisterPath & processVariableName) const {
    return _pvManager->getProcessVariable(processVariableName).second;
  }

  std::vector<ProcessVariable::SharedPtr> DevicePVManager::getAllProcessVariables() const {
    std::vector<ProcessVariable::SharedPtr> devProcessVariables;
    PVManager::ProcessVariableMap const & processVariables =
        _pvManager->getAllProcessVariables();
    // We reserve the capacity that we need in order to avoid unnecessary copy
    // operations.
    devProcessVariables.reserve(processVariables.size());
    for (PVManager::ProcessVariableMap::const_iterator i =
        processVariables.begin(); i != processVariables.end(); i++) {
      devProcessVariables.push_back(i->second.second);
    }
    return devProcessVariables;
  }

  ProcessVariable::SharedPtr DevicePVManager::nextNotification() {
    return _pvManager->nextDeviceNotification();
  }

  bool DevicePVManager::isAutomaticReferenceTimeStampMode() const {
    return _pvManager->isAutomaticReferenceTimeStampMode();
  }

  void DevicePVManager::setAutomaticReferenceTimeStampMode(
      bool automaticReferenceTimeStampMode) {
    _pvManager->setAutomaticReferenceTimeStampMode(
        automaticReferenceTimeStampMode);
  }

  TimeStamp DevicePVManager::getReferenceTimeStamp() const {
    return _pvManager->getReferenceTimeStamp();
  }

  void DevicePVManager::setReferenceTimeStamp(
      const TimeStamp& referenceTimeStamp) {
    _pvManager->setReferenceTimeStamp(referenceTimeStamp);
  }

}
