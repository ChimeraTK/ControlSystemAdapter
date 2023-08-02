/*
 * ApplicationBase.cc
 *
 *  Created on: Nov 11, 2016
 *      Author: Martin Hierholzer
 */

#include "ApplicationFactory.h"

#include "ApplicationBase.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  std::unique_ptr<ApplicationBase> ApplicationFactoryBase::_applicationInstance;
  std::function<void()> ApplicationFactoryBase::_factoryFunction;
  bool ApplicationFactoryBase::_factoryIsCreating{false};

  /*********************************************************************************************************************/

  ApplicationBase& ApplicationFactoryBase::getApplicationInstance() {
    // First check whether the application instance exists before going through the
    // effort of getting the factory lock.
    if(_applicationInstance) {
      return *_applicationInstance;
    }

    std::lock_guard<std::recursive_mutex> lock(ApplicationBase::instanceMutex);
    // Re-check that the application has not been created in the mean time (race condition)
    if(_applicationInstance) {
      return *_applicationInstance;
    }

    if(!_factoryFunction) {
      throw ChimeraTK::logic_error("No instance of ApplicationFactory created, but "
                                   "ApplicationFactoryBase::getApplicationInstance() called.");
    }
    _factoryIsCreating = true;
    _factoryFunction();
    _factoryIsCreating = false;

    return *_applicationInstance;
  }

  /*********************************************************************************************************************/

  ApplicationFactoryBase::~ApplicationFactoryBase() {
    std::lock_guard<std::recursive_mutex> lock(ApplicationBase::instanceMutex);
    _factoryFunction = nullptr;
    _applicationInstance.reset();
  }

} /* namespace ChimeraTK */
