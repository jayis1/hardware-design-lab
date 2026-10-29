# Flow Pause — declared-supply hopper silence and intervention notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, software and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. Its custom PCB is a routed passive interface for a separately purchased NUCLEO-F401RE. Target compilation, software behavior and CAD connectivity are verifiable; no assembly has been fabricated, flashed, mechanically calibrated or tested with real granules. Observed discharge silence is not proof of a blockage, and recovery after an intervention is not proof that the intervention caused it.

## Purpose, user and practical value

Flow Pause is a tabletop experiment notebook for an educator or small-fixture designer studying intermittent discharge through a gravity-fed hopper. The user changes an outlet insert, loads a small quantity of clean plastic beads, watches discharge, and occasionally taps the outside of the fixture. The instrument remembers when an operator-declared supply interval contained a long absence of completed collection pulses, whether an intervention was declared during that pause, and whether a completed pulse subsequently returned before observation was abandoned.

The recurring problem is not simply counting material. A hand-written note such as “three taps fixed the jam” conflates an empty hopper, delayed filling of the collection cup, a stuck reed switch, a genuinely blocked outlet, and a spontaneous restart. Flow Pause separates explicit operator declarations from electrical observations. Its primary evidence is a cumulative ledger of qualified discharge pulses, silence episodes, declared interventions, observed recoveries and abandoned episodes. It does not assign a physical cause or compute an efficiency score from those counters.

A useful first experiment compares two removable outlet inserts using the same beads, receiver, fill depth and procedure. The operator records the insert name separately and exports each session. A pause with no declared intervention is visibly different from a pause followed by intervention and recovery; a session interrupted by sampling loss is visibly different from an uninterrupted session. These distinctions are valuable even when absolute mass flow is unavailable. The instrument should be rejected as impractical if the collection mechanism dominates the pauses or if operators cannot reliably declare supply and intervention without changing their behavior.

This is not a silo alarm, feeder controller, food-processing appliance, pharmaceutical dispenser, industrial blockage detector or automatic unjamming system. There is no actuator. Do not use combustible powder, toxic material, airborne dust, pressurized gas, mains equipment or moving machinery. Use only a supervised tabletop fixture with large, nonhazardous beads and a stable catch tray. Keep small parts away from children and protect the floor from spilled beads.

## Originality assessment and rejected alternatives

The repository was pulled and its device directories and README concepts inspected before selection. The closest existing project, Tip Cadence, compares two irrigation emitter pulse streams and alternating-bucket intervals. It does not implement a declared-supply run, a conditional silence episode, prior-intervention attribution, or run/pause abandonment accounting. GrainGuard observes spoilage-related environmental conditions inside silos, rather than supervised discharge experiments. Siphon Witness tracks liquid vessel thresholds. Flow Pause has a different user experiment and evidence model, although it deliberately reuses jayis1's proven passive carrier and STM32 platform code.

A detached cable flex tester was rejected because LoomMapper already documents movement-correlated harness faults. A generic paired drip counter was rejected because Tip Cadence already covers that workflow. Flow Pause was selected for the distinction between supply declaration, observed silence, declared intervention and later recovery. Hardware is justified by continuous local sampling and explicit physical controls; a computer with a GPIO interface could implement the same algorithm, but would still need a protected contact fixture, bounded acquisition semantics and a reproducible interface. No claim is made that this is the only implementation strategy.

Related research already studies intermittent granular discharge and vibration-assisted unjamming. In particular, “Unjamming a granular hopper by vibration,” DOI 10.1209/0295-5075/87/24002, reports alternating flow and blockage regimes. The paper at https://arxiv.org/pdf/0907.4871 discusses avalanche statistics and distinguishes formation of blocking arches from their subsequent breakage. These are clear conceptual neighbors, not evidence that granular intermittency is a new discovery. The contribution here is a small, auditable, local-first instrument workflow with conservative attribution and explicit lost-observation accounting. Worldwide novelty, patentability and absence of commercial equivalents are not established.

## Hardware specification and block diagram

| Item | Revision A implementation |
|---|---|
| Processor | STM32F401RET6 on purchased NUCLEO-F401RE MB1136 |
| Clock and memory | Internal HSI 16 MHz; 512 KiB flash, 96 KiB SRAM |
| Sampling | Nominal 1 ms SysTick; independent 8-sample qualification |
| Inputs | Four voltage-free paths: discharge, supply, intervention, enable |
| Carrier | 80 × 70 mm, two layers, 1.6 mm FR-4, through-hole parts |
| Conditioning | Four 10 kΩ pull-ups and four 1 kΩ series resistors |
| Communication | USART2 PA2 TX, 115200 baud 8N1, ST-LINK virtual COM |
| Power | USB into Nucleo; 3.3 V and common ground to carrier |
| Storage | Volatile MCU ledger; local companion retains 500 snapshots |
| Outputs | Telemetry only; no wireless, motor, valve or command receiver |

