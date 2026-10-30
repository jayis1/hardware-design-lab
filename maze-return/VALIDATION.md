# Maze Return revision A validation

Author: jayis1

## Publication recovery

Recovered the untracked design from an interrupted execution of this same task; no second device was created. The configured destination is `jayis1/hardware-design-lab`, default branch `main`. A fast-forward-only pull reported up to date before recovery. Fresh verification rebuilt the target from a clean build directory, passed all three sanitizer executables and all eight Python tests, reran native ERC/DRC and exact netlist comparison, and rechecked source attribution and quantitative requirements. The schematic was freshly exported and visually inspected. Only the device's 35 source/design files are publication inputs; build outputs, Python caches and KiCad local preferences remain ignored. Review was standalone-owner review with advisory reports, not a completed collective review. Physical validation limitations below remain unchanged.

## Executed checks

- Target compiler: arm-none-eabi-gcc 14.2.1. `make -C firmware all` linked the actual Cortex-M4 image with warnings treated as errors, freestanding startup, vector table and linker script. GNU size reported text 2568 bytes, data 0 bytes, BSS 788 bytes; stack storage is additional. ELF and BIN are ignored build products.
- `make -C firmware test`: three sanitizer-enabled executables passed: measurement engine/encoder, clock/configuration driver and UART driver.
- Engine tests compare all 65,536 length-eight checkpoint walks against an independently maintained route oracle. They also exercise repeated tail visits, raw overlapping gates, insufficient clear separation, rejected short pulses, 99/100 ms clearing, clearing interrupted by a raw glitch, 5/6 ms sampling-gap boundaries, timestamp wrap, timeout/completion precedence, transactional capacity exhaustion and frozen terminal faults.
- Python 3.13.5: `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'` passed eight tests. These use the production C encoder, including four terminal fault fixtures; CRC-valid impossible transitions; corruption and truncated framing; freshness and duplicate handling; strict replay; private export and overwrite refusal; and the real Tk event loop with Linux pseudoterminals.
- GUI test visits all three tabs, establishes a serial connection, rejects a duplicate connection, receives advancing C-generated snapshots, checks displayed route and history, exports, handles PTY hangup, replays the fixture and reconnects with cleared numerical summaries.
- `python3 -m compileall -q app tools tests` completed successfully. Runtime tests resolve the standalone application imports; editor-only import-resolution warnings are not runtime failures.
- KiCad CLI 9.0.2: schematic ERC returned zero violations. PCB DRC returned zero violations and zero unconnected items. No exclusions were added to suppress failures.
- `tools/check_hardware.py` exported a fresh native XML netlist and compared schematic, PCB and connectivity manifest: 14 components, 30 pins and 10 nets, including exact values and full footprint identities.
- Schematic SVG exported with KiCad, converted using rsvg-convert, and visually inspected: readable references, values and gate labels; four distinct channel rows; power/input headers; author title block. Render intermediates are not deliverables.
- `tools/check_quality.py`: README 3,388 whitespace-delimited words; firmware 625 substantive nonblank C/header lines after removing comments. Tests and generated build products are excluded from the firmware count. Authorship is checked in all nonignored device source files.

## Review corrections

Inter-frame validation now rejects a changed route root and impossible single-visit append/repeat/truncate transitions even when both frames are individually valid and their CRCs match. Firmware also preserves an established terminal fault when the transport error counter subsequently saturates. Regressions demonstrated both failures before the corrections and passed afterward. The finite-trial boundaries and four fault types received additional encoder/application coverage.

## Evidence boundaries

No fabricated PCB, flashed Nucleo, reed capsule, magnet or enclosure was present. No physical sensing distance, real-time accuracy, power draw, EMC, ESD immunity, magnet retention or educational outcome has been measured. PTYs emulate the host serial device, not STM32 electrical behavior. Native CAD checks validate the passive carrier; they cannot establish the purchased module's wiring or mechanical safety. Manufacturer connector information was retrieved through an indexed distributor copy after direct ST PDF retrieval timed out; verify the actual board revision before assembly.

The application intentionally stores bounded snapshots rather than a complete visit log. Multiple unobserved visits between snapshots receive necessary consistency bounds, not an exact reconstruction. Reset inference cannot prove session identity without a persistent boot identifier. Unplugging USB may remove device power and erase volatile evidence. No safety, clinical, intelligence-assessment, shortest-path or worldwide novelty claim is supported.
