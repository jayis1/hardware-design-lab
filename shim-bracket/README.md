# Shim Bracket — indexed feeler-gauge declaration notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and locally authored hardware sources are MIT licensed. Installed KiCad footprint geometry retains its upstream licensing; see LICENSE.

Revision A is an unbuilt engineering prototype. Target firmware linking, host tests, a real desktop event loop and CAD checks are reproducible. No carrier has been fabricated, target flashed, gauge carousel assembled or physical gap measured. This is not a calibrated clearance instrument, automatic inspection system or safety interlock. The custom PCB is a passive carrier for a separately purchased NUCLEO-F401RE; the microcontroller and its support electronics are on that purchased board.

## Purpose, audience and useful outcome

Shim Bracket helps a repair-cafe technician or model-making workshop keep consistent notes while checking one stationary, unpowered gap with several feeler leaves. The operator selects a captive leaf, tries it in the gap, and presses either FIT or REFUSE. A two-bit indexed selector associates that declaration with the selected thickness. The instrument remembers every accepted result, including contradictory results, rather than replacing an earlier refusal with a later fit. It shows the thickest leaf declared to fit and the thinnest leaf declared not to fit, provided those observations remain logically compatible.

A concrete problem is the apparently convincing but impossible notebook: a thick leaf was recorded as fitting while a thinner leaf was recorded as refusing. That may mean the gap moved, a leaf was misidentified, insertion angle changed, the technician changed their force criterion, or a button was pressed accidentally. A simple maximum-thickness display hides the disagreement. Shim Bracket makes it visible and refuses to summarize contradictory evidence as a clean interval. Repeating a declaration is legal; changing one's answer does not erase history.

The primary audience is a workshop comparing manual fixture setups, not an industrial inspection department. An educator demonstrating go/no-go reasoning is a secondary user. The proposed experiment uses a blunt, adjustable slot model on a bench, four separately mounted gauge leaves and a rigid selector. The user can deliberately change the slot halfway through a survey and see why merging measurements from different physical states is misleading. One MCU acquisition session must correspond to one unchanged gap and one unchanged gauge set.

The result is explicitly a declaration bracket. No sensor measures insertion force, drag, gauge thickness, contact pressure or the actual clearance. An operator can press FIT without inserting anything. Surface contamination, bent leaves and off-axis insertion can produce consistent but wrong declarations. The electronic policy checks the logical consistency of statements, not their physical truth. Do not use this prototype for engine adjustment, brakes, moving machinery, certified inspection or any decision whose failure can injure someone.

## Why hardware, originality and alternatives

A phone checklist could store the same mathematical observations. The reason to add hardware is physical binding between a captive gauge selector and large tactile declaration paddles, so the operator need not type a thickness while holding the fixture. Sampling on the MCU also detects selector movement during a declaration, independent of desktop scheduling. This benefit remains a hypothesis until a real usability trial compares it with pencil notes. If the fixture is more distracting than a notebook, use the notebook.

The repository was pulled and its device directories and README titles inspected before choosing this concept. ContactAtlas records the electrical transition graph of a detached rotary switch; it does not attach thickness meaning to a selector or compare fit and refusal evidence. Pattern Sled learns repeated mechanical words by position; it does not retain monotonic go/no-go constraints. Duplex Witness associates stamp strokes with captive faces and checks exactly-once coverage; this device permits repeated statements and explicitly retains opposing statements. Their common passive interface is intentionally reused, not presented as new electronics.

External prior art is substantial. The RS feeler-gauge guide describes trying graded blades and judging fit. McMaster-Carr lists stepped, tapered and ordinary feeler gauges. Lisle's 68150 uses stepped go/no-go blades. Those are established practical alternatives and may be all a user needs. The proposed contribution is the small indexed declaration workflow, selector-envelope validation and contradiction-preserving notebook, not invention of feeler gauges or interval inference. Worldwide novelty, patentability and freedom to operate have not been established by this bounded search.

References consulted:

- https://uk.rs-online.com/web/content/product-knowledge/measurement-testing/precision-measurement-tools/feeler-gauges-guide
- https://www.mcmaster.com/products/feeler-gauge-sets/
- https://www.lislecorp.com/specialty-tools/go-no-go-feeler-gauge

An automatic force-sensing insertion rig was rejected because alignment, drag and fixture compliance would dominate its validation. A camera reading blade markings was rejected because reflection and obscured markings would create a second identification problem. A plain incrementing counter was rejected because it cannot distinguish incompatible statements. This revision is deliberately receive-only and low-energy, with no actuator and no connection to the machine whose detached part is being examined.

