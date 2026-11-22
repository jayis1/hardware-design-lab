# Foot Rock — missing-foot transition notebook for shim experiments

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software, documentation, tests and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Startup, clock, GPIO, UART, passive-carrier generation and serial framing infrastructure are explicitly reused from jayis1's Ghost Grid at repository commit 09d7e03. The support classifier, directed missing-foot accounting, FR1 contract and application presentation are specific to this design.

Revision A is an unbuilt engineering prototype. The custom board is a routed passive interface for a separately purchased NUCLEO-F401RE. Its MCU, reset network, regulator, decoupling, debugger and USB bridge are on that development board. Target compilation, host simulations, desktop operation and electrical CAD checks are reproducible. They do not establish physical sensing accuracy, mechanical safety, useful shim recommendations, or production readiness. No furniture, carrier or fixture was fabricated or physically tested for this revision.

## Purpose and practical utility

A four-legged model can appear stable when untouched yet rock between two configurations under a gentle alternating push. A repair-cafe demonstrator or model-furniture maker may know that a shim helps, but have difficulty remembering which foot opened, whether the next opening was diagonally opposite, and whether a different shim eliminated the pattern or merely changed it. Foot Rock places four electrical witnesses at the feet and records a directed transition notebook of the missing-foot configurations. It is intended to make before-and-after experiments repeatable without treating a binary contact as a load cell.

The principal experiment is deliberately small and supervised. Place a lightweight wooden table model on a firm test surface, adjust four identical contact witnesses, and gently alternate the same two push locations. Export one run. Add a reversible shim beneath one hard stop without changing witness adjustment, reset the observer, and repeat. Compare missing-foot dwell, opposite-corner transitions, adjacent-corner transitions, unknown acquisition and all-four contact dwell. A useful result might be a clear reduction in an alternating missing-foot pattern. An equally useful result might be discovering that the witness spring forces, operator motion or floor compliance dominate the apparent improvement.

This instrument never tells the user to cut a particular leg. It does not estimate shim thickness, support force, center of pressure, level, structural integrity or tipping margin. All-four closure is merely a joint electrical condition. A foot may actuate a witness while carrying negligible load; an open wire looks like an open witness. The name “Foot Rock” describes the experiment, not proof that every recorded transition was a physical rock about a known pivot. The output is a notebook for comparative reasoning, not an automatic repair prescription.

The target users are makerspaces, woodworking educators, repair-cafe demonstrators and designers comparing miniature support geometries. It is not intended for heavy furniture, occupied chairs, machine feet, access equipment, laboratory instrument certification or an operating production line. A person must never stand or sit on this fixture. There are no motors, brakes, relays, radio transmitters or machine-start outputs. The only output is local serial telemetry to a desktop computer.

Hardware is useful here because four underside witnesses need synchronized acquisition, and a human cannot conveniently watch all four while applying a repeatable gentle push. A phone-only checklist remains a good alternative for occasional work. A camera, carbon paper, a feeler gauge or four simple indicators may be sufficient. The proposed benefit of this device is the combination of directed transitions, explicit correlation breaks and retained evidence, not replacement of those established techniques.

## Originality and related designs

The repository was pulled on its configured default branch before implementation, and every top-level device directory and README title was inventoried. The closest existing designs were read in more detail. QuadraAlign uses force and tilt measurements to guide machine-foot balancing. Foot Rock does neither: it records transitions between binary missing-foot configurations on a lightweight model. Nest Settle records finite placement, full-contact settlement and reseating trials in a locating nest. Foot Rock instead runs a continuing directed missing-foot chain, ending correlation at raw multi-foot loss, acquisition gaps or a clean all-four resting interval. It does not report placement latency or nest-settling trials.

Shim Bracket associates operator fit/refusal declarations with captive feeler leaves and checks their logical consistency. This device has no thickness selector and records no fit declarations. Ghost Grid uses independent contacts to predict electrical keyboard ghosting; Foot Rock makes no matrix-scan prediction. The shared passive carrier is intentionally reused. The claimed new work is the experiment, conservative chain semantics, directed edge accounting and local interpretation interface, not invention of four switches or resistor pullups.

