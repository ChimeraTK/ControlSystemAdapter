#ifndef MTCA4U_PROCESS_VARIABLE_FACTORY
#define MTCA4U_PROCESS_VARIABLE_FACTORY

#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

#include <string>
#include <typeinfo>
#include <map>

#include "ProcessVariable.h"
#include "ProcessArray.h"

namespace mtca4u{
  
  /** The ProcessVariableFactory provides an interface to create ProcessVariables and ProcessArrays. 
   *  They are identified by a unique name, by which they can be accessed via the control system.
   *  The getProcessVariable() and getProcessArray() methods return a shared pointer.
   *  One shared pointer of the created ProcessVariable is held by the factory. Like this the instance
   *  of the ProcessVariable can allways be accessed by its name. This instance is only relased when the 
   *  factory goes out of scope.
   *  
   *  \par Technical details:
   *  The map holding the ProcessVariables and ProcessArrays is already implemented in this base class
   *  so this functionality does not have to be reimplemented in each implementation. It stores the ProcessX<T>
   *  objects as boost::any in a map with the name being used as a unique key.
   *  In order to abstract the creation each factory implementation has to implement the protected purely virtual
   *  createProcessVariable() function. In contrast to the getProcessVariable() function it is not templated
   *  because template functions cannot be virtual. Instead std::type_info is used to pass the type information.
   *  The concrete implementation has to determine the type at run time and create the corresponding
   *  ProcessVariable implementation. This overhead can be accepted because it only happens in the initialisation
   *  phase and is the only way (I know of) to implement the required abstration.
   */
  class ProcessVariableFactory{
  public:
    
    /** Get a (shared pointer to a) ProcessVariable of the simple type T.
     *  As there is no return type overloading in C++ this functions is templated. The 
     *  template argument defines which type of is returned. The concrete implementation 
     *  of the ProcessVariable depends on the implementation of the factory.
     *  
     *  \par Example
     *  \code
     *  boost::shared_ptr< ProcessVariable<int> > theVelocity = myFactory.getProcessVariable<int>("velocity");
     *  \endcode
     *
     *  \throws boost::bad_any_cast The bad_any_cast exception is thrown if a ProcessVariable with the 
     *  requested name has already been created as another type, or if a ProcessArray with this name exists.
     *
     *  \throws std::bad_typeid The bad_typeid exception will be thrown if a ProcessVariable with the requested
     *  type cannot be created (e.g. int64_t is not supported at the moment).
     */
    template<class T>
      boost::shared_ptr< ProcessVariable<T> > getProcessVariable(std::string name){
      // The square bracket operator cannot be used as ProcessVariables do not have a default constructor.
      // How could they, the concrete implementation is not known here. So we have to see
      // if an object with this name is already in the map.
      std::map< std::string, boost::any >::iterator mapEntry =  _processVariableMap.find(name);

      // If the object is not in the map (iterator is at end()) the object has to be created using the 
      // virtual createProcessVariable() function.
      if(mapEntry == _processVariableMap.end()){
	boost::any processVariableAsAny =  createProcessVariable( typeid(T), name );

	// Insert returns a pair of <iterator, bool>. We want to assign the iterator.
	// No need to check the bool, it is always true as this code is only executed 
	// if the entry did not exist.
	mapEntry = _processVariableMap.insert( std::make_pair(name, processVariableAsAny) ).first;
      }
      
      // this might throw a boost::bad_any_cast exception in case the object with this name was already
      // allocated as a different type
      return boost::any_cast< boost::shared_ptr< ProcessVariable<T> > >( mapEntry->second );
    }

    /** Get a (shared pointer to a) ProcessArray of type T of the size arraySize. The name must be 
     *  unique across arrays and process variables.
     *  
     *  \par Example
     *  \code
     *  boost::shared_ptr< ProcessArrar<int> > someIntegers = myFactory.getProcessArray<int>("twentyIntergers",20);
     *  \endcode
     *
     *  \throws boost::bad_any_cast The bad_any_cast exception is thrown if a ProcessArray
     *  with the requested name has already been created as another type, or if a ProcessVariable with the
     *  same name exists.
     *
     *  \throws std::length_error if a ProcessArray with the same name but a different size already exits.
     *
     *  \throws std::bad_typeid The bad_typeid exception will be thrown if a ProcessArray with the requested
     *  type cannot be created (e.g. int64_t is not supported at the moment).
     */
    template<class T>
      boost::shared_ptr< ProcessArray<T> > getProcessArray(std::string name, size_t arraySize){
      // See if there is an object with this name.
      std::map< std::string, boost::any >::iterator mapEntry =  _processVariableMap.find(name);

      // If the object is not in the map (iterator is at end()) the object has to be created using the 
      // virtual createProcessArray() function.
      if(mapEntry == _processVariableMap.end()){
	boost::any processArrayAsAny =  createProcessArray( typeid(T), name , arraySize );

	// Insert returns a pair of <iterator, bool>. We want to assign the iterator.
	// No need to check the bool, it is always true as this code is only executed 
	// if the entry did not exist.
	mapEntry = _processVariableMap.insert( std::make_pair(name, processArrayAsAny) ).first;

	return boost::any_cast< boost::shared_ptr< ProcessArray<T> > >( mapEntry->second );
      }else{
	// check that the existing thing is a ProcessArray of the right type and size

	// This throws a boost::bad_any_cast if it is not the right type
	boost::shared_ptr< ProcessArray<T> > processArray = 
	  boost::any_cast< boost::shared_ptr< ProcessArray<T> > >( mapEntry->second );

	// Check the size
	if (processArray->size() != arraySize){
	  throw std::length_error("ProcessArray already exists with a different size!");
	}

	// The entry is as requested. Return the already existing array.
	return processArray;
      }


    }


  private:
    /** The map which holds all ProcessVariables and ProcessArrays
     */
    std::map< std::string, boost::any > _processVariableMap;

  protected:
    /** Create a shared pointer of a ProcessVariable of a given type as boost any. This purely virtual
     *  function has to be implemented in the concrete factories.
     *  It should throw a std::bad_typeid exception if the requested type cannot be created.
     *  
     *  \attention When implementing make sure that 
     *  the boost::any object is of the type <tt>boost::shared_ptr< ProcessVariable<T> ></tt>!
     */
    virtual boost::any createProcessVariable( std::type_info const & variableType, std::string name ) = 0;

    /** Create a shared pointer of a ProcessVariable of a given type as boost any. This purely virtual
     *  function has to be implemented in the concrete factories.
     *  It should throw a std::bad_typeid exception if the requested type cannot be created.
     *  
     *  \attention When implementing make sure that 
     *  the boost::any object is of the type <tt>boost::shared_ptr< ProcessArray<T> ></tt>!
     */
    virtual boost::any createProcessArray( std::type_info const & variableType, std::string name, 
					   size_t arraySize ) = 0;

  };

}// namespace mtca4u

#endif// MTCA4U_PROCESS_VARIABLE_FACTORY
