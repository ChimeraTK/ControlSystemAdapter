#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_VARIABLE_LISTENER_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_VARIABLE_LISTENER_H

#include <boost/shared_ptr.hpp>

#include "ProcessVariableDecl.h"

namespace ChimeraTK {

/**
 * Listener receiving event notifications for a process variable. The exact
 * meaning of the event depends on the context in which the listener is used.
 */
class ProcessVariableListener {

public:
	/**
	 * Shared pointer to this type.
	 */
	typedef boost::shared_ptr<ProcessVariableListener> SharedPtr;

	/**
	 * Processes the notification. This method is called every time an event
	 * occurs. The passed process variable is the one for which the event has
	 * been triggered.
	 */
	virtual void notify(boost::shared_ptr<ProcessVariable> processVariable) =0;

protected:
	/**
	 * Destructor. The destructor is protected so that class implementing this
	 * interface cannot be destroyed through pointers to this interface.
	 */
	virtual ~ProcessVariableListener() {
	}

};

}

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PROCESS_VARIABLE_LISTENER_H
