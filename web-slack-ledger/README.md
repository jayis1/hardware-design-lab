# WebSlack Ledger — differential travel witness for hand-fed strip material

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and design sources: MIT.

Revision A is a build-tested engineering prototype, not a physically validated measuring instrument. The custom PCB is a fully routed passive interface used with a separate NUCLEO-F401RE and two externally powered encoders. It is not a bare-MCU board and must not be mistaken for one.

## Device brief and purpose

A small printmaking, paper-converting, or textile workshop often feeds a strip through a homemade guide, friction pad, or hand-operated laminating fixture. When the operator reverses direction, some material moves at the entrance before equivalent movement appears at the exit. The strip may form a loose loop, compress a compliant roller, slide under a measuring wheel, stretch, or release stored tension. A single length counter cannot show which end moved first or how the differential developed during reversal. Looking at the strip is useful, but it does not leave a repeatable travel record for comparing two guide arrangements.

WebSlack Ledger places one low-force measuring wheel on each side of that fixture. Its particular contribution is a reversal-indexed ledger of differential travel: signed upstream and downstream counts, their difference, the previous motion leg's largest differential excursion, and explicit evidence-quality faults. The companion desktop application displays both rollers and plots apparent accumulation rather than hiding the result behind a binary good/bad alarm. It is intended for slow manual experiments, not unattended production machinery.

Hardware is necessary because the useful signal is physical displacement at two separated material-contact points. A phone camera could sometimes track printed fiducials, but unmarked narrow ribbon, occluded exit paths, or changing illumination make that a different experiment. An individual encoder module supplies only one angular signal and does not provide the synchronized comparison, loss accounting, session export, and calibration workflow here.

The minimum demonstration is deliberately modest: turn both measuring wheels through the same number of quadrature edges and see zero differential; hold the downstream wheel while moving the upstream wheel and see a positive excursion; then reverse and inspect the previous-leg peak. These are bench procedures to perform after assembly, not claims of measurements already made. Workshop fixture designers, teaching laboratories, and makers investigating manual feed mechanisms are the intended contributors. The economic value is faster comparison of guide geometry and roller preload, not replacing a calibrated industrial tension controller.

## Originality assessment and alternatives

The repository was inventoried by device directory and README heading before this design was selected. ContactAtlas maps disconnected rotary-switch signatures, FoldTrace checks a folding-jig operation sequence, LoomMapper identifies harness connectivity, and StitchScope observes sewing-machine stitch formation. This design instead compares two continuous strip-contact displacement histories. Reusing a simple four-input carrier topology does not make those purposes interchangeable; the quadrature engine, physical installation, reversal accounting, protocol, and application are device-specific.

Two relevant established alternatives are Trumeter's 2600 Series material-length measuring assemblies and Kübler's encoder/measuring-system portfolio. The Trumeter product describes length measurement using a material-contact wheel. Kübler provides industrial encoder and measurement components. Neither cited overview is evidence that paired encoder comparison has never been done. This project makes no worldwide patentability or first-invention claim. Its proposed distinction is the small, open, reversible-feed investigation workflow with uncertainty carried alongside the differential result, rather than a new encoder principle.

References:

- https://www.trumeter.com/product/2600-series/
- https://www.kuebler.com/en/
- https://automation.omron.com/en/us/products/family/E6B2-C/e6b2-cwz6c%20100p/r%200.5m
- https://www.ia.omron.com/data_pdf/cat/e6b2-c_ds_e_6_3_csm491.pdf
- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 reference manual RM0368 and the NUCLEO-F401RE MB1136 schematic are the hardware bring-up references.

The product search confirmed the proposed Omron variant as 100 pulses per revolution, NPN open collector, 5–24 V supply, and ABZ output. Direct manufacturer PDF downloads were blocked or timed out during this run. Before purchasing or powering parts, verify the current manufacturer's wiring diagram, output leakage, low-state voltage, supply-current limits, and the exact board revision. Search results are not a substitute for those assembly checks.

## Architecture and system contract

```text
Hand-fed strip -> upstream measuring wheel -> fixture -> downstream wheel
                      | A/B NPN outputs                   | A/B NPN outputs
                      +----------------+------------------+
                                       |
                       4 x 1k series + 10k pull to 3V3
                         custom passive interface PCB
                                       |
                        PA0 PA1 PA4 PB0 on NUCLEO-F401RE
                          1 kHz quadrature state decoder
                       differential / reversal / gap accounting
                                       |
                         PA2 USART2 TX, 115200 baud 8N1
                                       |
                         on-board ST-LINK USB virtual COM
                                       |
                      Linux Python/Tk companion and CSV export

Encoder power: separate regulated current-limited 5 V, common GND.
MCU power: Nucleo USB. Interface VCC: Nucleo 3V3 only.
No actuator, wireless link, battery, charger, or autonomous control path.
```

