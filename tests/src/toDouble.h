#ifndef _CHIMERATK_CONTROL_SYSTEM_ADAPTER_TEST_TO_DOUBLE_H_
#define _CHIMERATK_CONTROL_SYSTEM_ADAPTER_TEST_TO_DOUBLE_H_

#include <string>

// helper function to covert string to double (abstraced by template)
template<class UserType>
double toDouble(UserType input){
  return input;
}

template<>
double toDouble<std::string>(std::string input){
  return std::stod(input);
}
  
#endif // _CHIMERATK_CONTROL_SYSTEM_ADAPTER_TEST_TO_DOUBLE_H_
