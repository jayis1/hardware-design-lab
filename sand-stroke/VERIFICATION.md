# Verification record

Author: jayis1

## Executed checks

- `make -C sand-stroke/firmware clean all test`: passed. Arm GNU compiler 14.2.1 linked the Cortex-M4 target ELF and produced a binary. Reported image sections: 2448 bytes text, 0 bytes data, 788 bytes BSS. This is a target link, not merely a host syntax check.
- Sanitizer-enabled engine, clock and UART C test executables: passed. Engine cases cover both directions on both faces, mid-travel attachment, short returns, lift, face change at arrival, both-endpoint collision, exact-deadline rejection, sampling gaps, rejected glitches, independent qualification, timer wrap, counter exhaustion, invalid inputs, encoder refusal and deterministic adversarial input histories.
- `xvfb-run -a python3 -m unittest discover -s sand-stroke/tests -p 'test_*.py'`: 14 tests passed under Python 3.13.5. Tests use frames emitted by the real C encoder, actual Tk event loops and Linux pseudoterminals. They exercise all three tabs, serial setup/restoration, connection exclusivity, hangup, retained replay, strict replay input bounds, CRC and conservation rejection, freshness, drift rejection, rollback, private CSV creation and overwrite refusal.
- `python3 sand-stroke/tools/check_hardware.py`: passed with KiCad CLI 9.0.2. Schematic ERC: 0 violations. PCB DRC: 0 violations and 0 unconnected items. Fresh XML netlist, PCB pads and connectivity manifest agree on 14 components, 30 pins and 10 nets, including values and full footprint identities.
- Schematic SVG export rendered successfully and was visually inspected. References, values, net labels and author/title block are separated and readable; connections use labelled pin stubs. No physical assembly was inspected.
- README has 3103 whitespace-delimited words. Firmware C/header files contain 639 nonblank, noncomment lines, excluding tests and generated outputs. `tools/check_quality.py` repeats the checks and verifies required files and author attribution.

## Review corrections

Read-only software review identified missing cross-frame duration/count constraints and error-accounting gaps. Regression tests first reproduced the failures. The parser now requires duration growth to match new face completions and bounds combined face duration by available time. The GUI copies parser errors before session acceptance can fail and counts a partial buffered frame at disconnect. The resulting 14-test suite passes.

The fixed 750 ms accumulated host/device clock allowance remains intentional: it bounds falsely fresh backlog but can reject a genuinely connected uncalibrated HSI stream after sustained rate drift. A regression demonstrates this conservative failure, and the README documents the limitation. This is not a calibrated timebase claim.

## Not verified

No fabrication, soldering, MCU flashing, real USB serial capture, physical switch testing, watchdog expiry, oscillator calibration, mechanical fixture construction, sanding trials, force measurement, surface measurement, ingress protection, EMC/ESD qualification or regulatory certification was performed. The external Nucleo and unbuilt fixture remain explicit system components. Low contact wetting current and dust sensitivity require bench evaluation. Sensor failures can resemble valid contact states; this is not a safety system.

The small external search establishes adjacent manual practices, not global novelty. Costs are allowances rather than quotations. Original locally authored sources credit jayis1; reused platform sources are recorded by tools/import_platform.py and footprint-library licensing is preserved.

## Reproduction

From the repository root, run the firmware, Python and hardware commands above, then `python3 sand-stroke/tools/check_quality.py`. Build products and local CAD preferences are ignored. Normal builds are self-contained inside the device directory; import_platform.py is a provenance aid that refuses overwriting destinations and is not required for building. Raw replay fixtures are synthetic software output, never claimed to be physical measurements.
