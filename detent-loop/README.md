# Detent Loop — hand-operated catch force–travel notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is an engineering prototype. Target compilation, automated software tests, schematic connectivity and PCB design-rule checking are reproducible. The design has not been fabricated, flashed, physically calibrated or tested on a catch. The custom PCB is a passive interface for a separately purchased NUCLEO-F401RE, not a standalone MCU board. Nominal force estimates must not be presented as calibrated measurements.

## Purpose, overview and why this should exist

Detent Loop is a small manual instrument for comparing the feel of sliding catches in noncritical model enclosures, educational mechanisms and reusable workshop boxes. A person experimenting with a printed detent often asks two different questions: how hard does the catch resist outward movement, and how much resistance remains on the way back? A single maximum-force number loses the second question. A phone video preserves motion but cannot directly observe the reaction force. This device records crosshead travel and elastic reaction together, separates the outward and return legs, and retains explicitly incomplete or invalid attempts.

The intended user is a maker comparing two geometries of a reusable enclosure catch. The moving half of the catch is clamped to a hand-driven slide. The other half is attached to a compliant reaction leaf. One resistive sensor follows the slide and another follows the reaction leaf. Two physical buttons establish the zero, start, turnaround and finish events. The desktop application plots a coarse force–travel loop and exports the actual snapshots and qualifications rather than issuing an opaque pass/fail judgement.

The practical alternative is a spring scale, ruler and notebook, or a proper motorized materials tester. The former is inexpensive but awkward to read simultaneously during a reversal; the latter offers controlled speed, traceable force measurement and much better performance at much greater cost. The proposed benefit is easier comparison of slow, clearly different catch geometries under the same fixture conditions. Whether that saves time or improves design decisions must be evaluated with a real workshop trial. No percentage improvement, measurement accuracy or commercial demand has been measured.

This is not a universal tensile tester, a calibrated load cell, a fatigue machine, an emergency-release evaluator or a safety-interlock qualification tool. It does not test child-resistant packaging, lifting hardware, restraint buckles or medical components. It does not control motion, arrest a falling weight or protect fingers. Its useful scope is deliberately low-energy, hand-operated, noncritical prototyping.

## Originality and prior art

The repository directories and README concepts were inventoried before implementation. Creep Pair follows foam displacement after loading and unloading at timed milestones; it neither estimates reaction force nor integrates a path. Takeup Atlas observes rotary reversal and the first response of a second shaft, without a calibrated ratio or force channel. Cam Dwell Map bins a digital follower state by cam angle. Detent Loop instead combines an elastic reaction reference with an analog crosshead coordinate and explicit two-leg manual acquisition. It reuses jayis1's existing ADC, UART and passive-carrier platform but supplies a different experiment engine and application.

Force–displacement testing of snap mechanisms already exists. IMADA's “Tactile Feeling Test for Snap-fit Hinged Caps” describes visualizing reaction force during cap closure. Covestro's “Snap-Fit Joints for Plastic” discusses mating forces and friction in snap geometry. The paper “Large deformation analysis of a backside-supported snap-fit with nonlinear behavior,” Heliyon 2023, uses displacement sensing and a load cell to study assembly and separation. These are important close alternatives, not evidence that force–travel curves were invented here.

The proposed original contribution is the particular small, open, receive-only workshop workflow: deliberately qualified zero, physical leg boundaries, signed work summaries, conservative rejection of reverse motion and a local record of uncertainty. This is differentiated within the inventoried repository, not a patentability opinion or proof of worldwide uniqueness. A commercial force gauge remains preferable when the force result itself must be trusted.

References and component documentation:

- IMADA application: https://www.forcegauge.net/en/solution/force/compression_test/69716
- Snap-fit analysis: https://doi.org/10.1016/j.heliyon.2023.e19388
- Covestro design guide: https://solutions.covestro.com/-/media/covestro/solution-center/brands/downloads/imported/1557218421.pdf
- Bourns PTA slide potentiometers: https://www.bourns.com/docs/Product-Datasheets/pta.pdf
- ST Nucleo platform manual UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 reference manual RM0368: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

