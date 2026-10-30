# Maze Return — magnetic checkpoint loop-erasure notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. The supplied PCB is a routed passive interface for a separately purchased NUCLEO-F401RE, not a standalone processor board. Target compilation, simulated input tests, desktop application tests and CAD checks are reproducible. Fabrication, flashing, magnetic calibration, physical timing, enclosure safety and classroom usefulness have not been verified with hardware.

## Purpose, overview and primary user

Maze Return records a person's checkpoint walk through a small tabletop maze and immediately shows its chronological loop-erased checkpoint route. Four removable reed-switch stations identify checkpoints A, B, C and D. A single captive magnetic puck passes those stations. Returning to an earlier retained checkpoint removes the suffix after that checkpoint, while returning to the current tail merely counts a repeat. The device retains visit, loop, repeat and erased-entry totals separately, making backtracking visible without recording a camera image of the participant.

The primary user is a computing teacher demonstrating how a chronological walk differs from the route remaining after loops are removed. On paper, students often erase the very history they are supposed to compare. With this instrument, a teacher can move a tangible puck through A, B, C, B and show both four completed visits and the retained route A, B. Continuing to D changes the retained route to A, B, D. The display does not say that this is the shortest path through the maze; it is the route produced by one explicitly defined transformation of observed checkpoint identities.

A secondary user is a museum exhibit maker checking whether a sign or wall arrangement encourages repeated visits to a particular checkpoint. The recorder is a removable bench instrument, not visitor tracking infrastructure. It has no camera, microphone, radio, account or network service. The application records local notes and cumulative snapshots, not identities. Anonymous qualitative comparison is the intended workflow; performance ranking, intelligence assessment and medical or therapeutic assessment are deliberately excluded.

The instrument does not know the walls, the distance travelled, the direction of approach, the location between gates, whether a participant lifted the puck, or whether the maze was solved. A checkpoint route can miss many spatial loops that occur between sensors. Conversely, a repeated checkpoint can be legitimate in the physical maze. Those limitations are part of the measurement definition rather than errors to hide behind a score.

## Originality and related designs

The repository was inventoried before implementation. Route Census correlates one released ball with one of three diverter outlets; it does not retain an arbitrary walk or erase loops on revisits. Teardown Stack remembers cup deposits and requires reverse retrieval; returning to an earlier identity there is not a legal chronological loop-erasure operation. Punch Register checks paper registration through a handle stroke. Maze Return reuses that device's low-voltage carrier topology, startup and peripheral drivers, while supplying a different algorithm, protocol, tests and user workflow.