A bounded external search found WOOD Magazine's “Equalize Uneven Legs,” which describes established rocking-table geometry and diagonal leg relationships. Conventional shims and adjustable feet are established prior art. This design does not claim worldwide novelty, patentability or freedom to operate. The search supports the practical problem, not a claim that no similar instrument exists. A broader product search and user study would be required before commercialization.

Reference: https://www.woodmagazine.com/woodworking-how-to/layout-measuring-marking/equalize-uneven-legs . Search-result content was available; direct extraction was unavailable in this environment, so the article is not represented as independently audited experimental evidence.

## Hardware specification and interface contract

| Function | Revision A |
| --- | --- |
| MCU | STM32F401RET6 on NUCLEO-F401RE, MB1136 |
| Core | Cortex-M4, nominal 16 MHz internal HSI |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Sensors | Four external normally-open, voltage-free signal switches |
| Input interface | 10 kohm pullup and 1 kohm series resistor per channel |
| Acquisition | Nominal 1 ms polling, whole-vector qualification |
| Transport | USART2 TX, 115200 baud, 8N1, ST-LINK virtual COM |
| Programming | Onboard ST-LINK SWD |
| Power | USB to Nucleo; carrier takes only 3.3 V and ground |
| PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Assembly | Axial through-hole resistors, 2.54 mm headers |
| Companion | Linux Python 3 and Tk; standard library only |
| Persistence | Volatile MCU history, optional local CSV exports |

Architecture and block diagram:

    NW witness -> J3 -> FOOT_NW -> 1k -> SENSE0 -> PA0 / A0
    NE witness -> J4 -> FOOT_NE -> 1k -> SENSE1 -> PA1 / A1
    SW witness -> J5 -> FOOT_SW -> 1k -> SENSE2 -> PA4 / A2
    SE witness -> J6 -> FOOT_SE -> 1k -> SENSE3 -> PB0 / A3
                                            |
                                      10k each to 3V3

    USB -> NUCLEO-F401RE -> raw support topology guard
                        -> bundled contact qualification
                        -> dwell partition / missing-foot directed chain
                        -> bounded CRC-framed UART transmitter
                        -> onboard ST-LINK VCP -> Linux companion

J1.1 is VCC, connected to Nucleo CN6.4, the 3.3 V pin. J1.2 is ground, connected to CN6.6. J2.1 through J2.4 connect respectively to CN8.1 through CN8.4: PA0, PA1, PA4 and PB0. These analog-header pins operate as digital inputs. J3 through J6 use pin 1 for signal and pin 2 for ground. NW, NE, SW and SE have protocol weights 1, 2, 4 and 8. Electrical low means closed, so firmware inverts the GPIO levels. NW and SE are opposite corners, as are NE and SW. The labels describe the fixture viewed from above, not geographic bearings.

The inherited Nucleo virtual-COM arrangement expects SB13 and SB14 fitted, SB62 and SB63 open. USART2 TX is PA2 alternate function 7. No receiver is enabled and no command protocol exists. Verify those solder bridges and connector orientation on the actual purchased board revision before wiring. The firmware explicitly restores HSI and bus prescalers rather than trusting a debugger's clock setup. Addresses and bit masks are collected in registers.h and drivers/clock.c.

Manufacturer references are ST UM1724 for the Nucleo and RM0368 for the MCU. The pin mapping and board-support implementation follow the existing repository platform; no fresh manufacturer-PDF audit or physical board inspection is claimed for this run. Do not substitute a similarly named Nucleo, an incompatible STM32, or a 5 V module merely because its headers look similar.

## Electrical limits and bill of materials

Each closed switch produces a nominal current of 3.3 V divided by 11 kohm. The MCU-side node is the divider junction above the 1 kohm resistor. The resulting nominal values are 0.30 mA and 0.30 V per closed input; all four closed paths draw 1.20 mA and consume 3.96 mW from the rail. These are ideal circuit calculations, not measured electrical performance. Component tolerances, contact resistance, leakage and actual input thresholds need verification during bring-up.

