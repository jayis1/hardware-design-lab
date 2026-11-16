# Case Delta — opening-session inventory changes for a three-pocket tool case

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original design, firmware, software and documentation are MIT licensed; see LICENSE. Acquisition drivers, startup, register definitions, transport and passive carrier infrastructure reuse jayis1's Latch Reach platform at repository commit a68dadc. The inventory engine, CD1 protocol, companion interpretation and experiment tests are specific to Case Delta. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. Its custom PCB is a routed passive contact interface for a separately purchased NUCLEO-F401RE. It is not a standalone processor board. Software builds, synthetic tests and native CAD verification are described in VALIDATION.md. No fabrication, flashing, electrical bring-up, mechanical calibration, tool identification, field trial or production certification is claimed.

## Purpose and practical value

Case Delta is a removable insert for a small case containing three distinct tool nests. Three presence contacts report pocket occupancy, while a fourth reports whether the lid is fully closed. The instrument learns the occupancy of the closed case, observes one opening session, and compares the newly settled closed inventory with the previous reference. It also retains which pockets changed occupancy during the session and which initially occupied pockets became absent and were occupied again before closure.

This answers two different questions that an ordinary missing-tool light merges. First, what changed between opening and closing? Second, which initially present pockets were emptied and refilled even though the final inventory looks unchanged? A repair instructor can distinguish a case that remained untouched from one whose three tools were all taken out and put back. A maker comparing case layouts can inspect whether a particular nested tool is repeatedly disturbed while retrieving another. A kit designer can observe a newly added tool without declaring it an error merely because the original case was not full.

For example, a case starts with A, B and C present. The operator opens it, removes A, returns A, removes C, and closes the lid. The result is missing C, no newly added pocket, occupancy changed in A and C, and A emptied and refilled. The next closed reference contains A and B. When C is put back in a later opening, that session reports C as added. Reference learning follows observed closures; the device does not impose an all-tools-present policy or silently call deliberate removal a failure.

The target audience is a repair-workshop educator, a maker designing a small fitted tool case, or a researcher doing supervised informal comparisons of organizer layouts. It is intended for ordinary low-risk hand tools or blunt demonstration objects. It is not a medical tray audit, aviation tool-control system, employee monitoring system, security seal, child-safety device or automatic assurance that a case is safe to transport. Nothing is locked or actuated. Human inspection remains the authority for the actual contents.

A contact detects occupancy at its mechanical follower, not object identity. A finger, a replacement object, a jammed lever or a loose insert can produce the same contact state as the intended tool. Conversely, lifting a tool slightly can open a follower without actually removing the tool from the case. The interface therefore uses pocket names and qualified occupancy changes, not verified tool uses. Emptying and refilling a pocket does not establish which person handled it, whether the tool was used, whether it was cleaned, or whether the same object came back.

## Distinction from existing devices

The repository directories and README purpose summaries were inventoried before this concept was chosen. The closest local descriptions were inspected: Teardown Stack observes reverse-order retrieval from dynamically ordered parts cups, Age Rack reports cohort-aware relative retrieval rank, and Kit Multiset counts order-independent declaration strokes against a recipe. Case Delta imposes no stack, age priority or recipe multiplicity. Its measurement unit is a lid-delimited session with a before/after occupancy difference and a separate emptied/refilled mask. It permits arbitrary pocket order, simultaneous changes, and partial inventories.

Electronic tool-control cabinets, foam shadow boards, inventory cameras, RFID systems, switches and event loggers are established technologies. The proposed contribution is this small, removable three-nest workflow and its explicit distinction between endpoint inventory and intermediate occupancy history. A fitted foam insert and a written checklist may be preferable when automatic history adds no value. A camera may give richer evidence when identity matters, at the cost of privacy, lighting and review effort. A larger RFID cabinet is a different cost and operational category.

