# Case Delta revision A — validation record

Author: jayis1

## Scope

This record covers executable software and native CAD validation in the repository. It does not claim physical manufacture, firmware flashing, calibrated timing, demonstrated switch reliability, tool identity, field usefulness or production readiness. All captures used in tests are synthetic. The external NUCLEO-F401RE and mechanical case insert have not been assembled or measured for this design.

## Reproducible commands and observed outcomes

From the repository root:

```
make -C case-delta/firmware clean all test fixture
xvfb-run -a python3 -m unittest discover -s case-delta/tests -p 'test_*.py'
/usr/bin/python3 case-delta/tools/check_hardware.py
python3 -m compileall -q case-delta/app case-delta/tools case-delta/tests
```

- GNU Arm Embedded GCC 14.2.1 cross-compiled and linked the complete STM32F401 target with `-Wall -Wextra -Werror`, no vendor HAL, and the included startup/linker files. ELF size: 2,700 bytes text, zero initialized data, 788 bytes BSS. The binary image was generated successfully. Stack usage is additional and protected by the linker reservation, not included in BSS.
- GCC host engine tests passed under AddressSanitizer and UndefinedBehaviorSanitizer. All 512 combinations of initial, intermediate and final three-pocket inventory were compared with independent set algebra. Regressions passed for exact closure credit, rejected raw glitches, unqualified lid openings, sealed-pocket disturbance, exact timeout precedence, acquisition gap abandonment, millisecond wrap, start/sequence/UART counter exhaustion, CRC reference vector and short output buffers.
- Production clock/input-configuration and UART driver tests passed under both sanitizers. Clock tests exercise bounded failure waits and configuration masks; UART tests cover buffer bounds, busy rejection, nonblocking drain, timer wrap and timeout recovery.
- Thirteen Python tests passed, including two real Tk event-loop GUI tests under Xvfb. Tests cover real C-encoded fixture replay, byte-at-a-time framing, malformed CRC and truncation resynchronization, impossible returned masks, retained-record mutation, rollback, replay EOF and FIFO rejection, serial attribute restoration, non-TTY rejection, PTY hangup, freshness, secure CSV creation, overwrite refusal, and stopped acquisition despite advancing timestamps.
- GUI tests exercised all three tabs, completed-session display, retained evidence after failed replay replacement, real PTY serial input, double-connect refusal, unplug handling and empty display on a replacement connection. The serial hardware was simulated with Linux PTYs; physical USB and Windows/macOS are not validated.
- Python byte compilation passed for application, tools and tests.
- KiCad CLI and pcbnew 9.0.2: ERC zero violations; DRC zero violations; zero unconnected items. Exported schematic XML, PCB pads and connectivity manifest agree for fourteen components, thirty pins and ten nets, including values and full library footprint identities.
- Native schematic SVG was rendered to PNG and visually inspected: four correctly named pocket/lid channels, resistor values, MCU-interface header labels, supply/ground labels and jayis1 title block are legible without label overlaps. The actual Tk dashboard was captured and inspected with replay/disconnected/not-live/HALTED provenance and the third synthetic result displayed correctly.
- README contains 3,521 whitespace-delimited words. Firmware `.c` files contain 516 nonblank, noncomment lines; headers add 79. Tests and vendor files are excluded from these code counts. No line-count claim is a substitute for the behavioral tests above.

## Review finding corrected

A receiver-side review found that advancing timestamps with a stopped acquisition sequence could indefinitely renew the LIVE label after an earlier sequence advance. A regression reproduced the failure before the fix. The model now separately tracks the receipt time of the last acquisition-sequence advance and latches the capture blocked after two seconds without one. Timestamp-only traffic does not renew that deadline. The regression and full suite pass with the fix.

## Electrical cross-check

Nominal resistor-network calculations were executed independently: 3.3 V / 11 kohm = 0.300 mA per closed contact; 1 kohm series drop = 0.300 V; four closed networks dissipate 3.96 mW total. These exclude the development board and host. They are calculations, not measurements. Firmware mapping agrees with the README and carrier: PA0/A0 pocket A, PA1/A1 pocket B, PA4/A2 pocket C and PB0/A3 lid; J3 through J6 pin 2 are ground. USART2 TX is PA2 AF7.

## Remaining engineering gates

Verify the purchased Nucleo revision, VCP bridge configuration, supply rail and GPIO margins. Fabricate and inspect the carrier before power-up. Measure contact wetting performance, mechanical drag, lid-before-pocket ordering and closure stability. Confirm physical USB behavior, watchdog recovery, sampling jitter and real clock drift. Compare observed masks with independent real-world observations and test whether the insert improves a workshop decision. Review cable ESD, enclosure strain relief and contact fault ambiguity. No fabrication-ready product, safety certification, security audit or globally novel invention is asserted.
