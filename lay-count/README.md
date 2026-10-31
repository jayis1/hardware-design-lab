# Lay Count — index-pulse notebook for hand-wound traverse layers

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype, not a winding-quality certificate. The supplied custom PCB is a routed passive contact interface for a separately purchased NUCLEO-F401RE. The MCU, regulator, decoupling, debugger and USB interface are on that development board, not on the custom carrier. Software and CAD checks are reproducible; no board has been fabricated, flashed, mechanically assembled or physically calibrated for this design.

## Purpose and realistic use

A person hand-winding a small spool of craft cord, insulated wire, or educational coil often wants to compare two guide settings. A total turn counter says how much rotation occurred but conceals how that rotation was distributed across successive left-to-right and right-to-left passes. A loose tension arm might drop for an instant halfway across the spool. Looking at the finished package does not say which pass contained that event. Stopping to write down the counter at every reversal interrupts the very winding technique being studied.

Lay Count combines two traverse endpoint witnesses, a once-per-revolution spindle index contact and a spring-arm tension indication. It opens an observation after the guide leaves a witnessed endpoint and closes it at the opposite endpoint. Each completed observation retains its counted index pulses, duration, destination, and whether the tension indication was lost. Attempts that return to their starting endpoint or have an ambiguous boundary are separately abandoned rather than quietly reported as completed layers. A desktop notebook retains received snapshots and exports their qualifications along with the measurements.

A concrete first use is a school workshop comparing two hand-guided winding widths on a cardboard spool. The demonstrator positions the guide at the left stop, establishes a light tension-arm indication, resets the observer, and winds slowly toward the right. The notebook records the first traverse. The demonstrator continues back to the left and compares the pulse counts. After several passes, they intentionally relax the feed to see the corresponding dropout flag. The useful result is a record of how the operation differed, not a claim that a particular winding is electrically or mechanically acceptable.

Target users are educators, fixture builders, craft-tool experimenters and people maintaining small hand winders. The instrument is not intended for production transformers, mains insulation evaluation, high-speed motor winders, unattended operation, wire-break protection, or acceptance testing of safety-critical parts. No output controls the winder. A person remains responsible for stopping and inspecting the mechanism. The project removes manual transcription steps; time savings and improved repeatability are hypotheses to measure during a physical trial, not demonstrated performance figures.

## Distinction, prior art and scope of originality

Existing repository directories and README concepts were inspected before selecting this project. Takeup Atlas observes reversal-to-following behavior using two quadrature encoders. WebSlack Ledger compares strip travel on opposite sides of a feed mechanism. Sieve Shuttle records alternating endpoint motion of a hand sieve. None associates spindle index pulses with individual traverse passes and a tension-arm dropout indication. Lay Count deliberately reuses the reviewed jayis1 STM32 platform and passive contact circuitry from Sieve Shuttle at repository commit ca7121a; its traverse accounting, protocol, tests, physical fixture and companion model are different.

Turn counters and traverse mechanisms are established technology. The U.S. Solid NZ-1 manual coil winder includes a total counter; the NZ-2 manual describes a mechanically coupled traverse and adjustable wire arrangement. Whitelegg's BVB hand-operated winding machines are another established alternative for occasional winding and training. These are reasons not to claim invention of coil winding, layer counting or tension monitoring. The proposed contribution is a small, open, retrofit observation workflow with explicit boundary ambiguity, abandoned-attempt accounting and evidence export. Worldwide novelty, patentability and absence of comparable products have not been established.

Prior-art references consulted through public search results:

- https://ussolid.com/products/u-s-solid-manual-coil-winder-nz-1-coil-winding-machine-w-counter-html
- https://file.ussolid.com/content/JFHWM/JFHWM00003-Manual.pdf
- https://www.whitelegg.com/products/coil-winding/coil-winding-machines/hand-operated-coil-winding-machines

A phone video can sometimes document the same operation, but it needs a clear view of the spindle mark, guide and tension arm simultaneously and later manual annotation. A stand-alone total counter cannot preserve individual traverse boundaries. This hardware is useful specifically where short wired contact witnesses fit the fixture and the operator wants an immediate local record without image processing, radio pairing or cloud storage.

## Architecture and hardware specifications

The processor is the STM32F401RET6, Cortex-M4, on a purchased ST NUCLEO-F401RE MB1136-family board. It provides 512 KiB flash and 96 KiB SRAM. The firmware explicitly restores the internal 16 MHz HSI clock configuration before setting up peripherals. SysTick supplies a one-millisecond observation tick. GPIO inputs are sampled in the foreground. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit, through PA2 AF7 and the development board's ST-LINK virtual COM interface. The target does not implement a USB stack, receive commands, expose wireless connectivity or need an FPGA.

