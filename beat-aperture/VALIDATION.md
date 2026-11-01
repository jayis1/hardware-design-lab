# Beat Aperture validation record

Author: jayis1

## Executed checks

- `python3 tools/verify.py`: passed target build, host tests, Python compilation, desktop/transport tests, CAD checks, authorship and minimum-content gates.
- Target: arm-none-eabi-gcc 14.2.1, Cortex-M4 Thumb, freestanding C11, warnings treated as errors. Linked ELF: 2612 bytes text, zero initialized data, 788 bytes static BSS. Stack is separately reserved by the linker and is not included in that BSS figure.
- Host GCC with AddressSanitizer and UndefinedBehaviorSanitizer: exhaustive 16,384 seven-symbol endpoint words against an independent return/window oracle; coincident returns, zero windows, independent qualification, duplicate time, timestamp wrap, acquisition gap, qualified overlap, timeout, fastest ten-millisecond synthetic return and transactional overflow passed.
- Boundary regressions: return at exactly 30,000 ms rejected; return one sample earlier accepted; B completion on the first A anchor unassigned; tied later A/B return assigned to the closing window.
- Clock and UART production-driver tests: bounded initialization failures, GPIO readback masks, busy rejection, nonblocking draining, timestamp wrap, timeout and recovery passed.
- Python 3.13.5: eight unittest methods passed under Xvfb. Tests use real C-encoder synthetic output, not invented device traffic presented as measurements. Three desktop tabs, replay replacement, history rows, exclusive mode-0600 export, serial pseudo-terminal reception, duplicate-open refusal, termios restoration and hangup were exercised.
- Framing tests cover corruption, truncated prefixes/tails and version framing. Snapshot tests reject impossible offsets and conservation. Sparse-transition regressions reject rewriting a pending event without another B return, extra cycles at exact ten-millisecond boundaries and returning phase without incrementing cycle count.
- KiCad 9.0.2: schematic ERC zero violations; PCB DRC zero violations and zero unconnected items. Exported schematic XML, actual PCB pads and connectivity manifest agree for 14 components, 30 pins and 10 nets, including values and full footprint identities.
- Schematic exported to PDF and rasterized for visual inspection: all four contact rows, power/MCU headers and author/title block are present; no visible label collision in the inspected sheet.
- README measured at 3,200 whitespace-delimited words. Firmware C/header content measured at 610 nonblank, non-comment lines, excluding tests. All original deliverable files contain jayis1 attribution in valid source comments or metadata.
- `git diff --check`: passed before publication. Only the new device folder and its root-index entry are intended publication scope.

## Review and corrections

Read-only implementation review identified three companion transition checks that individually valid snapshots could bypass. Each was reproduced by a failing regression test before correction. The engine policy and CAD were not changed by those corrections. The companion still implements necessary sparse bounds, not a complete reconstruction or proof of every hidden event sequence.

## Reference availability

Direct downloads of the current manufacturer UM1724 and RM0368 PDFs timed out. An accessible university-hosted copy of ST UM1724 Rev 13 was downloaded and its F401RE Table 16 checked against the carrier connector map. Retrieved manufacturer search excerpts corroborated default VCP solder bridges. ST AN4616's published peripheral-base table corroborated the reused F401 register bases. These checks and the target build do not replace review of the full applicable datasheet before physical bring-up.

## Not performed and not claimed

No fabrication, assembly, MCU flashing, electrical measurements, oscilloscope capture, actual reed operation, magnetic perturbation measurement, enclosure/guard validation, timing calibration, EMC test or classroom study was performed. Bench usefulness and worldwide novelty remain unproven. The KiCad PCB is a passive carrier: the external Nucleo, contact harness and guarded pendulum fixture are necessary parts of the design, not missing silicon represented as populated on the carrier.

The synthetic ten-millisecond return is an algorithm boundary test, not a recommended mechanical operating rate. Five sampled observations establish qualification, not immunity to arbitrary bounce. Timestamps may advance during HALT while acquisition remains frozen. USB CRC protects against corruption only; a malicious source can forge an internally consistent history. Reconnect is not a target reset and cannot establish a fresh physical experiment without an intentional baseline procedure.
