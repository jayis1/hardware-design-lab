# Age Rack validation record

Author: jayis1

## Scope

Revision A is an unbuilt prototype. This record separates executed software and CAD checks from physical verification. Standalone owner execution used the supplied advisory reports; no independent collective implementation review is claimed. Original design, code and documentation authorship is jayis1.

## Reproduction

From age-rack run `/usr/bin/python3 tools/verify.py`. The command clean-builds and links target firmware, executes host C tests with AddressSanitizer and UndefinedBehaviorSanitizer, regenerates the synthetic protocol fixture, runs Python tests with a real Tk event loop under Xvfb, runs KiCad ERC/DRC and schematic-to-PCB connectivity comparisons, and checks document/source volume and file attribution.

Toolchain exercised: GNU Arm Embedded GCC 14.2.1; KiCad CLI 9.0.2; system Python 3.13.5. The target image links for STM32F401RE Cortex-M4 without a vendor SDK or standard C runtime: 2824 bytes text, zero initialized data, 788 bytes static BSS. Stack storage is additional and constrained by the linker assertion; these numbers are not physical memory measurements.

## Executed acceptance checks

- Engine: all 65,536 four-mask words from an empty baseline compared against an independent cohort-array oracle. Each word contains four groups drawn from all sixteen occupancy masks. This covers arbitrary insertion/removal groups, tied placement, simultaneous removal, rolling reuse, rank intervals and retained evidence.
- Boundary tests: held-occupied startup, five-sample qualification, unrelated chatter, invalid raw values, missed acquisition, terminal freeze, ordinary uptime rollover, sequence exhaustion, last-ticket insertion and transactional two-insertion exhaustion with only one ticket left.
- Serialization: known CRC check vector, null output, insufficient capacity, valid engine snapshots and rejection of inconsistent engine accounting.
- Platform drivers: actual clock code tested for bounded waits and GPIO/UART configuration; actual serial code tested for bounds, busy rejection, nonblocking transmission, wrap, deadline timeout and recovery.
- Companion: nine unittest cases cover the C-encoder fixture, strict replay, CRC and truncated-frame recovery, CRC-valid impossible snapshots, retrospective stamp changes, classification rollback, exact single-removal histogram attribution, freshness, private exclusive export, FIFO/partial replay rejection, serial restoration and invalid serial paths.
- GUI integration: actual Tk event loop opens a Linux pseudo-terminal, accepts C-generated AR1 data, renders all trace rows, exercises all tabs, refuses double-connect, exports CSV, detects peer hangup and preserves evidence, and distinguishes replay from live acquisition. This uses synthetic contact histories, not a physical device.
- CAD: native schematic export succeeded; ERC found zero violations; DRC found zero violations and zero unconnected items. Every schematic pin and PCB pad matches the manifest: fourteen components, thirty pins, ten nets. Values and full library footprint identifiers also match.
- Visual check: KiCad-generated schematic SVG rendered with rsvg-convert and inspected. Power and four sense-header pins, four pullup/series networks, RACK_A through RACK_D labels and jayis1 title-block credit are visible without label overlap. The optional CairoSVG package was absent; the installed renderer provided the image instead.
- README: 3,624 whitespace-delimited words. Firmware: 562 nonblank, noncomment C lines plus 77 header lines, excluding tests and build output. Source includes genuine shared board support as explicitly documented, not a claim that every line was newly invented.
- AR1: 32 ordered fields checked against the encoder/model. Nominal divider calculations checked programmatically: 0.300 mA per closed contact, 0.300 V sense node, 1.20 mA for four closed contacts.

Initial test failures established missing engine/model/UI dependencies before implementation. One fixture expectation was corrected after reviewing the documented group rule: simultaneous C/D departure produces ranks zero and one against the pre-change inventory, not two rank-zero removals. The independent exhaustive oracle and final app fixture now agree.

## Unverified physical work

No fabrication, target flashing, physical switch handling, supply measurement, logic margin test, calibrated timing, watchdog reset experiment, brownout test, ESD/EMC qualification, enclosure-fit test or actual specimen experiment was performed. Zero CAD violations does not establish assembly or safety. The purchased Nucleo and external harness remain outside the carrier's passive-only ERC scope. Supply allowances, fixture dimensions and budget figures are planning assumptions.

The protocol is unauthenticated. It retains only volatile history and snapshots, not an exhaustive event log. Short glitches may be rejected by qualification, same-sample groups do not prove physical simultaneity, and some observer resets remain indistinguishable from forward traffic. The rack neither identifies specimens nor assesses cure, sterility or handling safety.
