#include "TypeChangingDecorator.h"

namespace ChimeraTK{

// The global instance of the map
std::map< boost::shared_ptr<ChimeraTK::TransferElement>,
          boost::shared_ptr<ChimeraTK::TransferElement> > globalDecoratorMap;

std::map< boost::shared_ptr<ChimeraTK::TransferElement>, boost::shared_ptr<ChimeraTK::TransferElement> > &
getGlobalDecoratorMap(){
  return globalDecoratorMap;
}

}// namespace ChimeraTK
