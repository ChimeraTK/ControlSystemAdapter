#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PERSISTENT_DATA_STORAGE_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PERSISTENT_DATA_STORAGE_H

#include <iostream>
#include <algorithm>
#include <string>
#include <typeinfo>
#include <vector>
#include <map>
#include <mtca4u/SupportedUserTypes.h>

namespace xmlpp {
  class Element;
}

namespace ChimeraTK {
  
  class ControlSystemPVManager;
  
  /**
   *  Persistent data storage for process variables. Note: This class can only be used in applications based on the
   *  class ApplicationBase.
   */
  class PersistentDataStorage {

    public:
      
      /** Constructor: Open and parse the storage file. */
      PersistentDataStorage(std::string const &applicationName);
      
      /** Destructor: Store variables to the file. */
      ~PersistentDataStorage();

      /** Register a variable to be stored to and retrieved from the data storage. The returned value is the ID which
      *   must be passed to the other functions. */
      template<typename DataType>
      size_t registerVariable(std::string const &name, size_t nElements);

      /** Retrieve the current value for the variable with the given ID */
      template<typename DataType>
      const std::vector<DataType>& retrieveValue(size_t id) const;

      /** Notify the storage system about a new value of the variable with the given ID (as returned by
       *  registerVariable) */
      template<typename DataType>
      void updateValue(int id, std::vector<DataType> const &value);
      
  protected:
    
      /** Write out the file containing the persistent data */
      void writeToFile();
    
      /** Read the file containing the persistent data */
      void readFromFile();
    
      /** Generate XML tags for the given value */
      template<typename DataType>
      void generateXmlValueTags(xmlpp::Element *parent, size_t id);
    
      /** Read value from XML tags */
      template<typename DataType>
      void readXmlValueTags(const xmlpp::Element *parent, size_t id);
      
      /** Application name */
      std::string _applicationName;
    
      /** File name to store the data to */
      std::string _filename;

      /** Vector of variable names. The index is the ID of the variable. */
      std::vector<std::string> _variableNames;

      /** Vector of data types. The index is the ID of the variable. */
      std::vector<std::type_info const *> _variableTypes;

      /** Type definition for the map holding the values for one specific data type. */
      template<typename DataType>
      using DataMap = std::map<size_t, std::vector<DataType> >;

      /** boost::fusion::map of the data type to the DataMap holding the values for the type */
      mtca4u::TemplateUserTypeMap<DataMap> _dataMap;
      
  };

  /*********************************************************************************************************************/

  template<typename DataType>
  size_t PersistentDataStorage::registerVariable(std::string const &name, size_t nElements) {
    // check if already existing
    auto position = std::find(_variableNames.begin(), _variableNames.end(), name);
    
    // create new element
    if(position == _variableNames.end()) {
      
      // store name and type
      _variableNames.push_back(name);
      _variableTypes.push_back(&typeid(DataType));

      // create value vector
      size_t id = _variableNames.size()-1;
      std::vector<DataType> &value = boost::fusion::at_key<DataType>(_dataMap.table)[id];
      value.resize(nElements);
      
      // return id
      return id;
    }
    // return existing id
    else {
      return position - _variableNames.begin();
    }
  }

  /*********************************************************************************************************************/

  template<typename DataType>
  const std::vector<DataType>& PersistentDataStorage::retrieveValue(size_t id) const {
    return boost::fusion::at_key<DataType>(_dataMap.table).at(id);
  }

  /*********************************************************************************************************************/

  template<typename DataType>
  void PersistentDataStorage::updateValue(int id, std::vector<DataType> const &value) {
    boost::fusion::at_key<DataType>(_dataMap.table)[id] = value;
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PERSISTENT_DATA_STORAGE_H


