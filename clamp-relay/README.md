# Clamp Relay — transfer-overlap evidence for manual workholding

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is an engineering prototype. The custom PCB is a routed passive contact interface for a separately purchased NUCLEO-F401RE. The processor, regulator, debugger, USB interface and processor decoupling belong to that purchased board. No physical assembly has been fabricated, flashed, calibrated or tested. Source-level tests and CAD checks are not evidence of holding force, mechanical safety, or measurement accuracy.

## Purpose and practical value

Clamp Relay helps a small workshop compare the way a flexible strip is handed from one manual clamp to another. A typical experiment uses two soft-jaw clamps to support a strip of paper, thin plastic or fabric while its accessible working region changes. The fixture designer wants to know whether the receiving clamp establishes its position witness before the sending clamp releases, whether both witnesses disappear briefly, and whether the operator simply returns to the original clamp instead of completing the transfer. A phone stopwatch cannot record those hidden contact relationships reliably.

The proposed instrument watches four independent voltage-free contacts: the jaw endpoint and material presence at clamp A, and the corresponding pair at clamp B. An effective holding witness means that both contacts for that clamp are qualified closed. This wording is intentional. The device does not measure clamp force, friction, tension, actual specimen restraint or the strength of a joint. A jaw can reach its switch and still grip poorly. A presence flag can remain displaced by scrap material. The useful output is a record of observed mechanical indications, not permission to lift, machine or release a workpiece.

The target users are fixture makers, craft studios, educational laboratories and makerspaces investigating low-energy manual handling. A day-one experiment compares two jaw-pad geometries with the same inexpensive sacrificial strip. The designer performs several transfers in each direction, deliberately tries an early release, then exports the observed gap and overlap durations. If one design requires awkward hand movement and repeatedly loses both witnesses, that is a reason to inspect the fixture and procedure. It is not a reason to rank an employee or claim a safety incident was prevented.

There is a physical reason for dedicated hardware: the contacts must be sampled on one clock near the fixture, independently of desktop event-loop scheduling. The carrier establishes repeatable biasing and labeled wiring. The MCU runs the experiment with the companion closed and retains cumulative counts plus the current or latest trial in volatile memory. The app adds interpretation, a bounded snapshot history, recording replay and export. This revision has no local numerical display; offline operation means continued acquisition, not an unattended human alarm.

## Originality and alternatives

The repository directories and README introductions were inventoried before selecting this concept. Nest Settle studies four-corner arrival, seating and subsequent corner dropout within one locating nest. Fold Trace checks the order and dwell of four folding leaves. Ratchet Audit measures encoder rollback relative to a stroke and pawl indication. Clamp Relay instead combines two different witness types per clamp and observes reversible ownership transfers, including both-held and neither-held intervals. Those existing designs do not provide this experiment or its evidence contract. The Nucleo board support, passive electrical platform and CAD checking approach are reused from the repository with jayis1 attribution preserved. Reusing an appropriate interface is preferable to inventing a different power circuit merely to look different.

Industrial clamp sensing is established prior art. DESTACO's manual power clamps offer position sensing, and its 8051/8331 pneumatic latch-clamp announcement describes open/closed monitoring. These are important alternatives, not proof of absolute novelty. A properly integrated PLC and two industrial clamp sensors may be the right solution in a production cell. A multichannel logic analyzer can also capture the electrical transitions, while video can show mechanical motion the contacts miss. Clamp Relay's proposed distinction is a low-energy, portable, camera-free workholding-transfer notebook with separate material-presence witnesses, explicit unsupported-witness intervals, unresolved direct swaps, returned trials and lost-acquisition reporting.

A bounded public search does not establish patentability or prove that no similar device exists. The contribution claimed here is an original repository implementation and a specific useful product proposal. No industrial safety equivalence is claimed. Candidate ideas rejected during selection were a four-flap folding recorder, which duplicated Fold Trace, and a four-corner placement recorder, which duplicated Nest Settle.

Prior-art and implementation references:

- DESTACO manual power clamps: https://www.destaco.com/clamping/pneumatic-clamping/manual-power-clamps
- DESTACO sensor-ready latch clamps: https://www.destaco.com/company/news/introducing-the-new-8051-and-8331-series-pneumatic-pull-action-latch-clamps-from-destaco
- STM32F401 reference manual RM0368: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Nucleo MB1136 user manual UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf

## Hardware specifications