```text
Hopper -> rocking catch cup -> magnet/reed -------- J3 DISCHARGE
Operator supply-present maintained switch -------- J4 SUPPLY
Operator intervention declaration pushbutton ------ J5 INTERVENE
Operator experiment-enable maintained switch ----- J6 ENABLE
                                                    |
                                   1k series + 10k pull-ups
                                                    |
USB -> NUCLEO-F401RE 3V3 -> J1           J2 -> PA0/PA1/PA4/PB0
            |                                       |
            |                           independent qualification
            |                                       |
            |                           run / pause / pulse engine
            |                                       |
            +---- ST-LINK VCP <- USART2 <- CRC-protected FP1 snapshots
                     |
              Linux desktop workbench
              Evidence / History / Setup / private CSV
```

The purchased Nucleo contains the microcontroller, power regulator, decoupling, programmer, USB interface, oscillator support and reset circuit. The supplied schematic is the complete passive interface, not a fabricated claim that those components are present on this custom board. All fourteen carrier components have real symbols, footprint identities and connected nets. Its PCB has routed copper and a closed outline. Passive-only ERC does not validate the external regulator, physical switch wiring or mechanical fixture.

### Pin allocation and assembly

| Carrier | External connection |
|---|---|
| J1.1 VCC | Nucleo CN6.4 regulated 3V3 |
| J1.2 GND | Nucleo CN6.6 ground |
| J2.1 SENSE0 | CN8.1 A0, PA0; discharge |
| J2.2 SENSE1 | CN8.2 A1, PA1; supply declaration |
| J2.3 SENSE2 | CN8.3 A2, PA4; intervention declaration |
| J2.4 SENSE3 | CN8.4 A3, PB0; enable |
| J3.1 / J3.2 | Reed switch / ground |
| J4.1 / J4.2 | Supply switch / ground |
| J5.1 / J5.2 | Intervention button / ground |
| J6.1 / J6.2 | Enable switch / ground |

Each channel follows VCC → 10 kΩ → SENSE → 1 kΩ → external contact → GND. Closed is logically one. A nominal closed path draws 0.30 mA and places approximately 0.30 V at the GPIO. The four-path maximum is approximately 1.20 mA from 3.3 V. These are circuit calculations, not measurements. Low wetting current can expose contact oxidation; select low-level signal contacts rather than power switches. The resistor network is not galvanic isolation, overvoltage protection, a certified current limiter or an ESD solution. Never attach an energized sensor output.

The carrier connects with short jumpers rather than stacking as a shield. Keep signal leads short, strain-relieved and away from static-prone plastic motion. Use labeled, keyed cable housings where possible, and verify pin 1 with a meter before applying power. The two-pin headers do not physically prevent polarity reversal. Insulate the solder side, support the board at its edges, and keep falling material away from connectors. The board outline has no mounting holes; an enclosure should use edge supports or a nonconductive tray, not improvised screws through copper.

Manufacturer reference documents are ST RM0368 for the STM32F401 register map and UM1724 for the MB1136 connector and solder-bridge arrangement. Their URLs are https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf and https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf. The direct UM1724 download failed during this run, including an HTTP/1.1 retry. This revision reuses the repository's documented platform and does not claim a fresh manufacturer-manual audit. Verify the actual purchased board revision, connector numbering and default USART solder bridges before assembly.

### Mechanical prototype and bill of materials

The discharge sensor is a proposed gravity-return rocking catch cup beneath the hopper outlet. Mount its pivot outside the falling stream, place a small magnet on the cup, and fix a normally open reed switch to the stationary base so each dump produces one closed interval followed by opening. Set an adjustable counterweight so the cup returns completely after dumping. The sensor must be open at rest. Arrange a surrounding tray to capture overflow and a guard so the user cannot strike the sensor while declaring a tap. This fixture is a design proposal, not a validated volumetric bucket.

