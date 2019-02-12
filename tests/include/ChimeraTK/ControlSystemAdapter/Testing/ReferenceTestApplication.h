#ifndef _REFERENCE_TEST_APPLICATION_H_
#define _REFERENCE_TEST_APPLICATION_H_

#include <boost/scoped_ptr.hpp>

#include <ChimeraTK/ControlSystemAdapter/DevicePVManager.h>
#include <ChimeraTK/ControlSystemAdapter/ProcessArray.h>
#include <ChimeraTK/ControlSystemAdapter/DeviceSynchronizationUtility.h>
#include <ChimeraTK/ControlSystemAdapter/SynchronizationDirection.h>
#include <ChimeraTK/ControlSystemAdapter/ApplicationBase.h>

#include <limits>
#include <atomic>
#include <mutex>

#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/for_each.hpp>

template<class DataType>
struct TypedPVHolder {
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr toDeviceScalar;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr fromDeviceScalar;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr toDeviceArray;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr fromDeviceArray;
  /** The "data type constant" is a value that depends on the data type. It is intended as
   *  'magic' constant which can be read out to test reading because the value it knows.
   *
   *  The value is:
   *  \li sizeof(type) for unsigned integer types
   *  \li -sizeof(type) for signed integer types
   *  \li 1./sizeof(type) for floating point types
   */
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr dataTypeConstant;
  typename ChimeraTK::ProcessArray<DataType>::SharedPtr constantArray;

  TypedPVHolder(boost::shared_ptr<ChimeraTK::DevicePVManager> const& processVariableManager, std::string typeNamePrefix)
  : toDeviceScalar(processVariableManager->createProcessArray<DataType>(ChimeraTK::controlSystemToDevice,
        typeNamePrefix + "/TO_DEVICE_SCALAR",
        1)),
    fromDeviceScalar(processVariableManager->createProcessArray<DataType>(ChimeraTK::deviceToControlSystem,
        typeNamePrefix + "/FROM_DEVICE_SCALAR",
        1)),
    toDeviceArray(processVariableManager->createProcessArray<DataType>(ChimeraTK::controlSystemToDevice,
        typeNamePrefix + "/TO_DEVICE_ARRAY",
        10)),
    fromDeviceArray(processVariableManager->createProcessArray<DataType>(ChimeraTK::deviceToControlSystem,
        typeNamePrefix + "/FROM_DEVICE_ARRAY",
        10)),
    dataTypeConstant(processVariableManager->createProcessArray<DataType>(ChimeraTK::deviceToControlSystem,
        typeNamePrefix + "/DATA_TYPE_CONSTANT",
        1)),
    constantArray(processVariableManager->createProcessArray<DataType>(ChimeraTK::deviceToControlSystem,
        typeNamePrefix + "/CONSTANT_ARRAY",
        10)) {
    if(std::numeric_limits<DataType>::is_integer) {
      if(std::numeric_limits<DataType>::is_signed) {
        // signed int
        dataTypeConstant->accessData(0) = static_cast<DataType>(-sizeof(DataType));
      }
      else {
        // unsigned int
        dataTypeConstant->accessData(0) = sizeof(DataType);
      }
    }
    else {
      // floating point
      dataTypeConstant->accessData(0) = 1. / sizeof(DataType);
    }
    for(size_t i = 0; i < constantArray->accessChannel(0).size(); ++i) {
      constantArray->accessChannel(0)[i] = dataTypeConstant->accessData(0) * i * i;
    }
  }

  void inputToOutput() {
    fromDeviceScalar->accessChannel(0) = toDeviceScalar->accessChannel(0);
    fromDeviceScalar->write();
    for(size_t i = 0; i < fromDeviceArray->accessChannel(0).size() && i < toDeviceArray->accessChannel(0).size(); ++i) {
      fromDeviceArray->accessChannel(0)[i] = toDeviceArray->accessChannel(0)[i];
    }
    fromDeviceArray->write();
  }
};

/// A boost fusion map which allows to acces the holder instances by type
typedef boost::fusion::map<boost::fusion::pair<int8_t, TypedPVHolder<int8_t>>,
    boost::fusion::pair<uint8_t, TypedPVHolder<uint8_t>>,
    boost::fusion::pair<int16_t, TypedPVHolder<int16_t>>,
    boost::fusion::pair<uint16_t, TypedPVHolder<uint16_t>>,
    boost::fusion::pair<int32_t, TypedPVHolder<int32_t>>,
    boost::fusion::pair<uint32_t, TypedPVHolder<uint32_t>>,
    boost::fusion::pair<int64_t, TypedPVHolder<int64_t>>,
    boost::fusion::pair<uint64_t, TypedPVHolder<uint64_t>>,
    boost::fusion::pair<float, TypedPVHolder<float>>,
    boost::fusion::pair<double, TypedPVHolder<double>>>
    HolderMap;

class ReferenceTestApplication : public ChimeraTK::ApplicationBase {
 public:
  // Sets the application into testing mode: The main control loop will stop at the
  // beginning, before executing mainBody.
  static void initialiseManualLoopControl();
  // Goes out of testing mode. The endless main loop starts again.
  static void releaseManualLoopControl();
  // When in testing mode, this runs the main loop exactly one. This is needed for testing.
  // It guarantees that the main body has completed execution and has been run exactly one.
  static void runMainLoopOnce();

  ReferenceTestApplication(std::string const& applicationName_ = "ReferenceTest");
  ~ReferenceTestApplication();

  /// Inherited from ApplicationBase
  void initialise() override;
  /// Inherited from ApplicationBase
  void run() override;

 protected:
  //  ChimeraTK::DevicePVManager::SharedPtr processVariableManager;

  boost::scoped_ptr<boost::thread> _deviceThread;
  boost::scoped_ptr<HolderMap> _holderMap;

  // the syncUtil needs to be initalised after the PVs are added to the manager
  boost::scoped_ptr<ChimeraTK::DeviceSynchronizationUtility> syncUtil;

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

  /// The 'body' of the main loop, i.e. the functionality once, without the loop around it.
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
          boost::fusion::make_pair<double>(TypedPVHolder<double>(_processVariableManager, "DOUBLE"))));
  syncUtil.reset(new ChimeraTK::DeviceSynchronizationUtility(_processVariableManager));
  syncUtil->sendAll();
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
  // we need to call shutdown from ApplicationBase. FIXME: why do we have to do this manually?
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
  template<typename T>
  void operator()(T& t) const {
    t.second.inputToOutput();
  }
};

inline void ReferenceTestApplication::mainBody() {
  syncUtil->receiveAll();
  for_each(*_holderMap, PerformInputToOutput());
}

inline void ReferenceTestApplication::runMainLoopOnce() {
  mainLoopExecutionRequested() = true;
  do {
    mainLoopMutex().unlock();
    boost::this_thread::sleep_for(boost::chrono::microseconds(1));
    mainLoopMutex().lock();
    // Loop until the execution requested flag has not been reset.
    // This is the sign that the loop actually has been performed.
  } while(mainLoopExecutionRequested());
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

#endif // _REFERENCE_TEST_APPLICATION_H_
