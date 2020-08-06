
#define BOOST_TEST_MODULE TypeChangingDecoratorUnifiedTest
// Only after defining the name include the unit test header.
#include <boost/test/included/unit_test.hpp>

using namespace boost::unit_test_framework;

#include <ChimeraTK/Device.h>
#include <ChimeraTK/TransferGroup.h>
#include <ChimeraTK/BackendFactory.h>
#include <ChimeraTK/DummyBackend.h>
#include <ChimeraTK/DummyRegisterAccessor.h>
#include <ChimeraTK/ExceptionDummyBackend.h>
#include <ChimeraTK/UnifiedBackendTest.h>

#include "TypeChangingDecorator.h"

namespace ChimeraTK {
  using namespace ChimeraTK;
}
using namespace ChimeraTK;

// Create a test suite which holds all your tests.
BOOST_AUTO_TEST_SUITE(TypeChangingDecoratorUnifiedTest)

class DecoratorBackend : public ExceptionDummy {
 public:
  DecoratorBackend(std::string mapFileName) : ExceptionDummy(mapFileName) {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
  }

  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const RegisterPath& registerPathName, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
    if(flags.has(AccessMode::raw)) throw ChimeraTK::logic_error("Raw accessors not supported");

    DecoratorType type;
    auto components = registerPathName.getComponents();
    if(components.back() == "casted")
      type = DecoratorType::C_style_conversion;
    else if(components.back() == "range_checking")
      type = DecoratorType::range_checking;
    else
      throw ChimeraTK::logic_error("Decorator type " + components.back() + " not supported");

    return getDecorator<UserType>(
        ExceptionDummy::getRegisterAccessor_impl<float>(components[0] + "/" + components[1], numberOfWords, wordOffsetInRegister, flags), type);
  }

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string> parameters) {
    return boost::shared_ptr<DeviceBackend>(new DecoratorBackend(parameters["map"]));
  }

  class BackendRegisterer {
   public:
    BackendRegisterer();
  };
  static BackendRegisterer backendRegisterer;

};

template<>
boost::shared_ptr<NDRegisterAccessor<std::string>> DecoratorBackend::getRegisterAccessor_impl(
    const RegisterPath&, size_t, size_t, AccessModeFlags) {
  std::terminate();
}

/********************************************************************************************************************/

DecoratorBackend::BackendRegisterer DecoratorBackend::backendRegisterer;

/********************************************************************************************************************/

DecoratorBackend::BackendRegisterer::BackendRegisterer() {
  std::cout << "DecoratorBackend::BackendRegisterer: registering backend type DecoratorBackend" << std::endl;
  ChimeraTK::BackendFactory::getInstance().registerBackendType("DecoratorBackend", &DecoratorBackend::createInstance);
}
/**********************************************************************************************************************/

static std::string cdd("(DecoratorBackend:1?map=decoratorTest.map)");
static auto exceptionDummy =
    boost::dynamic_pointer_cast<DecoratorBackend>(BackendFactory::getInstance().createBackend(cdd));

/**********************************************************************************************************************/

template<typename T>
struct TestRegister {
  bool isWriteable() { return true; }
  bool isReadable() { return true; }
  ChimeraTK::AccessModeFlags supportedFlags() { return {}; }
  size_t nChannels() { return 1; }
  size_t nElementsPerChannel() { return 1; }
  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  size_t nRuntimeErrorCases() { return 1; }
  typedef T minimumUserType;
  typedef minimumUserType rawUserType;

  static constexpr auto capabilities = TestCapabilities<>()
                                           .disableForceDataLossWrite()
                                           .disableAsyncReadInconsistency()
                                           .disableSwitchReadOnly()
                                           .disableSwitchWriteOnly();

  DummyRegisterAccessor<double> acc{exceptionDummy.get(), "", "/SOME/SCALAR"};

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    UserType val;
    try {
      val = numericToUserType<UserType>(acc + 3);
    }
    catch(boost::numeric::positive_overflow) {
      val = std::numeric_limits<UserType>::min() + 5;
    }
    catch(boost::numeric::negative_overflow) {
      val = std::numeric_limits<UserType>::max() - 5;
    }

    return {{val}};
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    return {{
        numericToUserType<UserType>(static_cast<double>(acc))
    }};
  }

  void setRemoteValue() { acc = generateValue<minimumUserType>()[0][0]; }

  void setForceRuntimeError(bool enable, size_t) {
    exceptionDummy->throwExceptionRead = enable;
    exceptionDummy->throwExceptionWrite = enable;
  }
};

template<typename T>
struct TestRegisterRoCasted : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR_RO/casted"; }
  bool isWriteable() { return false; }
};

template<typename T>
struct TestRegisterCasted : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR/casted"; }
};

template<typename T>
struct TestRegisterRangeChecked : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR/range_checking"; }
};

template<typename T>
struct TestRegisterRoRangeChecked : TestRegister<T> {
  std::string path() { return "/SOME/SCALAR_RO/range_checking"; }
  bool isWriteable() { return false; }
};

template<typename T>
struct TestRegisterRangeCheckedOverflows : TestRegister<T> {
  bool returnOutOfRangeValue{true};
  using TestRegister<T>::minimumUserType;

  std::string path() { return "/SOME/SCALAR/range_checking"; }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
      // Artificially create a value that exceeds the backing register's accessor (float)
      auto value = std::numeric_limits<T>::max();
      return {{value + 1}};
  }

  void setRemoteValue() {
    // Set a remote value that exceeds the limits of the target type
      auto value = std::numeric_limits<T>::max() + 1;
      TestRegister<T>::acc = value;
  }
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(testRegisterAccessor) {
  std::cout << "*** testRegisterAccessor *** " << std::endl;
  ChimeraTK::UnifiedBackendTest<>()
      .testOnlyTransferElement()
      .addRegister<TestRegisterCasted<long>>()
      .addRegister<TestRegisterCasted<double>>()
      .addRegister<TestRegisterRoCasted<long>>()
      .addRegister<TestRegisterRoCasted<double>>()
      .addRegister<TestRegisterRangeChecked<int>>()
      .addRegister<TestRegisterRangeChecked<float>>()
      .addRegister<TestRegisterRoRangeChecked<int>>()
      .addRegister<TestRegisterRoRangeChecked<float>>()
      // Fixme. These two tests will trigger errors all the time
      .addRegister<TestRegisterRangeCheckedOverflows<double>>()
      .addRegister<TestRegisterRangeCheckedOverflows<int8_t>>()
      .runTests(cdd);
}

/**********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
