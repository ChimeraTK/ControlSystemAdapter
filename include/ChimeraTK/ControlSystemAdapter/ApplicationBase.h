// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_APPLICATION_BASE_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_APPLICATION_BASE_H

#include "DevicePVManager.h"

#include <mutex>

namespace ChimeraTK {

  /** Base class for applications. Note: The class intended for use by the
   * application programmers is part of the ApplicationCore library and called
   * ChimeraTK::Application. ApplicationBase just contains the part which is
   *  necessary to write the middleware-specific code initialising the
   * application. */
  class ApplicationBase {
   public:
    /** Constructor: the first instance will be created explicitly by the control
     * system adapter code. Any second instance is not allowed, thus calling the
     * constructor multiple times will throw an exception. Design note: We are not
     * using a true singleton pattern, since Application is an abstract base
     * class. The actual instance is created as a static variable. The application
     * developer should derive his application from this class and implement the
     * initialise() function only. */
    explicit ApplicationBase(std::string name);

    /** The implementation of Application must call Application::shutdown() in its
     * destructor. This destructor just checks if Application::shutdown() was
     * properly called and throws an exception otherwise. */
    virtual ~ApplicationBase();

    /** Initialise the application. This function must be implemented by the
     * application programmer. In this
     *  function, the application must register all process variables in the PV
     * manager. */
    virtual void initialise() = 0;

    /** Optimise unmapped variables to avoid unnecessary copies. The application must implement this function. The
     *  ControlSystemAdapter implementation should call it with a list of PV names which are not mapped, i.e. the
     *  Application does not need to update the values of these PVs. If this function is not called, no such
     * optimisation can be done and the application may have an increased CPU usage. If the function is called, it must
     * be called after initialise() and before run(). */
    virtual void optimiseUnmappedVariables(const std::set<std::string>& unmappedVariables) { (void)unmappedVariables; }

    /** Run the application. This function must be implemented by the application
     * programmer. It will be called after initialise() and after all process
     * variables have been created and, if applicable, initialised with values
     * e.g. from a config file. The application must start one or more threads
     * inside this function, which will execute the actual application. The
     * function must then return, so the control system adpater can continue its
     * operation. */
    virtual void run() = 0;

    /** This will remove the global pointer to the instance and allows creating
     * another instance afterwards. This is mostly useful for writing tests, as it
     * allows to run several applications sequentially in the same executable. It
     * is mandatory to call this function inside the descructor of the
     * implemention of this class. */
    virtual void shutdown();

    /** Set the process variable manager. This will be called by the control
     * system adapter initialisation code. */
    void setPVManager(boost::shared_ptr<ChimeraTK::DevicePVManager> const& processVariableManager);

    /** Obtain the process variable manager. */
    boost::shared_ptr<ChimeraTK::DevicePVManager> getPVManager() { return _processVariableManager; }

    /** Obtain instance of the application. Will throw an exception if called
     * before the instance has been created by the control system adapter. */
    static ApplicationBase& getInstance();

    /** Return the name of the application */
    [[nodiscard]] const std::string& getName() const { return _applicationName; }

    /** Obtain the PersistentDataStorage object. You can specify the write interval in seconds. */
    boost::shared_ptr<PersistentDataStorage> getPersistentDataStorage(
        unsigned int writeInterval = PersistentDataStorage::DEFAULT_WRITE_INTERVAL) {
      if(!_persistentDataStorage) {
        _persistentDataStorage.reset(new PersistentDataStorage(_applicationName, writeInterval));
      }
      return _persistentDataStorage;
    }

   protected:
    /** The name of the application */
    std::string _applicationName;

    /** Pointer to the process variable manager used to create variables exported
     * to the control system */
    boost::shared_ptr<ChimeraTK::DevicePVManager> _processVariableManager;

    /** Pointer to persistent data storage object, if used */
    boost::shared_ptr<PersistentDataStorage> _persistentDataStorage;

    /** Flag if shutdown() has been called. */
    bool _hasBeenShutdown{false};

    /** Pointer to the only instance of the Application */
    static ApplicationBase* instance;

    /** Mutex for thread-safety when setting the instance pointer */
    static std::recursive_mutex instanceMutex;

    template<typename APPLICATION_TYPE>
    friend class ApplicationFactory;

    friend class ApplicationFactoryBase;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_APPLICATION_BASE_H */
