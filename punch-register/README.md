# Punch Register — registration-through-stroke notebook for manual paper punches

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. The custom PCB is a routed passive contact interface for a separately purchased NUCLEO-F401RE. Its processor, regulator, USB bridge, debugger and processor decoupling live on that purchased board. No assembly has been fabricated or flashed, no paper has been punched during validation, and no dimensional accuracy or operator-safety claim is made. Software execution and electrical-rule checks are evidence about these deliverables, not physical qualification.

## Purpose, audience and practical outcome

A bookbinder making short runs of punched covers places a stack against two registration stops, operates a manual lever, and removes the stack. A crooked hole pattern can result from incomplete initial insertion, a stack pulling away during the stroke, an incomplete stroke, or a second operation on the same stack. Inspection of the finished cover exposes the defect but does not necessarily identify which part of the hand movement caused it. Punch Register adds a removable contact-witness fixture that distinguishes those observed sequences without filming the operator or modifying the punch's cutting mechanism.

The instrument observes two low-force registration followers, one handle-home switch and one bottom-of-stroke switch. It records whether both registration contacts were seated for a minimum interval before handle departure and remained closed until return. It also distinguishes a missing bottom endpoint, repeated bottom visits, contradictory endpoints, excessive stroke duration and incomplete acquisition. The useful output is a small process-development notebook: compare a straight registration fence against a relieved fence, compare stock stiffness, or discover that a witness spring is itself pulling thin paper out of registration.

Target users are bookbinding studios, printmaking workshops, educational fabrication rooms and makers building slow manually operated paper fixtures. The device does not authorize a stroke, stop a lever, count individual sheets, inspect a hole, or measure punch force. A qualifying empty-fixture manipulation can produce a good observation. A shorted registration switch can imitate contact with paper. Consequently, the words good and bad classify the electrical sequence only. They must not become acceptance labels for finished products or assessments of an operator's competence.

The practical hypothesis is that a workshop can attribute otherwise ambiguous bad registration to before-stroke versus during-stroke behavior with a low-cost fixture. That hypothesis needs a physical pilot. Suggested acceptance work includes deliberately underseating one corner, withdrawing a corner during the stroke, stopping before bottom, returning twice to bottom, and then comparing the recorder with video and measured hole positions. No successful pilot is claimed here.

## Originality and alternatives

The repository's device folders and README concepts were inventoried before this addition. Fold Trace checks an ordered sequence of four folding leaves; Punch Register instead observes two simultaneous registration obligations spanning one handle cycle. Nest Settle studies all-corner seating and reseating without a separate tool operation. Sand Stroke records face-separated traversals, while Clamp Relay studies overlapping workholding transfer. These are related measurement techniques, but none has this paper-registration-through-punch contract. The electrical carrier and STM32 support code are deliberately reused from Merge Rhythm, with jayis1 attribution preserved. A new product does not require pretending that its UART or resistor network was invented again.

Prior art makes broad novelty claims inappropriate. The published application “Hole punch with alignment indicator,” US application 20140150617, describes visual alignment indicia and physical stops for a manual paper punch. Konica Minolta's Punch Adjustment documentation describes paper-edge detection and registration adjustments in automated finishing equipment. Registration aids and sensed paper alignment therefore already exist. This proposal's specific contribution is a removable, receive-only, independently clocked observation kit that retains a seating obligation through a manual stroke and separates extra strokes from new stack trials. The bounded search is not a patent search or freedom-to-operate opinion, and worldwide uniqueness is not established.

A phone video may be the better tool for an occasional investigation. It can reveal actual paper motion, but hands and the punch body often obscure the rear stops. A counter on the handle cannot distinguish a well-seated departure from a withdrawal during the stroke. A PLC can implement this exact policy and is preferable where industrial installation, protected inputs or validated integration already exists. This prototype is intended for a disconnected low-voltage bench fixture, not as a cheaper substitute for a machine safety controller.

References consulted for the concept and platform:

