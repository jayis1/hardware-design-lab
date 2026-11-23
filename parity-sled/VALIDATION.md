# Parity Sled revision A validation

Author: jayis1

## Scope and status

This is a software/CAD-verified, unbuilt engineering prototype. No fabricated board, physical MCU programming, magnetic fixture measurement, calibrated timing, ESD/EMC testing or user trial was performed. The passive carrier relies on an external NUCLEO-F401RE; native ERC does not validate that module or its harness. The repository target was confirmed as jayis1/hardware-design-lab, with remote default branch main. The initial worktree was clean, and `git pull --ff-only origin main` returned already up to date at 9416166046264ce230d3620dc43793ce0f21dcbd. No task branch was created.

## Executed verification

- `make -C firmware all test fixture`: passed. The Cortex-M4 target linked with warnings treated as errors and emitted ELF and binary artifacts. GNU size reported 2768 bytes text, zero initialized data and 1044 bytes BSS. Stack-resident engine snapshots are additional RAM use; the linker reserves stack headroom.
- Native C engine tests ran with AddressSanitizer and UndefinedBehaviorSanitizer. All 4096 ordered triples of settled four-bit words were compared with an independent population-count and retained-anchor oracle. Glitch rejection, odd-to-odd episode continuation, same/changed/unanchored recovery, exact recovery age, sampling-gap abandonment, terminal abandonment, uint32 time rollover, final permitted sample increment, counter exhaustion and CRC/output bounds passed.
- Native clock tests passed bounded clock-failure and configuration cases using simulated register storage. Native UART tests passed busy rejection, buffer limits, nonblocking drain, timestamp rollover and timeout recovery. These are production-driver unit tests, not tests against physical peripheral registers.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'`: 16 tests passed. Dense production-C-encoded traces cover every ordered raw-word pair; the same records pass sparse session checks at multiple strides. The generated fixture contains all sixteen words and same, changed, unanchored and abandoned excursions.
- Tk tests exercised actual widgets, tab navigation, synthetic serial delivery over a Linux pseudo-terminal, late-attachment fragment recovery, duplicate-connection refusal, evidence-preserving connection failure, serial hangup, terminal attribute restoration, offline replay and CSV export with mode 0600 and overwrite refusal. FIFO and oversized replay inputs were rejected. No physical USB connection or native file chooser was tested.
- CRC-valid negative cases reject impossible conservation, changing retained results, premature qualification, fabricated episode age, qualification-time dwell credit, continued episodes over a single-acquisition gap, and halted evidence mutation. Two sparse-bound defects found during read-only advisory review were reproduced with failing tests and corrected by the owner. The owner performed all repository writes and resolved the findings; this was not a full collective review.
- `python3 -m compileall -q app tests tools`: passed. Python runtime tests are the exercised acceptance gate; no claim is made that a project-wide static type checker was run.
- `/usr/bin/python3 tools/generate_hardware.py`: generated native KiCad sources for 14 components and 10 nets.
- `/usr/bin/python3 tools/check_hardware.py`: passed. Native ERC reported zero violations. Native DRC reported zero violations and zero unconnected items. A fresh schematic XML netlist matched actual PCB pads and the manifest for all 30 pins, including component values and complete footprint-library identifiers.
- Schematic and combined copper/silkscreen plots were generated using KiCad CLI and rasterized with rsvg-convert for visual inspection. Labels, the four input paths, power/header mapping, board title and placement were reviewed. Plot images are scratch inspection aids, not manufacturing or measured-hardware evidence.
- Release checks require every authored deliverable to contain jayis1, the README to exceed 2000 words, and implementation C alone to exceed 500 nonblank/comment-stripped lines. Measured README length is 3788 whitespace-separated words. Firmware contains 549 such C lines and 650 C/header lines, excluding tests. The PY1 schema contains 71 uint32 fields with a theoretical maximum frame length of 790 bytes, below the 1024-byte buffer.

## Tool versions

- GNU Arm Embedded GCC 14.2.1, package 15:14.2.rel1-1.
- Native GCC 14.2.0, Debian 14.2.0-19.
- Python 3.13.5 and Tk 8.6.
- KiCad CLI 9.0.2, with installed system pcbnew and footprint libraries.

The optional cairosvg renderer was unavailable; the installed rsvg-convert renderer successfully produced the inspection PNGs instead. GitHub CLI had no login, but the repository's configured Git HTTPS path successfully performed remote discovery and pull; publication uses Git directly, without printing or changing credentials.

## Reproduce

Run from parity-sled:

    make -C firmware clean all test fixture
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py

Build products, local KiCad preference files and Python caches are ignored. The import script is a one-time provenance aid and deliberately refuses existing destination files. Normal builds do not execute it.

## Remaining physical acceptance

Validate actual open/closed voltage margins, current consumption, Nucleo solder-bridge configuration, watchdog behavior and sample timing. Assemble and inspect the carrier, verify every connector orientation and program only the intended STM32 target. Characterize reed operation with the selected magnet geometry across all lane combinations, neighboring cells, both travel directions and repeated assembly. Verify that every magnet is permanently captive and that exposed glass and leads cannot be handled accidentally.

Establish a useful slow operating cadence and compare measurements to independent electrical and visual references. Demonstrate the even-parity blind spots, including two-bit changes and absent media. Fabrication readiness, enclosure safety, reliable per-position reading, calibrated durations, novel worldwide invention status and product certification are not established by these tests.
