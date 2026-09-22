# LoomMapper

Author and creator: jayis1
Copyright (c) 2026 jayis1
Hardware license: CERN-OHL-S-2.0 · Firmware and companion software: MIT

## Purpose and overview

LoomMapper is a portable, eight-channel instrument for discovering the topology of an unpowered wiring harness and finding movement-dependent faults that ordinary continuity meters routinely miss. A technician clips numbered leads to conductors, connector pins, test points, or breakout fixtures at the accessible ends of a harness. The instrument then applies a current-limited, coded 977 Hz excitation to one channel at a time, listens on every other channel with synchronous detection, and builds a confidence-scored pin-to-pin graph. After the baseline is frozen, a guided flex-watch mode tracks the entire matrix while the technician bends, taps, twists, or warms one physical section. The companion application identifies which electrical path changed with that movement and produces a service report that names the likely segment, evidence, and next inspection step.

The practical problem is common in vehicles, appliances, industrial machines, stage equipment, robots, agricultural machinery, and custom electronics. A schematic says which wire should connect two points, but an aging physical loom develops broken strands under insulation, incompletely seated terminals, fretted contacts, water-assisted leakage, crushed branches, and repairs whose colors no longer match documentation. A handheld meter can prove one path at one instant. It cannot efficiently watch dozens of possible paths while both of the operator's hands manipulate the loom. Traditional tone tracers help locate one conductor but do not establish a many-to-many topology or correlate intermittent changes with a physical flex action.

LoomMapper fills that gap. It is not an energized-circuit probe, insulation-withstand tester, or substitute for manufacturer safety procedures. It is a low-energy diagnostic mapper for harnesses that have been isolated from their power source and discharged. Its novelty is the workflow: a multiplexed, phase-sensitive graph scan establishes what is connected, a movement-correlated watch mode distinguishes stable topology from intermittent topology, and a local-first application preserves evidence in a technician-readable form. No materially equivalent product was identified in the project inventory. Commercial cable verifiers usually require known connector adapters and expected pinouts; wire tracers usually follow one tone; harness production testers usually use large fixed fixtures. LoomMapper targets uncertain, installed, repaired, or one-off looms with portable clips and no required golden netlist.

## User value and target audience

The primary user is a maintenance technician who has an intermittent electrical failure but cannot reproduce it with a static resistance measurement. The pain is not simply that a wire may be broken. The pain is that the failure disappears when a connector is unplugged, when the loom is moved into a comfortable meter position, or when the technician changes from driving the machine to holding probes. Existing tools force a serial process: select two points, attach probes, move the harness, remember the reading, and repeat. On an eight-conductor branch, that process becomes slow and error-prone.

LoomMapper keeps all eight clip channels under observation and turns the manipulation itself into a timestamped experiment. The user can label a harness, capture a baseline, start flex watch, manipulate only the door hinge boot or robot wrist section, and see a single source-to-sink path become unstable. A confidence score is always paired with raw variation evidence. The application does not claim that a particular crimp is defective; it reports that movement of a named segment correlates with a discontinuity or cross-coupling change, and asks the technician to inspect and confirm.

Target users include automotive electrical technicians, aviation and marine maintainers working under approved procedures, appliance repair specialists, field robotics teams, industrial controls engineers, theatrical and broadcast cable shops, makers restoring undocumented equipment, and small harness manufacturers handling rework. The design is especially valuable where a connector family lacks an affordable automated tester or where the harness cannot be removed without substantial disassembly.

## Fidelity and status

This repository is an engineering reference design, not a certified commercial instrument. The portable C firmware has a deterministic host simulation and self-tests. The web companion has protocol tests and a simulator so its complete workflow can be exercised without hardware. The KiCad schematic and PCB are text-based design artifacts with matching references, named nets, footprints, a board outline, planes, and routed critical signals. They still require a qualified engineer to complete ERC/DRC review, check library footprints against current component drawings, finish production routing, validate creepage for the intended environment, create fabrication outputs, and perform bench characterization.

