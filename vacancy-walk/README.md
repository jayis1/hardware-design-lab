# Vacancy Walk — single-vacancy transfer notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, software, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive contact carrier for a separately purchased NUCLEO-F401RE, not a complete MCU board. Target linking, synthetic experiments, application workflows and electrical CAD checks are verified in VALIDATION.md. Fabrication, programming a physical target, magnetic calibration, mechanical assembly, electrical bring-up and practical workshop value have not been physically verified. This is not an inventory security system, machine interlock, access controller or safety instrument.

## Purpose and practical use

Vacancy Walk is a four-pocket bench fixture for experimenting with three movable pucks and one empty position. Each pocket has a normally open magnetic contact. The operator removes one puck from a pocket adjacent to the vacancy, then either puts it into the vacancy or returns it to its original pocket. The instrument records the resulting movement of the vacancy, rather than assigning an identity to the puck. It explicitly separates successful adjacent transfers, cancelled lifts and observations that can no longer be correlated.

The intended user is an exhibit builder or engineering educator developing a small manual recirculation demonstration. A ring with one empty station is a useful physical model of moving a free buffer through an otherwise occupied system. In a workshop, the same fixture can compare pocket layouts: do users repeatedly lift and replace a puck because the next recess is awkward to reach, or does one transfer direction produce longer absence intervals? The useful output is an auditable occupancy experiment, not an automatic ranking of people or a claim about the quality of a production process.

A typical exercise begins with pockets 1, 2 and 3 occupied and pocket 0 empty. After the display says READY, lift the puck from pocket 1. The display changes to CARRY after the two-empty occupancy pattern qualifies. Put that puck in pocket 0. The device counts one clockwise vacancy step, because the vacancy moved from 0 to 1, even though the puck moved in the opposite direction. Wait for READY before lifting another puck. Replacing the puck in pocket 1 instead counts a cancellation, leaves the vacancy at 0 and preserves the lift duration separately from a successful move.

The distinction between vacancy motion and material motion is deliberate. Without tags, four occupancy contacts cannot prove which puck arrived. Another person could substitute an identical puck while the original is held. The system also cannot determine whether a puck was slid, lifted or exchanged through an unseen path. A reported transfer means that a particular admissible occupancy sequence was observed under the stated setup assumptions. Use independent video or tagged objects if physical identity matters.

## Originality and neighbouring designs

The repository's top-level device directories and README concept titles were inventoried before implementation. Teardown Stack and Coupon Transit were examined as the closest handling instruments. Teardown Stack learns deposition order and checks reverse retrieval from four uniquely addressed cups. Coupon Transit pairs two locations for each of two holders and permits overlapping transfers. Vacancy Walk instead maintains one free position in a four-node ring, admits only an adjacent occupied source, forbids a second outstanding lift, and reports the direction in which free space migrates. It is neither a reverse-retrieval stack nor two independent delivery channels.

The electrical carrier, STM32 startup, register definitions and UART driver are reused from inspected jayis1 designs. Reusing a suitable platform avoids inventing new circuitry merely to make a file look different. The new contribution is the fixture workflow, vacancy-specific state machine, conservative ambiguity rules, wire contract and companion presentation. The concept is distinct from the reviewed repository entries; worldwide uniqueness is not established.

Sliding puzzles and empty-space reasoning are established prior art. The CMU slide-puzzle exercise explicitly describes representing moves by the direction the empty space travels. Escape Room Supplier sells slider-position sensors that can trigger a puzzle outcome. Neither source establishes that this exact diagnostic fixture is new, and neither was treated as evidence of patentability. This proposal deliberately avoids a game-winning output: it is a measurement notebook for attempted adjacent moves, canceled lifts and lost correlation. A paper tally and camera are sensible alternatives for occasional use. Before productizing it, interview educators and compare the instrument against those simpler alternatives.

## Hardware specification

