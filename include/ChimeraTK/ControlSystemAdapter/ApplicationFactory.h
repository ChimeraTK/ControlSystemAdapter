// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include "ApplicationBase.h"

#include <ChimeraTK/Exception.h>

#include <functional>
#include <memory>
#include <mutex>

namespace ChimeraTK {


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

    /** Create the actual application and set the instance pointer.
     */
    static std::function<void()> _factoryFunction;

    /** Flag to signal to the ApplicationBase constructor that it is being created by a factory.
     *  Protected by the ApplicationBase::instanceMutex.
     */
    static bool _factoryIsCreating;

    friend class ApplicationBase;
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
      // Use the ApplicatioBase instance mutex to prevent concurrent creation of Applications outside a factory and of other factories
      std::lock_guard<std::recursive_mutex> lock(ApplicationBase::instanceMutex);
      if(ApplicationBase::instance) {
        std::cerr << "*************************************************************"
                     "***************************************"
                  << std::endl;
        std::cerr << "Logic error when creating an ChimeraTK::ApplicationFactory: An instance of "
                     "ChimeraTK::ApplicationBase already exists."
                  << std::endl;
        std::cerr << "You cannot use directly created Applications and an ApplicationFactory at the same time."
                  << std::endl
                  << std::endl;
        std::cerr << "*** Remove all directly created (probably static) instances of the Application and only use the "
                     "ApplicationFactory! ***"
                  << std::endl
                  << std::endl;
        std::cerr << "*************************************************************"
                     "***************************************"
                  << std::endl;
        throw ChimeraTK::logic_error(
            "Creating a ChimeraTK::ApplicationFactory when a ChimeraTK::Application already exists is not "
            "allowed.");
      }
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
