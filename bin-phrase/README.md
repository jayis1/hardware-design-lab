# Bin Phrase — tray-delimited bin-access word recorder

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, tests, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Shared startup, GPIO, clock, UART, passive carrier and serial transport infrastructure is imported from jayis1's Case Delta at a63a007. The grouped opening-word engine, BQ1 interpretation and application presentation are specific to Bin Phrase.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive interface for an external NUCLEO-F401RE development board, not a standalone MCU board. Target linking, synthetic contact tests, desktop workflows and electrical CAD checks are reproducible. Fabrication, flashing, electrical measurements, physical fixture construction, usability and field performance have not been verified. This is not a picking controller, safety monitor or certified production record.

## Purpose, overview and practical value

Bin Phrase records the order in which three storage-bin lids are opened while a removable receiving tray is seated. It preserves repeated visits rather than reducing the result to a set of visited bins or a count for each bin. It also preserves groups of openings qualified on the same acquisition sample without inventing a within-group order. Removing the tray with all lids already closed finishes the observed session. The most recent completed word remains available while another session runs or is abandoned.

An ordinary use is a makerspace preparing small demonstration kits from three bins of blunt, inert craft parts. An instructor wants to compare two arrangements of the bins: does placing spacers in the centre reduce returning to the first bin? A quantity checklist can confirm the final contents but cannot distinguish A,B,A,C from A,A,B,C. Video can recover that difference, but requires review, records people and may lose sight of the bins behind hands. A four-contact notebook gives an immediate, local, deliberately limited record of the physical access sequence.

For example, the displayed word A → {B+C} → A means that A opened, then B and C acquired their opening states on the same sample, then A opened again after a qualified closure. It does not mean two objects were taken from A. No source identity, material quantity, hand trajectory, picking success or actual deposit into the tray is measured. Opening an empty bin is still an opening. Holding a lid open while taking several objects produces one event. A user can substitute a different tray without identification if the sensor never observes absence. The terminology is therefore access word, not assembly recipe or verified kit.

The intended users are workshop educators, interaction-design students and makers comparing inexpensive storage layouts. Experiments should be consensual and supervised. Avoid employee scoring or identifying learners in exports. Only dry, nonhazardous demonstration objects belong in this revision. Medicines, allergens, food preparation, regulated manufacturing, hazardous reagents and safety-critical kits are excluded. Nothing is locked, driven, dispensed or electrically controlled by the device; it cannot stop a mistake.

## Distinction and originality limits

Existing repository directories and README titles were inventoried before selection. The relevant purpose and hardware sections of Kit Multiset, Scoop Window, Case Delta and Pantry Warden were inspected. Kit Multiset counts order-independent paddle declarations against a fixed multiplicity recipe. Scoop Window associates scoop absence with the opposite lid opening. Case Delta compares tool-pocket inventory at case closure and remembers returned pockets. Pantry Warden concerns shelf environmental condition. Bin Phrase instead stores an ordered, repeated, grouped lid-opening word delimited by a receiving tray. Neither a fixed count recipe nor inventory change is its result.

Pick-to-light stations, switches on bins, event loggers and sequence-learning systems are established ideas. This project proposes a particular transparent teaching fixture and an explicit evidence contract; it does not establish worldwide novelty, commercial absence, patentability or freedom to operate. An ordinary paper checklist may be preferable if only final quantities matter. A camera may be preferable when actual hand movement or object identity is required. Reusing a passive electrical platform is intentional and is not claimed as electrical invention. The new part is the bounded access-history experiment and its interpretation.

## Hardware specifications

The processor is an externally purchased ST NUCLEO-F401RE, MB1136 family, carrying an STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. Firmware restores the internal HSI reset clock to nominal 16 MHz. A SysTick interrupt supplies the one-millisecond software timebase. USART2 transmits at 115200 baud, eight data bits, no parity and one stop bit through the Nucleo ST-LINK virtual COM bridge. There is no firmware command receiver, network interface, radio, pairing workflow or cloud service.

