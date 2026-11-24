# Touch Tour revision A validation

Author: jayis1

## Scope and evidence

This was a standalone-owner implementation with advisory reports, not a full collective or independent hardware review. The checkout was confirmed as jayis1/hardware-design-lab on configured default branch main, initially clean. `git pull --ff-only` completed before device development. Inventory found 153 existing device README files, and closest coverage/recipe/contact designs were examined. No other repository was modified.

All results below came from actual tool execution. Firmware and fixtures are synthetic where explicitly stated. No physical board was available; no flashing, assembly, switch test or cleaning experiment was performed.

## Firmware

Toolchain: arm-none-eabi-gcc 14.2.1 20241119. Target: STM32F401RET6 Cortex-M4 on external NUCLEO-F401RE. `make -C touch-tour/firmware all test fixture` builds the freestanding ELF and BIN and runs host tests with AddressSanitizer and UndefinedBehaviorSanitizer, `-Wall -Wextra -Werror`.

Target link result: text 2672 bytes, data 0 bytes, BSS 1044 bytes. The linker reserves stack headroom in 96 KiB SRAM. This is a real target link, not only a host compilation. Physical timing and execution are unverified.

The coverage test compares all 65536 eight-visit identity words against an independent set-membership oracle. It checks arbitrary order, repeats, completion rollover, all 256 raw-to-raw pairs, held boot and completion, overlapping raw bits, sequential hand exchange without clear, acquisition gaps, timeout at the exact rejecting boundary and one tick before, timestamp wrap, terminal time faults, counter overflow, CRC reference vector and output-buffer refusal. Clock and UART driver tests cover register masks, bounded clock failures, input configuration, nonblocking output, busy rejection, timeout and wrap.

Tests were first executed before the missing engine implementation and failed as expected. A later failing timeout test was corrected to acquire actual elapsed samples rather than injecting an internally contradictory age. Production engine behavior passed the independent oracle and boundary cases.

## Companion application

Runtime: system Python 3.13.5 and Tk 8.6 on Linux. `xvfb-run -a /usr/bin/python3 -m unittest discover -s touch-tour/tests -p 'test_*.py'` passes nine tests. `python3 -m compileall -q touch-tour/app` passes.

Coverage includes real C-encoder replay and sparse subsequences, framing recovery after truncated input, EOF rejection, CRC-valid impossible accounting, rollback and latched warnings, duplicate receptions not refreshing acquisition, owner-only export with overwrite refusal, FIFO/oversized replay refusal, and non-TTY/missing serial paths. The real Tk event loop is exercised through all three tabs and a Linux pseudo-terminal: connection, batched receive, initial fragments, duplicate-connect refusal, hangup, retained evidence and replay replacement.

The desktop was launched against the synthetic fixture and visually inspected from an actual Xvfb screenshot. It displayed replay / NOT LIVE, two completed rounds, one repeat, two abandoned rounds, the latest A-B-D-C order, and seven missed ticks. Those are generated fixture outcomes, not observations of physical contacts. Serial USB, native desktop integration outside the test display and participant usability remain untested.

## Native electrical CAD

Tool: KiCad 9.0.2 with system pcbnew bindings. `tools/generate_hardware.py` generated the schematic, routed two-layer PCB, project metadata, custom passive symbols and connectivity manifest. `tools/check_hardware.py` ran actual native ERC and DRC, exported a fresh XML netlist, loaded the PCB, and compared every reference, pin, net, value and footprint identifier against the manifest.

Results: zero ERC violations, zero DRC violations, zero unconnected items; 14 components, 30 pins, 10 nets agree across schematic, board and manifest. The schematic was freshly rendered as SVG, converted to PNG and visually inspected: labels and references are separated and all four PAD_A through PAD_D circuits appear. Fresh native Gerber and drill export produced 26 nonempty files in the scratch workspace. These generated check artifacts are not committed manufacturing deliverables; regenerate and review before ordering.

All symbol pins are passive. Consequently a clean ERC is not an independent validation of the external Nucleo supply or switch ratings. The carrier lacks isolation, transient protection and mounting holes. No fabrication readiness, electrical robustness or safety certification is asserted.

## Quantitative and attribution gates

`tools/check_release.py` checks required deliverables, author attribution in every tracked/unignored project file, absence of symlinks, README word count, substantive production C line count, protocol-frame bounds and accidental marker reuse in sibling companions. It excludes comments and blank lines from the firmware count and does not count tests toward production C. Final actual counts are emitted by this check rather than estimated from file sizes.

Every newly authored document and source file credits jayis1. Platform reuse is pinned in the import script and README; upstream KiCad footprint licensing is not reassigned. Generated build products and KiCad preferences are ignored. Only touch-tour paths are intended for the publication commit. No workflow automation, system tree, unrelated device or credential file is included.

## Remaining limitations

The attempted fresh manufacturer PDF retrieval failed with an HTTP/2 connection error and then an HTTP/1.1 timeout. Connector and register assumptions are explicitly inherited from the reviewed platform and cited manual, not represented as freshly downloaded evidence. Check the exact board revision, bridges, supply, switch rating and pin assignment at bring-up.

Firmware keeps no nonvolatile boot identifier or historical archive. Certain resets can resemble valid forward traffic. Sparse host checks are necessary bounds, not a complete proof of hidden transitions. A pad visit does not establish identity, surface coverage, cleaning, inspection quality, force, effort or completed work. Physical assembly, watchdog and oscillator measurements, ESD behavior, cable robustness, ergonomic usefulness and real-world novelty remain open engineering work.
