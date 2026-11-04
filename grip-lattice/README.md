# Grip Lattice — three-zone handle-contact combination notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier for a separately purchased NUCLEO-F401RE. Target compilation, synthetic acquisition, desktop application workflows and electrical CAD checks are reproducible; fabrication, target programming, mechanical calibration, physical contact accuracy and usefulness with real participants remain unverified. This is not a grip-force meter, ergonomic assessment instrument or tool-safety interlock.

## Purpose and overview

Grip Lattice is a removable three-paddle sleeve for an unpowered dummy hand tool. A fourth contact, operated by a separate footswitch, declares a recording window. During that window the instrument records how much qualified time belonged to each of the eight possible contact combinations: none, thumb only, fingers only, palm only, each pair, and all three together. It also retains how many observed segments belonged to each combination and the longest uninterrupted segment. Time that cannot be assigned conservatively is shown separately as unknown rather than hidden by renormalizing the chart.

The intended user is a handle or control-layout designer making inexpensive foam, wood or printed mock-ups. Suppose a designer moves a thumb shelf between two versions of a manual applicator handle. A static ink impression shows where contact occurred at least once, but does not distinguish continuous contact from repeated brief touches. A video shows hand posture but often hides the palm-facing surface. Grip Lattice provides a limited, camera-free answer: did the designated paddles stay actuated together, or did the operator repeatedly move between combinations during the declared manipulation? It does not answer whether either handle is comfortable or safe.

A practical experiment uses the same dummy handle, sleeve, paddle adjustment and hand movement with two removable thumb-shelf inserts. The operator holds the footswitch while slowly following a printed motion sequence, releases it at the end, and exports a local snapshot notebook. The designer compares the all-three combination, thumb-without-palm combination and unknown fraction across repeated trials. The interpretation remains contextual: a change in paddle contact could be an intentional grip shift, a mechanical threshold difference, a loose sleeve, or an electrical fault. A larger number is not automatically better.

The device is useful only when this coarse binary observation is sufficient. For actual pressure distribution, force direction, contact area, skin load or musculoskeletal risk, use an appropriate calibrated instrument and qualified protocol. Do not attach the sleeve to a powered drill, saw, vehicle control, medical tool, live electrical appliance or any handle whose operation can cause injury. The entire pilot should use an inert mock-up on a clear bench. No actuator, relay, radio, battery charger, camera or microphone is present.

## Originality and relationship to existing designs

The hardware-design-lab device directories and README concepts were inventoried before selecting this proposal. Chord Window captures the timing of a finite keyboard chord, including press spread and shared hold. Nest Settle observes four locating contacts during placement and removal. QuadraAlign proposes calibrated force and level measurements beneath machine feet. None supplies a footswitch-delimited three-zone dummy-handle combination histogram that retains unknown intervals, repeated segments and longest runs for every subset. Grip Lattice deliberately does not infer a correct order, select a winning combination or treat all contacts closed as a success criterion.

The shared carrier, startup, clock restoration, GPIO acquisition and serial driver are reused from jayis1's Domino Ledger platform at repository commit 4dc253c32e2ac81d3cc8afa4a1fd168e5c54e816. The new work is the grip experiment, interval-accounting engine, GL1 protocol, desktop model and application, tests, mechanical sleeve specification and documentation. Electrical reuse is intentional; a different experimental question does not require inventing a less reliable resistor network.

Hand-handle sensing is established prior art. A bounded search found the review “Utilisation of tactile sensors in ergonomic assessment of hand–handle interface” and the Sensors article “Isolating In-Situ Grip and Push Force Distribution from Hand-Handle Contact Pressure with an Industrial Electric Nutrunner.” Those works describe substantially richer pressure sensing than this device. Grip Lattice's proposed contribution is a low-cost, explicit contact-combination notebook for dummy-handle iteration, not the invention of grip sensing. Worldwide novelty, patentability and absence of equivalent commercial or hobby devices have not been established.

References consulted:

- https://agronomy.emu.ee/vol123/2014_3_26_b5.pdf
- https://www.mdpi.com/1424-8220/21/23/8120
- https://stacks.cdc.gov/view/cdc/197083
- ST UM1724, connector table 16 and USART section: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 reference manual RM0368 for the register map; the addresses actually used are collected in firmware/registers.h.

## Hardware specifications

| Subsystem | Revision A contract |
|---|---|
| Processor | STM32F401RET6, Cortex-M4, on purchased ST NUCLEO-F401RE MB1136 |
| Memory | 512 KiB flash and 96 KiB SRAM on the development board |
| Clock | Internal HSI, nominal 16 MHz, explicitly restored before I/O setup |
| Acquisition | Foreground GPIO sampling at nominal 1 ms ticks |
| Qualification | Five consecutive identical samples per independent input |
| Grip sensing | Three normally-open, voltage-free low-level contacts behind paddles |
| Recording control | One separate normally-open voltage-free footswitch |
| Connectivity | USART2 transmit only, PA2 AF7, ST-LINK USB virtual COM |
| Serial format | 115200 baud, eight data bits, no parity, one stop bit |
| Power | Nucleo USB, carrier supplied by its regulated 3.3 V rail |
| Custom PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Companion | Python 3 and Tk on Linux; standard library, no Python package downloads |
| Outputs | Telemetry only; no control output or permission signal |