| Function | Revision A contract |
| --- | --- |
| MCU assembly | NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Processor | Cortex-M4, nominal 16 MHz internal HSI |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Input sensors | Four external normally-open, voltage-free signal contacts |
| Example switch | Omron D2F-01L, subject to fixture force/travel validation |
| Input network | 10 kohm pull-up and 1 kohm series resistor per contact |
| Acquisition | Nominal 1 ms polling; independent 5 ms qualification |
| Initial/final stability | 50 ms of one exclusive qualified holding witness |
| Trial bound | 10,000 ms; timeout wins a simultaneous settlement |
| Serial | USART2 transmit only, PA2 AF7, 115200 baud, 8N1 |
| Host transport | Nucleo ST-LINK virtual COM port |
| Power | USB into Nucleo; carrier takes regulated 3.3 V and ground |
| Carrier | 80 by 70 mm, two copper layers, 1.6 mm board thickness |
| Assembly | Through-hole resistors and 2.54 mm pin headers |
| Companion | Python 3 and Tk on Linux; no downloaded Python packages |

The delivered board is not a mechanically mating Arduino shield. Use short, labeled jumpers and mount the Nucleo separately. The design intentionally has no wireless radio, battery charger, motor drive, relay output or connection to machinery. A certified USB supply and a conventional 5 V, 500 mA source allowance are conservative planning assumptions for the development board and debugger, not measured power consumption.

### Exact pin and connector contract

| Carrier terminal | Meaning and destination |
| --- | --- |
| J1.1 / J1.2 | VCC / GND, Nucleo CN6.4 3V3 and CN6.6 ground |
| J2.1 SENSE0 | PA0, A0, CN8.1; A jaw endpoint |
| J2.2 SENSE1 | PA1, A1, CN8.2; material at A |
| J2.3 SENSE2 | PA4, A2, CN8.3; B jaw endpoint |
| J2.4 SENSE3 | PB0, A3, CN8.4; material at B |
| J3.1 / J3.2 | JAW_A / GND |
| J4.1 / J4.2 | SHEET_A / GND |
| J5.1 / J5.2 | JAW_B / GND |
| J6.1 / J6.2 | SHEET_B / GND |

Each switch closes its signal to ground. Firmware converts this active-low voltage into a logical closed bit. The four bit weights are 1, 2, 4 and 8. A holding witness uses bits 1 and 2 together; B uses bits 4 and 8 together. The protocol's separate grips mask has A=1 and B=2. Thus levels 3 means A only, levels 12 means B only, and levels 15 means both. Other levels can also yield an exclusive holding witness; for example, levels 7 includes A holding and an empty B jaw indication. The engine intentionally uses the combined witnesses rather than demanding that unused contacts all be open.

Preserve the Nucleo's default ST-LINK VCP routing: SB13/SB14 fitted and SB62/SB63 open where specified for the board revision. Confirm the actual MB1136 revision against UM1724 before wiring. PA2 is USART2 TX, not a carrier input. Receive is disabled; the app cannot change thresholds, erase MCU counters or actuate anything. Programming uses the onboard ST-LINK SWD interface, not the carrier connectors.

## Architecture and block diagram

    A jaw endpoint --- J3 -- JAW_A --- 1k --- SENSE0 --- PA0
    A material flag -- J4 -- SHEET_A - 1k --- SENSE1 --- PA1
    B jaw endpoint --- J5 -- JAW_B --- 1k --- SENSE2 --- PA4
    B material flag -- J6 -- SHEET_B - 1k --- SENSE3 --- PB0
                                             |
                                        10k each to 3V3
                                             |
    USB --> NUCLEO-F401RE ------------------- J1
                 |
                 +--> 1 ms samples --> independent qualification
                 |                  --> combined holding witnesses
                 |                  --> transfer state and durations
                 +--> CRC-framed UART --> ST-LINK VCP --> Linux/Tk
                                                       Connection
                                                       Live witnesses
                                                       History / export

The schematic contains actual locally authored connector and resistor symbols with pin numbers, wires, labels, references, values and footprint identities. The PCB contains actual through-hole footprints, assigned pad nets, routed tracks, a ground via, and a closed board outline. A manifest and verification script compare every reference/pin/net and component value between a freshly exported schematic netlist and the PCB. Passive ERC cannot establish that an external Nucleo supply or mechanical fixture is wired correctly.

## Electrical budget, BOM and assembly

At nominal 3.3 V a closed contact draws 0.30 mA through 11 kohm. The MCU-side low level is nominally 0.30 V. Four closed contacts draw 1.20 mA and dissipate 3.96 mW in the resistor networks. These calculated values ignore resistor tolerance, contact resistance and rail error. During bring-up, measure actual high and low voltages against the MCU's documented limits. Use low-level signal contacts; a power switch that depends on substantial wetting current may be unreliable here.

