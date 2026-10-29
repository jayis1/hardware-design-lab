# Ratchet Audit — stroke-resolved rollback evidence for manual mechanisms

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application and locally authored design sources are MIT licensed; see LICENSE.

Revision A is a build-tested engineering prototype. Its custom PCB is a routed passive input carrier for a separately purchased NUCLEO-F401RE. The MCU, regulator, decoupling, USB bridge and debugger are on that purchased board, not on this carrier. No physical assembly, target programming, mechanical calibration, electrical measurement or ratchet experiment has been performed. This is not a safety interlock, lock verifier, torque tester or certification instrument.

## Purpose and concrete user

Ratchet Audit helps a museum automaton builder or classroom mechanism designer investigate a slow hand-operated ratchet. A mechanism can apparently advance on each handle stroke yet lose some of that advance on the return. Watching the handle does not show the amount of shaft rollback. Watching the pawl alone does not show whether the shaft moves backward while the pawl-position indication remains asserted. The proposed instrument observes both and retains their relationship within a deliberately bounded handle stroke.

A concrete use is comparing two printed pawl profiles on a low-energy tabletop demonstrator. The builder couples a low-cost incremental encoder to the ratchet shaft, mounts one magnetic contact at the handle's home position, and another near the pawl's seated position. Each departure from home begins a trial after an explicit stable baseline. Returning home ends it. The instrument reports positive encoder transitions, reverse transitions, reverse transitions observed with the pawl contact closed, maximum rollback from the highest observed shaft position, and elapsed stroke time. It does not infer a tooth count or a load from those observations.

The action is manual investigation, not automatic shutdown. If reverse-with-pawl evidence appears, stop the experiment, inspect magnet alignment and coupling slip, then inspect pawl flexure and tooth engagement under independent video. If reverse counts occur only with the pawl indication open, compare normal disengagement and return geometry before declaring a defect. A zero discrepancy is not evidence that a mechanism is safe. Sub-resolution movement, misaligned sensors and missed encoder states can all conceal behavior.

A phone camera can help validate the fixture but does not directly record the electrical sequence and may hide the pawl behind the frame. A generic encoder counter can report net travel but collapses forward and reverse activity. A logic analyzer can reconstruct everything with sufficient analysis; Ratchet Audit packages that analysis into a small receive-only workbench with an explicit stroke boundary and visible rejected observations. Its advantage is a repeatable workflow, not a claim that basic counting requires new technology.

## Distinction and prior art

The repository's existing device directories and README purposes were inventoried before this concept was selected. Takeup Atlas is the nearest encoder instrument: it compares two shafts during reversals and waits for three output edges to confirm following. Ratchet Audit has one shaft, a separate handle boundary and a pawl-position indication. It measures rollback within a handle stroke, not reversal take-up between shafts. Cam Dwell Map observes a follower versus angular bins over a whole turn; it does not segment manual strokes or retain peak-to-current rollback. ContactAtlas maps electrical selector connectivity, while Detent Loop records force versus travel. None of those documented measurement contracts is being renamed here.

Pawl sensing is established prior art. US20080303260A1 describes a pawl and ratchet assembly adapted for indirect sensing of proper operation, including sensing an amplification arm. Its safety-restraint application is explicitly outside this project's permitted use. Handheld encoder testers are also established products. This design's proposed differentiator is the conjunction of stroke segmentation, signed movement accounting, conservative pawl-qualified reverse counts, and local evidence export for inexpensive workshop mechanisms. A bounded search is not a patent search; worldwide novelty, patentability and freedom to operate have not been established.

Why build this now rather than invent another sensor node? Small workshops can fabricate many ratchet profiles cheaply, but often compare them by remembered feel. Commodity contact encoders and a USB MCU make a narrow, transparent experiment inexpensive enough to repeat. This is useful only if mounting does not materially change the mechanism. The first physical value test must compare operation with and without the encoder attached. If its drag masks the original rollback, this implementation is unsuitable and needs a lower-torque sensing arrangement.

## Hardware specification and block diagram

The target is the STM32F401RET6 on NUCLEO-F401RE MB1136: Cortex-M4, 512 KiB flash, 96 KiB SRAM, reset-clock HSI at 16 MHz. A 1 kHz SysTick schedules acquisition. USART2 TX on PA2, alternate function 7, connects to the development board's ST-LINK virtual COM interface at 115200 baud, eight data bits, no parity, one stop bit. Firmware does not enable UART reception. There is no radio, cloud service, FPGA, battery charger, motor driver or actuator.

