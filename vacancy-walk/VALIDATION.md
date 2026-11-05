# Vacancy Walk validation record

Author: jayis1

## Scope and verdict

Revision A passed the local software and electrical-CAD gates below. This is an unbuilt engineering design, not a physically validated product. Review was standalone-owner execution with advisory reports; there was no full collective sign-off. No fabrication, target flashing, electrical measurements, magnetic characterization, EMC testing or user study was performed.

## Reproducible acceptance command

Run `python3 tools/verify.py` from this device directory. It performs a clean target build, sanitizer-enabled host tests, synthetic C-encoder fixture generation, Python model/transport tests, Tk/PTY integration and KiCad connectivity checks. It also requires the named deliverables, 2,000 README words, at least 500 nonblank noncomment lines in production `.c` files alone, and jayis1 attribution in every non-generated-cache deliverable.

## Observed results

- arm-none-eabi-gcc 14.2.1: Cortex-M4 freestanding link succeeded with `-Wall -Wextra -Werror`; binary generated. ELF size: text 2,676 bytes, initialized data 0 bytes, BSS 788 bytes. The linker separately reserves at least 4 KiB stack headroom.
- Host GCC 14.2.0: engine, clock and serial suites passed with address and undefined-behaviour sanitizers. No sanitizer findings were reported.
- Engine tests enumerate all vacancy/source pairs and all 16 stable arrival masks for every legal pending transfer. Regressions cover cancellations, both vacancy directions, ambiguous direct swaps, additional removals, 40 repeated moves, short raw glitches, rearm violations, acquisition gaps, clock wrap, exhaustion, and arrival just before versus exactly at the deadline.
- UART tests exercise null/oversize/busy rejection, nonblocking transmission, modular timestamp wrap, timeout and subsequent recovery. Clock tests use simulated register storage and exercise bounded failure paths and input configuration masks; they do not emulate physical clocks.
- Python 3.14.9: eight model/transport test methods passed. The actual C-generated fixture has 250 snapshots and ends with two moves, one cancellation and one abandoned lift, in HALT / AMBIGUOUS.
- Real Tk event-loop integration under Xvfb passed: all three screens, replay, batched pseudo-terminal delivery, complete retained-history rendering, cleared labels on reconnect, private CSV export and serial hangup.
- Strict command-line replay validation accepted the same 250 C-generated snapshots. Malformed CRC, contradictory counters, terminal mutation, empty recordings, incomplete tails and FIFO input are separately rejected by tests.
- KiCad 9.0.2: schematic ERC reported 0 violations; PCB DRC reported 0 violations and 0 unconnected items.
- Exported schematic XML, PCB pad nets and the manifest agree on 14 components, 30 pins and 10 nets, including values and full footprint identifiers.
- SVG schematic and combined-copper PCB plots were rasterized with rsvg-convert and visually inspected. The schematic labels and title are legible, component channels are separated, routes are visible and the author credit is present. Initial optional CairoSVG rendering was unavailable; rsvg-convert provided the successful alternative without changing the design.
- README: 3,367 whitespace-separated words. Production `.c` files: 554 nonblank, noncomment lines; including headers: 635. Tests and generated CAD text are not included in the C threshold.
- Attribution scan passed. Repository changes are confined to vacancy-walk. No dependency downloads, workflow changes or credentials are included.

## Review findings resolved before publication

The new experiment uses the reviewed passive carrier and board-support platform rather than presenting it as a complete standalone MCU design. Occupancy is qualified as a bundle to avoid inventing intermediate masks. Moving again during post-result rearm now halts rather than silently reseeding the vacancy. Tests distinguish a deadline-tied arrival from an arrival one sample earlier. Terminal accounting freezes, while transport diagnostics remain permitted. The companion rejects terminal evidence changes and exports provenance and truncation metadata rather than only numeric totals.

## Remaining physical acceptance gates

1. Inspect the purchased Nucleo board revision, solder bridges and USB/VCP routing against ST UM1724. Program and read back the ELF only after verifying supply and wiring.
2. Fabricate the carrier, inspect soldering and polarity, then measure idle and closed-contact voltages with a current-limited supply. CAD checks do not verify the external regulator or harness.
3. Build the nonconductive four-pocket fixture. Establish reed/magnet orientation, release distance, neighbour cross-talk and safe magnet retention. No sensing distance is guaranteed by this design.
4. Capture contact transitions and UART timing on a logic analyzer. Quantify sampling jitter, magnetic hysteresis and rejected bounce against independent observations.
5. Perform known adjacent moves, opposite-source removals, cancellations and two-piece lifts under video. Prove that the chosen geometry exposes the intermediate two-occupied state reliably.
6. Compare the workflow against a paper tally and video with adult educators. Do not describe recorded adjacency as verified puck identity, manufacturing quality or worker performance.

No worldwide novelty or patentability claim is made. The electronics have no isolation, industrial transient protection, actuator output, persistent boot identity or authenticated telemetry. Those limitations are intentional and remain visible in the README and application setup screen.
