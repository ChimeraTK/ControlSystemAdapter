#ifndef CHIMERATK_TRANSFER_FUTURE_DECORATOR_H
#define CHIMERATK_TRANSFER_FUTURE_DECORATOR_H

#include <mtca4u/TransferFuture.h>

namespace mtca4u {
  class TransferElement;
  class TransferFutureIterator;
}

namespace ChimeraTK {
  template<class T, class IMPL_T>
    class TypeChangingDecorator;
  
  template<class T, class IMPL_T>
  class TransferFutureDecorator: public TransferFuture {
    public:
      
      virtual void wait() override{
        _originalFuture->wait();
        _decorator->convertAndCopyFromImpl();
        _decorator->hasActiveFuture = false;
      }

      TransferFutureDecorator()
        : _originalFuture(nullptr), _decorator(nullptr)
      {}

      /** Constructor accepting another transfer future. I think I don't need anything else.*/
      TransferFutureDecorator(TransferFuture * futureToBeDecorated, TypeChangingDecorator<T, IMPL_T> *decorator)
        : _originalFuture(futureToBeDecorated), _decorator(decorator)
      {}

      void reset(TransferFuture * futureToBeDecorated, TypeChangingDecorator<T, IMPL_T> *decorator){
        _originalFuture = futureToBeDecorated;
        _decorator = decorator;
      }
      
  protected:
      // the decorated accessor will allways hold the instance of the future, so we just store a pointer
      TransferFuture *_originalFuture;

      // The decorator will always hold the instance of this future, so we can use a plain pointer
      TypeChangingDecorator<T, IMPL_T> *_decorator;
  };

} /* namespace ChimeraTK */

#endif /* CHIMERATK_TRANSFER_FUTURE_DECORATOR_H */
