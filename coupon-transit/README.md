# Coupon Transit — two-holder rack-to-rack transfer notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, tests and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype, not a validated material-testing instrument. The supplied PCB is a routed passive contact carrier for an external NUCLEO-F401RE. Its processor, regulator, reset circuit, decoupling, debugger and USB bridge are on that purchased assembly. Software builds and CAD checks are reproducible. Fabrication, programming, electrical bring-up, switch calibration, mechanical repeatability and practical usefulness have not been physically verified.

## Purpose, audience and practical value

Coupon Transit is a removable two-holder observation kit for a craft-materials instructor comparing transfer handling between a preparation rack and an inspection rack. Each labelled coupon holder has one keyed pocket at each rack. The instrument observes departure from one pocket, the interval when neither matching pocket is occupied, and arrival at either pocket. Arrival at the other rack is a delivery; arrival back at the departure rack is a return. Both holders can be carried simultaneously and can move in either direction without imposing a global ordering.

The concrete problem is a small classroom experiment in which two similar paper or polymer coupons are inspected after preparation. A student may lift a holder, hesitate, put it back, then try again. Another may move a second holder while the first remains in hand. A stopwatch started at preparation does not distinguish these movements, and a timer on the destination rack alone does not know which departure it belongs to. Coupon Transit separates successful rack changes from abandoned attempts and same-rack returns while explicitly declining to reconstruct missing observation.

The initial experiment must use inert, dry samples. For example, compare two layouts of a paper-coupon inspection bench and ask whether the awkward layout causes more returns or longer between-rack intervals. Keep sample treatment, labels and final inspection results in an independent experiment sheet. The instrument measures neither drying nor curing nor temperature. It does not know whether a holder contains a coupon at all. Its output supports process investigation rather than certifying a material result or ranking an operator.

The useful physical interaction is simply moving the existing holder. There is no button press at every transfer and no camera watching hands. A small microcontroller timestamps contact observations independently of a desktop event loop. The host provides two readable holder summaries, history and export, but is not required for continued MCU accounting. All counters are volatile; a power interruption loses them. Export important evidence before unplugging anything.

## Originality, alternatives and why this combination

Existing repository directories and README purposes were inventoried before selection. Rest Quartet is the closest sample-rack instrument: it measures uninterrupted residence in four independent pockets and classifies early removal. Coupon Transit instead pairs two pockets per holder, correlates absence between locations, distinguishes return from delivery, and permits two overlapping transfers. Rinse Ledger allocates immersion to declared water batches rather than correlating departure and destination. Teardown Stack learns deposit order and checks reverse retrieval; this design intentionally imposes no stack or global retrieval policy. These measurement contracts are different despite using the same economical contact interface.

Elapsed sample-removal tracking is established prior art. A bounded web search found US9863969, “Method of evaporation control of a sample stored in a cold container,” which describes measuring elapsed removal time and comparing it against a preset time. Laboratory automation also tracks rack transfers, and ordinary stopwatches remain a sensible alternative. This proposal does not invent sample tracking, rack sensing or timers. Its proposed differentiator is a low-cost, local, two-holder teaching workflow that separately preserves return, delivery, ambiguity and continuity-loss evidence with transparent firmware rules. Worldwide novelty, patentability and freedom to operate have not been established.

Why might this exact arrangement not already be common in small studios? Automated laboratory systems are designed for different environments and budgets, while a phone timer is inexpensive but requires manual association with every movement. This is a proposed missing workflow, not a verified claim that no equivalent product exists. A phone video or a borrowed logic analyzer may be the better choice for occasional investigation. The physical fixture earns its cost only if automatic movement logging is less distracting and more repeatable than a paper sheet.

Two alternatives were considered and rejected for this run. Another independent residence rack would overlap Rest Quartet without adding a new observation. A registration-through-tool-stroke recorder would overlap Punch Register and turn into a safety-adjacent workflow. The chosen transfer notebook remains passive, low-energy and concerned with handling evidence, with no permission output, lock, actuator or alarm that claims to protect samples.

## Hardware specifications and block diagram

