# Insert Code revision A validation

Author: jayis1

Status: software-build and CAD-check verified; unbuilt physical prototype. Results below are real executions against this directory, not simulated tool reports. The acquisition fixture is synthetic, generated through the production C encoder.

## Executed checks

- `make -C insert-code/firmware clean all test fixture`: passed. GNU Arm Embedded GCC 14.2.1 linked a freestanding ARM ELF and produced a binary. Final target size: 2832 bytes text, zero initialized data, 1044 bytes BSS. Stack is additional to reported BSS. `arm-none-eabi-readelf -h` confirmed ARM, ELF32, little endian and an entry point in target flash.
- Engine tests under AddressSanitizer and UndefinedBehaviorSanitizer passed. They cross all six captured identities with all sixteen later words; exercise startup while attached, exact qualification boundary, initially invalid identity, short empty glitches, sticky substitution, gap abandonment, rearming, terminal immutability, transactional overflow, clock wrap, contradictory qualified words and bounded serialization.
- Production clock-driver tests passed bounded wait failures, masks and configuration checks. Production UART tests passed busy rejection, buffer limits, nonblocking drain, wrap and timeout recovery. Host register models are not physical peripheral verification.
- `python3 insert-code/tests/test_app.py`: eight tests passed. Includes all ordered sparse pairs from the 100-record real-encoder fixture, framing one byte at a time, truncated tail, resynchronization, CRC-valid impossible records, missing required taints, identity migration, stale duplicates, clock divergence, FIFO rejection and empty replay.
- `xvfb-run -a /usr/bin/python3 insert-code/tests/test_gui.py`: three tests passed. Real Tk event loops exercised replay, all history rows, six identity rows, private CSV content/mode, overwrite refusal, failed serial construction preserving evidence, PTY live reception, double-connect rejection, termios restoration, hangup, and rendering accepted partial-batch evidence before reporting a later invalid row.
- `/usr/bin/python3 insert-code/tools/check_hardware.py`: KiCad 9.0.2 ERC zero violations; DRC zero violations and zero unconnected items. Fresh schematic XML and actual PCB pads match the manifest: fourteen components, thirty pins, ten nets. Values and complete footprint identities also match.
- Native schematic exported as SVG, converted using rsvg-convert, and visually inspected: separate labeled rows, readable connector/pin labels and author/title block. The files are not an empty board or a decorative block diagram.
- README: 2753 whitespace-separated words. Firmware C/header sources: 635 nonblank lines after removing block comments, excluding tests. The source includes executable acquisition, qualification, episode accounting, fault handling, serialization, startup, clock, GPIO, UART and watchdog logic.

## Review corrections

Read-only review identified two issues, both reproduced with failing regressions before correction. Active snapshots could omit the taint required by their qualified code; both firmware validation and companion parsing now reject them. The GUI could retain a newly accepted row without displaying it when a later row in the same batch failed; the error path now renders the retained evidence before disconnecting. Tests were rerun after both fixes. Review was advisory; publication and verification remain the owner's responsibility.

## Unverified and intentionally absent

No board was fabricated, target flashed, fixture built, switch force measured, code carrier calibrated, USB timing measured or operator study performed. ERC uses passive symbols and cannot verify the external development board's supply. DRC and connectivity equality do not establish electrical protection, mechanical fit, useful insertion speed, component procurement availability or manufacturing certification. There is no production enclosure CAD, authenticated identity, permanent MCU log, independent seated sensor or safety output.

A fresh ST UM1724 PDF download failed over HTTP/2 and timed out on HTTP/1.1; the pin map and register implementation are inherited with explicit source provenance, not represented as newly checked manufacturer documentation. Verify the actual board manual/revision before physical connection. Public search confirms established coded-tool prior art; global novelty and patent clearance are not claimed.

Build products, synthetic recordings, local KiCad preferences and Python caches are deliberately ignored. Rebuild them with the documented commands rather than treating stale files as evidence. The import script is provenance-only, refuses overwrite, and is not a replacement for the checked-in final implementation.
