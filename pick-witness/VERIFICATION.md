# Pick Witness verification record

Author: jayis1

## Executed checks

- ARM GCC 14.2.1: `make -C firmware all test` compiled and linked the freestanding STM32F401RE firmware with `-Wall -Wextra -Werror`, produced an ELF and binary, and exercised the native engine/encoder with GCC 14.2.0 address/undefined-behavior sanitizers. Final linked image: 2372 bytes text, zero initialized data, 276 bytes static BSS; the Engine and record buffer are on the stack and are not included in BSS.
- Native engine regression coverage: valid alternating picks, repeated shed, beat without pass, both docks, recovery, missed acquisition, bounce, timestamp wrap, pending timeout, flight timeout, mid-flight shed change, simultaneous departure/shed change, simultaneous arrival/shed change, CRC known answer, and encoder capacity rejection.
- `/usr/bin/python3 tests/test_app.py`: four test methods passed against the actual C-encoder synthetic fixture. Coverage includes two accepted sequences and one rejected beat, corrupted/truncated framing recovery, bounded malformed input, future protocol/range rejection, suspected reset blocking, provenance-preserving CSV and overwrite refusal.
- `xvfb-run -a /usr/bin/python3 tests/test_gui.py`: real Tk 8.6 event loop, all three views, Linux PTY receive, serial batch history preservation, 1.5-second freshness timeout, private mode-0600 CSV creation, disconnect and replay passed. Replay history row count matches retained model rows. This is not a physical USB test.
- KiCad 9.0.2: `tools/generate_hardware.py` generated the schematic/project/PCB. `tools/check_hardware.py` ran schematic ERC and PCB DRC with failure-on-violations: zero ERC violations, zero DRC violations and zero unconnected items. Exported XML netlist, PCB and manifest agree on 14 components, 30 pins and 10 nets, including values and full footprint identifiers.
- Schematic SVG export was converted to PNG and visually inspected: four separated pull-up/series channels, labelled connectors, readable title and no apparent clipped elements. Rendering is not physical inspection.
- Python 3.13.5 standard-library application, no external pip packages. `tools/check_release.py` checks source authorship, minimum README length, substantive C/header count and declared MCU mappings. The README contains 3178 whitespace-separated words. Production firmware contains 495 nonblank/comment-stripped C-source lines plus 101 header lines, 596 combined; tests and generated build products are excluded.

## Review corrections and test discipline

Tests were written and observed failing before engine/app implementation. A read-only code review then identified acceptance of a shed change during shuttle travel and omission of intermediate records in the visible history when several frames arrived together. New regressions reproduced both failures. The engine now records departure shed, rejects changes during flight, rejects ambiguous simultaneous changes and bounds flight duration. The UI rebuilds its bounded history from retained records. All corresponding tests passed after correction.

## Provenance and limitations

Platform drivers, startup, linker and passive-carrier generation/checking are adapted or reused from jayis1's ContactAtlas files within this repository. The new loom engine, wire protocol, Python workbench and tests are attributed to jayis1. KiCad footprint library identifiers refer to installed standard libraries; upstream component identities are not claims of newly invented components.

The manufacturer UM1724 endpoint failed; the manual was retrieved from its Harvey Mudd College mirror and its USART2/ST-LINK bridge description checked. The inspected inventory and limited loom search establish a differentiated repository concept, not worldwide novelty.

No board was fabricated, powered, flashed or attached to a loom. Flashing, watchdog behavior on silicon, actual UART timing, reed sensitivity, mechanical clearances, electronic noise immunity, current consumption and user value remain unverified. Firmware tests use synthetic contact vectors; app integration uses generated frames and a PTY. CAD checks validate the passive interface, not the external Nucleo electronics or loom mechanics. This is a reference prototype, not a certified product, automatic fabric inspector or safety interlock.

## Reproduction

From the device directory:

    make -C firmware clean all test
    /usr/bin/python3 tests/test_app.py
    xvfb-run -a /usr/bin/python3 tests/test_gui.py
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py

Generate CAD only when intentionally changing it with `/usr/bin/python3 tools/generate_hardware.py`; regeneration can change PCB serialization even when connectivity is unchanged. Build products, local KiCad preferences and Python caches are ignored, not publication artifacts. Git publication and remote-branch verification are reported separately by the task owner rather than claiming a self-referential commit identifier in this file.