| Function | Revision A specification |
|---|---|
| MCU assembly | ST NUCLEO-F401RE MB1136, STM32F401RET6 |
| CPU / memory | Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI restored to nominal 16 MHz |
| Sensors | Four external voltage-free normally-open pocket switches |
| Suggested switch | Omron D2F-01L low-level lever switch; validate purchased variant |
| Acquisition | Nominal 1 ms samples; ten consecutive samples qualify each contact |
| Baseline / deadline | 50 ms stable single-dock baseline; 5000 ms transit deadline |
| Carrier | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Interface network | Four 10k pullups and four 1k series resistors |
| Serial | USART2 TX on PA2 AF7, 115200 baud, 8N1 |
| Host bridge | Development board ST-LINK virtual COM over USB |
| Power | USB to Nucleo; regulated 3.3 V and ground to carrier |
| App | Python 3 / Tk desktop; Linux serial, replay and CSV export |
| Actuation / radio | None; host cannot command the target |

    Holder A source pocket ------ J3 A_SRC -- 1k -- SENSE0 -- PA0
    Holder A destination pocket - J4 A_DST -- 1k -- SENSE1 -- PA1
    Holder B source pocket ------ J5 B_SRC -- 1k -- SENSE2 -- PA4
    Holder B destination pocket - J6 B_DST -- 1k -- SENSE3 -- PB0
                                                     |
                                                10k pullups
                                                     |
    USB -> NUCLEO-F401RE 3V3 ----------------------- J1
              |
              +-> 1 ms acquisition -> independent qualifiers
              |                         -> two transfer state machines
              +-> USART2 TX <- CRC snapshots <- conserved counters
                        |
                     ST-LINK VCP -> Linux companion -> private CSV

J1 pin 1 connects to Nucleo CN6 pin 4, regulated 3V3; pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. These analog-header pins are configured as digital inputs. J3 through J6 each use pin 1 for their named signal and pin 2 for ground. The switch closes those pins when its holder is fully seated. A low electrical input is inverted to an occupied bit.

The standard MB1136 virtual-COM bridge arrangement is required; consult ST UM1724 for the actual purchased revision before changing solder bridges. Board support follows the existing repository platform and STM32F401 RM0368 register definitions. No vendor register map is invented. PA2 carries transmit data through the development board; the carrier does not contain a USB controller. UART receive is disabled, and the companion never sends commands. SWD programming is through the purchased ST-LINK assembly.

## Mechanical arrangement and limitations

Build two nonconductive rack bars separated by an initially suggested 250 mm hand-transfer distance. Each bar has two pockets, one for holder A and one for holder B. A starting holder size is 45 by 65 mm with a raised dry gripping tab. Use asymmetrical mechanical keys so A cannot seat in B's pocket. The same A key fits A at both source and destination. These dimensions are starting fixture specifications rather than supplied fabrication-ready enclosure CAD; confirm ergonomics and sensor travel with actual parts.

A separate insulating tab on each holder operates a low-force lever switch near the end of insertion. A rigid pocket stop, not the switch body, must carry insertion force and holder weight. Use slotted switch brackets to adjust the trip point and preserve appropriate overtravel. Do not let the coupon itself operate the switch: soft paper, curling material or a soft adhesive layer can change trip geometry. The holder should support the specimen without clamping a surface whose behavior is under study.

Identity belongs to a pocket pair, not an electronic tag. A wrong holder, a spare object depressing the switch, a hand pressing a lever, or a changed coupon inside the same holder can all produce misleading evidence. Keying and labels reduce mistakes but do not establish chain of custody. Simultaneous source and destination occupancy for one holder is contradictory evidence; the engine does not pick whichever sensor changed first. The device must never be advertised as an authenticated sample tracker.

Keep initial signal leads below half a meter and pair each signal with its ground return. Place the carrier and Nucleo in an insulating enclosure beside the racks, away from the moving holders. The carrier has no mounting holes; use insulated edge supports instead of drilling through routed copper. Allow roughly 160 by 110 by 45 mm for a combined enclosure, then verify board clearances, USB access and cable bends. No ingress rating, ESD qualification or certified mechanical design is supplied.

## BOM, power and electrical safety

The native carrier contains fourteen components: four 10k axial resistors, four 1k axial resistors, five two-pin headers and one four-pin header. Resistors use the standard DIN0207 7.62 mm pitch footprint; select one-percent quarter-watt parts. Headers have 2.54 mm pitch. The external purchase list adds one Nucleo, four suitable low-level switches, mating harnesses, two labelled holders, rack material and an enclosure. Verify exact manufacturer variants, availability and switch low-current suitability before ordering; current stock has not been checked.

Quantity-one planning allowances in USD are Nucleo 25, carrier fabrication allocation 5, resistors 2, headers and harnesses 4, four contacts 12, and simple rack/holder material 10, totaling 58. These are assumptions rather than live quotations, exclude labor, shipping, tax and a purchased enclosure, and presume access to ordinary workshop tools. Elaborate precision fixtures could cost substantially more. Reusing a Nucleo from another experiment is reasonable but requires reviewing its jumper and firmware configuration again.

At nominal 3.3 V, a closed channel draws 3.3 divided by 11000 amperes, or 0.300 mA. The MCU-side node sits approximately 0.300 V above ground through the 1k series resistor. Four closures draw 1.200 mA. These ideal calculations do not measure switch contact resistance, leakage, supply tolerance or MCU noise margin. Verify actual open and closed voltages during bring-up. Switch wetting-current suitability is especially important; an oxidized power contact is not an equivalent low-level switch.

