# Duplex Witness validation record

Author: jayis1

## Executed checks

- Target: arm-none-eabi-gcc 14.2.1, Cortex-M4 Thumb, freestanding, warnings as errors. Clean ELF link and BIN generation passed. Final linked sections: 2460 bytes text, zero initialized data, 788 bytes BSS. No target was flashed.
- Host firmware: GCC address and undefined-behavior sanitizer tests passed. The exact-coverage oracle enumerates 511 face words through eight strokes. Additional regressions cover raw detent loss on the qualifying press endpoint, duplicate faces, tied face/press changes, glitches during a press, unload while down, acquisition gaps, exact timeout precedence, counter exhaustion, timer wrap, startup while occupied, CRC check vector and undersized encoding buffers.
- Platform: bounded clock restore, GPIO/UART configuration checks, UART busy rejection, queue bounds, nonblocking drain and timeout recovery tests passed.
- Companion: Python 3.13.5; eight unittest methods passed. Tests exercised malformed and impossible CRC-valid frames, fragment resynchronization, stale/duplicate/backlogged telemetry, private exclusive export, bounded regular-file replay, terminal restoration and non-TTY rejection.
- GUI: actual Tk event loop under Xvfb, four tabs, production C engine/encoder fixture replay, three distinct outcomes, local CSV export, failed connection preserving prior evidence, reconnect clearing the view, double-connect refusal, simulated serial via a POSIX PTY, and hangup detection passed. These are simulated inputs, not physical USB or stamped tags.
- Hardware: KiCad 9.0.2 schematic ERC found zero violations. PCB DRC found zero violations and zero unconnected items. Fresh schematic netlist and PCB comparison passed for 14 components, 30 pins and 10 nets, including values and footprint identities. Schematic SVG was rendered and visually inspected for readable labels and intended connections.
- Content: README 3641 whitespace-delimited words; firmware 612 nonblank, noncomment C/header lines, excluding tests. The checker verifies format-compatible jayis1 attribution on every nonignored deliverable, wiring manifest structure and C/Python protocol field order.

## Review and fixes

The owner resolved advisory findings and made all repository writes. A targeted read-only review identified that a raw face contradiction at the qualified press edge tainted the trial but still credited face coverage. A new regression reproduced the issue before the guard was corrected. Coverage now also requires raw face agreement. Final matching had already been blocked, but the fix prevents overstating active or retained coverage.

An earlier test exposed premature disarming while the latch was still qualifying. READY now waits for latch qualification while continuing to reject a raw press. The full oracle suite passes after both changes.

## Reproduce

From the device directory:

    make -C firmware clean all test
    xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_quality.py

Generated build products, bytecode and KiCad local preferences are ignored. Hardware regeneration is optional with `/usr/bin/python3 tools/generate_hardware.py`; it requires system KiCad Python bindings and installed standard footprint libraries. It rewrites only this device's hardware artifacts. The platform import helper is for recreating missing files in a fresh scaffold and intentionally refuses to overwrite an existing project.

## Limits

No fabrication, physical programming, power measurement, switch-force measurement, ink-transfer inspection, mechanical calibration, ESD/EMC assessment or user trial occurred. Direct ST PDF downloads failed; indexed UM1724 connector and solder-bridge tables were available and checked. RM0368 register definitions are reused from the existing repository platform, not newly claimed as independently datasheet-audited. A matching electrical episode is not proof of two printed faces, continuous object identity or safe machine operation. See README for the complete measurement contract and bring-up procedure.
