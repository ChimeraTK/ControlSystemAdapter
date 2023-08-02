/*
 * ApplicationBase.cc
 *
 *  Created on: Nov 11, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationBase.h"

#include "ApplicationFactory.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  ApplicationBase* ApplicationBase::instance = nullptr;
  std::recursive_mutex ApplicationBase::instanceMutex;

  /*********************************************************************************************************************/

  ApplicationBase::ApplicationBase(const std::string& name) : applicationName(name) {
    std::lock_guard<std::recursive_mutex> lock(instanceMutex);
    // Protection against creating multiple instances manually
    if(instance != nullptr) {
      throw ChimeraTK::logic_error("Multiple instances of ChimeraTK::ApplicationBase cannot be created.");
    }
    // Protection against manual creation when a factory exists.
    if(ApplicationFactoryBase::_factoryFunction && !ApplicationFactoryBase::_factoryIsCreating) {
      std::cerr << "***************************************************************"
                   "*************************************"
                << std::endl;

      std::cerr << "Logic error when creating an ApplicationBase: An ApplicationFactory already exists." << std::endl;
      std::cerr << "You cannot use directly created Applications and an ApplicationFactory at the same time."
                << std::endl
                << std::endl;
      std::cerr << "*** Remove all directly created (probably static) instances of the Application and only use the "
                   "ApplicationFactory! ***"
                << std::endl
                << std::endl;
      std::cerr << "***************************************************************"
                   "*************************************"
                << std::endl;
      throw ChimeraTK::logic_error(
          "Directly creating a ChimeraTK::Application when a ChimeraTK::ApplicationFactory exists is not allowed.");
    }
    instance = this;
  }

  /*********************************************************************************************************************/

  ApplicationBase::~ApplicationBase() {
    if(!hasBeenShutdown) {
      std::cerr << "*************************************************************"
                   "****************"
                << std::endl;
      std::cerr << " BUG found in application " << applicationName << "!" << std::endl;
      std::cerr << " Its implementation of the class Application must have a "
                   "destructor which"
                << std::endl;
      std::cerr << " calls Application::shutdown()." << std::endl;
      std::cerr << " Since the application was not shutdown properly, we are now "
                   "about to crash."
                << std::endl;
      std::cerr << " Please fix your application!" << std::endl;
      std::cerr << "*************************************************************"
                   "****************"
                << std::endl;
      std::terminate();
    }
  }

  /*********************************************************************************************************************/

  void ApplicationBase::shutdown() {
    // finally clear the global instance pointer and mark this instance as shut
    // down
    std::lock_guard<std::recursive_mutex> lock(instanceMutex);
    instance = nullptr;
    hasBeenShutdown = true;
  }

  /*********************************************************************************************************************/

  void ApplicationBase::setPVManager(boost::shared_ptr<ChimeraTK::DevicePVManager> const& processVariableManager) {
    _processVariableManager = processVariableManager;
  }

  /*********************************************************************************************************************/

  ApplicationBase& ApplicationBase::getInstance() {
    if(instance == nullptr) {
      return ApplicationFactoryBase::getApplicationInstance();
    }
    return *instance;
  }

} /* namespace ChimeraTK */
