# Teardown Stack — reverse-retrieval notebook for four removable parts cups

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, software and locally authored hardware design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. The supplied custom board is a routed passive contact carrier for an external NUCLEO-F401RE, not a standalone MCU board. Software builds and electrical-design checks are reproducible. Fabrication, flashing, mechanical calibration, EMC, electrical bring-up and actual repair-workflow value have not been physically validated.

## Purpose, overview and intended users

Teardown Stack is a four-dock notebook for removable parts cups used during small, unpowered mechanical teardowns. A repairer places the pieces from one disassembly step into a labelled cup and docks that cup. The device remembers the order in which cups arrived. During reassembly it shows which docked cup was deposited most recently and records whether retrieval follows the reverse order. A wrong cup removal abandons the current correlated sequence rather than quietly rewriting the expected order. A fresh empty-dock baseline is needed before another sequence can be accepted.

The intended user is a repair instructor demonstrating a short nested mechanism: a cover, a retaining plate, a spacer group and a spring subassembly, for example. Another useful audience is a model maker comparing assembly instructions, or a volunteer repair workshop teaching disciplined parts organization. The device is not intended to inspect commercial repairs automatically, rank workers, or replace a manufacturer's maintenance instructions. Many assemblies do not reassemble in strict reverse order; this tool is useful only when that policy actually matches the chosen exercise.

Ordinary compartment trays preserve spatial organization but do not record chronology. A phone checklist can record chronology, but needs a deliberate screen interaction each time hands are already occupied. Here, docking a removable cup is the interaction. The host can be disconnected while the MCU continues tracking the four physical contacts. The design captures no video, microphone audio, customer device identifiers or employee names. There is no cloud account, radio or Internet service dependency.

The evidence is specifically cup docking and undocking. It does not prove that a cup contains any parts, that all screws were recovered, that a cup has the correct label, or that the retrieved parts were fitted correctly. A user can produce a perfectly ordered empty-cup sequence. For that reason every completion is called a completed cup sequence, never a successful repair. Include a paper or digital repair instruction beside the device; its checks remain independent of this notebook.

## Originality, adjacent work and bounded novelty claim

The repository was inventoried before selecting this concept. FoldTrace records a fixed four-leaf folding sequence with minimum dwell. Stencil Orbit associates tool use with a rotating station. Rest Quartet tracks independent coupon residence. Those concepts do not maintain a dynamic stack of uniquely identified cup docks with arbitrary deposit order and reverse-order retrieval. Teardown Stack reuses a proven carrier topology, startup code and serial drivers, but implements a different measurement engine, frame format and companion interface.

