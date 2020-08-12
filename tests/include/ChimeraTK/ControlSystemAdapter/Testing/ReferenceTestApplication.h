#ifndef _REFERENCE_TEST_APPLICATION_H_
#define _REFERENCE_TEST_APPLICATION_H_

#include <boost/scoped_ptr.hpp>

#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>
#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>
#include <ChimeraTK/ControlSystemAdapter/DeviceSynchronizationUtility.h>
#include <ChimeraTK/ControlSystemAdapter/ProcessArray.h>
#include <ChimeraTK/ControlSystemAdapter/SynchronizationDirection.h>

#include <atomic>
#include <limits>
#include <mutex>

#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/map.hpp>
#include <boost/optional.hpp>

#include "toType.h"

template<class DataType>
struct TypedPVHolder {
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr toDeviceScalar;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr fromDeviceScalar;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr toDeviceArray;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr fromDeviceArray;
  /** The "data type constant" is a value that depends on the data type. It is
   * intended as 'magic' constant which can be read out to test reading because
   * the value it knows.
   *
   *  The value is:
   *  \li sizeof(type) for unsigned integer types
   *  \li -sizeof(type) for signed integer types
   *  \li 1./sizeof(type) for floating point types
   */
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr dataTypeConstant;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr constantArray;

  std::vector<std::string> failedTransfers{};

  TypedPVHolder(boost::shared_ptr<ChimeraTK::DevicePVManager> const& processVariableManager, std::string typeNamePrefix)
  : toDeviceScalar(processVariableManager->createProcessArray<DataType>(
        ChimeraTK::SynchronizationDirection::controlSystemToDevice, typeNamePrefix + "/TO_DEVICE_SCALAR", 1)),
    fromDeviceScalar(processVariableManager->createProcessArray<DataType>(
        ChimeraTK::SynchronizationDirection::deviceToControlSystem, typeNamePrefix + "/FROM_DEVICE_SCALAR", 1)),
    toDeviceArray(processVariableManager->createProcessArray<DataType>(
        ChimeraTK::SynchronizationDirection::controlSystemToDevice, typeNamePrefix + "/TO_DEVICE_ARRAY", 10)),
    fromDeviceArray(processVariableManager->createProcessArray<DataType>(
        ChimeraTK::SynchronizationDirection::deviceToControlSystem, typeNamePrefix + "/FROM_DEVICE_ARRAY", 10)),
    dataTypeConstant(processVariableManager->createProcessArray<DataType>(
        ChimeraTK::SynchronizationDirection::deviceToControlSystem, typeNamePrefix + "/DATA_TYPE_CONSTANT", 1)),
    constantArray(processVariableManager->createProcessArray<DataType>(
        ChimeraTK::SynchronizationDirection::deviceToControlSystem, typeNamePrefix + "/CONSTANT_ARRAY", 10)) {
    double typeIdentifyingConstant=0;
    if(typeid(DataType) == typeid(std::string)) {
      typeIdentifyingConstant = 42;
    }
    else {
      if(std::numeric_limits<DataType>::is_integer) {
        if(std::numeric_limits<DataType>::is_signed) {
          // signed int
          typeIdentifyingConstant = -static_cast<int64_t>(sizeof(DataType));
        }
        else {
          // unsigned int
          typeIdentifyingConstant = sizeof(DataType);
        }
      }
      else {
        // floating point
        typeIdentifyingConstant = 1. / sizeof(DataType);
      }
    }
    dataTypeConstant->accessData(0) = toType<DataType>(typeIdentifyingConstant);
    
    for(size_t i = 0; i < constantArray->accessChannel(0).size(); ++i) {
      constantArray->accessChannel(0)[i] = toType<DataType>(typeIdentifyingConstant * i * i);
    }
  }

  void inputToOutput(boost::optional<ChimeraTK::VersionNumber> version,  ChimeraTK::DataValidity validity) {
    failedTransfers.clear();
    if (toDeviceScalar->readLatest()) {
      fromDeviceScalar->accessChannel(0) = toDeviceScalar->accessChannel(0);
      fromDeviceScalar->setDataValidity(validity);
      auto isDataLost = fromDeviceScalar->write(version.value_or(ChimeraTK::VersionNumber()));
      if(isDataLost) {
        failedTransfers.emplace_back(toDeviceScalar->getName());
      }
    }

    if (toDeviceArray->readLatest()) {
      for(size_t i = 0; i < fromDeviceArray->accessChannel(0).size() && i < toDeviceArray->accessChannel(0).size(); ++i) {
        fromDeviceArray->accessChannel(0)[i] = toDeviceArray->accessChannel(0)[i];
      }
      fromDeviceArray->setDataValidity(validity);
      auto isDataLost = fromDeviceArray->write(version.value_or(ChimeraTK::VersionNumber()));
      if(isDataLost) {
        failedTransfers.emplace_back(toDeviceArray->getName());
      }
    }

    if(failedTransfers.empty() == false) {
      std::cout << "WARNING: Data loss in referenceTestApplication for Process Variables: " << std::endl;
      for(auto& pv : failedTransfers) {
        std::cout << "\t" << pv;
      }
      std::cout << std::endl;
    }
  }

};

