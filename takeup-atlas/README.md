# Takeup Atlas — reversal-to-following notebook for hand-operated mechanisms

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources: MIT.

Revision A is an engineering prototype. The custom board is a fully routed passive encoder interface for a separate NUCLEO-F401RE, not a standalone microcontroller board. Target linking, host tests, application workflows and CAD checks are reproducible. Fabrication, programming, electrical timing, mechanical mounting, measurement accuracy and field usefulness have not been physically verified.

## Purpose, user and job to be done

Takeup Atlas helps a maker rebuilding a hand-cranked automaton, educational gearbox, wooden mechanism or coarse positioning fixture answer a specific question: after the handle reverses, how much input movement occurs before the output begins following consistently? A loose coupling can feel different after a repair, but memory of that feel is poor. A dial indicator can show a single displacement, while a phone video requires manual frame inspection and often misses the exact moment of reversal. This instrument keeps the input movement, first observed output response, confirmation movement, direction and rejected attempts together in a local notebook.

The hardware is essential because the input and output shafts must be observed independently. Two incremental rotary encoders are attached through lightly loaded couplings. The observer records their phase transitions without controlling either shaft. A desktop application separates completed observations from incomplete or interrupted reversals. The person can compare before-and-after repairs under the same mounting and loading conditions, or teach why a mechanism's response depends on direction, compliance and friction. The device deliberately does not report a single authoritative backlash value: first motion and confirmed following are different observations.

The useful original combination is a portable, receive-only reversal notebook with explicit priming, first-response capture, three-edge output confirmation and visible rejection accounting. It does not require a known transmission ratio and never subtracts output angle from input angle. That makes it useful for coarse irregular mechanisms where a continuous gear-error calculation would require more mechanical knowledge than the operator possesses. Its useful range is slow, visible lost motion, not precision gear metrology.

## Repository distinction and prior art

The repository directories and their README concepts were inventoried before implementation. The nearest electrical relative is WebSlack Ledger, whose two encoders continuously compare strip travel on opposite sides of a guide. Takeup Atlas reuses that project's jayis1-authored acquisition platform and passive interface, but implements a different measurement contract. There is no web, roller circumference calibration, accumulation estimate or differential-travel result here. A reversal opens a bounded trial; a sequence of output edges either confirms that trial or rejects it. ContactAtlas maps disconnected switch connectivity, Crossover Track compares timed cart passages, and Swing Envelope observes pendulum thresholds. None provides this reversal-response workflow.