```text
Hand crank -> spindle with one magnetic index -> winding spool
                                                 ^
Material -> spring-arm tension witness -> manually traversed guide
                                           left / right witnesses
                  | INDEX | TENSION | LEFT | RIGHT |
                  +-------+---------+------+-------+
                       passive carrier, 80 x 70 mm
                      10k pullups + 1k series paths
                                   |
                       NUCLEO-F401RE, 1 kHz input
                  qualification / traverse / loss accounting
                                   |
                      USART2 TX -> ST-LINK USB VCP
                                   |
                    Linux Tk notebook / replay / private CSV
```

Four normally open voltage-free reed contacts are used as external witnesses. Choose low-level signal contacts whose manufacturer permits operation at approximately 0.3 mA, with insulated leads and suitable magnet spacing. They are not included in the carrier PCB. A magnet on the guide closes LEFT or RIGHT near its endpoints. A single spindle magnet closes INDEX once per revolution. A separate magnet on a lightly loaded spring arm closes TENSION while the arm lies in the operator-defined acceptable region. This last signal is an indication of arm position, not a force transducer or a calibrated tension measurement.

The carrier is two-layer FR-4, nominally 1.6 mm thick, with an 80 by 70 mm outline, through-hole components, 0.30 mm tracks and a 0.20 mm default clearance. It contains fourteen populated components and thirty connected pads. All input networks are identical. The schematic uses locally authored real resistor and connector symbols; the PCB uses installed standard KiCad footprints. The processor is intentionally external rather than represented by an unpowered or unconnected decorative MCU symbol.

## Pin map, power and electrical limits

J1 pin 1 receives 3.3 V from Nucleo CN6 pin 4; J1 pin 2 connects to CN6 pin 6 ground. J2 pins 1 through 4 connect to CN8 pins 1 through 4, respectively PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These Arduino analog labels identify digital inputs here. Each field connector has signal on pin 1 and ground on pin 2. Square pads identify pin 1.

| Connector | Witness | MCU net | Series resistor | Pullup |
|---|---|---|---|---|
| J3 | LEFT | SENSE0 / PA0 | R2, 1k | R1, 10k |
| J4 | RIGHT | SENSE1 / PA1 | R4, 1k | R3, 10k |
| J5 | INDEX | SENSE2 / PA4 | R6, 1k | R5, 10k |
| J6 | TENSION | SENSE3 / PB0 | R8, 1k | R7, 10k |

All pullups connect to the 3.3 V VCC net. A closed ideal contact sinks approximately 0.3 mA and places about 0.30 V on its MCU input through the resistor divider. Four closed contacts draw approximately 1.2 mA total. These are circuit calculations, not measured values. Internal pull resistors are disabled. Keep field leads below approximately 0.5 m for initial bench work and verify actual rise times and noise margins. No filtering capacitor is fitted because unequal filtering could change relative event timing.

Only disconnected, voltage-free switches may be attached. Do not connect a machine's powered limit-switch wiring, relay supply, motor controller, 5 V logic, or a sensor with an unknown output. The carrier has no galvanic isolation, TVS network, industrial surge protection or certified hot-plug behavior. The 1k resistor is not a mains safety barrier. A broken contact wire usually looks like an open contact and is not independently diagnosed. USB ground is common to the fixture contacts; use only an unpowered hand mechanism with no other electrical connection.

Power comes from the Nucleo's normal USB arrangement. Reserve 150 mA at USB 5 V as an unmeasured development-board budget, plus the small carrier load on 3.3 V. There is no battery, charger, switched power stage or claimed battery life. Follow the actual board revision's power-jumper instructions. Keep the ST-LINK USART routing fitted as documented for MB1136; the established F401RE arrangement uses SB13/SB14 connected and SB62/SB63 open. Verify the exact physical board before changing solder bridges.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136 headers and board configuration, and RM0368 for STM32F401 peripheral registers. A fresh manufacturer PDF fetch timed out during this run; the implementation reuses the previously reviewed repository platform rather than inventing a new pin assignment. Before assembly, obtain the current manual and verify it against the purchased board:

https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf

## Bill of materials and mechanical fixture

