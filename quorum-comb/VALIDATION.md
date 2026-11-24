# Quorum Comb validation record

Author: jayis1

## Scope and maturity

This recovery was executed by one owner using advisory reports, not a full collective or independent design review. The prior unfinished quorum-comb files were identified in the originating task transcript and recovered in place. No unrelated repository content was adopted. The repository is jayis1/hardware-design-lab; daily-ideas was not modified.

This is an unbuilt engineering prototype. No board was fabricated, physical target flashed, contact fixture assembled, magnetic field characterized, timing calibrated, protection tested or user study performed. Passing the checks below is evidence about source and CAD, not proof of physical performance or safety.

## Executed checks

Environment: arm-none-eabi-gcc 14.2.1 20241119, KiCad CLI 9.0.2, system Python 3.13.5, Linux with Tk and Xvfb.

- `make -C quorum-comb/firmware all test fixture`: PASS. ARM Cortex-M4 Thumb firmware links with warnings as errors. Final ELF text 2744 bytes, initialized data 0 bytes, static BSS 1044 bytes; stack and local engine/frame allocations are additional. `arm-none-eabi-readelf -h` confirms ELF32 ARM EABI executable.
- Engine host tests under address and undefined-behavior sanitizers: PASS. All sixteen stationary masks, 256 ordered stable transitions, independent voter/reference qualification, rejected glitches, gaps, timer wrap, invalid time/input, transactional exhaustion, CRC reference vector and serialization bounds.
- Production clock/configuration host tests: PASS. Masks, bounded failure paths and input configuration checks.
- Production UART host tests: PASS. Capacity, busy rejection, nonblocking drain, timeout, wrap and recovery.
- `python3 quorum-comb/tests/test_app.py`: PASS, nine tests. Real C encoder replay, corruption and partial tail rejection, frame resynchronization, semantic contradiction, rollback and conflicting duplicate rejection, credit rollback, freshness, serial attribute restoration/hangup, FIFO and empty replay rejection.
- `xvfb-run -a /usr/bin/python3 quorum-comb/tests/test_gui.py`: PASS. Real Tk event loop renders both screens, replays 160 synthetic snapshots, creates private CSV, refuses overwrite, clears old rows on reconnect, rejects double connection, accepts paced C-encoder PTY input, preserves disconnected provenance and detects PTY hangup.
- `/usr/bin/python3 quorum-comb/tools/generate_hardware.py`: generated fourteen components and ten nets with routed footprints.
- `/usr/bin/python3 quorum-comb/tools/check_hardware.py`: PASS. Native ERC zero violations; native DRC zero violations and zero unconnected items. Fresh schematic XML versus PCB pads versus manifest: fourteen references, thirty pins, ten nets; values and full footprint identifiers also match.
- Native schematic SVG/PDF export: PASS. PDF raster visually inspected: power/header and four resistor/contact rows are legible, labels are attached without obvious overlap, title block credits jayis1.
- README contains 2808 whitespace-delimited words. Production firmware contains 510 nonblank, noncomment C lines plus 73 header lines; tests are excluded from these counts.
- Authored deliverables contain jayis1. Local KiCad preferences, Python caches, binaries and build products are ignored rather than attributed or committed.

The initial engine test failed because engine.h, engine.c and wire.c were absent; the initial app test failed because model.py was absent. Implementation followed those failures. Host tests use synthetic inputs and pseudo-terminals; they do not validate a real UART wire or switch. Cumulative sparse telemetry checks cannot reconstruct all hidden input histories.

## Limitations and reproducibility notes

A fresh ST PDF download timed out. Manufacturer references and inherited pin mapping are documented in README; physical assembly requires checking the purchased MB1136 revision and its VCP configuration. A missing optional CairoSVG package was bypassed by using native KiCad PDF export and pdftoppm; no Python dependency was added for rendering. GitHub CLI is not authenticated on this host; publication uses the repository's configured Git HTTPS credential helper, without reading or exposing stored credentials.

Generated test fixtures are intentionally not committed. Rebuild them with the fixture target before running app tests. The CAD generator uses installed KiCad footprint libraries; use KiCad 9 and compare fresh netlist/PCB results after any regeneration. Do not run import_platform.py on this populated directory: its overwrite refusal protects recovered edits.

No task branch is needed. Publication is directly to the configured default branch, main, with jayis1 as commit author and committer. The owner verifies the final remote branch and clean checkout after pushing; the conversational completion report gives the final commit identifier.
