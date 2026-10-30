# Sieve Shuttle — two-axis motion notebook for hand-sieving experiments

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. The supplied custom PCB is a routed passive contact carrier for a separately purchased NUCLEO-F401RE. Firmware target linking, simulated-input tests, desktop workflows and CAD checks are reproducible. No board has been fabricated, target flashed, fixture assembled, particle separation measured or physical calibration performed. Do not confuse a passing software test with a validated scientific instrument.

## Purpose, audience and practical value

Sieve Shuttle records the motion pattern of a small, manually moved sieve tray. Four stationary magnetic zones surround its permitted planar movement: west, east, north and south. A magnet attached to the tray closes a reed contact in each zone. Firmware turns qualified, separated visits into a transition matrix, counts complete opposite-zone traversals on each axis, records same-zone returns and axis changes, and retains the longest uninterrupted run of opposite traversals for each axis. The companion displays those results while keeping provisional visits, acquisition faults and stale transport visible.

The primary user is a workshop instructor comparing how beginners move a coarse teaching sieve. Two students may spend the same time shaking while one repeatedly crosses the same axis and the other changes direction frequently. A stopwatch and final retained mass do not explain that difference. This notebook offers a limited, inspectable motion record to accompany the physical outcome. The instructor can compare repeated runs using the same tray, granules and mesh, rather than asking a second observer to count every directional pass.

A secondary user is a small craft-material experimenter developing a repeatable manual recipe for screening coarse, inert granules. The device does not control the sieve or recommend a passing recipe. It helps the user record whether two nominally similar trials actually contained similar directional patterns. A simple exercise uses equal initial quantities, a prescribed number of X traversals, a turn toward north, and a prescribed number of Y traversals. A scale or separate particle analysis must establish what material passed through; no such measurement is implemented here.

The primary interaction is moving one captive tray. The computer is not touched for every stroke. A phone tally app therefore does not replace the fixture-derived record unless someone manually observes and enters the directional events. Video tracking is a credible alternative and can recover much richer trajectories, but requires camera placement, lighting, calibration and offline interpretation. This design deliberately trades that information for four binary zones and a small local notebook.

## Originality, alternatives and non-goals

The repository directories and README titles were inventoried before selecting this concept. The closest designs were read in more detail. Sand Stroke observes one-dimensional sanding traversals while a contact declares engagement and another selects a coupon face. Swing Envelope associates pendulum crossings with left/right outer thresholds. Maze Return performs chronological loop erasure on checkpoint walks. Sieve Shuttle instead measures directional recipe composition in a planar manual sieving fixture through a directed four-zone transition matrix and axis-run statistics. Its problem and user interaction differ even though its electrical interface intentionally reuses the established passive platform.

Three candidate concepts were considered: a generic sieve timer, a vibration-amplitude monitor, and this zone-transition notebook. The timer does not justify dedicated hardware. An amplitude monitor would need dynamic sensor calibration and could easily overstate the relationship between acceleration and separation quality. The zone notebook is more limited but provides an independently testable event contract and an inexpensive fixture for teaching directional recipes. The unique contribution proposed here is that combination, not the resistor network or the idea of counting motion.

Mechanical sieve shakers, laboratory acquisition systems, cameras and ordinary counters are adjacent alternatives. No worldwide novelty, patentability, freedom-to-operate or absence-of-commercial-equivalents claim has been established. This is an original project proposal within this repository, not a patent search. A sufficiently programmable logger could implement the same algorithm. Physical testing must determine whether the constrained fixture improves useful repeatability or merely teaches an artificial movement that does not transfer to ordinary hand sieving.

This device does not measure particle size, sieve efficiency, force, acceleration, distance, total mechanical work, mesh wear, mixing quality or operator competence. A zone-to-zone traversal can include pauses and unobserved reversals. A turn means a change between the X and Y zone pairs, not a measured rotation angle or a complete circular orbit. A return means another separated visit to the same zone, not proof that the tray moved a minimum distance. The application uses these literal distinctions rather than a misleading percentage-complete indicator.

## Hardware specifications

