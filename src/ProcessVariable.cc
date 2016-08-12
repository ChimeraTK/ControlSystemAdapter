#include <ChimeraTK/ControlSystemAdapter/ProcessVariable.h>

namespace ChimeraTK {

  ProcessVariable::ProcessVariable(const std::string& name) :
      _name(name) {
  }

  ProcessVariable::~ProcessVariable() {
  }

  const std::string& ProcessVariable::getName() const {
    return _name;
  }

}
