# Detent Route — four-station manual-indexing route notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint-library geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier for a separately purchased NUCLEO-F401RE. Target firmware linking, synthetic-input tests, local application workflows and electrical CAD checks are verified separately from fabrication, flashing, mechanical assembly and physical measurements. No production-readiness, angular accuracy, machinery safety or worldwide novelty claim is made.

## Purpose and user problem

Detent Route is a removable notebook for slow four-position manual indexing mechanisms: a classroom Geneva-wheel demonstrator, a hand-operated sample presentation wheel, or a small model-making indexing jig. Four individual station switches identify the marked resting positions. The instrument observes departures and subsequent station arrivals, preserving adjacent clockwise routes, adjacent counterclockwise routes, returns to the same station, and arrivals at the opposite station. It also retains direction reversals and uninterrupted four-step route laps. Its practical purpose is to answer whether a mechanism repeatedly follows the expected station sequence, not whether its shaft is accurate to a specified number of degrees.

A builder experimenting with a printed detent wheel often notices a jump or a return but cannot record which route occurred while keeping both hands on the fixture. Counting turns with a single magnet hides those details. A video recording can provide richer evidence, but it needs framing, lighting, review time and sufficient visibility. Detent Route provides a small, inspectable local record without photographing the operator. An instructor can compare two spring settings, inspect repeated departures that return to their origin, and distinguish an observed adjacent path from an opposite-station jump that cannot safely be assigned a direction.

The instrument never releases a mechanism, authorizes movement, drives a motor or verifies a machining operation. There is no output to an indexer controller. Use a guarded, low-energy manual demonstrator with independent mechanical stops. A detected adjacent arrival means that the two observed identities are neighbors in the configured ring; it does not prove that the shaft moved by the shortest route. A missed whole turn or several unobserved stations can produce the same endpoints. The application deliberately labels counts as route evidence, not certified revolutions.

## Differentiation and prior art

The repository directories and device titles were inventoried before this design was created. The closest measurement contracts, ContactAtlas and Stencil Orbit, were read in full. ContactAtlas learns electrical signature-to-signature transitions in a disconnected rotary switch, including multi-contact signatures. Stencil Orbit derives a relative index from quadrature and associates registration continuity with squeegee use. Detent Route instead uses four absolute physical station identities, requires a qualified all-open departure, and keeps same-station returns, opposite-station ambiguity and consecutive adjacent-route laps distinct. It requires no initial home declaration and has no tool-use or printing concept. Reuse of jayis1's passive carrier and F401 support code is intentional rather than a claim that the resistor circuit itself is new.

