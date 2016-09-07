#include <boost/chrono.hpp>

#include "ControlSystemSynchronizationUtility.h"

namespace ChimeraTK {

  ControlSystemSynchronizationUtility::ControlSystemSynchronizationUtility(
      ControlSystemPVManager::SharedPtr pvManager) :
      _pvManager(pvManager) {
    // We choose the load factor very small. This will increase the memory
    // footprint, however because of frequent lookups we want to get the
    // performance as close to O(1) as reasonably possible.
    _receiveNotificationListeners.max_load_factor(0.5);
  }

  void ControlSystemSynchronizationUtility::addReceiveNotificationListener(
      std::string const & processVariableName,
      ProcessVariableListener::SharedPtr receiveNotificationListener) {
    _receiveNotificationListeners.erase(processVariableName);
    _receiveNotificationListeners.insert(
        std::make_pair(processVariableName, receiveNotificationListener));
  }

  void ControlSystemSynchronizationUtility::removeReceiveNotificationListener(
      std::string const & processVariableName) {
    _receiveNotificationListeners.erase(processVariableName);
  }

  void ControlSystemSynchronizationUtility::receiveAll() {
    ProcessVariable::SharedPtr pv;
    while ((pv = _pvManager->nextNotification())) {
      if (pv->readNonBlocking()) {
        boost::unordered_map<std::string, ProcessVariableListener::SharedPtr>::iterator listenerIterator =
            _receiveNotificationListeners.find(pv->getName());
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

  void ControlSystemSynchronizationUtility::sendAll() {
    std::vector<ProcessVariable::SharedPtr> processVariables(
        _pvManager->getAllProcessVariables());
    for (std::vector<ProcessVariable::SharedPtr>::iterator i =
        processVariables.begin(); i != processVariables.end(); ++i) {
      if ((*i)->isWriteable()) {
        (*i)->write();
      }
    }
  }

  void ControlSystemSynchronizationUtility::waitForNotifications(
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