Originality is bounded to a distinct proposed experiment within the inspected repository, not a claim of worldwide commercial absence. No comprehensive patent search, freedom-to-operate assessment or market validation has been completed. Before investing in a product, compare the notebook with a plain organizer in a real workshop. If the contact followers increase handling effort or cause false changes, redesign the insert or keep the simpler organizer. Reusing a tested electrical topology is intentional; an arbitrary change to the pull-up network would not make the experiment more original.

## Hardware specification

| Function | Revision A contract |
|---|---|
| Processor module | ST NUCLEO-F401RE, MB1136 family |
| MCU | STM32F401RET6, Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI restored to nominal 16 MHz before UART startup |
| Inputs | Three pocket contacts plus one lid-closed contact |
| Sampling | Nominal one millisecond, no interpolation after missed acquisition |
| Qualification | Five consecutive identical observations per independent input |
| Closed settlement | One hundred complete raw-and-qualified stable intervals |
| Opening limit | 600,000 acquired milliseconds; deadline wins over completion |
| Carrier | 80 × 70 mm, two copper layers, 1.6 mm FR-4 |
| Supply | External Nucleo 3.3 V and common ground |
| Communication | USART2 TX, 115200 baud, 8N1, through ST-LINK virtual COM |
| Programming | External Nucleo onboard ST-LINK SWD |
| Companion | Linux Python 3 with Tk; standard library, local-only |
| Environment | Dry, supervised indoor bench; short contact cables |

The custom board contains fourteen components: six headers and eight axial resistors. It does not contain the MCU, oscillator, voltage regulator, debugger, USB connector or decoupling network. Those are supplied by the purchased Nucleo assembly. Consult ST's [UM1724 Nucleo-64 manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf), the actual MB1136 revision, and [RM0368 STM32F401 reference manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) before wiring or programming. Register definitions are specific to that MCU family, not a generic Cortex-M target.

### Electrical connections and bill of materials

| Logical bit | Input | Field header | J2 pin | Nucleo header | MCU |
|---|---|---|---|---|---|
| 0 | POCKET_A | J3 | 1 | CN8.1 / A0 | PA0 |
| 1 | POCKET_B | J4 | 2 | CN8.2 / A1 | PA1 |
| 2 | POCKET_C | J5 | 3 | CN8.3 / A2 | PA4 |
| 3 | LID, closed asserted | J6 | 4 | CN8.4 / A3 | PB0 |

Every field header uses pin 1 for signal and pin 2 for ground. J1 pin 1 connects to Nucleo 3V3 at CN6.4; J1 pin 2 connects to ground at CN6.6. J2 carries four sensing nodes, not power for active sensors. PA2 is reserved for USART2 TX AF7 through the ST-LINK virtual COM path. Check the Nucleo solder bridges against the purchased board revision. The application never transmits device commands, and firmware does not enable the USART receiver.

R1, R3, R5 and R7 are 10 kohm, one-percent pull-ups from 3.3 V to SENSE0 through SENSE3. R2, R4, R6 and R8 are 1 kohm, one-percent series resistors from those sensing nodes to the respective field contacts. Use DIN0207 axial parts with 7.62 mm lead spacing and at least 0.125 W rating. J1 and J3 through J6 are two-pin 2.54 mm headers; J2 is a four-pin header at the same pitch. Mating insulated connectors, cable strain relief, insulating supports and a separately purchased development board are required.

At nominal values, each closed channel draws 0.300 mA and presents about 0.300 V at the MCU input. All four closed channels draw 1.20 mA, with total resistor-network dissipation about 3.96 mW. These are calculated nominal network values, not measured total product consumption. Nucleo, ST-LINK, USB losses and the host are separate power consumers. Verify input voltage margins and real power on the assembled device before leaving it connected for an experiment.

There is no galvanic isolation, surge clamp, reverse-polarity protection, independent battery or charging circuit. Ground is common with USB and the host. Attach only disconnected, voltage-free contacts. Never connect a powered machine signal, mains circuit, PLC output, powered sensor output or another supply rail. A series resistor is not a protective isolation barrier. A broken wire looks absent or open; a short to ground looks present or closed. The firmware cannot distinguish these electrical faults from the corresponding mechanical state.