Use signal-rated gold contacts suitable for low wetting current. Four identical Omron D2F-01FL lever switches are a candidate for a first fixture, subject to checking their actual operating force, travel, minimum load and mechanical fit against the supplier datasheet. The PCB does not impose that package: switches are external, connected by two wires. Their spring force can disturb a light model, so the candidate is not a validated sensor selection. If the measured disturbance is significant, redesign the witness mechanism or choose another sensing method rather than trusting the notebook.

| Item | Quantity | Selection requirement |
| --- | --- | --- |
| NUCLEO-F401RE | 1 | Actual MB1136 assembly with working ST-LINK |
| 10 kohm resistor | 4 | 1%, quarter-watt axial, 7.62 mm lead pitch |
| 1 kohm resistor | 4 | Same package class |
| Two-pin header | 5 | J1 and four witness connectors |
| Four-pin header | 1 | J2, signal-only connection |
| Signal switch | 4 | Identical low-force, normally-open, voltage-free contacts |
| Carrier PCB | 1 | Supplied two-layer layout |
| Insulated tray and short leads | 1 set | Strain relief and protected underside |
| Model, hard stops and shims | 1 set | Lightweight bench experiment only |

Reserve a conventional 5 V, 500 mA USB source budget for the development board and debugger. That is a supply-planning allowance, not measured operating consumption or battery life. There is no battery, charger, regulator or secondary supply inlet on the carrier. Never power the carrier rail from another supply while the Nucleo rail is attached. Check polarity with power removed before connecting USB.

The board has no galvanic isolation, dedicated transient suppressors or industrial input protection. Series resistors are not a mains or ESD protection system. All switches must be disconnected from other circuitry. Keep leads short, initially below half a meter, on a dry indoor bench. Do not connect PLC outputs, powered sensors, keyboard matrix terminals or unknown wiring. USB shares ground with the computer. Avoid liquids, exposed metal underneath the board, and cable paths that could pull the model off the bench.

## Mechanical arrangement and setup

A proposed demonstration model uses a 160 by 120 mm rigid top with four hard foot centers forming a 120 by 80 mm rectangle. Short legs leave enough space to see and adjust each witness. These are starting dimensions, not a load rating or a fabrication-tested mechanical drawing. Use a light wooden or printed model that can be handled safely with one hand, not a real chair or machine. Secure the base surface against sliding and keep the model away from the edge of the bench.

Each hard foot must carry the model through a solid stop. Mount its switch on a separate adjustable bracket beside the foot, with a lever sensing the approach to the base. The switch must never become the load-bearing element or travel stop. Adjust each lever so the switch closes near the chosen hard-contact threshold without bottoming out. Lock the bracket adjustment. Identical witness geometry matters more than nominal switch identity: a longer lever or different offset changes the effective threshold and spring disturbance.

A shim goes beneath the relevant hard stop or foot, not underneath the electronics. Record where it went, its material and thickness, the model orientation and the witness adjustment. If adding a shim changes the relationship between foot and witness, recalibrate all witnesses and label the comparison as a changed setup; otherwise the instrument may measure the fixture adjustment instead of the intended effect. This is a central experimental limitation rather than a software problem.

The carrier has no mounting holes. Use an insulated edge-support tray, keeping the solder side clear. An electronics envelope of roughly 110 by 100 by 45 mm is a planning allowance; mount the Nucleo alongside with access to its USB and reset button. Provide strain relief at every moving witness. Mechanical drawings for a production enclosure, certified load paths and standardized witness brackets remain future work.

Before a run, close each witness separately and verify its label in the app. Open all four, then close all four. Introduce a known open witness while holding the others closed, and check the missing-foot dwell label. Open two witnesses briefly and confirm that the next qualified missing-foot state creates a new chain seed rather than an edge from the old state. This checks wiring and interpretation, not calibration. A broken wire and a genuinely open witness remain indistinguishable.

## Firmware behavior and design decisions

Firmware is freestanding C11 without a vendor HAL, RTOS, dynamic allocation or floating-point arithmetic. Startup installs the exception vectors, copies initialized data, clears BSS and calls main. The linker allocates the actual MCU flash and SRAM regions and reserves stack headroom. SysTick supplies nominal millisecond ticks. GPIO acquisition, watchdog feeding and nonblocking serial transmission run in the foreground. An interrupt changes only the tick counter; the evidence state has one writer.