The carrier is an 80 by 70 mm, two-copper-layer PCB with nominal 1.6 mm thickness. It contains fourteen components: six headers and eight axial resistors. The MCU, USB bridge, regulator, debugger, reset circuit and processor decoupling belong to the purchased development board. The passive carrier must not be powered or interpreted as a self-contained instrument. Programming uses the Nucleo onboard ST-LINK SWD connection; no second debug header is needed on this passive board.

Four external low-level voltage-free switches sense bin A open, bin B open, bin C open and tray present. The lid switches must CLOSE their electrical contact when the corresponding lid is OPEN. This polarity is intentional and differs from many closed-lid fixtures. Prefer mechanically protected lever switches suitable for low-current operation, with an independent hard stop carrying lid load. An Omron D2F-01L family low-level contact is a procurement starting point, not a verified endorsement of every suffix. Confirm minimum switching recommendations, force, travel and service life from the purchased component's datasheet.

### Wiring and bill of materials

| Logical bit | Signal | Field header | Sense header | Nucleo connection |
|---|---|---|---|---|
| 0 | OPEN_A | J3 pin 1 | J2 pin 1 | CN8.1, A0, PA0 |
| 1 | OPEN_B | J4 pin 1 | J2 pin 2 | CN8.2, A1, PA1 |
| 2 | OPEN_C | J5 pin 1 | J2 pin 3 | CN8.3, A2, PA4 |
| 3 | TRAY present | J6 pin 1 | J2 pin 4 | CN8.4, A3, PB0 |

Each field header's pin 2 is ground. J1 pin 1 takes regulated 3V3 from Nucleo CN6.4; J1 pin 2 takes ground from CN6.6. Never use 5 V or VIN. J2 carries input sense nodes, not supply outputs. J1 and J3 through J6 use two-pin 2.54 mm headers; J2 uses four pins at the same pitch. Insulated mating connectors, short paired signal/ground leads and strain relief are additional assembly requirements.

R1, R3, R5 and R7 are 10 kohm pull-ups from VCC to SENSE0 through SENSE3. R2, R4, R6 and R8 are 1 kohm series resistors from each sensing node to its field contact. Use one-percent DIN0207 axial parts, 7.62 mm lead spacing, rated at least 0.125 W. The GPIO driver inverts the low electrical level into a logical one. All four logical meanings, PCB net names and board.h connector comments agree with the table.

Calculated nominal current is 0.300 mA per closed 11 kohm path at 3.3 V. The corresponding SENSE voltage is 0.300 V. Four closed paths draw 1.20 mA, dissipating 3.96 mW in the resistor network. These values exclude the development board and host. Budget 150 mA at USB 5 V as an unmeasured development-board planning allowance, then measure the assembled unit. There is no battery, charger, battery protection or battery-life claim. No thermal claim beyond this low-power passive network is implied.

There is no galvanic isolation, reverse-polarity protection or dedicated cable surge/ESD protection. Series resistors are not safety barriers. Connect only disconnected voltage-free contacts and keep initial leads below 0.5 m, paired with ground. Never attach a PLC output, live machine switch, mains circuit or separately powered sensor. USB connects the fixture ground to the computer. An open cable resembles a released switch; a short resembles an asserted switch. Neither can be uniquely diagnosed by this design.

The connector mapping follows ST's UM1724 Nucleo-64 manual, while RM0368 defines the STM32F401 registers. Check the exact purchased board revision and solder bridges; the standard USART2 bridge uses SB13/SB14 fitted and SB62/SB63 open. Reference documents are https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf and https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf . This build does not require an external HSE clock.

## Mechanical fixture

Start with a rigid tabletop base approximately 450 by 300 mm, three labelled hinged bins along the back and a locating recess for a receiving tray at the front. Those dimensions are mock-up guidance, not manufacturing drawings. Give each lid a protected cam or tab that actuates its open-state switch before the user reaches the contents. Keep fingers away from pinch regions. A switch should not carry the weight of a lid, and its lever must not be bent beyond the manufacturer's travel limits.

