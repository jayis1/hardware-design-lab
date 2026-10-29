# Ratchet Audit verification

Author: jayis1

## Scope and recovery

This revision completes the interrupted implementation of this device. Repository history and the originating task transcript established that the previously untracked directory belonged to this same device-generation task. No unrelated files were adopted or modified. The repository inventory and closest concepts were checked before publication; the distinction from Takeup Atlas and Cam Dwell Map is described in README.md. Worldwide novelty is not established.

## Executed checks

- Target: `make -C firmware all test` using arm-none-eabi-gcc 14.2.1, Cortex-M4 Thumb, freestanding linking, warnings treated as errors. ELF and binary produced. Final ELF size: text 2528 bytes, data 0 bytes, BSS 532 bytes.
- Three host C test executables passed with AddressSanitizer and UndefinedBehaviorSanitizer: engine/wire, clock/input configuration, and nonblocking serial driver.
- Engine tests include stroke completion, reversal/rollback, pawl qualification, independent contact qualification, illegal transitions, gaps, timeout, overflow, timer wrap, wire bounds, and 200000 deterministic acquisition iterations with invariant and serialization checks.
- `xvfb-run -a /usr/bin/python3 tests/test_app.py -v`: all 11 tests passed. These cover real Tk event-loop interaction with a simulated serial PTY, all three tabs, history rows, terminal restoration, hangup, C-encoder fixture parsing, malformed frames, impossible snapshots, stale/reset evidence, replay bounds and exclusive JSON export.
- Recovery initially reproduced failing regressions. Firmware and companion now reject rollback smaller than net reverse travel and an ACTIVE trial at its expiry deadline. The companion also rejects changes to retained trial evidence without a new start. Existing failing tests now pass.
- `/usr/bin/python3 tools/check_hardware.py`, KiCad 9.0.2: zero ERC violations, zero DRC violations, zero unconnected items. Fresh schematic netlist and actual PCB pads match the manifest: 14 components, 30 pins, 10 nets, with matching values and footprint identities.
- Exported the schematic using `kicad-cli sch export svg`, rendered with rsvg-convert, and visually inspected the labeled connections and component layout. Render is a temporary verification artifact, not a manufactured board.
- README contains 3207 whitespace-delimited words. Production firmware contains 613 C-source lines, including 561 nonblank lines after removing comments; C plus headers totals 724 lines, 663 nonblank noncomment lines. Tests are excluded from these source counts.
- All original deliverable files contain jayis1 attribution. Build products, Python caches and KiCad local preferences are ignored. No third-party authorship notices were rewritten.
- A separate read-only implementation review found no concrete blocking defect in the reviewed firmware/protocol/pin-map scope. This was not full collective review.

## Reproduction and limitations

Run the commands above from this device directory except the initial target command, which already names firmware. Generated binaries are deliberately not committed. The Python application uses the standard library and operating-system Tk support, not a downloaded package dependency tree. Its three functional screens are Connection / replay, Live evidence, and History / export.

Tests use synthetic inputs, including the serial PTY and C-generated fixture. They are not physical ratchet measurements. Neither target programming nor a real USB acquisition has been performed. Board fabrication, voltage margins, wetting current reliability, contact bounce, watchdog operation on silicon, oscillator accuracy, encoder loading, fixture repeatability, calibration and useful physical performance still require bench validation. The passive PCB requires the separately purchased NUCLEO-F401RE; it is not a standalone MCU board. This is a low-energy workshop prototype, not a safety or certification instrument.
