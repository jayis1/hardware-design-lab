# Stroke Weave validation record

Author: jayis1

## Scope and reproducible command

Run `python3 tools/verify.py` from this directory. The verification run uses real host tools, a real freestanding STM32F401 target link, synthetic switch inputs, Linux pseudo-terminals and Xvfb. It does not claim a physical pump, MCU or USB device was exercised. Review was performed in standalone-owner mode with advisory planning material; no full collective or independent hardware sign-off is claimed.

## Observed results

- arm-none-eabi-gcc 14.2.1: Cortex-M4 ELF and binary built with C11, `-Wall -Wextra -Werror`, freestanding startup, board drivers and linker script. ELF size: 2800 bytes text, zero initialized data, 788 bytes BSS; stack headroom is separately enforced by the linker.
- Three sanitizer-enabled C executables passed: engine/wire, reset-clock/configuration, and nonblocking serial driver.
- Engine tests enumerate 256 eight-delivery words against an independent last-letter/count oracle, checking both cylinders, alternating/repeated pairs and peak streak. Additional cases cover tied deliveries, canceled excursions, missing-middle direct swaps, geometry faults, sampling gaps, deadline equality versus one sample earlier, independent peer chatter, transactional start exhaustion, timer wrap, CRC and encoder capacity.
- Python 3.13.5: seventeen unittest cases passed, comprising sixteen model/transport tests and one multi-step real Tk event-loop test under Xvfb.
- The C-generated synthetic fixture ends with A deliveries=2, B deliveries=3, alternating=1, repeated=1, ties=1 and A canceled=1. These are test inputs, not physical measurements.
- Companion checks include the production encoder-to-parser path, CRC/framing recovery, incomplete tails, impossible field combinations, counter rollback, active duration growth, completion backdating, tie-anchor invalidation, freshness, backlog detection, private exclusive CSV creation, FIFO replay rejection and serial terminal restoration.
- GUI verification visits every screen, replays the C fixture, reconnects with cleared summaries, refuses a second port, consumes the fixture through an actual Linux PTY, verifies advancing acquisition and retained rows, exports CSV, detects PTY hangup, preserves evidence and returns to replay.
- KiCad 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. No suppression or waiver was added.
- Exported schematic XML, actual PCB pads and the connectivity manifest agree on fourteen components, thirty connected pins and ten nets. Component values and full footprint-library identifiers also match.
- The schematic was exported to SVG and rendered for visual inspection. Endpoint names, resistor values, connector numbers and author title block are readable without label overlap. Passive-symbol ERC does not qualify the separately purchased processor assembly or external sensor mechanics.
- README: 3361 whitespace-delimited words. Production firmware: 580 nonblank, noncomment C lines plus 82 header lines, excluding tests and build products. All required firmware, app and KiCad files are present.
- Every deliverable contains jayis1 attribution in a valid comment, document text or supported metadata field. Reused jayis1 platform code is identified in tools/import_platform.py; upstream KiCad footprint licensing remains acknowledged.

## Regression development and review

Engine, application and GUI tests were first run before their corresponding implementations existed and failed on missing code. During owner review, two additional negative tests failed because the companion accepted an impossible backdated full-stroke duration and a retained order anchor after an exclusively tied delivery increment. Both checks were implemented and the complete verification suite rerun successfully. Endpoint validators remain necessary consistency bounds, not a formal proof of every hidden event in sparse snapshots.

The only Pallet Census name references retained are the README's explicit comparison and the import helper's source substitution. A source scan found no TODO/FIXME/stub markers or obvious hardcoded-password/API-key assignments. Build output, Python caches and KiCad local preferences are ignored, not shipped as source.

A supplemental read-only code review identified missing necessary bounds on delivery/refill balance, exact ordering after a single new delivery, and terminal abandonment ages. The owner reproduced these findings with failing tests, added matching C/Python snapshot invariants and sparse-transition checks, and reran all checks. Full strokes must alternate direction within each lane; abandonment occurs at most once per lane and only at terminal halt. The single-delivery ordering transition is exact rather than merely upper-bounded. This focused software review is not collective hardware certification.

## Outstanding physical verification

No carrier fabrication, assembly, MCU flashing, actual VCP enumeration, reed/magnet calibration, scope measurement, watchdog-reset trial, USB electrical test, current measurement, environmental test or pump experiment has been performed. No calibration, pressure, volume, flow, safety, ESD, EMC or regulatory qualification is implied. Physical bring-up must follow the README and use a supervised low-energy fixture. The design does not control equipment and must not be connected to energized machinery wiring.

Global market novelty is unproven. The bounded prior-art search found general pump-testing patents; it is not legal clearance. The distinct repository contribution is the independent full-stroke qualification and delivery-order/tie workflow, not generic pumping or contact sensing.