No claim is made that the instrument has been tested on physical hardware, certified for vehicle service, approved for aircraft maintenance, or proven safe on every harness. Battery runtime, noise rejection, fault thresholds, and coupling behavior are calculated targets awaiting prototype measurements. The software labels simulated values as simulated. Production deployment requires a formal hazard analysis and a test fixture that verifies every channel's output limiting and input protection.

## Operating requirements

LoomMapper shall map up to eight attached conductors in a full 8 by 8 matrix. A complete matrix consists of 64 source-to-receiver observations. The target baseline refresh interval is 1.6 seconds: each source receives a 200 ms aggregate window composed of shorter acquisitions. Flex watch retains the same graph cadence while extracting higher-rate envelope statistics from the 4 ksample/s ADC stream. The design targets detection of a stable good path above 1.8 mV synchronous amplitude, hard cross-connections above the calibrated range, and movement-correlated changes exceeding 8.5 mV. Those thresholds are starting points, not universal electrical limits.

The device must operate only on isolated, unpowered harnesses with no stored energy. Excitation is 180 mV nominal through current-limiting and analog protection. A future production design should perform a preflight voltage measurement on every clip and refuse excitation if any external potential exceeds a conservative threshold. The present reference architecture documents that requirement, but the physical threshold must be validated against component tolerances and fault cases.

The operating temperature target is 0 to 45 degrees Celsius for the handheld unit and leads. Storage target is -10 to 60 degrees Celsius with the lithium cell removed or managed according to its supplier limits. The enclosure target is 118 by 72 by 24 mm, with a 68 by 38 mm internal four-layer controller and a separate clip lead set. The design is intended for dry service areas. It has no hazardous-location, ingress, medical, or explosive-atmosphere rating.

## Hardware specifications

### Processing, conversion, and switching

The primary processor is a Nordic nRF5340-QKAA. Its application Cortex-M33 runs acquisition scheduling, synchronous feature extraction, graph inference, local logging, USB service transport, and the BLE protocol. The network core handles Bluetooth Low Energy. This separation keeps radio scheduling from perturbing timing-sensitive acquisition. Firmware flash and RAM budgets are conservative: approximately 220 KiB application image, 96 KiB working RAM, 32 KiB capture buffers, and at least half of the device resources reserved for the target SDK, bootloader, update slots, and future calibration code.

A Texas Instruments ADS131M04 provides four simultaneous 24-bit delta-sigma channels. The reference firmware models 4 ksample/s observations. Production firmware should use the data-ready interrupt and DMA-capable SPI rather than polling. The ADC observes protected receiver paths and reference channels so amplitude and phase can be normalized against the actual excitation. An Analog Devices AD5686R quad 16-bit DAC generates the low-amplitude excitation and calibration levels. The DAC output passes through a reconstruction network, current limiting, and the TMUX1308 switching tree. Two TMUX1308 devices are required in a fully populated eight-source/eight-receiver implementation even though the condensed schematic represents the switching function as U4; that simplification is a known schematic revision item before fabrication.

Each clip channel requires back-to-back low-leakage clamps, a resettable or fixed current-limiting element, a discharge path, series resistance, and a preflight-voltage sense path. The critical design principle is that no firmware state may defeat physical current limiting. A stuck multiplexer, reset MCU, or malformed command must not turn the device into an energy source for an attached circuit.

### Power and connectivity

A protected 1200 mAh single-cell lithium-polymer battery supplies the instrument. A production revision should pair a USB-C power-path charger such as BQ24074 with input protection, temperature-qualified charging, and a 3.3 V buck-boost regulator. MAX17048 provides battery state estimation over I2C. The reference design separates VBAT and 3V3 nets and includes a resettable fuse and USB ESD component. USB power is for charging and local service; harness clips remain galvanically referenced to device ground, so the user must not attach the instrument to powered equipment.

