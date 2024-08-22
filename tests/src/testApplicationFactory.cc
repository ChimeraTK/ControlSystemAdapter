// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later

#define BOOST_TEST_MODULE TestApplicationFactory
// Only after defining the name include the unit test header.
#include "ApplicationFactory.h"
#include "ReferenceTestApplication.h"

#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test_framework;
using namespace ChimeraTK;

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testNoFactoryNoInstance) {
  try {
    (void)ApplicationBase::getInstance();
    BOOST_ERROR("ApplicationBase::getInstance() did not throw as expected.");
  }
  catch(ChimeraTK::logic_error& e) {
    std::cout << "Exception message debug printout: " << e.what() << std::endl;
  }

  try {
    (void)ApplicationFactoryBase::getApplicationInstance();
    BOOST_ERROR("ApplicationFactoryBase::getInstance() did not throw as expected.");
  }
  catch(ChimeraTK::logic_error& e) {
    std::cout << "Exception message debug printout: " << e.what() << std::endl;
  }
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testFactory) {
  ApplicationFactory<ReferenceTestApplication> appFactory;

  auto* firstCall = &ApplicationBase::getInstance();

  auto* referenceAppPointer = dynamic_cast<ReferenceTestApplication*>(firstCall);
  BOOST_TEST(referenceAppPointer != nullptr);

  auto* secondCall = &ApplicationBase::getInstance();

  BOOST_TEST(firstCall == secondCall);
}

/*********************************************************************************************************************/

class AppWithParams : public ReferenceTestApplication {
 public:
  int a;
  float b;
  AppWithParams(int a_, float b_) : a(a_), b(b_) {}
};

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testWithParams) {
  ApplicationFactory<AppWithParams> appFactory(3, 5.8);

  auto* withParams = dynamic_cast<AppWithParams*>(&ApplicationBase::getInstance());
  BOOST_TEST(withParams != nullptr);
  BOOST_TEST(withParams->a == 3);
  BOOST_TEST(withParams->b == 5.8, boost::test_tools::tolerance(0.0001));
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(doubleFactoryInstance) {
  ApplicationFactory<ReferenceTestApplication> appFactory;

  try {
    ApplicationFactory<AppWithParams> appFactory2(3, 5.8);
    BOOST_ERROR("ApplicationFactory constructor did not throw as expected.");
  }
  catch(ChimeraTK::logic_error& e) {
    std::cout << "Exception message debug printout: " << e.what() << std::endl;
  }
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(doubleAppInstance) {
  ReferenceTestApplication app1;

  try {
    AppWithParams app2(3, 5.8);
    BOOST_ERROR("ApplicationFactory constructor did not throw as expected.");
  }
  catch(ChimeraTK::logic_error& e) {
    std::cout << "Exception message debug printout: " << e.what() << std::endl;
  }
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(appPlusFactory) {
  ReferenceTestApplication app;

  try {
    ApplicationFactory<ReferenceTestApplication> appFactory;
    BOOST_ERROR("ApplicationFactory constructor did not throw as expected.");
  }
  catch(ChimeraTK::logic_error& e) {
    std::cout << "Exception message debug printout: " << e.what() << std::endl;
  }
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(factoryPlusApp) {
  ApplicationFactory<ReferenceTestApplication> appFactory;

  try {
    ReferenceTestApplication app;
    BOOST_ERROR("ReferenceTestApplication constructor did not throw as expected.");
  }
  catch(ChimeraTK::logic_error& e) {
    std::cout << "Exception message debug printout: " << e.what() << std::endl;
  }
}

/*********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testNoFactory) {
  // Legacy use case: There is an instance of the application, but no factory.

  ReferenceTestApplication app;

  BOOST_CHECK_THROW((void)ApplicationFactoryBase::getApplicationInstance(), ChimeraTK::logic_error);

  BOOST_TEST(&app == &ApplicationBase::getInstance());
}