Support the tray on three stable pads, with an independent tab operating its presence switch only at the seated position. Keep the tray sensor away from a normal hand-resting surface. Rocking or flexing can split a session; verify the actual tray with realistic load. The carrier has no mounting holes. Use insulating edge rails inside an enclosure with at least 100 by 90 mm internal clearance, not improvised drilled holes through copper. Secure the development board separately with USB and reset accessible. No ingress, impact, sanitation or environmental certification is claimed.

Commission all sixteen raw contact combinations using loose test switches before mounting the fixture. Then open each lid independently and check its logical bit. Close everything, remove the tray, insert it again, open A, close A, open C, close C, remove the tray and compare the retained word with an independent note. Test two lids together, repeated A visits and removal with a lid open. Compare switch evidence with observation; do not assume successful software simulation calibrates the mechanical fixture.

## Architecture and block diagram

```text
Lid A OPEN contact -- J3 -- 1k -- SENSE0 -- PA0
Lid B OPEN contact -- J4 -- 1k -- SENSE1 -- PA1
Lid C OPEN contact -- J5 -- 1k -- SENSE2 -- PA4
Tray PRESENT ------- J6 -- 1k -- SENSE3 -- PB0
                                  |
                        each SENSE has 10k to 3V3
                                  |
NUCLEO-F401RE -> 1 ms acquisition -> independent five-sample qualifiers
                                  |
                         tray-delimited word engine
                                  |
                   BQ1 snapshots, CRC16, bounded UART TX
                                  |
                     ST-LINK USB virtual COM -> Linux Tk
                                  |
                  connection / current-result / history-export
```

Firmware main.c performs clock restoration, input verification, watchdog startup, sampling and bounded serial scheduling. Drivers use the documented register addresses rather than a vendor HAL. Startup supplies vectors, data initialization, BSS clearing and freestanding memory routines; linker.ld places the image in target flash and SRAM. Failure to establish the clock stops before transmitting potentially misclocked telemetry. An input-configuration or watchdog-start failure creates a HALT record before normal acquisition.

## Measurement and firmware decisions

Five consecutive matching observations independently qualify each channel. A sample is an observation, not proof of continuous electrical stability between samples. Chatter on one lid does not prevent another lid from qualifying. Qualification latency can alter apparent ordering for near-simultaneous movements; a same-sample group means simultaneous qualified observations, not proven physical simultaneity. Slow manual actions, with meaningful states held at least twenty milliseconds during commissioning, are the intended operating regime.

BASELINE waits for all four inputs to be qualified low and raw low: closed lids and absent tray. READY then accepts a tray-only transition with lids previously and currently clear. Booting with a held tray or opening a lid while docking cannot invent a session. An unsuitable start returns to BASELINE and requires an absent tray and closed lids again. ACTIVE starts with age zero and an empty sixteen-entry word. A qualified rising edge on any lid appends its three-bit opening mask. Repeated openings require qualified closure between them; held lids do not repeat.

Qualified tray removal finishes only with all lids already qualified closed, the preceding qualified lids closed and all raw inputs low. Removal with an open lid is abandoned. A new lid opening while raw tray absence is visible is also abandoned, even if tray qualification still says present. The rejecting age limit is 600,000 acquired milliseconds. Reaching that deadline wins over completion on the same sample; completion immediately before it is permitted. A seventeenth opening group abandons the session rather than truncating a word and pretending it is complete.

The current or most recently attempted word is separate from the retained completed word. Abandonment does not erase the last completed result. The next accepted start clears only the working word. Counters obey starts = completed + aborted + active_indicator. The lifetime bound is one billion accepted starts and acquisition observations; exhaustion stops correlation rather than wrapping. Time itself is an unsigned 32-bit millisecond counter with ordinary modular wrap supported. A sample gap after initial acquisition halts the instrument and abandons any active session. Unknown time is not filled with invented stable samples.

HALT freezes acquisition-derived evidence while allowing wall timestamps and transport diagnostics to advance. The watchdog is serviced by the main acquisition loop. Its reset loses volatile evidence, as do power loss and manual reset; no EEPROM persistence or wear-sensitive writes are implemented. This is deliberate: history survives only in host exports, not in an unverified crash-recovery store. A host that notices counter rollback blocks the session. Some resets remain indistinguishable from forward traffic because the protocol has no persistent boot identifier.

