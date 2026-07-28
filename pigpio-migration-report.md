# pigpio Migration Report

## Summary

The project hardware access path was migrated from WiringPi to a local pigpio-backed adapter.
The adapter preserves the existing legacy pin numbering used by the codebase and translates it to BCM GPIO numbers before calling pigpio.
GPIO, I2C register access, and servo pulse output now route through `System_Raspberry/pigpio_adapter.c`.

## Modified Files

- `.cproject`
- `.vscode/launch.json`
- `.vscode/rpi_diagnose.ps1`
- `.vscode/settings.json`
- `.vscode/tasks.json`
- `CMakeLists.txt`
- `IRcontrol/IRcontrol.c`
- `PositionControl/PositionControl.c`
- `PositionControl/PositionDirect.c`
- `PositionControl/ZoomControl.c`
- `README.md`
- `System_Raspberry/pigpio_adapter.c`
- `System_Raspberry/pigpio_adapter.h`
- `System_Raspberry/system.c`
- `System_Raspberry/system.h`
- `TestProg/TestProg.c`
- `cmake/toolchains/aarch64-linux-gnu.cmake`

## What Changed

- Removed all direct WiringPi headers and API calls from active source files.
- Added a pigpio runtime adapter loaded with `dlopen`, so the cross-build no longer depends on WiringPi headers or libraries in the sysroot.
- Preserved the project's legacy WiringPi-style pin numbers by mapping them centrally in the adapter.
- Replaced WiringPi I2C access with pigpio `i2cOpen`, `i2cReadByteData`, and `i2cWriteByteData`.
- Replaced WiringPi hardware PWM usage for the zoom servo with pigpio servo pulse output on the mapped BCM pin.
- Added an explicit startup preflight so the process exits early when pigpio is missing or cannot initialize on the Raspberry Pi.
- Updated VS Code diagnostics/debug metadata and project documentation to point at pigpio instead of WiringPi.

## Validation Performed

- Reconfigured the RasPi cross-build with `cmake_configure_rpi.ps1`.
- Built successfully with `cmake --build build-rpi`.
- Checked touched source files for editor diagnostics: no errors reported.
- Audited non-generated files for `wiringPi` references: none remain.
- Verified the startup path now fails fast if pigpio is unavailable instead of continuing into partial hardware initialization.

## Remaining Manual Actions

1. Install pigpio on the Raspberry Pi target so `libpigpio.so` or `libpigpio.so.1` is available in the runtime loader path.
2. Deploy the rebuilt `RasPiAutomation` binary to the Raspberry Pi and run a hardware smoke test for GPIO outputs, IR input/output, AS5048 I2C sensors, and zoom servo movement.
3. Verify the runtime user has the permissions pigpio needs on the target. If `gpioInitialise()` fails, adjust the service/user privileges or run with the required GPIO access.
4. Re-test the zoom servo motion on hardware and, if needed, retune the existing servo pulse values in the zoom configuration. pigpio servo timing is precise, but it is not the same implementation as WiringPi hardware PWM.
5. If the archived binary under `Docu/pi/pi/remote-debugging/RasPiAutomation` is still used operationally, rebuild and replace that artifact separately. It still contains historical WiringPi symbols because it is a checked-in binary, not regenerated source.

## Notes

- The cross sysroot used in this workspace did not contain pigpio headers or libraries during the migration.
- The new adapter avoids that build-time dependency by loading pigpio dynamically on the target at runtime.