Estimated active demand is 55 mA for MCU and radio, 18 mA for ADC and references, 6 mA for DAC and switching, 8 mA average for display and indicators, and 10 mA allowance for conversion loss and peripherals, for roughly 97 mA during continuous flex watch. At 1.2 Ah nominal, an ideal result is 12.4 hours. Applying an 80 percent usable-capacity allowance gives approximately 9.9 hours. Baseline-only duty cycling targets 35 mA average and roughly 27 hours usable operation. These are engineering estimates; cell age, regulator efficiency, radio use, temperature, and display choice will change them.

BLE exposes the versioned LoomMapper service for status, commands, edge pages, and event notifications. USB CDC is the preferred development and service transport. The application included here runs as a local web interface with a deterministic simulator. A production browser build can use Web Bluetooth where supported, while native mobile packaging can implement the same byte protocol.

### Form factor and human interface

The handheld enclosure target is 118 by 72 by 24 mm with eight color-and-number coded leads exiting through a replaceable locking connector. The device includes one scan button, one acknowledge button, a high-visibility status LED, a small monochrome display, and a recessed power control. The clip leads should use touch-proof insulated miniature grabbers, replaceable test hooks, or application-specific breakout fixtures. Lead labels must remain visible when several clips are crowded around a connector.

The controller PCB uses four layers: top signal and components, an uninterrupted ground plane, a 3.3 V plane with analog islands reviewed for return-current continuity, and bottom signal. The ADC, reference, and receiver network occupy the quiet end of the board. Radio antenna clearance is kept away from clip leads and the switching edge. SWD test pads, ADC data-ready, excitation monitor, receiver monitor, I2C, battery, and every clip path require fixture-accessible test points.

## System architecture

```text
  Eight numbered insulated clips
      |   |   |   |   |   |   |   |
      +---+---+---+---+---+---+---+------ [voltage preflight + protection]
                                              |
                    +-------------------------+-------------------------+
                    |                                                   |
           [source TMUX tree]                                  [receiver TMUX tree]
                    |                                                   |
     [AD5686R DAC + current limit]                         [low-noise receiver AFE]
                    |                                                   |
                    +-------- coded excitation ---- harness ----> [ADS131M04]
                                                                        |
                                                                  SPI + DRDY
                                                                        |
                                                                  [nRF5340]
                                                            /       |        \
                                                     [local log]  [USB CDC]  [BLE]
                                                                          |
                                                               Companion application
```

The architecture uses a low-frequency coded carrier rather than DC continuity alone. Synchronous in-phase measurement rejects much of the ambient noise collected by long leads. Quadrature energy is retained because capacitive coupling, wet contamination, and marginal contacts may not look like an ideal resistive path. The full matrix allows unexpected cross-connections to appear rather than limiting the test to a user-entered expected pair.

### Interface allocation

SPI0 runs at 8 MHz to ADS131M04, with dedicated ADC_DRDY. SPI1 or separate chip-select timing controls AD5686R. GPIO 2 through 5 select and enable the mux tree. GPIO 10 through 17 read clip presence or fixture-identification signals. I2C0 at 400 kHz serves MAX17048 and low-rate peripherals. GPIO 18 and 19 are SWDIO and SWCLK; GPIO 22 drives status indication. Exact nRF5340 peripheral instances and pin-drive settings must be frozen in the target board definition before PCB release.

The firmware models 64 samples per acquisition window and advances one source every 25 ms in simulation. Production acquisition can collect overlapping ADC windows and derive in-phase, quadrature, RMS noise, clipping count, and reference amplitude. The graph layer receives only bounded features, not unbounded raw streams. That division limits RAM demand and makes the inference code host-testable.

## Firmware design