External search confirmed closely related prior art. This run did not verify distributor inventory, prices or every current supplier document revision. Check the exact purchased parts and development-board revision before assembly.

## Hardware specifications

| Subsystem | Revision A selection |
| --- | --- |
| Processor assembly | ST NUCLEO-F401RE, STM32F401RET6 |
| CPU and memory | Cortex-M4 at nominal 16 MHz HSI, 512 KiB flash, 96 KiB SRAM |
| Travel sensor | Bourns PTA4543-2015CPB104, linear 100 kohm, 45 mm slide |
| Reaction sensor | Second identical slide potentiometer, sensing leaf-tip displacement |
| User inputs | Two normally-open, signal-rated dry-contact pushbuttons: ARM and TURN |
| ADC acquisition | ADC1 channels 0, 1, 4 and 8; discard then eight-sample mean per channel |
| Observation rate | Nominal 50 Hz; accepted sample spacing 20–25 ms |
| Transport | USART2 TX, 115200 baud 8N1, Nucleo ST-LINK virtual COM |
| Power | Nucleo USB; carrier receives regulated 3.3 V and ground only |
| Custom PCB | 80 by 70 mm, two copper layers, 1.6 mm nominal FR-4 |
| Companion platform | Linux, Python 3, Tk; no third-party Python packages |
| Actuation and radio | None; no battery, charger, Bluetooth or cloud service |

The F401 is selected because the purchased development board already supplies programming, USB serial, regulation and decoupling, and the complete freestanding firmware can be target-linked locally. No FPGA is useful for this slow manual experiment. The custom board's native schematic contains fourteen real connected components: eight resistors and six headers. The external processor and sensors are intentionally represented by interface connectors; their physical location is explicit in this document, not hidden behind a claim of a complete custom MCU PCB.

## Architecture and block diagram

    hand slide ---- moving catch half -> mating half on reaction leaf
        |                                         |
    travel rheostat                          spring rheostat
        | J3                                      | J4
        +--- 1k --- SENSE0              SENSE1 --- 1k ---+
                       |                  |
                    100k to 3V3        100k to 3V3
                       |                  |
                     PA0                PA1

    ARM button --- J5 --- 1k --- PA4 + 100k pullup
    TURN button -- J6 --- 1k --- PB0 + 100k pullup

    USB -> NUCLEO-F401RE -> ADC -> zero / outward / return engine
                                    -> signed path integration
                                    -> CRC snapshots -> USART2 / ST-LINK
                                    -> Linux live view / plot / private CSV

Every sensor connector has signal on pin 1 and ground on pin 2. J1 pin 1 is 3.3 V from Nucleo CN6 pin 4; J1 pin 2 is ground from CN6 pin 6. J2 pins 1–4 connect to Nucleo CN8 pins 1–4, corresponding to PA0/A0, PA1/A1, PA4/A2 and PB0/A3. J3 is TRAVEL, J4 is SPRING, J5 is ARM and J6 is TURN. USART2 uses PA2 alternate function AF7. Confirm the actual MB1136 board revision and virtual-COM solder bridges in UM1724; the usual routing is SB13/SB14 fitted and SB62/SB63 open.

Each input has a 100 kohm pullup on the MCU side of a 1 kohm series resistor. The two potentiometers operate as rheostats to ground. Identify the wiper and end terminals with an ohmmeter; tie the unused end to the wiper and orient motion so outward travel and positive reaction both increase resistance. Do not substitute logarithmic-taper sensors. The nominal conversion is R = 100000 × ADC / (4095 − ADC) − 1000 ohms, clamped to zero at the low end. Full-scale is invalid. Analog channels outside 80–2300 counts invalidate the experiment. Shorted or disconnected leads normally violate those bounds, but many damaged sensors can still produce plausible readings.

## Mechanical fixture and dimensional design

Use a rigid insulating base approximately 280 by 140 mm. Fit a manually driven linear slide with at least 25 mm available motion and adjustable mechanical stops limiting recorded travel to 0–20 mm. Clamp the moving catch half to the slide. Fix the travel sensor parallel to the slide and couple its actuator through a short articulated link. Set its zero near 40 kohm rather than at an end stop. The moving slide must be supported by its own guide; the sensor is not a bearing or a mechanical travel stop.

