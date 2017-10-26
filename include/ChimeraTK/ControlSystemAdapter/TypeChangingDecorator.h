#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H

#include <mtca4u/NDRegisterAccessor.h>
#include <boost/fusion/include/for_each.hpp>
#include <mtca4u/SupportedUserTypes.h>

namespace ChimeraTK {
  /** There are three types of TypeChanging decorators which do different data conversions
   *  from the user data type to the implementation data type.
   *  
   *  \li range_checking This decorator checks the limits and throws an exception of the range is exceeced (for instance if you cast a negative integer to an unsiged int, or 500 to int8_t).
   *      This decorator does proper rounding from floating point types t o integer (e.g. 6.7 to 7).
   *  \li limiting This decorator limits the data to the maximum possible in the target data type (for instance 500 will result in 127 in int8_t and  255 in uint8_t, -200 will be -128 in int8t, 0 in uint8_t).
   *      This decorator also does correct rounding from floating point to integer type.
   *  \li C_style_conversion This decorator does a direct cast like an assigment in C/C++ does it. For instance 500 (=0x1f4) will result in 0xf4 for an 8 bit integer, which is interpreted as 244 in uint8_t and -12 in int8_t. Digits after the decimal point are cut when converting a floating point value to an integer type. This decorator can be useful to display unsigned integers which use the full dynamic range in a control system which only supports signed data types (the user has to correctly interpret the 'wrong' representation), of for bit fields where it is acceptable to lose the higher bits.
   */
  enum class DecoratorType{ range_checking, limiting, C_style_conversion};

  /** The factory function for type changing decorators.
   * 
   *  TypeChanging decorators take a transfer element (usually NDRegisterAccessor<ImplType>) and wraps it in an NDRegisterAccessor<UserType>.
   *  It automatically performs the right type conversion (configurable as argument of the factory function).
   *  The decorator has it's own buffer of type UserType and synchronises it in the preWrite() and postRead() functions with the implementation.
   *  You don't have to care about the implementation type of the transfer element.
   *  The factory will automatically create the correct decorator.
   *
   *  @param transferElement The TransferElement to be decorated. It can either be an NDRegisterAccessor (usually the case) or and NDRegisterAccessorBridge (but here the user already picks the type he wants).
   *  @param decoratorType The type of decorator you want (see description of DecoratorType)
   */
  template<class UserType>
  boost::shared_ptr< mtca4u::NDRegisterAccessor< UserType > >  getDecorator( mtca4u::TransferElement & transferElement, DecoratorType decoratorType =  DecoratorType::range_checking );

  //===================================================================================================================================================
  //Everything from here are implementation details. The exact identity of the decorator should not matter to the user, it would break the abstraction.
  //===================================================================================================================================================  

  // An intermediate base class which is only typed on the user type and has a function to
  // provide the decorator type. It is needed to have something to cast to when the
  // IMPL_T has already been abstracted away (the implementations are all templated to two parameters)
  template<class T>
  class DecoratorTypeAccessor : public mtca4u::NDRegisterAccessor<T>{
  public:
    using mtca4u::NDRegisterAccessor<T>::NDRegisterAccessor;
    virtual DecoratorType getDecoratorType() const = 0;
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
  class TypeChangingDecorator : public DecoratorTypeAccessor<T> {

  public:

    TypeChangingDecorator(boost::shared_ptr< mtca4u::NDRegisterAccessor< IMPL_T> > const & impl) noexcept;

    virtual bool isReadable() const override {
      return _impl->isReadable();
    }

    virtual bool isWriteable() const override {
      return _impl->isWriteable();
    }

    virtual bool isReadOnly() const override {
      return _impl->isReadOnly();
    }
  
    virtual mtca4u::TimeStamp getTimeStamp() const override {
      return _impl->getTimeStamp();
    }

    virtual ChimeraTK::VersionNumber getVersionNumber() const override {
      return _impl->getVersionNumber();
    }

    virtual void doReadTransfer() override{
      _impl->doReadTransfer();
    }

    virtual bool doReadTransferNonBlocking() override{
      return _impl->doReadTransferNonBlocking();
    }

    virtual bool doReadTransferLatest() override{
      return _impl->doReadTransferLatest();
    }

    virtual void convertAndCopyFromImpl() = 0;
    virtual void convertAndCopyToImpl() = 0;
    
    virtual void postRead() override{
      _impl->postRead();
      convertAndCopyFromImpl();
    }
    
    virtual void preWrite() override{
      convertAndCopyToImpl();
      _impl->preWrite();
    }

    virtual void postWrite() override{
      _impl->postWrite();
    }
 
    virtual bool write(ChimeraTK::VersionNumber versionNumber={}){
      // don't call preWrite() here. It would trigger the preWrite on the impl, which is also
      // happening when _impl()->write() is called.
      convertAndCopyToImpl();
      return _impl->write(versionNumber);
    }

    virtual bool isArray() const override {
      return _impl->isArray();
    }

    /// @todo FIXME: can we unite stuff in a transfer group? Compare with impl?
    virtual bool isSameRegister(const boost::shared_ptr<const mtca4u::TransferElement>& e) const override {
      return _impl->isSameRegister(e);
    }

    virtual std::vector<boost::shared_ptr<mtca4u::TransferElement> > getHardwareAccessingElements() override {
      return _impl->getHardwareAccessingElements();
    }
    
    virtual void replaceTransferElement(boost::shared_ptr<mtca4u::TransferElement> e) override {
      _impl->replaceTransferElement(e);
    }
    
