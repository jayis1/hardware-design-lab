# Mixing Gap validation record

Author: jayis1

Revision A is an unbuilt prototype. These results verify supplied software and CAD, not physical device performance. Review was performed in standalone-owner mode with advisory material, not full independent collective review.

## Executed checks

- `make -C mixing-gap/firmware clean all test fixture`, followed by a rebuild and test after the final analysis optimization: PASS. GNU Arm GCC 14.2.1 links the STM32F401 Cortex-M4 target with `-Wall -Wextra -Werror`; final ELF reports 2584 bytes text, zero initialized data and 1044 bytes BSS. Stack use and real-time timing have not been measured on hardware.
- Four C test executables pass with address and undefined-behavior sanitizers. Analysis covers every one of the 65,536 retained configuration sets, all seven unordered bipartitions and parent witnesses. Acquisition covers all 256 ordered raw-word pairs, exact qualification boundary, glitches, gaps, timer wrap, CRC, terminal faults and counter exhaustion. Imported clock and UART tests exercise production register logic, bounded failure, input configuration, busy rejection, buffer bounds, timeout and wrap behavior.
- `xvfb-run -a python -m unittest discover -s mixing-gap/tests -p 'test_app.py' -v`: all 11 tests PASS under Python 3.14.9. The Python oracle explicitly splices pairs of observed words, independently of the production packed-projection firmware. The model covers all 65,536 histories and tests completeness disappearing and returning as projections expand.
- The real C encoder produces 48 sparse snapshots and 960 dense synthetic snapshots. The app accepts both histories, reaches identical final state, and accepts every ordered sparse prefix/end pair. A CRC-valid impossible adjacent visit is rejected. Bad CRC, bad summaries, invalid values, truncated frames, oversized/FIFO replay and empty replay are rejected.
- Real Tk widgets are exercised under Xvfb. Tests inspect both screens, the AB|CD missing-word row and explicit parent explanations. Linux PTYs exercise live input, batched reads, double-connect rejection, stale acquisition, reconnect and hangup. Serial attributes are restored on close and constructor failure. CSV export checks author, provenance, field names, private mode 0600 and overwrite refusal. No physical USB serial device was attached.
- `/usr/bin/python3 mixing-gap/tools/generate_hardware.py` and `/usr/bin/python3 mixing-gap/tools/check_hardware.py`: PASS with KiCad 9.0.2. Native ERC: zero violations. Native DRC: zero violations and zero unconnected items. Fresh XML schematic netlist agrees with PCB pads and manifest for 14 components, 30 pins and 10 nets; values and full footprint identities agree as well.
- The native schematic SVG was rendered with rsvg-convert and visually inspected: four consistent pull-up/series/contact channels and external power/sense headers, readable net labels and no visible symbol/pin label collision at the inspected scale. This is not fabrication inspection.
- README has 2896 whitespace-delimited words. Production firmware has 561 nonblank, noncomment C-source lines plus 92 header lines; test and generated files are excluded. Every first-party deliverable contains jayis1 in format-appropriate metadata or comments. Build products, Python caches and KiCad local preferences are ignored.

## Interpretation gates

The complete mask is not monotonic. A singleton history is product-complete for all seven splits, whereas adding an opposite word creates gaps. Filling the two cross words closes AB|CD, and introducing another AB pattern creates a new cross requirement. Tests explicitly cover this sequence. Empty history is UNOBSERVED. Missing recombinations do not prove mechanical coupling, and observed product completeness does not prove physical or statistical independence.

The generic suggestion minimizes changes to an unseen full word; it does not optimize gap closure. Numeric parent witnesses describe retained configurations, not the actual order or path used to reach them. Tests check acquisition consistency and necessary sparse endpoint bounds, not an exhaustive proof of hidden acquisition history.

## Unverified work before operational use

No PCB fabrication, soldering, physical MCU programming, contact fixture, regulator-load measurement, EMC/ESD qualification, calibration, electrical timing or user study was performed. Measure worst-case main-loop acquisition latency with telemetry active; packed-projection analysis bounds processing but is not a measured execution-time guarantee. Confirm Nucleo revision, VCP bridges, contact wetting, GPIO low/high margins, supply polarity and strain relief before use. Passive-only ERC cannot validate the external processor board or its power source.

The design excludes external powered signals, mains, hazardous machinery, battery charging, isolation and safety interlocking. Real measurements must be acquired in a new explicitly identified session. Historical evidence must be exported before reset or mechanical adjustment. Some resets cannot be distinguished without persistent boot identity. Files are engineering design artifacts, not a certification or manufacturing release.