Two-encoder gear measurement is established prior art, not a new sensing principle. The paper [High Precision Measurement of Transmission Errors of Gears under Load](https://www.jstage.jst.go.jp/article/kikaic1979/64/623/64_623_2718/_article) describes a gear-testing machine using two rotary encoders. [Measurement and analysis of backlash](https://beta.iopscience.iop.org/article/10.1088/1757-899X/542/1/012005/pdf) discusses lost motion, clearance, elastic deformation and the importance of loading conditions. A conventional dial-indicator reversal check is another close alternative. This project differs through its coarse hand-operated, ratio-independent observation workflow and explicit incomplete-trial evidence, not a claim to have invented backlash testing. Global novelty, patentability and absence of comparable products have not been established.

## System architecture and hardware specification

The processor is the STM32F401RET6 on a purchased NUCLEO-F401RE. Firmware uses the reset-default 16 MHz internal oscillator, Cortex-M4 instructions, 512 KiB flash and 96 KiB SRAM. A 1 kHz SysTick provides observation time. USB connectivity is the Nucleo ST-LINK virtual serial port, carrying target USART2 transmission at 115200 baud, eight data bits, no parity and one stop bit. There is no Bluetooth, Wi-Fi, cloud account, FPGA, actuator or custom battery charger.

The specified sensors are two Omron E6B2-CWZ6C 100P/R 0.5M NPN open-collector incremental encoders. Each provides A, B and Z; only A and B are used. Four decoded edges per pulse yield 400 counts per revolution and a nominal 0.9 degree input increment. Higher-resolution substitutions need a fresh speed and electrical review. The application can change the display scale, but this does not increase the firmware sampling rate or make the instrument accurate at higher speeds.

```text
Hand crank -> input coupling -> mechanism -> output coupling
                    |                            |
              encoder INPUT                encoder OUTPUT
                  A / B                        A / B
                    +------------+---------------+
                                 |
                80 x 70 mm passive interface
                  four 10k pullups to 3.3 V
                  four 1k series resistors
                                 |
                   NUCLEO-F401RE GPIO sampling
                     reversal qualification
                                 |
                    USART2 -> ST-LINK USB
                                 |
                  Linux Tk application and CSV
```

The custom two-layer FR4 PCB is 80 by 70 mm and 1.6 mm thick. It uses through-hole resistors, 2.54 mm headers, 0.3 mm tracks and a 0.2 mm clearance rule. Mount it in an insulating enclosure using edge supports; this revision has no mounting holes. A roughly 160 by 110 by 45 mm enclosure is a packaging allowance for the separate Nucleo and carrier, not a supplied mechanical CAD model. Provide cable strain relief, access to reset and USB, and covered terminal connections. Encoder brackets and couplings must match the mechanism, and should not use encoder bearings to support external gears or belt loads.

## Electrical boundaries and exact connector map

J1 pin 1 is VCC from Nucleo CN6 pin 4, the regulated 3.3 V rail. J1 pin 2 is GND from CN6 pin 6. J2 pins 1 through 4 connect respectively to Nucleo CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. These are digital inputs in this firmware despite the Arduino analog labels. PA2, configured as AF7, carries USART2 TX to ST-LINK. Keep the manufacturer's virtual COM solder-bridge configuration: SB13 and SB14 on, SB62 and SB63 off, and consult the actual board revision before changing anything.

| Carrier connector | Pin 1 | Pin 2 | Series resistor | Pullup and MCU net |
|---|---|---|---|---|
| J3 | INPUT_A | GND | R2, 1k | R1, 10k to VCC; SENSE0 / PA0 |
| J4 | INPUT_B | GND | R4, 1k | R3, 10k to VCC; SENSE1 / PA1 |
| J5 | OUTPUT_A | GND | R6, 1k | R5, 10k to VCC; SENSE2 / PA4 |
| J6 | OUTPUT_B | GND | R8, 1k | R7, 10k to VCC; SENSE3 / PB0 |

The encoder brown wire receives a separate regulated 5 V supply; blue is common ground, black is A and white is B. Insulate the unused orange Z wire. Never connect brown to a signal connector. The carrier does not distribute encoder power. Supply both sensors from a current-limited external source and connect its ground to carrier ground. Do not tie the external 5 V positive rail to the Nucleo USB 5 V rail. There is no galvanic isolation, so this is a common-ground bench instrument, not a field interface.

Only the NPN open-collector CWZ6C variant is approved for this wiring. PNP, voltage-output and line-driver variants are not interchangeable. With a nominal zero-volt collector, the pullup and series resistance draw 0.3 mA per asserted phase. At a conservative 0.4 V collector residual, the MCU input calculates to approximately 0.664 V. This is an engineering estimate; confirm actual input margins, rise time and cable behavior on the assembled board. Internal GPIO pullups are disabled. No capacitor deliberately filters the quadrature phases because phase-dependent delays could alter their ordering.

The board lacks TVS suppression, isolation and hot-plug qualification. Keep signal cables short, indoors and away from switching loads. The sensor's published output rise time at a different load does not certify this 10k interface. A disconnected collector reads high, indistinguishable from a legitimate phase state. Sensor power loss or a broken cable can therefore create missing motion without an explicit wiring fault. Run the manual motion check before every session.

## Power budget, bill of materials and operating limits

Use USB power for the Nucleo and a separate regulated 5 V encoder supply. The current Omron datasheet specifies up to 80 mA for each CWZ6C, so reserve 160 mA steady-state for the pair. It also describes a brief startup inrush; select a supply that tolerates the specified pulse without repeatedly collapsing, and verify it with the selected sensors. Do not assume a 200 mA hard-limit supply will start reliably. Reserve 150 mA at USB 5 V for the Nucleo as a conservative, unmeasured development-board allowance. The passive pullups add at most approximately 1.2 mA from 3.3 V. These are budgets, not measured consumption. No battery-life claim is made.

Planning allowances, not supplier quotations, are approximately: Nucleo $25 at quantity one or $22 each at quantity 100; two genuine encoders $160 or $130 per pair; carrier PCB $8 or $2; resistors and headers $4 or $1.50; supply and cables $15 or $9; enclosure, brackets and couplings $25 or $15. This intentionally does not claim that the prototype is cheaper than a dial indicator. Labor, tax, shipping, tooling and fixture-specific bearings are excluded. Reusing known encoders and an available development board may make an educational deployment economical.

Operate in a dry indoor workshop, nominally 10–35 degrees Celsius. This is not an environmental rating. Begin with no more than 50 decoded edges per second on either shaft, and keep every electrical phase dwell above 5 ms. Average speed alone cannot guarantee minimum phase dwell. Fast quadrature can alias into apparently valid transitions, so the invalid-transition flag is not an overspeed detector. Verify edge timing with an oscilloscope before expanding the operating envelope. With the specified resolution, this design is unsuitable for arc-minute backlash claims.

## Measurement policy and firmware design

The engine is a pure C state machine separated from memory-mapped drivers. It starts in PRIMING. An input edge establishes direction; a changed input direction clears the priming progress. Three output edges matching that direction, with no intervening opposite output edge, establish TRACKING. Stationary samples do not clear this progress. Initial startup motion is not reported as a completed reversal. It establishes the direction from which a later reversal can be observed.

In TRACKING, the first input edge of the opposite sign opens TAKING_UP. Its travel count is one because the reference lies immediately before that edge. The first subsequent output edge in the new direction captures `first`. The third matching output edge confirms the trial, captures `confirm` and `duration`, increments the completed-event counter, and returns to TRACKING. Same-sample input/output motion is deliberately included: the observer cannot order edges within one millisecond. It never reports zero input travel merely because both shafts moved in the same sample.

An opposite output edge during take-up rejects the trial. Another input reversal also rejects it. A trial reaching 10000 counted milliseconds is rejected before output confirmation on that deadline. The rejecting sample is not reused to prime another trial. A new sequence of following motion is necessary. This prevents hand wobble from being quietly converted into a new, favorable measurement. Pauses are allowed within a trial until the timeout, and output motion can complete confirmation while the input is stationary. No causality claim is made about that motion; the output must not be touched or externally driven during a test.

`first` is input travel at the first observed output response, while `confirm` is input travel by the third confirming output edge. The latter includes additional driven motion; it is not an upper confidence bound on pure mechanical clearance. Friction, compliance, encoder loading, coupling movement, resolution and phase alignment all influence both observations. Compare trials only with the same setup and load. The firmware retains only the most recent completed result and cumulative counters; the application can miss individual results if several complete between telemetry frames.

The Gray-code decoder rejects simultaneous two-bit transitions rather than guessing direction. A missed millisecond sample, invalid transition, counter/position saturation or watchdog initialization failure latches FAULT. Active trials are rejected once. Subsequent readings cannot silently restore measurement validity; inspect the setup and reset explicitly. Timestamp subtraction handles normal 32-bit wrap, but a full unobserved timer cycle cannot be reconstructed. Position counters saturate instead of invoking signed overflow.

The board driver configures GPIO, USART and SysTick directly using STM32F401 register addresses. SysTick only increments a volatile millisecond counter. Acquisition, serialization and state transitions run in the main loop. The serial driver drains one byte when TXE is available and abandons a frame after 100 ms, exposing a transport error. Peripheral waits for watchdog setup are bounded by the running tick. There is no command parser, writable configuration, flash logging or remote firmware update. Power loss discards state. Program the ELF or binary through the Nucleo debugger using your normal trusted flashing tool; no target programming was performed during this design run.

## Protocol and companion application

Every approximately 100 ms, the device sends an ASCII line beginning `TKA1`. It contains these comma-separated decimal fields in order:

`sequence,time,input,output,invalid_input,invalid_output,gaps,reversals,events,rejected,first,confirm,duration,direction,state,travel,flags,transport_errors`

The line ends with `*` followed by four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial value 0xffff, no reflection and no final XOR, covering the marker and fields but not the asterisk. Sequence and time are unsigned 32-bit values; positions are signed 32-bit counts; direction is -1, 0 or 1. State codes are 0 priming, 1 tracking, 2 taking up and 3 fault. Flag bits are sampling gap, invalid input transition, invalid output transition and saturation/watchdog-start failure. All completed-result fields remain zero until an event completes.

TKA1 fixes the three-edge confirmation and 10000 ms timeout policy. Changing those semantics requires a new protocol marker, not silently reusing this one. Frames remain below the 256-byte buffer. Without saturation, reversals equal completed plus rejected plus one if currently taking up. The parser checks this accounting, range constraints, result consistency and CRC. Within an attached session it also rejects duplicate/backward timestamps, disappearing counters, disappearing fault bits and changing completed results without new event IDs.

The Python standard-library Tk application is an appropriate desktop companion for a USB bench tool. It has three functional screens. Connect / Replay discovers Linux ACM and USB serial devices, opens a selected port and loads explicitly labeled replay captures. Reversal Trials displays current policy state, cumulative counts, current travel and the latest hundred observed completed events. Calibration / Export changes input edges per turn and writes a new private CSV file. There is no dependency download, backend service or browser permission workflow.

Serial access requires operating-system device permission; permission denial is shown rather than bypassed. Disconnects, parser errors, missing telemetry and stale data remain visible. After one second without device-time progress, live data is stale. Reconnect clears history and starts a new explicit session, so export first. Reset detection is heuristic because there is no persistent boot identifier: a reset whose counters and time happen to appear forward may be indistinguishable. The application never reconnects automatically or claims authenticated device identity. CRC protects against accidental corruption, not malicious data.

Replay is limited to two megabytes, rejects nonregular files such as FIFOs, and stays labeled NOT LIVE. History is capped at 6000 frames. Exports use exclusive creation and mode 0600, retaining raw fields, author, source, connection status, receive age, warnings and angular scale. No data leaves the host unless the user copies an export. Local serial recordings can reveal working habits; retain and share them deliberately. A successfully exported CSV is evidence of received telemetry, not proof of physical correctness.

## Assembly, calibration and acceptance procedure

First inspect the unpowered carrier for shorts and verify every connector against the manifest and exported netlist. Check resistor values and continuity through each 1k path. Mount the Nucleo separately, avoid conductive enclosure contact, and connect ground before signal wiring while all supplies are off. Do not wire the device into an energized machine. Remove motors and stored energy from the test mechanism; use only safely hand-operated fixtures.

Power the encoders first, allow at least 100 ms for their startup behavior, then reset the observer. Rotate each shaft separately and confirm which displayed position changes. Arrange A/B wiring so mechanically corresponding motion has the same sign on both channels. For an inverting gear pair, swap output A and B to normalize the sign. Perform changes only with power removed. Turn each encoder through a marked revolution and check the expected 400 edges; repeat in reverse. Inspect electrical levels and shortest phase dwell on an oscilloscope before trusting counts.

For a trial, drive both shafts steadily until the screen says Following. Reverse only the input slowly. Continue until the output confirms or a rejection is shown. Collect observations in both directions, write down load and coupling conditions, and repeat after the adjustment without changing mounts. Start with an intentionally loose educational coupling whose motion can be seen directly. A rigid reference coupling is also useful, but quantization means a nonzero first-following result can remain even with negligible true clearance.

Bench qualification still needs continuity measurements, supply startup capture, encoder rise-time checks, oscillator timing characterization, watchdog reset injection, cable-disconnect tests, controlled synthetic quadrature injection and repeatability trials against an independent reference. Passing CAD checks does not validate the external Nucleo power source or encoder wiring. Passing host tests does not validate interrupt timing or installed mechanics. Never use this device to certify a machine as safe, release a repaired product, measure a medical mechanism, or justify removing a guard.

## Reproduction and file layout

From this directory, run `make -C firmware all test` with GNU Arm Embedded tools and host GCC. The build links the actual Cortex-M4 image using the included startup vector table and linker script; the tests use AddressSanitizer and UndefinedBehaviorSanitizer. Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` for parser, export, actual Tk event-loop and pseudo-terminal tests. Launch the desktop application with `python3 app/atlas.py` on a graphical Linux session with Tk installed.

The `kicad` directory contains native schematic, PCB, project, local symbol library, library table, connectivity manifest and exported netlist. `tools/generate_hardware.py` regenerates the authored passive interface using KiCad 9's system Python bindings and installed standard footprints. `tools/check_hardware.py` invokes real ERC and DRC, exports a fresh XML netlist, and compares every reference, pin, net, value and footprint against the PCB and manifest. Use `/usr/bin/python3` for these tools where that interpreter provides pcbnew. Generated build products and local KiCad preferences are ignored.

Manufacturer references used for the electrical contract are [ST UM1724, Nucleo-64 MB1136 user manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf), especially the F401RE connector table and USART solder bridges, and [Omron E6B2-C current datasheet](https://www.ia.omron.com/data_pdf/cat/e6b2-c_ds_e_6_3_csm491.pdf), especially CWZ6C current consumption, NPN output wiring, startup precautions and coupling loads. Review the exact purchased revision rather than substituting another output variant.

See VERIFICATION.md for measured file counts, tool versions and checks actually executed. The source includes reused jayis1 platform code rather than presenting drivers as newly invented. Standard KiCad footprints retain their embedded library descriptions; this project's authorship does not replace the authorship or licensing of upstream libraries, vendor hardware or tools. The delivered sources define a testable prototype and a bring-up plan, not a claim that a physical instrument already exists.
