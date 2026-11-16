# Aisle Pair — opposing-drawer shared-space occupancy notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, tests, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. The startup, clock, GPIO, UART, passive carrier and serial transport infrastructure is reused explicitly from jayis1's Bin Phrase at df23325. The occupancy engine, AY1 interpretation, matrix presentation and associated tests are specific to this design.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive interface for a separately purchased NUCLEO-F401RE, not a standalone processor board. Target linking, synthetic-input tests, native electrical checks and desktop workflows are reproducible. Fabrication, flashing, electrical measurements, fixture construction, physical calibration and usefulness with real users remain unverified. This is not an anti-tip interlock, clearance instrument, evacuation monitor, furniture certification or protective system.

## Purpose, audience and practical value

Aisle Pair records the joint extension stages of two opposing miniature storage drawers. Each drawer has two maintained position thresholds, producing three ordinal stages. The device accumulates a three-by-three occupancy matrix, explicitly separates unclassifiable intervals, and reports how long both drawers were observed beyond their first thresholds at the same time. The result is a local, camera-free notebook for comparing storage layouts on a small tabletop mockup.

A workshop planning storage on both sides of a narrow work area can use a scale model to explore retrieval habits before building furniture. A designer might place frequently used supplies opposite each other, then move one group diagonally and repeat the same retrieval exercise. A simple open counter cannot distinguish short independent openings from prolonged simultaneous extension. A conventional stopwatch requires somebody to watch both drawers and maintain categories. Aisle Pair records those categories automatically while the operator handles lightweight dummy objects.

The target users are furniture-design students, makerspace educators and small-workshop planners. The intended question is whether a particular arrangement encourages both opposing drawers to remain extended together, and at what coarse extension stages. It is not whether a person can fit through the resulting space. There is no aisle-width sensor, person detector, force sensor, mass measurement or geometric collision model. Actual required access clearances must come from appropriate design standards and physical review, not this device's matrix.

A practical comparative protocol uses two small captive drawers mounted on a board, a fixed set of inert retrieval tasks, and identical thresholds across conditions. Run a baseline arrangement, export the observations, reset the instrument, rearrange the dummy contents, then repeat. Keep the retrieval task, drawer travel and sensing geometry constant. A lower shared-stage total may justify a follow-up usability experiment; it does not prove that a real workplace is safer or that the arrangement causes the difference. The device records a proxy for joint extension, not intent, workload or ergonomic quality.

## Distinction, prior art and scope

The repository directories and README descriptions were inventoried before selection. Rail Skew's complete purpose, pin contract and measurement rules were inspected: it compares two rails of the same constrained drawer during direction-anchored traverses. Aisle Pair instead observes two independently movable opposing drawers continuously and retains nine joint occupancy categories. It does not classify opening-origin versus closing-origin journeys or rail skew. Nest Settle observes seating at four corners during placement trials; Bin Phrase records tray-delimited ordered lid openings. Neither produces this shared-space matrix.

Mechanical drawer interlocks are established prior art. A bounded web search returned LEHMANN's SVS Interlock, described as allowing only one drawer to open at a time, and older drawer-interlock patents. See https://www.lehmann-locks.com/en/interlock-svs and https://www.freepatentsonline.com/5303994.html . Aisle Pair deliberately does not implement those protective functions. Independent drawer position loggers, switches, video observation and occupancy histograms are also established techniques. This proposal makes no claim of worldwide novelty, patentability, commercial absence or freedom to operate.

The specific contribution is a modest comparative fixture, a conservative interval-accounting contract, bundled threshold qualification, explicit unknown coverage, and a functioning local application. Reusing a verified passive platform is intentional; the electrical interface itself is not claimed as a new invention. A phone video is preferable when actual hand paths, object identity or continuous drawer displacement are needed. Paper notes are preferable when only the final storage arrangement matters. No cloud service, microphone, camera or personal identifier is required here.

## Hardware specifications

The processor is STM32F401RET6 on ST NUCLEO-F401RE, with a Cortex-M4 core, 512 KiB flash and 96 KiB SRAM. The firmware restores the internal nominal 16 MHz HSI reset clock before peripheral initialization. SysTick supplies a nominal one-millisecond sampling clock. USART2 transmits at 115200 baud, eight data bits, no parity and one stop bit over the Nucleo ST-LINK virtual COM connection. There is no application command receiver, wireless interface, network service, battery, charger or actuator.

The passive carrier is 80 by 70 mm, two copper layers and nominally 1.6 mm thick. It contains fourteen placed components: six headers and eight axial resistors. The MCU, regulator, decoupling, reset circuit, USB bridge and debugger live on the purchased development board. The custom PCB therefore cannot operate by itself. The board uses 0.30 mm tracks, nominal 0.20 mm clearance and one through via with 0.80 mm diameter and 0.40 mm drill. Native KiCad files contain actual symbols, numbered pins, net labels, footprints, tracks and board outline.

