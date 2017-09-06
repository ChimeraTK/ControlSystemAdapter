#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H

//#include <vector>
//#include <utility>
//#include <limits>
//#include <stdexcept>
//#include <typeinfo>
//
//#include <boost/smart_ptr.hpp>
//#include <boost/shared_ptr.hpp>
//#include <boost/weak_ptr.hpp>
//#include <boost/lockfree/queue.hpp>
//#include <boost/lockfree/spsc_queue.hpp>
//#include <boost/thread/future.hpp>
//
#include <mtca4u/NDRegisterAccessor.h>
//#include <mtca4u/VersionNumber.h>
//
//#include "ProcessVariableListener.h"
//#include "TimeStampSource.h"
//#include "PersistentDataStorage.h"

namespace ChimeraTK {
  
  /**
   * Strictly this is not a real decorator. It provides an NDRegisterAccessor of a 
   * different template type and accesses the original one as "backend" under the hood.
   *
   * This class is not thread-safe and should only be used from a single thread.
   * At the moment, it is only returned by the ControlSystemPVManager.
   */
  template<class T, class IMPL_T>
  class TypeChangingDecorator : public mtca4u::NDRegisterAccessor<T> {

  public:

    TypeChangingDecorator(boost::shared_ptr< mtca4u::NDRegisterAccessor< IMPL_T> > & impl) noexcept;

    ~TypeChangingDecorator();

    bool isReadable() const override {
      return _impl->isReadable();
    }

    bool isWriteable() const override {
      return _impl->isWriteable();
    }

    bool isReadOnly() const override {
      return _impl->isReadOnly();
    }
  
    mtca4u::TimeStamp getTimeStamp() const override {
      return _impl->getTimeStamp();
    }

    ChimeraTK::VersionNumber getVersionNumber() const override {
      return _impl->getVersionNumber();
    }

    void doReadTransfer() override{
      _impl->doReadTransfer();
    }

    bool doReadTransferNonBlocking() override{
      return _impl->doReadTransferNonBlocking();
    }

    bool doReadTransferLatest() override{
      return _impl->doReadTransferLatest();
    }

    void convertAndCopyFromImpl();
    void convertAndCopyToImpl();
    
    void postRead() override{
      _impl->postRead();
      convertAndCopyFromImpl();
    }
    
    void preWrite() override{
      convertAndCopyToImpl();
      _impl->preWrite();
    }

    void postWrite() override{
      _impl->postWrite();
    }
 
    virtual bool write(ChimeraTK::VersionNumber versionNumber={}){
      // don't call preWrite() here. It would trigger the preWrite on the impl, which is also
      // happening when _impl()->write() is called.
      convertAndCopyToImpl();
      return _impl->write(versionNumber);
    }

      bool isArray() const override {
      return _impl->isArray();
    }

    /// @todo FIXME: can we unite stuff in a transfer group? Compare with impl?
    bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const override {
      return _impl->isSameRegister(e);
    }

    std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
      return _impl->getHardwareAccessingElements();
    }
    
    void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement> e) override {
      _impl->replaceTransferElement(e);
    }
    
    void setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) override{
      _impl->setPersistentDataStorage(storage);
    }

  protected:
    boost::shared_ptr< mtca4u::NDRegisterAccessor<IMPL_T> > _impl;
  };

/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T, class IMPL_T>
  TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator(boost::shared_ptr< mtca4u::NDRegisterAccessor< IMPL_T> > & impl) noexcept
    : _impl(impl)
  {
    // FIXME: resize the buffers
    // update the internal data buffer with the values from the impl
    postRead();
  }
  
/*********************************************************************************************************************/

  template<class T, class IMPL_T>
    TypeChangingDecorator<T, IMPL_T>::~TypeChangingDecorator() {
    this->shutdown();
  }
    
/*********************************************************************************************************************/

  template<class T, class IMPL_T>
    void TypeChangingDecorator<T, IMPL_T>::convertAndCopyFromImpl() {
    //FIXME: copy the buffers
  }

  template<class T, class IMPL_T>
    void TypeChangingDecorator<T, IMPL_T>::convertAndCopyToImpl() {
    //FIXME: copy the buffers
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H
