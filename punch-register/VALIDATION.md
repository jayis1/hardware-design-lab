# Punch Register validation record

Author: jayis1

## Executed checks

- Target firmware: clean compile and link with arm-none-eabi-gcc 14.2.1, Cortex-M4, freestanding C11, -Wall -Wextra -Werror. Final ELF: 2,832 text bytes, zero initialized data, 788 BSS bytes. Binary conversion passed. Stack is reserved separately by the linker assertion.
- Host firmware: three executables under AddressSanitizer and UndefinedBehaviorSanitizer passed. Engine cases include correct stroke, early departure, raw registration loss, missing/repeated bottom, endpoint conflict, timeout, sampling gap, empty-baseline recovery, timer wrap, counter exhaustion, exact 99/100 ms seating and rearm boundaries, raw recovery without back-credit, gap-abandon duration, and all 256 pairs of settled four-bit input vectors. Board-clock and real UART-driver tests passed.
- Companion: 11 Python unittest methods passed under Xvfb using Python 3.13.5. Tests consume frames from the actual C encoder, exercise fragmented input, CRC rejection, truncated-frame resynchronization, incomplete tails, impossible accounting, rollback, duplicates, freshness and backlog, replay bounds/FIFO refusal, private export and overwrite refusal, serial PTY reads/restoration, injected serial-setup failure, all three GUI tabs, GUI serial hangup, replay replacement and export. These are simulated transport tests, not physical USB tests.
- KiCad 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. Exported XML netlist, PCB pads and connectivity manifest agree on 14 component references, 30 pins and 10 nets, including values and full footprint identifiers.
- Schematic SVG export succeeded. Its rendered image was visually inspected for separated labels, components and attribution. The available rsvg-convert renderer was used after the optional CairoSVG Python module proved unavailable.
- README: 3,443 whitespace-separated words. Production firmware C/header count: 680 nonblank lines after removing block and line comments, excluding tests and build outputs. This count includes declarations and register definitions, not only executable statements.
- Every deliverable contains jayis1 attribution in syntax-appropriate comments or metadata. Build products and local CAD preferences are ignored. The quality checker verifies minimum counts, required paths, attribution and selected secret patterns.

## Review disposition

A read-only implementation review found that interval accumulation could credit the sample interval preceding the first qualified observation, and that a gap-aborted trial retained the previous sample's duration. New regression assertions failed against the original code. Both paths were corrected: seating and rearm need agreement at both ends of a credited interval, and abandonment updates the bounded duration through detection. The entire firmware, companion and CAD suite was rerun after correction. The owner performed all repository writes and final acceptance; this was not full collective hardware review.

## Reproduce

From the repository root:

    make -C punch-register/firmware clean all test
    xvfb-run -a python3 -m unittest discover -s punch-register/tests -p 'test_*.py'
    /usr/bin/python3 punch-register/tools/check_hardware.py
    python3 punch-register/tools/check_quality.py

## Unverified boundaries

No hardware was fabricated, flashed or electrically measured. Switch force, follower geometry, paper disturbance, contact wetting, timing accuracy, USB current, EMC/ESD behavior, dimensional hole quality and workshop utility remain unverified. The passive board's clean ERC does not validate the external Nucleo supply or wiring. The cited component and cost choices are design/planning assumptions, not checked inventory or quotations. The device is an observation prototype, never a safety interlock. Prior-art reconnaissance supports a bounded differentiation statement, not worldwide novelty or patent clearance.