| Subsystem | Revision A choice |
|---|---|
| MCU assembly | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Processor | Cortex-M4; reset HSI at nominal 16 MHz |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Occupancy sensors | Four Littelfuse MDSR-7-10-15 normally open reed switches |
| Moving objects | Three inert nonferrous pucks, each with a captured magnet |
| Input sampling | One acquired vector per nominal millisecond |
| Qualification | Five consecutive identical complete vectors |
| Arming / rearming | Fifty observed clean intervals with exactly three occupied pockets |
| Pending-lift deadline | 10,000 acquired milliseconds, rejecting at the boundary |
| Host link | USART2 TX, PA2 AF7, 115200 baud, 8N1 via ST-LINK VCP |
| Custom PCB | 80 × 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Companion | Linux Python 3 with Tk; standard library only |
| Actuation / radio | None |

The Nucleo supplies the processor, regulator, oscillator, reset network, decoupling, debugger and USB interface. They are not omitted components on an allegedly self-contained carrier. The carrier has six connectors and eight resistors, with real schematic symbols, assigned through-hole footprints and routed copper. It connects by a short labelled harness; it is not a mechanically mating Arduino shield.

Each channel consists of a 10 kΩ pull-up from SENSE to 3.3 V and a 1 kΩ resistor between SENSE and the external reed signal. The reed closes that external signal to ground. At nominal supply and resistor values, a closed channel draws 0.30 mA and places SENSE at 0.30 V. All four closed channels draw 1.20 mA, or 3.96 mW. These are ideal calculations, not measured consumption. The manufacturer describes the selected reed family as suitable for microcontroller logic-level loads; actual contact lifetime at this excitation and in this fixture still requires validation. Do not substitute a power microswitch solely because its maximum current rating is larger.

Use one current-limited USB supply with a 5 V, 500 mA budget for the complete development-board assembly. Actual Nucleo consumption is not measured here. The carrier takes regulated 3.3 V only. Do not connect a second supply, a battery charger, a powered sensor output or a field control signal. The series resistors are not galvanic isolation, ESD qualification or industrial surge protection. The host, Nucleo and contacts share ground. Keep initial leads below half a meter, pair each signal with its return and provide strain relief. Only dry, indoor, low-energy tabletop experiments are within scope.

### Wiring and bill of materials

| Carrier pin | Destination or meaning |
|---|---|
| J1.1 / J1.2 | Nucleo CN6.4 3V3 / CN6.6 GND |
| J2.1 | CN8.1 A0 / PA0 / SENSE0 |
| J2.2 | CN8.2 A1 / PA1 / SENSE1 |
| J2.3 | CN8.3 A2 / PA4 / SENSE2 |
| J2.4 | CN8.4 A3 / PB0 / SENSE3 |
| J3.1 / J3.2 | POCKET_0 reed signal / GND |
| J4.1 / J4.2 | POCKET_1 reed signal / GND |
| J5.1 / J5.2 | POCKET_2 reed signal / GND |
| J6.1 / J6.2 | POCKET_3 reed signal / GND |

Bits 0 through 3 mean occupied pockets 0 through 3 after firmware inverts the low-active GPIO. Label pockets clockwise when viewed by the operator: upper left 0, upper right 1, lower right 2, lower left 3. Adjacent pairs are 0–1, 1–2, 2–3 and 3–0. Diagonally opposite pockets are not adjacent. A physical harness swap changes the experiment's meaning even if electrical checks pass.

The electrical BOM is one NUCLEO-F401RE, four 10 kΩ and four 1 kΩ axial resistors, five two-position and one four-position 2.54 mm headers, four external reeds, wire and the carrier PCB. Resistor lead pitch is 7.62 mm. The ordered pin-net manifest is kicad/connectivity.json; the Nucleo, reeds and puck magnets are external assemblies rather than unrepresented PCB-mounted components. Budget approximately USD 50–100 for low-volume prototype materials including an enclosure and fixture; this is a planning allowance, not a supplier quote or stock claim.

