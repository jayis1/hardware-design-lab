# Closure Basis verification record

Author: jayis1

## Status and scope

Revision A is target-build-verified and CAD-checked, not physically validated. This run used standalone-owner implementation and review. Supplied peer reports were advisory process reviews, not independent inspections of these artifacts. No claim of full collective artifact review is made.

## Reproduced results

- `make -C closure-basis/firmware clean all test fixture` passed with GNU Arm Embedded GCC 14.2.1, C11, Cortex-M4/Thumb, freestanding linking, `-Wall -Wextra -Werror`. ELF size: 2616 bytes text, 0 initialized data, 1044 BSS. A real target ELF and binary were generated, not only a native simulation.
- The sanitizer-enabled C analysis/acquisition test passed for all 65,536 retained configuration sets, every antecedent and singleton consequent, all 256 ordered raw-word pairs, qualification boundaries, disturbance, missing acquisition, timestamp wrap, terminal faults, saturation and bounded CRC encoding. The analysis oracle uses explicit per-bit absence counts rather than the production intersection implementation.
- Separate sanitizer-enabled tests passed against the production clock/configuration and UART drivers: bounded clock failures, pin checks, transmitter bounds, busy rejection, nonblocking drain, timer wrap and timeout recovery.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s closure-basis/tests -p 'test_*.py'` passed all 12 tests with Python 3.13.5. This includes independent exhaustive analysis, real C-encoder sparse and dense fixtures, all forward pairs of sparse fixture snapshots, malformed/CRC-valid impossible frames, exact adjacent visit attribution, replay size/type/EOF limits, POSIX serial restoration, real Tk screens, CSV permissions, overwrite refusal, stale acquisition, reconnect, unplug and failed opens.
- The sparse fixture contains 48 snapshots and the dense fixture 960 snapshots of synthetic input configurations. They demonstrate provisional A -> B disappearing after observation of raw word 1. They are not physical measurements.
- `/usr/bin/python3 closure-basis/tools/check_hardware.py` passed with KiCad 9.0.2: zero ERC violations, zero DRC violations, zero unconnected items. A freshly exported schematic XML netlist matched the manifest and PCB for all 14 components, 30 pins and 10 nets, including values and full footprint-library identifiers.
- The schematic was exported with the native KiCad CLI, rasterized, and visually inspected. The four independent resistor/contact channels, supply and sense connectors, net labels and jayis1 title credit were legible without label collisions. Rendering is a documentation check, not a substitute for rule checks.
- README word count: 2905 by whitespace splitting. Production firmware `.c` files contain 645 lines; 586 remain after removing block/line comments and blank lines. Tests and headers are excluded from that substantive-code count.
- All authored deliverables contain jayis1 attribution. Source scanning found no TODO/FIXME stubs, embedded credentials, dynamic eval/exec or shell=True patterns. Build products, Python caches and local KiCad preferences are ignored.

## Limits and physical acceptance work

No circuit was fabricated, powered or measured; no MCU was flashed. GPIO timing, HSI accuracy, watchdog behavior on a real target, USB behavior with a physical board, contact wetting, ESD tolerance, enclosure fit and switch loading remain unverified. The custom PCB is only a passive interface for an external NUCLEO-F401RE. All-pass passive ERC does not validate the external supply or cable orientation.

No final mechanical bracket CAD, calibrated switch installation or user study is claimed. The README specifies the fixture boundary and inspection procedure; actual brackets must match the detached mechanism. No runtime credentials, network services, actuator commands or persistent storage are present. CRC and host consistency checks detect some corruption and impossible evidence but are not authentication, reset-proof provenance or proof of a hidden history.

The firmware and app implement supported positive implications, not causal inference, negative-literal reasoning, a canonical stem basis or verified reachability. Worldwide novelty and market absence are unproven. Prior-art search results included established formal concept analysis and locking-system applications; the narrower repository differentiation is documented in README.md.

Before interpreting physical results, continuity-check the exact header map, measure all open/closed voltages, reproduce the six-configuration fixture on independent disconnected switches, then test all sixteen words. Evaluate raw glitches and gaps, reset and unplug recovery, repeated approach directions, switch hysteresis and mechanical loading. Preserve the fixture mapping alongside every exported session. Never connect this carrier to an energized circuit or use its provisional rules as interlock permission.