The firmware is split into board definitions, documented peripheral commands, acquisition drivers, topology inference, a versioned binary protocol, scheduler logic, and self-tests. `board.h` is the single source for channel counts, thresholds, pins, timing, and protocol identity. `registers.h` records only commands and register addresses tied to the named converters; target builds should replace direct bus helpers with SDK drivers while preserving the same higher-level interfaces.

At boot, the device initializes the platform, clears graph and event storage, verifies converter configuration, and enters baseline mode. The scheduler selects one source, captures all receiver evidence, updates eight graph edges, then advances to the next source. After three complete matrices, state changes from baseline to scan. Flex watch enables a movement-sensitive comparison policy. Report timing is independent of scan timing so radio congestion cannot halt acquisition.

Each graph edge records source, sink, classification, confidence, magnitude, variation, first observation, and last observation. The initial classifications are unknown, good, open, short, and crosstalk. Intermittent is a temporal classification: it requires a previously good path to collapse beyond the configured delta, or a prior intermittent path to continue showing high variation. This prevents a single absent receiver or unattached clip from being presented as a discovered intermittent.

Confidence uses signal-to-noise ratio and classification margin, then clamps output to 0 through 1000 permille. The code deliberately avoids floating-point math in the graph and protocol layers. The graph identity is a CRC-32 over canonical bounded fields. It is useful for quickly recognizing that a topology changed; it is not a cryptographic signature. Event storage is a fixed 64-entry ring so repeated faults cannot exhaust memory.

Fault paths are explicit. Invalid channels, null buffers, malformed frame lengths, unsupported versions, bad CRC values, and impossible state changes return deterministic error values. Three classes of recovery are planned for hardware: retry a bounded bus transaction, reset one converter after repeated timeouts, or move to fault state and remove excitation. A production watchdog must default analog switches to disabled and require periodic proof that the main acquisition loop is alive.

### Protocol version 1

Every binary frame begins with bytes `0x4C 0x4D`, protocol version 1, message type, and a little-endian payload length. Payload follows, then CRC-16/CCITT-FALSE and newline framing for service-console visibility. Status reports state, source index, battery millivolts, fault count, edge count, scan sequence, graph CRC, and overflow status. Edge-page reports carry bounded groups of source, sink, kind, confidence, variation, and scaled magnitude records.

Commands are start baseline (`0x80`), stop (`0x81`), flex mode (`0x82` with one Boolean byte), and clear session (`0x83`). Firmware validates the complete frame before applying a command. Unknown commands fail closed. The JavaScript codec implements the same magic, version, byte order, length rule, CRC vector, and command structure; tests exercise round trips and corruption rejection.

The host build uses any C11 compiler and enables `-Wall -Wextra -Wpedantic -Werror`. `make check` removes prior artifacts, builds all modules, executes six self-test suites, and runs a deterministic simulation. The target recipe documents Cortex-M33 flags and the Nordic SDK dependency but is not claimed as a full linkable target image because a production Zephyr board definition, startup objects, linker script, and generated device-tree headers are outside this concept repository.

## Companion application

The companion is a responsive local-first web application with four functional views. Map displays the eight-by-eight pin matrix, differentiates stable paths from opens and anomalies, and lets the technician select any cell to inspect confidence and variation. Flex watch controls a guided movement session and shows movement-correlated instability plus an inspection recommendation. Report produces a concise finding table and exports JSON containing the author, harness label, protocol version, time, and complete bounded model. Setup records the harness label, excitation profile, unpowered-harness acknowledgement, and local-storage preference.

The application includes a deterministic simulator rather than pretending a browser in this repository is connected to real hardware. Connection state is visible. A future transport module can replace the simulator without changing the rendering model. The app handles short frames, invalid magic, unsupported versions, incorrect length, and CRC corruption by throwing an explicit error before data reaches the interface. No raw acoustic, location, or cloud account data exists. Reports stay local unless the user exports them.

To run it, install no third-party packages. Use Node.js 20 or later, run `npm test`, then `npm start` and open `http://localhost:8080`. The static server whitelists only the four required assets and ignores path traversal attempts by reducing requests to a basename and checking an allowlist.

