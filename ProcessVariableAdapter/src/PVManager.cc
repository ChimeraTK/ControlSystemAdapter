#include <list>

#include "PVManager.h"
#include "ControlSystemPVManager.h"
#include "DevicePVManager.h"

namespace mtca4u {

  using boost::atomic;
  using boost::shared_ptr;
  using std::list;

  PVManager::PVManager() :
      _controlSystemNotificationQueue(0), _deviceNotificationQueue(0), _automaticReferenceTimeStampMode(
          true), _referenceTimeStamp(0) {
    // We choose the load factor very small. This will increase the memory
    // footprint, however because of frequent lookups we want to get the
    // performance as close to O(1) as reasonably possible.
    _processVariables.max_load_factor(0.5);
  }

  std::pair<ProcessVariable::SharedPtr, ProcessVariable::SharedPtr> PVManager::getProcessVariable(
      const std::string& processVariableName) const {
    ProcessVariableMap::const_iterator i = _processVariables.find(
        processVariableName);
    if (i != _processVariables.end()) {
      return i->second;
    } else {
      return std::make_pair(ProcessVariable::SharedPtr(),
          ProcessVariable::SharedPtr());
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

  ProcessVariable::SharedPtr PVManager::nextControlSystemNotification() {
    DeviceSendNotificationListenerImpl* listener;
    if (_controlSystemNotificationQueue.pop(listener)) {
      // The notification for the process variable has been handled, thus we can
      // reset the notification flag.
      listener->resetNotificationPending();
      return listener->getControlSystemProcessVariable();
    } else {
      return ProcessVariable::SharedPtr();
    }
  }

  ProcessVariable::SharedPtr PVManager::nextDeviceNotification() {
    ControlSystemSendNotificationListenerImpl* listener;
    if (_deviceNotificationQueue.pop(listener)) {
      // The notification for the process variable has been handled, thus we
      // can reset the notification flag.
      listener->resetNotificationPending();
      return listener->getDeviceProcessVariable();
    } else {
      return ProcessVariable::SharedPtr();
    }
  }

  bool PVManager::isAutomaticReferenceTimeStampMode() const {
    return _automaticReferenceTimeStampMode;
  }

  void PVManager::setAutomaticReferenceTimeStampMode(
      bool automaticReferenceTimeStampMode) {
    _automaticReferenceTimeStampMode = automaticReferenceTimeStampMode;
  }

  TimeStamp PVManager::getReferenceTimeStamp() const {
    if (_automaticReferenceTimeStampMode) {
      return TimeStamp::currentTime();
    } else {
      return _referenceTimeStamp;
    }
  }

  void PVManager::setReferenceTimeStamp(const TimeStamp& referenceTimeStamp) {
    _automaticReferenceTimeStampMode = false;
    _referenceTimeStamp = referenceTimeStamp;
  }

}
