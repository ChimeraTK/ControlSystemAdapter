#ifndef MTCA4U_COUNTING_PROCESS_VARIABLE_LISTENER_H
#define MTCA4U_COUNTING_PROCESS_VARIABLE_LISTENER_H

#include <ProcessVariableListener.h>

namespace mtca4u {

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

} // namespace mtca4u

#endif // MTCA4U_COUNTING_PROCESS_VARIABLE_LISTENER_H
