/*
 * ApplicationBase.cc
 *
 *  Created on: Nov 11, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationBase.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  ApplicationBase *ApplicationBase::instance = nullptr;
  std::mutex ApplicationBase::instance_mutex;

  /*********************************************************************************************************************/

  ApplicationBase::ApplicationBase(const std::string& name)
  : applicationName(name)
  {
    std::lock_guard<std::mutex> lock(instance_mutex);
    if(instance != nullptr) {
      throw ChimeraTK::logic_error("Multiple instances of ChimeraTK::ApplicationBase cannot be created.");
    }
    instance = this;
  }

  /*********************************************************************************************************************/

  ApplicationBase::~ApplicationBase() {
    if(!hasBeenShutdown) {
      std::cerr << "*****************************************************************************" << std::endl;
      std::cerr << " BUG found in application " << applicationName << "!" << std::endl;
      std::cerr << " Its implementation of the class Application must have a destructor which" << std::endl;
      std::cerr << " calls Application::shutdown()." << std::endl;
      std::cerr << " Since the application was not shutdown properly, we are now about to crash." << std::endl;
      std::cerr << " Please fix your application!" << std::endl;
      std::cerr << "*****************************************************************************" << std::endl;
      std::terminate();
    }
  }

  /*********************************************************************************************************************/

  void ApplicationBase::shutdown() {

    // finally clear the global instance pointer and mark this instance as shut down
    std::lock_guard<std::mutex> lock(instance_mutex);
    instance = nullptr;
    hasBeenShutdown = true;

  }

  /*********************************************************************************************************************/

  void ApplicationBase::setPVManager(boost::shared_ptr<ChimeraTK::DevicePVManager> const &processVariableManager) {
    _processVariableManager = processVariableManager;
  }

  /*********************************************************************************************************************/

  ApplicationBase& ApplicationBase::getInstance() {
    if(instance == nullptr) {
      throw ChimeraTK::logic_error("No instance of ChimeraTK::ApplicationBase created, but ApplicationBase::getInstance() called.");
    }
    return *instance;
  }

} /* namespace ChimeraTK */