The carrier BOM is four 10k and four 1k resistors, 1% axial DIN0207 at 7.62 mm pitch; five 1x2 and one 1x4 straight 2.54 mm headers; and the routed two-layer PCB. The external assembly requires one NUCLEO-F401RE, four suitable low-level reed contacts, four captive magnets, cable assemblies, an insulating carrier enclosure, guide-end brackets, a spindle-index bracket and a spring-arm bracket. Budget roughly US$25–40 for the development board, US$10–25 for small-quantity carrier parts and PCB, and US$20–50 for switches, magnets, wiring and enclosure. These are planning allowances, not current supplier quotations; the winder, computer, labor, tooling and shipping are excluded.

For the first prototype, use a nonmotorized spool with a 60 mm working width on a stable base. Arrange endpoint magnet zones so they cannot overlap; initial zones around 5 mm wide provide visible dwell while moving slowly. Install the single index magnet securely and guard it against ejection. Position the reed far enough from guide magnets that guide motion cannot trigger spindle counts. Use a smooth eyelet and a weak spring arm suited to the material. The instrument must not introduce sharp edges, pinch points or significant unrecorded drag into the winding path.

Mount both electronic boards in an insulating enclosure, allowing approximately 160 by 110 by 45 mm for the combined arrangement. That is a packaging allowance, not released mechanical CAD. The carrier has no mounting holes; use edge supports without drilling through copper. Add strain relief and route cables away from the hand crank. The delivered files fully specify the electronic carrier, but fixture brackets require adaptation and physical review for the selected winder. Operate only in a dry indoor workshop, initially around 10–35 degrees Celsius. No ingress-protection or environmental qualification is claimed.

## Measurement contract and firmware decisions

Inputs are active when their contacts close. Each input is qualified independently after five consecutive one-millisecond samples in the same state. A candidate's first sample counts as one; qualification therefore spans four observed intervals. Shorter index assertions may be rejected. Initial characterization should use at least 20 ms high and low index dwell and no more than two spindle revolutions per second. The one-kilohertz sample rate is not an overspeed guarantee: fast pulses can disappear between samples without a detectable error.

WAIT means no acceptable origin is established. A unique qualified endpoint, a low index and a closed tension indication, with matching raw endpoint/index/tension levels, establish READY. Starting in the middle of a spool does not invent an origin. When the endpoint becomes qualified open, with both endpoints and the index raw clear and index qualified low, a new ACTIVE traverse begins. A pulse already active at departure invalidates that departure and requires another endpoint baseline. The origin is retained until completion or abandonment.

During ACTIVE, qualified rising index edges add pulses only when both endpoints are raw and qualified clear. A pulse coincident with endpoint arrival is ambiguous and abandons the attempt. A still-asserted index at arrival also abandons it. Returning to the original endpoint is an abandoned attempt, not a zero-width completed layer. Reaching the opposite endpoint with an inactive index completes the traverse. If the new endpoint still supplies a suitable baseline, it immediately becomes READY for the return pass; otherwise WAIT requires a fresh suitable baseline.

Duration begins at qualified departure, not at the first raw switch opening. The completion interval includes endpoint qualification delay. A traverse expires at 600000 ms, with timeout taking precedence over arrival at that same observation. Zero-pulse traverses are permitted because the person may move the guide without rotating the spindle. These are completed guide traversals, not proof that material was laid. The term layer is a convenient operator interpretation, not a firmware-certified property of the spool.

Any sampled raw loss or qualified loss of the tension indication during ACTIVE latches the traverse's dropout flag. Even a one-millisecond raw dropout can flag it, while a shorter unsampled interruption can be missed. The system does not integrate tension, distinguish actual slack from arm vibration, or prove force remained within limits. Establish and document arm preload and geometry before comparisons. A single index channel cannot infer spindle direction; backward rotations and forward rotations both create positive pulses. Only wind in the intended direction when interpreting pulses as approximate turns. Boundary phase means an integer pulse count is not an exact angular displacement measurement.

Both endpoints simultaneously raw or qualified closed are a terminal wiring/geometry fault. A skipped acquisition millisecond is a terminal continuity fault. An active attempt is abandoned exactly once before HALTED. Numeric updates are staged transactionally; counter exhaustion rolls back the prospective sample and terminates acquisition without corrupting conservation. Starts equal completed plus abandoned attempts plus one active attempt. Counted pulses equal completed-pulse total plus abandoned-pulse total plus current active pulses. Transport errors remain observable. Reset or power loss discards volatile state; no automatic recovery silently erases a fault.

The target uses real memory-mapped GPIO, USART, RCC, SysTick and independent-watchdog registers. Bounded clock initialization occurs before peripheral setup. The UART driver is nonblocking and expires stalled transmissions after 100 ms. A watchdog initialization failure halts the engine before valid-looking data can be emitted. Watchdog feeding follows foreground acquisition. Startup code initializes data/BSS and installs the vector table; the linker script reserves the actual flash and SRAM regions. There is no placeholder sensor reading, network simulator or target-side operating system.

