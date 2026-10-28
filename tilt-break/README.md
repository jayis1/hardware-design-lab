# Tilt Break — release-bracket and downstream-arrival incline notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is a build-tested engineering prototype. The custom PCB is a routed passive sensor interface for a separately purchased NUCLEO-F401RE. No board has been fabricated, target flashed, fixture assembled, material calibrated or physical release measured. Software tests and CAD checks establish properties of these files, not the accuracy or usefulness of a finished instrument.

## Purpose, user and overview

Tilt Break helps a small packaging or model-making workshop compare the tendency of a lightweight coupon sled to remain parked on a slowly raised surface. Its particular job is to retain two pieces of evidence that a single slide-angle number hides: the ramp-position bracket around departure from a home gate, and whether that departure was followed by arrival at a downstream gate. A puck that leaves its home field but sticks again is not silently given the same record as one that traverses the observed section. A stalled, reversed, interrupted or improperly ordered attempt remains visibly invalid.

The intended user is comparing noncritical shelf liners, paper coatings, plastic surfaces or model conveyor coverings. The current low-cost workaround is to raise a board by hand, read a ruler and watch a puck simultaneously. A phone video can help, but synchronizing a small displacement with an angle reading is awkward, and an observer can accidentally remember only the clean slides. This instrument records the observations locally and makes failed attempts difficult to overlook. It does not improve the underlying material preparation or make hand-driven tilt speed reproducible.

The proposed contribution is an inexpensive, inspectable release-bracket workflow, rather than invention of inclined-plane friction testing. An accepted run contains explicit operator zeroing, a valid initial gate pattern, the last closed-home and first candidate-open height observations, three-sample departure qualification, and a separately qualified downstream arrival. The user can compare raw resistance evidence even before a trustworthy geometry calibration is available. Nominal angles are secondary derived values and must remain labeled as such.

A practical prototype success criterion is to distinguish deliberately different low-angle coupons in a repeated blinded workshop experiment while retaining all interrupted runs. Before such a study, an independent angle reference should verify the derived bracket at several heights, and a camera should verify the reed-gate release and arrival ordering. No accuracy, time-saving percentage, discrimination threshold or test-standard compliance has yet been measured. The first physical test can disprove the usefulness of the proposal; software success cannot substitute for it.

## Distinction from existing work and prior art

The repository directory inventory and README descriptions were checked before selecting this concept. Detent Loop measures crosshead displacement and elastic reaction during two manual motion legs and estimates fixture-inclusive work. Creep Pair follows foam compression and recovery at timed milestones. Crossover Track compares cart transit through an ABBA lane-swap study. Route Census records isolated releases through alternative paths. Tilt Break instead follows continuously increasing support height during a single home departure, freezes a departure bracket, and checks subsequent progress along that same incline. It reuses jayis1's existing freestanding F401 ADC/UART and passive-carrier platform; the experiment engine and companion are specific to this device.

Commercial inclined-plane testers already exist. The RDM CF-200i raises a bed at a controlled rate, detects initial sled motion optically and reports slip angle and tangent. Such equipment is a better choice for standardized or traceable friction testing. Tilt Break is not presented as a cheaper equivalent: it is manual, uses reed-field departure rather than first optical motion, and deliberately does not publish a coefficient of friction. Its limited originality is the open local workflow and retained departure-to-arrival evidence for small workshop experiments. Worldwide uniqueness, patentability and absence of similar maker projects are not claimed.

References:

- RDM CF-200i: https://rdmtest.com/product/cf-200i-inclined-plane-coefficient-of-friction-tester/
- Bourns PTA slide-potentiometer datasheet: https://www.bourns.com/docs/Product-Datasheets/pta.pdf
- ST UM1724 Nucleo platform manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 reference manual RM0368: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

The Bourns datasheet was retrieved for this design. It specifies linear taper options, 45 mm travel, substantial resistance tolerance and appreciable sliding force. The ST manual fetch failed during this run; the board mapping follows the existing repository platform and must be checked against the exact purchased MB1136 revision before assembly. Distributor stock, prices and lifecycle status were not independently established.

## Hardware specifications and BOM

