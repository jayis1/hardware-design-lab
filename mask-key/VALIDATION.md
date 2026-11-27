# Mask Key validation record

Author: jayis1

## Readiness and scope

Revision A is an unbuilt engineering prototype. This record reports executed software and CAD checks, not electrical measurements, fabricated-board acceptance or a physical experiment. Work was performed in standalone-owner mode with advisory reports, not full collective review. The live checkout and remote HEAD both identified jayis1/hardware-design-lab and default branch main; conflicting repository names in advisory material were not treated as configuration. No daily-ideas files were touched.

## Executed checks

- `git pull --ff-only`: already up to date before implementation. Device directories and README titles were inventoried; the closest Truth Sieve, Pair Cover and Label Veil purposes were examined.
- Firmware test-first gate: `make -C mask-key/firmware test` initially failed because the new subset APIs and analysis implementation were absent. After implementation, `make -C mask-key/firmware clean all test fixture` passed.
- Actual ARM target build: GNU Arm Embedded GCC 14.2.1, Cortex-M4 Thumb, freestanding C11, `-Wall -Wextra -Werror`. ELF size output: 2552 bytes text, zero initialized data, 1044 bytes BSS. Stack usage is not included in BSS. ELF and binary are generated under ignored firmware/build.
- Sanitizer-enabled C analysis test: all 65,536 retained relations checked against a bucket-label oracle for compatible subsets, inclusion-minimal subsets and valid refuting pairs. Every relation was also tested for response rows and guidance at all raw endpoints. All ordered stable-word pairs, exact qualification, gaps, retained contradiction, ordinary timer wrap, input failure, counter exhaustion, CRC and short output buffer were exercised.
- Additional production-driver tests: clock configuration masks and bounded failure paths; UART bounds, busy rejection, nonblocking drain, wrap and timeout recovery all passed.
- Companion test-first gate: initial import failed because the new subset model was absent. Final `xvfb-run -a /usr/bin/python3 -m unittest discover -s mask-key/tests -p 'test_*.py'`: 11 tests passed using distribution Python 3.13.5. Tests include exhaustive relations, 72 real-C-encoded synthetic snapshots, XOR A/B minimal-subset display, later all-subset contradiction, valid-CRC impossible data, fragment recovery, rollback, strict replay bounds, serial attribute restoration, real Tk screens, private CSV export and overwrite refusal, PTY stale acquisition/reconnect/hangup, failed non-TTY opens and injected permission denial.
- KiCad 9.0.2: `/usr/bin/python3 mask-key/tools/generate_hardware.py` generated 14 components and 10 nets. `/usr/bin/python3 mask-key/tools/check_hardware.py` passed native ERC with zero violations and DRC with zero violations and zero unconnected items. Fresh schematic XML connectivity matched all 30 PCB pads and manifest pins, and every component value and footprint identifier matched.
- Native schematic SVG export succeeded. The rendered schematic was converted using rsvg-convert and visually inspected for readable labels, component organization and title attribution. CairoSVG was unavailable; no dependency was installed merely to render the drawing.
- Source audit: README has 2,936 whitespace-delimited words. Firmware contains 726 C/header lines; C files alone contain 560 nonblank, noncomment lines. The count excludes tests. All deliverable files checked contained jayis1 attribution; no alternative creator or copyright name was introduced.
- Read-only advisory software/protocol review reported no blockers and independently re-exercised existing C test binaries and the Python exhaustive-relation and real-encoder tests. The owner remains responsible for the final build, CAD, scope and publication checks. This is not full collective signoff.

## Reproducibility and inherited infrastructure

The import script pins jayis1 infrastructure from truth-sieve at commit 30bd817a0068e09cd4aafbde2447d768be8d2e0b and refuses existing destination files. It is provenance for the initial reuse, not a command to overwrite the completed design. Device-specific changes include the three-control/one-response relation projection, compatible-subset lattice, inclusion-minimal results, explicit counterexamples, MK1 telemetry, GUI and exhaustive oracles. Hardware net labels identify CONTROL_A, CONTROL_B, CONTROL_C and RESPONSE_Y consistently with board.h and the README.

Use the firmware Makefile and app tests to regenerate synthetic recordings. Never relabel demo.mk1 as measured hardware data. The final synthetic stage deliberately contradicts earlier evidence; all subsets becoming refuted is the expected test result, not a communication error.

## Unverified physical and product claims

No board was fabricated or powered, no target was flashed, and no USB serial link to a physical Nucleo was exercised. Watchdog periods, actual oscillator error, worst-case execution latency, switch excitation quality, input noise, cable immunity, contact-force disturbance and total USB consumption remain unmeasured. The watchdog and serial logic were tested on a host and the target was cross-linked; those are different claims from silicon validation.

The passive carrier has no isolation or external-voltage protection. Its schematic has passive connector pins; ERC cannot validate the external Nucleo supply or wiring. Assembly, continuity, polarity, supply measurements, mechanical fit, real contact interpretation, reset behavior and known-function trials remain mandatory before physical use. The enclosure and mechanism-specific witness brackets are not fabrication-ready mechanical deliverables. No EMC, safety, medical, industrial or access-control certification is claimed.

Repository nonduplication is supported by the inventory and purpose comparison. Absolute worldwide originality, patentability and freedom to operate are not established. Compatible subsets describe only retained observations under a memoryless model; they are not permission to remove a sensor or a diagnosis of causality.