The MCU, its decoupling, regulator, oscillator support, debugger, reset circuit and USB bridge are on the external Nucleo. The carrier schematic accurately shows resistors and connectors rather than an unconnected decorative MCU. The carrier is cable-connected, not a mechanically mating Arduino shield. The drawing and routing represent the complete custom electronics but not the separately purchased development board or the mechanical sleeve.

### Wiring and bill of materials

J1 pin 1 connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled header pins are used as digital inputs. J3, J4 and J5 are THUMB, FINGERS and PALM. J6 is RECORD. Every field connector has signal on pin 1 and ground on pin 2. The schematic, board silkscreen values, firmware board.h and GUI setup tab use this same assignment.

| Item | Quantity | Specification |
|---|---:|---|
| R1, R3, R5, R7 | 4 | 10k ohm, 1%, axial DIN0207, 7.62 mm pitch |
| R2, R4, R6, R8 | 4 | 1k ohm, 1%, axial DIN0207, 7.62 mm pitch |
| J1, J3, J4, J5, J6 | 5 | 1x2, 2.54 mm through-hole straight pin header |
| J2 | 1 | 1x4, 2.54 mm through-hole straight pin header |
| Processor board | 1 | NUCLEO-F401RE, external purchased assembly |
| Paddle contacts | 3 | Littelfuse MDSR-7-10-15 normally-open reed, external, with captive magnet |
| Footswitch contact | 1 | Same reed with captive magnet in a mechanically protected pedal housing |
| Harness | 1 set | Insulated two-wire contact leads and six Nucleo interconnect wires |

Confirm the exact contact variant against the [MDSR-7 series documentation](https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7) before assembly. The 10–15 AT sensitivity is a procurement characteristic, not a calibrated operating distance. A large oxidized power microswitch is not a suitable substitution merely because it has continuity on a multimeter. The electrical excitation is small. An ideal closed contact draws 0.30 mA through the 10k-plus-1k path and leaves approximately 0.30 V at the GPIO. Four closed contacts draw approximately 1.20 mA. These nominal circuit calculations are not measured performance. Reserve 150 mA at USB 5 V as an unmeasured development-board planning allocation, not a power-consumption claim.

There is no galvanic isolation, reverse-polarity protection, TVS suppression, industrial input protection or mains-rated spacing. The 1k resistor is not a safety barrier. Attach only disconnected voltage-free switches. Never connect external sensor power, machine limit-switch circuits or 5 V logic. The USB host and the switch harness share ground. Initial field leads should remain below approximately half a metre and be strain-relieved. The square pad identifies connector pin one, but the headers are not keyed; inspect orientation before applying power.

UM1724 identifies the NUCLEO-F401RE pin map used here and the default VCP routing: SB13/SB14 connected, SB62/SB63 open. Check the actual board revision and power-jumper arrangement rather than modifying solder bridges from memory. The target enables only USART2 transmission; incoming desktop bytes do not arm, clear or configure the recorder.

## Mechanical sleeve and trial fixture

Build the first sleeve around a smooth, inert 35 mm diameter, 120 mm long cylindrical dummy handle. These dimensions are a starting fixture specification, not an anthropometric recommendation. Use two removable clamping collars outside the hand region to carry three independent hinged paddles. A practical starting paddle size is 18 by 25 mm for the thumb, 45 by 25 mm for the finger group and 30 by 30 mm for the palm. Round every accessible edge, cover fasteners and arrange the lead exit away from the hand. Keep a mechanical stop behind each paddle so deliberate squeezing cannot crush the switch body.

The three labels describe intended sleeve positions, not verified anatomical contact. Rotate and translate the sleeve for the participant, then mark its position and do not change it between compared trials. Fit one captive magnet to each paddle and a protected glass reed to its stationary bracket. Adjust magnet distance so a small paddle movement closes the reed without closing it at rest. Keep each magnet far enough from the other reeds that it cannot operate another zone; test cross-actuation explicitly. Measure and record the resulting actuation travel and release travel. No numerical force threshold is specified because neither spring stiffness nor applied force has been calibrated. A switch requires force and travel and therefore perturbs the interaction being studied; this is a central limitation, not a minor correction.

