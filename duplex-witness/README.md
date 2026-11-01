# Duplex Witness — captive two-face stamping evidence fixture

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint material retains its upstream licensing.

Revision A is an engineering prototype. Its firmware is target-linked, its local companion is executable, and its passive carrier is routed and electrically checked. No board has been fabricated or programmed, no flip holder has been assembled, and no physical stamping trial has been performed. The MCU, regulator, decoupling, debugger and USB interface are on a separately purchased NUCLEO-F401RE, not on the custom carrier. This is not a machine controller or a product-inspection system.

## Purpose, overview and intended users

A small print shop making two-sided hand-stamped tags can easily stamp the same face twice, forget the reverse face, or release a tag before finishing its second operation. A handle counter cannot distinguish these mistakes: two presses might mean two faces, the same face twice, or an empty fixture. Duplex Witness observes a captive reversible holder and records whether its electrical indications support exactly one manual stamp stroke on each face before the holder is unlatched. Either face may go first. The distinguishing feature is face coverage within one uninterrupted latch episode, not the total number of presses in a working day.

The proposed mechanical assembly holds a tag in a shallow pivoting frame with two detents. A latch retains the tag while the frame rotates. One switch indicates face A at the printing station, another indicates face B, a third indicates the hand-stamp platen reaching its selected contact region, and the fourth indicates the retaining latch closed. The two face switches describe holder orientation, not the identity of the printed artwork. The application shows active coverage, stroke count, mismatching trials, abandoned trials and recent outcomes without requiring a camera, wireless network or account.

Target users are book arts studios, community print rooms, craft businesses producing small runs of double-sided labels, and educators studying manual fixture design. A useful experiment compares two detent geometries or latch placements and asks whether one layout reduces repeated-face operations. The recorder also makes a good teaching demonstration of the difference between counting events and preserving identity across a process. It should not be used to grade people: fixture friction, switch placement and confusing artwork can all cause the same observed sequence.

A matching trial means only that the observed contacts satisfied the documented policy. The holder could be empty, the ink pad could be dry, the tag could slip, and the artwork could be upside down. The device does not measure pressure, image registration, optical density, sheet count, or whether the correct stamp was selected. A fixture manipulated without a tag can pass. Final visual inspection remains essential. The practical value is diagnosing sequencing mistakes, not certifying saleable output or preventing injury.

## Originality and rejected alternatives

The repository's device directories and README titles were inventoried before selection. Punch Register was read in detail because it is the closest process neighbor: it checks two registration followers through a single punch-handle cycle. Duplex Witness instead retains a set of visited faces across multiple presses and permits a controlled orientation change between them. Kit Multiset compares a tray-delimited multiplicity of independent bin declarations; this device associates every stroke with one exclusive physical detent and invalidates movement while the press is down. Pattern Sled compares repeated mechanical words by strip position, rather than processing a captive object on opposite faces. Sand Stroke separates traversals by face but does not impose exactly-once two-face completion before unlatching.

A bounded public search found existing stamping platforms, alignment aids and double-sided stamp mounting. The Stamparatus description explains reversible hinged plates and deliberate re-stamping at the same position. TONOSHO describes magnetic holders and alignment jigs. Those examples demonstrate that stamping fixtures themselves are not new. This proposal's narrower contribution is a removable, receive-only notebook for one-stroke-per-face coverage within a captive latch episode, with explicit ambiguity and interruption outcomes. No patentability, worldwide uniqueness, or freedom-to-operate conclusion is claimed.

Three alternatives were rejected for this revision. A camera could inspect actual artwork, but image lighting and training data would dominate a small workshop prototype. A total stroke counter is cheaper but cannot assign strokes to the two faces. A latch timer could measure overall cycle duration but would miss repeated-face work. An industrial PLC can implement the policy and is preferable in an existing industrial installation. This low-energy bench design intentionally avoids modifying a powered press, adding an interlock, or claiming that hobby electronics are suitable safety equipment.

Prior-art references:

