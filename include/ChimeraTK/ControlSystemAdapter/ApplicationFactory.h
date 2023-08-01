// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/Exception.h>

#include <functional>
#include <memory>
#include <mutex>

namespace ChimeraTK {

  class ApplicationBase;

  /*********************************************************************************************************************/

  /** Type-less base class. Defines one static instance of the application and a
   *  factory function to create it.
   *  The factory function is set in the constructor of the templated ApplicationFactory
   *  class, which then knows the actual application type and the required constructor arguments.
   */
  class ApplicationFactoryBase {
   public:
    /** Obtain instance of the ApplicationFactory. Will throw an exception if called if no instance has been created. */
    static ApplicationBase& getApplicationInstance();

    ~ApplicationFactoryBase();

   protected:
    /** Pointer to the only instance of the application.*/
    static std::unique_ptr<ApplicationBase> _applicationInstance;

    /** Mutex for thread-safety when setting the function pointer. */
    static std::mutex _factoryFunctionMutex;

    /** Create the actual application and set the instance pointer.
     */
    static std::function<void()> _factoryFunction;
  };

  /*********************************************************************************************************************/

  /** Templated factory that allows to create an application through ApplicationFactoryBase::getApplicationInstance()
   *  without knowing the application's type or constructor arguments.
   *
   *  The code implementing the application will create a static instance of the ApplicationFactory. The
   *  adapter can then determine the time when the application constructor is actually executed. It happens on
   *  the first call to ApplicationBase::getInstance().
   */
  template<typename APPLICATION_TYPE>
  class ApplicationFactory : public ApplicationFactoryBase {
   public:
    /** The constructor arguments of the application are simply passed to the ApplicationFactory.
     */
    template<typename... APPLICATION_ARGS>
    ApplicationFactory(APPLICATION_ARGS... args) {
      if(_factoryFunction) {
        throw ChimeraTK::logic_error("Multiple instances of ChimeraTK::ApplicationFactory cannot be created.");
      }
      std::lock_guard<std::mutex> lock(_factoryFunctionMutex);
      _factoryFunction =
          std::bind(&ApplicationFactory<APPLICATION_TYPE>::factoryFunctionImpl<APPLICATION_ARGS...>, this, args...);
    }

   protected:
    template<typename... APPLICATION_ARGS>
    void factoryFunctionImpl(APPLICATION_ARGS... args) {
      _applicationInstance = std::make_unique<APPLICATION_TYPE>(args...);
    }
  };

} // namespace ChimeraTK