## Architecture and mechanical arrangement

    Three magnet-equipped pucks in four clockwise-labelled pockets
        pocket 0 reed ---- J3 ---- 1k / 10k ---- SENSE0 ---- PA0
        pocket 1 reed ---- J4 ---- 1k / 10k ---- SENSE1 ---- PA1
        pocket 2 reed ---- J5 ---- 1k / 10k ---- SENSE2 ---- PA4
        pocket 3 reed ---- J6 ---- 1k / 10k ---- SENSE3 ---- PB0
                                                    NUCLEO-F401RE
    USB supply -> Nucleo regulator -> J1 3V3/GND             |
                                              1 ms acquisition
                                              vector qualifier
                                              vacancy state machine
                                              bounded CRC snapshots
                                                        |
                                   USART2 TX -> ST-LINK virtual COM
                                                        |
                                      Linux local companion workbench
                                      Pockets / History / Setup / CSV

Start with a nonconductive base approximately 160 × 160 × 12 mm. Place four circular recess centers at coordinates (40,40), (120,40), (120,120) and (40,120) mm measured from the upper-left corner. Use 32 mm diameter recesses approximately 3 mm deep with rounded edges. Prototype pucks may be 30 mm diameter and 12 mm tall. These are starting dimensions for a manually lifted fixture, not a tolerance-qualified sliding track. The extra diameter clearance should prevent the sensors or wires from serving as a mechanical stop.

Mount one reed beneath each recess in a protected, nonconductive sleeve with an adjustable vertical gap. Begin with a small captured magnet, for example a 3 mm diameter by 2 mm cylinder, in the underside of each puck. Magnet grade, orientation and final stand-off must be determined experimentally; the reed's ampere-turn sensitivity is not a promised sensing distance. Retain magnets mechanically and enclose them so they cannot be swallowed or detach during handling. This fixture is for supervised adult instruction, not an unsupervised child's toy.

Check magnetic cross-talk with every arrangement of three pucks before using firmware outcomes. An empty pocket must remain open when its neighbours are occupied. A seated puck should close only its own contact, and a clearly lifted puck should release that contact before reaching another recess. Establish and mark a maximum permitted hover height if hovering can leave the source closed. Support and bend reed leads according to the manufacturer's handling advice; do not load the glass envelope with a mounting screw or soldering strain.

House the PCB separately in an insulating enclosure with cable strain relief. The current board has no mounting-hole pattern: use an edge-retaining enclosure or insulated clips outside copper and component clearances. Do not drill ad hoc holes through routed areas. Keep the USB cable away from the puck travel path. The experiment is hand-operated and has no stored-energy drive, motors or pinch-producing moving linkage.

## Firmware behaviour and decisions

The firmware is freestanding C11 with an explicit Cortex-M4 linker map and reset vector. Startup copies initialized data, clears BSS and calls main. Clock setup confirms HSI readiness, switches the system clock to HSI, resets bus prescalers and waits for PLL shutdown with bounded iteration budgets. It does not rely on a vendor HAL or an arbitrary system clock inherited from a debugger. The register addresses are the STM32F401 RM0368 peripheral map. The clock and GPIO configuration checks have host-side register-storage tests, not physical silicon emulation.

SysTick increments a millisecond counter. The main loop acquires one vector when that counter changes. If the next acquired timestamp differs by anything other than one modulo 32 bits, the engine halts instead of crediting an unobserved interval. This intentionally includes gaps before arming. Tick wrap is supported; sequence exhaustion is terminal rather than silently wrapping evidence. There is no interrupt-driven access to the Engine structure, so frame encoding cannot see a half-updated experiment.

The occupancy vector qualifies as a bundle after five identical acquired samples. This avoids synthesizing an intermediate occupancy from independently delayed bits. The tradeoff is explicit: chatter in any one contact can delay all vector decisions. A raw glitch shorter than qualification can reset arming credit but is not counted as a lift. The instrument is intended for slow hand movement, not rapid sliding-puzzle speed records. A complete remove-and-replace action between samples can remain invisible.

WAIT initially permits the operator to arrange three pucks. A stable three-occupied vector must persist for fifty clean intervals before READY. The first qualified endpoint contributes no previous interval, and a raw disagreement breaks clean arming credit. READY retains the established vacancy. Removing exactly one adjacent occupied pocket begins CARRY; removing an opposite puck stops with NONADJACENT. A direct change from one three-occupied pattern to another without a qualified two-occupied interval stops with AMBIGUOUS, rather than inventing a transfer.