## Protocol and companion application

BQ1 is an ASCII comma-separated snapshot ending in an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection and no final XOR, over the bytes before the asterisk. The standard check string 123456789 produces 29B1. CRC detects corruption; it is not authentication and cannot prove that a recording came from real hardware.

The numeric fields are sequence, now, phase, fault, raw, qualified, valid, starts, completed, aborted, age, last_age, gaps, tx_errors, reason, count and last_count; these are followed by sixteen working masks and sixteen retained masks. Unused slots must be zero. Phase values are BASELINE=0, READY=1, ACTIVE=2 and HALT=3. Fault bits are configuration=1, watchdog=2, sample gap=4 and exhaustion=8. Reason values are none=0, removal=1, timeout=2, capacity=3 and system fault=4. Exact field definitions live in app/model.py and firmware/wire.c.

The application uses Python's standard library and Tk on Linux. Run python3 bin-phrase/app/workbench.py from the repository root. The Connection screen scans ttyACM and ttyUSB ports and permits a manually entered path. Connect never replaces an already owned port; disconnect restores terminal attributes when possible. A failed replacement connection preserves existing evidence. The Current / retained word screen distinguishes an attempted word from a completed result. The Snapshots / export screen displays bounded received history and writes a new private CSV without overwriting existing files.

Replay accepts only regular files no larger than two MiB and rejects malformed frames, invalid counters, impossible word encodings and a partial final frame. Live framing can resynchronize after a truncated frame and retains error counts. Sparse-session checks detect counter rollback, clock rollback, contradictory duplicates, frozen-result mutation and retained word-prefix changes without a new start. These are necessary endpoint checks, not a proof that every hidden intermediate sample was legal. Periodic telemetry retains only the most recent completed result: several short sessions between transmissions can be missed by the host. Operate slowly and export frequently when individual words matter.

Freshness requires a connected live source, advancing acquisition and recent progress. Device/host progress differing by over two seconds blocks continued live interpretation until reconnection; this conservative rule can also reject sufficiently long oscillator drift or a delayed host. Replay is never labelled live. Exports retain source, connection state, freshness, parser errors and the blocking reason, plus the bounded snapshots. There is no person identifier, remote upload, account, telemetry service or command execution from serial data.

## Build, verification and release limits

From the repository root run make -C bin-phrase/firmware clean all test fixture. The target needs arm-none-eabi-gcc, binutils and make; host tests use GCC with address and undefined-behavior sanitizers. The build produces an actual Cortex-M4 ELF and binary, not merely a host syntax check. The fixture target emits synthetic BQ1 records through the real C encoder. Those records demonstrate software integration only and must never be presented as measurements from an assembled device.

Run python3 -m unittest discover -s bin-phrase/tests -p test_app.py for parser, session, export and PTY tests. Run xvfb-run -a python3 -m unittest discover -s bin-phrase/tests -p test_gui.py for the actual Tk event-loop workflow. This exercises replay, failed connection, reconnect clearing, repeated-connect rejection, synthetic serial input, retained word display, history rendering and hangup. Python compileall is an additional syntax check, not a substitute for those workflows.

KiCad 9 and its system Python pcbnew bindings are required for CAD regeneration. Run /usr/bin/python3 bin-phrase/tools/generate_hardware.py, followed by /usr/bin/python3 bin-phrase/tools/check_hardware.py. The checker runs fresh schematic ERC, PCB DRC and XML netlist export, then compares reference, value, footprint and every pad connection with the manifest. Passive-pin ERC cannot verify an external power source. Visual schematic inspection and physical connector checks remain necessary even with zero rule violations.

After electrical inspection and with no live equipment attached, the documented flash target uses OpenOCD to program and verify the Nucleo. It has not been executed against physical hardware for this revision. Before practical use, fabricate and inspect the carrier, measure supply and sense voltages, verify clock and UART timing, adjust mechanical stops, characterize contact bounce, and compare repeated sessions with independently recorded observations. Validate whether the fixture changes the behavior being studied. See VALIDATION.md for actual software/CAD results and the explicitly unresolved physical gates.
