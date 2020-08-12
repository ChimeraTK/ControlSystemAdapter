#ifndef CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_SYNCHRONIZATION_DIRECTION_H
#define CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_SYNCHRONIZATION_DIRECTION_H

namespace ChimeraTK {

  /**
   * Direction when synchronizing process variables. A process variable may be
   * synchronized from the device library to the control system (input) or from
   * the control system to the device library (output).
   */
  enum class SynchronizationDirection {

    /**
     * Synchronize from the control system to the device library.
     */
    controlSystemToDevice,

    /**
     * Synchronize from the device library to the control system.
     */
    deviceToControlSystem,

    /**
     * Synchronize in both directions.
     */
    bidirectional

  };

} // namespace ChimeraTK

#endif // CHIMERA_TK_CONTROL_SYSTEM_ADAPTER_SYNCHRONIZATION_DIRECTION_H