## Hardware specifications

| Function | Revision A specification |
| --- | --- |
| Controller | STM32F401RET6 on external NUCLEO-F401RE, MB1136 |
| Core and memory | Cortex-M4, nominal 16 MHz HSI, 512 KiB flash, 96 KiB SRAM |
| Inputs | Two selector contacts and two normally-open declaration paddles |
| Input domain | Voltage-free contacts only; external 3.3 V pullups |
| Sampling | Nominal 1 ms acquisition, five equal samples to qualify |
| Selector | Two-bit Gray code, qualified as one bundle |
| Connection | USART2 TX through ST-LINK virtual COM, 115200 baud, 8N1 |
| Power | USB to Nucleo; Nucleo 3V3 and ground to carrier |
| Carrier | 80 by 70 mm, two layers, 1.6 mm FR-4 |
| Companion | Linux Python 3 with Tk, standard library only |
| Storage | MCU volatile SRAM; optional private desktop CSV exports |

The purchased controller supplies its regulator, reset circuitry, decoupling, debugger, USB interface and SWD programming path. Those parts are not placed on the passive PCB. This distinction matters when ordering: fabricating only the supplied board will not produce a working instrument. The design uses short jumper cables, not a shield footprint that mates directly to the Nucleo.

| Carrier path | Function | Nucleo terminal |
| --- | --- | --- |
| J1.1 VCC | 3.3 V supply only | CN6.4 3V3 |
| J1.2 GND | Common return | CN6.6 GND |
| J2.1 SENSE0, J3.1 SELECT0 | Selector low bit | CN8.1 A0, PA0 |
| J2.2 SENSE1, J4.1 SELECT1 | Selector high bit | CN8.2 A1, PA1 |
| J2.3 SENSE2, J5.1 FIT | Fit paddle | CN8.3 A2, PA4 |
| J2.4 SENSE3, J6.1 REFUSE | Refusal paddle | CN8.4 A3, PB0 |

J3 through J6 pin 2 is ground. Logical one means a closed contact, although the electrical input goes low. J2 contains signals only. J1 is not a 5 V input. All grounds are shared with the USB host; there is no galvanic isolation. Never connect powered machine contacts, sensor outputs, mains wiring or an external voltage to a contact header.

Each channel is 3V3 through 10 kohm to SENSE, then through 1 kohm to the contact and ground. Calculated closed voltage is 0.30 V and current is 0.30 mA per channel; all four closures consume 1.20 mA. These are nominal resistor calculations, not measurements. Select clean gold contacts suitable for low wetting current. A high-current switch with oxidized contacts may be unreliable at this excitation. Series resistance is not surge protection; no TVS, isolation or reverse-polarity protection is supplied.

The register map follows the existing repository STM32F401 platform. USART2 TX is PA2 alternate function 7. Consult ST RM0368 and UM1724 before assembly, including connector orientation and the purchased board revision's ST-LINK VCP solder bridges. The inherited MB1136 configuration expects SB13/SB14 fitted and SB62/SB63 open. No fresh manufacturer-PDF audit or physical bridge inspection was performed in this run. Do not substitute a similarly named Nucleo without checking its pin map and clock behavior.

- https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf

## Architecture and block diagram

    Captive selector track 0 --> J3 SELECT0 -- 1k -- PA0
    Captive selector track 1 --> J4 SELECT1 -- 1k -- PA1
    FIT paddle ---------------> J5 FIT ----- 1k -- PA4
    REFUSE paddle ------------> J6 REFUSE -- 1k -- PB0
                                                   |
                                           10k pullup per input
                                                   |
    USB --> NUCLEO-F401RE regulator --> J1 3V3 / GND
                    |
                    +-- SysTick acquisition --> bundled selector qualifier
                    |                       --> independent paddle qualifiers
                    |                                      |
                    |                            declaration envelope
                    |                                      |
                    |                           retained fit/refuse masks
                    |                                      |
                    +-- USART2 queue <--- SB1 CRC snapshot --+
                             |
                       ST-LINK virtual COM
                             |
                   Linux Tk workbench --> private CSV

Acquisition owns event timing. The app does not time button presses from packet arrival. A nonblocking transmit queue prevents serial waits from deliberately delaying input reads. Missing acquired ticks abandon the current declaration and invalidate qualification rather than inventing stability during the missing interval. Accepted historical masks remain visible across that loss, so a disconnect does not silently turn a contradictory survey into a consistent one.