    virtual void setPersistentDataStorage(boost::shared_ptr<PersistentDataStorage> storage) override{
      _impl->setPersistentDataStorage(storage);
    }

    virtual unsigned int getNInputQueueElements() const override {
      return _impl->getNInputQueueElements();
    }

    virtual mtca4u::FixedPointConverter getFixedPointConverter() const override {
      return _impl->getFixedPointConverter();
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

  /// A sub-namespace in order not to expose the classes to the ChimeraTK namespace
  namespace csa_helpers{
    
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
  }// namespace csa_helpers

  /** The actual partial implementation for strings as impl type **/
  template<class T>
    class TypeChangingStringImplDecorator<T, std::string >: public TypeChangingDecorator<T, std::string>{
  public:
    using TypeChangingDecorator<T, std::string>::TypeChangingDecorator;
    virtual void convertAndCopyFromImpl(){
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->buffer_2D[i][j] = csa_helpers::stringToT<T>(this->_impl->accessChannel(i)[j]);
        }
      }
    }
    virtual void convertAndCopyToImpl(){
      for (size_t i = 0; i < this->buffer_2D.size(); ++i){
        for (size_t j = 0; j < this->buffer_2D[i].size(); ++j){
          this->_impl->accessChannel(i)[j] = csa_helpers::T_ToString(this->buffer_2D[i][j]);
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
    virtual DecoratorType getDecoratorType() const override{
      return DecoratorType::range_checking;
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
    virtual DecoratorType getDecoratorType() const override{
      return DecoratorType::range_checking;
    }
  };

    
/*********************************************************************************************************************/
/*** Implementations of member functions below this line *************************************************************/
/*********************************************************************************************************************/

  template<class T, class IMPL_T>
  TypeChangingDecorator<T, IMPL_T>::TypeChangingDecorator(boost::shared_ptr< mtca4u::NDRegisterAccessor< IMPL_T> > const & impl) noexcept
    :  DecoratorTypeAccessor<T>(impl->getName(), impl->getUnit(), impl->getDescription()),  _impl(impl)
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
      boost::numeric::def_overflow_handler, csa_helpers::Round<double> > FromImplConverter;
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
      boost::numeric::def_overflow_handler, csa_helpers::Round<double> > ToImplConverter;
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
    virtual DecoratorType getDecoratorType() const override{
      return DecoratorType::C_style_conversion;
    }

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
    virtual DecoratorType getDecoratorType() const override{
      return DecoratorType::C_style_conversion;
    }
  };

  /** Quasi singleton to have a unique, global map across UserType templated factories.
   *  We need it to loop up if a decorator has already been created for the transfer element,
   *  and return this if so. Multiple decorators for the same transfer element don't work.
   */
  std::map< boost::shared_ptr<mtca4u::TransferElement>,
            boost::shared_ptr<mtca4u::TransferElement> > & getGlobalDecoratorMap();
  
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
            createdDecorator.reset( new TypeChangingRangeCheckingDecorator< UserType, TargetImplType> (
              boost::dynamic_pointer_cast< mtca4u::NDRegisterAccessor<TargetImplType> >( theImpl ) ) );
          }else if( wantedDecoratorType == DecoratorType::C_style_conversion){
            createdDecorator.reset( new TypeChangingDirectCastDecorator< UserType, TargetImplType> (
              boost::dynamic_pointer_cast< mtca4u::NDRegisterAccessor<TargetImplType> >( theImpl ) )  );
          }else{
            throw mtca4u::NotImplementedException("TypeChangingDecorator with range limitation is not implemented yet.");
          }
   }

  };


  // the factory function. You don't have to care about the IMPL_Type when requesting a decorator. Implementation, declared at the top of this file
  template<class UserType>
  boost::shared_ptr< mtca4u::NDRegisterAccessor< UserType > >  getDecorator( mtca4u::TransferElement & transferElement, DecoratorType decoratorType){

    // check if there already is a decorator for the transfer element
    auto decoratorMapEntry = getGlobalDecoratorMap().find( transferElement.getHighLevelImplElement() );
    if (decoratorMapEntry != getGlobalDecoratorMap().end()){
      // There already is a decorator for this transfer element
      // The decorator has to have a matching type, otherwise we can only throw
      auto castedType = boost::dynamic_pointer_cast< DecoratorTypeAccessor< UserType > >(decoratorMapEntry->second);
      if (castedType){// User type matches,  but the decorator type also has to match
        if (castedType->getDecoratorType() == decoratorType){
          // decorator matches, return it
          return castedType;
        }else{
        // sorry there is a decorator, but it's the wrong user type
        throw std::logic_error("ChimeraTK::ControlSystemAdapter: Decorator for TransferElement " +
                               transferElement.getName() +" already exists as a different decorator type.");          
        }
      }else{
        // sorry there is a decorator, but it's the wrong user type
        throw std::logic_error("ChimeraTK::ControlSystemAdapter: Decorator for TransferElement " +
                               transferElement.getName() +" already exists with a different user type.");
      }
    }
    
    DecoratorFactory< UserType > factory( transferElement.getHighLevelImplElement(), decoratorType );
    boost::fusion::for_each(mtca4u::userTypeMap(), factory);
    getGlobalDecoratorMap()[transferElement.getHighLevelImplElement()]=factory.createdDecorator;
    return factory.createdDecorator;
  }  
  
} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_TYPE_CHANGING_DECORATOR_H