Four low-force, voltage-free contacts report maintained extension thresholds. A candidate is an Omron D2F-01L low-level lever-switch family, subject to confirming the exact purchased suffix's operating force, travel and minimum switching requirements. A cam strip on each miniature drawer maintains the early contact closed after the first threshold and the late contact closed after the second threshold. A narrow tab that produces only a passing pulse is not suitable. Both thresholds must remain asserted through the rest of extension travel.

### Fixed wiring and bill of materials

| Bit | Meaning | Field connector pin 1 | MCU signal | Nucleo connector |
|---|---|---|---|---|
| 0 | A beyond early threshold | J3, A_EARLY | PA0 | CN8.1 / A0 |
| 1 | A beyond late threshold | J4, A_LATE | PA1 | CN8.2 / A1 |
| 2 | B beyond early threshold | J5, B_EARLY | PA4 | CN8.3 / A2 |
| 3 | B beyond late threshold | J6, B_LATE | PB0 | CN8.4 / A3 |

All field connector pin-two terminals are GND. J2 pins one through four carry SENSE0 through SENSE3 to the four Nucleo inputs in the table. J1 pin one takes only regulated 3V3 from CN6.4; pin two connects to CN6.6 ground. Do not connect J1 to 5 V or VIN. The square pad marks pin one; verify cable orientation with continuity testing rather than assuming the drawing's visual orientation matches a mating cable.

R1, R3, R5 and R7 are 10 kohm pullups from VCC to the corresponding sensing nodes. R2, R4, R6 and R8 are 1 kohm series resistors from the sensing nodes to field contacts. Use one-percent DIN0207 axial resistors with 7.62 mm lead spacing, rated at least 0.125 W. J1 and J3 through J6 use two-pin 2.54 mm headers; J2 uses a four-pin header. The BOM also requires the Nucleo, USB data cable, four switches, insulated harnesses, mechanical fixture, enclosure and strain relief. These external assemblies are not hidden inside the PCB BOM.

An ideal closed contact draws 0.300 mA through its 11 kohm path at 3.3 V and leaves approximately 0.300 V at the sensing pin. All four closed paths draw 1.20 mA. These are nominal circuit calculations, not bench measurements. Budget 150 mA at USB 5 V for the development board as an unmeasured planning allowance, then measure the assembled system. No battery-life, thermal endurance or production power rating is claimed.

There is no isolation, reverse-polarity protection or dedicated cable surge protection. USB and sensor grounds are common. A series resistor is not a safety barrier. Use only disconnected voltage-free contacts; never attach powered machine inputs, PLC outputs, lighting wiring, motor controls or live switches. Begin with paired signal/ground leads shorter than half a metre on a dry indoor bench. An open wire resembles an unasserted threshold; a short resembles an asserted threshold. The device cannot uniquely diagnose either condition.

Manufacturer references are ST UM1724 for Nucleo-64 MB1136 connectors and solder bridges, RM0368 for STM32F401 registers, and PM0214 for the Cortex-M4 core. The standard virtual-COM assumption is SB13/SB14 fitted and SB62/SB63 open. Confirm the exact board revision before wiring. Reference URLs: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf and https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf .

## Mechanical arrangement and commissioning

Build a tabletop mockup with two lightweight opposing captive drawers, approximately 100 mm travel each, supported on a rigid base. Place early and late threshold transitions around 25 mm and 75 mm of extension as initial mockup dimensions. These are proposed geometry, not released manufacturing tolerances. Use insulated cam strips with a maintained high plateau beyond each transition. Keep switches outside pinch regions, protect their levers, and provide independent hard stops so neither the switch nor its bracket carries drawer stopping loads.

Position the drawers so they cannot collide, even when both are fully extended. The region between them represents shared space for the experiment, not a passage occupied by a person. Do not attach this prototype to a real evacuation route or use it to justify reducing access clearances. Keep the entire mockup low and stable, secure loose parts, use blunt inert contents, and stop if the sensing force changes how the drawers move. A physical drawer interlock, if fitted for any reason, must remain independent and unmodified.

The carrier has no mounting holes. Use insulating edge rails inside an enclosure with sufficient clearance for its 80 by 70 mm outline; do not drill through routed copper. Mount the Nucleo separately with USB and reset accessible. Put labels on both ends of every harness. Mechanical design remains a prototype assembly specification rather than validated enclosure CAD. Humidity resistance, cleaning compatibility, repeated-cycle wear and cable fatigue require physical evaluation.

