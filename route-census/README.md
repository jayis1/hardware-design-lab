# Route Census — isolated-release diverter route and transit notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources: MIT.

Revision A is an engineering prototype. The custom PCB is a routed passive four-photogate interface for a separate NUCLEO-F401RE. The processor, regulator, decoupling, debugger and USB bridge are on that purchased board, not on the interface PCB. Software builds and CAD checks do not establish physical measurement accuracy. No board has been fabricated, target flashed, optical assembly calibrated or museum installation tested for this revision.

## Purpose, user and practical value

Route Census helps a museum exhibit maker tune a passive three-way marble diverter without confusing a visually impressive demonstration with a reliable distribution mechanism. Four removable break-beam pairs watch the inlet and the three outlet chutes. The operator releases one opaque ball, waits for the instrument to finish confirming the result, retrieves the ball, and repeats. The instrument accumulates accepted destination counts, transit-time ranges and a coarse time distribution. Overlapping releases, conflicting outlets, blocked beams and overdue arrivals remain visible as rejected trials rather than becoming plausible destinations.

The primary user is the technician developing a tabletop gravity exhibit before opening it to visitors. A diverter might favor the left branch when its base is tilted, stick after a paint change, or behave differently with worn balls. Today the technician asks a colleague to keep three tally columns while somebody else operates a stopwatch. After an interruption, neither person knows whether a particular arrival belonged to the most recent release. A phone video can help diagnose the mechanism, but reviewing repeated trials frame by frame is laborious and camera placement can obscure one of the branches. Route Census provides immediate feedback while keeping the operator's hands away from a touchscreen during release.

A secondary audience is a supervised maker-space class comparing two printed junction geometries. Students can keep the gate positions fixed, collect a baseline, change one physical factor, and export separate notebooks. The tool is not a people counter, a production sorter controller, a gambling fairness certifier, a calibrated particle counter or a machine safety interlock. It does not drive a gate or stop a ball. It deliberately excludes unrestricted concurrent flow because four binary beams cannot establish the identity of multiple balls in a branching network.

The proposed novelty is the combination of a removable diverter test fixture, provisional destination confirmation and explicit rejected-trial accounting, not a claim that photogates or counters are new. The niche is small and the mechanical fixtures vary, making a low-volume open design more appropriate than a universal consumer product. Repository inventory preceded selection. Crossover Track compares cart and lane timing through an ABBA experiment; this device measures three-outlet allocation of isolated releases. Swing Envelope measures bilateral pendulum excursions. Takeup Atlas measures reversal takeup between encoders. None supplies this destination census and ambiguity workflow. No global novelty or patent-clearance claim is made.

| Incumbent | What it does well | Why this instrument is different |
|---|---|---|
| Hand tally and stopwatch | Very low initial cost | Automatically associates one qualified inlet with one provisional outlet |
| Slow-motion phone video | Reveals bouncing and mechanical details | Produces immediate route totals without reviewing each video |
| Generic electronic counter | Counts pulses reliably within its specifications | Distinguishes armed trials, arrivals without an entry and conflicting arrivals |
| Industrial sorter PLC | Controls continuous material handling | This is a removable, receive-only prototype for isolated low-energy experiments |

## Feasibility contract and operating envelope

The acceptance scenario is intentionally narrow: in indoor shaded conditions, send one opaque ball through an inlet beam, then one of three outlet beams, with each blockage and each separating clear interval lasting at least ten milliseconds. The firmware qualifies three successive one-millisecond samples per input. It starts a trial only after all qualified inputs have been clear for one hundred milliseconds. The first outlet is provisional until all gates have cleared for another one hundred milliseconds without additional qualified activity. The whole trial, including confirmation, must finish before thirty seconds from the qualified entry.

A ball arriving at two outlets is not assigned a best guess. Any additional outlet rise during confirmation rejects the trial, including a repeated rise at the same outlet. This deliberately rejects some bouncing balls that a human could identify correctly. Conversely, a second ball arriving after the confirmation window cannot retroactively invalidate a completed trial. The operator must enforce single-ball operation and physically empty the track after every rejection. The hardware cannot prove that an uninstrumented section is empty. The Ready label describes the gate baseline only, not track occupancy.

