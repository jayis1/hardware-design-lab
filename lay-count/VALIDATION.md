# Lay Count validation record

Author and creator: jayis1

## Verified locally

- Repository identity: `https://github.com/jayis1/hardware-design-lab.git`; configured remote default branch `main`. Clean checkout before work; `git pull --ff-only` reported already up to date. Peer suggestions naming other repositories were not treated as authoritative. No daily-ideas files were accessed or changed.
- Target: GNU Arm Embedded GCC 14.2.1, Cortex-M4 Thumb, freestanding startup/linker and real STM32F401 register drivers. `make -C firmware clean all test` passed. ELF inspection reports ARM ELF32 executable, EABI5 soft-float. Linked text 2788 bytes, data 0 bytes, BSS 788 bytes; stack and local engine/frame storage are additional RAM use, not included in BSS.
- Three sanitizer-backed C executables passed: engine/encoder, clock register-mask/failure-path simulation, and production UART state/bounds/timeout tests. Engine tests cover mirrored passes with 0–39 index pulses, successive direction changes, same-end abandonment, raw tension dropout, rejected short index assertions, coincident arrival/index, gap termination, maximum duration, overlapping endpoints, timestamp wrap, numeric exhaustion, CRC reference vector and bounded encoding. All 65536 four-observation words over the four raw inputs are checked for accounting invariants. These are simulated inputs, not physical trials.
- Test-first execution was recorded: the initial engine build failed because the new engine/encoder were absent; initial companion tests failed because the new model/workbench were absent. Implementation then passed the tests. A successive-traverse test exposed a missing immediately reusable endpoint baseline after completion; that was corrected and the full suite rerun.
- Ten Python unittest methods passed through `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v`. They cover the actual C-generated fixture, malformed/truncated frames, valid-CRC impossible snapshots, timestamp/counter regressions, duplicate freshness, cumulative backlog, host pauses, aggregate transitions, strict regular-file replay, private CSV and overwrite refusal, serial termios restoration, PTY hangup, three actual Tk tabs, batched history rendering and cleared replacement-session views.
- Python app, tools and tests passed byte compilation. The application requires Linux, Python 3 and Tk; it has no pip dependencies or remote backend.
- KiCad 9.0.2 loaded the native schematic and PCB. `tools/check_hardware.py` reported zero schematic ERC violations, zero PCB DRC violations and zero unconnected items. Fresh schematic netlist, PCB pads and manifest agree on 14 components, 30 pins and 10 nets, including values and full footprint identities.
- Schematic SVG export was rendered to PNG and visually inspected. Four distinct input networks and the external power/MCU headers are readable, without overlapping signal labels. Render files stayed outside the repository in scratch storage.
- `tools/check_quality.py` passed: README 3124 whitespace-delimited words; production C/header sources 650 nonblank, noncomment lines, excluding tests. All deliverable files contain format-compatible jayis1 attribution. Header order, field-net order, project JSON and secret-pattern checks passed. `git diff --check` passed. Generated binaries, Python caches and KiCad local preferences are ignored.

## Limits and remaining physical work

The design is an unbuilt engineering prototype. No target flashing, actual UART timing capture, GPIO threshold measurement, oscillator characterization, watchdog-reset injection, fabrication, assembly, ESD test, regulatory review or winding experiment occurred. Passing passive schematic ERC cannot validate the external development board, contact specifications or assembly wiring. Reed-contact selection and fixture brackets must be checked against the actual material and winder before assembly.

The manufacturer UM1724 PDF retrieval timed out; the pin and peripheral platform was reused from reviewed jayis1 repository sources. Retrieve and compare the exact purchased-board documentation before wiring. Public winding-machine references support a bounded prior-art comparison, not a global originality or patentability claim.

Single-channel spindle index pulses are unsigned and cannot identify reverse rotation. Completed observations are guide traverses, not proof of a laid layer. Tension indication is a mechanical threshold, not measured force. Receiver-only reset inference has no persistent boot identity, and snapshots can omit intermediate completions. These limitations are documented in the README and app.

## Reproduce

From this device directory:

    make -C firmware clean all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_quality.py
    python3 app/workbench.py

This record documents standalone-owner verification using advisory reports, not a full independent collective hardware review.
