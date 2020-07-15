#define BOOST_TEST_MODULE ProcessArrayUnifiedTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include <ChimeraTK/DeviceBackendImpl.h>
#include <ChimeraTK/UnifiedBackendTest.h>
#include "UnidirectionalProcessArray.h"
#include "BidirectionalProcessArray.h"

using namespace ChimeraTK;

BOOST_AUTO_TEST_SUITE(ProcessArrayUnifiedTestSuite)

struct ProcessArrayFactoryBackend;

static boost::shared_ptr<ProcessArrayFactoryBackend> backend;

static uint32_t counter{0}; // used to generate values
std::string generateValueFromCounter() {
  return "Some string " + std::to_string(counter);
}

/**********************************************************************************************************************/
/* Pseudo backend which hands out ProcessArrays and otherwise just satisfies the UnifiedBackendTest */
struct ProcessArrayFactoryBackend : DeviceBackendImpl {
  ProcessArrayFactoryBackend() : DeviceBackendImpl() {
    FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
  }
  ~ProcessArrayFactoryBackend() override {}

  void open() override { _opened = true; }

  void close() override { _opened = false; }

  std::string readDeviceInfo() override { return "ProcessArrayFactoryBackend"; }

  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string>) {
    return boost::shared_ptr<DeviceBackend>(new ProcessArrayFactoryBackend());
  }

  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(SubdeviceBackend, getRegisterAccessor_impl, 4);
  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const RegisterPath&, size_t, size_t, AccessModeFlags) {
    assert(false); // wrong type, see specialisation
  }

  bool isFunctional() const override { return _opened; }

  void setException() override {}

  void activateAsyncRead() noexcept override {}

  boost::shared_ptr<ProcessArray<std::string>> _pv;

  class BackendRegisterer {
   public:
    BackendRegisterer();
  };
  static BackendRegisterer backendRegisterer;
};

/********************************************************************************************************************/

ProcessArrayFactoryBackend::BackendRegisterer ProcessArrayFactoryBackend::backendRegisterer;

ProcessArrayFactoryBackend::BackendRegisterer::BackendRegisterer() {
  std::cout << "ProcessArrayFactoryBackend::BackendRegisterer: registering backend type ProcessArrayFactory"
            << std::endl;
  ChimeraTK::BackendFactory::getInstance().registerBackendType(
      "ProcessArrayFactory", &ProcessArrayFactoryBackend::createInstance);
}

/********************************************************************************************************************/

template<>
boost::shared_ptr<NDRegisterAccessor<std::string>> ProcessArrayFactoryBackend::getRegisterAccessor_impl<std::string>(
    const RegisterPath& path, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
  if(numberOfWords > 1 || wordOffsetInRegister != 0) throw ChimeraTK::logic_error("Bad dimension");
  flags.checkForUnknownFlags({AccessMode::wait_for_new_data});

  // create a push and a poll pair always for convenience, only one of them will be used
  auto poll = createSynchronizedProcessArray<std::string>(1, "/unidir/poll", "", "", "", 3, nullptr, {});
  auto push = createSynchronizedProcessArray<std::string>(
      1, "/unidir/push", "", "", "", 3, nullptr, {AccessMode::wait_for_new_data});

  if(path == "/unidir/sender") {
    flags.checkForUnknownFlags({});
    _pv = push.second;
    return push.first;
  }
  if(path == "/unidir/receiver" && flags.has(AccessMode::wait_for_new_data)) {
    _pv = push.first;
    // need to write initial value
    _pv->accessData(0) = generateValueFromCounter();
    _pv->write();
    return push.second;
  }
  if(path == "/unidir/receiver" && !flags.has(AccessMode::wait_for_new_data)) {
    _pv = poll.first;
    // need to write initial value, otherwise the first read would block
    _pv->accessData(0) = generateValueFromCounter();
    _pv->write();
    return poll.second;
  }
  /*if(path == "/bidir/A") {
    return boost::make_shared<ThrowIfClosedDecorator<std::string>>(_biA);
  }
  if(path == "/bidir/B") {
    return boost::make_shared<ThrowIfClosedDecorator<std::string>>(_biB);
  }*/

  throw ChimeraTK::logic_error("Bad path");
}

/**********************************************************************************************************************/
/* First a base descriptors is defined to simplify the descriptors for the individual registers. */

/// Base descriptor with defaults, used for all registers
template<typename Derived>
struct RegisterDescriptorBase {
  Derived* derived{static_cast<Derived*>(this)};

  typedef std::string minimumUserType;
  typedef minimumUserType rawUserType;

  bool isPush() { return true; }

  ChimeraTK::AccessModeFlags supportedFlags() {
    ChimeraTK::AccessModeFlags flags{ChimeraTK::AccessMode::wait_for_new_data};
    return flags;
  }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    ++counter;
    return {{generateValueFromCounter()}};
  }

  size_t writeQueueLength() { return std::numeric_limits<size_t>::max(); }
  void setForceDataLossWrite(bool) { assert(false); }

  size_t nRuntimeErrorCases() { return 0; }
  bool testAsyncReadInconsistency() { return false; }
  [[noreturn]] void setForceRuntimeError(bool, size_t) { assert(false); }
  [[noreturn]] void forceAsyncReadInconsistency() { assert(false); }
};

/**********************************************************************************************************************/

struct UnidirSender : RegisterDescriptorBase<UnidirSender> {
  std::string path() { return "/unidir/sender"; }

  bool isWriteable() { return true; }
  bool isReadable() { return false; }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    backend->_pv->readLatest();
    return backend->_pv->accessChannels();
  }

  [[noreturn]] void setRemoteValue() { assert(false); }
};

/**********************************************************************************************************************/

struct UnidirReceiver : RegisterDescriptorBase<UnidirSender> {
  std::string path() { return "/unidir/receiver"; }

  bool isWriteable() { return false; }
  bool isReadable() { return true; }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    return backend->_pv->accessChannels();
  }

  void setRemoteValue() {
    auto v = this->generateValue<std::string>()[0][0];
    backend->_pv->accessData(0) = v;
    backend->_pv->write();
  }
};

/********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(unifiedBackendTest) {
  std::string cdd = "(ProcessArrayFactory)";

  backend = boost::dynamic_pointer_cast<ProcessArrayFactoryBackend>(BackendFactory::getInstance().createBackend(cdd));

  UnifiedBackendTest<>().testOnlyTransferElement().addRegister<UnidirSender>().addRegister<UnidirReceiver>().runTests(
      cdd);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
