# Texture Cycle validation record

Author: jayis1

## Scope

Build-verified, unbuilt Revision A design. Standalone-owner execution with advisory review, not full collective approval. Existing shared jayis1 acquisition, transport and passive-carrier infrastructure was imported from shim-bracket at 91e5062; the new preference graph policy, companion graph display, tests and documentation belong to this device. No daily-ideas repository content or unrelated device files were changed.

## Reproduce

From repository root:

    python3 texture-cycle/tools/verify.py

The command deliberately does not flash hardware, commit or push. All generated ELF, binary, test executables, recordings and Python caches are ignored. Hardware reports use a temporary directory under the configured scratch location.

## Executed checks

- ARM GCC 14.2.1: freestanding Cortex-M4 target compiled and linked with -Wall -Wextra -Werror. ELF sections: text 2584 bytes, data 0 bytes, BSS 788 bytes. The linker enforces a 4096-byte stack margin; stack high-water behavior has not been measured on hardware.
- GCC host tests under AddressSanitizer and UndefinedBehaviorSanitizer: all 4096 directed irreflexive four-node graphs compared with an independent bounded-walk cycle oracle. Includes repeated edges, orphan and self preferences, replaced references, consumed references, selector glitches, simultaneous and sequential mixed paddles, idle and active acquisition gaps, exact timeout precedence, timestamp wrap, invalid encoding and counter exhaustion.
- Shared clock tests: reset-clock masks, bounded failure paths and input configuration.
- Shared UART tests: bounds, busy rejection, nonblocking draining, timestamp wrap and timeout recovery.
- Python 3.13.5: compilation of application, tests and tools; eight model/transport tests plus one real Tk event-loop test under Xvfb. The GUI test visits all four pages, replays the actual C encoder's synthetic triangle, checks all three rendered arrows, connects through a Linux PTY, checks reconnect clearing and double-connect rejection, receives paced serial records, handles hangup, preserves historical evidence, rejects /dev/null and exports a private file.
- Parser and model checks cover malformed checksums, truncated streams, bounded replay, FIFO rejection, semantic accounting, disappearing/unfunded edges, reference reuse, wrong captured-identity completion, stale traffic and private overwrite-refusing exports.
- KiCad 9.0.2: zero schematic ERC violations, zero PCB DRC violations, zero unconnected items. Fresh schematic XML netlist agrees with actual PCB values, library footprint identities and all 30 pins across 14 components and 10 nets.
- Schematic SVG export succeeded. Rendered with rsvg-convert and visually inspected: readable labels, separated component rows, complete reference/pin annotations and jayis1 title-block attribution.
- README: 3324 whitespace-delimited words. Firmware C/header sources: 619 nonblank, noncomment lines, excluding tests. All authored source deliverables contain jayis1 attribution; imported footprint provenance is retained in LICENSE.

## Review findings resolved

A sequential raw REFERENCE-to-PREFER handoff could previously avoid the simultaneous-button guard before debounce. A regression first failed, then passed after adding raw paddle-envelope membership tracking. Sparse telemetry could also consume a reference while falsely retaining it; the companion now reserves reference credit for the final pending reference as well as new preferences, and its regression first failed and then passed.

A proposed five-millisecond minimum between every active and completed snapshot was not imposed: raw release qualification can already be underway while the reported qualified paddle remains high. The snapshot protocol does not expose raw qualification progress. Necessary bounds and their limitations are documented rather than claiming full reconstruction of hidden electrical histories.

## Not verified

No board was fabricated or flashed. No electrical voltages, physical switch timing, oscillator accuracy, watchdog timeout, EMC/ESD response, USB current, mechanical clearances, fabric contact, participant behavior or usability outcome were measured. No sensory, medical, industrial or safety certification is asserted. The dry-contact PCB does not provide galvanic isolation. Mechanical dimensions and BOM prices are prototype specifications and estimates. The selector has no independent detent-valid signal. Worldwide novelty and freedom to operate are not established.