- https://patents.justia.com/patent/20140150617
- https://kmbsmanuals.konicaminolta.us/AccurioPressC3080/contents/ADJ_103854885.html
- NUCLEO-F401RE board and connector reference, ST UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 peripheral reference, ST RM0368: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

## Hardware specifications

| Function | Revision A design |
| --- | --- |
| MCU | STM32F401RET6, Cortex-M4, on NUCLEO-F401RE MB1136 |
| Memory | 512 KiB flash, 96 KiB RAM |
| Clock | Internal HSI restored to nominal 16 MHz before initialization |
| Sensors | Four normally-open, voltage-free, low-level mechanical contacts |
| Sensor roles | Left registration, right registration, bottom endpoint, home endpoint |
| Acquisition | Nominal 1 ms polling, independently qualified 5 ms changes |
| Serial | USART2 TX, PA2 AF7, 115200 baud, 8N1, ST-LINK virtual COM |
| Supply | USB to Nucleo; its regulated 3.3 V and ground feed the carrier |
| Carrier | 80 by 70 mm, two copper layers, 1.6 mm nominal FR-4 |
| Interface electronics | Four 10 kohm pullups and four 1 kohm series resistors |
| Assembly | Through-hole axial resistors and 2.54 mm headers |
| Companion | Local Linux Python 3 / Tk application; standard library only |

This is a complete passive carrier design, not a bare-MCU PCB. J1 is a two-pin power header: pin 1 goes to Nucleo CN6 pin 4, 3V3; pin 2 goes to CN6 pin 6, ground. J2 pins 1, 2, 3 and 4 connect respectively to CN8 pins 1, 2, 3 and 4, corresponding to PA0, PA1, PA4 and PB0. These analog-header pins are configured as digital inputs. J3 is LEFT, J4 RIGHT, J5 BOTTOM and J6 HOME. On each contact header, pin 1 is the signal and pin 2 is ground. These mappings are shared by board.h, the GPIO driver, schematic, manifest and PCB.

Each SENSE input has a 10 kohm pullup to 3.3 V and a 1 kohm resistor between it and the remote contact. A closed switch therefore places approximately 0.30 V at the MCU input, with approximately 0.30 mA through that branch. Four simultaneous closures draw approximately 1.20 mA. This is a low wetting current: use a contact specified for low-level signals and validate its behavior with the intended cable and environment. An oxidized power-switch contact is not an equivalent substitute.

The default ST-LINK virtual-COM bridge configuration is required: SB13 and SB14 fitted and SB62 and SB63 open, subject to verification against the particular MB1136 revision. No external signal connects to the UART receiver. The supplied firmware never enables receive mode and accepts no host commands. Program through ST-LINK SWD using the target build described below. Physical programming is a later bring-up task, not part of the automated tests.

## Architecture and block diagram

    Left paper follower -- J3 -- LEFT ---- 1k -- SENSE0 -- PA0
    Right paper follower - J4 -- RIGHT --- 1k -- SENSE1 -- PA1
    Bottom lever cam ----- J5 -- BOTTOM -- 1k -- SENSE2 -- PA4
    Home lever cam ------- J6 -- HOME ---- 1k -- SENSE3 -- PB0
                                                   |
                                              10k pullups
                                                   |
    USB -> NUCLEO-F401RE 3V3 ---------------------- J1
              |
              +-> 1 ms acquisition -> four qualifiers -> trial engine
              |                                         |
              +-> USART2 queue <- CRC snapshot <--------+
                        |
                     ST-LINK VCP -> Linux notebook -> private JSON export

The firmware and app deliberately have different responsibilities. The MCU evaluates timing without relying on the desktop event loop. The app validates complete snapshots, detects obvious discontinuities, displays cumulative evidence and exports locally. It does not calculate new good strokes from serial arrival time. No radio, backend service, account, analytics, camera or network permission is required.

## Mechanical design and integration

