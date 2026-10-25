# Takeup Atlas verification record

Author: jayis1

## Executed checks

- Repository: jayis1/hardware-design-lab. Remote default HEAD resolves to main. Initial checkout was clean; `git pull --ff-only origin main` returned already up to date. Only the new device and the root device-index entry are in scope.
- Inventory: existing device directory names and README titles/overview text reviewed. Closest platform relative is WebSlack Ledger; reuse is disclosed. Reversal trial measurement, confirmation and rejection logic are new. External prior art prevents an absolute global-novelty claim.
- GNU Arm compiler: arm-none-eabi-gcc 14.2.1. `make -C firmware all test` compiled and linked the Cortex-M4 image with warnings as errors, the real startup file and linker script. ELF section sizes: text 2876 bytes, initialized data 0 bytes, BSS 624 bytes. A binary image was produced. Build products are ignored, not committed.
- Firmware size: 542 physical lines across production `.c` files, excluding tests. Production C and headers contain 585 nonblank lines after C comments are removed. README has 3142 whitespace-delimited words. These counts are scope checks, not claims of correctness.
- Host engine suite ran with AddressSanitizer and UndefinedBehaviorSanitizer. It exercises normal response, simultaneous input/output reversal, output-leading behavior, stationary-input confirmation, interrupted input direction, wrong output direction, timeout, sample gaps, invalid phase transitions, position and event saturation, time wrap, CRC and insufficient serialization capacity.
- Python 3.13.5: `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` passed all eight tests. Tests parse a frame emitted by the actual C encoder, reject valid-CRC impossible records, test stream resynchronization and bounded buffering, enforce session consistency, reject replay FIFOs and oversized files, and verify private non-overwriting CSV exports.
- The GUI suite drives an actual Tk event loop and Linux pseudo-terminal. It exercises connection, batched records, all three tabs, angular calibration and invalid scale rejection, stale indication, export dialog callbacks, replay dialog callbacks, FIFO rejection, reconnect clearing, contradictory-result disconnection, actual PTY hangup, and terminal restoration following an injected setup error. PTY input is synthetic, not measured hardware data.
- KiCad CLI 9.0.2: schematic ERC reported zero violations; PCB DRC reported zero violations and zero unconnected items. The hardware checker compared the fresh schematic XML netlist with actual PCB pads and the manifest: 14 components, 30 pins, 10 nets, including values and footprint identities. Native KiCad netlist export also succeeded and is included.
- Schematic SVG exported with KiCad, converted with rsvg-convert and visually inspected. Four resistor/header channels, Nucleo power and signal headers, labels and author/title block were visible without overlap. The passive schematic intentionally does not contain the external processor, encoder internals or regulator.
- Read-only source review of the firmware and application found no blocking defects. A final owner review also made watchdog-start failure immediately set the FAULT state before the first telemetry frame, keeping state and flag semantics consistent.
- Authorship scan found jayis1 in every deliverable. No conflicting author credit, symlink, embedded credential pattern or unintended build output was found. `git diff --check` passed before publication.

## Unverified and bounded claims

No PCB was fabricated or assembled. No Nucleo was flashed. No encoder waveform, power inrush, GPIO threshold, watchdog reset, serial timing, electromagnetic behavior, mechanical repeatability or physical user trial was measured. CAD rule checks cover the passive carrier, not external wiring correctness. The encoder and board manuals informed the interface contract, but bench confirmation remains mandatory.

No claim is made that first-following travel equals pure gear backlash. The instrument observes coarse input travel and output confirmation under the operator's chosen load. Encoder drag, fixture compliance, phase quantization, unobserved fast transitions and power loss can distort or destroy evidence. Read the README limits and bring-up procedure before connecting hardware.

## Reproduce

From the device directory:

    make -C firmware all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
    /usr/bin/python3 tools/check_hardware.py

The CAD checker requires KiCad 9 CLI, system pcbnew bindings and standard footprint libraries. The GUI requires Linux, Tk and a display or Xvfb. No network application service or Python package installation is required.
