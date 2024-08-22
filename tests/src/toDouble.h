// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <string>

namespace ChimeraTK {

  // helper function to covert string to double (abstraced by template)
  template<class UserType>
  double toDouble(UserType input) {
    return input;
  }

  template<>
  inline double toDouble<std::string>(std::string input) {
    return std::stod(input);
  }

} // namespace ChimeraTK