Provide a broad, non-slip foot pedal with a captive magnet, return spring and a stop that protects its enclosed glass reed. It has no electrical connection to any other equipment. The foot action declares the observation window and does not prove that a prescribed hand motion occurred. Use a helper or verbal cue when the participant cannot operate the pedal comfortably. Stop immediately for discomfort. Do not use this prototype to compare clinical populations, rank workers or make health recommendations. Before participant use, conduct a bench-only paddle test and inspect for pinching, sharp edges and loose wires.

## Architecture and block diagram

    Inert dummy handle
        thumb paddle ---- J3 THUMB ---- 1k ---- SENSE0 / PA0
        finger paddle --- J4 FINGERS -- 1k ---- SENSE1 / PA1
        palm paddle ----- J5 PALM ----- 1k ---- SENSE2 / PA4
    Separate pedal ------ J6 RECORD --- 1k ---- SENSE3 / PB0
                                                    |
                              each SENSE -- 10k -- 3V3
                                                    |
                           NUCLEO-F401RE GPIO, 1 ms clock
                                                    |
                         independent five-sample qualification
                                                    |
                          trial gate + eight-bin interval ledger
                                                    |
                          GL1 snapshots with CRC16, every 100 ms
                                                    |
                              USART2 -> ST-LINK USB VCP
                                                    |
                       Linux Tk contact lattice / history / CSV

Each nominal millisecond interval is counted once, either in one combination bin or in unknown. Bins are mutually exclusive: thumb-plus-palm is its own combination, not a simultaneous contribution to two independent occupancy percentages. Users can calculate marginal zone occupancy from the appropriate subsets, but must retain the unknown denominator and should not mistake it for an exact physical contact duration.

## Firmware semantics and design decisions

The five states are WAIT, READY, ACTIVE, DONE and HALT. At startup WAIT demands a qualified RECORD-low baseline. Once low is observed at both ends of 250 consecutive nominal intervals, the state becomes READY. The three grip inputs may already be actuated: gripping the handle before the pedal is normal. A subsequently qualified RECORD-high starts a trial with age zero and clears only the working trial bins. A held pedal at startup cannot create an initial trial without first satisfying the released baseline.

During ACTIVE, each contiguous acquired tick advances age. A bin receives the interval only when both endpoint raw vectors equal their respective qualified vectors, both qualified vectors are identical, all channels are qualified, and RECORD remains high. Any other acquired interval becomes unknown. Thus even a rejected one-sample raw glitch splits a segment and contributes uncertain time. Five equal samples qualify a channel; they do not imply five elapsed milliseconds before the first sample. The implementation never credits the unobserved time before a newly qualified endpoint.

A segment begins on the first credited interval after a different combination or an uncertain interval. Its longest-run counter increases only while that same bin continues uninterrupted. Segment counts are observations, not necessarily distinct physical grip episodes: bounce can split a continuous physical touch. Conversely, all-open occupancy is a legitimate observed combination. It can mean no paddle is pressed during the declared window, not necessarily that no hand touches the handle.

When RECORD qualifies low, the final acquired interval is accounted and the trial becomes DONE. The displayed result remains available while the pedal rests and the next baseline is established. READY may therefore contain the previous result. Only the next start replaces the trial arrays. The application keeps received snapshots independently, but a disconnected application may miss a completed result that is subsequently overwritten on target. There is no flash journal or target result queue.

The 60,000 ms rejecting deadline is evaluated before release completion. A release qualifying exactly on that deadline is an aborted timeout, not a completed trial. An acquisition gap, duplicated acquisition timestamp, invalid input range, hardware configuration failure, watchdog initialization failure or counter exhaustion enters terminal HALT. Gap time is not silently added to observed age: retained age ends at the last continuously acquired interval. Periodic status time can continue after HALT, but the app must not call that valid acquisition.

The conservation rules are explicit: starts equals completed plus aborted plus one if ACTIVE; age equals unknown plus the eight histogram durations. For each nonempty bin, longest plus segments minus one cannot exceed its duration, and duration cannot exceed longest times segments. Counter updates are staged in a copy so a failed cumulative increment cannot publish half a transition. The bounded trial duration prevents per-trial array overflow. A one-millisecond modular timestamp difference remains valid across unsigned wrap; larger gaps stop the experiment rather than being interpolated.

Clock restoration uses bounded waits before serial initialization. GPIO configuration is read back. The independent watchdog is started and fed after sampling; it is a recovery aid, not proof that acquisition occurred correctly. A hardware reset loses volatile history. Reset inference in the receive-only application is conservative but incomplete: an unseen reset whose counters and timestamps overtake earlier values can be indistinguishable from forward progress. Export evidence before intentional resets and start a new desktop session afterward.

## Protocol and companion application

