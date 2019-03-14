#ifndef _CHIMERATK_CONTROL_SYSTEM_ADATPER_TEST_TO_TYPE_H_
#define _CHIMERATK_CONTROL_SYSTEM_ADATPER_TEST_TO_TYPE_H_

/** A simple helper function to convert integers to any type.
 *  It is there to provice the special implementation for strings,
 *  which don't have an implcit converstion.
 */
template<class T>
T toType(int input) {
  return input;
}

/// Template specialisation for strings
template<>
inline std::string toType<std::string>(int input) {
  return std::to_string(input);
}

#endif // _CHIMERATK_CONTROL_SYSTEM_ADATPER_TEST_TO_TYPE_H_