Choose a small enclosed track with an unobstructed receiver view across each chute. Use opaque plastic balls rather than transparent glass marbles unless actual infrared blocking has been demonstrated. The desired beam span is twenty to fifty millimetres, well below the supplier's approximate maximum range. A large apparent ball is not necessarily opaque at the sensor wavelength. Direct sunlight, reflective interiors, multiple emitters illuminating the wrong receiver, vibration of mounts and an unpowered receiver can all corrupt observations. Separate beam locations, use black light shields, and test one emitter at a time.

## Hardware specifications and architecture

The target is the STM32F401RET6 on a NUCLEO-F401RE, with a Cortex-M4 core, 512 KiB flash and 96 KiB SRAM. Firmware uses the reset-default internal 16 MHz HSI clock, not a crystal-dependent configuration. SysTick supplies a nominal one-millisecond interval. USART2 TX on PA2 sends 115200-baud, eight-data-bit, no-parity, one-stop-bit telemetry through the board's ST-LINK virtual COM port. There is no radio, receive command parser, battery, filesystem or network dependency on the embedded side.

The four external sensor pairs are Adafruit product 2167, the 3 mm IR break-beam version with an open-collector receiver. The product page specifies 3.3–5.5 V supply, approximately 25 cm maximum sensing distance and less than 2 ms response time. The receiver's low output indicates a broken beam according to its wiring guide. This design powers sensors from a separate current-limited regulated 5 V supply. All receiver grounds connect to the interface ground. Do not connect sensor red power wires to signal connectors, and do not substitute push-pull, PNP or industrial 24 V sensors.

    Four emitter/receiver pairs on the track
       inlet       left        middle        right
         |           |            |             |
        J3          J4           J5            J6
         +-----------+------------+-------------+
                 1k series / 10k pullup to 3V3
                            |
                 J2: PA0, PA1, PA4, PB0
                            |
            separate NUCLEO-F401RE / STM32F401RET6
              SysTick -> qualifier -> trial engine
                            |
                   USART2 TX -> ST-LINK USB
                            |
                   Linux Tk companion notebook

    Nucleo 3V3/GND -> J1 -> passive interface
    External regulated 5V -> sensor power harness only
    External ground ------> common interface/Nucleo ground

The custom board measures 80 by 70 mm with two copper layers and a nominal 1.6 mm FR-4 thickness. It uses through-hole headers and axial resistors for hand assembly. There are no mounting holes; use insulated enclosure edge guides, not improvised screws through copper. A proposed bench enclosure is approximately 120 by 100 by 45 mm with strain-relieved sensor cables and separate space for the Nucleo. Those enclosure dimensions are a packaging target, not a supplied mechanical CAD model. Keep sensors near the fixture and cables short, preferably below half a metre for initial bring-up.

## Single electrical and connector map

| Reference | Pin or value | Connection and purpose |
|---|---|---|
| J1 | 1 / 2 | Nucleo 3V3 / GND; never connect external 5V here |
| J2 | 1 | SENSE0 to Nucleo CN8 pin 1, A0 / PA0, inlet |
| J2 | 2 | SENSE1 to CN8 pin 2, A1 / PA1, left outlet |
| J2 | 3 | SENSE2 to CN8 pin 3, A2 / PA4, middle outlet |
| J2 | 4 | SENSE3 to CN8 pin 4, A3 / PB0, right outlet |
| J3, J4, J5, J6 | 1 / 2 | Corresponding receiver signal / common ground |
| R1, R3, R5, R7 | 10k, 1%, axial | VCC 3V3 to corresponding SENSE net |
| R2, R4, R6, R8 | 1k, 1%, axial | SENSE net to receiver signal |

Use Nucleo CN6 pin 4 for 3V3 and pin 6 or 7 for ground. Verify the board revision against UM1724 rather than relying on wire colors. The default virtual-COM configuration has SB13/SB14 closed and SB62/SB63 open. This firmware transmits through the on-board bridge; do not expect the Arduino D1 header to expose PA2 unless its bridge configuration is changed. No bridge modifications are needed for the documented USB arrangement.

