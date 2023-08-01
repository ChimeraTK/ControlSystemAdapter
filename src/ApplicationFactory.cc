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
  std::mutex ApplicationFactoryBase::_factoryFunctionMutex;
  std::function<void()> ApplicationFactoryBase::_factoryFunction;

  /*********************************************************************************************************************/

  ApplicationBase& ApplicationFactoryBase::getApplicationInstance() {
    // First check whether the application instance exists before going through the
    // effort of getting the factory lock.
    if(_applicationInstance) {
      return *_applicationInstance;
    }

    std::lock_guard<std::mutex> lock(_factoryFunctionMutex);
    // Re-check that the application has not been created in the mean time (race condition)
    if(_applicationInstance) {
      return *_applicationInstance;
    }

    if(!_factoryFunction) {
      throw ChimeraTK::logic_error("No instance of ApplicationFactory created, but "
                                   "ApplicationFactoryBase::getApplicationInstance() called.");
    }
    _factoryFunction();

    return *_applicationInstance;
  }

  /*********************************************************************************************************************/

  ApplicationFactoryBase::~ApplicationFactoryBase() {
    std::lock_guard<std::mutex> lock(_factoryFunctionMutex);
    _factoryFunction = nullptr;
    _applicationInstance.reset();
  }

} /* namespace ChimeraTK */
