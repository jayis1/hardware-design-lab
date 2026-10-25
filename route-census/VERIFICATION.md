# Route Census verification record

Author: jayis1

## Scope and status

Revision A is a build-tested engineering prototype, not fabricated or physically calibrated hardware. The target is a separate NUCLEO-F401RE with the supplied passive photogate interface. All new repository work is scoped to `route-census/`. Existing device purposes were inventoried before selection; the closest comparison is Crossover Track, which measures paired cart/lane effects rather than destination allocation.

## Commands executed and results

- `make -C route-census/firmware clean all test`: passed. ARM ELF and binary linked with GNU Arm GCC 14.2.1, Cortex-M4/thumb, freestanding, warnings as errors. Final image: 3512 bytes text, zero initialized data, 1572 bytes BSS. Linker includes a stack reserve assertion.
- Host engine suite under address and undefined-behavior sanitizers: passed. Covers route results, overlapping inlet, simultaneous inlet/outlet, simultaneous and staggered multiple outlets, orphan activity, timeout, acquisition gaps, short pulses, independent qualification, timer wrap, gap/orphan counter exhaustion, encoder CRC, capacity rejection, worst-width serialization and 200000 deterministic noisy-input steps with conservation checks.
- Production UART driver under mocked memory-mapped registers, address and undefined-behavior sanitizers: passed. Covers null/empty/oversize rejection, busy submission, byte order, backpressure, timeout, wrap and recovery. This executes the actual driver source, not a second UART implementation.
- `xvfb-run -a /usr/bin/python3 route-census/tests/test_app.py`: nine tests passed with Python 3.13.5. Includes C-encoder roundtrip, CRC-valid invalid conservation, partial-frame resynchronization, duplicate freshness, counter rollback, incomplete replay EOF, FIFO rejection, real Linux PTY hangup and a real Tk event-loop connection/export/disconnect workflow.
- `.../app/census.py --check .../firmware/build/fixture.txt`: passed with one synthetic accepted left-route observation at 40 ms, zero parser errors and no frozen state. This is generated test data, not a hardware measurement.
- `.../tools/check_hardware.py`: passed with KiCad 9.0.2. Schematic ERC: zero violations. PCB DRC: zero violations, zero unconnected items. Fresh XML netlist/PCB/manifest comparison: 14 components, 30 pins, 10 nets, with exact value and footprint identity checks.
- Schematic SVG export: succeeded; rendered view inspected for connected labeled pins, legibility and title/author. The sparse labeled-net layout is intentional. A fresh KiCad S-expression netlist is included as `kicad/device.net`.
- README count: 3430 whitespace-separated words. Production firmware C/header count: 712 nonblank lines after removing comments, excluding tests and build output. These counts are acceptance checks, not substitutes for functional verification.
- All deliverable files were checked for `jayis1` attribution. No alternate creator attribution or credential assignment patterns were found. Build products, Python caches and KiCad local preferences are ignored.

## Review corrections

Read-only implementation review found that an immediately accepted first outlet could hide a later conflicting outlet. The implementation now retains a provisional SETTLE result and rejects additional qualified activity until the all-clear confirmation finishes. Failing regressions were added before the correction. Review also found counter-exhaustion state loss and incomplete replay tails being silently accepted; both were reproduced and corrected. A subsequent read-only source review passed with no concrete blockers for the documented scope.

## Reproduction and remaining work

The README lists dependencies and commands. Run firmware tests before application tests because they produce the synthetic raw fixture. Hardware regeneration requires system KiCad libraries; the delivered schematic embeds its authored symbols and the board contains footprints and routes. No generated ELF, binary, test executable or fixture is committed.

Unperformed: target programming, fabricated-board inspection, real sensor installation, supply-current measurement, GPIO voltage margins, HSI/optical timing calibration, environmental and ESD tests, enclosure fit, physical fault injection and user evaluation. The `flash` target is provided but was not executed. A passive ERC pass does not inspect external sensor power or the purchased MCU board. Unknown track occupancy, sensor disconnects that appear clear, overlapping balls outside the confirmation window and undetectable reboot patterns remain documented limitations.

The instrument is not a safety controller and has no actuator outputs. The tested software evidence should not be presented as physical performance data or production certification.