The proposed encoder is Bourns PEC11R-1015F-N0024: vertical terminals, no detents, 15 mm flatted shaft, no push switch, 24 pulses per revolution. Four quadrature transitions per pulse nominally give 96 edges per revolution. The companion deliberately displays edges rather than an unverified angle conversion. The manufacturer's PEC11R datasheet specifies contact quadrature, a 10 mA at 5 VDC contact rating and up to 2 ms bounce at its stated 15 RPM test condition. Those limits do not validate arbitrary wiring or the prototype's contact current. Use a manufactured breakout or suitable soldering process consistent with the datasheet; do not stress terminals or glass reed envelopes during assembly.

HOME and PAWL use two Littelfuse MDSR-7-10-15 normally open reed switches with independently positioned small magnets. Their contacts are external to the carrier. The MDSR-7 datasheet describes microcontroller logic-level switching, but magnet geometry and assembled hysteresis remain fixture-dependent. Encapsulate the reeds in nonconductive mounts, strain-relieve their leads, and position the magnets so they do not attract or load ferrous mechanism parts appreciably. The pawl magnet should follow the pawl itself, not merely its driving handle.

```text
Manual handle ----- HOME magnet / reed -------- J5 --- SENSE2 / PA4
Ratchet shaft ----- contact encoder A --------- J3 --- SENSE0 / PA0
                  + contact encoder B --------- J4 --- SENSE1 / PA1
Pawl position ----- PAWL magnet / reed -------- J6 --- SENSE3 / PB0
                                |                         |
                       common ground             10k pullups / 1k series
                                                          |
USB host <-- ST-LINK VCP <-- USART2 TX <-- NUCLEO-F401RE
                                                          |
                                         qualification + stroke engine
                                                          |
                                           RA1 CRC-protected snapshots
                                                          |
                                        Linux Tk workbench / JSON export
```

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4. It uses 0.3 mm tracks, a 0.2 mm default clearance, axial resistors and 2.54 mm through-hole headers. There are no mounting holes; use insulating edge supports in a covered enclosure. A 160 by 110 by 45 mm enclosure is a planning allowance for carrier plus Nucleo, not supplied mechanical CAD. Brackets and couplings must fit the specific demonstrator; the encoder bearings must not carry the ratchet's radial or axial working load.

## Exact wiring, power and BOM

J1 pin 1 receives only Nucleo CN6 pin 4, the regulated 3.3 V output. J1 pin 2 connects to CN6 pin 6 ground. J2 pins 1 through 4 connect respectively to Nucleo CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. Although those connector labels suggest analog inputs, all four are digital inputs in this firmware. Preserve the development board's documented ST-LINK virtual COM solder-bridge configuration and verify it against the actual purchased revision before modifying bridges.

| Connector | Pin 1 signal | Pin 2 | Pullup | Series | MCU-side net |
|---|---|---|---|---|---|
| J3 | ENC_A | GND | R1 10k | R2 1k | SENSE0 |
| J4 | ENC_B | GND | R3 10k | R4 1k | SENSE1 |
| J5 | HOME | GND | R5 10k | R6 1k | SENSE2 |
| J6 | PAWL | GND | R7 10k | R8 1k | SENSE3 |

Encoder common C connects to ground; encoder A and B connect to their signal terminals. Each reed simply closes its signal to ground. Do not add powered sensor outputs, external pullups, a 5 V supply, industrial logic, vehicle wiring or mains connections. This carrier has no galvanic isolation, TVS protection or qualified hot-plug behavior. The series resistors limit ordinary contact current; they do not make an unsafe external voltage acceptable. USB ties the assembly to host ground.

At nominal 3.3 V, a closed contact conducts 0.30 mA through 11 kilohms. Four closures total 1.20 mA. The input-side divider sits at approximately 0.30 V with an ideal closed contact. Verify actual logic margins, leakage, rise time and wetting reliability on assembled hardware. Supply the Nucleo through its supported USB connection; do not feed J1 from another supply simultaneously. Reserve 100 mA at USB 5 V as an unmeasured system planning allowance, not a guaranteed maximum under every debugger configuration. Continuous USB acquisition is intentional, so there is no sleep or battery-life claim.

Quantity-one planning allowances in USD are: Nucleo 25; carrier 8; encoder and breakout 6; two reeds and magnets 6; eight resistors and six headers 4; cable, enclosure and couplings 20. The estimated total is 69, excluding labor, shipping, tax and fixture fabrication. These are budgeting assumptions, not supplier quotations. A borrowed logic analyzer may cost less and provide better resolution, while a commercial encoder tester may be more robust. This prototype is justified by the stroke/pawl notebook, not by a blanket lowest-price assertion.

## Firmware measurement contract

Firmware starts in WAIT. Encoder A/B are qualified together using three consecutive samples, while HOME and PAWL each require ten consecutive samples independently. This avoids unrelated contact chatter delaying a handle edge. A stable 100-sample baseline requires both position contacts closed, stationary accepted encoder code, and agreement between raw and accepted input bits. Once READY, a qualified HOME opening starts ACTIVE. Encoder movement or a qualified PAWL opening before departure returns READY to WAIT. Simultaneous qualified HOME and PAWL openings can start a trial from the prior READY state; that trial is not a claim of maintained engagement.