Begin with a transparent hopper about 100 mm tall, interchangeable outlet inserts, a pivoted cup approximately 40 mm wide and a catch tray wider than the complete assembly. These are starting dimensions, not production drawings or guaranteed flow geometry. Adjust the outlet and receiver for the bead diameter. Confirm that the receiver does not touch or obstruct the outlet. Record material, approximate fill depth, insert geometry and receiver adjustment in the experiment notebook. Cup pulses are not bead counts or calibrated mass: partial fills, retained material and changing dump thresholds invalidate that interpretation.

| Part | Quantity | Rough budget at ten units |
|---|---:|---:|
| NUCLEO-F401RE, purchased | 1 | USD 20–30 |
| 10 kΩ axial resistor, 1%, 0.25 W | 4 | USD 0.40 combined |
| 1 kΩ axial resistor, 1%, 0.25 W | 4 | USD 0.40 combined |
| 2.54 mm 1×2 headers | 5 | USD 1 combined |
| 2.54 mm 1×4 header | 1 | USD 0.30 |
| Bare two-layer carrier | 1 | USD 3–8, supplier dependent |
| Low-level NO reed and magnet | 1 set | USD 2–5 |
| Maintained SPST switches | 2 | USD 2–6 combined |
| Momentary NO pushbutton | 1 | USD 1–3 |
| Fixture, wire, enclosure and tray | 1 set | USD 10–25 |

These are planning allowances, not fetched quotations. Resistor footprints are DIN0207 axial with 7.62 mm lead spacing; headers are through-hole vertical 2.54 mm. The board can be repaired with ordinary soldering tools and individually replaced resistors. A conservative initial USB allowance is 100 mA for the complete purchased-board assembly, including debugger and LEDs; this is not a measured consumption figure. There is no battery, charging circuit or promised runtime. Measure USB and 3.3 V demand before any power optimization.

## Firmware contract and design decisions

The firmware is freestanding C11, built for Cortex-M4 with no vendor HAL or network dependency. Startup installs a vector table, copies initialized data, clears BSS and enters main. The clock driver selects and verifies HSI, removes prescalers and bounds all pre-SysTick waits by iteration budgets. GPIOs remain inputs with external bias; USART2 is transmit-only. SysTick advances a millisecond counter, and the main loop samples once per new tick. The independent watchdog is started with bounded ready waits and fed after acquisition. Clock failure stops before normal interface setup; watchdog or input-configuration failure creates a sticky terminal fault.

Four input qualifiers operate independently. A raw value must appear in eight consecutive samples before becoming qualified. Chatter on the intervention input cannot postpone sustained supply withdrawal. Brief transitions can be rejected entirely, so the recorder does not measure raw electrical bounce or guarantee detection of short discharge pulses. A completed discharge pulse consists of a qualified rising edge followed by a qualified falling edge in less than 2000 ms. A held pulse reaching exactly 2000 ms is rejected and abandons the run, even if its release qualifies on that same sample.

WAIT requires all inputs to become known and enable to be observed low. READY accepts a new enable rising edge only while supply is declared and discharge and intervention are both low. Otherwise the attempt returns to WAIT. WATCH records pulses and waits for 5000 ms without a completed pulse. On reaching that boundary, it enters PAUSED and increments pauses. A completed pulse at the exact silence boundary takes precedence and prevents a new pause. The five-second criterion includes cup fill latency; it must never be described as five seconds of proven zero physical flow.

During PAUSED, the ledger accumulates observed pause duration and records rising intervention declarations. A completed discharge pulse increments recovered and returns to WATCH. Assisted increments only when at least one intervention was already recorded in that episode. Intervention qualifying on the same sample as recovery or pause onset is deliberately excluded. Held buttons count once, and buttons pressed before pause onset do not become retrospective help. No causal inference is justified: the display uses the terse field name assisted but explicitly explains it as temporal association only.

Enable withdrawal ends the run normally; supply withdrawal while still enabled abandons it. Both take precedence over coincident discharge. Either can abandon an open pause. Any missed or duplicate acquisition tick abandons active observation and restarts qualification, while retaining previous cumulative evidence. A held enable after that gap cannot silently rearm the instrument. Wraparound between adjacent uint32 timestamps remains legal. Unknown elapsed time is never added to observed pause duration.

Counter updates are transactional. Each sample updates a temporary engine and commits only if every checked increment fits. Exhaustion freezes the pre-update evidence, accounts for the existing active run and pause as abandoned, and enters HALT. The firmware and companion enforce runs = finished + aborted + active and pauses = recovered + abandoned + currently paused. Maximum pause cannot exceed total pause duration. Counts are not silently saturated into plausible-looking statistics. Power loss clears RAM; there is no flash-write path, persistence promise or interrupted-write hazard.