/// A boost fusion map which allows to acces the holder instances by type
/// IMPORTANT: The order in this map determines the order in which the data is
/// processed. This is important for some tests, so do not change the order
/// here!
typedef boost::fusion::map<boost::fusion::pair<int8_t, TypedPVHolder<int8_t>>,
    boost::fusion::pair<uint8_t, TypedPVHolder<uint8_t>>, boost::fusion::pair<int16_t, TypedPVHolder<int16_t>>,
    boost::fusion::pair<uint16_t, TypedPVHolder<uint16_t>>, boost::fusion::pair<int32_t, TypedPVHolder<int32_t>>,
    boost::fusion::pair<uint32_t, TypedPVHolder<uint32_t>>, boost::fusion::pair<int64_t, TypedPVHolder<int64_t>>,
    boost::fusion::pair<uint64_t, TypedPVHolder<uint64_t>>, boost::fusion::pair<float, TypedPVHolder<float>>,
    boost::fusion::pair<double, TypedPVHolder<double>>, boost::fusion::pair<std::string, TypedPVHolder<std::string>>>
    HolderMap;

class ReferenceTestApplication : public ChimeraTK::ApplicationBase {
 public:
  // Sets the application into testing mode: The main control loop will stop at
  // the beginning, before executing mainBody.
  static void initialiseManualLoopControl();
  // Goes out of testing mode. The endless main loop starts again.
  static void releaseManualLoopControl();
  // When in testing mode, this runs the main loop exactly one. This is needed
  // for testing. It guarantees that the main body has completed execution and
  // has been run exactly one.
  static bool runMainLoopOnce();

  ReferenceTestApplication(std::string const& applicationName_ = "ReferenceTest");
  ~ReferenceTestApplication() override;

  /// Inherited from ApplicationBase
  void initialise() override;
  /// Inherited from ApplicationBase
  void run() override;

  boost::optional<ChimeraTK::VersionNumber> versionNumber;

  ChimeraTK::DataValidity dataValidity{ChimeraTK::DataValidity::ok};

  /// Returns a list of process variables for which data transfer failed during the last runMainLoopOnce call.
  std::vector<std::string> getFailedTransfers();

 protected:
  //  ChimeraTK::DevicePVManager::SharedPtr processVariableManager;

  boost::scoped_ptr<boost::thread> _deviceThread;
  boost::scoped_ptr<HolderMap> _holderMap;

  static std::mutex& mainLoopMutex() {
    static std::mutex _mainLoopMutex;
    return _mainLoopMutex;
  }

  static std::atomic_bool& manuallyControlMainLoop() {
    static std::atomic_bool _manuallyControlMainLoop(false);
    return _manuallyControlMainLoop;
  }

  ///
  static bool& mainLoopExecutionRequested() {
    static bool _mainLoopExecutionRequested(false);
    return _mainLoopExecutionRequested;
  }

  static std::atomic_bool& initalisationForManualLoopControlFinished() {
    static std::atomic_bool _initalisationForManualLoopControlFinished(false);
    return _initalisationForManualLoopControlFinished;
  }

  /// An infinite while loop, running mainBody()
  void mainLoop();

  /// The 'body' of the main loop, i.e. the functionality once, without the loop
  /// around it.
  void mainBody();

};

inline ReferenceTestApplication::ReferenceTestApplication(std::string const& applicationName_)
// initialise all process variables, using the factory
: ApplicationBase(applicationName_) {}

