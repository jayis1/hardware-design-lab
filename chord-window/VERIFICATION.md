# Chord Window verification

Author: jayis1

## Executed checks

- `make -C chord-window/firmware clean all test`: successful ARM target compile/link and binary generation with `-Wall -Wextra -Werror`. Firmware .c sources contain 515 nonblank, non-comment lines, excluding headers and tests. README has 3265 whitespace-delimited words at verification.
- Target `arm-none-eabi-gcc` 14.2.1; ELF reports 2144 bytes text, zero initialized data, 276 bytes static BSS. Automatic state/frame variables and call stack additionally use SRAM; linker reserves at least 4096 bytes beyond static storage. `arm-none-eabi-nm -u` returned no undefined symbols.
- Host GCC 14.2.0 with AddressSanitizer and UndefinedBehaviorSanitizer passed engine and clock test executables. Cases cover complete chords, retaps, invalid-result zeroing, acquisition gaps, timeout and release exactly at deadline, rolling gestures without shared hold, independent-channel chatter, short rejected pulses, uptime wrap, exhausted identity, CRC and small output capacity. Clock tests simulate register masks and timeout paths, not physical oscillator behavior.
- `xvfb-run -a python3 chord-window/tests/test_app.py`: 11 tests passed with Python 3.13.5. Includes real C-encoder complete/retap/gap/timeout/rolling fixtures, CRC/field validation, truncation resynchronization, EOF rejection, duplicate freshness, contradictory evidence, reset inference, unsigned wrap, missed IDs, bounded history, CSV overwrite refusal, session metadata without trials, FIFO rejection, empty recording rejection, valid idle recording acceptance, and buffered hangup draining.
- The GUI test ran the actual Tk event loop against a Linux pseudo-terminal, received a trial, exercised all four tabs, checked policy comparison and table contents, then closed the peer and verified disconnect while preserving live provenance. This is simulated serial transport, not a real Nucleo or USB bridge.
- CLI validation of the real C-generated complete-trial fixture returned `author=jayis1 rows=1 errors=0 blocked=False`.
- KiCad 9.0.2 generated native schematic, PCB, project, symbol library and connectivity manifest. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML export matched all fourteen components, thirty pins and ten nets with PCB pad assignments, component values and footprint identifiers.
- Schematic was exported to SVG, rendered to PNG and visually inspected: header and resistor symbols, four labeled channels and author/title block were present and legible. Render is a scratch inspection artifact, not a committed deliverable. A schematic netlist is included in `kicad/device.net`.
- Electrical arithmetic independently recomputed: 0.30 mA per closed channel, 1.20 mA for four, approximately 3.96 mW total resistor-network draw and 0.30 V MCU input low. These are ideal calculations, not measured results.

## Review resolutions

Read-only precommit review found buffered-hangup tail loss, empty CSV metadata loss and acceptance of empty recordings. Regression tests reproduced the failures before fixes. Serial reads now drain with a bounded per-GUI-turn budget before final closure. CSV always starts with a `session` record containing provenance/diagnostics and follows with `trial` records. Replay requires at least one accepted snapshot but does not require a completed trial. Additional valid-CRC contradiction, rollback, wrap, retained-history and invalid-result fixtures were added.

## Scope and limitations

Only Chord Window and its root index entry are part of this change. All authored source and design files credit jayis1. No cloud services, credentials, radio stacks, automatic dependency installers or GitHub workflows were added. Board support and the passive-interface generator are adapted from existing jayis1 repository sources; the chord engine, contract and desktop interface are new.

No physical assembly, board programming, contact-force study, electrical timing measurement, ESD test, certification or ergonomic validation was performed. The direct ST manual retrieval timed out; the documented Nucleo connector map follows the repository's existing carrier and requires confirmation against the purchased board revision. The carrier is passive and the MCU/power/USB circuitry resides on an external Nucleo. Local CAD and build success do not establish a fabrication approval or global novelty. CSV provenance describes capture mode, not authenticated hardware identity.
