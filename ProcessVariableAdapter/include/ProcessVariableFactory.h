#ifndef MTCA4U_PROCESS_VARIABLE_FACTORY
#define MTCA4U_PROCESS_VARIABLE_FACTORY

#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>

#include <string>
#include <typeinfo>
#include <map>

#include "ProcessVariable.h"

namespace mtca4u{
  
  class ProcessVariableFactory{
  public:
    template<class T>
      boost::shared_ptr< ProcessVariable<T> > getProcessVariable(std::string name){
      std::map< std::string, boost::any >::iterator mapEntry =  _processVariableMap.find(name);

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

  private:
    std::map< std::string, boost::any > _processVariableMap;

  protected:
    virtual boost::any createProcessVariable( std::type_info const & variableType, std::string name ) = 0;

  };

}// namespace mtca4u

#endif// MTCA4U_PROCESS_VARIABLE_FACTORY
