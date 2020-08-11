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
    std::terminate(); // wrong type, see specialisation
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

  if(path == "/unidir/sender") {
    flags.checkForUnknownFlags({}); // test expects that write-only accessors never accept wait_for_new_data...
    auto pv =
        createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, nullptr, {AccessMode::wait_for_new_data});
    _pv = pv.second;
    return pv.first;
  }
  if(path == "/unidir/polledSender") {
    flags.checkForUnknownFlags({}); // test expects that write-only accessors never accept wait_for_new_data...
    auto pv = createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, nullptr, {});
    _pv = pv.second;
    return pv.first;
  }
  if(path == "/unidir/receiver" && flags.has(AccessMode::wait_for_new_data)) {
    auto pv = createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, nullptr, flags);
    _pv = pv.first;
    // need to write initial value
    _pv->accessData(0) = generateValueFromCounter();
    _pv->write();
    return pv.second;
  }
  if(path == "/unidir/receiver" && !flags.has(AccessMode::wait_for_new_data)) {
    auto pv = createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, nullptr, flags);
    _pv = pv.first;
    // need to write initial value, otherwise the first read would block
    _pv->accessData(0) = generateValueFromCounter();
    _pv->write();
    return pv.second;
  }
  if(path == "/bidir/A") {
    flags.add(AccessMode::wait_for_new_data);
    auto pv = createBidirectionalSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, nullptr, nullptr, flags);
    _pv = pv.first;
    // need to write initial value
    _pv->accessData(0) = generateValueFromCounter();
    _pv->write();
    return pv.second;
  }
  if(path == "/bidir/B") {
    flags.add(AccessMode::wait_for_new_data);
    auto pv = createBidirectionalSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, nullptr, nullptr, flags);
    _pv = pv.second;
    // need to write initial value
    _pv->accessData(0) = generateValueFromCounter();
    _pv->write();
    return pv.first;
  }

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

  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::wait_for_new_data}; }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    ++counter;
    return {{generateValueFromCounter()}};
  }

  static constexpr auto capabilities = TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency();

  size_t nRuntimeErrorCases() { return 0; }
  [[noreturn]] void setForceRuntimeError(bool, size_t) { std::terminate(); }
};

/**********************************************************************************************************************/

struct UnidirSender : RegisterDescriptorBase<UnidirSender> {
  std::string path() { return "/unidir/sender"; }

  static constexpr auto capabilities = RegisterDescriptorBase<UnidirSender>::capabilities.enableForceDataLossWrite();

  bool isWriteable() { return true; }
  bool isReadable() { return false; }

  size_t writeQueueLength() { return 3; }
  void setForceDataLossWrite(bool /*enable*/) {}

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    backend->_pv->readLatest();
    return backend->_pv->accessChannels();
  }

  [[noreturn]] void setRemoteValue() { std::terminate(); }
};

/**********************************************************************************************************************/

struct UnidirPolledSender : RegisterDescriptorBase<UnidirPolledSender> {
  std::string path() { return "/unidir/polledSender"; }

  static constexpr auto capabilities =
      RegisterDescriptorBase<UnidirPolledSender>::capabilities.enableTestWriteNeverLosesData();

  bool isWriteable() { return true; }
  bool isReadable() { return false; }

  size_t writeQueueLength() { return 3; }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    backend->_pv->readLatest();
    return backend->_pv->accessChannels();
  }

  [[noreturn]] void setRemoteValue() { std::terminate(); }
};

/**********************************************************************************************************************/

struct UnidirReceiver : RegisterDescriptorBase<UnidirReceiver> {
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

/**********************************************************************************************************************/

template<typename Derived>
struct Bidir : RegisterDescriptorBase<Derived> {
  bool isWriteable() { return true; }
  bool isReadable() { return true; }

  static constexpr auto capabilities = RegisterDescriptorBase<Derived>::capabilities.disableSyncRead();

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    backend->_pv->readLatest();
    return backend->_pv->accessChannels();
  }

  void setRemoteValue() {
    auto v = this->template generateValue<std::string>()[0][0];
    backend->_pv->accessData(0) = v;
    backend->_pv->write();
  }
};

/**********************************************************************************************************************/

struct BidirA : Bidir<BidirA> {
  std::string path() { return "/bidir/A"; }
};

/**********************************************************************************************************************/

struct BidirB : Bidir<BidirA> {
  std::string path() { return "/bidir/B"; }
  // See ProcessArrayFactoryBackend::getRegisterAccessor_impl() for the difference to BidirA.
};

/**********************************************************************************************************************/

BOOST_AUTO_TEST_CASE(unifiedBackendTest) {
  std::string cdd = "(ProcessArrayFactory)";

  backend = boost::dynamic_pointer_cast<ProcessArrayFactoryBackend>(BackendFactory::getInstance().createBackend(cdd));

  UnifiedBackendTest<>()
      .testOnlyTransferElement()
      .addRegister<UnidirSender>()
      .addRegister<UnidirPolledSender>()
      .addRegister<UnidirReceiver>()
      .addRegister<BidirA>()
      .addRegister<BidirB>()
      .runTests(cdd);
}

/********************************************************************************************************************/

BOOST_AUTO_TEST_SUITE_END()
