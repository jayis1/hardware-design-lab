# Catch Witness revision A validation

Author: jayis1

## Executed checks

- Repository identity: `jayis1/hardware-design-lab`; remote HEAD identifies `main`. Initial working tree was clean. `git pull --ff-only` returned already up to date before implementation. No task branch was created.
- Inventoried repository device directories and README concepts; compared close latch/door alternatives including HingeScribe, SealBeat, FoldTrace and Detent Loop. Novelty is a bounded repository concept claim, not global originality or patent clearance.
- `make -C firmware all test`: actual STM32F401 target linked using GNU Arm Embedded GCC 14.2.1, supplied startup, linker script and production drivers. Target reports 2,536 bytes text, zero initialized data, 788 bytes BSS. Stack and local variables are additional to BSS.
- Native address/undefined-behavior sanitizer tests passed for episode classification, inclusive credit expiry, held-input nonrenewal, simultaneous opening precedence, settlement glitches, ignored settlement-coincident tug, timeout, timestamp wrap, observation gaps and transactional counter exhaustion.
- Exhaustive mask-pair validity checks cover all 256 ordered four-bit mask pairs. A separate expected-outcome calculation tests every association presence combination over delays 5 through 510 ms. This is software simulation, not physical timing evidence.
- Production clock and UART tests passed, including bounded startup failure, input configuration, busy rejection, timeout, nonblocking drain and wrap.
- Five Python unittest methods passed under Xvfb. They exercise actual C-encoded recordings, strict replay, CRC-valid impossible records, truncation recovery, serial PTYs, non-TTY rejection, terminal restoration, all three Tk screens, full history rendering, exclusive private CSV export, stale acquisition blocking and HALT display.
- KiCad 9.0.2 schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items.
- Exported a fresh KiCad XML netlist and compared schematic, PCB pads and manifest: 14 components, 30 pins and 10 nets agree, including values and footprint identities.
- Exported the schematic to SVG and rendered it using `rsvg-convert`; visually inspected component symbols, labels and title block. Initial CairoSVG conversion was unavailable, so the installed renderer was used instead. No CAD check depended on CairoSVG.
- README measured 2,968 words; firmware measured 614 nonblank, noncomment C/header lines, excluding tests. Per-file author attribution and private-key-pattern checks passed. The source count is remeasured by `tools/verify.py` as documentation evolves.
- Ideal resistor calculation independently evaluated: 0.30 mA per closed contact, 0.30 V sensed low, 1.20 mA total and 3.96 mW with four closures. These are nominal calculations, not bench measurements.

## Review resolutions

Read-only software review identified that buffered snapshots and advancing HALT heartbeats could be described too strongly as live evidence. The interface now explicitly labels absolute acquisition age unverified, even with advancing timestamps, and prioritizes HALTED/acquisition-invalid status. Fault and gap counters are visible and export contains source, reception age and acquisition uncertainty. Regression tests exercise the production GUI paths.

The review also identified the settlement-completion boundary for tug credit. The selected policy deliberately requires a previously established LATCHED state, rather than treating a declaration during seating as a retention challenge. The README states this boundary and a C regression verifies that the coincident declaration is ignored. Handle/tug edge precedence and expiration remain independent of held levels.

## Reproduction

From this device directory, run `python3 tools/verify.py`. The script performs target build, sanitizer tests, real-encoder synthetic fixture generation, Xvfb GUI/transport tests, ERC/DRC/netlist comparison and source checks. It does not program hardware or publish changes. To force a full rebuild first run `make -C firmware clean`.

System prerequisites: Python 3 with Tk, Xvfb, native GCC, arm-none-eabi GCC/binutils, KiCad 9 CLI and its distribution Python bindings. There are no application package downloads. Generated binaries, test recordings, rendered images and local KiCad preferences are ignored build artifacts, not fabricated measurement results.

## Explicitly not verified

No board fabrication, physical target programming, oscilloscope measurement, USB electrical test, switch calibration, latch assembly, bracket repeatability, human trial, ESD/EMC test or regulatory certification was performed. The custom board is a passive carrier; MCU power support is supplied by the separately purchased Nucleo. Passive ERC does not prove the external wiring is safe or correct.

Serial CRC is not authentication. Sparse snapshots cannot reconstruct all intermediate events, and some resets can remain undetected. Declared tugs do not establish applied load. Settlement indicates contact states, not holding force. These limitations prevent use as a security monitor, safety interlock, certified retention tester or basis for safety-critical decisions.
