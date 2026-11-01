# Balance Order validation record

Author: jayis1

This record describes executed file-level checks, not fabricated hardware results. Revision A remains an unbuilt prototype. Reproduce with `python3 balance-order/tools/verify.py` from the repository root.

## Executed and passed

- Target build and link with arm-none-eabi-gcc 14.2.1, Cortex-M4 Thumb, freestanding C11, warnings as errors. ELF inspection identifies a 32-bit ARM executable. Final reported image sections: 2,792 bytes text, zero initialized data, 788 bytes BSS. Stack allowance is checked by the linker script.
- Production engine tests compiled with host GCC and AddressSanitizer/UndefinedBehaviorSanitizer. All 512 possible retained three-pair outcome masks agree with an independent half-deadband assignment oracle. The tests distinguish neutral-chain compatibility from equality closure.
- Engine tests cover no baseline, one result per key insertion, stable both-contact rejection, selector changes, exact settlement endpoint, raw glitch restart, exact timeout precedence, acquisition loss, terminal admission exhaustion and device-time wrap.
- Production clock and UART driver host tests pass. They exercise register masks, bounded clock failures, receiver-disabled configuration, null/oversized submissions, busy rejection, nonblocking drain, timer wrap and transmit-timeout recovery. Simulated registers do not validate physical register behavior.
- Ten Python unittest cases pass with Python 3.13.5. They include all 512 graph masks through the independent companion solver, actual C-encoder replay, malformed/truncated records, invalid accounting, counter migration, aborted-without-halt rejection, and outcome-specific settlement continuity.
- Real Tk widgets and event processing exercised under Xvfb using a Linux pseudo-terminal and timestamp-paced production C-encoder frames. Connection, duplicate-connect refusal, ledger rows, incompatible verdict, export, replay, reconnect clearing and actual PTY hangup pass. These are synthetic transport tests, not a connected STM32.
- Private CSV export tested for mode 0600, author/provenance metadata and overwrite refusal. FIFO replay rejection and non-TTY serial failure tested. Python app/tests/tools compile successfully.
- KiCad CLI 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items.
- Fresh schematic XML netlist agrees with PCB pads and the connectivity manifest for all 14 components, 30 pins and 10 nets. Values and full footprint-library identifiers agree.
- Schematic exported to PDF, rasterized and visually inspected. Four pullup/series/contact channels, supply and signal headers, net labels and jayis1 attribution are readable; no visible label collisions were found. PDF/raster exports remain scratch artifacts, not checked-in deliverables.
- README contains 3,362 whitespace-delimited words. Production C and headers contain 595 nonblank, non-comment lines, excluding tests. Authored source/design files carry jayis1 attribution; generated binaries and caches remain ignored.

## Review corrections verified

A sparse telemetry sequence could previously move 499 ms of neutral settlement into a different completed tilt outcome one millisecond later. Regression tests now reject that sequence and the corresponding continuing-ACTIVE variant. A second regression enforces that an aborted trial requires terminal HALT; the same invariant is checked by the C encoder validator. Both failures were reproduced before correction and the complete suite was rerun afterward.

## Limits and remaining physical acceptance

No PCB fabrication, assembly, target flashing, measured current, GPIO voltage measurement, ESD testing, oscillator measurement or watchdog timing experiment occurred. No real coupon, balance beam, magnet, reed threshold, flexure, deadband or human workflow was tested. The common symmetric threshold is a model assumption requiring assembled calibration, especially because reed hysteresis can invalidate it. The UI explicitly withholds gram values and a certified mass ranking.

The manufacturer Nucleo manual download timed out; pin/register mapping uses the documented existing repository platform and cited UM1724/RM0368 references. The reed manufacturer's current product/datasheet search results support normally-open logic-level contact selection, not the magnetic geometry. Prior-art search was bounded and cannot establish global originality.

The board's passive-only ERC cannot verify the externally powered controller or harness. Sparse snapshot checks are necessary consistency bounds and not a complete proof of underlying input history. CRC is not authentication. Some resets remain indistinguishable without a persistent boot identifier. These limitations are part of the release contract, not silently waived verification gates.
