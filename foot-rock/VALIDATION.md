# Foot Rock revision A validation

Author: jayis1

## Scope

This is software and electrical-CAD evidence for an unbuilt prototype, not physical acceptance. Work was executed by the standalone owner using advisory reports, not a completed independent collective review. Only foot-rock/ is added to the repository. Shared platform provenance is recorded in tools/import_platform.py and README.md; no unrelated project is modified.

## Executed checks

| Check | Actual result |
| --- | --- |
| Repository | jayis1/hardware-design-lab; configured default main; required git pull --ff-only reported already up to date |
| Firmware target | Clean arm-none-eabi-gcc 14.2.1 Cortex-M4 build and link passed with -Wall -Wextra -Werror |
| ELF identity | ELF32 little-endian ARM executable, EABI5, soft-float |
| Linked size | 2700 bytes text, 0 initialized data, 788 bytes BSS; stack usage is additional |
| Firmware tests | Three host executables passed with address and undefined-behavior sanitizers |
| Engine oracle | All 1024 length-five missing-foot words checked against independent visit/edge arrays |
| Topology enumeration | All 256 ordered raw-mask pairs checked for qualified state, anchor and edge behavior |
| Boundaries | Initial qualification, first credited dwell, 999/1000 settlement, final-interval glitch, raw multi-foot loss, sample gaps, timestamp wrap, invalid input, time fault, last legal counter increment and exhaustion passed |
| Platform tests | Clock failure bounds, pin configuration, UART busy/bounds/drain, timeout and wrap passed |
| Encoder | Production C fixture generated 2100 synthetic FR1 snapshots; CRC check vector and undersized output buffer tests passed |
| Companion | Eight Python unittest methods passed under Xvfb with actual Tk widgets/event loop |
| Integration | Real C-encoder replay, dense and sparse history validation, valid-CRC impossible records, duplicate/fault freeze and freshness checks passed |
| Serial GUI | Linux PTY connection, live acquisition, duplicate-connect refusal, disconnect/hangup, retained provenance and table rendering passed |
| Export | Real CSV creation, mode 0600 and overwrite refusal passed |
| Replay/framing | FIFO and empty-file rejection, split-byte parsing, truncated frame recovery and partial EOF rejection passed |
| Python compilation | compileall passed for application, tests and tools on Python 3.13.5 |
| KiCad | KiCad 9.0.2: zero ERC violations, zero DRC violations, zero unconnected items |
| Connectivity | Fresh schematic XML vs real PCB pads vs manifest: 14 components, 30 pins, 10 nets; values and full footprint identities match |
| Visual review | Fresh schematic SVG rendered to PNG and inspected: labels, connector references and four resistor paths are legible, with no obvious overlaps |
| Size gates | README 3701 whitespace-separated words; firmware has 526 nonblank noncomment .c lines, plus 93 header lines; tests excluded |
| Attribution | Every authored deliverable contains jayis1; installed footprint-library licensing is preserved |

Commands from repository root:

    make -C foot-rock/firmware clean all test fixture
    xvfb-run -a python3 -m unittest discover -s foot-rock/tests -p 'test_*.py' -v
    /usr/bin/python3 foot-rock/tools/check_hardware.py
    python3 -m compileall -q foot-rock/app foot-rock/tests foot-rock/tools
    python3 foot-rock/tools/check_release.py
    git diff --check

The firmware initially failed its test build because the engine and wire modules did not yet exist. The first UI test run failed because the workbench had not yet been implemented. Both were subsequently implemented and the complete commands above rerun successfully. A Python syntax error during workbench construction was corrected before the passing run. No fabricated test output or physical recording is included.

## Review findings and deliberate limits

The prior-art inventory found nearby force-balancing and seating devices. Foot Rock is distinguished by its directed missing-foot chain, raw topology guard and all-four resting cutoff; it does not claim to invent table leveling or prove worldwide novelty. Bounded external search results were available, while the configured direct web-extraction backend could not extract the referenced article.

The chain does not require a qualified all-four intermediate state. A direct observed transition from one three-contact vector to another is accepted if no two-or-fewer raw vector was observed. This is deliberately an endpoint notebook, not proof of a continuous pivot or of every intermediate support state. The README and UI use that restricted interpretation.

The entire contact vector qualifies together. Independent per-foot timing is not claimed. Raw two-or-fewer states break correlation before qualification, while every raw change breaks dwell and settlement credit. All-four contact for the exact configured resting window ends the prior chain; a glitch on the final would-be interval prevents that end. Directed edge counts have source/destination budgets as well as total conservation in firmware and companion.

No physical MCU was flashed, no switch fixture assembled, and no rail, GPIO threshold, contact force, timing accuracy, ESD immunity or USB-current measurement was performed. The passive carrier's ERC cannot verify the external Nucleo supply or its solder bridges. The candidate switch and mechanical envelope require real calibration and disturbance testing. No manufacturing release, calibrated shim thickness, load rating or furniture safety claim is made.

The raw serial protocol has CRC but no authentication or persistent boot identifier. It accepts no commands and is intended for trusted local USB use. A malicious source could manufacture plausible evidence. Sparse endpoint checks cannot prove hidden history. Some power resets remain indistinguishable from forward traffic; never merge runs merely because a parser accepts them. GUI replay and PTY tests are explicitly simulated transport, not physical USB tests.

The app keeps only the latest 1000 snapshots; cumulative device totals remain in each snapshot. Export metadata preserves provenance and qualifications after disconnect. No GitHub Actions workflow, cloud endpoint or changes to daily-ideas are part of this device.