Reserve 100 mA at USB 5 V as an unmeasured system planning allowance and use the Nucleo's supported powering arrangement with a suitable USB source. There is no battery, charger, sleep-duty-cycle claim or battery-life estimate. Do not connect a second supply to J1 while it is attached to Nucleo 3V3. All signals share the USB host ground. The series resistors do not provide galvanic isolation, surge protection, certified hot-plug behavior or mains safety. Never connect PLC outputs, powered sensors, machine-control wiring or unknown voltages. Use dry, inert samples only in the initial prototype; chemical, biological, hot, medical and food-handling applications are outside this revision.

## Firmware architecture and exact measurement contract

Firmware is freestanding C11, with actual Cortex-M4 startup vectors, RAM initialization, linker memory regions, clock restoration, GPIO checks, SysTick, independent watchdog and a bounded nonblocking UART. The engine is isolated from registers so host tests can inject samples without pretending to test physical electronics. The clock, board, transport and carrier generation reuse jayis1's platform from shutter-quartet at fcfd4d1. The holder state machine, CT1 format and holder application are new. The import script records reuse and refuses overwriting existing files; it is not a routine build step.

Each input independently requires ten consecutive nominal samples of its candidate value. The tenth sample accepts the change; there are nine one-millisecond intervals from the first candidate endpoint. This is not a claim that physical contact bounce lasts ten milliseconds. Qualification can hide short pulses and shift observed edges. Chatter on holder B cannot postpone a sustained A contact change. After boot or a gap, known bits distinguish retained levels from newly established observations.

Each holder has WAIT, DOCKED and TRANSIT states. WAIT requires exactly one of its two contacts qualified occupied and the current raw pair to match. The first matching endpoint earns no retrospective time. Fifty further matching intervals establish DOCKED and remember origin 1 for source or 2 for destination. Any raw disagreement restarts that baseline. Starting with no holder or with both docks occupied never invents a prior location or a transit.

From DOCKED, qualified absence at both contacts starts TRANSIT only if the raw pair is also absent. The departure timestamp is that accepted observation. A direct change from source to destination without accepted absence is ambiguous, not a zero-time delivery. Arrival is evaluated only after departure: a single occupied dock with matching raw evidence completes the attempt. Opposite-origin arrival increments delivered; same-origin arrival increments returned. Completion at exactly the 5000 ms deadline is rejected because timeout takes precedence. Qualified arrivals at 4999 ms can complete.

After either successful outcome the holder returns to WAIT and must earn another dock baseline. Last duration and outcome remain available while waiting or transferring again. Delivery sum and maximum include only opposite-rack deliveries; returns have their own count and can occupy the latest-outcome fields but do not contaminate the delivery mean. A return's duration is retained only while it remains the latest successful outcome. The protocol is not a permanent individual-event archive.

Contradictory qualified occupancy or raw disagreement at a proposed departure/arrival increments ambiguities and abandons a pending attempt. A deadline increments timeouts and abandons it. A missed acquisition interval, including two milliseconds between samples, increments gaps, abandons both pending transfers, and restarts qualification. No unknown interval is credited. Re-establishing a stable single dock permits new correlation; it does not retrospectively classify the abandoned holder's journey.

For each holder, started equals delivered plus returned plus abandoned plus one when TRANSIT. Updates across both holders are staged transactionally. If any counter or delivery sum would overflow, all speculative accounting for that sample is discarded and the previously pending transfers are abandoned exactly once before terminal halt. This prevents A from publishing a delivery when B's arithmetic fails. Fault bits are configuration 1, watchdog initialization 2, exhaustion 4 and invalid raw input 8. Terminal holder evidence freezes; uptime and transport diagnostics may continue.

The watchdog is fed after acquisition. Clock waits before SysTick use bounded iteration budgets; watchdog-start waits use tick deadlines. UART transmission has a 100 ms timeout and a fixed 768-byte buffer. An incomplete frame increments tx and can be replaced by a later complete snapshot. Ordinary uptime wrap is handled with unsigned arithmetic. Watchdog behavior, HSI timing accuracy and peripheral startup on physical silicon remain bring-up checks, not host-test results.

## Serial protocol and companion screens

Approximately every 100 ms the target emits CT1 followed by unsigned decimal fields separated by commas. The seven global fields are now, mask, known, gaps, tx, fault and halted. They are followed by thirteen fields for A, then thirteen for B: state, origin, elapsed, started, delivered, returned, abandoned, sum_ms, max_ms, last_ms, last_kind, ambiguities and timeouts. State codes are WAIT=0, DOCKED=1 and TRANSIT=2. Last-kind codes are none=0, delivered=1 and returned=2. Global mask bits are A source, A destination, B source and B destination from least to most significant.

