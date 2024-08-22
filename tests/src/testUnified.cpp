#define BOOST_TEST_MODULE ProcessArrayUnifiedTest
#include <boost/test/included/unit_test.hpp>
using namespace boost::unit_test_framework;

#include "BidirectionalProcessArray.h"
#include "UnidirectionalProcessArray.h"

#include <ChimeraTK/DeviceBackendImpl.h>
#include <ChimeraTK/UnifiedBackendTest.h>

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
  ProcessArrayFactoryBackend();

  void open() override { _opened = true; }

  void close() override { _opened = false; }

  std::string readDeviceInfo() override { return "ProcessArrayFactoryBackend"; }

  // NOLINTNEXTLINE(performance-unnecessary-value-param) - signature dictated by DeviceAccess
  static boost::shared_ptr<DeviceBackend> createInstance(std::string, std::map<std::string, std::string>) {
    return boost::shared_ptr<DeviceBackend>(new ProcessArrayFactoryBackend());
  }

  // NOLINTBEGIN(readability-identifier-naming, performance-unnecessary-value-param)
  // Reason: naming convention and signature dictated by DeviceAccess
  DEFINE_VIRTUAL_FUNCTION_TEMPLATE_VTABLE_FILLER(SubdeviceBackend, getRegisterAccessor_impl, 4);
  template<typename UserType>
  boost::shared_ptr<NDRegisterAccessor<UserType>> getRegisterAccessor_impl(
      const RegisterPath&, size_t, size_t, AccessModeFlags) {
    std::terminate(); // wrong type, see specialisation
  }
  // NOLINTEND(readability-identifier-naming, performance-unnecessary-value-param)

  void activateAsyncRead() noexcept override {}

  std::map<std::string, std::vector<boost::shared_ptr<ProcessArray<std::string>>>> pv{};

  [[nodiscard]] RegisterCatalogue getRegisterCatalogue() const override { throw; }

  [[nodiscard]] MetadataCatalogue getMetadataCatalogue() const override { return {}; }

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

ProcessArrayFactoryBackend::ProcessArrayFactoryBackend() : DeviceBackendImpl() {
  FILL_VIRTUAL_FUNCTION_TEMPLATE_VTABLE(getRegisterAccessor_impl);
}

/********************************************************************************************************************/

template<>
// NOLINTNEXTLINE(readability-identifier-naming) - naming convention dictated by DeviceAccess
boost::shared_ptr<NDRegisterAccessor<std::string>> ProcessArrayFactoryBackend::getRegisterAccessor_impl<std::string>(
    const RegisterPath& path, size_t numberOfWords, size_t wordOffsetInRegister, AccessModeFlags flags) {
  if(numberOfWords > 1 || wordOffsetInRegister != 0) {
    throw ChimeraTK::logic_error("Bad dimension");
  }

  std::string initialValue;
  if(pv[path].empty()) {
    initialValue = generateValueFromCounter();
  }
  else {
    initialValue = pv[path].back()->accessData(0);
  }

  if(path == "/unidir/sender") {
    flags.checkForUnknownFlags({}); // test expects that write-only accessors never accept wait_for_new_data...
    auto spv = createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, {AccessMode::wait_for_new_data});
    pv[path].push_back(spv.second);
    return spv.first;
  }
  if(path == "/unidir/polledSender") {
    flags.checkForUnknownFlags({}); // test expects that write-only accessors never accept wait_for_new_data...
    auto spv = createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, {});
    pv[path].push_back(spv.second);
    return spv.first;
  }
  if(path == "/unidir/receiver" && flags.has(AccessMode::wait_for_new_data)) {
    auto spv = createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, flags);
    pv[path].push_back(spv.first);
    // need to write initial value
    pv[path].back()->accessData(0) = initialValue;
    pv[path].back()->write();
    return spv.second;
  }
  if(path == "/unidir/receiver" && !flags.has(AccessMode::wait_for_new_data)) {
    auto spv = createSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, flags);
    pv[path].push_back(spv.first);
    // need to write initial value, otherwise the first read would block
    pv[path].back()->accessData(0) = initialValue;
    pv[path].back()->write();
    return spv.second;
  }
  if(path == "/bidir/A") {
    flags.add(AccessMode::wait_for_new_data);
    auto spv = createBidirectionalSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, flags);
    pv[path].push_back(spv.first);
    // need to write initial value
    pv[path].back()->accessData(0) = initialValue;
    pv[path].back()->write();
    return spv.second;
  }
  if(path == "/bidir/B") {
    flags.add(AccessMode::wait_for_new_data);
    auto spv = createBidirectionalSynchronizedProcessArray<std::string>(1, path, "", "", "", 3, flags);
    pv[path].push_back(spv.second);
    // need to write initial value
    pv[path].back()->accessData(0) = initialValue;
    pv[path].back()->write();
    return spv.first;
  }

  throw ChimeraTK::logic_error("Bad path");
}