## Mechanical assembly proposal

Mount four separate leaves on a detented selector disk with nominal thickness labels 0.25, 0.50, 0.75 and 1.00 mm. The leaves must remain individually accessible; do not stack them. Two cam tracks actuate low-force contacts to produce, in increasing thickness order, 00, 01, 11, 10. This Gray ordering changes only one contact between adjacent detents. It reduces transition ambiguity but does not prove that the selector is physically in a detent: all four codes are valid. A separate detent sensor is not present.

Use mechanical hard stops and a positive detent independent of switch spring force. An initial disk diameter of about 100 mm and a guarded handle are reasonable prototype starting dimensions, not manufacturing drawings or tolerances. Keep the sharp gauge edges away from fingers and deburr the holder without altering the gauge working surfaces. The selected blade must be visible and removable for independent thickness verification. The code tracks must turn with the blade carrier, not with a knob that can slip independently.

Place the FIT and REFUSE paddles where the free hand can reach them without moving the selector. Different shapes are preferable to color alone. Hold the chosen position from before pressing until both paddles have been released. Initially use a rigid, blunt demonstration slot, not an actual engine or energized mechanism. Mount the boards inside an insulating enclosure with edge supports; the carrier has no mounting holes and must not be drilled through routed copper. Keep leads below roughly half a metre and pair each signal with its ground return.

One accepted survey must describe one gap at one location, one blade set and one insertion criterion. Moving to another location requires exporting and resetting the MCU. The application reconnect button does not reset firmware. It is intentionally impossible to correct a contradictory result by deleting a single declaration. Export the evidence, inspect the reason, then explicitly begin a fresh survey. Record the real leaf identifiers and calibration information in the local fixture label or an accompanying workshop record.

## Bill of materials and power budget

| Item | Quantity | Selection |
| --- | --- | --- |
| MCU assembly | 1 | NUCLEO-F401RE MB1136 |
| Pullups | 4 | 10 kohm, 1%, 0.25 W axial, 7.62 mm pitch |
| Series resistors | 4 | 1 kohm, 1%, 0.25 W axial, 7.62 mm pitch |
| Two-pin headers | 5 | 1x2, 2.54 mm vertical |
| Four-pin header | 1 | 1x4, 2.54 mm vertical |
| External switches | 4 | Low-level gold-contact switches; D2F-01L is a candidate |
| Gauges | 4 | Independently checked leaves at the documented nominal sizes |
| Mechanical assembly | 1 | Detent disk, cam tracks, paddles, enclosure and wiring |

Reserve 200 mA at 5 V for the Nucleo and debugger as a planning allocation, not a measured maximum. The carrier's calculated input load is at most 1.20 mA at 3.3 V. Firmware runs continuously and no battery runtime is claimed. A preliminary small-quantity budget is USD 25–40 for the controller and USD 25–60 for carrier, switches and wiring, excluding gauges, custom mechanics, shipping and calibration. These estimates are not live supplier quotations. Measure actual current, rail voltage and regulator temperature during physical bring-up.

## Firmware policy and implementation

The shared startup, linker, GPIO, clock, watchdog and UART infrastructure is reused from jayis1's Duplex Witness at b58793b. The new measurement engine, SB1 payload and companion contract are specific to this device. `tools/import_platform.py` documents the substitutions and refuses to overwrite existing files. Reusing a tested hardware support layer is preferable to inventing a different register driver for every workflow.

The selector is qualified as a two-bit bundle. Each result paddle qualifies independently, preventing selector chatter from hiding a stable press. Five equal samples establish a qualified value; the first sample counts, so this spans four nominal inter-sample intervals. It is not a guarantee of five milliseconds of analog stability. Startup and recovery wait until both paddles are released. A press already held at startup is not credited as a new declaration.

States are WAIT, READY, ACTIVE and HALT. A qualified paddle press starts ACTIVE and captures the previous qualified selector identity. Simultaneous paddles or selector changes during the raw press setup taint the declaration. While ACTIVE, any raw or qualified opposite paddle, or selector disagreement, remains sticky. Releasing both paddles completes the declaration; only an untainted envelope updates the appropriate fit or refusal mask. Repeated accepted declarations increment the count but do not erase or duplicate mask bits.

A fit at index i contradicts a refusal at any index j less than or equal to i. This includes opposing declarations for the same leaf. Contradiction is derived from retained masks and is therefore persistent until reset. A compatible survey with no fit has an unknown lower bound; one with no refusal has an unknown upper bound. The display does not silently substitute zero or infinity. Nominal thickness labels are descriptive, not calibration constants applied to a measured sensor value.

