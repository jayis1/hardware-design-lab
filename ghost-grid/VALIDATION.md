# Ghost Grid validation record

Author: jayis1

This is a software/CAD verification record for an unbuilt Revision A prototype, not a certificate or physical measurement report. Repository owner execution used advisory review; no claim of full collective hardware review is made.

## Reproduction and observed results

From this directory:

    make -C firmware clean all test fixture
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py

- ARM toolchain: arm-none-eabi-gcc 14.2.1. Freestanding Cortex-M4 target compiled and linked with -Wall -Wextra -Werror. ELF size: 2,224 bytes text, zero initialized data, 788 bytes static BSS. Stack-resident engine snapshots and frame buffers are additional RAM use; linker reserves at least 4 KiB stack headroom.
- Three host C test executables passed with AddressSanitizer and UndefinedBehaviorSanitizer. Engine tests include all sixteen topologies against an independent row/column reachability oracle, all 256 ordered mask transitions, exact startup qualification, raw-glitch segment splitting, acquisition gaps, timer wrap, terminal freeze, last permitted counter increment, transactional overflow, invalid input, bounded encoding and the standard CRC check vector.
- Ten Python unittest cases passed under Python 3.13.5 and Xvfb. Tests cover C-encoded replay, truncated-frame resynchronization, CRC-valid impossible snapshots, startup qualification, exact adjacent bin/run/segment/peak accounting, counter rollback, halted-state freeze, acquisition freshness and contradictory duplicates.
- The real Tk workbench was exercised with production C-encoded synthetic data. Tested sixteen histogram rows, the predicted-phantom grid cell, CSV content and provenance, private file permissions, overwrite refusal, FIFO/oversized replay rejection, non-TTY connection failure, reconnect display clearing, double-connect refusal, initial-fragment recovery, PTY hangup and terminal restoration after ordinary disconnect and injected constructor failure. PTYs are simulated transport, not a USB hardware test.
- KiCad CLI 9.0.2: schematic ERC found zero violations; PCB DRC found zero violations and zero unconnected items. Fresh XML netlist compared exactly with actual PCB pads and the authored manifest: fourteen components, thirty pins, ten nets; values and full footprint library identities also agree. No exclusions were added to hide violations.
- Schematic SVG exported successfully and was rendered to PNG using rsvg-convert for visual inspection. Labels, the four channel rows, power connector, sensing connector and author/title block were readable. A failed optional CairoSVG import was replaced with the installed renderer; no dependency was silently fabricated.
- README contains 3,322 whitespace-delimited words. Production firmware contains 513 C-source lines and 99 header lines; after stripping block comments and blank lines, C plus headers contain 555 lines. Tests and generated artifacts are excluded from those firmware counts. Counts are volume gates, not evidence of correctness.

## Review corrections

Read-only advisory code review identified two malformed-evidence acceptance gaps. Both were first reproduced by failing regression tests and then fixed by the owner: accepting qualified data before six observations, and allowing impossible segment/run/peak changes between immediately adjacent acquired snapshots. The clean target build, sanitizer suites, GUI tests and CAD checks above were rerun after the corrections. This review does not establish complete realizability of every sparse snapshot history.

## Scope and limitations

No physical device was available. There was no fabrication order, component purchase, SWD programming, rail measurement, contact-threshold validation, USB/VCP hardware exercise, calibrated timing comparison, mechanical prototype test, ESD test, EMC assessment or user study. No result here claims those actions occurred.

The four inputs observe detached independent contacts. The hypothetical diode-less scan is an ideal electrical topology calculation; actual keyboard firmware, row drive characteristics, diode networks and operating-system events are not measured. Full four-key closure produces no additional phantom while remaining indistinguishable from any three-corner state to that ideal scanner. Acquisition gaps and chatter create unknown coverage and split observed segments.

UM1724 connector and virtual-COM solder-bridge mapping was checked against retrieved manufacturer search content. Direct page extraction was unavailable; register support is reused from the existing documented RM0368 platform. Component availability was a bounded search observation, not a procurement guarantee. Development-board suitability and every physical connection must still be verified on the actual unit.

All authored deliverables credit jayis1. Reused jayis1 sources have immutable provenance in tools/import_platform.py. Installed KiCad footprint geometry retains its upstream licensing. The repository change is confined to ghost-grid/; no unrelated device or other repository is part of this design.
