/*
 * PersistentDataStorage.cc
 *
 *  Created on: Nov 23, 2016
 *      Author: Martin Hierholzer
 */

#include "PersistentDataStorage.h"

#include "ApplicationBase.h"
#include <libxml++/libxml++.h>
#include <sys/stat.h>

#include <boost/lexical_cast.hpp>

namespace ChimeraTK {

  /*********************************************************************************************************************/

  PersistentDataStorage::PersistentDataStorage(std::string const& applicationName, unsigned int fileWriteInterval)
  : _fileWriteInterval(fileWriteInterval) {
    _filename = applicationName + ".persist";
    _applicationName = applicationName;
    readFromFile();

    _writerThread = boost::thread([this] { this->writerThreadFunction(); });
  }

  /*********************************************************************************************************************/

  PersistentDataStorage::~PersistentDataStorage() {
    try {
      _writerThread.interrupt();
      _writerThread.join();
    }
    catch(...) {
      std::cerr << "Cannot join writer thread!" << std::endl;
    }
    writeToFile();
  }

  /*********************************************************************************************************************/

  void PersistentDataStorage::writerThreadFunction() {
    while(true) {
      for(unsigned int i = 0; i < _fileWriteInterval; ++i) {
        sleep(1);
        boost::this_thread::interruption_point();
      }
      writeToFile();
    }
  }

  /*********************************************************************************************************************/

  void PersistentDataStorage::writeToFile() noexcept {
    try {
      // create XML document with root node and a flat list of variables below this root
      xmlpp::Document doc;
      xmlpp::Element* rootElement =
          doc.create_root_node("PersistentData", "https://github.com/ChimeraTK/ControlSystemAdapter");
      rootElement->set_attribute("application", _applicationName);

      for(size_t i = 0; i < _variableNames.size(); ++i) {
        if(!_variableRegisteredFromApp[i]) {
          continue; // exclude variables no longer present in the application
        }

        // create XML element for the variable and set name attribute
        xmlpp::Element* variable = rootElement->add_child("variable");
        variable->set_attribute("name", static_cast<std::string>(_variableNames[i]));

        // generate value XML tags and set type name as a string
        DataType dataType(*_variableTypes[i]);
        callForType(dataType, [&](auto t) {
          using UserType = decltype(t);
          generateXmlValueTags<UserType>(variable, i);
        });

        // set type attribute
        variable->set_attribute("type", dataType.getAsString());
      }

      // write out to file
      auto tempfile = _filename + ".new";
      doc.write_to_file_formatted(tempfile);
      std::rename(tempfile.c_str(), _filename.c_str());
    }
    catch(const std::exception& e) {
      std::cerr << "Error writing persistency file: " << e.what() << std::endl;
    }
    catch(...) {
      std::cerr << "Error writing persistency file (unknown exception)" << std::endl;
    }
  }

  /*********************************************************************************************************************/

  template<typename UserType>
  void PersistentDataStorage::generateXmlValueTags(xmlpp::Element* parent, size_t id) {
    std::vector<UserType>* pValue;
    {
      // obtain the data vector from the map
      std::lock_guard<std::mutex> lock(_queueReadMutex);
      auto& value = boost::fusion::at_key<UserType>(_dataMap.table)[id].readLatest();
      pValue = &value;
    }
    // add one child element per element of the value
    for(size_t idx = 0; idx < pValue->size(); ++idx) {
      xmlpp::Element* valueElement = parent->add_child("val");
      valueElement->set_attribute("i", userTypeToUserType<std::string>(idx));
      valueElement->set_attribute("v", userTypeToUserType<std::string>((*pValue)[idx]));
    }
  }

  /*********************************************************************************************************************/

  void PersistentDataStorage::readFromFile() {
    // check if file exists
    struct stat buffer {};
    if(stat(_filename.c_str(), &buffer) != 0) {
      // file does not exist: print message and do nothing
      std::cerr << "ChimeraTK::PersistentDataStorage: Persistency file '" << _filename
                << "' does not exist. It will be created when exiting the application." << std::endl;
      return;
    }

    try {
      xmlpp::DomParser parser;
      // parser.set_validate();
      parser.set_substitute_entities(); // We just want the text to be resolved/unescaped automatically.
      parser.parse_file(_filename);
      if(parser) {
        // obtain root node
        const xmlpp::Node* rootElement = parser.get_document()->get_root_node(); // object will be deleted by DomParser
        /// @todo TODO check if the application name is correct?

        // iterate through variables
        for(const auto& elem : rootElement->get_children()) {
          const auto* child = dynamic_cast<const xmlpp::Element*>(elem);
          if(!child) {
            continue; // comment or white spaces...
          }
          std::string name = child->get_attribute("name")->get_value();
          std::string type = child->get_attribute("type")->get_value();
          // compatibility with old persistency files - remove after July 2023
          if(type == "double") {
            type = "float64";
          }
          if(type == "float") {
            type = "float32";
          }
          DataType dataType(type);
          if(dataType == DataType::none) {
            std::cerr << "Unknown data type '" + type + "' found in persist file: " << name << std::endl;
            continue;
          }
          callForType(dataType, [&](auto t) {
            using UserType = decltype(t);
            readXmlValueTags<UserType>(child, registerVariable<UserType>(name, 0, true));
          });
        }
      }
      else {
        throw ChimeraTK::logic_error("Could not parse persist file " + _filename + ": Failed to create the parser.");
      }
    }
    catch(const std::exception& ex) {
      throw ChimeraTK::logic_error("Could not parse persist file " + _filename + ": " + ex.what());
    }
  }

  /*********************************************************************************************************************/

  template<typename UserType>
  void PersistentDataStorage::readXmlValueTags(const xmlpp::Element* parent, size_t id) {
    // obtain the data vector from the map
    std::vector<UserType>& value = boost::fusion::at_key<UserType>(_dataMap.table)[id].readLatest();

    // collect values
    for(const auto& valElems : parent->get_children()) {
      const auto* valChild = dynamic_cast<const xmlpp::Element*>(valElems);
      if(!valChild) {
        continue; // comment or white spaces...
      }

      // obtain index and value as string
      std::string s_idx = valChild->get_attribute("i")->get_value();
      std::string s_val = valChild->get_attribute("v")->get_value();

      // convert to data type
      auto idx = userTypeToUserType<size_t>(s_idx);
      auto val = userTypeToUserType<UserType>(s_val);

      // resize vector if needed
      if(value.size() <= idx) {
        value.resize(idx + 1);
      }

      // store value
      value[idx] = val;
    }
  }

  /*********************************************************************************************************************/

} /* namespace ChimeraTK */
