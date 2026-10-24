# Siphon Witness verification record

Author: jayis1

## Executed checks

Commands were run from the repository root unless noted otherwise.

- `make -C siphon-witness/firmware clean all test`: PASS. arm-none-eabi-gcc 14.2.1 produced the Cortex-M4 ELF and binary using the supplied startup and linker script. ELF size: 2108 bytes text, zero data, 288 bytes BSS. Stack and local engine storage are additional runtime RAM usage, not included in BSS.
- The Makefile's host executable links the production engine and wire encoder with GCC address and undefined-behaviour sanitizers. Assertions cover completion and paired band duration, rearm, reverse movement, impossible topology, sampling gaps, trial timeout, skipped thresholds, bounce rejection, clock wrap, saturating arithmetic, transport-loss accounting, CRC known vector and encoder capacity. PASS.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s siphon-witness/tests -p 'test_*.py' -v`: seven tests PASS. Parser tests consume a frame emitted by the real C encoder, reject CRC/range failures, resynchronize truncated frames, detect duplicate/restart traffic, and protect existing export files. The real Tk event loop opens a Linux pseudo-terminal, receives synthetic telemetry, updates the display, marks stale data, switches to replay, exports CSV, preserves replay provenance after disconnect, clears old evidence on reconnect, and handles TTY disconnection.
- `/usr/bin/python3 siphon-witness/tools/generate_hardware.py`: produced 14 carrier components and ten nets with KiCad 9.0.2 bindings and standard footprint libraries.
- `/usr/bin/python3 siphon-witness/tools/check_hardware.py`: ERC zero violations; DRC zero violations; zero unconnected items. Fresh schematic XML netlist agrees with every PCB pad and manifest assignment: 14 references, 30 pins, ten nets. Component values and full footprint library identities also agree. No rule exclusions were required.
- `kicad-cli sch export netlist --format kicadsexpr -o siphon-witness/kicad/device.net siphon-witness/kicad/device.kicad_sch`: native netlist exported successfully.
- Schematic SVG exported with KiCad CLI and rasterized for visual inspection. Left-side labels were corrected to avoid overlapping symbols, regenerated, checked and inspected again.
- README count excluding fenced code: 3140 words. Firmware C/header count excluding blank and block-comment lines: 593 lines, excluding tests, build products and third-party source.
- Power arithmetic checked programmatically: 0.300 mA per closed reed, 1.20 mA for four, approximately 0.300 V input-low level, 3.96 mW carrier resistor dissipation, 0.75 W allocated USB budget.
- Source review found two export-integrity issues: mutable connection state erased capture provenance, and CSV omitted parser/restart qualifications. Separate immutable session provenance and explicit integrity metadata now accompany exports. Regression tests were observed failing before implementation and passing afterward.

## Scope and provenance

All original files and commit authorship credit jayis1. The electrical topology reuses a proven passive four-input carrier pattern from this repository; the siphon measurement engine, two-vessel semantics, UI and tests are specific to this device. Installed KiCad footprint geometry retains its native library identity. The custom PCB does not contain the MCU, USB circuitry or the external floats; these are explicitly specified external assemblies.

No firmware image was flashed to hardware. No physical float switch, hydraulic experiment, USB serial adapter or fabricated board was available for testing. PTY tests demonstrate application/serial software behaviour, not an electrical link. Host-generated fixture timings are synthetic, not laboratory results. CAD tests validate the supplied carrier and do not certify the external module/harness, protection, enclosure or sensor procurement.

The ST board-manual retrieval attempt failed (HTTP/2 transport failure, then HTTP/1.1 timeout). The README names the relevant reference documents but does not claim successful fresh datasheet verification. Confirm the exact Nucleo revision, virtual-COM solder bridges and float contact suitability before assembly. Limited web search and repository comparison support differentiation only; universal novelty is not established.

## Reproduction requirements

Use a Linux environment with arm-none-eabi-gcc, host GCC sanitizer support, Python 3, Tk, Xvfb for headless GUI tests, KiCad 9 CLI/Python bindings and standard footprint libraries. Use `/usr/bin/python3` for KiCad scripts if an isolated interpreter lacks pcbnew. No npm, pip, vendor SDK, network service or cloud credentials are required for the build and test commands. Build outputs and Python caches are ignored rather than committed. All durable deliverables are under this device directory.