Construct a removable rear fence with two independent flexure followers spaced approximately 60 mm apart for a medium-width cover. Those are starting fixture dimensions, not a universal punch fit. Each follower should project slightly beyond a rigid registration stop, close its contact as paper approaches, and then yield so the rigid stop—not the switch body—sets the insertion depth. A useful initial follower travel is 0.5 to 1 mm. Adjust the trip point with slotted brackets and lock the adjustment after calibration. The final switch force must be low enough that the softest test stock does not buckle or slide; this cannot be established from the electrical design.

Do not use a switch as the load-bearing stop. Thin sheets may close only one follower, curl around both, or contact a follower without reaching the intended registration plane. Compare the electrical state with direct inspection and a square reference stack before interpreting any results. Two contact points cannot prove that every sheet in a stack is aligned. Surface coatings, dust, curl, loose pages and worn fences remain uncontrolled variables unless the experiment explicitly controls them.

Fit separate cams or flags to a safe external region of the manual lever. HOME is closed only near the fully returned handle position; BOTTOM is closed only after the chosen full-stroke endpoint. Their physical operating regions must not overlap. Leave a substantial intermediate travel region with both open. Nothing should protrude into the cutting path, add a pinch point, impede the return spring or defeat the punch's guards. Use a blunt surrogate or disconnected fixture for initial switch adjustment. Never reach into the punch to adjust contacts during operation.

Mount the carrier and Nucleo independently in an insulating enclosure with stand-offs and strain relief. The board files have an 80 by 70 mm outline but no drilled mounting holes; use edge supports or revise the mechanics rather than drilling through unknown copper. Budget roughly 150 by 100 by 45 mm internal enclosure space for both boards and jumpers, then verify actual connector clearance before manufacturing. Keep signal leads short, preferably below 0.5 m, with each signal paired with its ground return. This is not a shielded industrial cable system or an outdoor instrument.

## Power budget, BOM and procurement

The design uses USB power rather than a battery, so there is no charging circuit or estimated battery-life claim. For enclosure and supply planning, reserve 200 mA at 5 V for the Nucleo and its debugger, plus the small carrier load. This is an allocation, not a measured maximum or a claim about every board revision. The carrier's worst static resistor-network current is calculable from the supplied topology. Actual USB current and regulator temperature must be measured during bring-up with all four contacts closed.

| Reference / assembly | Quantity | Selection |
| --- | --- | --- |
| External controller | 1 | NUCLEO-F401RE |
| R1, R3, R5, R7 | 4 | 10 kohm, 1%, axial 0.25 W, 7.62 mm pitch |
| R2, R4, R6, R8 | 4 | 1 kohm, 1%, axial 0.25 W, 7.62 mm pitch |
| J1, J3, J4, J5, J6 | 5 | 1 by 2, 2.54 mm male header |
| J2 | 1 | 1 by 4, 2.54 mm male header |
| External contacts | 4 | Low-force gold-contact switches; D2F-01L is an electrical candidate, not a qualified paper follower |
| PCB | 1 | Two-layer carrier from supplied KiCad files |
| Mechanics | 1 set | Insulating fence, flexures, cams, brackets, enclosure and strain relief |

Planning allowances in USD are $25 for the controller, $8 for a single carrier, $4 for passive components and headers, $12 for switches, and $15 for cables and enclosure: $64 excluding custom fixture labor, shipping and taxes. A hypothetical 100-unit allocation is $20, $2, $2, $8 and $8 respectively, totaling $40 each before labor. These are budget assumptions, not supplier quotations or checked availability. At this volume a dedicated MCU board could reduce cost, but that would require a new power, programming, EMC and manufacturing design; it is not silently substituted here.

There is no isolation, ESD suppressor, reverse-polarity protection or field-voltage conditioning on the passive carrier. The series resistors limit ordinary contact current, not arbitrary external fault energy. Use only disconnected voltage-free contacts. Never connect an energized punch, mains circuit, motor drive, PLC output or other ground domain. USB shares ground with the computer. The prototype is restricted to dry indoor bench use and has no IP, EMC, safety or product-compliance certification.