Public indexing equipment already uses position cams and dwell switches. A limited search found the [Motion Index Drives RT Indexer operating manual](https://motionindexdrives.com/wp-content/uploads/2025/10/Rotary-Index-Drives-Operating-Manual-2-1.pdf), whose indexed description discusses position-cam adjustment and stopping within a dwell area. The [Taktomat Indexing Controller instructions](https://www.taktomat.de/fileadmin/Daten/Dokumente/Operating_instruction_TIC_Ausf3_SW1_V1_1_en.pdf) describe position feedback, cycle-time monitoring and reversing modes. These are relevant prior art, not evidence that station sensing is novel. This design's proposed distinction is an inexpensive removable, receive-only route notebook with explicit ambiguous endpoints and locally inspectable evidence. A broader product and patent search is required before asserting novelty or freedom to operate.

A conventional absolute encoder and data logger may be preferable where accurate angle, high speed or continuous motion matters. A logic analyzer can capture faster contact timing. This prototype is useful only if the four-switch fixture and its narrow workflow are easier to understand and maintain than those alternatives. Real workshop usefulness remains a hypothesis to test with builders, not a measured market result.

## Hardware specifications

| Item | Revision A contract |
|---|---|
| MCU assembly | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Core and memory | Cortex-M4, reset HSI 16 MHz, 512 KiB flash, 96 KiB SRAM |
| Acquisition | Nominal 1 ms foreground sampling, four-bit bundled qualification |
| Sensors | Four externally mounted voltage-free low-level dry contacts |
| Qualification | Five complete unchanged 1 ms intervals after the first sample |
| Transit deadline | Reject at 10000 acquired milliseconds after departure |
| Communications | USART2 TX PA2 AF7, 115200 baud, 8N1, ST-LINK USB VCP |
| Custom carrier | 80 by 70 mm, two copper layers, 1.6 mm nominal FR-4 |
| Power | USB to Nucleo; Nucleo regulated 3.3 V to carrier |
| Storage | Volatile device counters; local host snapshot history and CSV |
| Actuation/radio | None; no receiver commands, motor, battery or wireless stack |

The Nucleo supplies the MCU, regulator, decoupling, reset circuit, programmer and USB bridge. They are not hidden missing components on the supplied passive schematic. The carrier contains six through-hole headers and eight through-hole resistors with real component symbols, footprint identities, pad nets, copper routes and an outline. It attaches by a short harness, not as a mechanically interchangeable Arduino shield.

### Architecture and block diagram

```text
Station 0 dry contact -- J3 -- 1k -- SENSE0 -- J2.1 -- PA0
Station 1 dry contact -- J4 -- 1k -- SENSE1 -- J2.2 -- PA1
Station 2 dry contact -- J5 -- 1k -- SENSE2 -- J2.3 -- PA4
Station 3 dry contact -- J6 -- 1k -- SENSE3 -- J2.4 -- PB0
                                     |
                          four 10k pulls to 3V3
                                     |
USB --> external NUCLEO-F401RE --> J1: 3V3 / GND
             |
             +-- 1 ms sample --> bundled qualifier --> journey classifier
                                                        |
                                            counters + latest result
                                                        |
                               USART2 TX --> ST-LINK VCP --> Linux/Tk
                                                               |
                                         Live / History / Setup / CSV
```

Connect J1 pin 1 to Nucleo CN6 pin 4, 3V3. Connect J1 pin 2 to CN6 pin 6 or 7, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4, labeled A0/PA0, A1/PA1, A2/PA4 and A3/PB0. Each of J3 through J6 has signal on pin 1 and ground on pin 2. J3 is station zero, J4 station one, J5 station two and J6 station three. Mark clockwise station order on the fixture rather than inferring it from wire colors. The configured positive ring is zero, one, two, three, zero.

R1/R3/R5/R7 are 10 kilohm pull-ups from VCC to SENSE0 through SENSE3. R2/R4/R6/R8 are 1 kilohm resistors from those sense nets to STATION_0 through STATION_3. A closed switch pulls its signal toward ground. The board driver inverts active-low electrical levels, so a protocol bit of one always means contact closed. All contacts share USB-host ground; there is no isolation between channels or from the laptop.

### Mechanical reference assembly

Use a 100 mm outside-diameter insulating witness disk clamped to the rotating demonstration member and a stationary switch ring with four adjustable radial slots at 90-degree spacing. A small actuator tab on the disk should close only one station switch near each mark. The suggested first enclosure is an insulating base large enough to support the 80 by 70 mm carrier and the separate Nucleo, with guarded cable exits. These are starting dimensions and an assembly specification, not a fitted production mechanical drawing. Adapt the clamp bore to the actual model and verify clearance by hand with power removed.

Select low-force, gold-contact switches rated for low-level dry circuits. Mount them so their actuation force does not dominate the detent being studied. A single rotating magnet and four normally open low-current reed switches are an alternative, but their angular operating windows and magnetic hysteresis need characterization. Firmware supports either contact implementation; it does not correct mechanical hysteresis. Keep switch activation sectors separated by an all-open region. At commissioning speed each intended station and clear region should remain stable for at least 100 ms. The electrical minimum is six samples, not an assurance that six milliseconds is a useful mechanical test speed.

The carrier has no mounting holes. Use insulated edge supports or a purpose-made enclosure, not improvised holes through routed copper. Keep prototype sensor leads below approximately half a meter, paired with their ground returns and away from power wiring. Secure loose parts and protect pinch points. Never mount this fixture on an energized production machine or remove a guard to make a switch fit.

### BOM, power and manufacturer references

| Quantity | Reference | Selection |
|---|---|---|
| 1 | External controller | ST NUCLEO-F401RE |
| 4 | R1/R3/R5/R7 | 10k, 1%, 0.25 W axial, 7.62 mm pitch |
| 4 | R2/R4/R6/R8 | 1k, 1%, 0.25 W axial, 7.62 mm pitch |
| 5 | J1/J3/J4/J5/J6 | 1x2, 2.54 mm vertical header with mating harness |
| 1 | J2 | 1x4, 2.54 mm vertical header with mating harness |
| 4 | External sensors | Low-current-rated normally open contacts, mechanics selected by builder |
| 1 | Carrier | Two-layer PCB from supplied native KiCad files |
| 1 set | Fixture | Insulating witness disk, clamps, switch brackets, enclosure and strain relief |

At nominal 3.3 V, a closed contact draws 3.3/11000 amperes, approximately 0.30 mA. The MCU-side low voltage is approximately 0.30 V. Four simultaneous closures draw approximately 1.20 mA and consume 3.96 mW in the carrier resistor paths. Such simultaneous closures are electrically permitted but semantically invalid station evidence. This excitation may not penetrate oxide on power-switch contacts; a missed closure is not proof that a contact fails at its rated operating current. Reserve 100 mA at 5 V for an initial USB system budget and measure the actual assembly; the external debugger and MCU dominate consumption. No battery runtime is claimed.

Budgetary allowances are USD 25 for the Nucleo, USD 5 for passives and connectors, USD 10 for allocated PCB cost and USD 20–40 for switches, harness and mechanics. These are planning assumptions, not distributor quotations or verified availability. Component substitutions need electrical and mechanical review, especially contact material, connector orientation and switch travel.

The platform references are ST's [UM1724 MB1136 manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf) and [RM0368 F401 reference manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf). Fresh PDF downloads timed out during this run; this revision reuses the repository's inspected F401/MB1136 platform, not a newly claimed manufacturer audit. Before assembly, verify the actual Nucleo revision, CN6/CN8 orientation and ST-LINK USART2 solder bridges against those manuals. Retain the documented VCP routing rather than attaching a second driven transmitter to PA2.

## Firmware behavior and decisions

The target firmware is freestanding C11 with its own vector table, reset handler, linker script and register-level drivers. Clock startup restores HSI and bus prescalers with bounded iteration waits before initializing the external interface. GPIO configuration is checked. SysTick only increments a volatile timestamp; the foreground loop owns all engine state. The watchdog is started before acquisition and fed after sample progress. A failure of input configuration or watchdog initialization enters a visible terminal fault rather than publishing a normal-looking baseline.

Qualification operates on the entire four-bit signature. This is intentional: independently filtered overlapping inputs could manufacture a one-hot code that never physically existed. A raw change resets qualification age; five subsequent one-millisecond intervals with the same raw vector accept it. Chatter on any channel can delay the whole vector. Short raw overlap or all-open pulses below that threshold are not classified as journeys or faults. This is not a high-speed bounce analyzer. Unsampled motion cannot be reconstructed.

The first qualified singleton anchors its own station identity without creating a journey. A qualified all-open vector after an anchor starts a transit, retaining that origin and setting elapsed transit age to zero. The next qualified singleton completes it. Equal identity increments returns. The next identity in positive ring order increments CW; the previous identity increments CCW. Opposite identity increments jumps, without assigning two steps or a direction. The latest completed origin, destination and duration remain unchanged until a later completion, even if subsequent observations are aborted.

Adjacent movements retain a direction and a run remainder. A change between adjacent directions increments reversals and starts the new run at one. Each four consecutive adjacent movements in the same direction increments that direction's route-lap counter and clears the remainder. A same-station return, opposite jump, shortcut, overlap, timeout or sample gap breaks the run. Thus moving clockwise twice and counterclockwise twice is not a lap. A lap is four observed adjacent transitions, not proof of one physical revolution.

A direct singleton-to-different-singleton change without qualified all-open departure increments shortcuts and establishes the new station as an anchor, but creates no journey outcome. A qualified multi-contact vector increments overlaps and abandons any active journey. A held unchanged overlap is counted once; a different qualified multi-contact signature counts another overlap observation. A later singleton may re-anchor. These counters identify evidence limitations, not a diagnosis of which mechanical part failed.

The transit deadline rejects at age 10000 before evaluating an arrival qualifying on that same sample. An arrival at 9999 can complete; one at 10000 cannot. Timeout increments aborted and timeouts and removes the origin. If a singleton qualifies on the timeout sample, it can become a new baseline but cannot complete the expired journey. Sample intervals other than exactly one millisecond similarly abandon active evidence, increment gaps, reset qualification and require a new stable signature. A subsequent singleton can recover relative observation; no fabricated edge bridges a gap.

Every counted start is either CW, CCW, return, jump, aborted or currently active. Accounting transitions are staged in a copied engine and committed together, so counter exhaustion cannot publish half a completion. Exhaustion enters terminal fault and abandons the previously active journey exactly once. Diagnostic transmission losses and gaps saturate rather than wrap. Sequence and device timestamps use unsigned modular arithmetic; observations must be separated by much less than half a 32-bit timer cycle for host ordering inference. Reset clears all device history; no flash-backed log or persistent boot identity is implemented.

## Protocol and companion software

Every status record begins with DR1 and contains thirty unsigned decimal fields followed by an asterisk, four uppercase hexadecimal CRC digits, and LF. The fields are:

```text
sequence, now, fault, raw, qualified, ready, station, active, origin, age,
starts, cw, ccw, returns, jumps, aborted, reversals, cw_laps, ccw_laps,
overlaps, shortcuts, timeouts, gaps, tx_errors, direction, run,
last_valid, last_from, last_to, last_ms
```

CRC-16/CCITT-FALSE covers the complete payload from D through the last decimal digit, excluding the asterisk. It uses initial 0xffff, polynomial 0x1021, no reflection and no final xor. The standard check string 123456789 gives 0x29b1. Station value four means unknown; direction zero means no retained direction, one CW and two CCW. Fault bits are configuration=1, watchdog=2, counter exhaustion=4 and invalid software input=8. Ready says a full signature has qualified, not that the mechanism is safe. Raw and qualified values may differ during qualification.

Firmware attempts a snapshot every 100 ms when the UART is free. A bounded 768-byte transmit buffer drains without blocking acquisition. A stalled transmission is discarded at 100 ms and counted; the app can resynchronize at a complete DR1 marker. There is no command receiver or pairing mechanism. CRC protects against accidental corruption, not a malicious sender. Only use trusted local serial devices and inspect recordings before relying on them.

Run `python3 app/workbench.py` from this device directory on Linux with Python/Tk. Connect / Live opens a chosen serial path at 115200 8N1, displays route counters and shows provenance, connection and freshness. History / Export retains up to 2000 snapshots, not an exact list of every transit. Setup / Limits explains installation and interpretation. Replay accepts a regular file up to 2 MiB and is always marked NOT LIVE. `firmware/build/demo.dr1`, generated by the real C encoder, contains synthetic observations, not measurements from a built device.

The parser checks CRC, numeric formatting, field count, bounds and accounting conservation. Session checks reject counter rollback, contradictory duplicate sequences, unsupported timestamp ordering, changing retained results without new completions and impossible sparse completion classifications or durations. These are necessary consistency constraints, not a reconstruction of every hidden event between snapshots. Sparse records can conceal intervening valid or invalid motion that the delivered counters cannot uniquely identify.

LIVE requires an open connection and two advancing timestamps. No progress for two seconds is stale. A host reception pause beyond two seconds or cumulative device-versus-host progress disagreement beyond two seconds blocks further acceptance and requires explicit reconnect after export. The fixed allowance is deliberately conservative: oscillator drift can eventually force a reconnect even without a transport defect. Duplicate frames do not renew freshness. Some resets with forward-looking timestamps and counters remain indistinguishable because there is no persistent boot identifier.

Disconnect preserves history and its original live/replay provenance. Opening a successful new session replaces previous evidence; failed connection setup does not erase it. The app refuses a second simultaneous connection. Serial settings are restored on close and setup failure where the operating system permits it. CSV export uses exclusive creation with requested owner-only permissions, refuses overwriting, and includes a metadata row even when no measurements exist. Author, source, connection, blocking, freshness, parser errors, sequence losses and warnings accompany the retained snapshots. Data stays local unless deliberately shared. Workflow timestamps may still reveal operator activity, so obtain consent and choose a retention policy.

## Reproducible build and validation

From this directory run:

```sh
make -C firmware clean all test fixture
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 tools/verify.py
```

The firmware command links the actual Cortex-M4 target, creates a binary, and runs host tests under address and undefined-behavior sanitizers. The route oracle enumerates all six-destination words over four stations and independently checks directional counts, reversals and lap remainders. Boundary tests cover qualification, deadline precedence, overlap, missing clear departure, acquisition loss, timestamp wrap, exhaustion and encoding capacity. Python tests consume real C-generated fixtures, exercise framing and impossible records, use actual Linux pseudo-terminals and run the real Tk event loop under Xvfb. No npm package installation or runtime cloud dependency is needed.

The KiCad 9 generator is `tools/generate_hardware.py`; it requires distribution pcbnew bindings and installed footprint libraries. Regenerate only intentionally, then run the hardware checker. The checker executes actual ERC and DRC, exports a fresh schematic XML netlist, and compares each component's value and footprint plus every pin/net assignment against PCB pads and the connectivity manifest. The schematic has also been exported and visually inspected. Passive-board ERC does not validate the external controller, cable assembly or sensor mounting.

`make -C firmware flash` invokes OpenOCD with ST-LINK and the STM32F4 target, requesting program, verify and reset. This physical operation was not performed. Local SWD reflashing is the recovery mechanism; there is no OTA update, signed bootloader or rollback storage. Only program the specified controller using a trusted locally built image. See VALIDATION.md for executed results and limitations.

## Commissioning, safety and remaining work

Inspect unpowered continuity, component orientation and resistor values before connecting the Nucleo. Apply USB power with signal leads disconnected and verify 3.3 V and individual open/closed sense voltages. Remove power before completing the harness. Exercise each switch separately, confirm its station identity, then trace clockwise, counterclockwise, return and opposite routes slowly. Introduce overlapping closures, a direct station change, a prolonged all-open transit and a disconnected USB cable. Compare the display and exported record with independent observations before studying a mechanism.

A broken wire generally resembles an open switch; a short resembles a permanent closure. A stuck singleton may look like a stationary mechanism. There is no diagnostic excitation to distinguish these conditions. Run a manual channel test before every session. Validate timing with an independent logic analyzer and compare route results with video during supervised trials. Check that switch force, magnet attraction or the witness disk does not materially change the mechanism's behavior.

This design has no galvanic isolation, surge barrier, reverse-polarity protection, certified ESD protection or safety integrity rating. One kilohm is not protection against mains or industrial 24 V signals. Never attach energized controls, machine interlocks, exposed mains wiring, externally driven outputs or stored-energy mechanisms. Use only verified voltage-free contacts on a low-energy manual fixture. No clean software test or CAD report establishes electrical safety, EMC immunity, angular accuracy, durability or regulatory compliance. Physical fabrication, programming, calibration, enclosure review and user-value testing remain explicit next steps before any deployment beyond supervised prototyping.
