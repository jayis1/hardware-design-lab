# Sweep Memory validation record

Author: jayis1

This is standalone-owner verification, not collective or independent design approval. Advisory reports were resolved against the actual checkout: jayis1/hardware-design-lab, configured default branch main. Reports naming creative-projects or SoC-Device-Inventions did not describe this checkout. No other repository was changed.

## Executed checks

- Required `git pull --ff-only` completed before implementation; starting revision 396308d62ab2f6f041beff9168b56cf5a6eb8a7c.
- Clean target build with arm-none-eabi-gcc 14.2.1, Cortex-M4 Thumb, freestanding C11, `-Wall -Wextra -Werror`. ELF linked with 2,620 bytes text, zero initialized data, 1,044 bytes BSS. Stack storage is additional to BSS.
- ELF inspection verified ARM executable, vector section at 0x08000000 and BSS at 0x20000000. Only SysTick is enabled; board-specific peripheral interrupts are not used.
- Engine, production clock and production UART host tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. Phase decoding checked against an independent circular-list oracle for every ordered phase pair. Retraces tested in both directions across contact patterns. Separate cases cover full mismatch masks, early reversal, illegal jump, overshoot, acquisition gap, invalid inputs, timeout, counter exhaustion, CRC reference vector and short output buffer.
- Final return qualified one sample before the rejecting deadline completes; the exact deadline rejects. Tests acquire the actual age instead of injecting an inconsistent timer.
- Six Python protocol tests passed. They include actual C-encoder replay, malformed CRC, truncated tail, invalid mismatch arithmetic, contradictory duplicates, resynchronization and counter-reset rejection.
- Real Tk event loop tested under Xvfb with Python 3.13.5. Both screens were exercised, actual C-generated records traveled through a Linux serial PTY, and the latest completed result was retained after PTY hangup. Private CSV mode, overwrite refusal, FIFO rejection, double-connect guard, failed connection preservation and replay diagnostic reset passed. Synthetic exchanges are not physical-device tests.
- KiCad 9.0.2 ERC: zero violations. PCB DRC: zero violations and zero unconnected items. Fresh schematic XML netlist matched every actual PCB pad and manifest entry: 14 components, 30 pins, 10 nets, including values and full footprint identities.
- Fresh SVG schematic rendered to PNG and visually inspected. Four channel rows, supply connector, MCU connector, labels and jayis1 title-block credit were legible without visible collisions.
- README contains 3,053 whitespace-delimited words. Firmware has 687 C/header lines, including 624 nonblank noncomment lines, excluding tests and build products. Reused platform code is credited explicitly, not presented as wholly new firmware.
- Every authored deliverable was checked for format-valid jayis1 attribution. Build outputs, Python caches and local KiCad preferences are ignored. No credentials or unrelated source changes are intended for staging.

## Reproduce

From the repository root:

    make -C sweep-memory/firmware clean all test fixture
    python3 sweep-memory/tests/test_app.py
    xvfb-run -a /usr/bin/python3 sweep-memory/tests/test_gui.py
    /usr/bin/python3 sweep-memory/tools/check_hardware.py
    kicad-cli sch export svg -o <inspection-directory> sweep-memory/kicad/device.kicad_sch

The first test attempt intentionally failed because the new engine did not yet exist; the application test likewise initially failed because the new model was absent. Implementation then satisfied those contracts, followed by clean full reruns. Test fixtures are generated locally and are not committed as measurements.

## Remaining limits

No physical hardware was fabricated, flashed, calibrated, powered, or tested. Passive ERC cannot validate external Nucleo wiring, supply polarity or correct purchased switch variants. The carrier has no isolation or ESD protection. Contact wetting at the nominal 0.300 mA current requires measurement. Whole-word qualification can miss fast phase movement, and an entire missed quadrature cycle can remain undetectable. Matching relative cells does not prove equal physical angle; the encoder and fixture contribute directional hysteresis.

Manufacturer PDF retrieval was limited: the direct Bourns URL returned 403 and the ST manual timed out. Search retrieval returned a mirrored Bourns manufacturer datasheet with A/common/B labels and quadrature specifications. Current supplier revisions, Nucleo solder bridges and real electrical levels must be checked before assembly. The code and CAD are engineering deliverables, not manufacturing approval, accuracy certification or worldwide-novelty proof.
