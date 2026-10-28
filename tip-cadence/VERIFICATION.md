# Tip Cadence verification record

Author: jayis1

## Executed checks

`python3 tools/verify.py` was run from the device directory against these sources. It performs a clean target rebuild, all host tests, GUI tests, electrical checks, content thresholds and attribution checks. The following results were observed, not synthesized measurements:

- ARM target: arm-none-eabi-gcc 14.2.1, Cortex-M4/Thumb, C11, freestanding, `-Wall -Wextra -Werror`, linked with the supplied startup and memory map. ELF and binary generated. Linker size: text 2608 bytes, initialized data 0 bytes, BSS 532 bytes. Stack and automatic variables are additional to BSS; the linker reserves at least 4096 bytes of headroom.
- Three C test executables passed with AddressSanitizer and UndefinedBehaviorSanitizer. Coverage includes cadence sums, independent lane handling, qualification rejection, gaps, stuck closures, timestamp wrap, transactional overflow, bounded serialization, CRC reference vector, pulse limits 9/10/2000/2001 ms, period limits 99/100/60000/60001 ms, and 100000 deterministic randomized input samples checked against engine invariants. Clock and UART tests use simulated register storage, not a physical MCU.
- Eight Python tests passed under Xvfb. Tests use records produced by the real C encoder, not a separately guessed protocol. They exercise CRC and semantic rejection, duplicate freshness, reset blocking, incomplete replay rejection, resynchronization after truncated output, backlog detection, bounded regular-file replay, private exclusive export, terminal-attribute restoration, and the actual Tk event loop reading and disconnecting a Linux pseudo-terminal. GUI replay and export provenance were also exercised. No physical USB serial device was attached.
- KiCad CLI 9.0.2: schematic ERC found zero violations. PCB DRC found zero violations and zero unconnected items.
- Independent export comparison matched 14 components, 30 reference/pin assignments and 10 nets among fresh schematic XML, the connectivity manifest and real PCB pads. Values and footprint identifiers also matched. Passive-only ERC cannot establish that the external Nucleo supply, jumper configuration or field sensors are correct.
- The schematic was exported as SVG, rasterized and visually inspected. The connector and resistor rows, net labels and author/title block were readable without overlapping functional labels. The board is routed; it is not a placement-only ratsnest.
- README measured 3053 whitespace-separated words. Production `.c` files alone measured 532 nonblank, comment-stripped lines; headers and tests were excluded from that figure. C plus header sources measured 685 physical lines, or 624 nonblank comment-stripped lines.
- Every deliverable source, document and generated CAD/netlist file contains jayis1 attribution in format-valid comments, text or metadata. Ignored build products, Python caches and KiCad local preference files are not deliverables.

## Review and scope

Execution used standalone-owner review with the supplied advisory reports; no full collective design approval is claimed. The repository origin and remote HEAD were checked as jayis1/hardware-design-lab and main, and `git pull --ff-only` succeeded before implementation. Existing device titles and descriptions were inventoried, with the relevant water and rainfall devices compared by purpose. No daily-ideas repository was modified.

Reusable carrier generation, clock, GPIO, UART, startup and driver tests were adapted from the repository's existing jayis1-authored MIT sources. The cadence engine, protocol, application and measurement tests implement this device's distinct contract. Freestanding linking exposed a missing memcpy dependency from the transactional engine copy; an explicit byte-copy implementation was added and the target rebuilt successfully. Self-review also found that export freshness needed connection state as well as recent progress; disconnected exports now retain source provenance but never claim fresh data, and the GUI test verifies that behavior.

The authoritative ST manual's pin table and virtual-COM bridge settings were retrieved through indexed search content. Direct PDF download failed and timed out; no claim is made that a new complete PDF audit was performed. The register driver source is the locally inspected existing STM32F401 implementation, with address/mask tests and a real cross-link. Upstream manual links remain in the README for assembly review.

## Explicitly unverified

No carrier fabrication, PCB physical inspection, target flashing, oscilloscope measurement, electrical safety qualification, ESD test, timing calibration, selected-sensor pulse qualification, actual current measurement, gravimetric bucket calibration or blinded emitter comparison occurred. External component stock and quoted prices were not verified. Manufacturer connector assignments do not excuse inspecting the actual board revision and solder bridges before use.

A positive alternating-cohort contrast is not a diagnosis of unequal bucket sides. Parity is relative to the first observed complete tip, and any break invalidates a pooled physical interpretation. The app intentionally suppresses that contrast after integrity loss. Neither a successful build nor zero EDA violations validates the physical utility target. The novelty research establishes bounded comparisons, not worldwide uniqueness or patentability.

## Reproduction

From this directory run:

    python3 tools/verify.py

This requires GCC, arm-none-eabi GCC/binutils, Python with Tk, Xvfb, and KiCad 9 with its system Python pcbnew bindings and standard footprint libraries. The script does not access the network, flash hardware, commit or push. Build artifacts and local CAD preferences are ignored. To inspect rendered schematics, use `kicad-cli sch export svg kicad/device.kicad_sch` and choose a scratch output directory. To regenerate CAD intentionally, use `/usr/bin/python3 tools/generate_hardware.py`, then rerun the hardware checks and export a fresh netlist before publication.