GL1 is the version and device discriminator on every line. The ASCII body starts with GL1 followed by comma-separated canonical unsigned decimal fields in app/model.py FIELDS order. Fields are now, samples, phase, fault, raw, mask, valid, starts, completed, aborted, age, unknown, baseline, tx_errors; then eight durations h0 through h7, eight segment counts r0 through r7, and eight maxima m0 through m7. A star, four uppercase hexadecimal digits and line feed finish the record. CRC16-CCITT uses polynomial 0x1021, initial value 0xffff, no reflection and no final XOR, over the body before the star. CRC is corruption detection, not authentication.

The decoder bounds records, read chunks and replay file size; it resynchronizes on the complete GL1 marker after a truncated live frame. Strict replay rejects corruption and unfinished tails. Each row is checked for conservation and range relationships. The session additionally rejects counter rollback, contradictory duplicates, implausible time growth, movement between retained bins without a new trial, and changes after halt. These are necessary sparse-snapshot checks, not a proof reconstructing every hidden sample.

The desktop has three screens. Contact lattice shows the eight combination rows with duration, segment count and longest segment. Snapshot history shows the newest 2000 accepted snapshots including trial number and uncertainty. Setup explains wiring and interpretation. Connection failures preserve existing evidence. Reconnecting deliberately creates a new session and clears old numeric displays. A second Connect cannot reopen an already-owned port. The serial implementation restores terminal settings on close and detects actual hangup rather than merely waiting for stale data forever.

LIVE requires a connected source, at least two advancing accepted timestamps, no blocked session, no HALT and recent device progress within two seconds. Duplicated frames do not refresh progress. The host also compares cumulative device advancement to host elapsed time with a fixed 1500 ms allowance; prolonged reception pauses or growing backlog require reconnect. This conservative bound can reject long captures on an inaccurate oscillator. It does not calibrate the clock or guarantee zero serial latency. Replay is always identified as replay, regardless of how recent its timestamps appear.

CSV export creates a new file with mode 0600 and refuses overwrite. Its metadata includes jayis1 attribution, source, connection and freshness state, parser errors, blocked warning, local note and retention limit. Even an empty session exports metadata. Notes should name fixture geometry and procedure rather than personal identifiers. The application has no networking, telemetry service, account, automatic discovery, package manager hook or cloud synchronization.

## Build, run and verification

From this device directory, run `make -C firmware clean all test` using GNU Arm Embedded tools and host GCC. The target is a freestanding Cortex-M4 ELF and binary; startup vectors and linker script are included. Run `/usr/bin/python3 tools/verify.py` for the complete software, author/count and CAD gates. The verifier requires KiCad 9 CLI and its system Python pcbnew bindings, Python Tk, Xvfb, host GCC and arm-none-eabi-gcc. It does not install dependencies, flash hardware or contact a cloud service.

Launch `/usr/bin/python3 app/workbench.py` on a Linux desktop. Select the actual serial path, typically /dev/ttyACM0, and Connect. Appropriate local serial-device permission is required; do not run the whole application as root to bypass a permission problem. For a hardware-free demonstration, verification generates firmware/build/demo.gl1 from the real C engine and encoder using synthetic inputs. Load it with Replay. The fixture is not a physical measurement and is never labelled live.

The Makefile's flash target uses OpenOCD with ST-LINK and the stm32f4x target configuration. Flash only a positively identified NUCLEO-F401RE, after reviewing the wiring and saving any existing firmware. The project has not exercised that target against real hardware. SWD through the onboard ST-LINK remains the recovery route; no OTA or host command update mechanism is implemented.

The KiCad generator provides local real resistor/connector symbols, footprint identities, named nets, board outline and complete two-layer routing. The hardware checker exports a fresh schematic XML netlist and compares every reference, value, footprint, pin and net with the actual PCB and connectivity manifest. ERC and DRC must report zero violations and no unconnected items. Passive-only ERC cannot validate the external power supply, sensor actuation or harness. A clean CAD report is not fabrication or electrical bring-up evidence.

## Pilot acceptance and remaining work

Before involving a participant, independently press each paddle and observe the corresponding raw and qualified bits. Try all eight combinations and compare the app labels with physical labels. Test the footswitch held at boot, released baseline, brief raw glitches, release near the deadline, unplugged serial, reconnect and intentional target reset. Use a logic analyzer or externally timed contact source to measure tick accuracy and latency. Inspect shorted and open harness behavior: neither is uniquely diagnosed, because a broken lead resembles an open contact and a short resembles a held one.

Then compare a scripted bench sequence with a separate timestamped reference recording. Require preservation of unknown time and verify that interrupted trials are not mistaken for completed results. Only after those checks should a small consent-based dummy-handle study assess whether the instrument changes natural grip behavior too much to be useful. If spring force or travel dominates the interaction, switch to an appropriate noncontact or calibrated pressure method rather than interpreting software precision as physical validity. Physical validation, enclosure safety review and useful-study repeatability remain open engineering work, explicitly outside the automated success claims in VALIDATION.md.