Low-level lever contacts such as a suitable Omron D2F-01L suffix are a starting point for procurement investigation, not a tested component endorsement. Verify the purchased contact material, minimum switching recommendations, operating force and lifetime at approximately 0.3 mA. Do not assume a general-purpose high-current microswitch works reliably at this wetting current. No claim about environmental sealing or ESD resilience is made for the exposed headers.

## Mechanical insert and installation

A practical starting assembly is a rigid insert approximately 250 × 180 mm inside a shallow tool case. Make three custom nests labelled A, B and C, each fitted to a different blunt demonstration tool. Leave enough finger clearance that retrieving A does not require pressing the follower under B. Use a lightly sprung lever follower beneath each tool and a physical stop that carries the tool's weight; the switch itself should not support a heavy object. These dimensions are fixture starting points, not supplied manufacturing drawings or universal tool dimensions.

Mount the lid switch so it is asserted only near the final closed position and releases before a hand can reach any pocket. That ordering is a functional requirement. It creates a useful observation boundary: pocket changes while the lid is still asserted are treated as a sealed-case disturbance rather than silently rewriting the reference. A lid switch that opens too late, or an insert that flexes under closing pressure, can therefore halt the experiment. Verify this mechanically instead of weakening the rule to accommodate an unsuitable fixture.

Keep contact leads short, strain-relieved, and away from pinch regions. Put the passive board in an insulating enclosure or edge rails; the supplied layout has no mounting holes. Place the Nucleo outside the moving lid and secure its USB cable. Do not crush cables under hinges or allow exposed pins to touch metal tools. Keep conductive debris and liquids away. The design is a bench notebook, not a portable untethered product; it requires host or USB power and has no nonvolatile session storage.

During commissioning, manually inspect all sixteen raw input combinations using loose test contacts before installing the insert. Then test each pocket independently with the actual objects. Slowly open the lid without touching tools, remove each tool, replace it, and close the case. Compare the displayed result with an independently written observation or video. Repeat with different insertion angles and with neighboring tools present. Document misfires rather than treating an apparently correct display as calibration evidence.

## Architecture and block diagram

```text
Pocket A follower ---- J3 -- 1k -- SENSE0 -- PA0
Pocket B follower ---- J4 -- 1k -- SENSE1 -- PA1
Pocket C follower ---- J5 -- 1k -- SENSE2 -- PA4
Lid CLOSED contact --- J6 -- 1k -- SENSE3 -- PB0
                                 |
                       each SENSE: 10k to 3V3
                                 |
NUCLEO-F401RE -> 1 ms samples -> four independent qualifiers
                                 |
           closed reference -> opening session -> settled closure
                                 |
            before / after / touched / emptied-and-refilled
                                 |
                    bounded CD1 frame + CRC-16
                                 |
          USART2 TX -> ST-LINK USB VCP -> Linux Tk companion
                       inventory / snapshots / setup / CSV
```

The hardware generator emits real embedded connector and resistor symbols, wires, named nets, footprint identities and routed copper. The schematic uses labels to connect the repeated channels, not decorative unconnected blocks. A local symbol library and connectivity manifest accompany the project. The checker runs native ERC and DRC, exports a fresh schematic XML netlist, and compares every reference, value, footprint and pin/net pair against actual PCB pads and the manifest. Passive-only ERC cannot establish correct external power or mechanical operation.

## Firmware behavior and design decisions

BASELINE waits for all four channels to qualify and for a closed, unchanged full input word to remain stable for one hundred complete intervals. Five identical samples establish qualification; they are not themselves credited as one hundred stable intervals. With an immediately stable input after reset, readiness is reached at sample 105. The resulting three-pocket word becomes the sealed reference. Startup with an open lid cannot establish a reference, because the device has no trustworthy previous closed inventory.

