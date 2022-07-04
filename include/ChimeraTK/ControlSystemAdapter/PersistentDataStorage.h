#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PERSISTENT_DATA_STORAGE_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PERSISTENT_DATA_STORAGE_H

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <typeinfo>
#include <vector>

#include <boost/fusion/include/for_each.hpp>
#include <boost/thread.hpp>

#include <ChimeraTK/RegisterPath.h>
#include <ChimeraTK/SupportedUserTypes.h>
#include <ChimeraTK/cppext/future_queue.hpp>

namespace xmlpp {
  class Element;
}

namespace ChimeraTK {

  class ControlSystemPVManager;

  /**
   *  Persistent data storage for process variables.
   *
   *  The PersistentDataStorage will create a file in the current working
   * directory based on the provided application name
   * ("<applicationName>.persist"). This file will be an XML file containing all
   * values of the registered variables. The file is written when the
   * PersistentDataStorage is destroyed and read when it is constructed. After
   * reading the file, all variables will be updated with the current values taken
   * from the file. This will be seen as a value received by the application just
   * like any other update.
   *
   *  @todo TODO list:
   *    * thread safety (only an issue when having multiple
   * ControlSystemPVManagers)
   *    * automatic periodic commits
   *    * manual commits triggered by the application (with support for other
   * implementations, e.g. if the persistency is already provided by the control
   * system middleware)
   */
  class PersistentDataStorage {
   public:

    /** unit in seconds */
    static const unsigned int DEFAULT_WRITE_INTERVAL{30}; // 30 seconds

    /** Constructor: Open and parse the storage file. */
    PersistentDataStorage(std::string const& applicationName, unsigned int fileWriteInterval = DEFAULT_WRITE_INTERVAL);

    /** Destructor: Store variables to the file. */
    ~PersistentDataStorage();

    /** Register a variable to be stored to and retrieved from the data storage.
     * The returned value is the ID which must be passed to the other functions.
     * The last argument "fromFile" should be false when the function is called
     * from within UnidirectionalProcessArray<T>::setPersistentDataStorage() etc.
     * and true when called from within readFromFile(). */
    template<typename DataType>
    size_t registerVariable(ChimeraTK::RegisterPath const& name, size_t nElements, bool fromFile = false);

    /** Retrieve the current value for the variable with the given ID */
    template<typename DataType>
    const std::vector<DataType> retrieveValue(size_t id);

    /** Notify the storage system about a new value of the variable with the given
     * ID (as returned by registerVariable) */
    template<typename DataType>
    void updateValue(int id, std::vector<DataType> const& value);

   protected:
    /** Write out the file containing the persistent data */
    void writeToFile() noexcept;

    /** Read the file containing the persistent data */
    void readFromFile();

    /** Generate XML tags for the given value */
    template<typename DataType>
    void generateXmlValueTags(xmlpp::Element* parent, size_t id);

    /** Read value from XML tags */
    template<typename DataType>
    void readXmlValueTags(const xmlpp::Element* parent, size_t id);

    /** Application name */
    std::string _applicationName;

    /** File name to store the data to */
    std::string _filename;

    /** Vector of variable names. The index is the ID of the variable. */
    std::vector<ChimeraTK::RegisterPath> _variableNames;

    /** Vector of flags whether the variable was registers from the application.
     * The index is the ID of the variable. This flag is used to clean up
     * variables only coming from the file and are no longer present in the
     *  application. */
    std::vector<bool> _variableRegisteredFromApp;

    /** Vector of data types. The index is the ID of the variable. */
    std::vector<std::type_info const*> _variableTypes;

    /** Thread safe queue for write to file thread. */
    template <typename DataType> class Queue {
      cppext::future_queue<std::vector<DataType>> _q;
      std::vector<DataType> _latestValue{};

    public:
      Queue(size_t queueSize = 2) : _q(queueSize){}
      void push_overwrite(const std::vector<DataType> &e) {
        _q.push_overwrite(e);
      }
      std::vector<DataType> &read_latest() {
        while (_q.pop(_latestValue)) {
        }
        return _latestValue;
      }
    };