Positive rotation is the closed-contact bit sequence 0, 1, 3, 2, 0. Reverse that sequence for negative rotation. Align the wiring so intended ratchet advance is positive; swap A/B only with power removed. Both contacts are active low electrically and converted to closed=one before decoding. A two-bit Gray-code jump is invalid rather than guessed. At boot and after a sampling gap the first qualified encoder state establishes a new anchor without inventing an edge from an unknown previous state.

At departure, forward, reverse, seated_reverse, rollback, position, peak and duration clear for the new trial. Each forward edge increments forward and signed position. Each reverse edge increments reverse and decrements position. Peak is the largest position reached, starting from zero. Rollback is the largest peak-minus-position value seen during the trial, not the sum of all reverse activity. A trace that advances four edges, retreats two, advances two and retreats two has four total reverse edges but a maximum rollback of two. Raw counts remain the primary evidence; neither value automatically equals a tooth skip.

A reverse edge contributes to seated_reverse only if PAWL was closed in the previous qualified vector, remains closed in the incoming qualified vector, and is also closed in the current raw sample. This conservative test excludes an edge coincident with a raw pawl opening, even before the ten-sample filter accepts that opening. It cannot prove actual tooth engagement, force transmission or within-sample ordering. Sensor hysteresis, flexure and magnet motion are potential explanations for every discrepancy.

A qualified HOME closing completes an active trial. An encoder step accepted in that same sample is included before completion. Duration is counted in accepted one-millisecond intervals after departure. Reaching 30000 ms aborts before accepting completion on that deadline. A skipped sample or illegal encoder transition aborts the current trial, preserves its partial evidence, sets a sticky diagnostic bit and returns to WAIT. A new stable baseline may then re-arm; the earlier diagnostic remains visible. Results survive idle time but clear at the next start, so export before replacing an interesting observation.

Starts always equals completed plus aborted plus one if ACTIVE. Counter exhaustion halts instead of wrapping and abandons an active trial once. Total forward plus reverse is bounded to one million edges per trial, keeping signed arithmetic bounded. Configuration or watchdog-start failure also enters HALTED. A transmit timeout is separately counted and flagged; it does not pretend the acquisition failed. Main-loop serial output is nonblocking, and the independent watchdog is fed only following acquisition. Clock restoration uses bounded pre-SysTick polling, and UART transmission has a 100 ms timeout.

Operate slowly: begin below 15 shaft RPM and require every stable electrical encoder code to last at least 10 ms. Average speed alone cannot establish that condition, especially around bounce. A skipped full quadrature cycle can look stationary, and an aliased sequence can look valid. The invalid-transition diagnostic is therefore not an overspeed detector. Operate indoors, dry, around 10–35 degrees Celsius; this is an intended use range, not an environmental qualification. Never attach to powered machinery, lifting gear, restraint systems, brakes, medical equipment or stored-energy mechanisms.

## Protocol, screens and evidence handling

Every approximately 100 ms the target sends a complete ASCII snapshot starting RA1, followed by fourteen comma-separated unsigned decimal fields in this exact order:

`now,phase,mask,starts,completed,aborted,duration,forward,reverse,seated_reverse,rollback,faults,gaps,tx_errors`

The suffix is an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xffff, without reflection or final XOR, over the marker and fields before the asterisk. Phase values are WAIT=0, READY=1, ACTIVE=2 and HALTED=3. Mask bits are A=1, B=2, HOME=4 and PAWL=8. Fault bits are configuration=1, watchdog initialization=2, quadrature=4, timeout=8, sampling gap=16, counter exhaustion=32 and transport timeout=64. RA1 fixes these qualification and deadline policies; incompatible changes require another marker.

The Python standard-library application is appropriate for a local USB bench instrument. Connection / replay opens a manually specified Linux serial device or reads a saved RA1 capture. Live evidence displays the accepted fields without hiding faults. History / export retains up to 2000 advancing snapshots and writes a private, exclusively created JSON file containing measurements and provenance. Export does not overwrite an existing experiment. No package installation, network account, telemetry backend or embedded credential is required. Install the operating system's Tk package if Python lacks Tk support.

Serial permission failures are reported rather than bypassed. Disconnect retains evidence and its live/replay origin; reconnect deliberately starts a new session, so export first. The app restores terminal settings on ordinary close and setup failure, detects hangup, bounds receive framing, and resynchronizes on a complete RA1 marker after truncated output. CRC-valid but impossible count relationships are rejected. Counter rollback, disappearing sticky faults, contradictory duplicates and backward modular time block the session instead of silently relabeling a reset as continuous measurement.