With an ideal grounded receiver output, the divider gives approximately 0.30 V at the MCU input and 0.30 mA through a channel. With all channels asserted, pullup current is approximately 1.20 mA. Actual receiver residual voltage and the MCU thresholds must be measured during bring-up; the calculation alone is not qualification. The series resistor limits some accidental current but is not a level shifter, surge suppressor or isolation barrier. There are no TVS devices, reverse-polarity protection or isolated grounds on this passive interface. Do not connect energized machinery, outdoor wiring, long building cables or unknown signal sources.

## Power budget, BOM and assembly

The supplier specifies approximately 20 mA per emitter at 5 V, giving 80 mA for four emitters. Budget a provisional additional 10 mA for each receiver and wiring overhead until measured, yielding a 120 mA sensor allocation. Use a regulated 5 V supply rated for at least 250 mA, initially current-limited to 200 mA after checking polarity. That receiver allowance is an engineering reserve, not a published receiver consumption specification. The Nucleo is powered separately by USB; reserve 150 mA at its USB source as a conservative planning allowance, then measure actual operation. Do not join the separate 5 V positive rails. Only the grounds are shared.

There is no battery-runtime claim. The device is a USB bench instrument and deliberately busy-polls between sampling ticks to drain UART traffic. The interface has no active regulator or thermal source beyond low-power resistors. Nevertheless, check for assembly shorts before power-up. Power down both supplies before rewiring, disconnect the sensor harness before running an unpowered Nucleo, and avoid relying on unspecified receiver behavior during power sequencing.

| Quantity | Item | Planning allowance, USD |
|---:|---|---:|
| 1 | NUCLEO-F401RE | 20 |
| 4 | Adafruit 2167 emitter/receiver pairs | 28 total |
| 4 each | 10k and 1k, 1%, 0.25W axial resistors | 2 total |
| 5 + 1 | 1x2 and 1x4, 2.54mm straight headers | 3 total |
| 1 | Two-layer 80x70mm passive PCB | 8 per prototype allocation |
| 1 set | Regulated sensor supply, harness and insulated fixture | 20 |

The planning total is 81 USD before freight, tax, assembly and the existing laptop. These are budget targets, not current distributor quotations or availability guarantees. Reusing a development board reduces the incremental cost. The justification is time saved during repeated exhibit tuning, not replacing a cheap tally counter in a one-off demonstration. A custom bare-MCU revision could reduce cost after the optical and mechanical assumptions have been validated; it is not silently included in this revision.

Assembly order is resistors, headers, inspection, continuity checks, then connection to an unpowered Nucleo. The square pad identifies header pin one. The sensor harness splits power and signal: red wires go to external 5 V, black wires to common ground, and each receiver's white or yellow signal goes to the matching J3–J6 pin one. Keep a printed connector map beside the jig. Verify that swapping left and middle receivers changes the live bit positions as expected before collecting any comparison data.

## Firmware behavior and design decisions

`main.c` initializes GPIO, UART and SysTick, attempts bounded watchdog startup, initializes an unarmed engine, and begins transmitting snapshots every 100 ms. The board driver reads PA0/PA1/PA4 and PB0, complements the electrical levels, and exposes a four-bit blocked mask. If the millisecond tick changes during the GPIO read, the sample is deferred rather than assigned an invented timestamp. GPIO ports are read sequentially, so very close cross-port edges remain limited by polling and qualification resolution.

`engine.c` independently qualifies each channel. Chatter on an unrelated outlet cannot postpone inlet qualification. Three consecutive samples establish a new accepted level. A raw pulse too short to qualify is deliberately ignored, and the firmware does not claim continuous raw optical stability. A skipped acquisition tick rejects an active trial, increments a sticky gap counter, restarts qualification and requires a new clear baseline. The prior qualified levels are retained across the gap to avoid treating an isolated post-gap clear sample as proof of recovery.