The suffix is an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE covers CT1 and all fields before the asterisk, with polynomial 0x1021, initial 0xffff, no reflection and no final XOR. CT1 fixes the qualification, baseline and deadline policy; changing it incompatibly requires a new marker. Periodic self-identifying snapshots allow late attachment without a one-shot boot record. CRC detects accidental corruption, not authenticity or specimen identity.

The Holder transfers screen displays both phases, origins, delivery/return/abandonment counts, latest duration and delivery-only mean and maximum. Snapshot notebook retains up to 500 accepted rows and displays cumulative evidence. Fixture and interpretation explains wiring, operation and limitations. Toolbar actions connect an explicitly selected Linux tty, disconnect, replay a saved capture and export CSV. No server, cloud account, browser permission, radio pairing or third-party Python package is required. The appropriate desktop interface avoids adding a wireless stack solely to satisfy a mobile-app template.

Parsing checks length, marker, decimal syntax, CRC, integer range, state/occupancy agreement and counter conservation. Session checks reject rollback, contradictory duplicates, disappearing faults, impossible delivery aggregates, continuing-transit time changes, implausible start rates and latest-outcome contradictions. These are necessary consistency bounds, not a proof that every possible accepted sparse sequence was physically realizable. Several outcomes may occur between emitted snapshots; cumulative counts survive but individual timing details may be absent.

Live freshness requires two advancing timestamps and an open connection. Duplicate timestamps do not refresh it. A reception pause beyond one second or cumulative host/device progress disagreement beyond half a second blocks resumed acceptance until reconnect. The fixed drift allowance intentionally does not grow forever; an uncalibrated oscillator can force conservative reconnects during long captures. Reset inference is heuristic because there is no persistent boot identity. Some forward-looking resets remain indistinguishable from continued acquisition.

Disconnect preserves provenance and evidence. Successful reconnect starts a fresh desktop notebook but does not clear MCU totals; failed connection setup preserves the old session. Replay requires a regular file no larger than two MiB, rejects devices and FIFOs, rejects malformed frames and partial EOF tails, and is always offline. Export exclusively creates a private CSV and refuses overwrite. Its metadata includes jayis1, source, connection state, freshness, age, parser errors, blocked reason and evicted rows even when no transfer completed.

## Reproduction and acceptance plan

From the repository root run:

    make -C coupon-transit/firmware all test
    xvfb-run -a /usr/bin/python3 -m unittest discover -s coupon-transit/tests -p 'test_*.py' -v
    /usr/bin/python3 coupon-transit/tools/check_hardware.py
    /usr/bin/python3 coupon-transit/tools/check_quality.py
    /usr/bin/python3 coupon-transit/app/workbench.py

GNU Arm Embedded GCC and binutils build and link the real target ELF and binary. Native GCC executes address/undefined-behavior sanitizer tests. Python with Tk supplies the real desktop app; Xvfb is needed only for headless GUI testing. KiCad 9 and its standard footprint libraries supply pcbnew and native ERC/DRC. Use the distribution Python providing those bindings. No npm or pip install is needed. Linux termios is intentional; Windows serial support is not claimed.

The hardware generator can reproduce the native schematic, board, project, local symbol library and connectivity manifest. The checker executes native ERC and DRC and compares a fresh schematic XML netlist against actual PCB pads, component values, references and full footprint identities. Symbols contain real numbered pins and assigned footprints; the board contains routed nets and a closed outline. Passive ERC does not validate the external supply or Nucleo bridges. Review the rendered schematic as well as machine checks before fabrication.

Before applying power, inspect polarity, resistor values and continuity against the schematic. Power the Nucleo alone, then connect the carrier with USB removed. Measure every open and closed input. Flash only the positively identified reviewed board using the Makefile flash target and OpenOCD; no flashing was performed during automated validation. Compare known contact sequences with a logic analyzer and an independent timer. Verify both directions, same-rack return, simultaneous independent transfers, direct handover, both-dock contradiction, timeout, startup empty and an intentional unplug.

Finally compare repeated holder movement with independent video, including gentle insertion, slow removal, a bounce-prone placement and wrong-holder attempts. Record fixture geometry and switch variants. Compare two bench layouts in alternating order rather than collecting all observations from one layout first. Preserve rejected attempts and report whether the fixture itself changes handling. The first real usefulness gate is an instructor deciding that the notebook explains a repeatable workflow difference more clearly than a stopwatch. Software and CAD checks alone cannot establish that result.

## Sources and attribution

- ST Nucleo-64 MB1136 manual UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 reference manual RM0368: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Bounded prior-art comparison, US9863969B2: https://patents.google.com/patent/US9863969B2/en
- Repository platform provenance: tools/import_platform.py; source device shutter-quartet at fcfd4d1.

All original authored materials credit jayis1. See VALIDATION.md for actual executed checks, source counts and remaining unverified physical work. This release is a build-tested design proposal, not a fabricated or certified product.