| Item | Revision A specification |
|---|---|
| Processor module | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Core / memory | Cortex-M4; 512 KiB flash; 96 KiB SRAM |
| Clock | Nominal 16 MHz internal HSI |
| Sensors | Four external normally-open, voltage-free magnetic reed contacts |
| Carrier | 80 × 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Interface components | Four 10k pull-ups, four 1k series resistors, six headers |
| Connectivity | USART2 TX through onboard ST-LINK USB virtual COM |
| Serial settings | 115200 baud, 8 data bits, no parity, one stop bit |
| Input cadence | Nominal 1 ms polling; independent 5 ms qualification |
| Clearing policy | 100 ms with both raw and qualified inputs clear |
| Storage | Volatile MCU totals; local bounded desktop history and JSON export |
| Supply | USB to Nucleo; regulated 3.3 V from Nucleo to carrier |
| Outputs | Telemetry only; no motion control, radio or safety interlock |

The Nucleo supplies the processor, regulator, reset, programming interface, USB bridge and decoupling. The custom schematic represents only the passive carrier, not a fictional integrated MCU board. The assembly connects through a short harness and is not a shield with mating Arduino geometry. Relevant manufacturer references are ST RM0368 for STM32F401 registers and ST UM1724 for MB1136 pin assignments and VCP routing. Verify the purchased board revision and USART2 solder-bridge configuration before wiring.

| Header | Net and connection |
|---|---|
| J1.1 | VCC to Nucleo CN6.4 regulated 3V3 |
| J1.2 | GND to Nucleo CN6.6 ground |
| J2.1 | SENSE0 to CN8.1 A0 / PA0; west, bit 0 |
| J2.2 | SENSE1 to CN8.2 A1 / PA1; east, bit 1 |
| J2.3 | SENSE2 to CN8.3 A2 / PA4; north, bit 2 |
| J2.4 | SENSE3 to CN8.4 A3 / PB0; south, bit 3 |
| J3.1, J4.1, J5.1, J6.1 | WEST, EAST, NORTH, SOUTH contact signals |
| J3.2 through J6.2 | Individual contact returns to GND |
| Nucleo PA2 | USART2 TX alternate function 7, onboard VCP routing |

Each SENSE node is pulled toward 3.3 V through 10k and connected to its contact through 1k. Firmware inverts the GPIO level, so a closed contact produces a set mask bit. At nominal voltage a closed contact draws 3.3 / 11000 A, approximately 0.30 mA, with approximately 0.30 V at SENSE. All four closed contacts draw approximately 1.20 mA and dissipate 3.96 mW across the resistor network. This all-closed case is electrically considered but rejected as ambiguous measurement evidence. These are nominal calculations, not bench measurements. Reserve a provisional 150 mA at USB 5 V for the development board and interface until actual current is measured.

The input circuit accepts disconnected voltage-free contacts only. There is no galvanic isolation, surge suppression, protected external supply input or qualified ESD design. Never attach powered sensor outputs, PLC wiring, mains switches or another instrument's excited contacts. USB shares ground with the host. Short paired leads, provisionally below half a metre, and strain relief are required. An open cable looks like an absent visit; a shorted cable looks like continuous occupancy. Software cannot uniquely identify either fault.

## Architecture and block diagram

```text
           Captive lightweight sieve tray with retained magnet
                     moving slowly on a planar guide
                         W    E    N    S zones
                         |    |    |    |
                   four normally-open reed contacts
                         |    |    |    |
                       J3   J4   J5   J6
                         |    |    |    |
                       1k series resistors
                         |    |    |    |
                SENSE0..3 -- individual 10k pulls -- 3V3
                         |    |    |    |
                         J2 -> PA0 PA1 PA4 PB0
                                      |
 USB power -> NUCLEO-F401RE -> independent qualification
                                      |
                      provisional visit / clear-window state machine
                                      |
                      transition matrix + X/Y runs + sticky faults
                                      |
                     SS1 frames -> PA2 -> ST-LINK USB VCP
                                      |
                   Linux Tk: Connection | Motion balance | History
                                      |
                     strict raw replay / private JSON evidence export
```

There is no command receiver. Desktop actions cannot change thresholds, clear MCU state or actuate a mechanism. Changing acquisition policy requires a source review, rebuild and physical reflashing through ST-LINK. USB reconnect is only a new host session; reset the board explicitly to start new MCU totals. Volatile evidence disappears on power loss, so export before intentional disconnection of power.

## Mechanical fixture and commissioning

Build a small lightweight tray with an interchangeable coarse mesh insert over a catch pan. Support it on a captive two-axis planar guide, with mechanical travel stops that do not load the sensors. A reasonable exploratory layout uses a 200 × 200 mm base, a roughly 90 mm tray, and approximately 30 mm permitted movement from center toward each cardinal zone. These are starting dimensions, not validated production drawings. The delivered CAD describes the electrical carrier; it does not supply a manufactured guide, certified containment or dimensioned universal enclosure.

