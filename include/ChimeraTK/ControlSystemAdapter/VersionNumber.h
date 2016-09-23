#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_VERSION_NUMBER_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_VERSION_NUMBER_H

#include <cstdint>

namespace ChimeraTK {

  /**
   * Type alias for the version number type. The version number is used for
   * resolving conflicting updates.
   */
  typedef std::uint64_t VersionNumber;

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_VERSION_NUMBER_H