| Subsystem | Revision A choice |
| --- | --- |
| Processor | NUCLEO-F401RE, STM32F401RET6, Cortex-M4 |
| Clock and memory | Nominal 16 MHz internal HSI; 512 KiB flash, 96 KiB RAM |
| Height sensor | Bourns PTA4543-2015CPB104; 45 mm, linear 100 kohm slide potentiometer |
| Departure/arrival sensors | Two normally-open, signal-rated reed switches actuated by one captive puck magnet |
| User control | One normally-open ARM pushbutton, suitable for microampere dry switching |
| Analog input network | Four 100 kohm pullups and four 1 kohm series resistors, 1% axial |
| Connectors | Five 2-pin and one 4-pin 2.54 mm headers |
| Custom PCB | 80 by 70 mm, two copper layers, 1.6 mm FR-4 |
| Power | Nucleo USB supply; passive carrier at regulated 3.3 V |
| Connectivity | USART2 TX at 115200 baud, 8N1, through ST-LINK virtual COM |
| Application | Linux Python 3 and Tk; standard library only |
| Actuation | Hand-operated tilt screw; no motor, power output or radio |

The MCU, decoupling, USB bridge, regulator and programmer are on the purchased development board, not on the custom carrier. The native KiCad schematic and PCB contain fourteen connected components: eight resistors and six connectors. The external sensors are deliberately connected through headers rather than represented as imaginary parts soldered onto the carrier. There is no FPGA, wireless module, battery charger or mobile pairing subsystem because none is required by this slow tethered experiment.

Budget approximately USD 60–130 for electronics, a small PCB order, fixture materials and a basic enclosure, excluding calibration equipment and labor. This is a planning allowance, not a quotation or promised retail price. An assembled low-volume kit would also need assembly, inspection, instructions and warranty costs; no retail release is planned. A workshop that already owns an angle gauge and a Nucleo may have lower incremental costs.

## Architecture and pin contract

    hand tilt screw -> vertical support shoe -> inclined coupon bed
                              |
                         height rheostat
                              | J3 HEIGHT
                       1k series / 100k pullup -> PA0 ADC1_IN0

    captive magnet puck -> HOME reed J4 -> PA1 ADC1_IN1
                        -> EXIT reed J5 -> PA4 ADC1_IN4
    operator ARM button ----------- J6 -> PB0 ADC1_IN8

    USB -> NUCLEO-F401RE -> 50 Hz acquisition -> zero / ramp / slide engine
                         -> CRC snapshots -> USART2 PA2 / ST-LINK VCP
                         -> Linux live, geometry and evidence screens

Every J3–J6 sensor connector has signal on pin 1 and ground on pin 2. J1 pin 1 connects to Nucleo CN6 pin 4, 3.3 V; J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1–4 connect in order to CN8 pins 1–4, the Arduino A0–A3 inputs PA0, PA1, PA4 and PB0. UART transmission uses PA2 alternate function AF7. Check virtual-COM solder bridges for the actual Nucleo revision; the existing platform expects the usual ST-LINK USART2 routing. No connection to the host's RX signal is needed.

Each ADC input is pulled to 3.3 V through 100 kohms. A 1 kohm series resistor connects it to its external sensor. The height potentiometer is used as a rheostat to ground, with the unused end tied to its wiper. Identify the terminals using an ohmmeter and orient rising height to increasing resistance. The nominal inverse-divider conversion is R = 100000 × ADC / (4095 − ADC) − 1000 ohms, clamped at zero. Codes outside 80–2300 invalidate acquisition. That catches typical shorts and open leads but not every plausible-value sensor failure.

The contacts use the same network: at most 200 ADC counts means closed and at least 3800 means open. Intermediate values interrupt qualification. Use dry contacts only, not powered proximity-sensor outputs. Nominal total passive-network current is below 0.14 mA even with every input shorted. Reserve a 5 V, 500 mA USB supply allowance for the complete Nucleo assembly; actual current was not measured. There is no sleep or battery-runtime claim.

## Mechanical fixture and measurement geometry

Build a rigid insulating base approximately 300 by 180 mm and a lightweight, smooth, hinged coupon bed approximately 250 by 100 mm. Use a straight horizontal hinge with little play. Locate a vertical screw jack at a fixed horizontal distance of 100 mm from the hinge axis. Its rounded, low-friction support shoe bears against the underside of the bed and can slide along it as the angle changes. The horizontal position of that support is fixed; it is not a point fixed at a constant radius along the rotating bed.

Couple the height potentiometer vertically to the support shoe with an articulated link. The screw and separate guide carry the bed load; the potentiometer must not be a structural guide or travel stop. Allow roughly 2 mm initial sensor stroke at the flat reference, and mechanically limit further rise to 38 mm to retain margin at both ends. The nominal software limit of 88000 ohms corresponds to approximately 39.6 mm with a nominal 100 kohm/45 mm slope; mechanical stops must be tighter than the software ceiling. Resistance tolerance makes the nominal conversion unsuitable as an actual travel limit.