Attach one firmly retained magnet below the tray and mount four reed contacts to the stationary base. A normally-open Littelfuse MDSR-7-family part is a candidate, but sensitivity, magnet orientation and actual closure geometry must be selected experimentally. Keep glass packages protected and unload their leads mechanically. Establish disjoint closure regions with no two contacts active together. A side path must allow return to the clear center between visits. If the fixture geometry creates overlapping zones or cannot provide the specified clear interval, change the fixture rather than interpreting faulted records as valid trials.

Operate slowly for initial experiments. Hold each zone for at least 20 ms and allow at least 150 ms clear before entering the next zone. Firmware requires five milliseconds between the first unchanged candidate and qualification, followed by a full hundred milliseconds of qualified clear evidence after departure. This is not intended to capture high-frequency powered shaking. Reed hysteresis and magnetic attraction can perturb motion; compare independent video with and without the fixtures before asserting that measurements represent ordinary hand technique.

Use coarse inert teaching granules in a covered catch arrangement. Do not use respirable silica, combustible powders, food-contact work, hazardous chemicals or valuable samples. Keep fingers clear of the slide mechanism and remove loose magnets from children. The board belongs outside the material path in an insulating tray or enclosure. It has no mounting holes; use edge supports rather than drilling through copper. The design carries no ingress-protection, EMC, medical, food-safety or metrological certification.

Commission electrically before adding material. Inspect solder joints, confirm no VCC-to-GND short, check each resistor and map every connector to the table. With the MCU disconnected, use continuity measurements to verify each contact path. Then power from USB, inspect all four input masks, and move the magnet slowly into each zone independently. Verify raw closures and clearing without competing zones. Compare a known scripted sequence against the expected transition matrix. Only after those checks should an instructor compare actual sieving outcomes with independent mass measurements.

## Firmware implementation and measurement contract

The firmware is freestanding C11. It includes direct register definitions, reset startup, a linker script, GPIO acquisition, reset-clock restoration, a bounded nonblocking UART transmitter and an independent watchdog. Clock initialization uses iteration-bounded waits before SysTick exists. Watchdog setup has bounded waits after SysTick starts. Input configuration is checked before measurements. No vendor SDK, heap, radio stack, flash journal or dynamic allocation is required. The foreground owns the complete measurement state; SysTick only advances a millisecond counter.

WAIT requires all channels qualified and a full clear interval. ARMED accepts one qualified zone whose raw level agrees. CAPTURE retains that provisional zone until all raw and qualified inputs have remained clear for 100 ms. Re-entry into the same zone before the clearing interval ends remains part of the same provisional visit; it does not create a return count. Any other raw or qualified zone during CAPTURE halts correlation, even if that competing raw pulse is too short to qualify. Raw simultaneous closures also halt immediately. These conservative guards prevent qualification delays from manufacturing a definite ordering.

A committed first visit establishes an anchor but does not create a transition. Every later committed visit increments exactly one of the sixteen directed matrix bins. West-to-east or east-to-west increments X; north-to-south or south-to-north increments Y. A diagonal bin increments returns. All eight cross-axis bins increment turns. Therefore visits equals one anchor, when present, plus X, Y, returns and turns. The matrix sum equals visits minus the anchor. Validation checks those relationships before serialization.

Opposite-zone transitions extend the current run if the axis matches. A transition on the other axis begins a new run, while a same-zone return or adjacent-axis turn clears the current run. Longest X and Y runs remain cumulative. For example W, E, W produces two X traversals and an X peak of two. W, N, S produces one turn and one Y traversal. W, W produces one return and no traversal. These examples count only completed visits separated by the clearing interval.

A capture reaching 30,000 ms halts before it can commit on that deadline. A sampling interval greater than 5 ms halts and records one gap; shorter scheduling variation is tolerated by the elapsed-time qualifier and is not individually reported. Sub-millisecond activity and intermediate states between samples are not observed. This means the design establishes sampled stability, not continuous electrical stability. Counter limits are one billion. Visit updates are staged transactionally, so capacity exhaustion cannot publish a matrix increment without its associated totals. HALT freezes correlation, qualifiers and provisional evidence; device time, raw levels and transport diagnostics may continue. Recovery requires an explicit board reset, not a convenient quiet interval.

## Companion application and protocol

A Linux Python 3/Tk application is appropriate for a bench fixture connected through USB. Run `python3 app/workbench.py`. The Connection screen selects the serial path, connects, disconnects, opens a raw recording and exports evidence with experiment notes. Motion balance shows X/Y counts, returns, turns, peaks, pending clearing and a four-by-four matrix. History shows every retained received snapshot, up to 2,000 rows. There is no simulated-data button that could be mistaken for live hardware.

