# Sleeve Order revision A validation

Author: jayis1

Executed on 2026-11-22 UTC. This is software/CAD evidence for an unbuilt prototype, not physical device validation.

## Recovery and scope

The pre-existing untracked sleeve-order directory was traced to an interrupted execution of the same authorized hardware task. Its original firmware, transport and CAD were inspected and rebuilt rather than treated as verified output. This recovery adds the missing desktop workbench and documentation, adds UI and semantic regression coverage, and fixes acceptance of implausible latest-result duration and disappearing sticky fault bits. No unrelated device files were changed. The repository is jayis1/hardware-design-lab; daily-ideas was not used.

## Executed gates

- GNU Arm Embedded GCC 14.2.1: `make -C firmware clean all test fixture` passed. Target is STM32F401RE Cortex-M4, freestanding C11. Linked image: text 2460 bytes, data 0 bytes, BSS 788 bytes; stack is separately reserved by the linker. ELF and BIN are generated and intentionally ignored, not committed.
- Host GCC 14.2.0: all three C test executables passed with AddressSanitizer and UndefinedBehaviorSanitizer. Engine coverage includes nested and outer-only cycles, repeated inner trips, invalid departures, raw guards, gaps, timestamp wrap, rejecting timeout and transactional exhaustion. Clock tests cover register masks and bounded failure paths; UART tests cover bounded buffers, busy rejection, drain, wrap and timeout recovery.
- Python 3.13.5: thirteen unittest tests passed under Xvfb with the real Tk event loop. Coverage includes C-encoder replay, every ordered sparse pair from that fixture, invalid fields, CRC and tail failures, duplicates/reset suspicion, reception pause, private export, duration budget, sticky faults, no-acquisition completion rejection, replay screens, serial ownership, reconnect, real PTY hangup and failed-open evidence preservation.
- GUI tests consume 144 synthetic frames from the production C engine/encoder. The final fixture contains two completed outer cycles, one nested completion, one inner round trip and one aborted outer cycle. These are test inputs and outputs, not physical measurements.
- The missing workbench initially failed its import test. Two additional semantic regressions failed before their model fixes. All subsequently passed in the final clean verification run.
- `python3 -m compileall` passed for app, tests and tools.
- KiCad CLI 9.0.2 ERC: zero violations. PCB DRC: zero violations, zero unconnected items. No rule exclusions were introduced.
- Fresh exported XML netlist compared with actual pcbnew pads and the manifest: fourteen components, thirty pins, ten nets; exact reference/pin/net, value and footprint correspondence passed.
- Schematic SVG exported successfully and rendered to PNG for visual inspection: component values, references, labels, connections and jayis1 title-block attribution were legible.
- README: 3388 whitespace-delimited words. Production firmware `.c` files: 597 total lines, 553 nonblank lines after removing comments, excluding tests and headers. Every deliverable text file includes jayis1 attribution in a format-valid header or metadata field. Generated build outputs, bytecode and KiCad local preference files are ignored.
- A bounded read-only code review found no actionable blockers in the documented contract. This was not full collective review, exhaustive formal verification or a physical inspection.

## Reproduction

Run from this device directory:

```sh
make -C firmware clean all test fixture
xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
/usr/bin/python3 -m compileall -q app tests tools
```

The firmware has no external SDK dependency. The app uses only the Python standard library and the platform Tk installation. KiCad footprint libraries are required to regenerate the board; native project files already contain their instantiated geometry. Author attribution applies to original work and does not replace upstream tool or footprint licensing.

## Not performed

No target was flashed. No PCB was fabricated or powered. No electrical current, GPIO threshold, oscillator accuracy, USB timing, watchdog recovery, contact bounce, mechanical trajectory, switch force or specimen behavior was physically measured. PCB ERC cannot establish the external Nucleo supply or connector wiring. No EMC, ESD, environmental, endurance, compliance, safety or commercial-uniqueness validation is claimed. The mechanical fixture is a documented proposed assembly, not released production CAD. Follow the README bring-up gates before any supervised bench experiment.