For the ideal geometry, angle is atan(h / d), where h is support rise relative to the level reference and d is the fixed horizontal support distance. The app defaults to 2222.222 ohms/mm and 100 mm but allows measured positive values. Measure the assembled resistance-to-rise slope with independent dimensional references, and check linearity at several positions. The datasheet's resistance tolerance means a nominal slope can be substantially wrong. Hinge height, bed flex, shoe motion and base leveling contribute additional errors. This revision offers a single slope, not a polynomial calibration or uncertainty propagation system.

Use a polymer puck of no more than 100 grams with a captive small magnet securely enclosed in its body and a replaceable underside coupon. Locate both reed switches beneath or alongside the nonmagnetic bed, fixed to the bed itself. Adjust HOME so that the initially seated puck closes it, then opens it after a small, reproducible downhill displacement. Adjust EXIT approximately 30 mm farther downhill so it closes after further travel. These are field gates, not dimensionally exact planes: verify actual activation positions in both directions and prevent overlap of the magnetic fields.

The height sensor acts on the tilt support, not the sliding puck, so its significant operating force does not directly drag the coupon. Reed magnetic interaction can nevertheless perturb a very light puck, and ferromagnetic specimens or fixtures can strongly distort the experiment. Compare runs with the magnet and reeds moved or replaced by a camera reference. Reject the approach if magnetic forces are comparable with the effects being studied. A soft catch at the lower end, side walls and a transparent guard keep the puck contained. Keep magnets inaccessible to children and away from sensitive implanted devices.

The reported bracket surrounds departure from the HOME field, not the first microscopic onset of sliding. Creep before field departure, reed hysteresis, firmware sampling and ADC averaging all shift its meaning. The downstream interval is gate-to-gate time under a still manually changing incline, not a material dynamic-friction coefficient or an acceleration measurement. Do not apply tan(angle) and label the result a certified static coefficient of friction.

## Firmware behavior and design decisions

The firmware is real freestanding C11 with startup vectors, data/BSS initialization, an F401RE linker script, direct register definitions, analog GPIO setup, bounded clock initialization, ADC acquisition, nonblocking UART transmission and an independent watchdog. It uses the internal HSI oscillator and a 1 ms SysTick. ADC1 samples channels 0, 1, 4 and 8 sequentially using the longest sampling interval, discarding one conversion after switching channels and averaging eight more. Acquisition nominally occurs every 20 ms. Channels are not simultaneous; fast events remain unresolved.

The states are WAIT=0, ZERO=1, READY=2, RAMP=3, SLIDE=4, DONE=5 and INVALID=6. A held button at startup does not automatically establish a zero. Begin with the puck closing HOME and EXIT open. Press and hold ARM; fifty stable height samples, within a 200 ohm peak-to-peak window, establish the baseline. Raw ARM release or loss of the initial gate pattern cancels zeroing. Motion restarts the zero window. Release ARM, then press again while height remains within 200 ohms of zero to begin a run. Release the button before raising the bed.

During RAMP, raise the bed slowly and monotonically. A height change larger than 800 ohms per sample, retreat more than 200 ohms from the observed maximum, out-of-range height, early EXIT activity, a fresh ARM press or total trial duration exceeding 120 seconds invalidates the attempt. Contact channels qualify independently after three consecutive endpoint observations. Unrelated chatter cannot postpone another contact's transition. An intermediate ADC value breaks the streak, so it does not count as evidence of continued stability.

On the first candidate HOME opening, firmware retains the preceding closed-home height and the first candidate-open height. If opening qualifies, those endpoints become the immutable release bracket and the timestamp is the first sample of that candidate. A short rejected opening does not end the ramp; a later candidate replaces it. If EXIT is already active before departure qualifies, ordering is invalid rather than guessed. The bracket may span more than one sampling period if intermediate contact voltages intervene.

After qualified departure, the SLIDE phase waits for an independently qualified EXIT closure. Transit is the difference between first-candidate timestamps. Returning to HOME, cancelling, reversing the ramp or failing to reach EXIT within ten seconds invalidates the run. The ten-second check uses current qualification time, so an arrival candidate just before the limit can still be rejected if confirmation falls after it. The ramp remains monitored during transit; the user must not lower it to rescue a stuck puck.

DONE freezes height, elapsed time, release bracket and transit summary. INVALID also retains established evidence. Acquisition and sticky faults continue to report, and a later acquisition or communication fault can turn DONE into INVALID without rewriting the saved measurements. Sampling gaps outside 20–25 ms, converter failure, UART timeout and watchdog initialization failure are sticky invalidations. Export before pressing the Nucleo reset button for another trial. No nonvolatile trial storage exists, and reboot cannot preserve a baseline or completed run.

## Companion application and protocol