- https://www.handmakewithlove.com/stamparatus-stamp-positioning-tool-stamping-platform/
- https://tonosho.com/pages/metal-stamp-alignment-tool

The common F401 support platform, carrier topology, transport framing infrastructure and license are reused from jayis1's Kit Multiset at repository revision dbf6043. Reuse is explicit: a new workflow does not require inventing another GPIO driver. The face-coverage engine, DW1 payload, companion evidence rules and tests are specific to this device. `tools/import_platform.py` records the source and substitutions and refuses to overwrite existing files.

## Hardware specifications and interface contract

| Function | Revision A choice |
| --- | --- |
| Processor | STM32F401RET6 on external NUCLEO-F401RE MB1136 |
| Memory | 512 KiB flash and 96 KiB SRAM |
| Clock | Internal HSI restored to nominal 16 MHz |
| Sensors | Four low-level, normally-open, voltage-free contact switches |
| Acquisition | One sample per nominal millisecond, independent five-sample qualifiers |
| Connectivity | USART2 TX through ST-LINK virtual COM, 115200 baud, 8N1 |
| Commands | None; target UART receiver disabled |
| Supply | USB into Nucleo; its regulated 3.3 V supplies carrier |
| Carrier | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Assembly | Through-hole resistors and 2.54 mm headers |
| Companion | Linux Python 3 with Tk, standard library only |

The following map is authoritative for the firmware, manifest, schematic and wiring. Logical one means closed contact, although the electrical input is low. Do not connect a powered sensor or machine signal to these inputs.

| Bit | Meaning | Contact header | Carrier signal | J2 pin | Nucleo connection |
| --- | --- | --- | --- | --- | --- |
| 0 | Face A detent | J3 | FACE_A / SENSE0 | 1 | CN8.1, A0, PA0 |
| 1 | Face B detent | J4 | FACE_B / SENSE1 | 2 | CN8.2, A1, PA1 |
| 2 | Press contact region | J5 | PRESS / SENSE2 | 3 | CN8.3, A2, PA4 |
| 3 | Captive holder latch | J6 | LATCH / SENSE3 | 4 | CN8.4, A3, PB0 |

Every J3–J6 connector has signal on pin 1 and ground on pin 2. J1 pin 1 connects to CN6.4, the Nucleo 3V3 output; J1 pin 2 connects to CN6.6, ground. Never use the 5 V pin. J2 is signal-only and must not be mistaken for a power connector. Number both ends of every cable before fitting the enclosure.

Each branch has a 10 kohm pullup from SENSE to 3.3 V and a 1 kohm resistor between SENSE and the switch. With a closed switch the nominal input is 0.30 V and branch current is 0.30 mA; four closures draw 1.20 mA. These are topology calculations, not measured results. The switch must support low wetting current: a tarnished power contact is not an equivalent component. There is no galvanic isolation, TVS network, reverse-polarity protection or industrial input conditioning. USB ground is shared with the host computer.

USART2 uses PA2 alternate function 7. The normal MB1136 virtual-COM configuration has SB13 and SB14 fitted and SB62 and SB63 open. Verify the exact purchased board revision rather than assuming another Nucleo model is interchangeable. Programming uses the board's ST-LINK SWD connection. The design intentionally keeps processor decoupling, reset, voltage regulation, clock components and USB routing on the purchased assembly instead of drawing an incomplete bare-MCU circuit.

Platform references are ST UM1724, Table 16 and USART communication section, and RM0368 for the STM32F401 peripheral map. Indexed UM1724 content was checked for the connector and bridge mapping; direct PDF downloads timed out in this run. Register definitions are retained from the existing F401 platform and must be reviewed against RM0368 during independent board bring-up.

- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

## Architecture and block diagram

    Face A detent ----- J3 -- FACE_A -- 1k -- SENSE0 -- PA0
    Face B detent ----- J4 -- FACE_B -- 1k -- SENSE1 -- PA1
    Platen witness ---- J5 -- PRESS --- 1k -- SENSE2 -- PA4
    Captive latch ----- J6 -- LATCH --- 1k -- SENSE3 -- PB0
                                                |
                                           10k pullups
                                                |
    USB -> NUCLEO-F401RE 3V3 ------------------- J1
                |
                +-> SysTick / GPIO -> qualifiers -> face-coverage engine
                |                                        |
                +-> USART2 queue <- CRC snapshot <-------+
                         |
                      ST-LINK VCP -> Linux companion -> private CSV

