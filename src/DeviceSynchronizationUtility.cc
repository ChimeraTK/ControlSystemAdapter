#include "DeviceSynchronizationUtility.h"

namespace ChimeraTK {

  DeviceSynchronizationUtility::DeviceSynchronizationUtility(
      DevicePVManager::SharedPtr pvManager) :
      _pvManager(pvManager) {
    // We choose the load factor very small. This will increase the memory
    // footprint, however because of frequent lookups we want to get the
    // performance as close to O(1) as reasonably possible.
    _receiveNotificationListeners.max_load_factor(0.5);
  }

  void DeviceSynchronizationUtility::addReceiveNotificationListener(
      mtca4u::RegisterPath const & processVariableName,
      ProcessVariableListener::SharedPtr receiveNotificationListener) {
    auto pv = _pvManager->getProcessVariable(processVariableName);
    _receiveNotificationListeners.erase(pv.get());
    _receiveNotificationListeners.insert(std::make_pair(pv.get(), receiveNotificationListener));
  }

  void DeviceSynchronizationUtility::removeReceiveNotificationListener(
      mtca4u::RegisterPath const & processVariableName) {
    auto pv = _pvManager->getProcessVariable(processVariableName);
    _receiveNotificationListeners.erase(pv.get());
  }

  void DeviceSynchronizationUtility::receiveAll() {
    ProcessVariable::SharedPtr pv;
    while ((pv = _pvManager->nextNotification())) {
      if (pv->readNonBlocking()) {
        auto listenerIterator = _receiveNotificationListeners.find(pv.get());
        if (listenerIterator != _receiveNotificationListeners.end()) {
          ProcessVariableListener::SharedPtr receiveListener(
              listenerIterator->second);
          receiveListener->notify(pv);
          while (pv->readNonBlocking()) {
            receiveListener->notify(pv);
          }
        } else {
          while (pv->readNonBlocking()) {
            continue;
          }
        }
      }
    }
  }

  void DeviceSynchronizationUtility::sendAll() {
    std::vector<ProcessVariable::SharedPtr> processVariables(
        _pvManager->getAllProcessVariables());
    for (std::vector<ProcessVariable::SharedPtr>::iterator i =
        processVariables.begin(); i != processVariables.end(); ++i) {
      if ((*i)->isWriteable()) {
        (*i)->write();
      }
    }
  }

  void DeviceSynchronizationUtility::waitForNotifications(
      long timeoutInMicroseconds, long checkIntervalInMicroseconds) {
    boost::chrono::high_resolution_clock::time_point limit =
        boost::chrono::high_resolution_clock::now()
            + boost::chrono::microseconds(timeoutInMicroseconds);
    boost::chrono::microseconds checkInterval(checkIntervalInMicroseconds);
    do {
      receiveAll();
      if (timeoutInMicroseconds > 0 && checkIntervalInMicroseconds > 0) {
        boost::this_thread::sleep_for(checkInterval);
      } else {
        return;
      }
      // Look for a pending notification until we reach the timeout.
    } while (boost::chrono::high_resolution_clock::now() < limit);
  }

} // namespace ChimeraTK
