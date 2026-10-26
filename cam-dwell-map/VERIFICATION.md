# Cam Dwell Map verification record

Author: jayis1

## Executed checks

Run `python3 tools/verify.py` from this project directory to reproduce the automated gates. These checks use synthetic inputs and host pseudo-terminals; none is a physical measurement.

- GNU Arm Embedded GCC 14.2.1 linked a Cortex-M4 ELF using the supplied startup, linker script and register-level drivers, with `-Wall -Wextra -Werror`. Binary conversion succeeded. Linked sections: 2372 bytes text, zero initialized data, 276 bytes BSS. The stack-resident engine, frame buffer and call stack are additional RAM consumption, not included in BSS.
- GCC 14.2.0 host engine tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. Cases cover the forty-bin half-closed oracle, complete index-to-index turn, reverse motion, two-bit quadrature jump, acquisition gap, missing/early index, timeout, counter exhaustion, timestamp wrap, follower qualification, encoder capacity and impossible result rejection.
- Register-storage clock tests passed for clock switching, bounded failure paths, GPIO configuration and receive-disabled USART setup. These are memory-backed tests, not register observations on an MCU.
- Python 3.13.5: eight application test methods passed. They cover real C-encoded fixtures, CRC/range/conservation validation, truncated-stream resynchronization, replay EOF checks, duplicate freshness, contradictory timestamps, slow backlog, impossible completion rate, terminal-fault freeze, private CSV/overwrite refusal, regular-file replay restrictions and serial PTY hangup.
- The actual Tk 8.6 application was exercised under Xvfb. All three tabs were selected. Synthetic replay produced forty bars and a history row. A real Linux pseudo-terminal supplied live bytes through the production serial class and Tk event loop. Tests confirmed cleared old data on reconnect, stalled-stream freeze, reconnection, disconnection after hangup, CSV provenance and replay replacement. No physical USB device was involved.
- KiCad CLI 9.0.2: ERC zero violations; DRC zero violations and zero unconnected items. No violation exclusions were added.
- Fresh XML schematic export matched the PCB and authored connectivity manifest: fourteen components, thirty pins, ten distinct nets, matching values and full footprint-library identifiers.
- A native netlist was exported as `kicad/device.net`. Schematic SVG export was rendered and visually inspected; left-side labels were adjusted to avoid crowding pin numbers, regenerated, rechecked and inspected again.
- README prose exceeds 2000 words excluding fenced blocks. Firmware has 644 nonblank comment-stripped C/header lines, excluding tests. Every delivered text file includes jayis1 attribution using valid source comments or format-supported metadata.

## Review findings and resolution

Independent read-only source review identified three validation gaps. Slow device-time progress could keep queued frames apparently current; live sessions now compare cumulative device progress with host monotonic elapsed time. An impossible increase in completed turns could pass; the parser session now bounds completion increments by available milliseconds and freezes acquisition fields after terminal fault. An impossible transition count could accompany homogeneous bins; both firmware encoding and application parsing now apply bin-derived change bounds. Regression tests were first run failing, then passed with the corrections.

The transition bounds are necessary rather than sufficient conditions for arbitrary mixed-bin sequences. This limitation is documented rather than presenting the validator as a proof of physical origin. CRC is not authentication. Without persistent boot identity, certain resets remain indistinguishable from forward traffic. The live lag allowance is 750 ms plus two percent of host elapsed time; reception gaps above one second require reconnect independently.

## Attribution and reused platform

All original material is credited to jayis1. Register, startup, clock, GPIO, UART and passive-carrier infrastructure reuse the repository's jayis1-authored Nest Settle platform. Cam-specific acquisition, result validation, protocol, desktop workflow, tests and documentation are supplied here. Library footprints remain KiCad library resources; this project does not claim authorship of their upstream design.

## Not verified

Fabrication, assembly, sensor availability at a supplier, encoder wiring on a purchased unit, target flashing, signal integrity, pulse widths, oscillator tolerance, physical sampling jitter, watchdog recovery on silicon, mechanical repeatability, ESD immunity and regulatory compliance remain unverified. No battery, radio, actuator, motor driver, cloud connection or remote firmware update is implemented. No global novelty or patentability claim has been established. Human electrical and mechanical review is required before building and using the prototype.
