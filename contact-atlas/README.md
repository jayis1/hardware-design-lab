# ContactAtlas — detached rotary-switch transition cartographer

Author and creator: jayis1

Copyright (c) 2026 jayis1. Software and locally authored design sources: MIT.

Revision A: engineering prototype. Target firmware and CAD are locally verifiable; physical fabrication, programming and electrical performance remain unverified.

## Purpose and overview

ContactAtlas helps a repair technician recover the electrical behavior of an undocumented, disconnected rotary selector. Instead of asking a multimeter whether one pair of terminals is connected at one moment, it watches four contact paths simultaneously. It discovers stable four-bit signatures, records transitions between those signatures, and measures how much of each transition window contained no closed contacts or multiple closed contacts. The result is a directed map that can expose a missing connection, a brief return to the previous signature, or a different path when the knob is turned in the opposite direction.

The primary user is a repair-cafe technician restoring low-voltage audio equipment, educational apparatus, or an unpowered laboratory selector whose original wiring diagram is missing. The recurring difficulty is keeping handwritten continuity notes synchronized with knob position while holding probes and turning a shaft. A secondary user is an electronics teacher demonstrating why the static truth table of a switch is not a complete description of its switching behavior. ContactAtlas preserves observations without filming people or requiring a cloud service.

The proposed original contribution is a portable signature-to-signature map with explicit sample-gap invalidation, directional transition statistics, and a local evidence viewer. It is not a claim to have invented continuity testing, contact monitoring, or rotary selectors. It is also not a general-purpose development board: its carrier, firmware state model, protocol, and screens are narrowly focused on documenting detached mechanical switching behavior.

A useful success criterion for a first physical prototype is reproducing a known switch's stable signatures over ten clockwise and ten counterclockwise sweeps, while preserving deliberately introduced self-return excursions. A second criterion is that unplugging a signal or interrupting sampling never produces a falsely continuous transition. These are proposed acceptance experiments, not measurements already performed. Millisecond-scale timing should be checked against an independent logic analyzer before any measured interval is used in a repair decision.

## Prior art, alternatives, and originality limits

The repository inventory was checked before this folder was created. FoldTrace is the closest electrical platform: it observes four folding-jig stops and rejects wrong sequences or insufficient dwell. ContactAtlas instead learns a directed graph of arbitrary contact signatures, accepts multiple closed contacts, retains self-edges, and has no prescribed folding recipe or pass/fail production counter. LoomMapper concerns harness connectivity, while HexaScope is a general mixed-signal instrument. None of those performs this specific detached-selector mapping workflow. Reusing jayis1's passive carrier and STM32 startup code is intentional; originality resides in the application and evidence semantics, not in pretending a resistor network is new.