| Reference | Part or specification | Quantity |
| --- | --- | --- |
| External MCU board | NUCLEO-F401RE | 1 |
| R1, R3, R5, R7 | 10 kohm, 1%, axial resistor, 7.62 mm lead pitch | 4 |
| R2, R4, R6, R8 | 1 kohm, 1%, same footprint class | 4 |
| J1, J3, J4, J5, J6 | Two-pin 2.54 mm vertical header | 5 |
| J2 | Four-pin 2.54 mm vertical header | 1 |
| External witnesses | Four low-force signal switches and adjustable brackets | 1 set |
| Custom board | Supplied two-layer passive interface | 1 |
| Mechanical support | Insulating tray, strain relief, cables, two manual clamps | 1 set |

Allow roughly USD 60–120 for a one-off experimental assembly, excluding labor and existing clamps. At quantity 100, a USD 40–80 electronics-and-contact planning range is a design target, not a quotation. The Nucleo, four quality switches, and enclosure/bracket set are likely to dominate cost; procurement and fabrication quotes are still required. A future dedicated MCU PCB could reduce recurring cost but would need its own power, programming, EMC and manufacturing review.

The input resistors are not certified overvoltage or ESD protection. There is no galvanic isolation or TVS array. Connect only disconnected, voltage-free contacts with no other equipment attached. Never connect a PLC output, industrial powered proximity sensor, mains terminal, powered machine switch or unknown field cable. Keep initial leads below approximately half a meter, indoors and dry. USB connects the fixture ground to the computer. Never feed an independent 3.3 V supply into J1 while the Nucleo is attached.

## Mechanical fixture and first useful experiment

Use a lightweight sacrificial strip above a bench or catch tray. The clamps must carry load through their own structures and stops, never through the witness switches. Install a jaw switch that indicates the chosen endpoint without limiting overtravel. Install a separate low-force material flag near each jaw, positioned so the specimen displaces it when inserted. Confirm that the flag does not meaningfully support the strip or create enough friction to change the transfer behavior. The exact brackets depend on clamp geometry and are not universal mechanical production drawings.

Mount the carrier in an insulating edge-support tray; revision A has no mounting holes. Allow approximately 110 by 100 by 45 mm for the carrier enclosure plus separate Nucleo space and USB cable clearance. Protect the solder side. Add strain relief near every switch and at the electronics enclosure. Label A/B and jaw/material at both ends of each lead. No cable should cross a hand-clearance area or be capable of pulling a clamp closed.

After flashing and connecting, inspect all four live indicators individually. Open both jaws and remove the specimen. Verify that every contact returns open. Insert the sample in A, close A, and wait for the qualified A owner. Close B over the sample before opening A. After B-only stability, the latest trial should show completion with overlap. Repeat in reverse. Deliberately open A before establishing B to demonstrate a gap trial. Finally, release and reclose the original clamp to demonstrate a returned trial. These are proposed physical acceptance exercises, not tests already performed on hardware.

For comparative trials, record pad material, clamp spacing, sample material, bracket settings and operator procedure externally with the exported data. Keep switch adjustment unchanged between alternatives. A broken lead is indistinguishable from an open contact; a shorted lead resembles a closed one. A presence witness cannot establish that the material in both clamps is the same piece. If any of these ambiguities matters, this design is insufficient and the sensing system must be upgraded.

## Firmware state and measurement contract

The firmware is freestanding C11, without an allocator, operating system or downloaded vendor HAL. Startup initializes RAM and installs the core exception and SysTick vectors. The linker describes the STM32F401RE flash and SRAM. The clock driver restores the reset HSI clock tree using bounded polling before configuring peripherals. Configuration checks verify that the carrier pins remain inputs and the UART remains transmit-only. An independent watchdog is enabled after initialization and fed from acquisition progress. A clock-start failure stops before interface setup; a watchdog/configuration failure becomes a terminal diagnostic rather than valid transfer evidence.

Each channel independently accepts a new level after five milliseconds of unchanged consecutive samples. Shorter raw excursions are deliberately rejected. Contact qualification delays edge times and cannot prove continuous raw mechanical restraint. The initial baseline requires all four channels qualified, then one exclusive combined witness stable for 50 milliseconds. Startup while both or neither holds does not invent an owner. Acquisition with a skipped millisecond aborts an active trial, increments the gap diagnostic, clears ownership and restarts qualification. No skipped interval is counted as known gap or overlap.

A trial starts when the qualified combined witness leaves its accepted exclusive owner. While active, the previous qualified grip state accounts each observed one-millisecond interval. Neither witness adds uncovered time; both add overlap time; exclusive states contribute only to elapsed time. Consecutive neither-held time builds the longest-gap metric. The opposite exclusive witness stable for 50 milliseconds completes the trial. The original exclusive witness stable for the same duration returns and aborts it. Completion establishes the new owner so a reverse transfer can follow without a reset.