/**********************************************************************************************************************/
/* First a base descriptors is defined to simplify the descriptors for the individual registers. */

/// Base descriptor with defaults, used for all registers
template<typename Derived>
struct RegisterDescriptorBase {
  Derived* derived{static_cast<Derived*>(this)};

  using minimumUserType = std::string;
  using rawUserType = minimumUserType;

  bool isPush() { return true; }

  ChimeraTK::AccessModeFlags supportedFlags() { return {ChimeraTK::AccessMode::wait_for_new_data}; }

  template<typename UserType>
  std::vector<std::vector<UserType>> generateValue() {
    ++counter;
    return {{generateValueFromCounter()}};
  }

  static constexpr auto capabilities =
      TestCapabilities<>().disableForceDataLossWrite().disableAsyncReadInconsistency().disableTestCatalogue();

  size_t nRuntimeErrorCases() { return 0; }
  [[noreturn]] void setForceRuntimeError(bool, size_t) { std::terminate(); }
};

/**********************************************************************************************************************/

struct UnidirSender : RegisterDescriptorBase<UnidirSender> {
  static std::string path() { return "/unidir/sender"; }

  static constexpr auto capabilities = RegisterDescriptorBase<UnidirSender>::capabilities.enableForceDataLossWrite();

  static bool isWriteable() { return true; }
  static bool isReadable() { return false; }

  static size_t writeQueueLength() { return 3; }
  void setForceDataLossWrite(bool /*enable*/) {}

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    backend->pv[path()].back()->readLatest();
    return backend->pv[path()].back()->accessChannels();
  }

  [[noreturn]] static void setRemoteValue() { std::terminate(); }
};

/**********************************************************************************************************************/

struct UnidirPolledSender : RegisterDescriptorBase<UnidirPolledSender> {
  static std::string path() { return "/unidir/polledSender"; }

  static constexpr auto capabilities =
      RegisterDescriptorBase<UnidirPolledSender>::capabilities.enableTestWriteNeverLosesData();

  static bool isWriteable() { return true; }
  static bool isReadable() { return false; }

  static size_t writeQueueLength() { return 3; }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    backend->pv[path()].back()->readLatest();
    return backend->pv[path()].back()->accessChannels();
  }

  [[noreturn]] static void setRemoteValue() { std::terminate(); }
};

/**********************************************************************************************************************/

struct UnidirReceiver : RegisterDescriptorBase<UnidirReceiver> {
  static std::string path() { return "/unidir/receiver"; }

  static bool isWriteable() { return false; }
  static bool isReadable() { return true; }

  template<typename UserType>
  std::vector<std::vector<UserType>> getRemoteValue() {
    return backend->pv[path()].back()->accessChannels();
  }

  void setRemoteValue() {
    auto v = this->generateValue<std::string>()[0][0];
    for(auto& pv : backend->pv[path()]) {
      pv->accessData(0) = v;
      pv->write();
    }
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
    backend->pv[RegisterDescriptorBase<Derived>::derived->path()].back()->readLatest();
    return backend->pv[RegisterDescriptorBase<Derived>::derived->path()].back()->accessChannels();
  }

  void setRemoteValue() {
    auto v = this->template generateValue<std::string>()[0][0];
    for(auto& pv : backend->pv[RegisterDescriptorBase<Derived>::derived->path()]) {
      pv->accessData(0) = v;
      pv->write();
    };
  }
};

/**********************************************************************************************************************/

struct BidirA : Bidir<BidirA> {
  static std::string path() { return "/bidir/A"; }
};

/**********************************************************************************************************************/

struct BidirB : Bidir<BidirB> {
  static std::string path() { return "/bidir/B"; }
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