The states are DRAIN=0, READY=1, FLIGHT=2, SETTLE=3 and FAULT=4. DRAIN waits for the qualified clear interval; an outlet without an active trial increments orphan episodes. READY admits one qualified inlet rise. An outlet qualifying in the same vector rejects the new trial as simultaneous. FLIGHT waits for one outlet while checking overlap, blocked-gate and deadline conditions. SETTLE retains the candidate internally without incrementing accepted totals. A new inlet or any new outlet during this phase rejects the trial. Only a complete clear interval commits the destination and transit duration. FAULT is terminal until reset.

A continuously qualified blocked gate reaches its diagnostic threshold at two seconds. Each blocked episode is counted once and the blocked fault flag remains sticky. A thirty-second trial timeout rejects late outcomes, including a candidate whose confirmation does not finish before the deadline. Entries means admitted trials, not every physical inlet passage: releases during DRAIN are not new trials. Rejected counts likewise represent invalid admitted trials, not lost balls. An orphan episode may contain several simultaneous outlet rises. These distinctions prevent inappropriate conservation claims about the physical track.

Accepted route statistics include count, minimum, maximum, a 64-bit duration sum and six histogram bins. Bin upper boundaries are 250, 500, 1000, 2000 and 5000 milliseconds, followed by the remaining valid range below 30000 milliseconds. Boundaries belong to the next bin. Means are descriptive, not calibrated estimates. State-machine conservation requires entries to equal accepted plus rejected plus one when a trial is in FLIGHT or SETTLE. Counter exhaustion enters a visible terminal fault rather than wrapping back to zero. All state is volatile; reset clears the experiment.

`registers.h` uses STM32F401 register addresses from RM0368. Startup code supplies a vector table, initializes data and BSS and enters main. The linker reserves flash at 0x08000000 and RAM at 0x20000000 with a stack-space assertion. No vendor runtime library is bundled or relabeled. The independent watchdog uses a nominal divide-by-64 prescaler and reload 1023; its real timeout depends on the internal low-speed oscillator. UART waits are nonblocking, and an unfinished transmission is abandoned at 100 ms with a cumulative error count. There is no firmware update protocol; recovery uses the Nucleo's independent SWD debugger.

## Wire protocol and application interface

Every record has the form `RTC1,<45 unsigned decimal fields>*<four uppercase CRC hex digits>\n`. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xFFFF, no reflection and no final XOR. It covers every byte from R through the last decimal digit, excluding the asterisk. The known check value for ASCII 123456789 is 0x29B1. Integers are not JSON and there is no escaping. Maximum allocated frame size is 640 bytes. Snapshot repetition supplies version identification even if an application connects after boot.

The first fifteen numbers are sequence, device milliseconds, state, blocked mask, entries, accepted, rejected, orphan episodes, gaps, blocked episodes, last accepted route, last transit milliseconds, rejection reason, sticky flags and UART errors. Each of the three routes then contributes count, minimum, maximum, sum and six bins, in left/middle/right order. Last route is zero when no current accepted result is available; otherwise it is one through three. During a new active trial the prior result is cleared. Reason codes are NONE=0, OVERLAP=1, SIMULTANEOUS=2, MULTIPLE=3, EXPIRED=4, GAP=5, BLOCKED=6 and LIMIT=7. Sticky flag bits are gap=1, blocked=2, counter exhaustion=8 and startup/watchdog failure=16.

The Linux companion is a standard-library Python/Tk application, an appropriate desktop interface for a USB bench instrument. It has a route-census screen, an evidence-snapshot table and a setup/limits screen. It opens the serial device read-only, configures 115200 8N1, and sends no commands. Connection errors are displayed rather than hidden. Permission problems should be fixed by granting access to the serial device through the operating system, not by running the GUI as root. Disconnect retains evidence and source identity; a deliberate reconnect creates a new notebook.