The processing target is STM32F401RE, Cortex-M4, with 512 KiB flash and 96 KiB RAM on the commercial Nucleo module. Firmware uses the reset HSI clock at 16 MHz, not a guessed external crystal or an unconfigured PLL. An FPGA would add no value at the intended manual-feed sample rate. Connectivity is transmit-only serial through the Nucleo's existing ST-LINK VCP; no USB device stack is falsely implied in the application firmware.

The four interface inputs have these fixed assignments:

| Connector | Pin 1 net | MCU / Arduino header | Meaning |
|---|---|---|---|
| J3 | UP_A | PA0 / A0 through SENSE0 | upstream phase A |
| J4 | UP_B | PA1 / A1 through SENSE1 | upstream phase B |
| J5 | DOWN_A | PA4 / A2 through SENSE2 | downstream phase A |
| J6 | DOWN_B | PB0 / A3 through SENSE3 | downstream phase B |

Every J3–J6 pin 2 is GND. J1 pin 1 is VCC, meaning 3.3 V from the Nucleo; J1 pin 2 is GND. J2 pins 1–4 are SENSE0–SENSE3 in order and connect to A0–A3. Pin 1 pads are square. J1 is not an encoder power input and must never receive 5 V. The Z/index outputs are unused and individually insulated. Do not connect a PNP, push-pull 5 V, differential line-driver, or unknown output variant merely because its cable looks similar.

## Hardware specifications, power, and bill of materials

| Reference | Quantity | Part or specification | Placement |
|---|---:|---|---|
| R1, R3, R5, R7 | 4 | 10 kΩ, 1%, axial DIN0207, 7.62 mm pitch | 3V3 input pull-ups |
| R2, R4, R6, R8 | 4 | 1 kΩ, 1%, axial DIN0207, 7.62 mm pitch | signal series resistors |
| J1, J3, J4, J5, J6 | 5 | 1×2, 2.54 mm vertical through-hole header | power and phase inputs |
| J2 | 1 | 1×4, 2.54 mm vertical through-hole header | MCU input connection |
| External MCU module | 1 | ST NUCLEO-F401RE, MB1136 family | USB power, debug, MCU, VCP |
| External encoders | 2 | Omron E6B2-CWZ6C 100P/R, NPN ABZ | wheel shafts |
| External wheels | 2 | nominal 100 mm effective circumference, 6 mm bore | measuring contacts |
| External supply | 1 | regulated 5 V, adjustable current limit | encoder power only |
| Mechanical hardware | set | brackets, idlers, guards, strain relief | test fixture |

This board contains fourteen actual components and thirty connected pads. The KiCad generator uses standard through-hole resistor/header footprints and locally authored two-pin resistor and connector symbols. Symbols are embedded and also included in a project-local library. It does not contain a pretend MCU symbol hiding an unimplemented power circuit. The Nucleo supplies its own regulation, decoupling, reset, programming connector, and USB interface; those circuits remain on the purchased module.

The board outline is 80 × 70 mm, two copper layers, 1.6 mm nominal FR-4. Routed signals are 0.30 mm wide with a 0.20 mm default clearance. The single through-via is 0.80 mm diameter with a 0.40 mm drill. Through-hole parts are hand-solderable. Use an insulated enclosure tray with edge clips, because mounting holes are not provided. Do not drill through the released copper layout. The carrier does not need RF antenna clearance because it contains no radio.

For engineering budget purposes, reserve 250 mA at 5 V for the two encoders and 200 mA of USB capacity for the Nucleo; these are conservative allocation targets, not measured consumption or substitutes for the supplier rating. Do not take encoder power from the Nucleo 3V3 regulator. Start with a current-limited bench supply and verify each encoder individually. The passive pull network draws at most roughly 1.2 mA total if every channel is low. With a hypothetical 0.4 V encoder low state, the 10k/1k network predicts approximately 0.264 mA sink current and 0.664 V at the MCU input. Actual low and high levels must be measured against the MCU's guaranteed thresholds before use.

Parts budgeting should allow roughly US$10–25 for a small PCB order and passives, US$20–40 for the development board, and substantially more for genuine industrial encoders and mechanics. A pair of genuine encoders may dominate the budget; obtain supplier quotations rather than treating this document as current pricing. Budget US$250–600 for an assembled experiment including mechanics, excluding an existing computer and bench supply. Used or substituted encoders require a new electrical and pulses-per-revolution review.

## Mechanical installation and limits

Use two identical nominal 100 mm circumference wheels, corresponding to approximately 31.831 mm effective diameter, each coupled to an encoder shaft with a suitable 6 mm bore hub. Effective circumference includes the compressed contact coating; machining diameter alone does not calibrate travel. Provide independent bearings or suitably supported wheels so a spring idler does not impose excessive radial load on the encoder bearings. Respect the encoder manufacturer's permissible loads. No dimension in this concept overrides those mechanical limits.