## Firmware behavior and decision contract

The state machine has WAIT, READY, ACTIVE, DRAIN and HALT states. On boot it starts in WAIT and refuses to infer a trial from whatever switch state happens to be present. A full empty baseline is HOME closed, BOTTOM open and both registration contacts open, with raw and qualified agreement for 100 ms. This moves the engine to READY. The operator may now load the stack. Both registration contacts and HOME must agree closed, with BOTTOM open, for 100 ms to establish seating eligibility.

A qualified falling HOME edge starts one trial in READY, even if seating eligibility is missing. Missing eligibility is retained as an early-departure reason; a later seating recovery cannot erase it. The seating accumulator is retained, but not advanced, during HOME's falling qualification while both registration inputs remain closed. This prevents the qualification interval itself from destroying a valid prior seating observation or awarding new seating time after raw handle departure.

During ACTIVE, any sampled raw or qualified registration release becomes a sticky lost-registration reason. This is intentionally more conservative than ordinary debouncing: one observed low sample can reject the trial even if it would not qualify as a new stable switch state. Qualified BOTTOM rises are counted, with a second rise marking repeated bottom travel. HOME and BOTTOM simultaneously closed, whether raw or qualified during the active stroke, mark a conflict. A qualified HOME rise finishes the trial, which is good only if one bottom visit occurred and no reason was recorded. Otherwise it is bad. Return with BOTTOM still closed is not accepted as clean ordering.

The stroke deadline is 30,000 nominal milliseconds from qualified departure. Timeout takes precedence over a same-sample return and produces an aborted trial. A sampling gap greater than 5 ms also abandons an active trial, invalidates qualification and returns to WAIT. Shorter intervals are credited by elapsed ticks; this is a sampled observer and cannot exclude a pulse that begins and ends between samples. Timestamp subtraction handles normal 32-bit wrap, but a complete unobserved timer cycle cannot be distinguished from continuity.

After completion the engine enters DRAIN. It requires another 100 ms empty baseline before READY. Extra qualified departures while DRAIN increment an extra-stroke diagnostic, not the new-trial counter. This is important: repeated punching on the same retained stack must not look like several separately registered stacks. Initial motion while WAIT is outside claimed coverage. A stack is inferred from an empty-to-loaded workflow, not identified by a tag; replacing one stack while preserving contact closures cannot be detected.

Cumulative accounting preserves starts = good + bad + aborted + active, where active is one only in ACTIVE. Counters are capped by a one-billion operational limit. Before an acquisition update that lacks diagnostic capacity, the engine abandons a pending trial and enters terminal HALT. Board-configuration and watchdog-start failures also halt. Periodic telemetry continues where possible, but no new experiment is admitted. A host cannot clear these states; reset creates a new volatile session. All evidence is RAM-resident and is lost on power removal.

The platform drivers establish the reset clock, read GPIO, check input direction and UART setup, drive a nonblocking transmit queue and service the independent watchdog. UART backpressure never waits inside acquisition. A transmit lasting 100 ms is discarded and counted. The host resynchronizes on the complete PR1 marker, so a truncated frame need not destroy the next complete record. Clock setup failure stops before external interface initialization because UART baud would otherwise be untrustworthy.

## Protocol and companion application

Every approximately 100 ms the MCU emits ASCII PR1 followed by twenty unsigned decimal fields, then an asterisk, four uppercase hexadecimal CRC digits and a newline. CRC-16/CCITT-FALSE covers the bytes from P through the final decimal digit, with polynomial 0x1021 and initial value 0xffff. There is no authentication: this is local evidence framing, not tamper-proof logging. The exact field order is:

    now, raw, mask, qualified, state, fault, starts, good, bad, aborted,
    extra, gaps, tx_errors, reason, elapsed, bottoms, last_reason,
    last_duration, seat_ms, empty_ms

