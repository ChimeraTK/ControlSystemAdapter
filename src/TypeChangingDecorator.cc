#include "TypeChangingDecorator.h"

namespace ChimeraTK{

// The global instance of the map
std::map< boost::shared_ptr<mtca4u::TransferElement>,
          boost::shared_ptr<mtca4u::TransferElement> > globalDecoratorMap;

std::map< boost::shared_ptr<mtca4u::TransferElement>, boost::shared_ptr<mtca4u::TransferElement> > &
getGlobalDecoratorMap(){
  return globalDecoratorMap;
}

}// namespace ChimeraTK
