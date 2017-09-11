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

    virtual void convertAndCopyFromImpl();
    virtual void convertAndCopyToImpl();
    
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
    /** Internal exceptions to overload the what() function of the boost exceptions in order to fill in the variable name.
     *  These exceptions are not part of the external interface and cannot be caught explicitly because they are protected.
     *  Catch boost::numeric::bad_numeric_cast and derrivatives if you want to do error handling.
     */
    template<class BOOST_EXCEPTION_T>
    struct BadNumericCastException: public BOOST_EXCEPTION_T{
      BadNumericCastException(std::string variableName) :
      errorMessage("Exception during type changing conversion in " + variableName + ": " + BOOST_EXCEPTION_T().what()){
      }
      std::string errorMessage;
      virtual const char* what() const throw() override{
        return errorMessage.c_str();
      }
    };


    
    boost::shared_ptr< mtca4u::NDRegisterAccessor<IMPL_T> > _impl;

    template<class S>
    struct Round {
      static S nearbyint ( S s ){
        return round(s);
      }

      typedef boost::mpl::integral_c<std::float_round_style,std::round_to_nearest> round_style ;
    };

  };

/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T, class IMPL_T>
  TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator(boost::shared_ptr< mtca4u::NDRegisterAccessor< IMPL_T> > & impl) noexcept
    :  mtca4u::NDRegisterAccessor<T>(impl->getName(), impl->getUnit(), impl->getDescription()),  _impl(impl)
  {
    this->buffer_2D.resize(impl->getNumberOfChannels());
      for (auto & channel : this->buffer_2D){
      channel.resize(impl->getNumberOfSamples());
    }
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
    typedef boost::numeric::converter<T, IMPL_T, boost::numeric::conversion_traits<T, IMPL_T>,
      boost::numeric::def_overflow_handler, Round<double> > FromImplConverter;
    //fixme: are iterartors more efficient?
    try{
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->buffer_2D[i][j] = FromImplConverter::convert(_impl->accessChannel(i)[j]);
        }
      }
    }catch(boost::numeric::positive_overflow &){
      throw( BadNumericCastException<boost::numeric::positive_overflow>( this->getName() ) );
    }catch(boost::numeric::negative_overflow &){
      throw( BadNumericCastException<boost::numeric::negative_overflow>( this->getName() ) );
    }catch(boost::numeric::bad_numeric_cast & boostException){
      throw( BadNumericCastException<boost::numeric::bad_numeric_cast>( this->getName() ) );
    }
  }

  template<class T, class IMPL_T>
  void TypeChangingDecorator<T, IMPL_T>::convertAndCopyToImpl() {
    typedef boost::numeric::converter<IMPL_T, T, boost::numeric::conversion_traits<IMPL_T, T>,
      boost::numeric::def_overflow_handler, Round<double> > ToImplConverter;
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        _impl->accessChannel(i)[j] = ToImplConverter::convert(this->buffer_2D[i][j]);
      }
    }
  }

  template<class T>
    T stringToT(std::string const & input){
    std::stringstream s;
    s << input;
    T t;
    s >> t;
    return t;
  }

  template<class T>
    std::string T_ToString(T input){
    std::stringstream s;
    s << input;
    std::string output;
    s >> output;
    return output;
  }
  
  template<>
  void TypeChangingDecorator<int, std::string>::convertAndCopyFromImpl() {
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        this->buffer_2D[i][j] = stringToT<int>(_impl->accessChannel(i)[j]);
      }
    }
  }
  template<>
  void TypeChangingDecorator<int, std::string>::convertAndCopyToImpl() {
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        _impl->accessChannel(i)[j] = T_ToString(this->buffer_2D[i][j]);
      }
    }
  }
  template<>
  void TypeChangingDecorator<float, std::string>::convertAndCopyFromImpl() {
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        this->buffer_2D[i][j] = stringToT<float>(_impl->accessChannel(i)[j]);
      }
    }
  }
  template<>
  void TypeChangingDecorator<float, std::string>::convertAndCopyToImpl() {
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        _impl->accessChannel(i)[j] = T_ToString(this->buffer_2D[i][j]);
      }
    }
  }
  template<>
  void TypeChangingDecorator<double, std::string>::convertAndCopyFromImpl() {
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        this->buffer_2D[i][j] = stringToT<double>(_impl->accessChannel(i)[j]);
      }
    }
  }
  template<>
  void TypeChangingDecorator<double, std::string>::convertAndCopyToImpl() {
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        _impl->accessChannel(i)[j] = T_ToString(this->buffer_2D[i][j]);
      }
    }
  }

  //Does not do proper type conversion but directly assigns the data types (C-style direct conversion).
  //Probably the only useful screnario is the conversion int/uint if negative data should be
  //displayed as the last half of the dynamic range, or vice versa. (e.g. 0xFFFFFFFF <-> -1).
  template<class T, class IMPL_T>
  class TypeChangingDirectCastDecorator : public TypeChangingDecorator<T, IMPL_T> {
    using TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator;

    void convertAndCopyFromImpl() override {
      //fixme: are iterartors more efficient?
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->buffer_2D[i][j] = this->_impl->accessChannel(i)[j];
        }
      }
    }
    
    void convertAndCopyToImpl() override {
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->_impl->accessChannel(i)[j] = this->buffer_2D[i][j];
        }
      }
    }
  };

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H