A direct exclusive-A to exclusive-B change starts a trial but marks ordering unresolved. It must not be interpreted as proof of zero physical gap. Completion is still a directional observation, not a safe-transfer score. A 10-second timeout takes precedence over settlement on the same tick. Gap, timeout, return and terminal faults are separate retained results. Durations freeze after a terminal result until the next trial starts. Cumulative starts equal completed plus aborted plus one if active. Counters never wrap; exhaustion ends correlation in a fault while preserving that conservation relationship.

## Protocol and companion

Every periodic record repeats the policy so late attachment does not depend on a single boot announcement. The line is ASCII, LF terminated, with CRC-16/CCITT-FALSE over the bytes preceding the asterisk. The final four uppercase hexadecimal characters are the checksum. The format is:

    CR1,qualify,settle,limit,now,levels,qualified,grips,owner,result,direction,starts,completed,aborted,elapsed_ms,gap_ms,overlap_ms,max_gap_ms,direct,gaps,tx_errors,faults*CCCC

Policy values are 5, 50 and 10000 milliseconds. All numbers are unsigned decimal. `now` is a wrapping 32-bit millisecond count. Results are 0 none, 1 active, 2 completed, 3 returned, 4 timeout, 5 acquisition gap and 6 terminal fault. Direction 1 means an A-origin trial; 2 means B-origin. Fault bits are 1 input configuration, 2 watchdog setup and 4 counter exhaustion. The stream is approximately ten records per second. UART transmission is nonblocking, bounded and subject to a 100 ms timeout. Truncated records can resynchronize at a subsequent complete `CR1,` marker. CRC detects accidental corruption, not source identity or hostile forgery.

Run `python3 app/relay.py` from this folder. The Connection screen opens a Linux serial path at 115200 baud and flushes old input. The Live witnesses screen displays qualified contacts, current transfer result and measured intervals. The History / export screen shows up to 2,000 retained periodic snapshots; these rows are not unique trials. Faster completed trials can be absent from this history while still contributing to device cumulative counters. Export creates a new private CSV file, refuses overwrite and includes author, source, connection, freshness, framing-error and warning metadata even if no trial completed.

Replay accepts bounded regular CR1 text recordings, not CSV exports. Files above 2 MB, FIFOs, empty files, malformed records and truncated final frames are rejected. Replay is labeled separately from live capture. Disconnect retains source provenance and evidence. Two advancing device timestamps are required before live freshness; duplicate timestamps do not renew it. Counter rollback, contradictory duplicate records and excessive host/device progress disagreement block the session until reconnect. Drift tolerance is capped, so sufficiently long captures with an uncalibrated HSI clock can conservatively require reconnect. There is no persistent boot identifier: resets with plausible forward time and counters can remain indistinguishable. Recordings must not be treated as authenticated or uninterrupted audit evidence.

## Building, verification and remaining work

From this device directory run:

    make -C firmware all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v
    /usr/bin/python3 tools/check_hardware.py
    python3 -m py_compile app/protocol.py app/relay.py

The default firmware target produces a real Cortex-M4 ELF and binary using `arm-none-eabi-gcc`; host tests exercise the actual engine and encoder with sanitizers. The app tests use a Linux pseudoterminal and real Tk event loop under Xvfb, not a physical USB device. CAD validation uses KiCad 9, its installed footprint libraries, and its Python bindings. `tools/generate_hardware.py` reproducibly regenerates the carrier; normal verification does not require regeneration. `make -C firmware flash` uses OpenOCD and a physically attached ST-LINK, and is intentionally not part of automated verification here.

See VERIFICATION.md for the measured build sizes, counts and checks for this revision. Fabrication, target programming, electrical threshold measurement, oscillator calibration, mechanical repeatability, contact contamination, ESD behavior and workshop usefulness remain unverified. There is no certification for industrial machinery, medical use, lifting, personal protection or product inspection. Do not use the device for suspended loads, hazardous tooling, irreplaceable objects or any decision that depends on guaranteed gripping.

The next development stage should assemble a catch-tray fixture, compare contact traces with slow-motion video, measure switch-force disturbance, verify 3.3 V input thresholds and deliberately interrupt cables and USB power. A pilot should compare deliberately scripted overlap, gap, direct-swap and return trials against external observation before anyone interprets small timing differences. Only then should work proceed toward enclosure refinement, protected inputs or a dedicated processor board. Privacy is simple by design: no camera, radio, cloud account or automatic upload; information remains in MCU RAM and local host files until the operator explicitly shares it.