A limited external search for “rotary switch contact truth table detent transition overlap tester repair” located [ShieldHz's rotary cam switch explanation](https://shieldhz.com/what-is-rotary-cam-switch/) and [Anssin's rotary switch repair guide](https://www.anssin.com/how-to-repair-a-rotary-switch/). Search results explicitly mention contact truth tables, detent positions, switching sequences, continuity tests and overlap behavior. This establishes close conceptual prior art. The search does not establish worldwide novelty or patentability, and no assertion is made that a similar maker instrument cannot exist. The proposed distinction is the compact, inspectable, gap-aware transition-map implementation provided here.

A continuity multimeter is still the cheapest choice for a simple static check. A logic analyzer can capture much faster transitions, but usually needs suitable pull-ups, a fixture, decoding and interpretation. A commercial switch tester may offer calibrated resistance and durability testing that this prototype cannot match. ContactAtlas is useful only when an inexpensive local map is more convenient than configuring those alternatives. It deliberately does not replace a four-wire milliohm measurement, insulation tester, high-speed contact-bounce analyzer, or manufacturer-approved service procedure.

## Hardware specifications

| Item | Revision A contract |
|---|---|
| Controller | Purchased NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Processing | Cortex-M4 at reset HSI 16 MHz; 512 KiB flash, 96 KiB SRAM |
| Inputs | Four detached voltage-free contact paths, active low |
| Carrier | 80 × 70 mm, two-layer 1.6 mm FR-4, through-hole parts |
| Conditioning | Four 10 kΩ pull-ups, four 1 kΩ series resistors |
| Communications | USART2 PA2 to ST-LINK virtual COM; 115200 baud, 8N1 |
| Power | USB into Nucleo; regulated 3.3 V and ground to carrier |
| Sampling | Nominal 1 ms; every missed tick invalidates continuity |
| Qualification | A signature must remain unchanged for 200 ms |
| Outputs | Telemetry only; no relay, motor, stimulus sequencer, or interlock |
| Storage | Volatile RAM; sixteen queued transition events |

The supplied KiCad files describe the complete passive carrier, not a standalone MCU motherboard. The purchased Nucleo supplies the microcontroller, regulator, decoupling, SWD programmer, USB interface, reset circuit and oscillator support. These components must not be inferred to exist on the carrier schematic. The carrier connects through short jumpers rather than mechanically mating as an Arduino shield. Its six connectors and eight resistors have actual symbols, library footprint identities, pad assignments, routed copper and a closed board outline.

### Pin allocation and wiring

| Carrier connection | Destination |
|---|---|
| J1 pin 1, VCC | Nucleo CN6 pin 4, regulated 3V3 |
| J1 pin 2, GND | Nucleo CN6 pin 6, ground |
| J2 pin 1, SENSE0 | CN8 pin 1, A0 / PA0 |
| J2 pin 2, SENSE1 | CN8 pin 2, A1 / PA1 |
| J2 pin 3, SENSE2 | CN8 pin 3, A2 / PA4 |
| J2 pin 4, SENSE3 | CN8 pin 4, A3 / PB0 |
| J3 through J6 pin 1 | CONTACT0 through CONTACT3 |
| J3 through J6 pin 2 | Common ground return |

Connect each inspected contact between its CONTACT terminal and ground. A single-pole four-throw switch can use the common terminal as ground and the four throws as CONTACT0 through CONTACT3. Four independent contact pairs may also be observed, but grounding one side of every pair deliberately joins those sides. That changes the topology of some multi-pole switches. Review the disconnected device's terminal arrangement before wiring; this fixture cannot identify arbitrary terminal networks automatically. Do not connect other circuitry to the switch during testing.

Each path is VCC → 10 kΩ → SENSE → 1 kΩ → CONTACT → switch → GND. The nominal closed input is approximately 0.30 V at a current of 0.30 mA. Four closed paths draw approximately 1.20 mA, or 3.96 mW from 3.3 V. This low wetting current suits clean low-level contacts but may fail to penetrate oxide on power-switch contacts. A failure to register is evidence about this test condition, not proof that the switch cannot carry its rated load. The series resistor is neither galvanic isolation nor adequate protection from external voltage.

The register and connector references are ST's [RM0368 STM32F401 manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) and [UM1724 MB1136 manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf). Direct downloads failed during this run; the design reuses the repository's documented F401/MB1136 platform and does not claim a fresh manufacturer-document audit. Before assembly, verify the actual board revision, connector orientations, reset clock, and default ST-LINK USART solder bridges against the manufacturer manuals. Only TX is configured; no command receiver is enabled.

## Architecture and block diagram

```text
Detached selector common ------------------------------- GND
Four throws --> J3..J6 --> 1k --> SENSE0..3 --> J2 --> GPIO
                                  |
                                10k
                                  |
USB --> NUCLEO-F401RE 3V3 -------- J1
          |
          +--> SysTick 1 ms --> raw signature tracking
          |                         |
          |                200 ms stable qualification
          |                         |
          |                nodes + directed edges + queue
          |                         |
          +--> USART2 / ST-LINK VCP --> Web Serial companion
                                          |
                             Live / Transition map / Export
```

The device observes contacts and does not control the equipment being repaired. During reset or power failure the external pull-ups remain passive; there are no actuator outputs to leave energized. The serial transmitter is the only actively driven application signal. The USB host and switch fixture share ground. This is a dry indoor bench instrument, not a field probe for installed electrical panels.

## Firmware details and design decisions

The firmware is self-contained C11. The Makefile builds and links an actual Cortex-M4 ELF with a startup vector table and linker script, then creates a binary. There is no vendor HAL download, dynamic allocator, RTOS, network stack, or opaque generated application layer. `board.h` defines the wiring contract and buffer limits; `registers.h` defines the addresses used by the GPIO, RCC, USART, watchdog and SysTick drivers. Board initialization assumes the MCU's hardware-reset HSI configuration rather than inheriting an arbitrary bootloader clock state.

`engine.c` is hardware-independent. Its input is a timestamp and a four-bit mask in which one means closed. There are sixteen possible signatures, including all-open and all-closed. A signature is not a physical shaft-position identifier: two detents with the same connectivity collapse into one graph node. The system also cannot distinguish a long pause between detents from an intended detent. The operator should move deliberately and hold each intended position for at least a quarter-second, while noting the physical knob markings separately.

At startup the engine is unknown. It seeds a raw candidate, then requires 200 consecutive milliseconds without a change before accepting the first node. That first acceptance does not invent a transition from zero. Once a node is known, the first raw change starts a journey. Every subsequent raw change increments the change count and restarts candidate qualification. When the candidate finally remains unchanged for 200 ms, the journey ends and a directed edge from the previous stable signature to the new signature is recorded. A return to the original signature is retained as a self-edge, because such an excursion may be diagnostically useful.

Timing uses left-hand sample intervals. The state sampled at the start of an observed millisecond is charged for that interval. Journey duration includes the final 200 ms qualification window. Zero time counts journey intervals whose raw mask is zero; multi time counts intervals with two or more closed bits. Therefore a final all-open detent contributes its qualification time to zero time, and a final multi-contact detent contributes its qualification time to multi time. These values are not automatically break-before-make dead time or forbidden overlap. Interpretation requires knowledge of which contacts should connect in the endpoint states. The UI labels them literally to avoid a false safety conclusion.

Node visits and dwell, and edge visits, minimum/maximum duration, zero time and multi time are accumulated in RAM. Those full lifetime tables are available to a debugger, not exported as a snapshot protocol. The companion reconstructs its map from received event records. All counters use saturating addition; timestamps use unsigned subtraction for ordinary rollover. A gap greater than one millisecond abandons the active journey and marks the current signature unknown. Historical evidence remains, but no synthetic edge bridges the missing interval. Qualification starts again from the current raw sample. Invalid masks are rejected and counted rather than silently truncated.

A sixteen-entry ring buffer preserves event order. If full, the newest event is dropped and a visible counter increments. The nonblocking UART sends one byte per poll; a 100 ms transmit timeout prevents the transport from trapping the main loop. Firmware sends status every 100 ms and drains events between status records. A watchdog is enabled before inspection begins and serviced only after sampling progress. Watchdog startup failure produces no telemetry. There is no claim that software can distinguish every broken wire from an open contact, or every shorted wire from a closed contact.

## Protocol and application interface

Every line contains ASCII payload, an asterisk, four uppercase hexadecimal CRC digits, and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xffff over the payload before the asterisk. The standard check string `123456789` yields 0x29b1. CRC provides accidental-corruption detection, not authentication. There are no incoming commands or remote configuration writes.

Status payload is `CA1,S` followed by these twelve unsigned decimal fields: sequence, uptime_ms, known, travelling, raw_mask, stable_mask, transitions, sample_gaps, invalid_samples, queue_dropped, tx_loss, candidate_age_ms. Event payload is `CA1,E` followed by nine fields: sequence, completion_uptime_ms, event_number, from_mask, to_mask, duration_ms, zero_ms, multi_ms, raw_changes. Sequence is shared by both record types and wraps modulo 2^32. Event timestamps refer to completion and can precede a later status if buffered; they must not be interpreted as boot indicators.

The application is a dependency-free local browser app with three screens. Live contacts shows raw and qualified masks, movement, qualification age, totals, loss counters and status freshness. Transition map aggregates the latest thousand retained events into directed signature pairs with count, minimum/maximum journey time and accumulated zero/multi time. Connection and export provides explicit port selection, disconnect, synthetic-log replay and CSV download. No user-provided text is inserted as HTML. No cloud endpoint, account, password or analytics service is involved.

Run `python3 -m http.server 8766 --bind 127.0.0.1 --directory contact-atlas/app` from the repository root, then open `http://127.0.0.1:8766` in desktop Chromium with Web Serial support. The API requires a secure context and explicit user permission. If unavailable, the app reports it rather than pretending to connect. Port denial, read failure, cancellation and closure are handled. A pending browser chooser cannot be forcibly dismissed by JavaScript; cancellation displays a waiting message and prevents a second operation until the first settles. Generation tokens prevent a canceled open or file read from reviving a session.

Status older than one second is not live. Replay is always marked SYNTHETIC REPLAY, even when the file contains plausible timestamps. The parser enforces bounded line length, CRC, field count and integer ranges. A complete `CA1,` marker resynchronizes an interrupted short frame; overlong garbage is dropped until newline. Sequence regressions or implausible status-time regressions freeze acceptance and require export followed by an explicit reconnect. Existing evidence is preserved, not silently erased as if a reboot were confirmed. Exact duplicate sequence records are rejected without refreshing status.

There is no persistent boot identifier, so a reset followed by sufficiently delayed reconnection can be indistinguishable from normal forward traffic. Always begin a fresh app session after known power loss or firmware reprogramming; do not use these records as forensic proof of uninterrupted operation. Modular timestamp arithmetic also assumes successive status records are less than half a timer cycle apart. CSV exports include author, source mode, epoch field, status age, live state and each event's fields. Epoch remains zero within this manually delimited revision-A session. Reconnect clears local history, so export first. The map is a retained-event view, not a guaranteed complete device-lifetime graph.

## Build, verification, and programming

Required tools are GNU make, host GCC, Arm bare-metal GCC, Node.js, Python 3, and KiCad 9 with Python bindings and footprint libraries. No npm install is required. Run from the repository root:

```sh
make -C contact-atlas/firmware clean all test
node --test contact-atlas/tests/*.test.mjs
node --check contact-atlas/app/app.mjs
python3 contact-atlas/tools/check_hardware.py
```

The host test exercises qualification, directional edges, return excursions, gap invalidation, timer wrap, saturation, queue overflow and serializer bounds under address and undefined-behavior sanitizers. Application tests compile the real C fixture, feed its synthetic output through the real JavaScript parser and UI handlers, exercise screens and CSV Blob generation, and explicitly mock serial transport and DOM behavior. They are not evidence of native browser downloads or physical USB testing. `tools/check_hardware.py` runs actual ERC/DRC, exports a fresh schematic XML netlist, and compares component identity, value, footprint and each pin's net against the PCB and manifest.

Program `firmware/build/contact-atlas.elf` using the Nucleo ST-LINK SWD programmer and verify flash contents before reset. The supplied ELF contains address information; do not relocate it arbitrarily. Programming has not been performed on a physical board in this run. There is no authenticated bootloader, OTA update or rollback storage; recovery is a trusted local SWD reflash. Rebuilding the carrier with `tools/generate_hardware.py` requires the installed KiCad footprint libraries and should be followed by the full hardware check.

## Mechanical design, BOM, power and use cases

The first enclosure should accommodate the 80 × 70 mm carrier, a separately supported Nucleo, short interconnects and USB strain relief. Use insulated edge supports because the carrier has no mounting holes. Keep signal leads below roughly 0.5 m for initial trials, bundle each with its return, and use insulated clips that cannot bridge adjacent selector terminals. Mark J3 through J6 on the leads. A printed knob-index sheet is recommended because electrical signatures alone cannot identify repeated detents. Do not let clips or test leads carry the weight of the switch body.

The carrier BOM is five two-pin 2.54 mm headers, one four-pin header, four 10 kΩ axial resistors and four 1 kΩ axial resistors. The resistor footprint uses 7.62 mm lead spacing. Quarter-watt parts are adequate at the intended supply. Budgetary prototype costs, not supplier quotations, are USD 15–30 for a Nucleo, USD 3–8 for passives and connectors, USD 5–15 allocated PCB cost, and USD 10–25 for leads and enclosure. A USD 70–100 kit is only a commercial target, not a quoted product. Existing workshop instruments may make building this device unnecessary.

Reserve 100 mA at 5 V for a conservative initial USB supply budget, dominated by the Nucleo and debugger. The carrier's calculated maximum switch load is only 1.20 mA at 3.3 V. Measure actual consumption before assigning a product rating. There is no battery, charging circuit, sleep-runtime promise or wireless regulatory claim. Classroom demonstrations, disconnected low-voltage selector repair and comparison of clean versus contaminated contacts are appropriate uses. Industrial load switching, medical release testing, machine guarding and installed mains diagnostics are not.

## Safety and physical acceptance plan

Disconnect the selector completely from all powered equipment, stored energy and external wiring before attaching probes. This carrier has no galvanic isolation, reverse-power protection, TVS array or certified overvoltage category. Even its ground lead connects to the host computer. Never attach mains, industrial 24 V signals, powered sensors, or a supposedly isolated contact without verifying that it is truly voltage-free. The absence of active outputs does not make accidental external voltage safe.

Begin with unpowered continuity checks against the connector table. Inspect soldering and resistor values, then power the Nucleo and measure VCC and each open/closed SENSE voltage. Confirm individual mask bits before connecting a real selector. Exercise all sixteen masks with a safe test fixture, hold endpoints for more than 200 ms, and compare the event stream to independent observations. Repeat forward and reverse movement, include short self-return excursions, unplug a contact, reset power, interrupt USB and verify that stale or discontinuous evidence is visibly unknown.

No physical result, contact life, timing accuracy, EMC robustness, ESD immunity, regulatory compliance or production suitability is established by software compilation or clean CAD reports. Carrier ERC is passive and does not validate the purchased controller or external wiring. The design is intended for supervised prototyping and transparent experimentation. Detailed automated outcomes and remaining limitations are recorded in VERIFICATION.md.
