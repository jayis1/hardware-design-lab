# FoldTrace — folding-jig sequence and dwell recorder

Author and creator: jayis1

Copyright (c) 2026 jayis1

Software and locally authored design sources: MIT

Revision A: build-tested engineering prototype, not physically validated

## Purpose and overview

FoldTrace is a removable four-station witness for a manual carton-folding jig. It records whether a worker moved four folding leaves in the intended order, held each leaf against its stop for a minimum interval, and released it before moving to the next. The immediate user is a small packaging workshop assembling short runs of specialty presentation boxes, where automatic carton machinery is uneconomical and a visually similar wrong fold may only become obvious when the finished box is filled. The device does not move the jig, control a machine, inspect a person's hands, or photograph the work. Its evidence is a sequence of four low-force mechanical contacts.

Today the operator normally follows a printed diagram and checks the finished shape. FoldTrace adds a second, independent source of feedback: the actual travel of the folding leaves. A missed second fold, a repeated first fold, overlapping leaves, or an abbreviated press produces a rejected cycle. The dashboard shows which step was expected and the last qualified dwell durations. This is useful for developing a reliable work instruction, finding awkward jig geometry, and comparing a trial batch before and after a mechanical improvement. It is not a quality certificate for the carton and is not intended for individual employee ranking.

The specific proposed contribution is a portable, camera-free, dwell-aware retrofit for folding leaves, with an explicit all-open rearm interval and invalidation when sampling continuity is lost. That combination is distinct from the devices inventoried in this repository. Worldwide originality is not established: industrial sequencing and error-proofing are mature fields, and this project makes no patentability claim. Its practical hypothesis is that a workshop can discover deliberately omitted or abbreviated folds during assembly rather than during final inspection. A useful pilot acceptance target is detection of every scripted wrong-order trial in a twenty-trial bench set, with no false rejection in twenty correctly executed cycles. Those are proposed experimental criteria, not measured results.

## Why hardware, and what this does not measure

A phone timer cannot observe four separate leaves without an additional sensing fixture or a camera view that may be blocked by hands and cardboard. Switches mounted near the mechanical stops directly measure whether each leaf reached its endpoint. The MCU continues checking timing while the browser is closed, and the serial stream is optional for the state machine. Conversely, FoldTrace does not measure crease pressure, adhesive cure, material thickness, fold angle away from the endpoint, carton presence, or dimensional accuracy. A completed empty-jig cycle can pass. A shorted switch can resemble a permanently operated leaf, and a broken wire can resemble an open leaf. These limitations must be understood before interpreting counters.

The product is deliberately bounded to four leaves operated in order 1, 2, 3, 4. A recipe that needs simultaneous closures or a different sequence requires a reviewed firmware change and new tests. There is no hidden adaptive model, remote provisioning, cloud account, wireless radio, motor output, or persistent production database. This keeps a small workshop tool understandable and prevents a configuration mistake in the browser from silently changing the inspection criteria.

## Prior art and differentiation