Run `python3 app/tilt.py` on Linux with Python 3, Tk and permission for the chosen serial port. The Connection / live screen opens a real serial device, disconnects without erasing evidence, loads a recording and shows state, acquisition validity and faults. Its chart is explicitly retained height-resistance history, not proof that the sensor is currently valid. The Geometry screen accepts a measured slope and support distance. The Evidence screen displays raw snapshots, records operator notes and exports a new private CSV. There are no network requests, credentials or hidden simulated readings.

Every 200 ms the firmware emits a complete ASCII snapshot beginning TB1. The sixteen decimal fields are sequence, device milliseconds, state, faults, acquisition-valid, ADC0, ADC1, ADC2, ADC3, baseline ohms, height delta ohms, elapsed milliseconds, release-present, release-low ohms, release-high ohms and transit milliseconds. Four uppercase hexadecimal CRC digits and LF follow the final comma. CRC-16/CCITT-FALSE covers bytes before that final comma, polynomial 0x1021, initial value 0xFFFF. TB1 identifies this fixed policy; changing thresholds or field meanings requires a new version.

Fault bits are range=1, sampling gap=2, ADC=4, UART=8, watchdog=16, gate ordering=32, reversal=64, excessive step=128, timeout=256 and cancellation=512. Sequence and milliseconds wrap modulo 2^32. The parser checks CRC, field ranges, bracket order, state/fault consistency, duplicate identity, sequence/time direction, fixed baselines, retained release evidence and terminal-summary immutability. These checks reject many impossible records but are not an authentication protocol against malicious producers.

Identical duplicates do not renew freshness. More than two seconds without progress, or excessive cumulative divergence between device time and host monotonic time, blocks live capture and requires reconnecting. Reconnect flushes input and starts a new evidence session only after the port opens successfully. A reboot with plausible forward counters can remain indistinguishable because there is no persistent boot identifier. Full periodic frames allow late attachment but cannot establish historical continuity before attachment.

Disconnect before selecting Connect again; a second connection while a port is already owned is refused to preserve serial configuration. Initial partial-frame suffixes from late attachment are counted and discarded until a complete protocol marker is found. Replay remains strict and never silently discards those fragments.

Replay accepts only regular files no larger than two MiB, requires complete newline-terminated frames and labels the source as replay. The C test fixture is synthetic evidence, not a physical measurement. The model retains at most 2000 snapshots and shows the latest hundred in the table. CSV export refuses overwrite, uses mode 0600, and includes author, provenance, connection state, age, errors, warnings, geometry and notes even for an empty session. Save complete serial logs separately if a longer history is needed. Raw evidence remains unchanged when geometry values are edited.

## Verification, assembly and limits

Run `make -C firmware all test` to link the actual Cortex-M4 image and run host tests with address and undefined-behavior sanitizers. Run `xvfb-run -a python3 tests/test_app.py` to exercise the actual encoder fixture, malformed input, replay restrictions, export, Tk navigation and a live pseudoterminal connection/disconnection. Run `/usr/bin/python3 tools/check_hardware.py` for KiCad ERC, PCB DRC and exact schematic-netlist/PCB-pad/manifest comparison. Regenerate native KiCad artifacts with `/usr/bin/python3 tools/generate_hardware.py`. Build outputs and local KiCad preferences are ignored.

Flash only after assembly review using `make -C firmware flash`, which invokes OpenOCD for ST-LINK and STM32F4. Before connecting USB, inspect resistor values, header numbering, solder bridges and shorts. Confirm the regulated 3.3 V rail, test height input with known fixed resistors, then operate each reed and ARM independently. Deliberately disconnect the height sensor and verify invalidation. Confirm sample timing and UART behavior on real hardware. Only then attach the guarded mechanical fixture and begin geometry calibration.

The carrier has no isolation, industrial transient suppression, reverse-polarity protection or hazardous-voltage rating. USB ground is shared with the computer. The series resistor is not an overvoltage barrier. Use short leads, covered terminals and passive disconnected sensors only; never attach mains, PLC outputs or charged external circuits. Mount the board in an insulating edge-support tray because no mounting holes are supplied. The delivered mechanical dimensions are a prototype construction specification, not a machining drawing or a proven enclosure.

Physical validation must include height linearity, fixture flex, repeatable home placement, reed field geometry, false arrival tests, magnetic interaction, slow creep, stuck coupons and repeat runs over realistic temperature and handling variation. Materials above the fixture's modest angle range will simply fail to depart; do not overtravel the sensor. This is not a safety assessment for walking surfaces, brake materials, restraints, medical devices or transport packaging certification. See VERIFICATION.md for the checks actually executed and the remaining work.