The parser checks CRC, field count, numeric ranges, trial conservation, histogram totals and route sums. It recovers when a truncated frame is immediately followed by a complete RTC1 marker. Replayed recordings must be regular files no larger than four MiB; FIFOs and devices are refused without waiting. An incomplete final record is an error, not silently successful replay. Duplicate frames do not refresh the one-second freshness timer. Conflicting sequence/time progression, disappearing sticky flags or rolling-back cumulative counts freeze acceptance and require an explicit new session. Without a persistent boot identifier, some resets that look like forward traffic remain undetectable; this limitation is not solved by CRC.

Exports are private, exclusively created local JSON files containing author, protocol, live-versus-replay provenance, connection status, freshness age, parser errors, frozen-state warning and up to the last 2000 snapshots. The notebook does not silently overwrite an existing report. Snapshot history is bounded and is not a lossless event log: several trial changes can occur between telemetry snapshots, while cumulative counters still retain their totals. Replay is labeled as such and cannot establish live device freshness. No cloud upload, account, telemetry service or embedded credential is needed.

## Reproduce builds and verification

From the repository root, run the following commands. The firmware test first creates a synthetic raw recording using the production C encoder; the app tests use that recording rather than invented serial data.

    make -C route-census/firmware all test
    xvfb-run -a /usr/bin/python3 route-census/tests/test_app.py
    /usr/bin/python3 route-census/app/census.py --check route-census/firmware/build/fixture.txt
    /usr/bin/python3 route-census/tools/check_hardware.py
    /usr/bin/python3 route-census/app/census.py

Dependencies are GNU Arm Embedded GCC and binutils, host GCC with address/undefined sanitizers, Python 3 with Tk, and KiCad 9 with Python bindings and standard footprint libraries. Xvfb is only needed for headless GUI tests. No pip packages, JavaScript build chain or remote installation scripts are required. `make flash` in the firmware directory uses OpenOCD and the ST-LINK configuration; it is a documented hardware action, not a test that has been performed here. Disconnect sensors and inspect wiring before the first real programming and bring-up session.

The KiCad generator produces a real embedded-symbol schematic, routed board, project metadata, local symbol library and connectivity manifest. `check_hardware.py` runs ERC and DRC, exports a fresh XML netlist, and compares every reference, pin, net, value and footprint against the actual PCB and manifest. Its passive-only ERC cannot validate the external sensor supply or Nucleo circuitry. Source generation requires the distribution's KiCad footprint libraries. The delivered files can be opened directly without regenerating them. All authored files identify jayis1; generated build artifacts and local CAD preferences are ignored.

## Physical validation still required

Before trusting a route distribution, scope the inlet and outlet signals at the MCU-side SENSE pins, check high/low margins, and establish the shortest reliable blockage with the intended balls and lighting. Measure the timing offset against a reference instrument, including HSI drift, sensor latency and independent qualification delay. Repeat at the operating-temperature extremes you intend to use. Deliberately unplug a receiver and block an emitter to determine whether each failure appears blocked or falsely clear; disconnected open-collector wiring is not fail-safe.

Next, compare manual video annotations with instrument results over a planned set of single-ball trials, overlapping releases, bouncing exits and balls lodged between gates. Verify that every rejection is retained in the written report. Reposition and shade the receivers until cross-illumination is eliminated. Test power loss and reconnect without confusing old exports with a fresh run. Do not expose this prototype to unsupervised children: small balls are choking hazards and cables can snag. Keep it away from mains, liquids, pinch points and powered industrial mechanisms.

A clean software test result and zero CAD violations are reproducibility evidence, not product certification. Fabrication inspection, enclosure construction, ESD testing, electrical fault qualification, optical calibration and real user evaluation remain open. The most important next experiment is whether the proposed isolated-release workflow actually saves the exhibit maker time without hiding a systematic sensing bias.

## Technical references

- ST UM1724, Nucleo-64 MB1136 board manual, connector table 16 and USART bridge discussion: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Adafruit 2167 product specifications: https://www.adafruit.com/product/2167
- Break-beam wiring, polarity and sunlight limitations: https://learn.adafruit.com/ir-breakbeam-sensors/arduino

Author and creator of Route Census: jayis1.