Timing and classification happen on the MCU, not according to desktop packet arrival. The UART driver moves bytes without blocking acquisition and abandons a queued frame after its bounded timeout. A later complete marker permits the companion to recover from a truncated transmission. The app checks field ranges, conservation and cross-frame continuity before displaying evidence. It cannot reconstruct every hidden transition from periodic snapshots, and it does not claim to.

## Mechanical fixture proposal

Use a pivoting open-window frame approximately 100 by 70 mm for tags up to roughly 70 by 40 mm. These are prototype starting dimensions, not production tolerances. A rigid perimeter supports the tag outside the printable region. Two opposed detents locate the frame at the same printing plane after a half-turn. Provide a mechanical stop at each detent; the switch lever must never serve as the load-bearing stop. Stamp force should go through the frame supports into the bench, not through the pivot bearings or contact switches.

Retain the tag with a small over-center edge clamp or hinged perimeter latch that stays closed through rotation. Place the latch contact on the retaining member itself, not on a separate operator button. The logical trial ends when this contact opens. If the tag can be removed or exchanged without opening the latch, the recorded episode no longer corresponds reliably to one captive object. This limitation cannot be repaired in software. Validate it with the thinnest and most flexible stock intended for use.

Position two separate detent followers so only one is closed at either printing position. Between positions both are open. Sustained both-closed indications during a press are invalid, not a third face. Route flexible switch wiring away from the rotating window, adding strain relief and a service loop. The wires must not wrap around the axle after repeated flips. Limit rotation mechanically to alternating half-turns rather than allowing unlimited rotation that twists the cable.

The PRESS switch is an external witness for a low-force hand-stamp platen. Set its trip region only after checking that the selected manual tool reaches the intended stamping position without the switch impeding travel. This design does not require a hammer, motor, pneumatic cylinder or powered press. Do not install it in a crush zone or remove guards. Make initial adjustments with a blunt dummy platen and no ink. A switch closure proves neither contact force nor adequate transfer, so compare witness timing with direct observation and finished tags during commissioning.

Mount both electronic boards in an insulating enclosure with edge supports and covered terminals. The carrier has no mounting holes; do not improvise holes through routed copper. An initial internal envelope of about 160 by 110 by 45 mm leaves room for the Nucleo, passive board and cable bends, but actual purchased connector clearances must be measured. Keep contact leads short, preferably below half a metre, pairing every signal with its ground return. No environmental sealing or EMC immunity is claimed.

## Bill of materials, power and sourcing

| Item | Quantity | Selection / constraint |
| --- | --- | --- |
| Controller | 1 | NUCLEO-F401RE, MB1136 assembly |
| R1, R3, R5, R7 | 4 | 10 kohm, 1%, 0.25 W axial, 7.62 mm pitch |
| R2, R4, R6, R8 | 4 | 1 kohm, 1%, 0.25 W axial, 7.62 mm pitch |
| J1, J3, J4, J5, J6 | 5 | 1x2, 2.54 mm male header |
| J2 | 1 | 1x4, 2.54 mm male header |
| External contacts | 4 | Gold-contact low-force switch; D2F-01L is a candidate |
| Carrier PCB | 1 | Supplied two-layer KiCad design |
| Mechanical kit | 1 | Pivot frame, stops, latch, platen witness, enclosure and wiring |

Switch choice is conditional on actual force, travel and low-level contact specifications. Equivalent resistors from another manufacturer are acceptable if resistance, tolerance, rating and body fit remain equivalent. Header substitutions need matching pitch and pin numbering. Another MCU development board is not a drop-in substitute without checking its connector mapping, memory map, debugger and UART bridges. The external switches and purchased controller are assembly items, not fictitious on-board components in the carrier BOM.