During CARRY, the only continuing vector is the two-occupied transit vector. Reoccupying the original source counts CANCELED. Filling the previous vacancy while leaving the source empty counts MOVED and transfers the vacancy to the source. Any other qualified vector stops with AMBIGUOUS. A second removal cannot become another simultaneous transfer. The active age is bounded, and a completion qualifying at exactly 10,000 milliseconds loses to TIMEOUT. An arrival at 9,999 milliseconds can succeed. Results describe qualified endpoints; qualification latency and magnetic hysteresis are part of the uncertainty.

After a move or cancellation the instrument returns to WAIT for fifty clean intervals. Unlike initial setup, a qualified occupancy change during this rearm period is an error, not an opportunity to reseed the vacancy silently. Wait for READY before the next move. This conservative rule makes operator pacing visible and avoids hiding extra actions in an unobserved reconfiguration window.

Counters obey starts = moves + canceled + abandoned + active, and moves = clockwise + counterclockwise. A pending lift abandoned by timeout, ambiguity or acquisition loss is counted exactly once. Invalid attempts before a correlated lift do not increment starts. Source and last duration remain available on terminal errors. HALT freezes experimental accounting while continuing input and transport diagnostics. A new session requires reset and deliberate operator setup; there is no automatic recovery from quiet inputs that might conceal missing actions.

All history is volatile. There are no flash writes, credentials, pairing records or persistent counters. The independent watchdog is fed after acquisition; a stalled processor can reboot, but a reboot is not evidence that the experiment continued. The host rejects apparent clock or counter rollback and requires reconnection. Without a persistent boot identifier some resets remain indistinguishable from forward traffic. This limitation is preferable to claiming authenticated continuity that the hardware cannot prove.

## Protocol and companion software

Firmware transmits receive-only VW1 snapshots approximately every 100 ms. UART reception is not enabled. Each frame is ASCII: VW1 followed by comma-separated unsigned decimal fields, an asterisk, four uppercase CRC hexadecimal digits, then LF. CRC is CRC-16/CCITT-FALSE: polynomial 0x1021, initialization 0xFFFF, no reflection, no final XOR. It covers bytes before the asterisk. The standard 123456789 check value is 0x29B1. Frames are length-bounded and contain no NUL terminator on the wire.

Field order is sequence, now, state, result, fault, raw, qualified, valid, base, vacancy, source, starts, moves, canceled, abandoned, clockwise, counterclockwise, age, last_ms, baseline, tx_errors, gaps. State numbers are WAIT=0, READY=1, CARRY=2, HALT=3. Result numbers are NONE=0, MOVED=1, CANCELED=2, NONADJACENT=3, AMBIGUOUS=4, TIMEOUT=5, LOST=6, ABORT=7. Vacancy and source use 4 for absent. Fault bits are configuration=1, watchdog-start=2, exhaustion=4, invalid-input=8 and acquisition-gap=16. The result field retains the previous outcome while another lift is active; inspect state before interpreting it.

Sequence counts acquisition calls, not transmitted packets. A jump in sequence between status rows usually means unreported intermediate samples, not packet loss. The wire parser checks CRC, canonical decimal syntax, field count, ranges, conservation and state-specific occupancy. The session validator rejects counter regression, terminal changes and several impossible sparse transitions. These are necessary endpoint checks, not a proof of every hidden intermediate movement. At ten snapshots per second, complete short moves can occur between reports; cumulative counters remain useful, but a full chronological vacancy route cannot always be reconstructed.

Run `python3 app/workbench.py` from this device directory. The Pockets screen shows qualified occupancy, vacancy, pending source, direction totals and retained duration. History displays every retained row, including batches received together. Setup explains wiring and experimental limitations. Enter an explicit serial path, such as /dev/ttyACM0; the application does not guess which unrelated serial device is safe to open. A second connection is refused until the first is closed. Permission errors, malformed recordings and serial hangup are visible rather than silently treated as successful collection.

