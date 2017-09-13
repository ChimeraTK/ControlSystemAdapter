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
#include <boost/fusion/include/for_each.hpp>
#include <mtca4u/SupportedUserTypes.h>

namespace ChimeraTK {
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
  
  template<class S>
  struct Round {
    static S nearbyint ( S s ){
      return round(s);
    }
    
    typedef boost::mpl::integral_c<std::float_round_style,std::round_to_nearest> round_style ;
  };

  /**
   * Strictly this is not a real decorator. It provides an NDRegisterAccessor of a 
   * different template type and accesses the original one as "backend" under the hood.
   *
   * This is the base class which has pure virtual convertAndCopyFromImpl and convertAndCopyToImpl functions.
   * Apart from this it implements all decorating functions which just pass through to the impl.
   * It intentionally does not call shutdown in the constructor in order not to shadow the 
   * shutdown detection for the child classes, so it is not forgotten there.
   *
   * This class is not thread-safe and should only be used from a single thread.
   *
   */
  template<class T, class IMPL_T>
  class TypeChangingDecorator : public mtca4u::NDRegisterAccessor<T> {

  public:

    TypeChangingDecorator(boost::shared_ptr< mtca4u::NDRegisterAccessor< IMPL_T> > const & impl) noexcept;

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

    virtual void convertAndCopyFromImpl() = 0;
    virtual void convertAndCopyToImpl() = 0;
    
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

  /** This class is intended as a base class and intentionally does not call shutdown in the destructor in order not to shadow the 
   *  shutdown detection for the child classes, so it is not forgotten there.
   *  It provides only partial template specialisations for the string stuff. Don't directly instantiate this class. The factory would
   *  fail when looping over all implementation types.
   */
  template<class T, class IMPL_T>
  class TypeChangingStringImplDecorator: public TypeChangingDecorator<T, IMPL_T>{
  public:
    using TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator;
  };

  /** The actual partial implementation for strings as impl type **/
  template<class T>
    class TypeChangingStringImplDecorator<T, std::string >: public TypeChangingDecorator<T, std::string>{
  public:
    using TypeChangingDecorator<T, std::string>::TypeChangingDecorator;
    virtual void convertAndCopyFromImpl(){
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->buffer_2D[i][j] = stringToT<T>(this->_impl->accessChannel(i)[j]);
        }
      }
    }
    virtual void convertAndCopyToImpl(){
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->_impl->accessChannel(i)[j] = T_ToString(this->buffer_2D[i][j]);
        }
      }
    }
  };

  /** This decorator uses the boost numeric converter which performs two tasks:
   *
   *  - a range check (throws boost::numeric::bad_numeric_cast if out of range)
   *  - the rounding floating point -> int is done mathematically (not just cut off the bits)
   */
  template<class T, class IMPL_T>
  class TypeChangingRangeCheckingDecorator: public TypeChangingStringImplDecorator<T, IMPL_T>{
  public:
    using TypeChangingStringImplDecorator<T, IMPL_T>::TypeChangingStringImplDecorator;
    virtual void convertAndCopyFromImpl();
    virtual void convertAndCopyToImpl();
    virtual ~TypeChangingRangeCheckingDecorator(){
      this->shutdown();
    }
  private:
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


  };

  template<class T>
    class TypeChangingRangeCheckingDecorator<T, std::string>: public TypeChangingStringImplDecorator<T, std::string>{
  public:
    using TypeChangingStringImplDecorator<T, std::string>::TypeChangingStringImplDecorator;
    virtual void convertAndCopyFromImpl(){
      TypeChangingStringImplDecorator<T, std::string>::convertAndCopyFromImpl();
    }
    virtual void convertAndCopyToImpl(){
      TypeChangingStringImplDecorator<T, std::string>::convertAndCopyToImpl();
    }
    virtual ~TypeChangingRangeCheckingDecorator(){
      this->shutdown();
    }
  };

    
/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T, class IMPL_T>
  TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator(boost::shared_ptr< mtca4u::NDRegisterAccessor< IMPL_T> > const & impl) noexcept
    :  mtca4u::NDRegisterAccessor<T>(impl->getName(), impl->getUnit(), impl->getDescription()),  _impl(impl)
  {
    this->buffer_2D.resize(impl->getNumberOfChannels());
      for (auto & channel : this->buffer_2D){
      channel.resize(impl->getNumberOfSamples());
    }
  }
  