## Wire protocol and companion application

Every approximately 100 ms, firmware emits an ASCII FP1 record. The complete numeric field order is defined in app/protocol.py as FIELDS and mirrored explicitly by firmware/wire.c: time, gaps, tx, fault, qualify, silence_limit, pulse_limit, state, mask, known, runs, finished, aborted, pulses, bad_pulses, pauses, recovered, assisted, abandoned, taps, silent, pause_age, pause_total, pause_max, episode_taps. Fields are unsigned decimal uint32 values. The record ends with an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xFFFF over the bytes before the asterisk.

Every record repeats the measurement policy, so a late connection does not depend on receiving a one-shot boot message. No command parser is exposed. The serial driver copies a bounded frame into its own buffer, drains without blocking acquisition and abandons a stalled transmission after 100 ms. A later complete FP1 marker resynchronizes the host after a partial record. Parser errors and discarded prefix bytes remain visible and exportable. CRC protects accidental corruption, not authenticity; do not treat an untrusted replay file as signed laboratory evidence.

The appropriate companion is a Linux Python/Tk desktop application, not an unused mobile shell. It provides connection and disconnection, a pause-evidence screen, snapshot history, setup guidance, strict replay and private CSV export. It uses only Python's standard library and Tk; no pip package or cloud account is needed. Connect through a serial device for which the current user has permission. Permission errors appear in the application without discarding an existing session. A successful new connection deliberately replaces the session, so export retained evidence first.

Live status requires two advancing device snapshots and a connected port. Duplicate timestamps do not refresh freshness; contradictory duplicates, counter rollback, disappearing faults and impossible duration growth block the session. A reception pause over one second or host/device cumulative time divergence over 500 ms requires explicit reconnect. That bounded drift allowance may reject a long otherwise valid capture from an uncalibrated oscillator; it is intentionally conservative and is not a calibrated clock synchronizer. Some reboots remain indistinguishable without a persistent boot identifier. Replays always remain labeled offline.

The host retains at most 500 snapshots and reports eviction. It does not preserve every pulse edge or every historical sample. CSV includes an authorship/provenance record even for an empty session, plus parser diagnostics, freshness, connection state and blocked reason. Files are created with owner-only permissions and overwrite refusal. Replay is limited to a regular file no larger than one MiB; FIFOs and devices are rejected before reading, and a nonempty truncated tail fails validation. Linux hangup detection handles both errors and poll flags, restoring terminal attributes where possible.

## Reproducible build and validation

From the repository root:

```sh
make -C flow-pause/firmware all test
xvfb-run -a python3 flow-pause/tests/test_app.py
/usr/bin/python3 flow-pause/tools/check_hardware.py
python3 flow-pause/tools/check_quality.py
python3 flow-pause/app/workbench.py
```

Install the GNU Arm Embedded toolchain, host GCC, Python with Tk, Xvfb for headless GUI tests, KiCad CLI and the distribution pcbnew Python bindings. The provided Makefile performs a real target link and creates ELF and binary images. It also runs address/undefined-behavior-sanitized host tests for the engine and production clock/UART drivers. Hardware generation is available as tools/generate_hardware.py; checked-in files do not require regeneration to inspect or build. tools/import_platform.py documents the source reuse and refuses to overwrite existing files; it is not part of normal build execution.

Programming requires a physical Nucleo and OpenOCD; make -C flow-pause/firmware flash provides the command but has not been run against a board. Do not mistake host fixtures for physical measurements. The GUI regression drives the actual Tk event loop through a Linux PTY and consumes a fixture emitted by the real C encoder. CAD validation runs native ERC and DRC, exports a fresh schematic netlist, and compares every reference, pin, net, value and footprint against the PCB and manifest. See VERIFICATION.md for recorded results and remaining limits.

Before trusting a physical experiment, perform continuity and polarity checks with USB disconnected, then measure GPIO levels with each switch open and closed. Exercise known pulse widths and gaps against a logic analyzer. Verify a deliberately held discharge switch aborts rather than counts. Run an empty-hopper trial, a slow-flow trial and a visually confirmed blocked-outlet trial; all may legitimately produce silence, and the instrument must not distinguish their causes automatically. Finally compare synchronized video, independent catch mass and exported evidence while introducing declared and undeclared taps. If the receiver mechanism or declaration workflow masks the phenomenon, redesign it rather than weakening the interpretation caveats.
