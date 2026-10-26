# Verification record

Author: jayis1

Status: software-build-tested engineering prototype; no physical hardware validation.

## Reproducible checks

Command: `python3 tools/verify.py` from the device folder.

- README: 2,988 whitespace-delimited words.
- Production C: 584 nonblank, non-comment lines in .c files; 117 in headers, excluding tests.
- Target: STM32F401RET6, ARM GCC 14.2.1, Cortex-M4/Thumb, freestanding C11 with warnings treated as errors.
- Linked image: 2,816 bytes text, zero initialized data, 532 bytes static BSS. The linker reserves 4 KiB of stack margin; the engine and frame buffers also use stack at runtime.
- Host engine tests: passed with address and undefined-behavior sanitizers. Covered independent lanes, baseline, load/recovery milestones, short hold, maximum hold, ambiguous release spanning a milestone, reload, sampling gaps, ADC faults, uint32 timestamp wrap, CRC reference vector, output capacity and final-result retention.
- Python 3.13.5 application: nine tests passed, including actual Tk event loops under Xvfb and Linux serial pseudoterminals. Covered real C-encoder fixture ingestion, malformed and contradictory records, baseline immutability, failed-connect evidence retention, unplug, all three tabs, private CSV creation, overwrite refusal, empty-session metadata, replay FIFO rejection, truncated replay, stale reception and backlog checks.
- KiCad 9.0.2 schematic ERC: zero violations.
- KiCad PCB DRC: zero violations and zero unconnected items.
- Fresh exported schematic netlist versus actual PCB pads and manifest: 14 components, 30 pins, 10 nets matched, including values and footprint identities.
- Schematic exported to SVG, rasterized and visually inspected: four readable resistor/connector channels, supply and ADC headers, connected named nets and jayis1 title-block credit.
- Per-file author credit and basic secret-pattern checks passed. Build products and KiCad local preferences are ignored.

## Review corrections

Read-only review identified a release-qualification ambiguity at a loaded milestone, a maximum-hold boundary ordering error, acceptance of changed established baselines in the app and loss of evidence on failed connection attempts. Regression tests reproduced those defects before fixes; the final test suite passes. No reviewer wrote repository files.

## Not verified

No fabrication, flashing, real USB session, actual ADC behavior, watchdog reset, timing accuracy, ESD testing, mechanical friction characterization, sensor linearity calibration, coupon loading or repeatability trial was performed. Pseudoterminals simulate transport, not MCU execution. Fixed input arrays simulate observations, not foam behavior. The chosen slide sensor has substantial friction; its reduced-motion linkage and follower preload require measured qualification before material comparisons are meaningful. Native CAD checks do not establish manufacturing readiness of the complete fixture.

This is an original implementation and differentiated bench concept, not a global novelty determination, patent clearance or ASTM-compliant instrument.
