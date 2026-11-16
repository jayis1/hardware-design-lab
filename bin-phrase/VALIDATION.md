# Bin Phrase validation record

Author: jayis1

Status: software/CAD-checked engineering prototype; not physically validated.
Execution mode: standalone owner verification. Advisory reports were not a full independent implementation review.

## Executed checks

- ARM GNU GCC 14.2.1 linked the real Cortex-M4 firmware with -Wall -Wextra -Werror, freestanding startup and linker script. Image size: text 2,416 bytes, data 0 bytes, static BSS 788 bytes; stack is additional SRAM usage.
- Host engine tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. They enumerate 2,401 four-group words over the seven nonempty opening masks against an independent array expectation, preserving repeated groups and ties. Additional tests exercise held inputs, rejected short pulses, tied docking/opening, tied removal/opening, open-lid removal, capacity exhaustion, deadline-minus-one versus deadline, sample gap, counter exhaustion, time wrap, bounded encoding and the standard CRC check vector.
- Clock/input tests and production UART tests passed. UART coverage includes busy rejection, bounded buffering, nonblocking drain, timeout recovery and modular time wrap. These are host register simulations, not oscilloscope measurements.
- Python 3.13.5 application tests cover real C-encoder fixture decoding, checksum and truncation handling, invariant rejection including a valid-CRC impossible record, rollback latching, frozen-result mutation, FIFO rejection, PTY input, private CSV export, overwrite refusal, live pause blocking, staleness and duplicate nonrefresh.
- An actual Tk event loop under Xvfb exercised replay, displayed the retained A / tied B+C / A word, verified all received rows render, preserved replay on failed /dev/null connection, cleared results on a new connection, rejected duplicate connection, consumed synthetic C-encoder records through a Linux PTY, exported CSV and detected PTY hangup. No physical USB device participated.
- KiCad 9.0.2 fresh schematic ERC: zero violations. Fresh PCB DRC: zero violations, zero unconnected items.
- A fresh schematic XML netlist was compared against the loaded PCB and connectivity manifest: fourteen component references, thirty pins and ten nets matched, including values and full footprint identities.
- Schematic SVG was exported and rendered using rsvg-convert, then visually inspected. Component labels, contact net names, input header and jayis1 title-block credit were legible and did not overlap.
- README contains 2,900 whitespace-separated words. Firmware C/header sources contain 583 nonblank, noncomment lines, excluding tests and generated build outputs.
- Every deliverable source/design/document file contains jayis1 attribution. Build output, Python caches and KiCad local preferences are ignored. No symlinks are delivered.

## Reproduce

From repository root:

    make -C bin-phrase/firmware clean all test fixture
    python3 -m compileall -q bin-phrase/app
    xvfb-run -a python3 -m unittest discover -s bin-phrase/tests -p 'test_*.py'
    /usr/bin/python3 bin-phrase/tools/check_hardware.py

To regenerate native CAD before checking it:

    /usr/bin/python3 bin-phrase/tools/generate_hardware.py

The one-time import script intentionally refuses existing destinations. It is provenance tooling, not a required build step. The full package is standalone after checkout; it does not import sibling Python modules or link sibling firmware at build/runtime.

## Unverified and bounded claims

No PCB was fabricated, development board flashed, contact voltage measured, fixture assembled or user study conducted. EMC, cable ESD, USB power demand, mechanical repeatability, input timing and real utility remain unverified. Passive-symbol ERC cannot validate the external Nucleo supply or switch selection. Procurement must confirm low-level switch suitability. The instrument cannot establish object identity, quantity, transfer, correct contents or physical simultaneity.

Periodic snapshots are lossy with respect to intermediate completed sessions; they retain only the latest result. Host transition checks reject important impossible endpoints but do not reconstruct or prove hidden history. A reset without an identifiable counter/time regression can evade reset inference. There is no authenticated transport, persistent boot ID, nonvolatile history or safety function. Keep these limitations with any exported demonstration evidence.