READY preserves that reference. A raw lid departure starts an ACTIVE session immediately, so pocket changes during lid debounce belong to the same session. A later qualified open lid is still required for a completed result. A short raw lid glitch therefore creates an aborted session when the closed case settles again; it never claims a genuine opening. While READY, any raw pocket disagreement with the stored reference while the lid remains raw closed produces terminal F_SEALED. This deliberately exposes mechanical disturbance and electrical glitches instead of learning them as a new inventory.

ACTIVE tracks qualified pocket changes without imposing an order. The touched mask is the union of all changed occupancy bits. The absent mask is the union of pockets observed qualified absent. Initially absent pockets may contribute to absent, but the returned result is restricted to pockets present both before and after the session. Consequently returned is precisely reference AND final AND absent, while missing is reference AND NOT final and added is final AND NOT reference. These are set operations, not object identity comparisons or counts of repeated handling.

Closure requires the raw and qualified complete words to agree at both endpoints of every credited interval, with the lid closed. Any raw disturbance resets settlement, even if it is too short to change a qualified channel. Once one hundred stable intervals have elapsed, a session with a qualified opening completes, its record replaces the previous completed record, and its final inventory becomes the next reference. An aborted or timed-out session never overwrites the previous completed record. After an abort, BASELINE must learn a new settled closed reference before another session can start.

The opening deadline is ten minutes of nominal acquired time. Reaching the deadline rejects the session even if that exact sample would otherwise complete settlement. Timeouts are reported as aborted sessions and return to BASELINE, not as completed inventory comparisons. There is no event queue on the MCU. Telemetry repeats current counters and the latest completed result every approximately one hundred milliseconds; an offline host can learn the total number of completions later but cannot recover overwritten intermediate records.

A sampling gap is terminal. Firmware neither fills missing samples nor interprets a skipped interval as stable contact exposure. A gap abandons an active session exactly once, freezes acquisition-derived evidence, sets F_GAP and continues diagnostic telemetry. Other faults are invalid input configuration, watchdog initialization failure, bounded-counter exhaustion and sealed pocket disturbance. Counters are bounded at one billion; there is no wraparound into apparently fresh evidence. Device millisecond timestamps do wrap modulo 32 bits, and ordinary one-tick wrap is tested.

The invariant is starts equals completed plus aborted plus one when ACTIVE. Main, startup, clock restoration, GPIO checks, watchdog support and nonblocking UART drivers are included. The build is freestanding C11 with no vendor HAL or dynamic allocation. UART submission is bounded and refuses an occupied buffer; transmission timeouts increment diagnostics. An HSI initialization failure stops before serial initialization because a frame at an unknown clock is not trustworthy. No remote update mechanism, command receiver, persistent audit trail or secret key is implemented.

## Protocol and companion application

Each line begins `CD1,`, contains twenty-three unsigned decimal fields, then an asterisk, four uppercase hexadecimal CRC digits and LF. CRC is CCITT-FALSE, polynomial 0x1021, initial 0xffff, over the ASCII body before the asterisk. Field order is sequence, now, phase, fault, raw, qualified, valid, starts, completed, aborted, reference, touched, absent, opened, age, quiet, last_before, last_after, last_touched, last_returned, last_age, gaps, tx_errors. The source of truth is wire.c and app/model.py. Firmware phases are BASELINE=0, READY=1, ACTIVE=2 and HALT=3.

The Linux application has three real screens: Inventory delta, Snapshots, and Setup and limits. Connect opens a user-selected serial device at 115200 8N1. Disconnect restores its previous terminal attributes where the operating system permits, preserves displayed evidence, and marks the session disconnected. A second connection is refused until the first is released. Permission failures, missing paths, non-TTY paths and unplug errors are displayed rather than misrepresented as empty successful sessions. No network or cloud account is needed.

Replay reads a regular file of at most two MiB and validates the entire stream before replacing existing evidence. FIFOs, devices, oversized inputs, malformed lines, wrong CRCs, impossible field combinations, contradictory duplicates, counter rollback and partial final frames are rejected. Live decoding can resynchronize at the complete CD1 marker after a damaged record; parser error totals remain visible and are included in exports. A frame CRC detects accidental corruption, not an adversarial sender. Anyone controlling the serial stream can fabricate a valid capture.