The active deadline is 10,000 acquired ticks. Timeout takes precedence over completion on the exact deadline. A sampling gap aborts an active declaration and requires fresh release qualification; previous accepted masks survive. Starts always equals accepted plus rejected plus aborted plus the active indicator. Checked start counters reserve disposition capacity, and counter exhaustion halts rather than wrapping. Configuration and watchdog initialization failures also halt. Reset clears SRAM and there is no persistent boot identity or flash journal.

## Protocol and software interface

SB1 snapshots are transmitted approximately every 100 ms as ASCII. The body contains the marker and these unsigned decimal fields in order:

    SB1,now,state,levels,valid,starts,accepted,rejected,aborted,gaps,faults,
    tx_errors,pass,refuse,conflict,age,tainted,selected,outcome

The physical record is one line, not wrapped. Append an asterisk, four uppercase CRC hexadecimal digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial value 0xffff and no final XOR over the body before the asterisk. The test string 123456789 produces 0x29b1. Pass and refuse are four-bit index masks. Outcome is 4 for FIT, 8 for REFUSE and 12 for ambiguous simultaneous paddles. Selected is the decoded ordinal, not the raw Gray code. Fault bits are configuration 1, watchdog initialization 2 and overflow 4.

Run `python3 app/workbench.py` on Linux with Tk installed. The four pages provide connection/replay, current bracket and per-gauge evidence, history, and policy/export. Serial permission and invalid-path errors preserve prior evidence. Only one port is owned at a time. Original terminal attributes are restored when possible on close. There are no remote commands, pairing credentials, cloud uploads or discovery assumptions. Selecting a serial path is an explicit local action.

A live label requires two advancing valid snapshots and recent progress. Duplicate timestamps do not renew freshness. Counter rollback, lost mask bits, inconsistent duplicates and impossible evidence freeze the session. A reception pause or accumulated host/device drift beyond two seconds requires reconnect so buffered old data cannot quietly become live again. An uncalibrated HSI can also cause conservative rejection during a long run. Some resets that resemble forward traffic remain indistinguishable without persistent boot identity; deliberately reconnect after reset.

Replay accepts only regular files up to two MiB and rejects corrupt frames, empty recordings and partial final records. It is always marked replay, including synthetic C fixtures. Sessions retain at most 2000 snapshots, not a complete event journal. CSV export uses exclusive file creation and mode 0600, includes author, provenance, freshness, connection state, warnings and parser-error counts, and refuses overwrite. Free-form labels are protected against spreadsheet formula interpretation. CRC is accidental-corruption detection, not authentication.

## Reproduction, tests and physical acceptance

From this device directory:

    make -C firmware clean all test
    xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_quality.py

The target build uses arm-none-eabi-gcc and produces an ELF plus binary with real startup and memory mapping. Host C tests use address and undefined-behavior sanitizers. Tests enumerate all three-declaration words against an independent monotonicity oracle, including repeats and both declaration polarities, then exercise raw setup/hold glitches, opposite paddles, timeout boundaries, gaps, overflow and timestamp wrap. Python tests enumerate all mask combinations and challenge framing and session continuity.

The GUI test compiles the production C engine and encoder into a synthetic signal fixture, then delivers its snapshots through a Linux pseudo-terminal at device-time cadence. A real Tk event loop displays compatible evidence, contradictions, rejected declarations and an aborted press. It exercises all tabs, disconnection, replay, invalid connection attempts and private export. This is functional software evidence, not a USB hardware test or an observed human experiment.

KiCad checking runs schematic ERC and board DRC with violations fatal, exports a fresh XML netlist and compares every reference, value, footprint identity and pad net with the manifest. The board is routed, not merely populated. Passive-symbol ERC cannot establish correct external supply wiring or safe cable installation. The schematic has also been rendered for visual inspection. Generator and checker require KiCad 9 and its system Python bindings.

Before physical use, inspect the unpowered carrier, verify cable continuity and polarity, and measure the rail before connecting signals. Check every selector detent against its printed leaf identity and exercise both paddles separately. Run intentionally consistent and contradictory surveys against a fixed test slot. Compare switch events with an independent recorder; inspect whether operator force changes the answer. Test broken and shorted leads, unplugging and reset. Only then decide whether this fixture is useful enough to improve. Fabrication, programming, calibrated thickness, EMC, ESD, endurance, mechanical repeatability and real user benefit remain unverified.