The reaction element is a strip of certified full-hard AISI 301 spring stainless, nominally 75 mm long, 10 mm wide and 1.0 mm thick. Clamp 25 mm of its length between two flat blocks, leaving a 50 mm free cantilever. Load its free end perpendicular to the strip's broad face, along the slide axis. Attach the fixed catch half to a lightweight tip bracket at the same load point sensed by the second potentiometer. Provide adjustable stops at plus and minus 4 mm leaf-tip deflection and a transparent guard around the leaf and catch. Radius and deburr the strip edges, and use stock with documented mechanical properties; do not improvise a brittle hardened strip or use a visibly cracked clamp region.

A simple small-deflection beam estimate with E = 193 GPa gives stiffness 3.86 N/mm and nominal root bending stress 463.2 MPa at 4 mm tip displacement. Stored elastic energy at that displacement is about 30.88 mJ. These calculations are planning estimates, not finite-element validation, fatigue qualification or a measured spring constant. Clamp compliance, tip-bracket mass, material variation, geometric nonlinearity and the sensor linkage all change the result. The firmware uses a rounded nominal 4 N/mm, but this value must be replaced by a measured effective calibration before interpreting forces quantitatively.

Mount the reaction sensor parallel to tip motion with a loose articulated link and its zero near midrange. Its slide force is a serious limitation: the PTA family specifies appreciable operating force, not a frictionless displacement probe. Sensor drag and leaf hysteresis can become part of the observed loop. Calibrate the complete assembled reaction subsystem in both directions using an independent force gauge and reject the approach if residual hysteresis is comparable with the catch differences under study. Do not subtract a guessed friction constant. The first experiment should be a blank fixture with no catch, followed by a compliant reference with independently known behavior.

The reported coordinate is crosshead travel, not the relative displacement between the two catch halves. Leaf deflection moves the nominally fixed half. Consequently the reported signed work includes elastic storage and fixture effects; it is not automatically the energy dissipated in the specimen. Returning close to the unloaded home reduces, but does not prove elimination of, residual elastic storage. Keep this distinction in captions, exported notes and any comparison report.

Retain the carrier PCB in an insulating edge-support tray; there are no mounting holes in this revision. Allow roughly 160 by 110 by 45 mm for the Nucleo, carrier and wiring, adjacent to the mechanical base. Provide strain relief and covered terminals. This is a dimensional fixture specification, not a supplied machining drawing or validated enclosure model. Fit and assembly tolerances require a physical build review.

## Power budget, BOM and safety boundaries

Reserve a 5 V, 500 mA USB supply allowance for the Nucleo and debugger. Actual consumption was not measured. The four passive input paths draw at most approximately 0.131 mA total at nominal 3.3 V with all inputs shorted. There is no sleep mode, battery runtime or charging claim. Continuous polling is intentional for a tethered bench instrument.

| Item | Quantity | Assembly requirement |
| --- | --- | --- |
| NUCLEO-F401RE | 1 | Purchased ST development board with ST-LINK |
| PTA4543-2015CPB104 | 2 | External 100 kohm linear slide potentiometers |
| 100 kohm axial resistor | 4 | 1%, quarter-watt, 7.62 mm lead pitch |
| 1 kohm axial resistor | 4 | Same package and tolerance |
| 2-pin 2.54 mm header | 5 | J1 and J3–J6 |
| 4-pin 2.54 mm header | 1 | J2 |
| Signal-rated NO pushbutton | 2 | Reliable dry switching at microampere current |
| Two-layer carrier PCB | 1 | Native KiCad layout supplied |
| Full-hard 301 strip | 1 | 75 × 10 × 1 mm, 50 mm free length |
| Slide, clamps, guard, stops | 1 set | Must support parts independently of sensors |

An initial quantity-one planning allowance is USD 90–180 excluding a reference force gauge and machining labor. At one hundred units, USD 65–130 per assembly is a provisional materials allowance while retaining the Nucleo. These are estimates, not supplier quotations. A maker already owning a suitable slide and reference gauge may find the workflow worthwhile; a professional laboratory should prefer traceable instrumentation rather than spend time defending a low-cost rheostat arrangement.