Session checks preserve completed-result immutability when completion counters do not advance, active-reference stability, monotonic active evidence and frozen halted accounting. They also bound completion growth and retained completion age by acquired samples available across snapshots. These are necessary consistency checks, not a proof of every hidden physical transition. The device emits snapshots, not a cryptographically authenticated event history. A reset whose counters and time happen to look forward can remain indistinguishable; reset suspicion requires explicit reconnect, and a genuinely halted MCU must itself be reset and rebaselined.

LIVE progress requires advancing acquisition sequence, an owned live connection and recent receipt. A two-second reception pause latches the session blocked, and a fixed two-second cumulative host/device progress discrepancy also blocks it. This limits gradual backlog but is not absolute freshness proof after initial late attachment. Oscillator drift can eventually exceed that conservative fixed allowance and require reconnect. Replay and halted acquisition are never labelled live. Repeated identical timestamps do not renew freshness. The application may continue showing the last trustworthy numbers with a warning; it does not silently erase evidence.

Export creates a new mode-0600 CSV and refuses overwriting an existing file. Its first record contains author, provenance, connection state, freshness, warnings, parser errors and retained-versus-accepted snapshot counts. Subsequent records contain up to the latest two thousand accepted snapshots. Even an empty capture exports metadata, so a failed session is not reduced to an unexplained header. Files are local and contain no user identity by default. Export before connecting to a new device or replacing a replay; the application has no hidden permanent database.

## Build, test and commissioning

Run these commands from the repository root on Linux with GNU Arm Embedded tools, GCC, Python with Tk, KiCad 9 and Xvfb installed:

```sh
make -C case-delta/firmware clean all test fixture
xvfb-run -a python3 -m unittest discover -s case-delta/tests -p 'test_*.py'
/usr/bin/python3 case-delta/tools/check_hardware.py
python3 case-delta/app/workbench.py --replay case-delta/firmware/build/demo.cd1
```

The fixture is synthetic C-generated evidence, not a physical measurement. It demonstrates a returned pocket, a missing pocket, an added pocket, and a deliberate acquisition gap. Host firmware tests enumerate all eight initial inventories, all eight intermediate inventories and all eight final inventories against independent set algebra. Additional regressions cover debounce boundaries, raw glitches, exact deadline rejection, startup, timer wrap, sealed disturbance, counter exhaustion, CRC and output buffer bounds. Driver tests exercise clock failure paths and UART behavior. GUI tests run an actual Tk event loop with Linux pseudo-terminals.

Regenerate CAD only when intentionally changing the design, using `/usr/bin/python3 case-delta/tools/generate_hardware.py`, then rerun the hardware checker. The import script records the reviewed infrastructure provenance and refuses overwriting existing imported files; it is not an installation command for normal users. Build products, Python bytecode and KiCad local preferences are ignored. Do not mistake a successful netlist comparison for electrical bring-up, and do not fabricate a PCB merely because a parser accepted it.

Flashing is intentionally manual: after verifying the exact external board and wiring, `make -C case-delta/firmware flash` uses OpenOCD with ST-LINK and STM32F4 configuration. No physical flashing was performed during repository validation. Begin with disconnected contact harnesses, confirm supply and ground with a meter, and only then attach one verified voltage-free switch at a time. Check the expected low-active mapping and UART timestamps before fitting the insert.

Acceptance on real hardware requires repeatable mechanical lid-before-pocket ordering, adequate contact wetting behavior, stable USB transport, expected input voltages, visible gap handling and agreement with independent observations across repeated sessions. Measure the real sampling interval and clock error rather than treating nominal milliseconds as calibrated metrology. Compare results with the same tools and insert without followers to detect mechanical interference. Until those gates are passed, this remains a reproducible design and software prototype, not a verified organizer product.
