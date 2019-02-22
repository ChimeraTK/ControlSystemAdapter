#include <boost/chrono.hpp>

#include "ControlSystemSynchronizationUtility.h"

namespace ChimeraTK {

  ControlSystemSynchronizationUtility::ControlSystemSynchronizationUtility(ControlSystemPVManager::SharedPtr pvManager)
  : _pvManager(pvManager) {
    // We choose the load factor very small. This will increase the memory
    // footprint, however because of frequent lookups we want to get the
    // performance as close to O(1) as reasonably possible.
    _receiveNotificationListeners.max_load_factor(0.5);
  }

  void ControlSystemSynchronizationUtility::addReceiveNotificationListener(
      ChimeraTK::RegisterPath const& processVariableName,
      ProcessVariableListener::SharedPtr receiveNotificationListener) {
    std::cout << "Use of deprecated function: Use "
                 "addReceiveNotificationListener(ProcessVariable::SharedPtr "
                 "const & processVariable, ProcessVariableListener::SharedPtr "
                 "receiveNotificationListener) instead of the version with "
                 "processVariableName!"
              << std::endl;
    auto pv = _pvManager->getProcessVariable(processVariableName);
    addReceiveNotificationListener(pv, receiveNotificationListener);
  }

  void ControlSystemSynchronizationUtility::addReceiveNotificationListener(
      ProcessVariable::SharedPtr const& processVariable,
      ProcessVariableListener::SharedPtr receiveNotificationListener) {
    _receiveNotificationListeners.erase(processVariable.get()); // get the raw pointer
    _receiveNotificationListeners.insert(std::make_pair(processVariable.get(), receiveNotificationListener));
  }

  void ControlSystemSynchronizationUtility::removeReceiveNotificationListener(
      ChimeraTK::RegisterPath const& processVariableName) {
    std::cout << "Use of deprecated function: Use "
                 "removeReceiveNotificationListener(ProcessVariable::SharedPtr const & "
                 "processVariable) instead of the version with processVariableName!"
              << std::endl;
    auto pv = _pvManager->getProcessVariable(processVariableName);
    // althougt it's just one line: call the new interface so the old test covers
    // both
    removeReceiveNotificationListener(pv); // get the raw pointer
  }

  void ControlSystemSynchronizationUtility::removeReceiveNotificationListener(
      ProcessVariable::SharedPtr const& processVariable) {
    _receiveNotificationListeners.erase(processVariable.get()); // get the raw pointer
  }

  void ControlSystemSynchronizationUtility::receiveAll() {
    ProcessVariable::SharedPtr pv;
    while((pv = _pvManager->nextNotification())) {
      if(pv->readNonBlocking()) {
        auto listenerIterator = _receiveNotificationListeners.find(pv.get());
        if(listenerIterator != _receiveNotificationListeners.end()) {
          ProcessVariableListener::SharedPtr receiveListener(listenerIterator->second);
          receiveListener->notify(pv);
          while(pv->readNonBlocking()) {
            receiveListener->notify(pv);
          }
        }
        else {
          while(pv->readNonBlocking()) {
            continue;
          }
        }
      }
    }
  }

  void ControlSystemSynchronizationUtility::sendAll() {
    std::vector<ProcessVariable::SharedPtr> processVariables(_pvManager->getAllProcessVariables());
    for(std::vector<ProcessVariable::SharedPtr>::iterator i = processVariables.begin(); i != processVariables.end();
        ++i) {
      if((*i)->isWriteable()) {
        (*i)->write();
      }
    }
  }

  void ControlSystemSynchronizationUtility::waitForNotifications(long timeoutInMicroseconds,
      long checkIntervalInMicroseconds) {
    boost::chrono::high_resolution_clock::time_point limit =
        boost::chrono::high_resolution_clock::now() + boost::chrono::microseconds(timeoutInMicroseconds);
    boost::chrono::microseconds checkInterval(checkIntervalInMicroseconds);
    do {
      receiveAll();
      if(timeoutInMicroseconds > 0 && checkIntervalInMicroseconds > 0) {
        boost::this_thread::sleep_for(checkInterval);
      }
      else {
        return;
      }
      // Look for a pending notification until we reach the timeout.
    } while(boost::chrono::high_resolution_clock::now() < limit);
  }

} // namespace ChimeraTK
