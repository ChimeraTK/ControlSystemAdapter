// SPDX-FileCopyrightText: Deutsches Elektronen-Synchrotron DESY, MSK, ChimeraTK Project <chimeratk-support@desy.de>
// SPDX-License-Identifier: LGPL-3.0-or-later
#pragma once

#include <ChimeraTK/SupportedUserTypes.h>

#include <cassert>
#include <string>
namespace ChimeraTK {

  /** A simple helper function to convert integers to any type.
   *  It is there to provide the special implementation for strings,
   *  which don't have an implcit converstion.
   */
  template<class T, class U>
  T toType(U input) {
    if constexpr(std::is_same_v<T, std::string>) {
      return std::to_string(input);
    }
    else if constexpr(std::is_same_v<T, ChimeraTK::Void>) {
      return {};
    }
    else if constexpr(std::is_integral_v<T> && !std::is_integral_v<U>) {
      // Converting floating point values into integers is undefined behaviour, if the range of the target value range
      // is exceeded. Hence we first convert into int64_t before we convert into the target tyoe, so we get the "roll
      // over" behaviour of the narrowing integer conversion as wanted.
      assert(input < U(std::numeric_limits<int64_t>::max()));
      assert(input > U(std::numeric_limits<int64_t>::lowest()));
      return T(int64_t(input));
    }
    else {
      return T(input);
    }
  }

} // namespace ChimeraTK