/*********************************************************************************************************************/

  template<class T, class IMPL_T>
  void TypeChangingRangeCheckingDecorator<T, IMPL_T>::convertAndCopyFromImpl() {
    typedef boost::numeric::converter<T, IMPL_T, boost::numeric::conversion_traits<T, IMPL_T>,
      boost::numeric::def_overflow_handler, Round<double> > FromImplConverter;
    //fixme: are iterartors more efficient?
    try{
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->buffer_2D[i][j] = FromImplConverter::convert(this->_impl->accessChannel(i)[j]);
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
  void TypeChangingRangeCheckingDecorator<T, IMPL_T>::convertAndCopyToImpl() {
    typedef boost::numeric::converter<IMPL_T, T, boost::numeric::conversion_traits<IMPL_T, T>,
      boost::numeric::def_overflow_handler, Round<double> > ToImplConverter;
    for (size_t i = 0; i < this->buffer_2D.size(); ++i){
      for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
        this->_impl->accessChannel(i)[j] = ToImplConverter::convert(this->buffer_2D[i][j]);
      }
    }
  }

  /** This decorator does not do mathematical rounding and range checking, but
   *  directly assigns the data types (C-style direct conversion).
   *  Probably the only useful screnario is the conversion int/uint if negative data should be
   *  displayed as the last half of the dynamic range, or vice versa. (e.g. 0xFFFFFFFF <-> -1).
   */
  template<class T, class IMPL_T>
  class TypeChangingDirectCastDecorator : public TypeChangingStringImplDecorator<T, IMPL_T> {
  public:
    using TypeChangingStringImplDecorator<T, IMPL_T>::TypeChangingStringImplDecorator;

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
    ~TypeChangingDirectCastDecorator(){
      this->shutdown();
    }
  };

  /** Partial template specialisation for strings as impl type.
   */
  template<class T>
  class TypeChangingDirectCastDecorator<T, std::string>: public TypeChangingStringImplDecorator<T, std::string>{
  public:
    using TypeChangingStringImplDecorator<T, std::string>::TypeChangingStringImplDecorator;
    virtual void convertAndCopyFromImpl(){
      TypeChangingStringImplDecorator<T, std::string>::convertAndCopyFromImpl();
    }
    virtual void convertAndCopyToImpl(){
      TypeChangingStringImplDecorator<T, std::string>::convertAndCopyToImpl();
    }
    virtual ~TypeChangingDirectCastDecorator(){
      this->shutdown();
    }
  };

  enum class DecoratorType{ range_checking, limiting, C_style_conversion};
  template<class UserType>
  class DecoratorFactory{
  public:
    DecoratorFactory(boost::shared_ptr< mtca4u::TransferElement > theImpl_, DecoratorType wantedDecoratorType_)
      : theImpl(theImpl_), wantedDecoratorType(wantedDecoratorType_){}
    boost::shared_ptr< mtca4u::TransferElement > theImpl;
    DecoratorType wantedDecoratorType;
    mutable boost::shared_ptr<mtca4u::NDRegisterAccessor<UserType> > createdDecorator;
    
    template<typename PAIR>
    void operator()(PAIR&) const {
          typedef typename PAIR::first_type TargetImplType;
          if(typeid(TargetImplType) != theImpl->getValueType() ) return;
          
          if (wantedDecoratorType == DecoratorType::range_checking){
            std::cout << "creating TypeChangingRangeCheckingDecorator " << typeid(UserType).name()
                      << " " <<  typeid(TargetImplType).name() << std::endl;
            createdDecorator.reset( new TypeChangingRangeCheckingDecorator< UserType, TargetImplType> (
              boost::dynamic_pointer_cast< mtca4u::NDRegisterAccessor<TargetImplType> >( theImpl ) ) );
          }else if( wantedDecoratorType == DecoratorType::C_style_conversion){
            std::cout << "creating TypeChangingDirectCastDecorator " << typeid(UserType).name()
                      << " " <<  typeid(TargetImplType).name() << std::endl;
            createdDecorator.reset( new TypeChangingDirectCastDecorator< UserType, TargetImplType> (
              boost::dynamic_pointer_cast< mtca4u::NDRegisterAccessor<TargetImplType> >( theImpl ) )  );
          }else{
            throw mtca4u::NotImplementedException("TypeChangingDecorator with range limitation is not implemented yet.");
          }
          std::cout << "UserType id: " << typeid(UserType).name() << ", TargetType id is " << typeid(TargetImplType).name() << std::endl;
   }

  };


  // the factory function. You don't have to care about the IMPL_Type when requesting a decorator
  template<class UserType>
  boost::shared_ptr< mtca4u::NDRegisterAccessor< UserType > >  getDecorator( mtca4u::TransferElement & transferElement, DecoratorType decoratorType =  DecoratorType::range_checking ){
    DecoratorFactory< UserType > factory( transferElement.getHighLevelImplElement(), decoratorType );
    boost::fusion::for_each(mtca4u::userTypeMap(), factory);
    return factory.createdDecorator;
  }  
  
} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H