The live decoder resynchronizes on the complete VW1 marker and counts discarded fragments. Strict replay rejects corruption, trailing partial frames and empty files. Replay is restricted to regular files no larger than 2 MiB, avoiding blocking on FIFOs or device paths. Live evidence requires advancing device timestamps, recent receipt, an open connection and a nonterminal state. A reception pause over two seconds or excessive host/device progress divergence blocks further acceptance until reconnection; queued old bytes cannot simply refresh a green indicator. Oscillator drift can conservatively force a long-session reconnect.

CSV export creates a new mode-0600 file and refuses overwrites. Its metadata identifies jayis1, live/replay provenance, connection state, freshness, parser errors, warning state and dropped history rows. The history bound is 2,000 snapshots; cumulative counters retain MCU totals, but old detailed rows are discarded. Export regularly for longer investigations. No network service, telemetry upload or cloud dependency is included. CRC is not authentication, and local serial devices or malicious replay files are not trusted measurement authorities.

## Build, tests and fabrication gate

From this directory run `make -C firmware all test fixture`, `python3 tests/test_app.py`, and `xvfb-run -a python3 tests/test_gui.py`. Target compilation needs arm-none-eabi-gcc and binutils; host C tests need GCC with address and undefined-behaviour sanitizers. The GUI needs Python Tk, and headless GUI tests need Xvfb. No pip dependencies or package installation scripts are required. A synthetic recording is generated by the actual C encoder at firmware/build/demo.vw1. Validate it without a window with `python3 app/workbench.py --check firmware/build/demo.vw1`.

KiCad 9 and its Python pcbnew bindings are required for `/usr/bin/python3 tools/check_hardware.py`. The checker runs ERC and DRC, exports a fresh schematic XML netlist, and compares every reference, value, footprint and pin-net mapping against the PCB and manifest. Use `/usr/bin/python3 tools/generate_hardware.py` only when intentionally regenerating the design. The import script is a guarded one-time platform bootstrap and refuses overwriting existing files; it is not part of normal builds. Build products and local KiCad preferences are ignored by Git.

The supplied tests cover every vacancy/source pairing, every possible stable arrival vector from every legal pending transfer, cancellation, repeat traversals in both directions, raw glitches, immediate rearm violations, timestamp wrap, acquisition loss, deadline boundary precedence, counter exhaustion, UART timeout and bounded clock failure. Application tests exercise real encoder recordings, CRC-valid impossible records, framing recovery, FIFO rejection, private export, terminal immutability and live pauses. A real Tk event loop consumes a Linux pseudo-terminal stream and checks screens, batched history, reconnect label clearing and hangup. These are software and simulated-I/O results, not evidence of a physical device experiment.

Before fabrication, visually inspect the rendered schematic and PCB, check connector orientation against the actual harness and verify the purchased board revision against ST UM1724. Before connecting sensors, measure the carrier supply and each sense voltage using a current-limited source. Then validate all pocket combinations, magnetic hysteresis and cross-talk with a logic analyzer. Record independent video while performing known cancellations, wrong-source removals and two-piece handling. If sensor geometry hides these events, improve the fixture rather than loosening firmware rules to make the test pass. Only after those measurements should an educator evaluate usefulness with real demonstrations.

## Reference sources

- ST UM1724, NUCLEO-F401RE connector table and VCP configuration: https://www.st.com/resource/en/user_manual/dm00105823-stm32-nucleo-64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Littelfuse MDSR-7 reed family and logic-level suitability: https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7
- CMU slide-puzzle vacancy-direction discussion: https://www.andrew.cmu.edu/course/15-121/labs/HW-7%20Slide%20Puzzle/lab.html
- Existing commercial slider-position puzzle sensor: https://www.escaperoomsupplier.com/product/slider-sensor/

These sources support component selection and bounded prior-art context, not worldwide novelty or certification. The owner must resolve supplier revisions, final magnet selection and mechanical tolerances before building a physical prototype.