Three adjacent commercial organizers found in a bounded product search are the [iFixit Magnetic Project Mat](https://www.ifixit.com/products/magnetic-project-mat), [FixMat](https://www.ifixit.com/products/fixmat), and [Anti-Static Project Tray](https://www.ifixit.com/products/anti-static-project-tray). Their published descriptions emphasize magnetic retention, writable notes, sorting compartments and, for some products, ESD-safe construction. They are sensible alternatives when chronology can be recorded manually. This proposal adds physical dock chronology and explicit loss-of-correlation reporting; it does not claim to improve their material safety or replace their storage function.

Generic PLCs and data loggers can implement stack checking, and sensor-equipped trays may exist outside this limited search. The intended contribution is the particular inexpensive, removable-cup repair teaching workflow with transparent reverse-retrieval semantics and local evidence export. Worldwide novelty, patentability and freedom to operate have not been established. Before investing in a product, observe actual repair classes and ask whether the extra cup motion is more helpful than a numbered mat. If it is not, use the simpler mat.

## Hardware specifications

| Item | Revision A contract |
|---|---|
| Processor module | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| CPU / memory | Cortex-M4; 512 KiB flash; 96 KiB SRAM |
| Operating clock | Internal HSI, nominal 16 MHz |
| Sensors | Four external normally-open, voltage-free dock switches |
| Contact example | Omron D2F-01L low-level lever switch; verify exact purchased rating |
| Carrier | 80 × 70 mm, two-layer FR-4, nominal 1.6 mm thickness |
| Carrier components | Six pin headers; four 10k pulls; four 1k series resistors |
| Communication | USART2 TX via Nucleo ST-LINK virtual COM port |
| Link | 115200 baud, 8 data bits, no parity, 1 stop bit |
| Power | Nucleo USB; regulated 3.3 V and common ground to carrier |
| Acquisition | Nominal 1 ms sampling; ten consecutive samples per contact qualification |
| Application | Local Python 3 / Tk desktop, Linux serial support |
| Actuation | None; no motor, lock, relay or permission output |

The Nucleo supplies the MCU, reset circuitry, regulator, decoupling, programming interface and USB bridge. None of those functions is pretended to exist on the passive carrier. J1 and J2 connect through short insulated jumper harnesses; the PCB is not a mechanically mating Arduino shield. The schematic deliberately shows the carrier boundary and its labelled connections. Passive-only ERC cannot validate the purchased board or its power supply.

### Exact connector contract

| Carrier connector | Pin / net | External destination |
|---|---|---|
| J1 | 1 VCC | Nucleo CN6 pin 4, regulated 3V3 |
| J1 | 2 GND | Nucleo CN6 pin 6, ground |
| J2 | 1 SENSE0 | CN8 pin 1, A0 / PA0 |
| J2 | 2 SENSE1 | CN8 pin 2, A1 / PA1 |
| J2 | 3 SENSE2 | CN8 pin 3, A2 / PA4 |
| J2 | 4 SENSE3 | CN8 pin 4, A3 / PB0 |
| J3 | 1 CUP_A, 2 GND | Cup A dock switch |
| J4 | 1 CUP_B, 2 GND | Cup B dock switch |
| J5 | 1 CUP_C, 2 GND | Cup C dock switch |
| J6 | 1 CUP_D, 2 GND | Cup D dock switch |

Every SENSE input has a 10k pull-up to VCC and a 1k resistor between SENSE and its external CUP net. The switch connects CUP to ground when a cup is fully docked. Firmware inverts the active-low GPIO value so a set telemetry bit means docked. Bit zero corresponds to A, bit three to D. The UART uses PA2 alternate function 7; it is routed through the development board rather than the carrier. Verify ST-LINK VCP solder bridges against the actual MB1136 revision before wiring.

The design basis is ST's [UM1724 Nucleo-64 manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf) for connector assignments and VCP routing, and [RM0368](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) for STM32F401 register definitions. Reused platform sources are explicitly enumerated in tools/import_platform.py. That script refuses to overwrite existing files and is a provenance/reproduction aid, not a routine build dependency.

## Architecture and block diagram

```text
Labelled cups A / B / C / D
        | insulating docking tabs operate voltage-free switches
        v
J3..J6 CUP nets -- 1k series -- SENSE0..3 -- J2 -- PA0/PA1/PA4/PB0
                                 |
                             10k pullups
                                 |
USB --> NUCLEO-F401RE 3V3 -------- J1
        |
        +-- SysTick --> independent qualification --> bounded four-entry stack
        |                                              |
        |                              completed / aborted / ambiguity counters
        +-- USART2 PA2 --> ST-LINK VCP --> Linux Tk companion
                                          Connect / Live / History / CSV
```

There is no return command path. Configuration is fixed in the reviewed firmware. The host cannot change the sequence policy, clear MCU counters, or operate an actuator. Unplugging the host does not release anything mechanical because there is no mechanically controlled output. Disconnecting power loses volatile evidence, so export before intentionally resetting the board.

## Mechanical arrangement and commissioning

Build four identical shallow docks on a nonconductive base, with space between them for gloved fingers. Start with removable cups roughly 45 mm wide and a base roughly 260 by 90 mm; these are suggested prototype dimensions, not a fitted manufacturing drawing. Provide an insulating tab on each cup that operates a switch only near the fully seated position. A hard mechanical stop must carry cup weight and handling force rather than loading the switch beyond its rated overtravel. An adjustable switch bracket lets the builder set this threshold after assembly.

The sensing arrangement detects cup presence, not weight. A light plastic cup can therefore be detected without a load cell, amplifier or calibration mass. Keep the cups uniquely labelled A through D and, preferably, mechanically keyed to their intended docks. Putting cup A into dock B is interpreted as B because identity belongs to the dock, not to an RFID tag. The delivered design does not include a fabricated keyed enclosure; mechanical fit and labelling are explicit prototype tasks.

At the beginning of a lesson, keep all cups outside their docks. Put the first removed parts in any cup, then dock it. Repeat with other cups as appropriate. The first cup need not be A: C, A, D is a valid deposit order, and D, A, C is its valid retrieval order. Intermediate undo and resume are permitted: deposit A, deposit B, retrieve B, deposit C, retrieve C, retrieve A is also legal. This models a nested workflow, not a fixed four-step recipe.

Commission first with the MCU disconnected from the object being repaired. Verify every switch with a meter, then verify each letter in the live display. A qualified closure requires ten consecutive nominal samples, so very short switch motion may be rejected. Exercise each dock slowly and hold each state at least 50 ms during commissioning. Compare physical cup movement against recorded transitions, then deliberately retrieve a lower stack entry, move two cups together, and interrupt USB power. Record whether the observed behavior matches this document. Those experiments have not been performed on physical hardware in this revision.

Use short paired signal/ground wires, provisionally below 0.5 m, and strain relief at both ends. Cover all solder joints. The carrier has no mounting holes; support its edges in an insulating tray rather than drilling through routed copper. Keep wiring away from moving mechanisms, hot tools, solvents and sharp component leads. Never use the sensing contacts as a shared connection to powered equipment. The cups and enclosure are not certified ESD-safe; add an independently reviewed ESD strategy before handling sensitive electronics, without connecting this circuit to arbitrary voltages.

## Firmware semantics and design decisions

The portable engine has WAIT, READY, ACTIVE and HALT states. At boot it begins WAIT with no known contact levels. Each channel has its own ten-sample qualifier. A continuously chattering unrelated switch cannot prevent another channel from qualifying its own change. Accounting begins only after all four inputs have been observed and an empty baseline is qualified. Booting with cups already docked never invents their historical deposit order.

In READY, a single newly docked cup starts a session and pushes that dock index. In ACTIVE, a newly docked cup pushes its index, while undocking the current top pops it. Emptying the stack after valid pops completes one session and returns READY. The physical occupancy mask ensures no dock can be pushed twice concurrently. A reused dock is legal after its previous entry has been popped. No ordering is imposed on deposits; the reverse retrieval constraint is derived from actual accepted deposits.

If more than one qualified input changes in the same observation, ordering is ambiguous. The collision counter increments and any active sequence is abandoned. If exactly one cup is removed but it is not the top, the order-error counter increments and the sequence is abandoned. Both cases enter WAIT; remaining cups must be removed until a qualified empty baseline is reached. Silence alone does not reconstruct an unknown nonempty stack. An error while already WAIT is not counted as a new failed session because no session is correlated there.

Any missed sample interval, including a delay of two milliseconds, invalidates continuity. The gap counter increments, active entries are abandoned, and all contact qualification restarts. Firmware does not fabricate repeated samples for the missing interval. Ordinary unsigned timer wrap is supported when successive samples are one millisecond apart. There is no fixed session timeout: a repair can pause with a cup docked indefinitely, but evidence remains volatile and a loss of power still destroys the stack.

The accounting identities are started = completed + aborted + active and pushes = pops + abandoned + depth. A pop means an accepted reverse retrieval. Abandoned counts retained entries discarded at correlation loss, not failed physical parts. Updates are staged in a temporary engine value; if any bounded counter would overflow, the entire update is discarded, the previously accepted stack is abandoned, and HALT is latched. This avoids half-published totals. Configuration and watchdog startup failures also latch HALT before the first telemetry frame.

The target is freestanding C11. startup.c initializes RAM and supplies core vectors. The SysTick interrupt only advances a tick counter. Direct register drivers restore HSI clock assumptions, configure input modes, check those modes, operate USART2 and start the independent watchdog. Clock waits have bounded iteration budgets. Serial transmission is nonblocking with a fixed buffer and a 100 ms transmit deadline, so a stalled UART cannot deliberately monopolize acquisition. Transport failures increment tx. Hardware watchdog behavior and actual oscillator accuracy remain bring-up checks, not host-test claims.

## Protocol and companion application

A status frame is attempted every 100 ms. Every frame identifies protocol TS1; no special boot packet is required for a late attachment. The payload is ASCII TS1 followed by twenty comma-separated, eight-digit uppercase hexadecimal unsigned integers. A star, four uppercase hexadecimal CRC digits and LF terminate it. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xFFFF over the complete payload before the star. There is no final XOR. The frame is 189 bytes including LF.

Field order is now, state, mask, known, depth, stack0, stack1, stack2, stack3, started, completed, aborted, pushes, pops, abandoned, order_errors, collisions, gaps, tx, fault. Time is milliseconds modulo 2^32. State codes are WAIT=0, READY=1, ACTIVE=2, HALT=3. Active stack entries use A=0 through D=3; unused slots must be zero. Fault bits are configuration=1, watchdog=2, exhaustion=4 and invalid raw input=8. Protocol fields do not include absolute time or persistent device identity.

The Linux desktop application uses only Python's standard library and Tk. Start it with python3 teardown-stack/app/workbench.py. Connect / fixture explains the wiring and opens an explicitly selected tty at 115200 baud. Live stack displays deposit order, next expected retrieval and cumulative diagnostics. History / export contains every retained accepted snapshot, bounded to the latest 1000, and writes a private CSV using exclusive creation so an existing file is never silently overwritten. Export includes author, provenance, connection state, freshness, parser errors and warning metadata even when no observations exist.

Replay is deliberately separate from live acquisition. Disconnect before selecting a recording. The reader accepts only regular files up to two megabytes, refuses FIFOs and devices, requires a complete final frame, and rejects empty, corrupt or inconsistent recordings. Failed replay preserves previous evidence. All replay screens say replay and never display live progress. Live serial framing tolerates a late-attachment suffix, counts discarded bytes as parser errors, and resynchronizes on the complete TS1 marker rather than a character that could occur in a CRC.

The application requires two advancing timestamps before declaring live progress. Duplicates do not renew freshness. Contradictory duplicate frames, cumulative-counter rollback, disappearing fault bits and impossible conservation identities block the session. A reception pause over 1.5 seconds or host/device accumulated progress disagreement over 0.75 seconds requires reconnect. The fixed allowance intentionally does not expand forever, so long captures with an inaccurate HSI oscillator may conservatively require reconnect. Freshness expires after 0.75 seconds without device progress.

Reset detection is heuristic because the protocol has no persistent boot identifier. Some MCU resets with forward-looking timestamps and plausible counters cannot be distinguished from continued operation. Likewise CRC detects accidental corruption, not hostile forgery. Keep the serial source local and trusted; do not treat a log as tamper-evident. Disconnect retains the previous session's provenance rather than relabelling it replay or erasing uncertainty.

## BOM, power and protection limits

A provisional cost allowance is USD 25 for one Nucleo, USD 5 for the carrier allocation, USD 2 for eight resistors, USD 4 for headers and harnesses, USD 12 for four suitable contacts, and USD 15 for cups, brackets and enclosure. These are design allowances rather than live supplier quotations, exclude labor and shipping, and do not establish a retail price. Equivalent resistors must retain the specified footprint and resistance values; mechanically different switches require a new bracket review.

At nominal 3.3 V the 10k/1k path draws approximately 0.30 mA per closed contact and produces approximately 0.30 V at SENSE. Four closed channels draw approximately 1.20 mA and dissipate approximately 3.96 mW in total. Reserve a provisional 100 mA at 5 V for the complete Nucleo-based USB system and measure it before choosing a supply. There is no battery, charger or claimed battery life. The series resistors limit modest fault current but are not isolation, surge suppression, ESD qualification or protection against mains voltage.

A broken cable resembles an absent cup and a short resembles a docked cup. Startup baseline rules may expose some such faults but cannot diagnose them reliably. A wrong-label cup, sticky switch, accidental bump or rapid two-cup move can also invalidate evidence. These are reasons to inspect the fixture and retain the error counters, not reasons to suppress errors in software. The tool must remain a passive educational observer, never a safety interlock or authority to energize repaired equipment.

## Build, tests and design reproduction

Run make -C teardown-stack/firmware all test for the Cortex-M4 ELF/binary and sanitizer-enabled host tests. The target build uses arm-none-eabi-gcc and libgcc, with no downloaded SDK or dynamic allocation. The linker script uses the actual F401RE memory regions. Flash only a reviewed physical setup with the Makefile flash target and OpenOCD; this run did not flash hardware.

Run xvfb-run -a python3 -m unittest discover -s teardown-stack/tests -p 'test_*.py' to exercise protocol, replay, real Tk event loops and Linux pseudoterminal serial operation. The fixture is emitted by the actual C encoder and represents synthetic cup movement, not a bench measurement. C tests cover all 24 four-cup deposit permutations and their reverse retrievals, wrong-order removal, simultaneous changes, dock reuse, startup occupied, glitches, sampling gaps, timer wrap and exhaustion.

Generate hardware with python3 teardown-stack/tools/generate_hardware.py on a machine with KiCad 9 pcbnew and its footprint libraries. Then run python3 teardown-stack/tools/check_hardware.py. The checker runs ERC and DRC, exports a fresh schematic netlist and compares every reference/pin/net, component value and full footprint identity against both the PCB and connectivity manifest. The source includes real symbols, connected pins, pads, tracks, a ground via and a closed outline. Passing these checks is not proof of assembly quality or enclosure suitability.

See VERIFICATION.md for measured results from this revision. Before fabrication, review the rendered schematic and board, inspect procurement variants, and conduct an independent electrical review. Before use, bring up one switch at a time and execute the scripted negative trials. The most important remaining experiment is a repair-class pilot comparing this workflow with a numbered paper mat; practical usefulness cannot be inferred from software tests alone.
