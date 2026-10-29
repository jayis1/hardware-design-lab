# Clamp Relay verification

Author: jayis1

## Scope

This is a standalone-owner design and software verification, not collective hardware review. Advisory reports informed the gates; no peer independently inspected this implementation. The deliverable is an engineering prototype design. No hardware was fabricated, flashed, electrically measured or mechanically calibrated. No force or safe-holding claim is supported.

## Executed checks

- `git pull --ff-only` before implementation: already up to date on main, in jayis1/hardware-design-lab. Initial tree was clean. No task branch was created; daily-ideas was not touched.
- Firmware target: `make -C firmware all test`, Arm GNU GCC 14.2.1. Actual Cortex-M4 ELF and binary linked successfully with warnings treated as errors. ELF header reports ARM, ELF32, EABI5. Image: 2,520 bytes text, zero initialized data, 532 bytes static BSS; stack additionally holds engine and transmit staging buffers.
- C test executables: engine/encoder, clock/configuration, and serial queue all passed under AddressSanitizer and UndefinedBehaviorSanitizer. Engine tests cover both directions, duration oracles over 6–200 ms gap/overlap holds, returned trials, direct swaps, timeouts and recovery, acquisition gaps, independent qualification under chatter, short rejected glitches, material absence, timestamp wrap, terminal faults and counter exhaustion. The randomized loop exercises 50,000 transitions and checks conservation plus serialization on every iteration. CRC uses the standard 123456789 -> 0x29B1 vector.
- Python 3.13.5: `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v`: nine tests passed. The tests consume a synthetic record emitted by the real C encoder. They exercise framing recovery, malformed and CRC-valid impossible records, policy mismatch, counter rollback, duplicate timestamps, prolonged and gradual backlog, regular-file replay restrictions, actual Linux PTY transport and terminal restoration, all three Tk screens, export overwrite refusal, disconnect detection and preserved provenance. PTY and GUI tests are not physical USB tests.
- `python3 -m py_compile app/protocol.py app/relay.py`: passed.
- KiCad 9.0.2: `tools/check_hardware.py` ran schematic ERC and PCB DRC with violation-sensitive exit status. Zero ERC violations, zero DRC violations and zero unconnected items. Exact schematic XML netlist versus PCB versus manifest comparison passed: 14 components, 30 pins, 10 nets, including component values and footprint library identities. No rule waiver was needed.
- Schematic exported as SVG, rasterized and visually inspected: four labeled resistor/contact channels and Nucleo power/input connectors are legible and separated. The custom carrier contains passive circuitry only; the external MCU power source is not validated by passive ERC.
- Documentation and size checks: README has 3,163 whitespace-delimited words. Firmware has 554 nonblank, non-comment C implementation lines, or 650 including headers; tests are excluded. This satisfies the source threshold without counting generated CAD or test code.
- All deliverable source files contain jayis1 attribution. Installed KiCad footprint-library material retains its upstream license; external Nucleo circuitry is not claimed as original hardware.
- Manufacturer pin information was cross-checked against indexed UM1724 revision 17, table 16 and USART section 7.10: CN8 A0/A1/A2/A3 map to PA0/PA1/PA4/PB0; CN6.4 and CN6.6 provide 3V3 and ground; default VCP uses SB13/SB14 on and SB62/SB63 off. Direct PDF retrieval failed at the vendor endpoint; indexed manufacturer content supplied the corroboration. Verify the actual board revision before wiring.

## Design-review boundaries

The carrier and target share one documented pin map. The board support and passive routing are intentionally reused from the existing jayis1 repository platform; the clamp-transfer state machine, protocol, companion and application tests are specific to this device. The companion is receive-only and has no actuator command path. The protocol validates necessary invariants, not physical truth or source authentication. A reset can escape heuristic detection if counters and time appear plausible; no persistent boot identity is provided.

History is a bounded collection of periodic snapshots, not a lossless event journal. Device counters include completed trials that can occur between transmitted snapshots. Current/latest trial metrics freeze after completion but are replaced on the next trial. Raw contact excursions below the five-millisecond qualification interval are intentionally unreported. A direct exclusive-owner swap has unresolved ordering. Neither a completed transfer nor an overlap duration certifies that the specimen was held with sufficient force.

Remaining work is physical: fabricate the passive PCB, assemble and measure the wiring, program a real Nucleo, test watchdog behavior and clock accuracy on target, correlate witnesses with video, quantify mechanical disturbance, and evaluate whether this experiment helps fixture makers. EMC, ESD, enclosure durability, certification and industrial integration are not established. Do not use this design for suspended loads, hazardous machinery or irreplaceable material.
