# Pole Bridge validation record

Author: jayis1

## Recovery and scope

This design was recovered from the interrupted original hardware-device task. Its originating execution transcript records creation of the firmware tests, shared-platform importer, new topology engine, encoder, companion, GUI tests and README. It was not unrelated user work. Recovery retained the existing implementation and reran validation from a clean firmware build before publication. The destination is jayis1/hardware-design-lab, whose remote default branch is main; no changes belong in daily-ideas or another repository.

This was standalone-owner verification using advisory peer reports, not full collective or independent hardware review. The repository inventory and nearby ContactAtlas contract distinguish per-pole topology partitions from general contact-signature graphs. Worldwide novelty is not established.

The final recovery run independently repeated the complete verification command, including a clean target rebuild, all thirteen Python tests, sanitized C tests, and fresh KiCad ERC, DRC and netlist comparison. The schematic was freshly exported with kicad-cli and rasterized with rsvg-convert for visual inspection. The publication scope is exclusively pole-bridge; no task branch was created. Commit author and committer are jayis1. Git's existing HTTPS credential helper is used for publication; an unauthenticated gh CLI is not treated as evidence that Git cannot push.

## Reproducible checks

From the repository root:

    python3 pole-bridge/tools/verify.py

Verified with Arm GNU GCC 14.2.1 (20241119), KiCad 9.0.2 and distribution Python 3.13.5:

- Cortex-M4 freestanding target build and binary generation passed with warnings treated as errors. ELF sizes: 3048 bytes text, zero initialized data, 788 bytes BSS.
- AddressSanitizer/UndefinedBehaviorSanitizer engine tests passed, including topology partitions, returns, sample loss, timeout, deadline ties, raw glitches, timestamp wrap, exhaustion and 512 mask paths.
- Sanitized clock/register and nonblocking UART tests passed, including bounded failure, busy rejection, wrap and timeout recovery.
- Thirteen Python tests passed. They cover the real C encoder, independent interval-partition oracle, sparse snapshots, invalid CRC-valid evidence, partial framing, replay bounds, serial attribute restoration, freshness and private non-overwriting exports.
- The real Tk event loop was exercised under Xvfb with a pseudo-terminal: all three screens, replay, connection, double-connect rejection, serial reception, CSV export, stale status, hangup and evidence retention passed. This is synthetic integration, not physical USB validation.
- KiCad schematic ERC reported zero violations. PCB DRC reported zero violations and zero unconnected items.
- Fresh schematic XML, PCB pads and connectivity manifest agreed exactly for 14 components, 30 pins and 10 nets; values and footprint library identities were also checked.
- The schematic was exported to SVG and visually inspected after rasterization. Connector and resistor labels, contact channels and author title block were legible; electrical connectivity is additionally checked by the exported netlist.
- README counted 3073 words. Production C sources counted 606 nonblank, noncomment lines, with another 84 header lines; test code is excluded from these counts.
- Every publishable file carries jayis1 attribution in its format-compatible metadata or text. Build products, Python caches and KiCad local preferences are excluded by the device ignore file.

## Limits and physical acceptance

No board was fabricated or programmed. No real switch, electrical threshold, timebase accuracy, watchdog recovery on silicon, USB behavior, protection rating, ESD, EMC or enclosure performance was physically tested. The purchased Nucleo is part of the design; the supplied custom board is only its passive contact carrier. Current manufacturer manual retrieval was not completed in the original run, so the reused pin map and solder-bridge assumptions require confirmation against the actual board revision before assembly.

The instrument is restricted to completely detached, voltage-free contacts. Both commons join the USB host ground, there is no galvanic isolation, and low wetting current may not operate oxidized power contacts reliably. Five-sample qualification cannot establish microsecond break-before-make behavior. Sparse snapshot checks are necessary consistency bounds, not reconstruction of every hidden transition. Physical bring-up and comparison with an independent logic analyzer remain required before practical measurement claims.