    /** Type definition for the map holding the values for one specific data type.
     */
    template<typename DataType>
    using DataMap = std::map<size_t, Queue<DataType>>;

    /** boost::fusion::map of the data type to the DataMap holding the values for
     * the type */
    ChimeraTK::TemplateUserTypeMap<DataMap> _dataMap;

    /** */
    boost::thread writerThread;

    void writerThreadFunction();

    std::mutex _queueReadMutex;

    // write interval in seconds (does not have to be atomic. Only used in the writer thread and is const.)
    unsigned int const _fileWriteInterval{};

    /** A functor needed in registerVariable() */
    struct registerVariable_oldTypeRemover {
      template<typename PAIR>
      void operator()(PAIR& pair) const;
      const std::type_info* type;
      size_t id;
    };
  };

  /*********************************************************************************************************************/

  template<typename PAIR>
  void PersistentDataStorage::registerVariable_oldTypeRemover::operator()(PAIR& pair) const {
    // search for the right type
    if(*type != typeid(typename PAIR::first_type)) return;

    // remove entry from data map
    pair.second.erase(id);

  }

  /*********************************************************************************************************************/

  template<typename DataType>
  size_t PersistentDataStorage::registerVariable(ChimeraTK::RegisterPath const& name, size_t nElements, bool fromFile) {
    // check if already existing
    auto position = std::find(_variableNames.begin(), _variableNames.end(), name);

    size_t id = position - _variableNames.begin();
    std::lock_guard<std::mutex> lock(_queueReadMutex);

    // create new element
    if(position == _variableNames.end()) {
      // output information
      if(!fromFile) std::cout << "PersistentDataStorage: registering new variable " << name << std::endl;

      // store name and type
      _variableNames.push_back(name);
      _variableTypes.push_back(&typeid(DataType));

      // set flag whether this variable has been registered from the application
      _variableRegisteredFromApp.push_back(!fromFile);

      // create value vector
      id = _variableNames.size() - 1;
      std::vector<DataType>& value = boost::fusion::at_key<DataType>(_dataMap.table)[id].read_latest();
      value.resize(nElements);

      // return id
      return id;
    }
    // replace element (changed data type)
    else if(boost::fusion::at_key<DataType>(_dataMap.table).count(id) == 0) {
      std::cout << "PersistentDataStorage: changing type of variable " << name << std::endl;
      assert(_variableTypes[id] != &typeid(DataType));
      assert(!fromFile);

      // remove value vector from old type
      boost::fusion::for_each(_dataMap.table, registerVariable_oldTypeRemover({_variableTypes[id], id}));

      // update type
      _variableTypes[id] = &typeid(DataType);

      // update flag that this variable has been registered from the application
      _variableRegisteredFromApp[id] = true;

      // create value vector
      std::vector<DataType>& value = boost::fusion::at_key<DataType>(_dataMap.table)[id].read_latest();
      value.resize(nElements);

      // return id
      return id;
    }
    // return existing id
    else {
      assert(!fromFile);

      // update flag that this variable has been registered from the application
      _variableRegisteredFromApp[id] = true;

      // check if resize required
      std::vector<DataType>& value = boost::fusion::at_key<DataType>(_dataMap.table)[id].read_latest();
      if(value.size() != nElements) {
        std::cout << "PersistentDataStorage: changing size of variable " << name << " from " << value.size() << " to "
                  << nElements << std::endl;
        value.resize(nElements);
      }

      return id;
    }
  }

  /*********************************************************************************************************************/

  template<typename DataType>
  const std::vector<DataType> PersistentDataStorage::retrieveValue(size_t id) {
    std::lock_guard<std::mutex> lock(_queueReadMutex);
    return boost::fusion::at_key<DataType>(_dataMap.table)[id].read_latest();
  }

  /*********************************************************************************************************************/

  template<typename DataType>
  void PersistentDataStorage::updateValue(int id, std::vector<DataType> const &value) {
    boost::fusion::at_key<DataType>(_dataMap.table)[id].push_overwrite(value);
  }

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_PERSISTENT_DATA_STORAGE_H


