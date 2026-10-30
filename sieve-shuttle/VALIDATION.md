# Sieve Shuttle revision A validation

Author: jayis1

## Executed checks

Validation was performed locally on 2026-10-30. This is standalone-owner validation with an advisory read-only code review, not a full collective review or physical prototype test.

- GNU Arm Embedded GCC 14.2.1 (20241119): `make -C firmware all test` passed. The final target ELF has 2,904 bytes text, zero initialized data and 788 bytes BSS, excluding stack use. A linked binary was produced; `arm-none-eabi-nm -u` reported no unresolved symbols.
- Host C tests use AddressSanitizer and UndefinedBehaviorSanitizer plus `-Wall -Wextra -Werror`. The measurement test enumerated 4,096 six-zone words against an independent transition/run oracle, then exercised clearing boundaries, short rejected input, conflicting raw zones, capture timeout, sampling gap, transactional counter exhaustion, CRC, serializer capacity and timestamp wrap. Production clock and serial-driver tests also passed.
- Python 3.13.5: `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'` passed all 11 tests. Tests consume actual C-encoder fixtures, explicitly synthetic, and exercise strict decoding, broken framing, invalid accounting, session progression, stale data, replay rejection, private exclusive export and serial attribute restoration.
- The actual Tk application was exercised through a Linux pseudo-terminal: replay, rendered matrix and all retained history rows, export, connect, stale-summary clearing, duplicate-connect rejection, paced live input, staleness, hangup and post-disconnect export. This is not physical USB or reed-contact testing.
- KiCad CLI / pcbnew 9.0.2: schematic ERC reported zero violations; PCB DRC reported zero violations and zero unconnected items. Fresh schematic XML netlist comparison matched the PCB and manifest for all 14 component values and footprint identities, 30 reference/pin connections and 10 nets. The board has a closed outline and routed copper.
- Schematic SVG export succeeded and the rendered page was visually inspected for labels, symbol placement, title and attribution. The supplied symbols and passive network were visible without clipped labels. ERC on passive components does not validate the external Nucleo or its power source.
- `tools/check_quality.py` measured 3,162 README words and 647 nonblank, non-comment firmware C/header lines, excluding tests. Every project deliverable contains jayis1 attribution. The protocol has 37 fields in both encoder and companion.

## Review corrections

Read-only review produced three concrete companion validation findings. Each was reproduced with a failing regression before correction:

1. A single new committed visit now respects previously pending identity and minimum remaining clear-window time.
2. ARMED requires fully qualified inputs in both firmware and companion validators. Qualification bits cannot disappear between accepted snapshots.
3. Peak-run growth is bounded by the number of newly observed matching-axis traversals even across sparse snapshots with several visits.

All regression tests passed after the corrections. Larger sparse snapshot jumps still receive necessary aggregate bounds, not an exhaustive proof that every unobserved movement occurred. This limitation is documented rather than hidden by CRC validity.

## Reused source provenance

The low-level STM32 platform, passive electrical carrier generator/checker and transport framing/serial implementation derive from jayis1's maze-return at commit 5fd0821439c6e3fd5615900e7374021a3b961d0f. `tools/import_platform.py` and `tools/import_transport.py` identify the precise imports and refuse overwrite. The new measurement engine, directed-matrix protocol, model, interface and experiment documentation are specific to Sieve Shuttle. Standard installed KiCad footprints retain their library provenance and licensing; this project does not claim authorship of the upstream library.

## Not verified

No fabrication, physical programming, assembled current measurement, reed sensitivity selection, magnetic-zone calibration, electrical timing, material test, usability study, safety certification or regulatory assessment was performed. Device novelty beyond this repository is not proven. The engineering deliverable is complete as source and checked CAD, but the physical experiment still needs commissioning and independent outcome measurement. Do not use these synthetic results as a claim of sieving performance.