## Application, protocol and evidence handling

LC1 is a fixed-policy ASCII protocol emitted approximately every 100 ms. Each line begins `LC1,` and contains the following unsigned decimal fields in order:

`time,raw,levels,known,state,anchor,starts,completed,aborted,pulses,abandoned,completed_pulses,active_pulses,active_ms,active_bad,last_pulses,last_ms,last_bad,last_end,bad_total,flags,gaps,tx`

The suffix is an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection and no final XOR over the marker and comma-separated fields. State values are 0 WAIT, 1 READY, 2 ACTIVE, 3 HALTED. Endpoint identity is 1 left or 2 right. Flags are 1 acquisition gap, 2 conflicting endpoints, 4 exhaustion, 8 invalid peripheral configuration and 16 watchdog setup failure. Inputs use bits 1 left, 2 right, 4 index and 8 tension. Timestamps are modular unsigned 32-bit milliseconds.

The Linux Python/Tk companion has three functional tabs. Connection / Replay discovers serial devices, opens a selected path, disconnects and loads bounded recordings. Traverses shows live state, accounting and the latest 200 received distinct completion results. Evidence / Export explains limitations and writes a new private CSV containing retained snapshots and session metadata. No third-party Python package, account, server, radio permission or app store is required. Launch it with `python3 app/workbench.py` in a graphical Linux session with Tk installed.

Replay must be a regular file no larger than two MiB. FIFOs and device nodes are rejected before reading, malformed frames and partial trailing records fail, and replay never becomes live. Live framing resynchronizes on the complete LC1 marker after truncated records. Session storage is bounded to 6000 snapshots. Exports use exclusive creation and mode 0600, never overwrite an existing file, and preserve source, connection status, freshness, parser-error count, warning and received data. Export before replacing the session; the device itself retains only the latest result, so fast or missed traverses cannot be reconstructed retrospectively.

Freshness requires two advancing device timestamps, a connected transport and recent device progress. Duplicate frames do not refresh it. A host pause exceeding one second or cumulative device/host progress disagreement above 0.75 seconds blocks continued live interpretation and requires explicit reconnect. This conservative fixed allowance can reject a long session because of uncalibrated oscillator drift. Counter rollback, timestamp rollback, disappearing faults and contradictory duplicates also block the session. There is no persistent boot identifier: resets whose state and time appear forward can remain indistinguishable. CRC detects accidental corruption, not a malicious sender or authenticated hardware identity.

## Assembly, acceptance and reproduction

Before power, compare all header pins with the manifest, inspect solder joints and measure resistance to exclude supply shorts. Confirm every 1k series path and 10k pullup. Wire the Nucleo and carrier with power removed. Check idle input voltages, then close each switch separately while observing its input bit. Confirm the two endpoint zones cannot overlap and that moving other magnets does not trigger INDEX. Mark the spindle, count twenty slow forward revolutions manually and compare index counts during a sufficiently long traverse. Repeat under video observation to identify mounting bounce and missed pulses.

Exercise a normal outward and return pass, a same-side return, an index held through arrival, a deliberately released tension arm, and simultaneous endpoint closure. Confirm the documented classifications before collecting comparisons. A physical commissioning campaign must also measure UART and sample timing, watchdog resets, supply behavior, contact levels, cable-noise sensitivity, reed repeatability and mechanical disturbance. None of those tests has been performed by the software build or CAD checker.

Build and test with `make -C firmware clean all test`. This links the Cortex-M4 ELF and binary and runs host engine, clock and UART tests under AddressSanitizer and UndefinedBehaviorSanitizer. Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` for protocol, export, replay, termios restoration and actual Tk/PTY workflows. Synthetic fixtures come from the real C encoder, not invented device captures. `make -C firmware flash` invokes OpenOCD for an attached ST-LINK; flashing has not been executed here.

Regenerate native KiCad files using `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9 Python bindings and standard footprints installed. `/usr/bin/python3 tools/check_hardware.py` invokes real schematic ERC and PCB DRC, exports a fresh netlist and checks every schematic and PCB reference, value, footprint and pin/net assignment against the manifest. Passive-only ERC does not validate the external Nucleo supply or physical fixture. See VALIDATION.md for checks actually executed and remaining hardware work. A passed software suite and zero CAD violations establish a reproducible design baseline, not fabrication readiness or measured usefulness.