Reserve 200 mA at 5 V for the Nucleo and debugger as a supply-planning allocation, not a measured maximum. The passive carrier adds at most the calculated 1.20 mA at 3.3 V for all four contacts closed. Firmware runs continuously; there is no battery, charger, radio duty cycle or sleep-life claim. USB disconnect ends observation. Measure actual USB current and regulator temperature with all contacts closed before enclosing the assembly. A rough small-quantity planning allowance is USD 25–40 for the controller and USD 20–50 for carrier, contacts and wiring, excluding custom mechanics and shipping; these are estimates, not vendor quotations.

## Firmware policy and design decisions

`main.c` restores the reset clock, configures inputs, starts the independent watchdog and polls acquisition. `drivers/board.c` owns GPIO, SysTick and watchdog registers. `drivers/clock.c` bounds early waits and checks the critical input and UART configuration. `drivers/serial.c` owns the fixed transmit buffer. `engine.c` contains the portable policy. `wire.c` encodes snapshots without a hosted C library. Startup vectors and the linker script are supplied; the target build really links an ELF and produces a flashable binary.

Each contact must produce five consecutive equal sampled values before its qualified level changes. The first sample counts as one; five samples span four nominal inter-sample intervals, not a guaranteed five milliseconds of physical stability. The filters are independent so an unrelated chattering detent cannot conceal a stable latch release. Faster pulses may be missed. This is a slow hand-operation witness, not a logic analyzer.

The states are WAIT, READY, ACTIVE and HALT. Startup begins in WAIT. An observed unlatch and released press establish READY even if one face detent is already engaged. A fresh qualified latch closure starts ACTIVE only with the press released. Merely powering on with the latch already closed does not start a trial. Face orientation may settle after latching, but must be established before a press. The user must unlatch and release the platen to establish a new baseline after an interrupted trial.

Every qualified PRESS rising edge increments the active stroke count. It contributes a face only if exactly one detent is qualified and agrees with the prior sample and raw contact vector. Repeated coverage of the same face taints the trial. During the whole raw or qualified press envelope, including the release endpoint, a face change, contradictory face code or raw latch opening also taints it. Even a raw glitch too short to pass qualification can therefore invalidate a press association. The conservative rule deliberately favors a visible mismatch over silently assigning a stroke to an uncertain face.

Unlatching completes the trial. Exactly two strokes, both faces covered and no taint yield a matching outcome; every other completed trial is mismatching. A held press at unlatch is mismatching. The 60,000-tick active deadline takes precedence over a simultaneous qualified unload and abandons the trial. A sampling gap also abandons it, clears qualification and demands a new empty baseline. Counter exhaustion produces a terminal HALT after preserving the active trial as abandoned. Arithmetic maintains starts = matching + mismatching + abandoned + active indicator. Lifetime counters do not wrap into plausible small values.

Only observation state is retained in SRAM. Reset erases it. There is no flash event log and no persistent boot identity. The companion detects obvious counter and timestamp rollback, but a reset whose new counters and timestamps happen to resemble forward traffic can remain indistinguishable. Reconnect deliberately after resetting hardware. For traceable work, export one session before beginning another and keep external notes about the fixture and stock.

## Protocol and companion application

DW1 is a newline-delimited ASCII snapshot every approximately 100 ms. The body starts with `DW1,`, followed by unsigned decimal fields in this order:

    now,state,levels,valid,starts,good,bad,aborted,gaps,faults,tx_errors,
    age,tainted,faces,strokes,last_faces,last_strokes,last_result,last_age

The suffix is an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial value 0xffff and no final XOR, covering the entire body before the asterisk. The check value for ASCII `123456789` is 0x29b1. Faces is a bit mask: 1 means A, 2 means B, and 3 means both. Results are zero for none, one matching, two mismatching and three abandoned. Fault bits are configuration 1, watchdog initialization 2 and counter exhaustion 4. Time is modular uint32 milliseconds.