The complete four-bit contact vector qualifies as a bundle after five unchanged one-millisecond intervals following its candidate observation. Six observations are therefore required from a fresh candidate to its qualified endpoint. This is intentional: the measured object is a joint support topology, not four independent event timestamps. Chatter on one foot can delay the joint result. The app does not present that delay as exact mechanical contact timing.

Every successfully acquired observation is accounted once, either in unknown or in one of six dwell classes. Classes zero through three mean exactly the named foot is open and the other three are closed. Class four means all four closed. Class five means two or fewer closed. A dwell interval requires matching raw and qualified vectors at both endpoints. The first qualified endpoint does not receive earlier time; short rejected glitches interrupt credited dwell even if the eventual qualified vector returns to the same value. Consequently the displayed dwell is conservative sampled evidence, not continuous analog exposure.

The chain anchor is a missing-foot identity or NONE. A newly qualified missing-foot identity different from the current anchor increments that foot's visit count. If the anchor was NONE it increments seeds; otherwise it increments the directed edge from the old identity to the new one and classifies the edge as diagonal or adjacent. Same-anchor observations do not create self edges. An intervening all-four state is permitted, so a normal transfer through joint contact can connect two missing-foot configurations.

Three conditions clear correlation. First, any raw vector with two or fewer closed contacts clears the anchor immediately, even when it lasts only one observation and never qualifies. Second, any acquisition gap clears the anchor and restarts qualification. Third, 1000 credited all-four intervals clear the anchor, separating a resting interval from the next rocking sequence. The counter saturates at that threshold; it is not a lifetime duration. A raw glitch resets it. Exactly 999 credited intervals preserve the old anchor, while exactly 1000 clear it. These semantics are tested at the boundary.

A gap of ten device ticks between observations contributes nine missed ticks and one unknown acquired observation. Missed time is never credited to dwell or settlement. An invalid raw value, duplicate/backward time, initialization error or arithmetic exhaustion enters a terminal fault. Subsequent device timestamps and transmit-error diagnostics may advance, but evidence freezes. Each observation updates a speculative whole-state copy, so exhaustion cannot publish half a visit, edge or duration update. Ordinary unsigned timestamp wrap is accepted; a half-range or larger jump is rejected.

The independent watchdog is initialized with bounded waits and fed after acquisition. Clock initialization also uses finite iteration budgets before SysTick is available. Failure to establish a trustworthy core clock stops before serial output. A watchdog reset or power interruption loses volatile evidence. The protocol has no persistent boot identifier, so a reset may be inferred from time/counter rollback but cannot always be proved. Export and reconnect deliberately; do not merge separate MCU runs into one experiment.

## Protocol and companion application

FR1 is receive-only ASCII. Each line starts with FR1 followed by comma-separated canonical unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits and LF. CRC is CCITT-FALSE, polynomial 0x1021, initial 0xFFFF, over all bytes before the asterisk. The check string 123456789 produces 29B1. There is no NUL terminator on the wire. Firmware transmits a periodic snapshot approximately every 100 ms when the bounded UART queue is available.

Field order is: now, samples, missed, unknown, tx_errors, fault, raw, qualified, valid, anchor, settle, seeds, diagonal, adjacent; then dwell0 through dwell5; visits0 through visits3; edges0 through edges15 in row-major source/destination order. All are uint32. NONE is anchor 4. Fault bits are configuration 1, watchdog initialization 2, time 4, exhaustion 8 and input range 16. Protocol version is embedded in FR1; incompatible versions are rejected rather than guessed.

The desktop workbench has Foot contacts, Missing-foot transitions, and Experiment and limits screens. Enter the local serial device path and press Connect. There is deliberately no automatic hardware discovery or remote configuration: ordinary operating-system enumeration and explicit port selection avoid silently attaching to another instrument. Disconnect preserves retained evidence. A second Connect is refused while the app owns a port. Invalid connection attempts leave the old evidence intact.