Each line is `SS1,<37 unsigned decimal fields>*<four uppercase hex CRC digits>\n`. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial value 0xffff and covers all bytes before the asterisk. The ordered fields are now, raw, mask, qualified, state, fault, visits, x, y, returns, turns, gaps, tx_errors, last, pending, clear_ms, elapsed, axis, run, peak_x, peak_y, followed by m0 through m15. Matrix index is four times previous zone plus current zone. Zones are W=0, E=1, N=2, S=3, absent=4. Axis is X=0, Y=1, absent=2. States are WAIT=0, ARMED=1, CAPTURE=2, HALT=3. Fault bits from low to high denote configuration, watchdog, counter capacity, sampling gap, ambiguous zones and capture timeout.

The firmware sends approximately every 100 ms when the transmitter is available. Frames carry their version marker every time, so late attachment does not require a lost boot announcement. Parsing is bounded, validates CRC and accounting, rejects malformed numbers, and resynchronizes on the full marker after truncation. Strict replay rejects partial tails, empty input, FIFOs and files over two MiB. Raw SS1 recordings are the replay format; exported JSON is an evidence report, not a raw recording and not directly replayable.

The session model rejects counter rollback, contradictory duplicate timestamps, disappearing faults and changed terminal evidence. A single newly committed visit must produce exactly the corresponding matrix update and run/peak transition. Larger telemetry gaps receive conservation and rate checks, not a proof of complete trajectory realizability. Timestamp reset inference is heuristic: without persistent boot identity, some resets can remain indistinguishable from forward traffic. CRC is corruption detection, not authentication.

Live labeling requires advancing device timestamps and a connected transport. Duplicate packets do not renew freshness. More than one second without progress is stale; a reception pause over 1.5 seconds or accumulated host/device progress disagreement over one second blocks acceptance and requires reconnect. The fixed allowance deliberately fails conservatively over long runs with oscillator drift. Exports retain source, connection and freshness state, parser errors, blocking reason, notes and snapshots, even when no visit completed. Files are created exclusively with mode 0600 and never overwrite an existing path. There is no cloud service, listener, credential, automatic upload or network dependency.

## Reproduction, costs and validation boundaries

From this device directory, run:

```sh
make -C firmware all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
/usr/bin/python3 tools/check_hardware.py
python3 tools/check_quality.py
python3 app/workbench.py
```

The target build uses GNU Arm Embedded GCC and links a complete ELF and binary. `make -C firmware flash` invokes OpenOCD with ST-LINK and STM32F4 configuration; it must only be run with the correct physical board attached. No flashing was performed during this design run. KiCad generation uses distribution Python with pcbnew and installed standard footprints; generation is not required to open the delivered files. The import scripts document source provenance and intentionally refuse to overwrite existing files. Normal users do not need to run them.

The approximate planning BOM is one NUCLEO-F401RE, four normally-open reed contacts, four retained mounting positions, one magnet, eight axial resistors, five two-pin headers, one four-pin header, the custom PCB, a USB cable and insulated harness. Allow roughly US$25–40 for the development board, US$8–20 for contacts and magnet, US$5–15 for carrier fabrication and passive components in a small batch, and US$15–40 for mechanical parts and an enclosure. These are rough budget allowances, not distributor quotes or verified availability. Reusing an existing Nucleo reduces incremental cost; a mechanically stable fixture can cost more than its electronics.

The host engine tests enumerate every six-zone word over four identities against an independent classification oracle. Additional cases exercise clearing boundaries, rejected pulses, overlapping raw zones, competing arrivals, timeout, sampling loss, counter exhaustion, CRC and millisecond wrap. Application tests ingest actual C-encoder synthetic fixtures and exercise framing, invalid snapshots, resets, freshness, export privacy, replay limits and serial restoration. GUI tests use real Tk event handling and Linux pseudo-terminals, not physical USB or reed sensors. CAD checks require zero ERC and DRC violations, zero unconnected items, and exact schematic/PCB/manifest agreement for component values, footprints and every pin/net.

See VALIDATION.md for the executed results and tool versions. These checks establish properties of the delivered files. Outstanding work includes physical assembly, magnet-zone characterization, real serial timing, oscillator calibration, material containment, operator usability and controlled comparison against independent sieving outcomes. None of those pending activities is silently replaced by synthetic fixtures.