[Easy Fold Fixtures](https://www.easyfoldfixtures.com/) markets fixtures that accelerate manual box folding. Those fixtures establish that the mechanical use case is practical; FoldTrace is an instrumentation proposal for observing leaf travel rather than a claim to have invented a box-folding fixture. Compatibility depends on the particular jig and must be designed and checked locally.

[Banner pick-sequence indication and verification](https://www.bannerengineering.com/us/en/solutions/pick-to-light/pick-sequence-indication-and-pick-verification.html) describes sensors and a process controller guiding sequential part picks. That is close conceptual prior art for sequence checking. FoldTrace instead watches folding-stop contacts, requires a minimum endpoint dwell, and uses a small USB-connected recorder rather than pick-bin lights. It does not claim equivalent industrial protection or reliability.

A generic PLC with four digital inputs can implement the same state machine. FoldTrace's intended advantage is the explicit open-source inspection semantics, cheap dry-contact carrier, reproducible firmware, and local evidence display for a very small fixture. If a workshop already has a properly integrated PLC, using that existing system may be the better engineering decision.

## Hardware specifications and design contract

| Item | Revision A specification |
|---|---|
| MCU module | NUCLEO-F401RE, MB1136, STM32F401RET6, LQFP64 |
| CPU and memory | Cortex-M4, reset HSI clock 16 MHz; 512 KiB flash, 96 KiB SRAM |
| Sensing | Four normally-open voltage-free switches, preferably gold low-level contacts |
| Example contact | Omron D2F-01L lever switch, verify mechanical travel and procurement variant |
| Carrier | 80 × 70 mm, two copper layers, 1.6 mm FR-4, through-hole assembly |
| Carrier electronics | Four 10 kΩ pull-ups and four 1 kΩ series resistors |
| Communication | USART2 TX on PA2 through the Nucleo ST-LINK virtual COM port |
| Link settings | 115200 baud, eight data bits, no parity, one stop bit |
| Power | Nucleo USB supply; carrier takes regulated 3.3 V and common ground |
| Timing | Nominal 1 ms sampling, 20 ms debounce, 500 ms minimum dwell |
| Limits | 30 s cycle timeout, 1 s all-open rearm, >5 ms sampling gap invalidates active work |
| Outputs | Serial telemetry only; no mechanical control output |

The MCU, USB interface, clock support, programming interface, and regulator are on the purchased Nucleo board, not duplicated on the passive carrier. The supplied KiCad design is the complete carrier, not a fabricated claim to be a standalone MCU board. It uses six headers, eight resistors, actual footprint pads, routed tracks, a ground via, and a closed rectangular outline. J1 and J2 connect by short jumpers to the Nucleo; this is not a mechanically mating Arduino shield. Secure the modules independently inside an enclosure so the cable cannot carry the carrier's weight.

### Exact electrical mapping

| Carrier terminal | Electrical destination |
|---|---|
| J1 pin 1 VCC | Nucleo regulated 3V3 output, CN6 pin 4 |
| J1 pin 2 GND | Nucleo GND, for example CN6 pin 6 |
| J2 pin 1 SENSE0 | Nucleo A0, CN8 pin 1, PA0 |
| J2 pin 2 SENSE1 | Nucleo A1, CN8 pin 2, PA1 |
| J2 pin 3 SENSE2 | Nucleo A2, CN8 pin 3, PA4 |
| J2 pin 4 SENSE3 | Nucleo A3, CN8 pin 4, PB0 |
| J3 / J4 / J5 / J6 pin 1 | CONTACT0 / CONTACT1 / CONTACT2 / CONTACT3 |
| J3 / J4 / J5 / J6 pin 2 | GND, the other terminal of the associated switch |

Each SENSE net has a 10 kΩ pull-up to VCC. Its 1 kΩ series resistor goes to the corresponding CONTACT net. Closing the switch connects CONTACT to ground. At nominal 3.3 V, the closed input is approximately 0.30 V and the contact current approximately 0.30 mA. Four simultaneously closed contacts draw approximately 1.20 mA, although normal operation permits only one qualified closure at a time. Use contacts appropriate for this low wetting current; a large oxidized power switch is a poor substitute. The resistors are not an isolation barrier, transient suppressor, or permission to connect external voltage.

## Architecture and block diagram

```text
Four folding leaves with insulated stop brackets
        | four normally-open, voltage-free switches
        v
J3..J6 -- 1k series -- SENSE0..3 -- J2 -- PA0/PA1/PA4/PB0
                         |
                      10k pulls
                         |
USB --> NUCLEO-F401RE 3V3 -- J1 VCC/GND
          |
          +-- 1 ms sample --> debounce --> sequence/dwell state machine
          |                                  |
          |                            counters + last dwell
          +-- USART2 PA2 --> ST-LINK VCP --> browser Web Serial
                                               |
                                  Live / History / Connection screens
```

The carrier contains no energy storage beyond incidental capacitance. All inputs remain inputs during normal operation, reset, and faults. Firmware drives only the UART transmitter. The board does not interlock with a press, enable an actuator, or stop an operator. Losing power loses the current evidence session; it must never prevent the mechanical jig from returning freely to its resting position.

## Mechanical integration

Use four low-force lever switches on adjustable insulating brackets mounted to a stationary jig base. Each moving leaf should reach its normal mechanical stop before the bracket permits excessive switch overtravel. The stop must carry folding force, not the switch body. Start with a slotted bracket permitting approximately 5 mm adjustment, two fasteners, and a locking washer or thread-retention method compatible with the jig material. Route a paired signal and ground cable along the fixed structure, not across a hand-clearance opening. Provide strain relief beside each switch and at the carrier enclosure. Keep cable lengths below approximately 0.5 m for the first bench prototype; longer unshielded cables need a redesigned protection and EMC strategy.

A prototype enclosure should provide at least 90 × 80 mm usable space for the carrier plus separate clearance for the Nucleo and USB bend radius. The board has no mounting holes in revision A: use insulated edge supports or a correctly sized PCB tray, not improvised screws through copper. Label stations 1 through 4 both on the brackets and on their cables. Confirm order by operating one disconnected switch at a time before folding material. Avoid pinch points, sharp bracket edges, exposed solder joints, and cables that could snag a sleeve. Mechanical fixture suitability remains a physical validation task.

## Firmware details and decisions

The firmware is plain C11 and has no vendor HAL, RTOS, allocator, radio stack, or dependency download. `registers.h` names the specific STM32F401 register addresses used by the board driver. Register references are the [STM32F401 RM0368 reference manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) and the [MB1136 UM1724 user manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf). Check the actual board revision, connector numbering, and ST-LINK USART solder bridges against that manual before wiring. Preserve the default ST-LINK VCP routing on PA2; do not use an external UART transmitter on this net.

`startup.c` installs core exception vectors, initializes RAM, and enters `main`. No external interrupt is enabled. `drivers/board.c` enables GPIOA, GPIOB, and USART2 at the reset HSI clock and configures SysTick. `drivers/serial.c` owns a fixed-size transmit buffer and sends at most one byte per loop visit. Transmission is nonblocking and expires after 100 ms rather than trapping the sampling loop. A transport-loss counter makes buffer rejection and transmit timeout observable. Link loss does not stop local inspection.

`engine.c` is independent of MCU registers and receives explicit timestamped contact masks. Each contact has a candidate state, qualification time, and stable state. A change must remain present for 20 ms before entering the sequence engine. Mechanical chatter shorter than the qualification interval is ignored. This is a stable-state measurement, not a precise force-integral measurement. In particular, short excursions below the debounce interval do not subtract from the dwell interval. Nominal timing accuracy is limited by HSI tolerance, switch repeatability, scheduler continuity, and millisecond quantization.

At startup, CLEAR requires all raw and qualified contacts open for one second. READY waits for station 1. HOLD checks that the current station remains the only qualified closed input; its release records the dwell and advances to RELEASE. RELEASE waits for the next numbered station. After station 4 releases with adequate dwell, the good counter increments and the engine returns to CLEAR. Wrong order, overlapping qualified contacts, short hold, timeout, or a sampling gap during active work produces REJECT and increments bad exactly once. The reason persists until all-open rearming completes. Counts saturate rather than wrap, while the timestamp uses unsigned subtraction for ordinary timer rollover.

A scheduling delay longer than 5 ms resets debounce qualification. During an active cycle it rejects that cycle because the missing interval cannot prove continued contact. During READY it forces CLEAR again. A stuck closed switch prevents rearming; a stuck open switch eventually produces a timeout if a cycle has begun, but cannot be distinguished from an absent action before a cycle begins. No diagnostic claims are made for disconnected wires. A nominal roughly two-second independent watchdog is enabled before sampling. If watchdog startup fails, inspection never starts and telemetry remains absent. The browser must treat absent data as unknown, not as a pass.

## Wire protocol and application interface

Every 100 ms the firmware attempts one ASCII status line. The payload is `FT1` followed by fifteen comma-separated unsigned decimal fields, then `*`, four uppercase hexadecimal CRC digits, and LF. CRC-16/CCITT-FALSE covers the payload before the asterisk, with polynomial 0x1021 and initial value 0xffff. Its standard check value for `123456789` is 0x29b1. CRC detects corruption; it does not authenticate a device or make the link suitable for adversarial environments.

Field order is sequence, uptime_ms, state, reason, step, closed_mask, good, bad, sample_gaps, last_cycle_ms, dwell0_ms, dwell1_ms, dwell2_ms, dwell3_ms, tx_loss. States are CLEAR=0, READY=1, HOLD=2, RELEASE=3, REJECT=4. Reasons are NONE=0, ORDER=1, OVERLAP=2, SHORT_HOLD=3, TIMEOUT=4, SAMPLE_GAP=5. The step is zero-based and becomes four after successful completion. Dwell values describe completed holds in the current or most recently completed cycle, not live durations. Counters and all state are volatile and restart at zero after reset.

The companion is a dependency-free browser application rather than a mobile framework. A workstation already beside the jig can display large state cards without a native installation. Its three screens are Live jig, History, and Connection and recipe. Live shows contacts, completed dwells, counts, reason, and data-loss indicators. History retains at most one thousand status snapshots and exports CSV. Connection requests an explicitly selected Web Serial port, explains the fixed recipe, and stores an optional station-label prefix locally. Labels are inserted as text, never HTML.

Run `python3 -m http.server 8766 --bind 127.0.0.1 --directory fold-trace/app` from the repository root and visit `http://127.0.0.1:8766` in a desktop Chromium browser supporting Web Serial. A secure context and explicit user permission are required. If serial permission is denied or the API is unavailable, the UI reports it. Telemetry older than one second is marked not live. Disconnecting preserves old values for inspection but removes live status. There is no background reconnection to an arbitrary serial port.

The app rejects malformed, oversized, out-of-range, and CRC-invalid frames. The streaming parser drops oversized lines until LF and then resumes without unbounded buffering. Duplicate or backwards sequence records are rejected, except that an uptime rollback conservatively starts a new inferred epoch. This is a heuristic: uptime rollover or a lost boot frame cannot be positively identified without a persistent boot identifier. Exports include epoch, receive timestamp, device uptime, and source mode so downstream analysis can preserve those boundaries. The history is not a lossless event log: a complete fast state transition may occur between status frames, although counters retain its result until reset.

## Build, test, flash, and reproducibility

Install an Arm bare-metal GCC toolchain, GNU make, host GCC, Node.js, Python 3, and KiCad 9 with its Python bindings and footprint libraries. No npm installation is necessary because the app uses only browser APIs and Node's built-in test runner. From the repository root run:

```sh
make -C fold-trace/firmware clean all test
node --test fold-trace/tests/*.test.mjs
node --check fold-trace/app/app.mjs
python3 fold-trace/tools/check_hardware.py
```

The first command links an actual Cortex-M4 ELF and generates a binary; it also runs the portable engine and encoder under host address and undefined-behavior sanitizers. `tools/generate_hardware.py` regenerates the carrier from its source contract when a deliberate layout change is needed. `tools/check_hardware.py` runs actual KiCad ERC and DRC, exports a fresh schematic XML netlist, and compares reference, value, footprint identity, and pin-to-net mappings with the PCB and connectivity manifest. A clean passive-board ERC does not validate the external Nucleo, wiring, or switch specifications.

Program `firmware/build/fold-trace.elf` through the Nucleo's ST-LINK SWD using a compatible programmer, with address information from the ELF. Verify the flash and reset the target. Programming has not been exercised on physical hardware for this revision. There is no custom bootloader, OTA update, signing scheme, or rollback storage. Recovery is an explicit trusted local SWD reflash; interrupted programming requires repeating that procedure. Never configure the instrument as a safety dependency during firmware update.

The test fixture in `tests/fixture.c` generates synthetic contact sequences using the same engine and encoder as the target. Compile it with host GCC together with `engine.c` and `wire.c`, then redirect its output to a scratch `.log` file. Select that file in the app's replay input. Replay is visibly labeled SYNTHETIC REPLAY and never gains live status. This exercises file handling and cross-language protocol compatibility without claiming a real USB instrument was connected. Disconnect live serial before starting replay; opening a new connection or replay clears the in-memory history.

## Power, BOM, and deployment cost

The carrier's calculated worst-case static switch current is 1.20 mA at 3.3 V; its nominal power is therefore about 3.96 mW. The Nucleo and ST-LINK dominate consumption. Reserve a conservative 100 mA at 5 V USB supply budget for initial planning, but measure the actual assembled board before specifying a product power limit. This is not a battery-operated design, and no invented battery runtime is promised. There is no charging circuit or lithium cell in the carrier.

Budgetary, not supplier-quoted, prototype costs are approximately USD 15–30 for the Nucleo, USD 8–20 for four suitable low-level switches, USD 3–8 for headers and resistors, USD 5–15 allocated to a small PCB batch, and USD 10–25 for enclosure, brackets, and wiring. Mechanical integration can cost more than the electronics. The PCB assembly list is J1 and J3–J6 as five two-pin 2.54 mm headers, J2 as one four-pin header, R1/R3/R5/R7 as 10 kΩ resistors, and R2/R4/R6/R8 as 1 kΩ resistors. Use quarter-watt axial parts with the specified 7.62 mm lead pitch; their dissipation is comfortably below rating at the intended voltage.

## Safety, privacy, use cases, and validation limits

Only connect disconnected, voltage-free switches. No mains, 24 V industrial sensor, powered alarm contact, or building wiring may be attached. Ground is shared with the USB host. There is no galvanic isolation, TVS array, reverse-power protection, or industrial ESD qualification on this carrier. Keep it in a dry indoor enclosure and start on an isolated bench fixture. A separate industrial-input design is required for electrically noisy machinery or externally powered sensors.

Suitable audiences include carton prototyping shops, makerspaces teaching repeatable fixture design, packaging engineers comparing fold order, and small manufacturers validating a work instruction. Unsuitable applications include medical packaging release, machine guarding, personnel tracking, critical bond inspection, or replacing final product quality checks. No camera or microphone is present. Telemetry stays in the local browser until an explicit CSV export; optional local storage contains only the label prefix. Exported process records should be handled according to the workshop's agreed retention and privacy policy.

Before a real pilot, check resistance and continuity unpowered, inspect polarity, then power from the Nucleo and measure VCC and each open/closed SENSE voltage. Exercise each input alone and confirm displayed numbering. Deliberately reverse two leads, hold two switches, pulse briefly, leave a switch closed, remove a lead, restart power during a cycle, disconnect USB, and induce a controlled scheduling gap in a test build. Verify that unknown conditions never look like fresh acceptance. Finally fold real cartons and compare against independent inspection, adjusting mechanical stops rather than hiding inconsistent contact behavior in software.

Automated build and CAD results are recorded in VERIFICATION.md. Fabrication, electrical bring-up, switch life, calibration, real USB behavior, fixture ergonomics, EMC, and field reliability remain unverified. These limitations are not waived by a successful compiler or a zero-violation board report. FoldTrace is a concrete, reproducible design ready for supervised prototyping, not a claim of a tested commercial product.
