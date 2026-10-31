# Rebound Rungs validation record

Author: jayis1

## Scope and status

Revision A source and CAD checks were performed in degraded standalone-owner mode. Peer reports were advisory. A read-only implementation review was also used; the owner applied and tested the corrections. No claim of a full collective review or physical validation is made.

Repository: jayis1/hardware-design-lab. The starting default branch was main at 8452cdb97547687fe3ba1a728ac2b0fc1ba2fc10. The working tree was clean and `git pull --ff-only` reported already up to date before implementation. Existing device directory titles and the closest measurement contracts were inspected before selecting this concept. No task branch was created.

## Executed checks

- `make -C rebound-rungs/firmware clean all test`: passed. GNU Arm Embedded GCC 14.2.1 linked the actual STM32F401 Cortex-M4 target, then produced its binary. ELF size: 2612 bytes text, zero initialized data, 788 bytes BSS. Stack is additional and bounded by the supplied linker layout.
- Three host C test executables passed with `-Wall -Wextra -Werror` and address/undefined-behavior sanitizers. Engine coverage includes all 256 four-event words following descent against a separate list oracle, all four result bands, incorrect descent choices, skipped ascent rungs, raw overlap, acquisition gaps, timeout precedence, qualification boundaries, initial occupancy, timestamp wrap, saturation and encoder rejection of impossible evidence. Clock and UART tests exercise the imported production drivers.
- `xvfb-run -a python3 -m unittest discover -s rebound-rungs/tests -p 'test_*.py'`: nine tests passed using Python 3.13.5. Tests compile and execute the real C encoder, decode its synthetic trace in seven-byte chunks, validate framing and temporal invariants, check replay restrictions, and exercise Tk tabs, serial ownership, PTY hangup, failed replacement, private exports and overwrite refusal.
- The headless companion validated the actual generated synthetic RR1 recording and returned: `Rebound returned through rung 1; did not visit rung 2`. This is generated test input, not a measured rebound.
- `/usr/bin/python3 rebound-rungs/tools/check_hardware.py`: passed using KiCad 9.0.2. Schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. A freshly exported schematic netlist matched the manifest and actual PCB across 14 components, 30 pins and 10 nets, including values and full footprint library identifiers.
- The schematic was exported to PDF and SVG and the rasterized PDF was visually inspected. Connector groups, pullups, series resistors and gate labels were readable without visible collisions. Native sources, not generated PDFs or scratch images, are the delivered CAD artifacts.
- `python3 rebound-rungs/tools/check_quality.py`: passed. README: 3334 whitespace-delimited words. Production `.c` files: 601 nonblank lines after removing comments, excluding headers and tests. All deliverable files carry jayis1 attribution. The checker also verifies text-only files, bounded sizes, connector roles, resistor values and basic credential-pattern screening.

## Review corrections

The read-only review found that the companion could accept newly reported crossings backdated before the preceding active snapshot, and could accept a completed trial longer than the elapsed interval since a prior idle snapshot. It also found that both validators accepted DONE at the firmware's rejecting 5000 ms deadline. New tests first reproduced those failures. The validators now reject them, enforce minimum qualified-crossing spacing, and preserve earlier visit timestamps and terminal evidence. All firmware and application checks were rerun after those changes.

## Unverified or limited

No carrier was fabricated or connected; no target was flashed; no USB device, actual reed switch, magnet, guide, coupon or falling slider was tested. The actual reset/oscillator behavior, watchdog period, cable noise susceptibility, wetting current, contact hysteresis, operating speed, release repeatability, rebound interpretation, mechanical safety and useful discrimination between coupons remain unverified. CAD checks cover the passive carrier only; they do not certify the external Nucleo or a hand-built fixture.

The manufacturer manual could not be retrieved during this run: the initial request failed and an HTTP/1.1 retry timed out. Pin and register definitions reuse the repository's existing platform and require confirmation against the exact purchased board revision before assembly. Public prior-art search confirmed that commercial rebound testers exist; worldwide uniqueness is not asserted. The design deliberately does not claim standards compliance, a coefficient of restitution, a calibrated peak height or production readiness.

Build outputs, raw synthetic fixtures, local KiCad preferences and rendered inspection images are ignored or kept in scratch. Only attributed source, documentation, tests and native CAD are intended for publication.