A bounded external search found magnetic maze prior art, including [US4043558A](https://patents.google.com/patent/US4043558A/en), and microprocessor-controlled maze games, including [US4240638A](https://patents.google.com/patent/US4240638A/en). It also found the open [SeniorWizard laptimer](https://github.com/SeniorWizard/laptimer), which uses reed sensing for magnetic track passages. Magnetic maze toys, electronic games and reed-based passage counting are therefore not claimed as inventions here. The proposed contribution is a removable four-station educational recorder with provisional visit qualification, explicit ambiguity halting and simultaneous retained-route/history accounting. This is a distinct repository design, not a patentability or worldwide novelty determination.

Alternatives considered were a camera-based full-path tracker, an RFID puck and station array, and a fully instrumented switch grid. A camera can capture more geometry but introduces lighting, occlusion and privacy concerns. RFID requires a substantially more complicated station interface. A switch grid is useful for a manufactured game but less convenient for removable checkpoints on an existing hand-built maze. Four reed contacts are intentionally limited and inexpensive, with the measurement contract kept equally limited.

## Three practical use cases

1. Teaching chronological loop erasure. Before: the instructor writes every visit on a board, then students confuse erased marks with an unvisited path. After: the physical walk produces separate visit and erased-entry totals while the live screen shows the retained route. A paper diagram still supplies the graph and discussion of why the resulting route is not necessarily shortest.
2. Comparing exhibit instructions. Before: a technician watches two signs being tried and remembers only a vague impression of backtracking. After: separate reset-bounded trials retain explicit checkpoint loop and repeat counts, exported with a note identifying the sign and layout. The technician must keep checkpoint placement fixed and cannot infer total walking distance from those counts.
3. Routine sensor maintenance. Before: an intermittently sticking reed station is discovered during a lesson and its effect on the demonstration is unclear. After: a standard A, B, C, B, D test walk checks labels, erasure and clear intervals before participants arrive. An occupied station or ambiguous overlap stops correlation rather than producing an apparently complete result.

## Hardware specifications and power

| Property | Implemented design |
|---|---|
| Processor assembly | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| MCU resources | Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz |
| Sensors | Four external Littelfuse MDSR-7-10-15 SPST normally-open reed switches |
| Excitation | Passive 10k pullups to 3.3 V, 1k series resistor per channel |
| Sampling | Nominal 1 ms polling; independently qualified contacts |
| Connectivity | USART2 TX through ST-LINK USB virtual COM port, 115200 8N1 |
| Power | Nucleo USB; carrier fed only by Nucleo regulated 3V3 and ground |
| Carrier | 80 by 70 mm, two-layer FR-4, 1.6 mm nominal thickness |
| Application | Python 3 / Tk, Linux desktop; no third-party Python dependencies |
| Actuators | None |

The [Littelfuse MDSR-7 series page](https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7) identifies the normally-open glass capsule and its suitability for microcontroller logic-level loads. The selected 10–15 AT sensitivity is a purchase requirement, not a calibrated magnetic distance. The magnet, its orientation, station thickness and assembled lead stresses determine the actual detection zone. Do not substitute a powered Hall module, a normally-closed reed, an industrial proximity sensor or a switch connected to another electrical circuit.

The processor, regulator, decoupling capacitors, USB bridge, reset circuitry and programming hardware are all on the purchased Nucleo. The custom KiCad sheet correctly represents only the passive carrier. It contains real local symbols, resistor values, footprint assignments, wires and named nets. It does not imply that a generic connector is an onboard MCU. Passive-only ERC cannot validate the external Nucleo supply, its solder bridges or the assembled sensor harness.

| Operating condition | Carrier contribution | Complete instrument planning allocation |
|---|---|---|
| All gates open | Negligible ideal DC pullup current | Reserve 150 mA from USB at 5 V |
| One closed gate | Approximately 0.30 mA from 3V3 | Same USB allocation; not a measured total |
| Four closed gates | Approximately 1.20 mA from 3V3 | Fault condition, still within passive current budget |
| Sleep or battery operation | Not implemented | No battery-life claim |

For an ideal closed switch the divider produces 3.3 × 1k / 11k = 0.30 V at the MCU input. Each channel draws 3.3 / 11k = 0.30 mA. Those calculations are nominal design values; actual low levels, noise margins and current must be measured after assembly. The main loop busy-polls to drain serial traffic and is not a low-power design. USB power removes battery charging and runtime claims from the prototype but does not eliminate short-circuit or wiring hazards.

## Exact connection contract

| Carrier connection | External connection or meaning |
|---|---|
| J1 pin 1 VCC | Nucleo CN6 pin 4, regulated 3V3 |
| J1 pin 2 GND | Nucleo CN6 pin 6, ground |
| J2 pin 1 SENSE0 | CN8 pin 1, A0 / PA0; gate A |
| J2 pin 2 SENSE1 | CN8 pin 2, A1 / PA1; gate B |
| J2 pin 3 SENSE2 | CN8 pin 3, A2 / PA4; gate C |
| J2 pin 4 SENSE3 | CN8 pin 4, A3 / PB0; gate D |
| J3 pin 1 / 2 | GATE_A / GND, external reed A |
| J4 pin 1 / 2 | GATE_B / GND, external reed B |
| J5 pin 1 / 2 | GATE_C / GND, external reed C |
| J6 pin 1 / 2 | GATE_D / GND, external reed D |
| R1, R3, R5, R7 | 10k, 1%, VCC to SENSE0–3 |
| R2, R4, R6, R8 | 1k, 1%, SENSE0–3 to GATE_A–D |

The header square pads identify pin one. The four switch connections are nonpolar two-wire contacts, but the power header is polarized by wiring convention only. There is no reverse-polarity protection or keyed connector. The carrier has no TVS protection, galvanic isolation, level conversion or mains-rated spacing. Use short insulated bench cables, initially less than half a metre. The 1k resistors limit some current; they are not a protection guarantee. Never connect powered equipment or external voltage sources to J3–J6.

Connector and bridge assignments are based on ST UM1724, including its NUCLEO-F401RE Arduino table and USART section. A [distributor-hosted copy of UM1724](https://akizukidenshi.com/goodsaffix/User_mau_hard_DM00105823.pdf) was available through search when direct ST PDF retrieval timed out. Default ST-LINK VCP routing uses SB13/SB14 on and SB62/SB63 off. PA2 alternate function 7 is the USART2 transmitter; no receiver is enabled. Verify the actual board revision before wiring. The register map in registers.h follows STM32F401 RM0368, not a generic STM32 address guess.

## Architecture and block diagram

    Captive magnetic puck on an unpowered tabletop maze
                   |
         external reed gates A B C D
                   |
       J3–J6 --> 1k series --> SENSE0–3 --> J2
                                 |            |
                              10k pulls    PA0/PA1/PA4/PB0
                                 |            |
       USB --> NUCLEO 3V3 ------ J1       STM32F401
                                              |
                      1 ms samples --> independent qualifier
                                              |
                      provisional visit --> clear confirmation
                                              |
                      append / repeat / chronological truncation
                                              |
                         CRC snapshots --> USART2 --> ST-LINK USB
                                              |
                              Linux Tk notebook and JSON export

There is no path from the host back to a motor, lock or display on the fixture. Serial loss does not control the puck. The hardware keeps cumulative evidence while powered even if the application is closed, but USB removal may also remove power. All embedded state is volatile. The host can attach late and receive the current route, but cannot recover an exact event sequence that it never observed. This is why history rows are labelled snapshots rather than a visit log.

## Mechanical fixture and assembly

Build a supervised bench fixture, not a loose-magnet children's toy. A suggested maze base is a 300 by 300 mm insulating panel with removable nonferrous walls and four labelled station pockets. Start with station centres at least 60 mm apart, then verify that the actual puck cannot close two switches together. That spacing is a starting construction constraint, not a proven magnetic specification. Fix reed axes and magnet orientation during comparisons. Avoid steel screws immediately around sensors because they can reshape the field.

Use an enclosed puck approximately 30 mm in diameter with a fully captive magnet; fasten the housing mechanically so a magnet cannot become a loose ingestion hazard. Glass reed capsules must be sleeved and supported inside removable station housings, with no bending load at the glass-to-metal seals. No magnet size or height is prescribed as a guaranteed solution: select a weak captive magnet and adjust the station gap until each gate has a repeatable, exclusive detection zone. If this cannot be achieved, do not use the fixture for evidence collection.

The PCB has no mounting holes. Mount it in insulating edge guides inside a bench enclosure, with strain relief on every cable. A 120 by 100 by 45 mm electronics enclosure is a planning envelope; the separate Nucleo may require more space depending on connectors. No mechanical CAD is supplied for that enclosure. Keep the maze and wiring visually separate from the user's hand path. Use large printed A–D labels and tactile station markers as appropriate; color alone must not encode identity.

Solder resistors first, then headers. With all power disconnected, check resistance between VCC and ground and confirm every connector pin against the table. Attach the Nucleo harness without sensors, inspect the wiring, and then power from USB. Open gates should read open; momentarily joining a sensor connector's two pins with an insulated jumper should show its matching gate. Power down before attaching the reed harness. This manual bring-up has not been performed during source generation.

## Firmware state machine and algorithm

The application starts in WAIT. Each contact has independent candidate, stable, validity and candidate-start time. A level becomes qualified after five milliseconds without a candidate change. Initial activity is excluded: the engine requires all raw and qualified inputs clear for a full 100 ms before entering ARMED. Qualification time is elapsed sampled time, not proof of electrical continuity between samples. Firmware checks elapsed ticks; a gap greater than five milliseconds halts instead of bridging unobserved time.

In ARMED, a qualified single gate whose raw level agrees becomes a provisional CAPTURE. Raw or qualified simultaneous gates halt as ambiguous. The selected gate may chatter or be revisited while the capture is still pending; those motions remain one provisional visit. Another gate becoming raw-active at any point during capture halts, even if that other pulse would have failed debounce. This conservative policy favors rejecting ambiguous experiments over inventing checkpoint order.

Completion requires 100 ms of clear raw and qualified inputs after release. Both endpoints must agree before an interval earns clearing time. A raw glitch resets clearing even if it never becomes qualified. A visit held through 30 seconds, including clearing time, halts with a timeout; timeout takes precedence if completion would occur at the same endpoint. Recommended operation is at least 20 ms near a station and at least 150 ms entirely clear between stations. The minimum software thresholds are not a promised maximum operating speed.

At commitment, the engine searches the retained array for the checkpoint identity. If absent, it appends it and increments added. If already the last element, it increments repeats without changing the route. If earlier in the route, it truncates the suffix, increments loops and adds the suffix length to erased. Because only four identities exist, a distinct route needs at most four entries. Every commitment preserves visits = added + loops + repeats and depth = added − erased. Each loop erases between one and three entries. The first retained checkpoint never changes during a correlated run.

Counter capacity is checked before publishing an update. A capture that cannot commit leaves the previous route intact and halts. Sampling gaps, ambiguity, timeout, configuration failures and watchdog initialization failure also halt correlation. Clearing the sensors cannot reconstruct an unseen visit, so HALT requires a deliberate new run rather than automatic recovery. The watchdog is fed during sampled progress; a stalled program can reset the MCU, losing volatile evidence. Reset is not a continuation of the earlier walk.

The main loop and peripheral drivers use real register access, bounded clock-start waits, GPIO configuration readback, a nonblocking serial queue and a bounded transmit timeout. Startup initializes data and BSS and supplies a real interrupt vector and linker script. Only SysTick is enabled as an interrupt source. There is no vendor SDK installation dependency, heap allocation, flash writing, remote update mechanism or command parser. The initial imported platform files are listed in tools/import_platform.py; tools/import_transport.py records the initial transport specialization. These refuse overwrites and are provenance aids, not routine build steps or exact regenerators of subsequent reviewed changes.

## Protocol and companion application

Firmware emits an ASCII MR1 snapshot approximately every 100 ms. The comma-separated unsigned decimal fields are, in order: now, raw, mask, qualified, state, fault, visits, added, loops, repeats, erased, gaps, tx_errors, depth, route, pending, clear_ms and elapsed. The body starts with MR1 and ends before an asterisk; four uppercase hexadecimal digits after the asterisk encode CRC-16/CCITT-FALSE with polynomial 0x1021 and initial value 0xffff. LF terminates a record. The C encoder and Python decoder are tested together using synthetic fixtures produced by the real engine.

States are WAIT=0, ARMED=1, CAPTURE=2 and HALT=3. Fault bits are configuration=1, watchdog=2, capacity=4, gap=8, ambiguity=16 and timeout=32. Raw and qualified masks use bit zero for A through bit three for D. Route packs two bits per identity, least-significant checkpoint first; depth disambiguates an empty route from a route beginning at A. Pending is 0–3 for a provisional gate and 4 for none. Elapsed is the latest capture wall duration, bounded to 30000 ms; it freezes outside an active capture and is not a cumulative walking timer.

The Connection screen opens a Linux tty, supports explicit disconnect, loads recordings and exports a new local evidence file with experiment notes. The Route screen displays retained checkpoint order, counters, pending capture, contact qualification and faults. History displays the last 2,000 snapshots. The application uses Tk and Python's standard library, including termios; it is an appropriate desktop companion, not an unimplemented mobile shell. Run it with python3 app/workbench.py from this device directory on Linux with python3-tk installed.

Framing bounds prevent unlimited buffering, CRC failures are counted, and strict replay rejects malformed or partial tails. Replay must be a regular file no larger than 2 MiB; FIFOs and devices are rejected without waiting. Session validation rejects timestamp or counter rollback, contradictory same-time records, impossible one-visit route changes and changes to frozen halted evidence. Sparse multi-visit transitions receive necessary consistency bounds, not a proof of every unobserved intermediate state. CRC is corruption detection, not authentication; an attacker controlling the serial source can forge evidence.

Live status requires a connected stream with advancing device time. Duplicate frames do not refresh it. A reception pause or accumulated host/device time disagreement forces explicit reconnection rather than treating queued snapshots as fresh measurements. Uncalibrated HSI drift can eventually cause a conservative disconnect. Reset detection is heuristic because there is no persistent boot identifier; a reboot whose counters and timestamps appear forward can be indistinguishable. Export records source, connection, freshness, parser errors, blocked state and warning text alongside snapshots. Files are created with private permissions and existing files are never overwritten.

## Build, tests, cost and remaining work

From this device directory run:

    make -C firmware all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_quality.py
    python3 app/workbench.py

Use arm-none-eabi-gcc and binutils for the target, GCC for sanitizer-enabled host tests, Python 3 with Tk, Xvfb for headless GUI tests, and KiCad 9 with its Python pcbnew bindings. The PCB and schematic are committed outputs; generating them is optional via tools/generate_hardware.py. The hardware checker runs native ERC/DRC, exports a fresh schematic netlist and compares every reference, pin, net, value and footprint against the board and connectivity manifest. Flashing is an explicit operator action with make -C firmware flash and an attached Nucleo; it was not executed here.

| Item | Planning allowance USD |
|---|---:|
| NUCLEO-F401RE | 20 |
| Four MDSR-7-10-15 switches | 8 |
| Eight axial resistors | 2 |
| Headers and small connectors | 3 |
| Prototype carrier PCB allocation | 8 |
| Captive puck, sleeves, wires and simple fixture materials | 15 |
| Total before tax, freight, labor and host computer | 56 |

These are design allowances rather than current supplier quotations. No inventory reservation or purchase was made. Standard through-hole parts and removable harnesses allow replacement without discarding the processor assembly. Keep the firmware source, label map and experiment notes together when repairing the instrument. If sensor replacement changes the detection zone, repeat the entire calibration walk rather than assuming electrical continuity proves equivalent measurements.

Validation results and tool versions are in VALIDATION.md. Automated testing covers the algorithm on synthetic signals, application behavior through a serial pseudoterminal, and native CAD connectivity. Physical acceptance still requires checking power and logic levels, mapping exclusive magnetic zones, comparing timing to an independent reference, verifying interruption behavior on a real target, inspecting the captive magnet enclosure, and trying the workflow with its intended teacher. Passing software tests does not turn this prototype into a calibrated instrument or a certified toy.