Commission with four loose switches before mounting. Confirm each input's electrical low is displayed as logical one. For each drawer, verify raw local codes proceed 00, 01, 11 during extension and reverse during retraction. Code 10 is forbidden because the late threshold should never be asserted without the early threshold. Hold each stage for at least 20 ms during initial tests. Then compare a slow recorded mockup exercise with independent observation or video. Test both drawers independently, both together, threshold chatter, disconnected contacts and an intentionally misadjusted late switch.

## Architecture and block diagram

```text
A early maintained cam -- J3 -- 1k -- SENSE0 -- PA0
A late maintained cam  -- J4 -- 1k -- SENSE1 -- PA1
B early maintained cam -- J5 -- 1k -- SENSE2 -- PA4
B late maintained cam  -- J6 -- 1k -- SENSE3 -- PB0
                                      |
                           each SENSE: 10k to 3V3
                                      |
USB -> NUCLEO-F401RE -> 1 ms sampling -> two bundled qualifiers
                                      |
                    3 x 3 matrix + unknown interval bucket
                    shared segments / current and longest run
                                      |
                        AY1 ASCII snapshots with CRC16
                                      |
                       ST-LINK virtual COM -> Linux Tk
                       setup / matrix / history and CSV
```

The system never drives a drawer or grants permission to move one. An indicator in the application is an observation, not a control output. Raw input acquisition remains in the MCU; desktop scheduling does not determine the matrix. The host receives periodic cumulative snapshots rather than individual motion events. Missing serial records can therefore leave useful cumulative totals, but cannot reconstruct the detailed motion history between them.

## Firmware details and evidence contract

Each drawer's two bits qualify as a bundle after five consecutive matching samples. The first observation counts as one, so qualification spans four sampled intervals. A and B qualify independently; chatter on A cannot postpone B's qualification. Bundling avoids synthesizing a local position from two different per-bit histories. Local 00 means stage zero, 01 means stage one, and 11 means stage two. Local 10 is invalid. Stage numbers are ordinal threshold regions, not measured distance, extension percentage or occupied width.

The engine starts in WAIT, enters OBSERVE when both bundles have qualified, and enters terminal HALT on an acquisition gap, configuration failure, watchdog-initialization failure or counter exhaustion. It does not require closed drawers at attachment because its result is occupancy rather than an inferred journey. Initial observations are explicitly unknown. Reboot starts a new volatile notebook; there is no flash persistence or command to clear selected counters while preserving others.

For every pair of consecutive acquired samples, exactly one millisecond interval is assigned either to one matrix cell or to unknown. Cell attribution requires both endpoints to have fully qualified legal words, raw inputs equal to the qualified words, and the same complete qualified vector at both endpoints. Thus qualification boundaries, stage transitions, forbidden codes and sampled glitches are unknown. The first four intervals are necessarily unknown. A short raw disturbance that never changes qualified state still splits the interval record instead of disappearing from the evidence.

Cell index is three times A's stage plus B's stage. Shared exposure is the sum of cells four, five, seven and eight: both drawers beyond their early thresholds. Each contiguous run of credited shared intervals is an observed shared segment. An unknown interval, a nonshared cell or a terminal fault resets the current run; the maximum remains retained. Moving between two shared cells also introduces an unknown boundary and splits the segment. Consequently segment counts are not unique physical conflicts, and the maximum is a longest clean observed run rather than the full duration of a physically continuous obstruction.

The invalid counter counts acquired samples with both bundles qualified and a forbidden qualified word, even while a returning raw word is still qualifying. It is not a count of broken wires or separate malfunctions. The invariant is intervals equals sequence minus one after acquisition begins, and sum of the nine cells plus unknown equals intervals. Shared totals, segment count and longest run obey additional bounds enforced before serialization. No observation credits missed time. A detected nonconsecutive sample freezes acquisition evidence and requires a new run.

Counters are bounded at one billion. The final permitted sample is fully accounted; the next sample halts without partially updating a cell. Run maxima and segment products use sufficiently wide validation arithmetic. The nominal millisecond clock wraps modularly, while the acquisition sequence does not wrap. SysTick and processor faults can still invalidate physical time assumptions: the firmware detects observed software-tick gaps, not every possible hardware interrupt failure. An independent calibrated reference is required before interpreting timing accuracy.

The shared platform includes bounded reset-clock waits, GPIO configuration checks, a nonblocking UART queue and a 100 ms transmit timeout. UART failure increments a diagnostic without blocking acquisition. The independent watchdog is fed after acquisition work; its physical behavior remains untested. Startup cannot establish a valid interface if clock restoration fails. Normal builds use no dynamic allocation, vendor HAL or external runtime library beyond compiler support routines.

## Protocol and companion application

