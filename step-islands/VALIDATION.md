# Step Islands validation record

Author: jayis1

Revision A is an unbuilt engineering prototype. This record distinguishes executed checks from physical work that has not happened. Advisory review was read-only; the owner performed implementation and verification. No full collective approval is claimed.

## Executed checks

- `make -C step-islands/firmware clean all test fixture`: passed. GNU Arm target compile/link with `-Wall -Wextra -Werror`, Cortex-M4 startup and STM32F401 linker map. ELF size: 2788 bytes text, zero initialized data, 1044 bytes BSS. This is a target link, not proof of successful flashing or timing on silicon.
- Four host C test executables passed with address and undefined-behavior sanitizers. Graph checks cover all 65,536 retained sets against an independent all-pairs distance oracle, including component counts, distinct-island bridge masks and shortest-path witnesses. Path tests cover each start with a rotating destination for all sets, plus every endpoint pair for representative sets; they are not an exhaustive enumeration of every path query on every set.
- Acquisition tests passed for all 256 ordered raw-word pairs, the qualification boundary, rejected glitches, gaps, timestamp wrap, checksum vector, encoder capacity, terminal faults and counter exhaustion. Clock tests use simulated registers, and serial tests exercise the production nonblocking driver with simulated status registers; neither test uses actual MCU peripherals.
- `xvfb-run -a python3 -m unittest discover -s step-islands/tests -p test_app.py -v`: eleven tests passed. These include all 65,536 host graph histories against union-find, shortest-path cases, C-encoder replay, 960 dense synthetic snapshots, sparse transition bounds, CRC-valid false summaries, bounded replay inputs, private export and overwrite refusal, real Tk screens, PTY input, reconnect, stale acquisition, disconnect and termios restoration.
- A new partial-live-record regression failed before the fix and passed afterward. Manual disconnect, fault shutdown and window closure share the tail-accounting path. A discarded buffered tail increments parser errors once and leaves a visible warning.
- `/usr/bin/python3 step-islands/tools/generate_hardware.py`: generated fourteen component instances and ten nets with real symbols, footprints and routing.
- `/usr/bin/python3 step-islands/tools/check_hardware.py`: native KiCad 9.0.2 ERC found zero violations; DRC found zero violations and zero unconnected items. Fresh schematic XML netlist, PCB pad connectivity and manifest matched at fourteen references, thirty pins and ten nets, including component values and full footprint identities.
- Exported schematic SVG was rasterized with rsvg-convert and visually inspected. Four repeated pull-up/series/contact channels, supply/sense headers and jayis1 title attribution were visible without net-label collisions. This is schematic inspection, not a fabrication or assembly inspection.
- README word count: 3031. Firmware C/header count: 757 total lines and 686 nonblank lines after C comments were stripped, excluding tests. These counts include shared, explicitly attributed platform code and are acceptance checks, not proof of functional quality.

## Reproduction and dependencies

Build from the repository root using the commands above. Required programs are GNU Arm Embedded GCC/binutils, host GCC with sanitizers, Python 3.10 or later with Tk, Xvfb for headless GUI tests, KiCad 9 CLI and pcbnew Python bindings, and the standard KiCad connector/resistor footprint libraries. The desktop application has no pip or cloud dependencies. `/usr/bin/python3` is used for the system KiCad bindings. The generated fixture is explicitly synthetic and is ignored with other build products.

The manufacturer's primary PDF download failed through the available network path. An indexed university-hosted copy of ST UM1724 corroborated the F401RE connector mapping. Manufacturer register references and external board support are inherited from the pinned platform; this run did not physically audit a purchased board or its solder bridges. A missing optional CairoSVG module did not block the render: the installed rsvg-convert supplied the alternative conversion.

## Remaining physical validation

Not performed: PCB fabrication, soldering, shorts inspection, cable verification, target flashing, measured GPIO thresholds, low-current contact reliability, ESD immunity, oscillator calibration, worst-case sample latency, USB electrical tests, brownout recovery on real hardware, enclosure fit, mechanical contact loading, useful experimental repeatability or user evaluation. No production readiness, certification, worldwide novelty or patent clearance is asserted.

The graph is a model of retained contact words, not a motion trace. Hidden coordinates, approach history, damaged wiring and contact adjustment can make candidate paths physically meaningless. Disconnected graph components are not proof of mechanical impossibility. Bridge candidates are investigation prompts, never commands to force a mechanism. Use only detached, voltage-free, low-energy fixtures and keep the external Nucleo power/debug boundary explicit.
