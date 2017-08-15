/*
 * PersistentDataStorage.cc
 *
 *  Created on: Nov 23, 2016
 *      Author: Martin Hierholzer
 */

#include <sys/stat.h>

#include <libxml++/libxml++.h>

#include "PersistentDataStorage.h"
#include "ApplicationBase.h"

namespace ChimeraTK {

  /*********************************************************************************************************************/

  PersistentDataStorage::PersistentDataStorage(std::string const &applicationName) {
    _filename = applicationName+".persist";
    _applicationName = applicationName;
    readFromFile();
  }

  /*********************************************************************************************************************/
      
  PersistentDataStorage::~PersistentDataStorage() {
    writeToFile();
  }

  /*********************************************************************************************************************/
      
  void PersistentDataStorage::writeToFile() {

    // create XML document with root node and a flat list of variables below this root
    xmlpp::Document doc;
    xmlpp::Element *rootElement = doc.create_root_node("PersistentData", "https://github.com/ChimeraTK/ControlSystemAdapter");
    rootElement->set_attribute("application", _applicationName);

    for(size_t i=0; i<_variableNames.size(); ++i) {

      // create XML element for the variable and set name attribute
      xmlpp::Element *variable = rootElement->add_child("variable");
      variable->set_attribute("name",static_cast<std::string>(_variableNames[i]));

      // generate value XML tags and set type name as a string
      std::string dataTypeName{"unknown"};
      if(*_variableTypes[i] == typeid(int8_t)) {
        generateXmlValueTags<int8_t>(variable, i);
        dataTypeName = "int8";
      }
      else if(*_variableTypes[i] == typeid(uint8_t)) {
        generateXmlValueTags<uint8_t>(variable, i);
        dataTypeName = "uint8";
      }
      else if(*_variableTypes[i] == typeid(int16_t)) {
        generateXmlValueTags<int16_t>(variable, i);
        dataTypeName = "int16";
      }
      else if(*_variableTypes[i] == typeid(uint16_t)) {
        generateXmlValueTags<uint16_t>(variable, i);
        dataTypeName = "uint16";
      }
      else if(*_variableTypes[i] == typeid(int32_t)) {
        generateXmlValueTags<int32_t>(variable, i);
        dataTypeName = "int32";
      }
      else if(*_variableTypes[i] == typeid(uint32_t)) {
        generateXmlValueTags<uint32_t>(variable, i);
        dataTypeName = "uint32";
      }
      else if(*_variableTypes[i] == typeid(float)) {
        generateXmlValueTags<float>(variable, i);
        dataTypeName = "float";
      }
      else if(*_variableTypes[i] == typeid(double)) {
        generateXmlValueTags<double>(variable, i);
        dataTypeName = "double";
      }
      else if(*_variableTypes[i] == typeid(std::string)) {
        generateXmlValueTags<std::string>(variable, i);
        dataTypeName = "string";
      }
      else {
        /// @todo TODO what todo here?
      }
      
      // set type attribute
      variable->set_attribute("type",dataTypeName);

    }

    // write out to file
    doc.write_to_file_formatted(_filename);

  }

  /*********************************************************************************************************************/

  template<typename DataType>
  void PersistentDataStorage::generateXmlValueTags(xmlpp::Element *parent, size_t id) {
    
    // obtain the data vector from the map
    std::vector<DataType> &value = boost::fusion::at_key<DataType>(_dataMap.table)[id];

    // add one child element per element of the value
    for(size_t idx=0; idx<value.size(); ++idx) {
      xmlpp::Element *valueElement = parent->add_child("val");
      valueElement->set_attribute("i", boost::lexical_cast<std::string>(idx));
      valueElement->set_attribute("v", boost::lexical_cast<std::string>(value[idx]));
    }
  }

  /*********************************************************************************************************************/

  void PersistentDataStorage::readFromFile() {
  
    // check if file exists
    struct stat buffer;   
    if(stat(_filename.c_str(), &buffer) != 0) {
      // file does not exist: print message and do nothing
      std::cerr << "ChimeraTK::PersistentDataStorage: Persistency file '" << _filename << "' does not exist. "
                   "It will be created when exiting the application." << std::endl;
      return;
    }
    
    try {
      xmlpp::DomParser parser;
      //parser.set_validate();
      parser.set_substitute_entities(); //We just want the text to be resolved/unescaped automatically.
      parser.parse_file(_filename);
      if(parser) {
        // obtain root node
        const xmlpp::Node *rootElement = parser.get_document()->get_root_node(); // object will be deleted by DomParser
        /// @todo TODO check if the application name is correct?
        
        // iterate through variables
        for(auto &elem : rootElement->get_children()) {
          const xmlpp::Element *child = dynamic_cast<const xmlpp::Element*>(elem);
          if(!child) continue;    // comment or white spaces...
          std::string name = child->get_attribute("name")->get_value();
          std::string type = child->get_attribute("type")->get_value();
          if(type == "int8") { readXmlValueTags<int8_t>(child, registerVariable<int8_t>(name, 0)); }
          else if(type == "uint8") { readXmlValueTags<uint8_t>(child, registerVariable<uint8_t>(name, 0)); }
          else if(type == "int16") { readXmlValueTags<int16_t>(child, registerVariable<int16_t>(name, 0)); }
          else if(type == "uint16") { readXmlValueTags<uint16_t>(child, registerVariable<uint16_t>(name, 0)); }
          else if(type == "int32") { readXmlValueTags<int32_t>(child, registerVariable<int32_t>(name, 0)); }
          else if(type == "uint32") { readXmlValueTags<uint32_t>(child, registerVariable<uint32_t>(name, 0)); }
          else if(type == "float") { readXmlValueTags<float>(child, registerVariable<float>(name, 0)); }
          else if(type == "double") { readXmlValueTags<double>(child, registerVariable<double>(name, 0)); }
          else if(type == "string") { readXmlValueTags<std::string>(child, registerVariable<std::string>(name, 0)); }
          else { /* @todo TODO ??? */ }
        }
      }
      else {
        std::cout << "No parser... " << std::endl;    // @todo TODO proper exception handling
      }
    }
    catch(const std::exception& ex) {   // @todo TODO proper exception handling
      std::cout << "Exception caught: " << ex.what() << std::endl;
    }    
      
  }

  /*********************************************************************************************************************/

  template<typename DataType>
  void PersistentDataStorage::readXmlValueTags(const xmlpp::Element *parent, size_t id) {
    
    // obtain the data vector from the map
    std::vector<DataType> &value = boost::fusion::at_key<DataType>(_dataMap.table)[id];

    // collect values
    for(auto &valElems : parent->get_children()) {
      const xmlpp::Element *valChild = dynamic_cast<const xmlpp::Element*>(valElems);
      if(!valChild) continue;    // comment or white spaces...
      
      // obtain index and value as string
      std::string s_idx = valChild->get_attribute("i")->get_value();
      std::string s_val = valChild->get_attribute("v")->get_value();
      
      // convert to data type
      size_t idx = boost::lexical_cast<size_t>(s_idx);
      DataType val = boost::lexical_cast<DataType>(s_val);
      
      // resize vector if needed
      if(value.size() <= idx) value.resize(idx+1);
      
      // store value
      value[idx] = val;
    }
  }

  /*********************************************************************************************************************/
  
} /* namespace ChimeraTK */
