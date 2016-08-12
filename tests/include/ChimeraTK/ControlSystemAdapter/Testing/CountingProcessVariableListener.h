#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_COUNTING_PROCESS_VARIABLE_LISTENER_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_COUNTING_PROCESS_VARIABLE_LISTENER_H

#include <ChimeraTK/ControlSystemAdapter/ProcessVariableListener.h>

namespace ChimeraTK {

  /**
   * Simple implementation of a process-variable listener that simply counts
   * the number of times that it has been invoked and stores the pointer passed
   * with the last invocation.
   */
  struct CountingProcessVariableListener: public ProcessVariableListener {

    int count;
    ProcessVariable::SharedPtr lastProcessVariable;

    CountingProcessVariableListener() :
        count(0) {
    }

    void notify(ProcessVariable::SharedPtr processVariable) {
      ++count;
      lastProcessVariable = processVariable;
    }

  };

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_COUNTING_PROCESS_VARIABLE_LISTENER_H
