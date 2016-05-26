#ifndef MTCA4U_SYNCHRONIZATION_DIRECTION_H
#define MTCA4U_SYNCHRONIZATION_DIRECTION_H

namespace mtca4u {

  /**
   * Direction when synchronizing process variables. A process variable may be
   * synchronized from the device library to the control system (input) or from
   * the control system to the device library (output).
   */
  enum SynchronizationDirection {

    /**
     * Synchronize from the control system to the device library.
     */
    controlSystemToDevice,

    /**
     * Synchronize from the device library to the control system.
     */
    deviceToControlSystem

  };

}

#endif // MTCA4U_SYNCHRONIZATION_DIRECTION_H
