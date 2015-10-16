#ifndef MTCA4U_CONTROL_SYSTEM_SYNCHRONIZATION_UTILITY
#define MTCA4U_CONTROL_SYSTEM_SYNCHRONIZATION_UTILITY

#include <boost/unordered_map.hpp>

#include "ControlSystemPVManager.h"

namespace mtca4u {

  /**
   * Utility class for synchronizing process variables on the control-system
   * side. This class makes it easier to process changes coming from the device
   * library by checking the notification queue and only receiving those process
   * variables which have new values pending.
   */
  class ControlSystemSynchronizationUtility {

  public:
    /**
     * Creates a new synchronization utility for use by a control system. The
     * synchronization utility should be created after registering all process
     * variables with the PV manager.
     */
    ControlSystemSynchronizationUtility(ControlSystemPVManager::SharedPtr);

    /**
     * Register a receive notification listener for a process variable. This
     * listener is triggered every time after calling <code>receive</code> on
     * the process variable if the receive operation returned <code>true</code>.
     *
     * If another listener has previously been registered for the named process
     * variable, that listener is replaced with the new listener.
     */
    void addReceiveNotificationListener(std::string const & processVariableName,
        ProcessVariableListener::SharedPtr receiveNotificationListener);

    /**
     * Removes a receive notification listener that has previously been
     * registered with
     * {@link #addReceiveNotificationListener(std::string const &, ProcessVariableListener::SharedPtr)}.
     * If no listener is registered for the specified process variable name,
     * this method does nothing.
     */
    void removeReceiveNotificationListener(
        std::string const & processVariableName);

    /**
     * Runs receive on all process variables that have data pending. This method
     * retrieves the process variables with pending data from the PV manager's
     * queue and runs receive on them until no new data is available.
     */
    void receiveAll();

    /**
     * Sends all process variables that are senders. This method cannot know
     * which process variables have actually changed, therefore one of the
     * <code>send(...)</code> methods that take a list of process variables
     * should be preferred. However, this method can be used if the the control
     * system always updates all process variables that transfer data to the
     * device library.
     */
    void sendAll();

    /**
     * Run the receive operation on a list of process variables. This can be any
     * type of container that has <code>begin()</code> and <code>end()</code>
     * methods that return an appropriate iterator. The elements must be of type
     * {@link ProcessVariable::SharedPtr}.
     */
    template<class T>
    void receive(T processVariables);

    /**
     * Run the receive operation on a list of process variables. The list is
     * specified in the form of a begin and end iterator. For example, this can
     * be an STL iterator or a raw pointer. The elements must be of type
     * {@link ProcessVariable::SharedPtr}.
     */
    template<class T>
    void receive(T processVariablesBegin, T processVariablesEnd);

    /**
     * Run the send operation on a list of process variables. This can be any
     * type of container that has <code>begin()</code> and <code>end()</code>
     * methods that return an appropriate iterator. The elements must be of type
     * {@link ProcessVariable::SharedPtr}.
     */
    template<class T>
    void send(T processVariables);

    /**
     * Run the send operation on a list of process variables. The list is
     * specified in the form of a begin and end iterator. For example, this can
     * be an STL iterator or a raw pointer. The elements must be of type
     * {@link ProcessVariable::SharedPtr}.
     */
    template<class T>
    void send(T processVariablesBegin, T processVariablesEnd);

    /**
     * Periodically runs {@link #receiveAll()} in order to process incoming
     * changes.
     *
     * This method calls {@link #receiveAll()} every
     * <code>checkIntervalInMicroseconds</code> microseconds, until
     * <code>timeoutInMicroseconds</code> microseconds have passed in total,
     * sleeping in between. If either of the parameters is less than or equal to
     * zero, this method returns immediately after calling {@link #receiveAll()}
     * once.
     *
     * This is useful if you want to spend time sleeping while still
     * periodically checking for new values.
     */
    void waitForNotifications(long timeoutInMicroseconds,
        long checkIntervalInMicroseconds);

  private:
    /**
     * Pointer to the PV manager used by this class.
     */
    ControlSystemPVManager::SharedPtr _pvManager;

    /**
     * Map storing the receive listeners. The maps uses the process variable
     * as the key and the corresponding receive listener as the value.
     */
    boost::unordered_map<std::string, ProcessVariableListener::SharedPtr> _receiveNotificationListeners;

    // Disable copy construction and assignment.
    ControlSystemSynchronizationUtility(
        ControlSystemSynchronizationUtility const &);
    ControlSystemSynchronizationUtility & operator=(
        ControlSystemSynchronizationUtility const &);
  };

  template<class T>
  void ControlSystemSynchronizationUtility::receive(T processVariables) {
    receive(processVariables.begin(), processVariables.end());
  }

  template<class T>
  void ControlSystemSynchronizationUtility::receive(T processVariablesBegin,
      T processVariablesEnd) {
    for (T i = processVariablesBegin; i != processVariablesEnd; ++i) {
      if ((*i)->receive()) {
        boost::unordered_map<std::string, ProcessVariableListener::SharedPtr>::iterator listenerIterator =
            _receiveNotificationListeners.find((*i)->getName());
        if (listenerIterator != _receiveNotificationListeners.end()) {
          ProcessVariableListener::SharedPtr receiveListener(
              listenerIterator->second);
          receiveListener->notify((*i));
          while ((*i)->receive()) {
            receiveListener->notify((*i));
          }
        } else {
          while ((*i)->receive()) {
            continue;
          }
        }
      }
    }
  }

  template<class T>
  void ControlSystemSynchronizationUtility::send(T processVariables) {
    send(processVariables.begin(), processVariables.end());
  }

  template<class T>
  void ControlSystemSynchronizationUtility::send(T processVariablesBegin,
      T processVariablesEnd) {
    for (T i = processVariablesBegin; i != processVariablesEnd; ++i) {
      (*i)->send();
    }
  }

} // namespace mtca4u

#endif // MTCA4U_CONTROL_SYSTEM_SYNCHRONIZATION_UTILITY
