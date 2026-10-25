# Verification — Stencil Orbit revision A

Author: jayis1

## Executed checks

- `make -C firmware check`: passed with arm-none-eabi-gcc 14.2.1, C11, Cortex-M4 Thumb, freestanding linking, `-Wall -Wextra -Werror`. Final ELF sections: text 2004 bytes, data 0, BSS 280. Vector table, startup, linker layout and binary generation were exercised; no physical flash operation was performed.
- Host engine tests passed with GCC address and undefined-behavior sanitizers. Cases cover forward/reverse station indexing, per-station counts, repeats/out-of-order use, simultaneous pickup/registration, motion during tool use, invalid quadrature, gap loss, transient suppression, timer wrap, counter/duration saturation, overdue use, CRC reference vector and output capacity failure.
- Reset-policy regression tests passed: pin reset alone permits home qualification; every automatic reset cause suppresses it, including watchdog reset with the pin flag also present. Production main applies the tested predicate to the board's captured RCC_CSR value. Physical reset-flag behavior remains to be measured on the selected board.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v`: all 7 tests passed. Real C encoder output was consumed by the application. Actual Tk event-loop and pseudo-terminal tests exercised all three screens, serial reception, complete batched history rendering, disconnect, reconnect clearing, replay replacement, and pseudo-terminal hangup. Another test injects constructor failure after serial settings change and verifies restoration. These tests do not constitute physical USB testing.
- Parser tests passed for malformed/oversized input, CRC rejection, truncation resynchronization, suspected restart blocking and staleness. CSV tests verify author/provenance and overwrite refusal.
- `/usr/bin/python3 tools/check_hardware.py`: KiCad 9.0.2 schematic ERC found 0 violations; PCB DRC found 0 violations and 0 unconnected items. No waivers were used. Actual exported schematic netlist and loaded PCB agree with the manifest on 14 components, 30 pins and 10 nets, including values and full footprint identifiers.
- Schematic exported through KiCad SVG and rendered using rsvg-convert; visually inspected for component placement, readable net labels and jayis1 title attribution. All four channels are present, together with power and signal headers.
- README: 3209 whitespace-separated words. Firmware: 664 C/header lines, including 599 nonblank lines after block comments are removed; excludes tests and generated build products. Counts describe substantive implementation, not evidence of physical function.
- Planning arithmetic checked: estimated BOM USD 63; nominal closure current 0.30 mA; four closures dissipate approximately 3.96 mW across the passive network. These remain estimates, not measured electrical values or distributor quotes.

## Review resolution

Read-only independent review identified an unsafe assumption in the original homing implementation: automatic watchdog recovery could relabel whichever registered station was present as zero. The owner corrected this by capturing and clearing reset causes and permitting home only after an explicit pin reset without automatic reset flags. Power-on therefore requires a subsequent deliberate home reset. The dedicated regression test failed before implementation and passed after it. A pseudo-terminal hangup regression also exposed a serial disconnect detection gap; explicit POLLHUP/POLLERR handling now passes the real event-loop test.

## Not verified

No PCB fabrication, assembled-board inspection, programming operation, oscilloscope timing measurement, physical cam construction, switch-life trial, USB hardware-in-loop session, EMC/ESD assessment, electrical safety certification or actual print run was performed. The companion's labels and README consistently describe observation and uncertainty rather than print-quality assurance. The external Nucleo and custom mechanical mounting need separate assembly review. Limited prior-art searches support the documented differentiation, not a global novelty or patentability guarantee.

## Reproduction

Run commands from this directory. GCC, GNU Arm Embedded tools, Python 3 with Tk, Xvfb, KiCad 9 CLI and distribution pcbnew bindings are required. Runtime application dependencies are only Linux Python/Tk and serial-device permissions. Generated firmware outputs and local KiCad preferences are ignored. Test fixtures are generated in firmware/build and are explicitly synthetic.
