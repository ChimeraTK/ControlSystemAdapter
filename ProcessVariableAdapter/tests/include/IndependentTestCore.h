#ifndef _INDEPENDENT_CONTOL_CORE_H_
#define _INDEPENDENT_CONTOL_CORE_H_

#include <boost/scoped_ptr.hpp>

#include "DevicePVManager.h"
#include "ProcessScalar.h"
#include "DeviceSynchronizationUtility.h"
#include "SynchronizationDirection.h"

#include <limits>

#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/fusion/include/for_each.hpp>

template<class DataType>
struct TypedPVHolder{
  typename mtca4u::ProcessScalar<DataType>::SharedPtr toDeviceScalar;
  typename mtca4u::ProcessScalar<DataType>::SharedPtr fromDeviceScalar;
  /** The "data type constant" is a value that depends on the data type. It is intended as
   *  'magic' constant which can be read out to test reading because the value it knows.
   *  
   *  The value is:
   *  \li sizeof(type) for unsigned integer types
   *  \li -sizeof(type) for signed integer types
   *  \li 1./sizeof(type) for floating point types
   */
  typename mtca4u::ProcessScalar<DataType>::SharedPtr dataTypeConstant;

  TypedPVHolder(boost::shared_ptr<mtca4u::DevicePVManager> const & processVariableManager, std::string typeNamePrefix):
    toDeviceScalar( processVariableManager->createProcessScalar<DataType>(mtca4u::controlSystemToDevice, typeNamePrefix + "/TO_DEVICE_SCALAR") ),
    fromDeviceScalar( processVariableManager->createProcessScalar<DataType>(mtca4u::deviceToControlSystem, typeNamePrefix + "/FROM_DEVICE_SCALAR") ),
    dataTypeConstant( processVariableManager->createProcessScalar<DataType>(mtca4u::deviceToControlSystem, typeNamePrefix + "/DATA_TYPE_CONSTANT") ){
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
    }

  void inputToOutput(){
    fromDeviceScalar->set(*toDeviceScalar);
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

  //  boost::scoped_ptr< boost::thread > _deviceThread;
  HolderMap holderMap;

  // the syncUtil needs to be initalised after the PVs are added to the manager
  mtca4u::DeviceSynchronizationUtility syncUtil;

  // void mainLoop();

  /// The 'body' of the main loop, i.e. the functionality once, without the loop around it.
  void mainBody();

  /** The constructor gets an instance of the variable factory to use. 
   *  The variables in the factory should already be initialised because the hardware is initialised here.
   */
  IndependentTestCore(boost::shared_ptr<mtca4u::DevicePVManager> const & processVariableManager_)
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


    // start the device thread, which is executing the main loop
    //_deviceThread.reset( new boost::thread( boost::bind( &IndependentTestCore::mainLoop, this ) ) );
  }
  
  ~IndependentTestCore(){
    // stop the device thread before any other destructors are called
    //    _deviceThread->interrupt();
    //    _deviceThread->join();
 }

};

//inline void IndependentTestCore::mainLoop(){
//  mtca4u::DeviceSynchronizationUtility syncUtil(_processVariableManager);
// 
//  while (!boost::this_thread::interruption_requested()) {
//    syncUtil.receiveAll();
//    *_monitorVoltage = _hardware.getVoltage();	  (*dataTypeConstant) = -size_of(DataType);
//
//    _hardware.setVoltage( *_targetVoltage );
//    syncUtil.sendAll();
//    boost::this_thread::sleep_for( boost::chrono::milliseconds(100) );
//  }
//}

struct PerformInputToOutput{
  template< typename T >
  void operator()( T& t ) const{
    t.second.inputToOutput();
  }
};

inline void IndependentTestCore::mainBody(){
  
  syncUtil.receiveAll();
 
  //  boost::fusion::at_key<int32_t>( holderMap ).inputToOutput();

  for_each( holderMap, PerformInputToOutput() ),

  syncUtil.sendAll();
}

#endif // _INDEPENDENT_CONTOL_CORE_H_
