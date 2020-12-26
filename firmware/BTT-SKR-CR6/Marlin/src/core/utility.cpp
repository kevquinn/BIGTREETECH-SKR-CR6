/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "utility.h"

#include "../Marlin.h"
#include "../module/temperature.h"

void safe_delay(millis_t ms) {
  while (ms > 50) {
    ms -= 50;
    delay(50);
    thermalManager.manage_heater();
  }
  delay(ms);
  thermalManager.manage_heater(); // This keeps us safe if too many small safe_delay() calls are made
}

#if ENABLED(DEBUG_LEVELING_FEATURE)

  #include "../module/probe.h"
  #include "../module/motion.h"
  #include "../module/stepper.h"
  #include "../libs/numtostr.h"
  #include "../feature/bedlevel/bedlevel.h"

  void log_machine_info() {
    #if ENABLED(DELTA)
      #define UTILITY_MACHINE_TYPE "Delta"
    #elif IS_SCARA
      #define UTILITY_MACHINE_TYPE "SCARA"
    #elif IS_CORE
      #define UTILITY_MACHINE_TYPE "Core"
    #else
      #define UTILITY_MACHINE_TYPE "Cartesian"
    #endif
    SERIAL_ECHOLNPGM("Machine Type: " UTILITY_MACHINE_TYPE);

    #if ENABLED(PROBE_MANUALLY)
      #define UTILITY_PROBE_TYPE "PROBE_MANUALLY"
    #elif ENABLED(FIX_MOUNTED_PROBE)
      #define UTILITY_PROBE_TYPE "FIX_MOUNTED_PROBE"
    #elif ENABLED(BLTOUCH)
      #define UTILITY_PROBE_TYPE "BLTOUCH"
    #elif HAS_Z_SERVO_PROBE
      #define UTILITY_PROBE_TYPE "SERVO PROBE"
    #elif ENABLED(TOUCH_MI_PROBE)
      #define UTILITY_PROBE_TYPE "TOUCH_MI_PROBE"
    #elif ENABLED(Z_PROBE_SLED)
      #define UTILITY_PROBE_TYPE "Z_PROBE_SLED"
    #elif ENABLED(Z_PROBE_ALLEN_KEY)
      #define UTILITY_PROBE_TYPE "Z_PROBE_ALLEN_KEY"
    #elif ENABLED(SOLENOID_PROBE)
      #define UTILITY_PROBE_TYPE "SOLENOID_PROBE"
    #else
      #define UTILITY_PROBE_TYPE "NONE"
    #endif
    SERIAL_ECHOLNPGM("Probe: " UTILITY_PROBE_TYPE);

    #if HAS_BED_PROBE
      SERIAL_ECHOPAIR_P(PSTR("Probe Offset X"), probe_offset.x, SP_Y_STR, probe_offset.y, SP_Z_STR, probe_offset.z);
      if (probe_offset.x > 0)
        SERIAL_ECHOPGM(" (Right");
      else if (probe_offset.x < 0)
        SERIAL_ECHOPGM(" (Left");
      else if (probe_offset.y != 0)
        SERIAL_ECHOPGM(" (Middle");
      else
        SERIAL_ECHOPGM(" (Aligned With");

      if (probe_offset.y > 0) {
        #if IS_SCARA
          SERIAL_ECHOPGM("-Distal");
        #else
          SERIAL_ECHOPGM("-Back");
        #endif
      }
      else if (probe_offset.y < 0) {
        #if IS_SCARA
          SERIAL_ECHOPGM("-Proximal");
        #else
          SERIAL_ECHOPGM("-Front");
        #endif
      }
      else if (probe_offset.x != 0)
        SERIAL_ECHOPGM("-Center");

      if (probe_offset.z < 0)
        SERIAL_ECHOPGM(" & Below");
      else if (probe_offset.z > 0)
        SERIAL_ECHOPGM(" & Above");
      else
        SERIAL_ECHOPGM(" & Same Z as");
      SERIAL_ECHOLNPGM(" Nozzle)");
    #endif

    #if HAS_ABL_OR_UBL
      #if ENABLED(AUTO_BED_LEVELING_LINEAR)
        #define UTILITY_ABL_TYPE "LINEAR"
      #elif ENABLED(AUTO_BED_LEVELING_BILINEAR)
        #define UTILITY_ABL_TYPE "BILINEAR"
      #elif ENABLED(AUTO_BED_LEVELING_3POINT)
        #define UTILITY_ABL_TYPE "3POINT"
      #elif ENABLED(AUTO_BED_LEVELING_UBL)
        #define UTILITY_ABL_TYPE "UBL"
      #endif
      SERIAL_ECHOLNPGM("Auto Bed Leveling: " UTILITY_ABL_TYPE);
      if (planner.leveling_active) {
        SERIAL_ECHOLNPGM(" (enabled)");
        #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
          if (planner.z_fade_height)
            SERIAL_ECHOLNPAIR("Z Fade: ", planner.z_fade_height);
        #endif
        #if ABL_PLANAR
          SERIAL_ECHOPGM("ABL Adjustment X");
          LOOP_XYZ(a) {
            float v = planner.get_axis_position_mm(AxisEnum(a)) - current_position[a];
            SERIAL_CHAR(' ');
            SERIAL_CHAR('X' + char(a));
            if (v > 0) SERIAL_CHAR('+');
            SERIAL_ECHO(v);
          }
        #else
          #if ENABLED(AUTO_BED_LEVELING_UBL)
            SERIAL_ECHOPGM("UBL Adjustment Z");
            const float rz = ubl.get_z_correction(current_position);
          #elif ENABLED(AUTO_BED_LEVELING_BILINEAR)
            SERIAL_ECHOPGM("ABL Adjustment Z");
            const float rz = bilinear_z_offset(current_position);
          #endif
          SERIAL_ECHO(ftostr43sign(rz, '+'));
          #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
            if (planner.z_fade_height) {
              SERIAL_ECHOPAIR(" (", ftostr43sign(rz * planner.fade_scaling_factor_for_z(current_position.z), '+'));
              SERIAL_CHAR(')');
            }
          #endif
        #endif
      }
      else
        SERIAL_ECHOLNPGM(" (disabled)");

      SERIAL_EOL();

    #elif ENABLED(MESH_BED_LEVELING)

      SERIAL_ECHOPGM("Mesh Bed Leveling");
      if (planner.leveling_active) {
        SERIAL_ECHOLNPGM(" (enabled)");
        SERIAL_ECHOPAIR("MBL Adjustment Z", ftostr43sign(mbl.get_z(current_position), '+'));
        #if ENABLED(ENABLE_LEVELING_FADE_HEIGHT)
          if (planner.z_fade_height) {
            SERIAL_ECHOPAIR(" (", ftostr43sign(
              mbl.get_z(current_position, planner.fade_scaling_factor_for_z(current_position.z)), '+'
            ));
            SERIAL_CHAR(')');
          }
        #endif
      }
      else
        SERIAL_ECHOPGM(" (disabled)");

      SERIAL_EOL();

    #endif // MESH_BED_LEVELING
  }

#endif // DEBUG_LEVELING_FEATURE
