# Aisle Pair verification record

Author: jayis1

## Scope and provenance

Target: jayis1/hardware-design-lab, remote default branch main. The working tree was clean before the required `git pull --ff-only origin main`; it reported already up to date at df23325372d91f894314a93dbaaad7e2e5f85d6b. No work is placed in daily-ideas. Aisle Pair is a new device directory. Repository inventory and the closest measurement contracts were reviewed; the README distinguishes Rail Skew, Nest Settle and Bin Phrase and limits novelty claims.

This run uses standalone-owner execution. Peer material is advisory rather than collective signoff. A read-only software review identified missing rejection of unreported acquisition without HALT, shared-run growth consuming unknown intervals, incorrect adjacent matrix attribution, and startup credit before qualification. Each issue was reproduced with failing tests and fixed by the owner. All source and design credit belongs to jayis1. No task branch was created.

## Executed software verification

Environment: GNU Arm Embedded GCC 14.2.1 (20241119), Python 3.13.5, KiCad 9.0.2, Linux Tk and Xvfb. Python uses only the standard library. Native GCC builds host tests with `-Wall -Wextra -Werror -fsanitize=undefined,address`. Target compilation uses Cortex-M4 Thumb, freestanding C11, warnings as errors, the supplied startup/vector table and linker script.

Commands from repository root:

```sh
make -C aisle-pair/firmware clean all test fixture
xvfb-run -a /usr/bin/python3 -m unittest discover -s aisle-pair/tests -p 'test_*.py'
/usr/bin/python3 -m compileall -q aisle-pair/app aisle-pair/tests
/usr/bin/python3 aisle-pair/tools/check_hardware.py
```

The target links and produces an ELF and binary. Current target size is 2,732 bytes text, zero initialized data and 788 bytes BSS. Stack is additionally reserved by the linker assertion; the size output is not total physical RAM use under execution.

Three native C test executables pass: engine/encoder, production clock/configuration driver, and production nonblocking UART driver. Engine tests cover all nine legal joint stages and all eighty-one ordered stage transitions; raw glitches split evidence; forbidden thermometer codes remain unknown; clock wrap, acquisition-gap freeze, initial configuration fault and startup credit rejection are exercised. Boundary fixtures cover the last permitted acquisition increment, subsequent halt, unknown/invalid accounting at the limit and transmit-counter exhaustion. CRC test vector and output-buffer truncation rejection pass.

Seventeen Python tests pass, including three tests running a real Tk event loop under Xvfb. The real C encoder supplies both the demonstration recording and a dense stress stream over every ordered raw-vector pair, including forbidden inputs and short disturbances. Dense records and sparse subsequences are accepted through the production decoder and session checker. Negative cases include CRC damage, shape/conservation errors, monotonic rollback, impossible halted changes, unacquired evidence, incorrect adjacent attribution, run growth without shared credit, missing acquisition without halt and stale/duplicate traffic.

GUI tests verify matrix and history presentation of the C fixture, private CSV export, serial connection through a Linux pseudo-terminal, advancing acquisition status, duplicate-connect rejection, replay refusal while connected, successful reconnect clearing old values, failed open preserving evidence and actual pseudo-terminal hangup. These are software/transport simulations, not physical USB tests. Replay tests reject FIFO and empty inputs and truncated tails; export refuses overwrite and uses mode 0600. Serial close restores terminal settings in the tested PTY case.

## Electrical design verification

Native KiCad generation and checks pass. ERC reports zero violations. DRC reports zero violations and zero unconnected items. A fresh exported schematic XML netlist is compared with actual PCB pads and the connectivity manifest: fourteen components, thirty pins and ten nets agree exactly. Component values and full footprint library identifiers also match. The board has real routing and an outline, not only footprints or a demonstration drawing.

The schematic was exported to SVG, converted with rsvg-convert and visually inspected. Four labelled threshold channels, supply/input connectors, reference/value fields and pin-level net labels are legible without apparent overlap. Rendering files are scratch artifacts and not manufacturing deliverables. Installed footprint geometry retains upstream licensing. Passive-pin ERC cannot independently verify the externally supplied Nucleo power source or the harness polarity.

Nominal circuit calculation gives 0.300 mA per closed contact, 1.20 mA with all four closed and approximately 0.300 V at each closed sensing node. These calculations do not substitute for measuring the assembled board. The design uses a purchased external NUCLEO-F401RE; the passive carrier contains no MCU, regulator, USB bridge or debugger.

## Content gates

README: 3,299 whitespace-delimited words. Firmware production `.c` files: 565 physical lines, including 506 nonblank lines not starting with a comment marker; headers and tests are additional and excluded from those totals. The shared platform is explicitly identified rather than represented as wholly new code. Every deliverable text file carries jayis1 attribution through comments or format-supported metadata. Build products, bytecode, local KiCad preferences and temporary renderings are excluded from version control.

## Remaining physical work

Not performed: fabrication, assembly, programming, oscilloscope timing measurements, actual ST-LINK USB capture, supply/current measurement, ESD/EMC assessment, cam construction, switch qualification, repeated-cycle testing or participant study. No hardware is connected to the test environment. No anti-tip, geometric clearance, safety, certification or ergonomic-efficacy claim follows from these software and CAD results. Human review and the commissioning sequence in README are required before physical use.

Publication verification is performed after committing and pushing to main; the final task report identifies the exact remote-verified commit. This record intentionally does not invent a future commit identifier or claim publication before the remote has been read back.