Run `python3 app/workbench.py` from this device directory on Linux with Tk installed. The four pages are Connection / replay, Face coverage, History and Policy / export. Choose the serial path explicitly; no device is automatically trusted based on a USB name. Permission failures are shown without deleting previously loaded evidence. Only one serial port may be owned at a time, and original terminal attributes are restored on close. Disconnect before replacing a live session with replay data.

Live status requires two advancing valid timestamps and recent progress. Repeated packets do not renew freshness. A reception pause or accumulated host/device drift beyond two seconds freezes acceptance and requires reconnect, rather than relabeling queued old packets as current. The conservative drift bound may also reject a long run on an uncalibrated oscillator. The UI preserves old evidence but labels it stale or disconnected. Parser corruption counts are included in export metadata.

Replay accepts only a regular file no larger than two MiB, checks every frame strictly and rejects an unfinished final line. It is always labeled replay, including recordings produced by the C fixture. The history retains the last 2000 snapshots, not every lifetime event. Several trials between snapshots can increase counters without delivering all intermediate outcomes. CSV exports explicitly carry provenance, connection state, freshness, parser errors, warnings and a locally supplied fixture label. Files are created privately with mode 0600 and never overwritten; free-form labels are protected from spreadsheet formula interpretation.

## Reproducible verification and bring-up

From this directory run:

    make -C firmware clean all test
    xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_quality.py

The target toolchain is arm-none-eabi-gcc. Host tests use GCC with address and undefined-behavior sanitizers. GUI tests use an actual Tk event loop under Xvfb and a POSIX pseudo-terminal, not physical USB. The synthetic fixture runs the production C engine and encoder, producing matching, mismatching and abandoned outcomes that pass through the real app parser and UI. The face-word test enumerates all binary face words through eight strokes against an independent exact-coverage oracle. Boundary tests cover timeout precedence, gaps, held contacts, raw glitches, saturation and modular time rollover.

KiCad 9 is required for these generated files and its Python bindings are available through the system interpreter in the validation environment. The checker runs ERC and DRC with violations treated as errors, exports a new schematic XML netlist and compares every component value, footprint identity and pin net with the PCB and manifest. It also requires the DRC result to have no unconnected items. The schematic was rendered for visual inspection. Passive-symbol ERC cannot verify the external supply, cable polarity or Nucleo solder bridges. A successful CAD check is not a fabrication or electrical-performance certificate.

For physical bring-up, first inspect the unpowered board for shorts and check every cable end-to-end. Power only the Nucleo and measure the carrier rail before attaching the signal header. Exercise each switch separately and confirm the corresponding bit, then test the empty baseline, A-first and B-first valid paths, duplicate-face strokes, held-press flips, premature unlatch and deliberate cable interruption. Compare timestamps with independent observation and inspect the printed tags. Flash only a deliberately connected target using `make -C firmware flash`; that target was not invoked during this design run.

## Failure modes, security and release limits

An open switch wire can resemble an open contact; a short can resemble a permanently closed one. There is no line-supervision resistor coding and no redundant sensing. A broken latch switch may prevent starting, while a shorted latch switch can hide an exchange of stock. An incorrect detent adjustment can produce convincing but wrong face attribution. None of these faults is made safe by the app. Keep the instrument advisory and separate from machine operation, and include deliberate sensor fault tests in fixture acceptance.

No network service, credential, cloud storage or remote command parser is present. Anyone with physical access can manipulate contacts or reprogram the target. CRC detects accidental corruption, not malicious forgery or operator identity. Local exports may still reveal production activity, so protect them according to the workshop's needs. Dependencies are limited to installed toolchains, KiCad and Python/Tk; there is no downloaded application package tree.

Fabrication, target flashing, mechanical tolerances, contact endurance, ESD, EMC, calibration and real user benefit remain unverified. The design is ready for an independently reviewed bench prototype, not unattended production deployment. The most important next experiment is a small controlled run comparing actual tag faces with the recorded coverage while intentionally introducing repeated-face strokes and early unlatching. If the fixture itself creates more confusing handling than it prevents, simplify the mechanics before adding software features.
