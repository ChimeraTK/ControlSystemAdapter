#ifndef _INDEPENDENT_CONTOL_CORE_H_
#define _INDEPENDENT_CONTOL_CORE_H_

#include <boost/scoped_ptr.hpp>

#include <ControlSystemAdapter/DevicePVManager.h>
#include <ControlSystemAdapter/ProcessScalar.h>
#include <ControlSystemAdapter/DeviceSynchronizationUtility.h>
#include <ControlSystemAdapter/SynchronizationDirection.h>

#include <limits>
#include <atomic>

#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/for_each.hpp>

template<class DataType>
struct TypedPVHolder{
  typename mtca4u::ProcessScalar<DataType>::SharedPtr toDeviceScalar;
  typename mtca4u::ProcessScalar<DataType>::SharedPtr fromDeviceScalar;
  typename mtca4u::ProcessArray<DataType>::SharedPtr toDeviceArray;
  typename mtca4u::ProcessArray<DataType>::SharedPtr fromDeviceArray;
  /** The "data type constant" is a value that depends on the data type. It is intended as
   *  'magic' constant which can be read out to test reading because the value it knows.
   *  
   *  The value is:
   *  \li sizeof(type) for unsigned integer types
   *  \li -sizeof(type) for signed integer types
   *  \li 1./sizeof(type) for floating point types
   */
  typename mtca4u::ProcessScalar<DataType>::SharedPtr dataTypeConstant;
  typename mtca4u::ProcessArray<DataType>::SharedPtr constantArray;
 
  TypedPVHolder(boost::shared_ptr<mtca4u::DevicePVManager> const & processVariableManager, std::string typeNamePrefix):
    toDeviceScalar( processVariableManager->createProcessScalar<DataType>(mtca4u::controlSystemToDevice, typeNamePrefix + "/TO_DEVICE_SCALAR") ),
    fromDeviceScalar( processVariableManager->createProcessScalar<DataType>(mtca4u::deviceToControlSystem, typeNamePrefix + "/FROM_DEVICE_SCALAR") ),
    toDeviceArray( processVariableManager->createProcessArray<DataType>(mtca4u::controlSystemToDevice, typeNamePrefix + "/TO_DEVICE_ARRAY", 10) ),
    fromDeviceArray( processVariableManager->createProcessArray<DataType>(mtca4u::deviceToControlSystem, typeNamePrefix + "/FROM_DEVICE_ARRAY", 10) ),
    dataTypeConstant( processVariableManager->createProcessScalar<DataType>(mtca4u::deviceToControlSystem, typeNamePrefix + "/DATA_TYPE_CONSTANT") ),
    constantArray( processVariableManager->createProcessArray<DataType>(mtca4u::deviceToControlSystem, typeNamePrefix + "/CONSTANT_ARRAY",10) ){
      if (std::numeric_limits<DataType>::is_integer){
	if (std::numeric_limits<DataType>::is_signed){
	  // signed int
	  (*dataTypeConstant) = static_cast<DataType>(-sizeof(DataType));
	}else{
	  // unsigned int
	  (*dataTypeConstant) = sizeof(DataType);
	}
      }else{
	// floating point
	(*dataTypeConstant) = 1./sizeof(DataType);	  
      }
      for (size_t i = 0; i < constantArray->get().size(); ++i){
	constantArray->get()[i] = (*dataTypeConstant)*i*i;
      }
      
    }

  void inputToOutput(){
    fromDeviceScalar->set(*toDeviceScalar);
    for (size_t i = 0; i < fromDeviceArray->get().size() &&  i < toDeviceArray->get().size() ; ++i){
      fromDeviceArray->get()[i] = toDeviceArray->get()[i];
    }
  }
};

/// A boost fusion map which allows to acces the holder instances by type
typedef boost::fusion::map<
  boost::fusion::pair<int8_t, TypedPVHolder<int8_t> >,
  boost::fusion::pair<uint8_t, TypedPVHolder<uint8_t> >,
  boost::fusion::pair<int16_t, TypedPVHolder<int16_t> >,
  boost::fusion::pair<uint16_t, TypedPVHolder<uint16_t> >,
  boost::fusion::pair<int32_t, TypedPVHolder<int32_t> >,
  boost::fusion::pair<uint32_t, TypedPVHolder<uint32_t> >,
  boost::fusion::pair<float, TypedPVHolder<float> >,
  boost::fusion::pair<double, TypedPVHolder<double> >
  > HolderMap;


