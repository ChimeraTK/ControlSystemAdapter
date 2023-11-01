// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <string>
namespace ChimeraTK {

  /** A simple helper function to convert integers to any type.
   *  It is there to provide the special implementation for strings,
   *  which don't have an implcit converstion.
   */
  template<class T>
  T toType(double input) {
    return input;
  }

  /// Template specialisation for strings
  template<>
  inline std::string toType<std::string>(double input) {
    return std::to_string(input);
  }

} // namespace ChimeraTK