Freshness requires a serial connection and at least two advancing timestamps, with less than one second since progress. Duplicate timestamps do not renew freshness. A reception pause beyond two seconds blocks resumed data; cumulative host/device timing is also compared with a two-second plus five-percent allowance to expose gradual backlog or clock discontinuity. These checks are heuristics, not authentication. Without a persistent boot identifier some resets remain indistinguishable. A hostile serial source could fabricate valid CRCs; attach only a trusted local board.

Replay is explicitly not live. It requires a regular file no larger than two MiB, rejects FIFOs and devices, rejects invalid frames, requires at least one snapshot, and rejects a partial EOF tail. JSON exports retain author, source, connection status, freshness, age, parser-error count, blocked reason and discarded-history count even if no trial completed. This is a snapshot stream, not an event queue: several short trials can complete between frames, or old rows can leave the history buffer. Cumulative totals may expose that loss but cannot reconstruct the missing individual trials. Slow down and export frequently when individual strokes matter.

## Assembly, calibration and physical acceptance plan

Inspect the unpowered PCB for shorts, resistor values and connector orientation. Compare the native schematic and PCB against connectivity.json. Connect the Nucleo ground, regulated rail and four GPIO leads with USB removed. Cover exposed contacts and provide cable strain relief. Put the encoder on an independently supported coupling, not directly under a working bearing load. Secure the reed bodies without bending their glass-to-lead seals. Keep wires short and away from switching power equipment.

Before a mechanism trial, manually exercise the contacts and confirm each mask bit independently. Check one marked encoder revolution in both directions; an ordinary complete revolution should traverse the expected number of edges, subject to phase at the marked endpoints. Observe bounce and minimum stable code duration on an oscilloscope or logic analyzer. Check both contact levels at the MCU pins. Compare observer elapsed time against an independent timer. Verify the actual USB/current budget and deliberately provoke a watchdog recovery only on an unloaded bench fixture.

Align HOME to a repeatable handle position and PAWL to a visible pawl-position band. Record magnet orientation and the separate opening/closing hysteresis positions. Run a transparent reference ratchet under video, including a deliberately held-open pawl, a reverse twitch, an incomplete stroke, a slow return and a timeout. Repeat with the encoder disconnected mechanically to quantify loading. Do not substitute an invented calibration coefficient for this work. The initial acceptance goal is agreement with independently counted, slow encoder transitions and correct rejection of every intentionally interrupted acquisition.

For a profile comparison, keep magnet mounts, coupling, load and hand-motion range fixed. Alternate the old and new profiles rather than testing all of one first. Retain rejected attempts and blank-fixture observations. Compare distributions of rollback and seated_reverse, not only a favorable minimum. At least ten repeatable strokes per condition are a reasonable exploratory plan, not a statistical certification threshold. A difference smaller than one encoder edge, or a result that changes when sensor mounting changes, needs better instrumentation before drawing conclusions.

## Reproduction, sources and limits

From this directory run `make -C firmware all test`. This links the actual Cortex-M4 ELF and binary and runs host tests under AddressSanitizer and UndefinedBehaviorSanitizer. Run `xvfb-run -a /usr/bin/python3 tests/test_app.py -v` for parser, export, replay and actual Tk event-loop tests with a simulated serial PTY. Start the application with `/usr/bin/python3 app/audit.py`; validate an existing recording with `--check path`. No physical serial device is required for the automated tests, and none is implied by their success.

Run `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9 Python bindings to regenerate CAD. Run `/usr/bin/python3 tools/check_hardware.py` to execute real ERC and DRC and compare fresh schematic nets with actual PCB pads, references, values and footprint identifiers. The local symbol library is included; standard installed KiCad footprints retain upstream licensing. No external MCU is falsely represented as a component on the passive board. Passive-only ERC cannot validate the external board's supply, jumper configuration or sensor fixture.

Manufacturer references are Bourns [PEC11R datasheet](https://www.bourns.com/docs/product-datasheets/pec11r.pdf), Littelfuse [MDSR-7 series](https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7) and its BO.12/08/20 datasheet, ST [RM0368](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), and ST [UM1724](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf). Prior art includes [US20080303260A1](https://patents.google.com/patent/US20080303260A1/en). Review the actual purchased revisions before assembly.

The clock, board, UART, startup and passive-carrier generation platform reuse existing jayis1-authored repository work. The ratchet engine, RA1 contract, workbench and device-specific tests define this design's behavior. See VERIFICATION.md for executed checks and measured source counts. Fabrication, target programming, timing accuracy, electrical robustness, sensor loading, mechanical usefulness and regulatory compliance remain unverified physical work rather than claimed accomplishments.