class IndependentTestCore{
 public:
  mtca4u::DevicePVManager::SharedPtr processVariableManager;

  boost::scoped_ptr< boost::thread > _deviceThread;
  HolderMap holderMap;

  // the syncUtil needs to be initalised after the PVs are added to the manager
  mtca4u::DeviceSynchronizationUtility syncUtil;

  ///
  static std::atomic_bool & mainBodyCompletelyExecuted(){
    static std::atomic_bool _mainBodyCompletelyExecuted;
    return _mainBodyCompletelyExecuted;
  }

  /// An infinite while loop, running mainBody()
  void mainLoop();

  /// The 'body' of the main loop, i.e. the functionality once, without the loop around it.
  void mainBody();

  /** The constructor gets an instance of the variable factory to use. 
   *  The variables in the factory should already be initialised because the hardware is initialised here.
   *  If needed for the test, a thread can be started which automatically executes the 'mainBody()' function in 
   *  an endless loop.
   */
   IndependentTestCore(boost::shared_ptr<mtca4u::DevicePVManager> const & processVariableManager_,
		       bool startThread = false)
    //initialise all process variables, using the factory
    : processVariableManager( processVariableManager_ ),
    holderMap(
      boost::fusion::make_pair<int8_t>( TypedPVHolder<int8_t>( processVariableManager, "CHAR") ),
      boost::fusion::make_pair<uint8_t>( TypedPVHolder<uint8_t>( processVariableManager, "UCHAR") ),
      boost::fusion::make_pair<int16_t>( TypedPVHolder<int16_t>( processVariableManager, "SHORT") ),
      boost::fusion::make_pair<uint16_t>( TypedPVHolder<uint16_t>( processVariableManager, "USHORT") ),
      boost::fusion::make_pair<int32_t>( TypedPVHolder<int32_t>( processVariableManager, "INT") ),
      boost::fusion::make_pair<uint32_t>( TypedPVHolder<uint32_t>( processVariableManager, "UINT") ),
      boost::fusion::make_pair<float>( TypedPVHolder<float>( processVariableManager, "FLOAT") ),
      boost::fusion::make_pair<double>( TypedPVHolder<double>( processVariableManager, "DOUBLE") )
    ),
    syncUtil(processVariableManager){

    syncUtil.sendAll();

    mainBodyCompletelyExecuted() = true;
    
    // start the device thread, which is executing the main loop
    if (startThread){
      _deviceThread.reset( new boost::thread( boost::bind( &IndependentTestCore::mainLoop, this ) ) );
    }
  }
  
  ~IndependentTestCore(){
    // stop the device thread before any other destructors are called
    if (_deviceThread){
      _deviceThread->interrupt();
      _deviceThread->join();
    }
 }

};

inline void IndependentTestCore::mainLoop(){
 
  while (!boost::this_thread::interruption_requested()) {
    mainBody();
    boost::this_thread::sleep_for( boost::chrono::milliseconds(100) );
  }
}

struct PerformInputToOutput{
  template< typename T >
  void operator()( T& t ) const{
    t.second.inputToOutput();
  }
};

inline void IndependentTestCore::mainBody(){
  // Only set the completed flag if it was 'false' when entering this function.
  // We have to remember this. Like this we can be sure that one full run of this
  // function was executed while the flags was 'false'.
  bool setMainBodyCompletelyEceutedWhenFinished = false;
  if ( !mainBodyCompletelyExecuted() ){
    setMainBodyCompletelyEceutedWhenFinished = true;
  }
  
  syncUtil.receiveAll();
 
  for_each( holderMap, PerformInputToOutput() );

  syncUtil.sendAll();

  // We have reached the end of the mainBody function. If the
  if (setMainBodyCompletelyEceutedWhenFinished){
    mainBodyCompletelyExecuted() = true;
  }
}

#endif // _INDEPENDENT_CONTOL_CORE_H_