A practical experiment uses a 400 × 120 mm base plate, with wheel contact centers about 250 mm apart and the device under investigation between them. Fit adjustable guides for a 10–30 mm wide paper strip. Each wheel needs a lightly sprung opposing idler, just enough force to prevent obvious loss of contact. Use smooth nonmarking surfaces and shields around pinch points. The mechanical assembly is a documented bench arrangement, not a released machined enclosure or a claim of universal material compatibility.

At 100 pulses per revolution and four quadrature edges per pulse, the nominal scale is 250 micrometres per count. Start below 20 mm/s, approximately 80 edges per second per wheel. Keep encoder leads at or below their short supplied length where practical, and keep the carrier-to-MCU wiring short. Firmware samples at 1 kHz, but sampling frequency is not a guarantee of detecting all overspeed. Multiple transitions between samples can alias to a valid-looking transition or no transition at all. An invalid-transition counter detects some errors, not every error. This is why the device must not be installed on an operating powered production line.

## Firmware behavior and design decisions

`firmware/engine.c` is the portable device-specific algorithm. Each wheel is decoded using the Gray-code sequence 00, 01, 11, 10, 00. One-bit transitions yield signed quarter-cycle counts; unchanged phases yield zero; two-bit jumps increment an invalid counter without inventing direction. Swapping A and B reverses the reported sign. Orient the two wheels so ordinary forward material travel produces positive counts on both, which may require opposite physical rotation because of the fixture geometry.

Signed positions saturate rather than overflowing. Differential subtraction is widened before arithmetic. Edge counters, invalid counters, reversal counts, idle duration, and elapsed leg duration are bounded. A missing one-millisecond acquisition interval increments the gap counter, resets phase baselines to the new observation, and does not infer movement across the missing interval. Position remains a partial record and quality flags remain latched until hardware reset. Continuing to display counts after a fault is useful for diagnosis but does not restore their measurement validity.

An upstream direction change closes the previous motion leg. Its peak is the largest absolute departure of the raw differential from that leg's origin, not peak tension or a calibrated millimetre quantity. The new origin is the differential before both wheels' current sample, so simultaneous matched reversals do not introduce a fictitious edge of slack. Direction is not debounced: a genuine single-edge reversal or a noisy but valid Gray-code reversal can each create a new leg. Use the trace and raw counts rather than treating reversal count as a certification metric.

`main.c` polls acquisition and a nonblocking UART transmitter. A frame is attempted every 100 ms; it is never allowed to block acquisition waiting for a whole line. The driver aborts a transfer that remains pending for 100 ms and exposes a transport error count. The next complete marker allows the app to recover from a truncated line. An independent watchdog uses bounded initialization waits; failure latches the general unusable-data flag. Busy polling is appropriate for this USB bench device. No unimplemented sleep mode or battery-life estimate is advertised.

Startup copies initialized data, clears BSS, and provides Cortex-M core exception vectors. No external peripheral interrupt is enabled. The linker targets the exact flash and RAM regions and reserves stack headroom. The code has no heap, vendor SDK download, RTOS, or hidden generated HAL requirement. Build and host-test commands, run from this device directory, are:

```sh
make -C firmware all test
```

Use an ARM GNU toolchain with `arm-none-eabi-gcc` and a native GCC for tests. The default build produces ELF, map, and binary under ignored `firmware/build/`. Program the binary at flash address 0x08000000 using a compatible ST-LINK programming tool, or use the ELF's embedded addresses. Verify the exact Nucleo solder-bridge routing for PA2 to ST-LINK VCP before expecting serial output. Programming and physical serial timing were not tested in this run.

## Protocol and application interface

Every telemetry record is ASCII:

```text
WSL1,sequence,time,up,down,difference,invalid_up,invalid_down,gaps,reversals,peak,leg_ms,idle_ms,flags,transport_errors*CCCC\n
```

The checksum is CRC-16/CCITT-FALSE: initial value 0xFFFF, polynomial 0x1021, no reflection, no final XOR, covering every byte from W through the last decimal digit before the asterisk. Four uppercase hexadecimal characters follow the asterisk. The known input `123456789` yields 0x29B1. Firmware bounds its output to a 256-byte buffer and the app rejects oversized records. Sequence and millisecond time are unsigned 32-bit wrapping counters. Positions are signed 32-bit, while differential and peak require wider parsing. There are no app-to-device commands, configuration passwords, or remote actuation endpoints.

Flags mean: bit 0 sampling gap, bit 1 upstream invalid transition, bit 2 downstream invalid transition, bit 3 counter saturation or startup-watchdog failure. The app validates flag consistency, numeric ranges, field count, marker version, CRC, and the algebraic differential. It rejects duplicate or backward sequence/time observations using modular differences, disconnecting rather than silently blending sessions. A reset whose counters happen to appear forward cannot be identified with certainty because this revision has no unique boot identifier. Reconnect after any reset and do not combine captures across it.