inline void ReferenceTestApplication::initialise() {
  // fixme : if ! processVariableManager_ throw
  _holderMap.reset(
      new HolderMap(boost::fusion::make_pair<int8_t>(TypedPVHolder<int8_t>(_processVariableManager, "CHAR")),
          boost::fusion::make_pair<uint8_t>(TypedPVHolder<uint8_t>(_processVariableManager, "UCHAR")),
          boost::fusion::make_pair<int16_t>(TypedPVHolder<int16_t>(_processVariableManager, "SHORT")),
          boost::fusion::make_pair<uint16_t>(TypedPVHolder<uint16_t>(_processVariableManager, "USHORT")),
          boost::fusion::make_pair<int32_t>(TypedPVHolder<int32_t>(_processVariableManager, "INT")),
          boost::fusion::make_pair<uint32_t>(TypedPVHolder<uint32_t>(_processVariableManager, "UINT")),
          boost::fusion::make_pair<int64_t>(TypedPVHolder<int64_t>(_processVariableManager, "LONG")),
          boost::fusion::make_pair<uint64_t>(TypedPVHolder<uint64_t>(_processVariableManager, "ULONG")),
          boost::fusion::make_pair<float>(TypedPVHolder<float>(_processVariableManager, "FLOAT")),
          boost::fusion::make_pair<double>(TypedPVHolder<double>(_processVariableManager, "DOUBLE")),
          boost::fusion::make_pair<std::string>(TypedPVHolder<std::string>(_processVariableManager, "STRING"))));

  for (auto const& variable : _processVariableManager->getAllProcessVariables()) {
    if (variable->isWriteable())
      variable->write();
  }
}

inline void ReferenceTestApplication::run() {
  _deviceThread.reset(new boost::thread(boost::bind(&ReferenceTestApplication::mainLoop, this)));
}

inline ReferenceTestApplication::~ReferenceTestApplication() {
  // stop the device thread before any other destructors are called
  if(_deviceThread) {
    _deviceThread->interrupt();
    _deviceThread->join();
  }
  // we need to call shutdown from ApplicationBase. FIXME: why do we have to do
  // this manually?
  shutdown();
}

inline void ReferenceTestApplication::mainLoop() {
  mainLoopMutex().lock();

  while(!boost::this_thread::interruption_requested()) {
    mainBody();

    if(manuallyControlMainLoop()) {
      mainLoopExecutionRequested() = false;
      initalisationForManualLoopControlFinished() = true;
      do {
        mainLoopMutex().unlock();
        boost::this_thread::sleep_for(boost::chrono::microseconds(10));
        mainLoopMutex().lock();
      } while(!mainLoopExecutionRequested());
    }
    else {
      boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    }
  }

  mainLoopMutex().unlock();
}

struct PerformInputToOutput {
  PerformInputToOutput(boost::optional<ChimeraTK::VersionNumber> version, ChimeraTK::DataValidity validity)
    : _version(version), _validity(validity) {}

  template<typename T>
  void operator()(T& t) const {
    t.second.inputToOutput(_version, _validity);
  }

  boost::optional<ChimeraTK::VersionNumber> _version;
  ChimeraTK::DataValidity _validity;
};

inline void ReferenceTestApplication::mainBody() {
  for_each(*_holderMap, PerformInputToOutput(versionNumber, dataValidity));
}

inline bool ReferenceTestApplication::runMainLoopOnce() {
  mainLoopExecutionRequested() = true;
  do {
    mainLoopMutex().unlock();
    boost::this_thread::sleep_for(boost::chrono::microseconds(1));
    mainLoopMutex().lock();
    // Loop until the execution requested flag has not been reset.
    // This is the sign that the loop actually has been performed.
  } while(mainLoopExecutionRequested());

  auto isSuccessful = [](bool initialState, auto mapEntry){
    bool success = mapEntry.second.failedTransfers.empty();
    return initialState && success;
  };
  auto& holderMap = *dynamic_cast<ReferenceTestApplication&>(ApplicationBase::getInstance())._holderMap;
  return boost::fusion::fold(holderMap, true, isSuccessful);
}

inline void ReferenceTestApplication::initialiseManualLoopControl() {
  manuallyControlMainLoop() = true;
  do {
    boost::this_thread::sleep_for(boost::chrono::milliseconds(10));
  } while(!initalisationForManualLoopControlFinished());
  mainLoopMutex().lock();
}

inline void ReferenceTestApplication::releaseManualLoopControl() {
  manuallyControlMainLoop() = false;
  initalisationForManualLoopControlFinished() = false;
  mainLoopMutex().unlock();
}

inline std::vector<std::string> ReferenceTestApplication::getFailedTransfers(){
  std::vector<std::string> result;
  auto populateFailures = [&](auto mapElement){
    for(auto pv: mapElement.second.failedTransfers){
      result.emplace_back(std::move(pv));
    }
  };
  auto& holderMap = *dynamic_cast<ReferenceTestApplication&>(ApplicationBase::getInstance())._holderMap;
  boost::fusion::for_each(holderMap, populateFailures);
  return result;
}

#endif // _REFERENCE_TEST_APPLICATION_H_