## Use cases

### Vehicle door or tailgate loom

A door module fails only when the door is half open. The technician isolates the battery according to the service procedure, disconnects both ends of the door loom, attaches a breakout fixture, and captures a baseline. During flex watch, moving the rubber boot repeatedly changes path 5 to 1 while other paths remain stable. The report identifies a movement-correlated discontinuity with high variation. The technician still verifies the conductor and repair location using approved methods; LoomMapper reduces the search region.

### Robotic wrist cable chain

An encoder intermittently drops during one robot pose. With the machine locked out and the harness isolated, LoomMapper maps the suspect branch. A guided motion of each chain section reveals one data conductor whose magnitude collapses near the tightest bend. The fixed matrix simultaneously verifies that the shield and adjacent power conductors did not become cross-connected.

### Undocumented restoration harness

A restorer has a modified machine with faded wire colors and no trustworthy diagram. Clips at two connector breakouts produce a topology graph that can be exported and annotated. Unexpected weak coupling is marked separately from strong continuity so contamination or shared components do not silently become asserted wire connections. The result is a starting map, not an automatically certified schematic.

### Harness manufacturing rework

A small shop receives a low-volume harness with occasional crimp escapes but cannot justify a dedicated fixture. A configurable clip breakout and expected-map import can turn LoomMapper into a rework aid. The portable tool does not replace a production hipot or pull test, but it catches wrong cavities, open crimps, and movement-sensitive terminations before the assembly returns to the customer.

## Power, throughput, and resource budgets

A 4 ksample/s, four-channel, 24-bit raw stream is 384 kbit/s before status words and bus framing. Allowing 32-bit words and one status word per sample yields approximately 640 kbit/s, comfortably below an 8 MHz SPI bus even with transaction overhead. Firmware need not retain the full stream. Four channels times 64 samples times four bytes is 1024 bytes per window; double buffering consumes 2048 bytes. Feature records and the 96-edge graph remain under several kilobytes.

BLE sends summaries, not raw ADC data. A status frame is 27 bytes including framing. Sixty-four edge records at ten payload bytes each fit into several paged notifications. At a conservative five 100-byte notifications per second, a complete graph transfers in under two seconds without stressing BLE. USB service mode can expose optional debug windows at higher throughput.

The GPIO budget is eight clip-presence inputs, four mux controls, three SPI signals plus chip selects and data ready, two I2C pins, two SWD pins, two buttons, one LED, USB, and optional display control. Pin multiplexing remains feasible on nRF5340, but the selected package and radio layout require detailed pin assignment review. DMA requires one receive and one transmit channel for ADC SPI, one transmit channel for DAC updates, and optional USB/BLE buffers. CPU utilization target is below 20 percent during scan and below 35 percent during flex watch, leaving margin for radio and UI.

## Safety, reliability, privacy, and limitations

The most important safety rule is simple: use LoomMapper only on disconnected, de-energized, discharged harnesses. The operator must follow the equipment manufacturer's lockout and service procedures. The tool is not rated for mains, traction batteries, pyrotechnic circuits, airbag initiators, medical equipment, flight-critical wiring, intrinsically safe areas, or live vehicle buses. The low excitation target does not make every connection safe. Sensitive semiconductor inputs, stored charge, or an incorrect ground reference can still be damaged.

Hardware protection must be verified by destructive and fault-insertion testing. Required cases include external voltage on every clip, adjacent clip short, mux stuck on, DAC at full scale, reversed battery, USB ESD, brownout during flash write, missing ADC, corrupted calibration, and a watchdog reset during excitation. The safe state disables every analog switch and discharges excitation nodes. Calibration records need version, range checks, and CRC. Firmware updates need signature verification and rollback.