Run the Linux desktop application with:

```sh
python3 app/ledger.py
```

It uses only Python's standard library plus the operating system's Tk package (`python3-tk` on Debian). The Connect / Replay screen discovers ttyACM and ttyUSB devices, supports an explicitly entered serial path, handles missing devices and permission errors, and offers disconnection. Reconnecting starts a new in-memory history; export first. The Differential travel screen displays the two wheel counts, scaled apparent accumulation, a rolling trace, reversal peak, quality flags, missing frames, bad frames, and transmitter errors. The Calibration / Export screen validates two independent scale factors and creates user-private CSV files without overwriting an existing file.

The app keeps the newest 6,000 records, about ten minutes at nominal cadence; it is not an unlimited archival recorder. Its graph shows the newest 200 records with index-based spacing, not a rigorously uniform time axis when frames are missing. Missing-frame and age indicators are therefore part of interpretation. After one second without a live frame it shows STALE. Replay is explicitly labelled NOT LIVE even when data has just been loaded. Export includes provenance, connection state, age, calibration values, raw fields, and the author. File replay is capped at two megabytes. CRC protects against accidental corruption, not a hostile local sender; serial device permissions are the access boundary.

## Calibration and first experiment

First build and run the synthetic tests without attaching hardware. Then assemble the carrier, inspect soldering, verify ground continuity, and power the Nucleo alone. Confirm J1 polarity and 3.3 V. With encoder outputs disconnected, all SENSE lines should read high. Connect one powered, verified NPN encoder and rotate it slowly while observing each signal with a meter or oscilloscope; confirm that neither signal exceeds the safe input domain and both reach a valid low. Repeat for the other encoder before fitting material.

For travel calibration, move a marked strip through a measured distance with one wheel at a time, recording the signed edge change. Divide reference distance in micrometres by edge count and enter the positive scale. Repeat forward and backward several times. Investigate disagreement rather than averaging away a slipping wheel. The app applies scale changes to the whole retained history, so export separate sessions for separate calibration revisions. Firmware's previous-leg peak remains raw differential counts and cannot be converted with one common scale when wheel scales differ.

For the first paired test, route the strip straight through both contacts without a friction fixture. Equal positive travel should produce a differential near the combined quantization and calibration error. Next introduce a deliberately loose section between rollers and hand-feed only the upstream side. The displayed positive differential should grow. Finally draw out the loose section at the downstream side and compare the recovery trace. These tests establish basic interpretation, not a numerical accuracy specification. Repeat with the intended fixture only after the simple setup behaves sensibly.

## Verification, safety, and open questions

The software requirements map to native tests for forward/reverse motion, equal-wheel cancellation, simultaneous reversal origins, diagonal faults, missed acquisition ticks, timestamp wrap, signed saturation, CRC known vectors, and output capacity. Python tests use the actual C encoder output, fragmented framing, malformed records, history rejection, and counter wrap. The GUI integration test opens a Linux pseudo-terminal, feeds telemetry through the real serial reader, visits all three screens, changes calibration, checks stale/disconnect behavior, replays a capture, and verifies private export and overwrite refusal. Synthetic PTY testing is not physical USB validation.

KiCad verification exports a fresh XML netlist and compares every component value, footprint identifier, reference, and pin/net association against the actual PCB pads and the connectivity manifest. ERC and DRC run with violations treated as failures. Their passing result verifies the authored passive carrier, not the external encoder supply, external harness, MCU module schematic, or mechanical installation. The schematic was also rendered for visual inspection. Exact run results are recorded in `VERIFICATION.md`.

This instrument is not a tension meter, strain gauge, emergency stop, machine interlock, safety controller, or production acceptance gauge. Differential travel cannot uniquely separate wheel slip, material stretch, true stored loop length, backlash, and guide compliance. Stop if material snags or if fingers approach a pinch point. Never connect these headers to mains, existing energized machinery signals, 24 V logic, or an unknown industrial encoder variant. The interface has series resistors but no isolation, surge clamp, reverse-power protection, or industrial ESD qualification. Use only a controlled low-energy bench setup, power down for wiring changes, and verify the common-ground arrangement before connection.

Still unverified are PCB fabrication, soldering, physical MCU programming, encoder electrical margins, worst-case acquisition jitter, EMC behavior, wheel traction, mechanical preload, and repeatable accuracy on real materials. A future revision could use hardware timer encoder interfaces, isolated industrial inputs, a session identifier, and a tension reference to distinguish phenomena. None of those capabilities is represented as present in revision A. The current deliverable is a reproducible, build-tested design and a functional companion tool whose limitations are deliberately visible.
