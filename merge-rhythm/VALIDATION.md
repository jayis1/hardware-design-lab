# Revision A verification

Author: jayis1

This package was recovered from an interrupted execution of the same authorized device-design task. The recovered sources were inspected and all checks below were freshly executed before publication. No unrelated files were incorporated.

## Executed checks

- `make -C merge-rhythm/firmware clean all test`: passed with GNU Arm Embedded GCC 14.2.1. The actual Cortex-M4 ELF links with 3056 bytes text, zero initialized data and 788 bytes BSS. Binary generation passed.
- Host C engine, clock and serial test executables passed with address and undefined-behavior sanitizers. Engine coverage includes all 256 eight-admission source sequences, independent qualification, queue conservation, timeout boundaries, ambiguous inputs, rearming, acquisition gaps, timer wrap and transactional counter exhaustion.
- `xvfb-run -a python3 -m unittest discover -s merge-rhythm/tests -p 'test_*.py' -v`: 13 tests passed on Python 3.13.5. These exercise the real Tk event loop, Linux pseudo-terminals, C-encoded synthetic records, replay, exports, disconnects, corruption, constructor rollback and progress validation.
- `/usr/bin/python3 merge-rhythm/tools/check_hardware.py`: passed using KiCad 9.0.2. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh exported schematic connectivity matches PCB and manifest for all 14 components, 30 pins and 10 nets; values and footprint identifiers also match.
- Schematic exported to SVG and rendered to PNG for visual inspection. The four pull-up/series-resistor channels, signal connectors, supply connector, external MCU header and author title block are legible. Connections use named nets rather than long crossing wires.
- `python3 merge-rhythm/tools/check_quality.py`: README contains 3462 whitespace-separated words. Production firmware contains 572 nonblank C lines after comment removal, plus 85 header lines. Tests are excluded from those counts. All deliverable files carry jayis1 attribution using valid format metadata or comments.

## Scope and originality review

The repository is jayis1/hardware-design-lab and the remote default branch is main. Existing device directories and README titles were inventoried again during recovery. Route Census was checked directly: it observes one inlet and three outlets with isolated trials, unlike this two-inlet/one-outlet FIFO and source-streak instrument. Existing passive carrier support is explicitly reused; the measurement algorithm and workflow are distinct. The README describes established merging prior art and does not claim patentability or proven worldwide novelty.

## Evidence limits

Host tests use synthetic stimuli, not physical measurements. The PCB is a passive carrier for a separately purchased NUCLEO-F401RE. Fabrication, target programming, electrical bring-up, timing accuracy, mechanical gate disturbance, contact lifetime and the no-overtaking fixture assumption remain unverified. The manufacturer's current board manual must be checked against the physical board revision before assembly. There is no safety certification, isolation, external-voltage protection or production-machine control claim.

Build products, Python caches, local CAD preferences and review renders are intentionally excluded from publication. No third-party package installation is required for the companion; Python's Tk and Linux termios support are required. Exact final Git commit and remote verification are reported separately because embedding a commit's own hash in its contents is not reproducible.
