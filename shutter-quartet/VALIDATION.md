# Validation record

Author: jayis1

This record describes software and design-file checks, not physical measurements. Execution mode was standalone owner with advisory reports; no full collective review or physical approval is claimed.

## Executed checks

- arm-none-eabi-gcc 14.2.1 linked the freestanding Cortex-M4 firmware with -Wall -Wextra -Werror. Target image: 2876 bytes text, zero initialized data, 788 bytes BSS. The linker reserves stack headroom.
- make -C firmware test passed the exposure engine, production clock/configuration tests and production nonblocking serial tests with address/undefined-behavior sanitizers.
- The exposure engine compares all 576 opening-order/closing-order combinations with an independent time-window oracle. Additional cases cover simultaneous masks, fragments, independent-channel chatter, startup illumination, clear-window boundaries, gaps, timeout precedence, modular time wrap, terminal exhaustion, CRC and encoder bounds.
- Python 3.13.5: ten protocol/model/transport tests and two actual Tk GUI tests passed under Xvfb. GUI input was synthetic output from the real C encoder delivered through Linux PTYs. Tests cover navigation, live progress, hangup, replay, export and reconnect; they do not establish physical USB behavior.
- KiCad 9.0.2: schematic ERC reported zero violations. PCB DRC reported zero violations and zero unconnected items. No waivers were added.
- A fresh exported schematic XML netlist matched the manifest and actual board for fourteen components, thirty pins and ten nets, including values and full footprint library identities.
- The exported schematic SVG was converted with rsvg-convert and visually inspected. Labels and connector/resistor rows were legible without overlap. An initial optional CairoSVG conversion attempt failed because that package is not installed; the installed renderer was used instead.
- The quality gate checked at least 2000 README words, at least 500 nonblank/noncomment C/header lines, attribution in every delivered file, expected signal names, required artifacts and obvious credential patterns. README: 3395 words. Firmware: 660 substantive C/header lines, excluding tests.

## Scope review

The authored engine and app use the same SQ1 field order and fixed policy. Reused board support maps PA0, PA1, PA4 and PB0 to A through D and uses PA2 USART2 transmit only. Carrier sensor inputs are unpowered NPN collectors, not powered industrial outputs. Completed evidence is separate from working capture evidence, so abandonment does not overwrite a prior result. A first dark endpoint earns no preceding clear time; a raw glitch restarts the full clear window.

Only the new shutter-quartet directory and one root device-index row are intended for publication. The work uses the existing MIT-licensed jayis1 platform; installed KiCad library material retains upstream licensing. No credentials, vendor SDK tree, generated firmware binaries or Python caches belong in the commit.

## Limits and outstanding physical work

No board was fabricated, no MCU flashed, no detector mounted and no optical timing measured. Detector threshold margins, lamp stability, saturation recovery, sampling accuracy, mechanical repeatability, ESD, EMC and usefulness in a teaching session remain unverified. No worldwide novelty or patent-clearance claim is made: commercial and open-source multi-point shutter testers were found and are explicitly discussed in README.md.

The desktop's reset inference is heuristic without persistent boot identity. The snapshot protocol can miss individual completed captures during transport loss. Necessary consistency checks do not prove that arbitrary valid-CRC data came from a real shutter. The current design is a slow educational experiment recorder, not a photographic calibration instrument.

## Reproduction

From this directory:

    make -C firmware clean all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_quality.py

Run tools/generate_hardware.py only when intentionally regenerating the design files; it overwrites generated CAD artifacts. The first tests were run before their engine/model/GUI implementations and failed for missing implementations, then passed after implementation. Exact dark-window tests use two observed dark endpoints around each credited millisecond.