Replay accepts only a regular file up to two MiB, parses it completely before replacing the session, and rejects an empty recording or partial final frame. Live framing can resynchronize after a truncated frame by looking for the complete FR1 marker. Parser-error counts and discarded initial fragments remain visible. The companion checks range, accounting, directed-edge source and destination budgets, time progression and frozen fault evidence. Adjacent acquired snapshots receive exact dwell and edge attribution checks. Sparse checks are necessary bounds, not reconstruction or proof of every unreported observation.

LIVE acquisition requires an open connection, advancing acquired counts in at least two snapshot intervals, recent acquisition advancement, no terminal fault and bounded host/device progress disagreement. Receipt of duplicate frames does not refresh acquisition. Long host pauses or growing backlog latch a session error and require reconnect. A fixed one-second drift allowance is conservative: an uncalibrated oscillator can eventually force reconnect even without a real failure. Latest contact tiles are explicitly retained snapshots, not a guarantee of present physical position.

Export CSV creates a new private file with mode 0600 and refuses overwrite. Its metadata row includes author, source, connection state, freshness, parser errors, discarded fragments, warning and session error. The app retains the latest 1000 snapshots, while their counters remain cumulative for the MCU run. CSV is an analysis export, not an FR1 replay recording. To preserve raw input for later replay, capture the serial byte stream with a separate terminal program while the app is disconnected; do not let two programs compete for the port.

## Build, test and bring-up

From the repository root:

    make -C foot-rock/firmware clean all test fixture
    xvfb-run -a python3 -m unittest discover -s foot-rock/tests -p 'test_*.py' -v
    /usr/bin/python3 foot-rock/tools/check_hardware.py
    python3 foot-rock/app/workbench.py foot-rock/firmware/build/demo.fr1

Requirements are GNU Arm Embedded tools, host GCC with address/undefined sanitizers, Python 3 with Tk, Xvfb for unattended UI testing, KiCad 9 CLI and pcbnew bindings. No pip packages, cloud account or Node build is required. The application is executable Python, checked by compilation and exercised through a real Tk event loop. The demo is generated by the production C encoder from synthetic switch observations and is not a physical recording.

The tools/import_platform.py script records the one-time platform provenance and refuses overwrites. It is not a normal build step. To regenerate electrical sources intentionally, run tools/generate_hardware.py with the system Python that has pcbnew installed, then rerun the checker. The checker exports a fresh schematic XML netlist and compares every reference, value, footprint identity, pin and net with the PCB and manifest, in addition to ERC and DRC. Passive-only ERC cannot validate the externally supplied Nucleo rail or witness mechanics.

Flashing requires a connected real target and OpenOCD. After inspecting wiring and installing the correct tools, the optional command is make -C foot-rock/firmware flash. This run did not execute it because no physical target was available. Initial bring-up must check the rails, GPIO high and low levels, pin labels, serial baud, watchdog recovery and switch adjustment on the actual assembly. Follow with deliberately induced wire opens and gentle known shim changes. Do not infer physical readiness from a successful target link.

## Troubleshooting and release limits

If all contacts read open, inspect the shared ground and J2 mapping before adjusting switches. If a contact is permanently closed, disconnect its witness to distinguish a shorted lead from a mechanism problem. If unknown time rises continuously, look for chatter or a disturbed bracket; lowering the qualification window to make the display look better changes the experiment. If the app reports drift or a session error, export retained evidence, reconnect and repeat a short run rather than relabeling buffered data as live.

A diagonal edge is evidence that two opposite missing-foot endpoints were observed within the same retained chain. It is not evidence that the other diagonal carried all load, that the floor was flat, or that a particular leg should be shortened. Adjacent edges can arise from handling, flexible construction, threshold differences or actual changing support. Interpret both classes with the physical procedure and photographs of the setup. Compare runs only when placement, orientation, witness geometry and handling are controlled.

VALIDATION.md separates target-build results, software tests, CAD evidence and physical work not performed. Before fabrication for repeated public use, obtain mechanical review, validate contact forces and thresholds, add a durable enclosure and keyed connectors, evaluate ESD and cable faults, and perform a controlled usability comparison against simple manual methods. This release is a reproducible engineering design package, not a tested commercial measuring instrument or a safety certification.