Keep sensor leads shorter than half a meter for initial bring-up. Only passive disconnected contacts and resistive sensors may be attached. USB shares ground with the computer; there is no galvanic isolation, TVS network, industrial transient protection or mains safety rating. A 1 kohm resistor is not an overvoltage barrier. Do not hot-plug a charged external circuit, apply a powered PLC output or attach a specimen that can release stored hazardous energy. Cover pinch points, keep hands outside the guarded catch region, and use the mechanical stops rather than the firmware as overload protection.

## Firmware details and design decisions

The firmware is freestanding C11 with explicit reset vectors, data/BSS initialization and STM32F401RE linker regions. It restores the internal HSI clock and prescalers, configures analog pins and UART transmission, starts a 1 ms SysTick and enables a bounded independent-watchdog setup. ADC conversion waits, clock waits and UART transmission are bounded. The longest ADC sampling interval, one discarded conversion and an eight-conversion mean reduce settling sensitivity, but do not prove ADC accuracy with the actual wiring.

There are seven states: WAIT=0, ZERO=1, READY=2, OUT=3, BACK=4, DONE=5 and INVALID=6. Buttons qualify independently after three consecutive samples; closed is at most 200 counts and open is at least 3800 counts. Intermediate contact readings discard qualification. No command from the computer can operate the state machine or drive the fixture.

At startup the instrument waits for an explicit ARM press while TURN is open. Keep ARM held for fifty stable samples. Both resistance channels must remain within a 200 ohm peak-to-peak band; motion restarts the zero window. A raw ARM release or raw TURN closure interrupts zeroing immediately. An accepted zero freezes both resistance baselines. Release ARM, then press it again to start the outward leg while the fixture is within 0.5 mm of home and within nominal 2 N of zero reaction. This two-stage action prevents an automatic reboot from silently declaring an arbitrary loaded position a completed tare.

Move outward slowly, release ARM, then press TURN only after at least 5 mm observed travel. Return toward home, release TURN, and press ARM to finish. Completion requires home within plus or minus 0.5 mm and force within plus or minus nominal 2 N. Repeated phase buttons, premature turnaround and premature finish invalidate the trial. These thresholds are policy tolerances, not measurement accuracy specifications.

Each active sample computes nominal position from resistance using 2222 ohms/mm and reaction using 4000 mN/mm. The outward leg records the maximum positive reaction and greatest travel; the return leg records the most negative reaction. A trapezoidal signed integral accumulates force times signed crosshead movement independently for each leg, using 64-bit twice-nanojoule arithmetic and converting only when publishing microjoules. Negative return work is allowed if the mechanism gives energy back. The sum is a fixture-inclusive signed loop integral, not necessarily positive specimen loss.

Travel outside −0.5 to 20 mm, nominal force outside ±20 N, a step greater than 2 mm per sample, movement more than 0.5 mm against the current leg's extreme, or an active duration over 120 seconds invalidates capture. The physical leaf stops are intentionally tighter than the software force ceiling. Sampling intervals outside 20–25 ms, converter failure, UART timeout and watchdog-start failure also invalidate capture. Terminal summaries freeze; later diagnostics can still mark a previously completed record invalid. Reset is required for another trial and discards volatile data, so export first.

## Application and wire interface

Run `python3 app/detent.py` from this folder on Linux with Tk installed and permission to open the selected serial device. The Connection / live screen shows state, current acquisition validity and faults. The Force–travel loop screen plots outward and return samples in different colors and shows peaks, leg work and their signed sum. The Evidence screen retains up to 2,000 snapshots, shows the latest hundred and writes a new private CSV with session metadata and operator notes. No cloud account, network connection, pip installation or mobile pairing is required.

