# Detent Route revision A — verification record

Author and creator: jayis1

This is software and CAD verification of an unbuilt prototype, not physical qualification. Work was executed by the standalone repository owner; advisory material was not treated as a collective approval. All original deliverables are credited to jayis1.

## Executed gates

- Repository: jayis1/hardware-design-lab. The initial worktree was clean on main. Remote HEAD identified main. `git pull --ff-only origin main` completed with Already up to date before implementation.
- Duplicate check: repository device directories and titles were listed. ContactAtlas and Stencil Orbit measurement contracts were read in full. The README records distinctions and the limits of the public prior-art search.
- Toolchain: GNU Arm GCC 14.2.1, Python 3.13.5, KiCad CLI 9.0.2.
- `make -C firmware clean all test fixture`: passed. The actual Cortex-M4 ELF linked with `-Wall -Wextra -Werror`, freestanding startup, direct drivers and linker script. Target size was text 2792 bytes, data 0 bytes, BSS 788 bytes. The ELF and binary are ignored build products, reproducible with the Makefile.
- Engine host tests: passed under address and undefined-behavior sanitizers. An independent ordinal oracle checked all 4096 six-destination station words. Additional assertions cover exact qualification, adjacent reversal, missing clear departure, overlap, held overlap, sample gaps, completion at 9999 ms versus rejection at 10000 ms, ordinary timestamp wrap, start-counter exhaustion, fault abandonment, CRC check string and short output buffer.
- Register and UART host tests: passed. These exercise bounded reset-clock failure paths, GPIO/USART masks, buffer bounds, busy rejection, nonblocking drain, timestamp wrap and timeout recovery using simulated registers, not actual silicon.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v`: nine tests passed. They consume the production C encoder's 18 synthetic snapshots, verify route totals, malformed-frame recovery, strict replay endings, impossible CRC-valid records, duplicate consistency, freshness, host pause rejection, private export and overwrite refusal, empty-session metadata, FIFO rejection, real POSIX serial attribute restoration, and the actual Tk event loop through a Linux pseudo-terminal. The GUI test navigates all three screens, receives all fixture rows, exports, observes hangup without erasing evidence and loads replay as NOT LIVE.
- A read-only advisory review identified a multiple-completion sparse-snapshot hole. The owner reproduced it with a failing regression, generalized last-result classification and duration checks to every positive completion delta, then reran the full tests successfully. No reviewing agent wrote repository files.
- `python3 -m compileall -q app tools tests`: passed.
- `/usr/bin/python3 tools/check_hardware.py`: passed. Actual schematic ERC reported zero violations. PCB DRC reported zero violations and zero unconnected items. Exported schematic XML and actual PCB pads matched the manifest for 14 components, 30 pins and 10 nets, including values and footprint identifiers.
- `kicad-cli sch export svg`: succeeded. The rendered sheet was visually inspected: four separate resistor/header channels, power and MCU headers, legible station labels and author title block. This is a schematic readability inspection, not a physical layout approval.
- README count: 3384 whitespace-separated words. Firmware: 644 nonblank C/header lines after excluding comments, tests and generated products. `tools/verify.py` reproduces these thresholds and checks attribution and station wiring metadata.

## Evidence interpretation

The real C fixture is synthetic: it drives the portable engine with constructed contact masks. The PTY is an operating-system serial surrogate, not a physical USB device. Register simulations test driver decisions without establishing electrical behavior. Whole-vector qualification intentionally suppresses short signatures; short overlap and hidden complete rotations can remain undetected. Necessary host snapshot checks do not prove a unique history between records. Route laps are uninterrupted four-step adjacent paths, not certified shaft revolutions.

The board is a complete passive carrier; it does not contain the processor, regulator, USB bridge, decoupling or programmer. Those are supplied by the external NUCLEO-F401RE. Clean passive ERC does not validate that external assembly. Two attempts to retrieve fresh ST manufacturer PDFs timed out, so the README accurately identifies the reused platform and requires board-revision and wiring verification before assembly.

## Not performed

No PCB fabrication, solder assembly, target flash, physical switch test, logic-analyzer timing calibration, mechanical fit or force measurement, angular-accuracy study, EMC/ESD test, safety certification, commercial availability audit or workshop user trial was performed. No physical performance numbers were invented. No motors, mains signals, machine controllers or safety interlocks are supported.

## Reproduction

From the device folder:

```sh
make -C firmware clean all test fixture
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 tools/verify.py
```

The project has no Python package dependencies beyond the system Tk installation, no npm installation and no network runtime. Regeneration of the CAD uses `/usr/bin/python3 tools/generate_hardware.py` with installed KiCad footprints and pcbnew bindings. Validate the regenerated files again before fabrication. Publication is separately checked against the remote main reference after push; this file does not pretend that writing a local commit proves remote publication.
