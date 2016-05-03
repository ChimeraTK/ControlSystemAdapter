#ifndef _INDEPENDENT_CONTOL_CORE_H_
#define _INDEPENDENT_CONTOL_CORE_H_

#include <boost/scoped_ptr.hpp>

#include "DevicePVManager.h"
#include "ProcessScalar.h"
#include "DeviceSynchronizationUtility.h"
#include "SynchronizationDirection.h"

#include <limits>

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
      /*      if (std::numeric_limits<DataType>.is_integer){
	if (std::numeric_limits<DataType>.is_signed){
	  // signed int
	  (*dataTypeConstant) = -size_of(DataType);
	}else{
	  // unsigned int
	  (*dataTypeConstant) = -size_of(DataType);
	}else{
	  // floating point
	  (*dataTypeConstant) = 1./size_of(DataType);	  
	}
	}*/
    }

  void inputToOutput(){
    fromDeviceScalar->set(*toDeviceScalar);
  }
};

class IndependentTestCore{
 public:
  mtca4u::DevicePVManager::SharedPtr processVariableManager;

  //  boost::scoped_ptr< boost::thread > _deviceThread;
  TypedPVHolder<int> intHolder;
  TypedPVHolder<short> shortHolder;

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
      intHolder( processVariableManager, "INT"),
      shortHolder( processVariableManager, "SHORT"),
      syncUtil(processVariableManager){

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

inline void IndependentTestCore::mainBody(){
  syncUtil.receiveAll();

  intHolder.inputToOutput();
  shortHolder.inputToOutput();

  syncUtil.sendAll();
}

#endif // _INDEPENDENT_CONTOL_CORE_H_
