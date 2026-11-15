# Float Band revision A validation

Author: jayis1

## Executed verification

- GNU Arm Embedded GCC 14.2.1 cross-compiled and linked the actual Cortex-M4 target with `-Wall -Wextra -Werror`, freestanding startup and the supplied linker script. ELF size: 2,672 bytes text, zero initialized data, 788 bytes BSS. A binary image was produced. This is target linking, not target execution.
- GCC 14.2.0 host engine tests passed with AddressSanitizer and UndefinedBehaviorSanitizer. They enumerate all 65,536 four-word stable traces against an independent polarity/band oracle, and separately test every valid band and initial polarity, short excursions, invalid reference words, same-endpoint ambiguity, raw reference disturbance, skips, reversals, sampling gaps, overflow, timestamp wrap, buffer exhaustion and the standard CRC check vector.
- Shared production clock/input-configuration and UART-driver host tests passed with both sanitizers. These simulate registers; they do not establish physical clock startup or serial electrical behavior.
- Python 3.13.5 ran 12 application tests successfully under Xvfb. Coverage includes CRC/framing recovery, conservation and monotonicity, startup qualification, diagnostic growth, zero-acquisition halt transitions, contradictory duplicates, strict replay, partial recording tails, nonoverwriting CSV export, POSIX serial attributes, invalid TTY paths, GUI replay, actual GUI event-loop serial ingestion through a PTY, duplicate connection rejection and hangup with retained evidence.
- A real C-encoder synthetic fixture was accepted by the application. The headless export command wrote eight validated replay snapshots. GUI tests also consumed those exact frames. No physical serial device was connected.
- KiCad 9.0.2 native schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. A fresh exported schematic XML netlist matched the PCB and connectivity manifest: 14 components, 30 pins, 10 nets; values and full footprint library identities were also compared.
- The schematic was exported to SVG, rasterized with rsvg-convert, and visually inspected for readable reference/net labels, connector numbering and author/title placement. An attempted CairoSVG conversion was unavailable; rsvg-convert completed the render instead. No renderer package was added to the project.
- README word count: 2,887 by whitespace splitting. Firmware C/header source: 640 lines, including 579 nonblank noncomment lines. Tests and generated CAD are excluded from the firmware count. Shared acquisition drivers are explicitly identified as reused jayis1 source rather than represented as newly invented code.
- All authored deliverable files contain jayis1 attribution. Build artifacts, Python bytecode and KiCad local preferences are ignored. No packages were installed and the app has no third-party Python dependencies.

## Review corrections

A read-only software review identified three impossible-frame cases in the first implementation of the companion validator. Each was reproduced as a failing regression before correction: an initial fault row changing evidence without acquiring samples, diagnostic counters growing beyond available samples, and a first DUT edge appearing before its startup qualification interval. All regression tests now pass. This was additional advisory review during standalone-owner execution, not a full collective hardware review.

## Reproduction

From `float-band/`:

1. `make -B -C firmware all test fixture`
2. `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'`
3. `python3 tools/check_hardware.py`
4. `python3 app/workbench.py --replay firmware/build/demo.fb1`
5. For a new export path: `python3 app/workbench.py --replay firmware/build/demo.fb1 --export /path/to/new.csv`

The generator requires KiCad pcbnew and installed standard footprint libraries. The generated schematic embeds its symbols and the PCB contains footprint geometry. Regeneration should be followed by native checks and a diff review. System tools are dependencies, not vendored files; versions above describe what was actually exercised.

## Remaining physical release gates

No fabrication, assembly, target flashing, water experiment, contact calibration, enclosure test, EMC/ESD evaluation or regulatory certification occurred. Reference float hysteresis and liquid-density dependence can move the effective brackets. Reference switch procurement is specified functionally and must be verified for low-current operation. Complete a meter-based wiring inspection, dry-switch bring-up, measured bidirectional reference calibration and video-correlated slow fill/drain trial before treating the fixture as useful measurement hardware. No claim of dimensional hysteresis accuracy, worldwide novelty, commercial readiness or safety-system suitability is supported by these tests.