Topology can be ambiguous when the harness contains lamps, coils, suppression diodes, termination resistors, electronic modules, or multiple parallel paths. LoomMapper therefore reports measured evidence and confidence rather than asserting a perfect wire list. It cannot locate a break along distance by itself. Flex correlation narrows the mechanical region only when the operator manipulates one region at a time. A passing scan does not prove insulation integrity, current capacity, EMC performance, or mechanical pull strength.

Harness topology can reveal proprietary machine design and maintenance history. The reference app is local-first, has no analytics, and exports only by explicit user action. A production BLE link should use authenticated pairing, reject unauthenticated commands, rotate keys when ownership changes, and rate-limit session control. Device identifiers should not be embedded in exported filenames unless requested.

## Manufacturing and validation plan

Prototype bring-up begins with the power tree and safe-state verification before converters or radio are enabled. A precision resistor matrix fixture emulates opens, known resistances, shorts, capacitance, diode paths, and intermittent relay contacts. Each channel is tested as source and receiver. The fixture records amplitude linearity, phase error, crosstalk, noise floor, and response to injected common-mode interference. Thermal tests repeat calibration across the operating range.

The next stage uses representative automotive, appliance, and robotics harnesses with seeded broken strands, backed-out terminals, corrosion films, and adjacent-wire abrasion. Blind trials compare LoomMapper findings with microscopy, four-wire resistance, and teardown results. Flex guidance is evaluated for false correlation when several harness regions move together. Thresholds must be tuned from measured distributions rather than from the simulation constants.

Before fabrication, run KiCad schematic ERC and PCB DRC with the installed project libraries, inspect every footprint against the manufacturer's current drawing, complete the second mux and receiver protection detail, route all remaining nets, add controlled test points, review antenna clearance, and produce an assembly drawing. The project is deliberately labeled an engineering reference until those tasks and physical tests are complete.

## Repository contents and build instructions

- `firmware/main.c` implements startup, scheduler, deterministic integration flow, and six self-test suites.
- `firmware/drivers.c` implements converter setup boundaries and a deterministic harness model.
- `firmware/topology.c` implements classification, confidence, graph identity, intermittent detection, and event history.
- `firmware/protocol.c` implements version 1 framing, status, edge pages, and command validation.
- `firmware/board.h`, `registers.h`, and `loommapper.h` define hardware and module contracts.
- `kicad/` contains the project, schematic, PCB, and structural validator.
- `app/` contains the responsive companion, protocol codec, local server, and tests.
- `verify.py` enforces structure, authorship, README length, and substantive firmware size.

Run the complete available validation from this directory:

```sh
python3 verify.py
make -C firmware check
python3 kicad/validate.py
cd app && npm run check
```

A successful run proves that the host firmware compiles with strict warnings, its deterministic tests pass, the app codec tests pass, required authored files exist, the KiCad text is structurally balanced with matching references and critical nets, and the size gates are met. It does not prove target linking, BLE interoperability, ERC/DRC cleanliness, electrical safety, or physical performance.

## Design decisions and roadmap

Eight channels were selected because they cover many sub-harnesses while keeping clips, mux leakage, scan time, and report complexity manageable. More channels can be supported with fixture modules, but a monolithic 32-channel handheld would become harder to protect and calibrate. Phase-sensitive low-voltage excitation was selected over a simple buzzer because it supports noise rejection and weak-path characterization. Explainable thresholds were selected over an opaque classifier because technicians need evidence and serviceability more than a mysterious score.

The first roadmap item is a complete protected dual-mux analog sheet and preflight voltage lockout. The second is a Zephyr board definition, signed update flow, USB CDC transport, and BLE service implementation on prototype hardware. The third is expected-netlist import and visual comparison without hiding unexpected edges. Later options include four-wire low-resistance fixtures, distributed wireless clip pods for inaccessible harness ends, and a time-domain accessory for distance-to-fault estimation. Each extension should preserve the central promise: bounded excitation, explicit uncertainty, offline operation, and a report that helps a human make the final repair decision.