AY1 is the fixed version marker. A frame contains the marker, twenty-four comma-separated unsigned decimal fields, an asterisk, four uppercase CRC hexadecimal digits and LF. CRC-16/CCITT-FALSE covers bytes before the asterisk, starting at 0xffff with polynomial 0x1021. The check string 123456789 produces 0x29b1. CRC detects damaged bytes; it does not authenticate the device or make a plausible measurement true.

Field order is sequence, now, phase, fault, raw, qualified, valid, intervals, unknown, invalid, episodes, run, maximum, gaps, tx_errors, followed by bin0 through bin8. Phase values are zero WAIT, one OBSERVE and three HALT; two is reserved and rejected. Fault bits are configuration one, watchdog two, acquisition gap four and overflow eight. Valid bits zero and one indicate the A and B bundles. All cumulative measurements describe the current firmware boot's acquisition epoch; now is a modular software timestamp and not civil time.

The Linux application uses Python standard-library Tk, POSIX serial, CSV and termios facilities. Its setup screen offers port selection, scanning, connect, disconnect and strict recording replay. The matrix screen presents all nine totals and explicit unknown coverage, current position, shared segments, longest run and diagnostics. The history screen retains at most two thousand received snapshots and exports those retained rows, not an unlimited archive. Firmware totals remain cumulative even when old host rows are evicted.

The parser bounds frames and replay sizes, validates CRC and field shape, checks conservation, rejects decreasing totals, and freezes on contradictory duplicate timestamps. Sparse transition rules constrain growth by actual acquisition and shared credit, not merely wall time. Adjacent snapshots receive exact matrix-attribution checks. These are necessary endpoint constraints, not proof that every hidden sample occurred as claimed. An unexpected version, malformed field or impossible transition is never silently displayed as trusted data.

Live status requires advancing acquisition, not merely recent bytes. Duplicate records do not refresh it. A reception pause or cumulative host/device drift exceeding two seconds requires reconnect; freshness expires after one second without acquisition progress. The fixed drift allowance can conservatively reject long runs with oscillator error. A stopped observer can still transmit current timestamps while HALT remains visibly stale. There is no persistent boot identifier: counter or clock rollback catches many resets, but some resets followed by sufficient activity can be indistinguishable. Keep experimental runs short and explicitly export/reset between conditions.

Disconnect retains the evidence and provenance but removes live status. Successful replacement connection clears the old matrix immediately; a failed open preserves the prior session. Replay requires disconnection, accepts only regular files up to two MiB, and rejects empty files and truncated tails. CSV uses exclusive creation with private permissions, refuses overwrite, and includes author, source, connection state, freshness, parser errors and blocking reason even when no useful measurements exist. No data is transmitted to a network service.

## Build, test and use

The validated environment uses GNU Arm Embedded GCC 14.2.1, native GCC, Python 3.13.5 with Tk, KiCad 9.0.2 with its system Python bindings, and Xvfb for GUI tests. There are no pip or JavaScript dependencies. On a compatible Linux installation, run the following from the repository root:

```sh
make -C aisle-pair/firmware clean all test fixture
xvfb-run -a /usr/bin/python3 -m unittest discover -s aisle-pair/tests -p 'test_*.py'
/usr/bin/python3 aisle-pair/tools/check_hardware.py
/usr/bin/python3 aisle-pair/app/workbench.py
```

The fixture target emits firmware/build/demo.ay1 from the real C engine and encoder. It is synthetic test data, not a recording from hardware. Replay it to exercise all matrix cells and the terminal acquisition-gap display. The import script records platform provenance and deliberately refuses overwrites; do not run it on an already populated design. The hardware generator can regenerate native CAD but should only be used when intentionally reviewing changes to those deliverables.

With an assembled, independently inspected unit, the Makefile flash target uses OpenOCD and the Nucleo onboard ST-LINK. Flashing has not been performed in this run. Check supply and connector orientation first, disconnect external contacts during initial programming, then use loose test switches. Serial-port permissions should be granted through the operating system rather than running the companion as root. Export before pressing reset because firmware evidence is volatile.

## Validation limits and next engineering gate

VALIDATION.md records the executed builds, tests and CAD results separately from physical unknowns. Software checks cover all legal stage pairs and ordered transitions, glitches, forbidden codes, clock wrap, gaps, counter boundaries, parser corruption, sparse impossible records, replay, private export and real Tk event-loop operation with Linux pseudo-terminals. Native ERC, DRC and exact schematic-to-PCB pin comparison establish properties of the supplied files, not manufacturing quality or sensor performance.

The next gate is a supervised hardware bring-up: inspect fabrication, measure supply and closed-contact levels, verify all connector mappings, compare MCU timing with a reference, test unplug and reset behavior, then characterize cam repeatability and sensor loading. Only after that should a comparative mockup study begin. No fabrication order, physical programming, electrical certification, furniture test, human-subject study or deployment approval is implied by publishing this engineering package.