Mask bits 0 through 3 mean LEFT, RIGHT, BOTTOM and HOME. A one means contact closed despite the low electrical voltage. State values are 0 through 4 in the order listed above. Reason bits are early departure, lost registration, no bottom, repeated bottom, endpoint conflict, timeout, gap and capacity fault. Fault bits identify configuration, watchdog and counter faults. Durations are milliseconds. Last duration and reason refer only to the most recently completed or abandoned trial; cumulative totals preserve older outcomes but not their individual details. An aborted duration extends through detection of abandonment, capped at 30,000 ms; it includes unknown time across an acquisition gap and must not be interpreted as continuously observed registration.

The companion has Connection / experiment, Live registration and Snapshot history screens. Connect to a selected serial path, observe contact qualification and counters, add a fixture/stock note, open a saved serial recording after disconnecting, or export evidence to a new JSON file. The app is intentionally receive-only. Policy settings are displayed but not editable; changing policy requires rebuilding and retesting the firmware. Mechanical calibration is documented rather than replaced by a misleading software calibration button.

Parser validation checks framing, checksum, decimal ranges and trial conservation. Session checks reject timestamp rollback, cumulative-counter rollback, contradictory duplicate timestamps, disappearing faults and mutated halted evidence. Two advancing snapshots are required before displaying LIVE. Data older than one second is stale. A live reception pause exceeding 1.5 seconds or an accumulated host/device progress difference exceeding one second requires explicit reconnect, protecting against some forms of queued stale traffic. This fixed drift allowance may reject long sessions with an uncalibrated clock. It is deliberately conservative, not a synchronization guarantee.

Without a persistent boot identifier, some resets with forward-looking timestamps and counters cannot be detected. Explicitly reconnect after a known reset and never merge separate power sessions as continuous evidence. Disconnect preserves the old source label and counters but removes live status. Export includes author, provenance, connection/freshness state, parser-error total, warnings, notes and up to 2,000 snapshots. Even an empty export retains metadata. Files are created with private permissions and exclusive creation, so an existing file is never overwritten. Intermediate individual strokes can be missed between snapshots; this is not an event-complete production database.

Replay is strict, bounded to a regular file of at most 2 MiB, and rejects empty recordings or unfinished tails. It is always labeled REPLAY and never becomes live. Serial transport is Linux termios with nonblocking reads; GUI work is performed in bounded polling callbacks. A failed replacement replay or connection preserves existing evidence. A second connection while the port is owned is rejected. On close, original terminal settings are restored where the operating system still permits it.

## Build, tests and bring-up

Requirements are GNU Arm Embedded tools, host GCC, Python 3 with Tk, KiCad 9 with its Python bindings, and Xvfb for headless GUI testing. The app needs no pip packages. Run from the repository root:

    make -C punch-register/firmware clean all test
    xvfb-run -a python3 -m unittest discover -s punch-register/tests -p 'test_*.py'
    /usr/bin/python3 punch-register/tools/check_hardware.py
    python3 punch-register/tools/check_quality.py
    python3 punch-register/app/workbench.py

The hardware generator is retained for reproducibility; rerunning it intentionally overwrites this device's generated CAD sources. The import script documents platform provenance and refuses to overwrite existing imported files. Neither is needed for normal firmware builds or application use. Firmware build products and local CAD preferences are ignored by Git. Check VALIDATION.md for the actual executed checks and tool versions rather than assuming every possible validation was performed.

After software checks, inspect the unpowered carrier for shorts, resistor values and header orientation. Confirm 3.3 V before attaching MCU signal jumpers. Test each contact separately with the cutter inaccessible, verify its named input in the app, then test all-open and HOME-only baselines. Only after mechanical review should a blunt surrogate exercise the intended sequence. Measure real current, signal levels, polling timing, switch force, cable noise and endpoint alignment. Repeat deliberately faulty trials and correlate with direct observation before relying on the notebook for process development. Fabrication, electrical bring-up, physical programming, fixture accuracy and usefulness remain future validation work.