Every 200 ms the controller emits an ASCII CSV snapshot beginning with D1. The nineteen numeric fields after the marker are, in order: sequence, device milliseconds, state, sticky faults, acquisition-valid, ADC0, ADC1, ADC2, ADC3, baseline0 ohms, baseline1 ohms, crosshead micrometers, force millinewtons, active elapsed milliseconds, maximum span micrometers, outward peak millinewtons, return minimum millinewtons, outward microjoules and return microjoules. A final comma, four uppercase hexadecimal CRC digits and LF end the record. CRC-16/CCITT-FALSE covers the bytes before the final comma, with polynomial 0x1021 and initial value 0xFFFF. D1 denotes the fixed nominal calibration policy in this revision; changing its scale or meaning requires a new protocol version and matching parser.

Fault bits are range=1, sampling gap=2, ADC=4, UART=8, watchdog=16, sequence=32, direction=64, speed=128 and timeout=256. Sequence and device time wrap modulo 2^32. Positive and negative signed fields are decimal. Full periodic snapshots allow late attachment, including attachment after completion, but cannot prove uninterrupted boot identity. A reboot that happens to produce plausible forward sequence, time and summary fields may remain indistinguishable. CRC protects against accidental corruption, not a malicious serial producer.

The application rejects malformed frames, impossible ranges, contradictory duplicate sequences, regressing timestamps, disappearing sticky faults, changing established baselines and changed terminal summaries. Duplicate identical packets do not refresh freshness. More than two seconds without device progress, a long host reception pause or excessive divergence between host time and accumulated device time blocks live capture and requires reconnection. Numerical terminal values are labeled retained rather than displayed as new measurements merely because telemetry continues.

Replay accepts only regular files of at most two MiB, validates every line including the final newline, and labels data as replay. Synthetic test recordings are explicitly not physical measurements. Exports refuse overwrite and use permissions 0600; even an empty export includes author, provenance, parser-error count, connection status, age, notes and calibration warning. Disconnect preserves evidence. A successful new connection clears prior capture, whereas failed connection setup preserves it. Export before reconnecting. CSV notes are untrusted text; use care with spreadsheet import behavior.

The plot receives five snapshots per second while firmware computes at fifty samples per second. Peaks and integrated work therefore need not agree with integrating the displayed points. This is intentional and documented, not hidden interpolation. Neither rate captures a fast snap transient reliably; operate slowly and treat narrow impulses as unresolved. A higher-speed force transducer and acquisition chain are needed for tactile click metrology.

## Reproducible verification and physical bring-up

Run `make -C firmware all test` to cross-link the actual Cortex-M4 target and execute host engine tests with address and undefined-behavior sanitizers. Run `xvfb-run -a python3 tests/test_app.py` for parser, replay, real Tk event-loop, pseudoterminal, export and disconnect checks. Run `/usr/bin/python3 tools/check_hardware.py` for KiCad ERC, DRC and exact comparison of a freshly exported schematic netlist, PCB pads and connectivity manifest. Run `python3 tools/verify.py` for the combined content and execution gates. CAD regeneration uses `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9 pcbnew bindings.

Flash only a reviewed assembly using `make -C firmware flash`; that command invokes OpenOCD with ST-LINK and STM32F4 target scripts. Flashing was not performed during software verification. Before connecting sensors, check polarity, resistance values, header numbering and shorts. Exercise each channel with known fixed resistors and contacts, then deliberately disconnect an analog sensor and confirm invalidation. Check UART framing and timing on the real board. Inspect the leaf, guards and travel stops before attaching any catch.

Next calibrate travel with independent dimensional references over the full intended stroke. Calibrate reaction force in both directions, including dwell and reversal, using the complete leaf, bracket and sensor assembly. Record zero drift, blank-loop hysteresis and repeatability. If those errors are comparable with the specimen differences, the concept fails its intended use and needs a different transducer; do not conceal the result by smoothing. Temperature, speed, conditioning, clamp position and the particular printed specimen must be recorded for every comparison.

ERC on a passive carrier cannot validate the purchased Nucleo or external sensor wiring. Zero DRC violations cannot establish assembly quality, material strength, electrostatic discharge performance or fitness for purpose. See VERIFICATION.md for checks actually executed and remaining physical work. This project provides inspectable engineering sources and an executable prototype workflow, not a manufacturing release or certified test instrument.
