# Tilt Break verification record

Author: jayis1

## Executed checks

- Repository preflight: origin is jayis1/hardware-design-lab, configured remote default main, initially clean; required `git pull --ff-only` completed before implementation. Existing device directories and descriptions were inventoried. No daily-ideas files were touched.
- Actual Cortex-M4 target linked and converted to a binary with arm-none-eabi-gcc 14.2.1, freestanding C11, warnings treated as errors. Image sections: text 3656 bytes, initialized data 0 bytes, BSS 532 bytes. Startup vectors and linker script are included.
- Host C tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. They exercise zero/start, release qualification, transit, terminal freezing, rejected glitches, early arrival, acquisition gaps/failure, range faults, transit timeout, cancellation, speed, reversal, timestamp wrap, short encoding buffers and the standard CRC check vector.
- Ten Python application tests passed under Python 3.13.5 and Xvfb. Coverage includes the real C encoder fixture, CRC/truncated frames, valid-CRC impossible fields, duplicate freshness, rollback/mutation rejection, safe replay and FIFO rejection, private no-overwrite export, empty-session metadata, failed connection preservation, real Tk tabs, pseudoterminal receive/hangup, late-attachment fragments, double-connect refusal and restoration of terminal attributes.
- KiCad 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items.
- Fresh schematic XML netlist, actual PCB pads and design manifest agree: fourteen components, thirty pins, ten nets. Values and footprint identities are compared, not only net names. An exported XML netlist is also retained as kicad/device.net.
- Schematic exported to SVG, rendered to PNG and visually inspected: four separate channels, readable labels, 100k pullups, 1k series resistors, ground connections and jayis1 attribution present.
- README: 3162 whitespace-delimited words. Firmware C/header files: 743 total lines, including 683 nonblank lines after stripping comments; tests excluded from these counts.
- Attribution gate requires jayis1 in every delivered file. Build products, Python caches and KiCad local preferences are ignored.

Reproduce the combined gates with `python3 tools/verify.py` from this device directory. Individual commands are documented in README.md. The compiler and KiCad versions above identify the executed toolchain; other versions have not been tested.

## Review corrections

A read-only code review identified two serial lifecycle issues. The owner added a regression test, observed its failure and corrected late-attachment handling and double-connect ownership. The regression and complete application suite then passed. All repository writes were performed by the owner; the advisory review is not claimed as full collective hardware review.

## Not verified

No physical hardware was available. Fabrication, flashing, real UART timing, oscillator accuracy, ADC performance, electrostatic-discharge immunity, switch wetting reliability, geometry calibration, magnetic interaction, mechanical fit and practical comparison utility remain unverified. The passive PCB does not contain the Nucleo processor or regulator; ERC/DRC cannot validate their external wiring or the mechanical assembly.

Bourns' PTA datasheet was retrieved and inspected. The ST manual download failed in this run, so the inherited board mapping still requires verification against the purchased Nucleo revision. Supplier stock, quotations and product certification were not checked. The external prior-art search established that commercial incline testers exist, not worldwide novelty for this workflow.

This record reports build and file validation only. Neither synthetic fixtures nor nominal derived angles are physical measurements. This is not a fabrication release or a calibrated friction-testing instrument.
