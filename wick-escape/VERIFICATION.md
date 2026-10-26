# Wick Escape revision A verification

Author: jayis1

## Executed checks

The reproducible entry point is `python3 tools/verify.py`, run from this device directory or by absolute path. Results below are from actual local execution, not physical hardware measurements.

- README: 3162 whitespace-separated words.
- Production firmware: 522 nonblank, noncomment C source lines, plus 103 header lines. Tests are excluded. The count includes ordinary declarations and control-flow braces, not just executable statements.
- Target: GNU Arm Embedded GCC 14.2.1, `-mcpu=cortex-m4 -mthumb`, C11, `-Wall -Wextra -Werror`, freestanding, no standard library. ELF and BIN produced successfully for STM32F401RET6.
- Linked image: 2428 bytes text, zero initialized data, 276 bytes static BSS; stack-resident Engine/frame buffers are additional. The linker reserves a 4096-byte stack margin.
- Host compiler: GCC 14.2.0. AddressSanitizer and UndefinedBehaviorSanitizer engine tests passed with no reported findings.
- C tests exercise dry readiness, ordered arrivals, independent qualification, rejected short excursions, simultaneous ambiguity, early and late bypass, source loss, inclusive expiry, missing acquisition, invalid ADC data, 32-bit clock wrap, bounded encoding, invalid success summaries, and the CRC reference vector.
- Python 3.13.5: eleven parser/model/replay tests passed. They include the real C encoder's synthetic fixture, CRC corruption, partial-frame recovery, incomplete EOF, impossible state fields, terminal mutation, ADC-validity marking, duplicate/rollback handling, stale reception, private export and FIFO rejection.
- Three Tk GUI tests passed under Xvfb with actual Linux pseudoterminals. Tests operate the Connect button and all four tabs, receive a synthetic serial frame, verify displayed evidence, invoke the export dialog path, verify file mode 0600 and JSON contents, detect PTY hangup, preserve evidence, replace replay with a fresh live session, and restore terminal settings after injected setup failure.
- KiCad 9.0.2 schematic ERC: zero violations.
- KiCad 9.0.2 PCB DRC: zero violations, zero unconnected items.
- Fresh schematic XML netlist, manifest and PCB comparison: fourteen component references, thirty pin assignments and ten nets match exactly. Values and full footprint library identifiers match as well.
- Schematic SVG exported using KiCad and rendered through rsvg-convert. Visual inspection found separated component rows, readable signal labels and no apparent label/pin overlap.
- Every delivered source/document/CAD text file includes jayis1 attribution. Build products, synthetic raw fixtures, Python caches and KiCad local preferences are ignored rather than published.

## Review corrections

A read-only implementation review found four issues. Qualification now treats intervals above 25 ms as a terminal gap, so an entire missing 20 ms sample cannot complete a pending streak. ADC failures now suppress displayed bars and expose `adc_valid: false` rather than presenting retained counts as current measurements. Both encoders and parsers reject success with flank evidence. The app follows the actual terminal transition graph and freezes terminal timing evidence. Regression tests were observed failing before those corrections and passed afterward.

Late bypass after a completed observation is intentionally untimed: it invalidates the result without inserting a fabricated arrival into the frozen trial timebase. An expired trial cannot become a completed one. These are interpretation constraints, not proofs of physical liquid paths.

## Reproduction

1. Install GNU Arm Embedded GCC, host GCC, Python with Tk, Xvfb, KiCad 9 and its system Python bindings.
2. Run `python3 tools/verify.py`.
3. Optionally run `/usr/bin/python3 tools/generate_hardware.py` and repeat verification to check regenerated CAD. This regenerates authored hardware files; do not run it over unrelated edits.
4. Run `python3 app/wick.py --replay firmware/build/fixture.txt` to validate the generated synthetic capture without opening a GUI.
5. Run `python3 app/wick.py` for the desktop application. Connect only after checking the actual carrier/Nucleo wiring.

## Not verified and release boundaries

No fabricated carrier, flashed MCU, real ADC voltage, graphite electrode, wet coupon, clock tolerance, watchdog reset timing, ESD event or EMC compliance test was available. Real USB hardware was not connected: PTY tests simulate transport. The ADC driver was target-compiled, not exercised against silicon. The schematic contains the passive carrier only; the purchased Nucleo supplies the MCU and power circuitry.

Bench release requires resistor-substitution tests on all four channels, measurement of bias current and ADC settling, continuous frame reception on a physical ST-LINK connection, reset/watchdog checks, and controlled coupon comparisons against video. Verify that electrode contact pressure and grounding do not create a preferential liquid path. Until then this is a software/CAD-verified design proposal, not an experimentally validated barrier assay or production instrument.
