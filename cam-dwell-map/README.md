# Cam Dwell Map — indexed angular notebook for hand-operated cams

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources: MIT.

Revision A is an engineering prototype. The custom PCB is a routed passive collector/contact interface for a separate NUCLEO-F401RE. The processor, regulator, debugger, USB bridge and decoupling are on that purchased board. This design has not been fabricated, programmed into a physical target, calibrated or tested with a real cam. Software tests and CAD checks are evidence about the files, not certification of the assembled instrument.

## Purpose, audience and practical problem

Cam Dwell Map helps an automaton builder, museum workshop technician or mechanical teaching laboratory record where a slow cam keeps a follower switch closed during a complete turn. The operator turns a disconnected mechanism by hand. An incremental encoder measures shaft movement, its index marks the turn boundary, and a separate low-level follower contact records the actual switching behavior. The result is a forty-column angular map whose columns each represent ten observed encoder transitions. It answers a narrower and more useful question than a stopwatch: which parts of the rotation keep this particular follower engaged, and did the instrument really observe a complete, forward-only revolution?

A paper protractor and continuity meter can answer the same question laboriously. A camera can help, but requires reviewing images and aligning shaft angle with contact state. A time-based logger produces misleading comparisons when the hand crank pauses or speeds up. This device gives each angular increment equal weight regardless of how long the operator spent there. It separately reports turn duration, contact-state transitions between angular samples, completed turns, rejected attempts, acquisition gaps and transport errors. The distinction between angle-weighted occupancy and time-weighted duty cycle is central to its purpose.

The intended user is not measuring an engine cam's precision lift profile. A simple switch observes a chosen threshold, not displacement, force, acceleration or surface geometry. The device is useful for a wooden automaton, theatrical model or classroom timing mechanism in which a manually adjusted cam operates a detached contact. It can reveal whether moving a follower bracket changed the engagement region, whether a second lobe was added, or whether a repair shortened a broad dwell plateau. It never drives the mechanism and must not be used as a machine interlock.

### A concrete workshop scenario

Mara repairs a small museum demonstration whose cam lifts a model bird. She wants the beak switch to stay closed through roughly the same sector after replacing a worn follower roller. She mounts the encoder with a light flexible coupling and places an electrically isolated low-level contact at the follower. With the mechanism unpowered, she turns the crank slowly in the approved direction until the first index is observed, then completes another revolution. The app displays the forty-bin map and keeps the complete turn in its history table.

She exports the baseline to a new CSV, changes the roller, and repeats with the same bracket position and index alignment. If she accidentally rocks the crank backwards, that attempt is rejected rather than becoming an apparently shorter dwell. If she pauses, the angular map is unaffected until the sixty-second acquisition limit is reached. Her decision is whether the coarse engagement sector changed enough to justify further mechanical adjustment. The instrument does not decide whether the repair is safe or whether the bird's motion is artistically correct.

## Originality, alternatives and repository distinction

The repository directories and device descriptions were inventoried before this project was created. ContactAtlas learns electrical contact signatures and transitions of a detached selector; it has no shaft-angle reference. Takeup Atlas compares the onset of output movement after an input reversal; it uses two encoders and deliberately does not measure an indexed full turn. Nest Settle records seating at four fixture corners. Cam Dwell Map instead combines one indexed encoder and one follower threshold to create angle-weighted occupancy maps with explicit complete-turn rejection. The acquisition platform and passive carrier construction are reused from jayis1's existing designs, not presented as new inventions themselves.

A limited external search found substantial close prior art. [Marposs M57 Profile](https://www.marposs.com/eng/product/measuring-station-for-cam-profile-inspection) uses linear and angular encoders for dimensional camshaft inspection. [Auditech Cam Pro Plus](https://audietech.com/product/cam-pro-plus/) measures lifter and valve motion, including opening, closing and duration. These are much richer metrology tools. This project does not claim to have invented cam measurement, rotary encoding or angular dwell analysis. Worldwide uniqueness and patentability have not been established.

The proposed original contribution is the small, open, receive-only workflow for coarse digital follower thresholds: every retained map has a witnessed index-to-index interval, exactly four hundred accepted phase transitions and explicit rejection accounting. No motor drive, linear displacement probe, cloud service or proprietary desktop package is necessary. A general logic analyzer remains preferable for fast electrical transitions. A protractor remains preferable for a one-off static check. The design is worthwhile when repeatable, inspectable records of many slow hand-operated trials save more effort than building the fixture costs.

## Hardware specification and architecture

| Subsystem | Revision A selection |
|---|---|
| Controller | NUCLEO-F401RE, STM32F401RET6, MB1136 platform |
| Processor configuration | Cortex-M4, reset HSI 16 MHz, 512 KiB flash, 96 KiB SRAM |
| Encoder | Omron E6B2-CWZ6C 100P/R 0.5M, NPN open-collector A, B and Z |
| Follower | Omron D2F-01L low-level lever switch, COM and NO only |
| Interface board | 80 by 70 mm, two-layer 1.6 mm FR4, through-hole assembly |
| Signal conditioning | Four 10 kΩ pullups to 3.3 V and four 1 kΩ series resistors |
| Connection to computer | USART2 PA2, ST-LINK virtual COM, 115200 baud, 8N1 |
| Sampling | Nominal 1 kHz polling; continuity invalidated by any skipped millisecond |
| Power | USB for Nucleo; separate regulated 5 V for encoder, common ground |
| Actuators / radio / battery | None |

```text
hand crank --> cam shaft --> follower lever --> isolated NO contact
                   |                                      |
          100 P/R encoder A/B/Z                           |
                   |                                      |
                   +-------- passive interface -----------+
                              10k bias / 1k series
                                      |
                         PA0 PA1 PA4 PB0 digital inputs
                                      |
                            NUCLEO-F401RE, HSI 16MHz
                              phase/index qualification
                              400-count complete-turn map
                                      |
                             USART2 TX -> ST-LINK USB
                                      |
                         Linux Tk companion, local CSV
```

This is a modular instrument, not a bare-MCU board. The Nucleo must remain part of the assembly. The passive carrier is connected with short jumpers; it is not an Arduino shield with a mechanically matching pin layout. The supplied KiCad schematic includes actual authored connector and resistor symbols, embedded symbol definitions, wire labels and footprint references. The PCB includes matching footprints, assigned pad nets, routed copper and a closed outline. The project has no unconnected pads in its checked netlist. It contains no encoder power distribution, TVS network, galvanic isolation or battery charger.

## Exact wiring and electrical boundaries

J1 pin 1 takes regulated 3.3 V from Nucleo CN6 pin 4. J1 pin 2 connects to CN6 pin 6 ground. J2 pins 1 through 4 go respectively to CN8 pins 1 through 4, labeled A0, A1, A2 and A3 on the Nucleo. Their MCU pins are PA0, PA1, PA4 and PB0. The analog header names do not mean the firmware uses an ADC. All four are digital inputs with internal pull resistors disabled.

| Carrier input | Signal pin 1 | Return pin 2 | MCU | Series / pullup |
|---|---|---|---|---|
| J3 | ENC_A | GND | PA0 / SENSE0 | R2 1k / R1 10k |
| J4 | ENC_B | GND | PA1 / SENSE1 | R4 1k / R3 10k |
| J5 | INDEX | GND | PA4 / SENSE2 | R6 1k / R5 10k |
| J6 | FOLLOWER | GND | PB0 / SENSE3 | R8 1k / R7 10k |

The specified encoder's brown wire receives external 5 V and blue connects to common ground. Black A goes to J3 pin 1, white B to J4 pin 1, and orange Z to J5 pin 1. Verify the actual purchased part's wiring sheet before connection. Only the NPN open-collector CWZ6C variant is within this electrical contract. PNP, push-pull and differential line-driver versions are not interchangeable. Never connect encoder brown to any signal terminal. Do not join external positive 5 V to the USB supply; only grounds are joined.

Connect the follower's COM to J6 ground and NO to J6 signal. Leave NC insulated. The switch must be detached from any other powered circuit, including the original apparatus wiring. Closed contact current is approximately 0.30 mA and the nominal MCU low level is 0.30 V. With a collector residual voltage of 0.4 V, the calculated MCU node is approximately 0.664 V. Verify margins on the real assembly. This small current can fail to wet oxidized contacts; a missed closure is not proof of a broken cam. The 1k resistor limits incidental current but does not make externally powered wiring safe.

PA2 is AF7 USART2 TX to the Nucleo ST-LINK virtual COM bridge. The firmware never enables the receiver. Keep the manufacturer's applicable virtual COM solder-bridge arrangement; for the repository's MB1136 configuration this is SB13/SB14 on and SB62/SB63 off. Consult [UM1724](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf) and the actual revision before assembly. Register definitions follow [RM0368](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf). This run reused the existing documented platform rather than claiming a fresh independent manufacturer-document audit.

## Mechanical construction, power and cost

Mount the encoder coaxially using a low-load flexible coupling. Its bearings should not support the cam, crank or belt tension. Mount the follower separately so adjusting the switch does not alter encoder alignment. Mark the encoder-to-cam relationship mechanically and do not compare maps after loosening that coupling without establishing a new baseline. Index defines the origin, not a particular cam feature. Reversing the A/B connections reverses the accepted direction; make that choice with power removed and document it in the workshop record.

An approximately 160 by 110 by 45 mm insulating enclosure is a planning allowance for the Nucleo and carrier, not a supplied enclosure CAD model. The carrier has no mounting holes; use insulated edge supports and strain relief. Cover the headers to prevent accidental metallic contact. Keep wiring short, initially below 0.5 m, and away from motors and switching supplies. Operation is intended for a dry indoor bench around 10–35 degrees Celsius; there is no certified environmental rating or ingress protection.

Reserve 150 mA at USB 5 V for the Nucleo as an unmeasured planning allowance. Reserve 80 mA steady-state at external 5 V for the selected encoder, and choose a supply that tolerates its published startup inrush. Do not assume a tight 80 mA current limiter will start it successfully. The four carrier branches draw at most about 1.2 mA from 3.3 V, or 3.96 mW. Combined nominal planning allowances are about 1.15 W before regulator and supply losses. No battery-life figure is applicable because this revision has no battery.

A quantity-one planning BOM is: Nucleo $25, encoder $80, low-level lever switch $3, carrier PCB $8, resistors and headers $4, regulated supply and cables $15, brackets/coupling/enclosure $25. The total allowance is $160, excluding labor, tax and shipping. A quantity-100 target is approximately $120 per assembled kit before labor and certification, not a supplier quote. Reused workshop encoders can reduce cost, but substitutions require reviewing output topology, index width, resolution and speed rather than merely matching the shaft diameter. Suggested resistor family is Yageo CFR-25, 1% preferred, 0.25 W; standard 2.54 mm vertical pin headers fit the listed footprint dimensions.

## Measurement contract and firmware design

The engine consumes an inverted four-bit GPIO vector every millisecond. A and B are decoded without debounce because filtering quadrature independently can change phase order. The legal positive sequence is 00, 01, 11, 10, 00 after input inversion. Both-bit transitions are invalid. Negative transitions reject an active turn. A valid rising index opens a trial only after the follower has qualified. Booting while index is already asserted does not create an origin. A later low-to-high index transition is necessary.

The follower alone uses five milliseconds of stable candidate state before its qualified value changes. This is electrical qualification, not a claim about instantaneous cam geometry. A rejected short glitch never becomes an angular sample. Qualification continues while the shaft is stationary. Every positive encoder edge increments travel and samples the qualified follower. Counts one through ten populate bin zero, eleven through twenty bin one, and so on. Each bin therefore contains a number from zero through ten. A full column means all ten sampled positions were closed, not that every unobserved position between them was closed.

The next rising index completes the trial only when exactly four hundred positive transitions have been counted. A simultaneous final phase edge is counted before index validation. The same index starts the next trial at zero. An early index, a 401st phase edge without index, reversal, invalid quadrature, skipped sample or sixty-second timeout rejects the current trial. A rejecting sample cannot also establish a new origin. The operator must reach another valid rising index. Rejection does not erase the last completed map, and its reason stays visible through subsequent successful turns until reset or a later rejection.

The last completed result contains duration, total closed samples, sampled-state changes and forty bin counts. State changes count differences between consecutive angular samples within the turn, not the closing boundary from the last sample back to the first. Motion stopping does not add samples. Direction, elapsed time and a map are not sufficient to reconstruct lift or continuous contact behavior. A turn with all samples open is a legitimate result, as is one with all samples closed.

Use no more than fifty decoded transitions per second initially and verify minimum phase and Z pulse dwell of at least five milliseconds. At that conservative speed a complete turn takes eight seconds. The nominal angular increment is 0.9 degrees and a displayed bin spans nine degrees. Neither is a calibrated accuracy claim. The five-millisecond contact qualification produces speed-dependent displacement of observed boundaries. Missing complete quadrature cycles can alias without an invalid transition; the four-hundred-count check catches many but not all combinations of extra and missing edges. Coupling slip and bad index alignment remain major physical failure modes.

The implementation uses bounded memory, no heap, no RTOS and no vendor HAL dependency. Startup initializes data and BSS, the linker describes actual flash and SRAM, and SysTick only increments a counter. Main samples GPIO, advances the engine and services a nonblocking single-frame UART queue. A frame waiting over 100 ms is abandoned and counted. Watchdog initialization has bounded waits; clock initialization uses bounded iteration loops before SysTick is available. Exhausted counters or startup failures latch terminal fault rather than wrapping evidence identities. Power loss discards all RAM evidence. Recovery is explicit reset or debugger programming, not an unauthenticated remote update.

## Protocol and companion application

Every approximately 100 ms the device emits one ASCII line with marker CDM1. The decimal fields are, in order: time, state, raw, follower, travel, completed, rejected, gaps, reason, duration, closed, transitions and tx_errors. A final comma introduces exactly forty uppercase hexadecimal bin digits, each between 0 and A. An asterisk, four uppercase CRC digits and LF end the frame. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xffff, covering marker and comma-separated payload before the asterisk. It has no reflection or final XOR. CDM1 permanently identifies this fixed 400-count, 40-bin, 5-ms qualification, 60-second timeout policy.

State values are zero waiting, one acquiring and two terminal fault. Reasons are zero none, one reverse, two invalid quadrature, three sample gap, four wrong length, five timeout and six exhaustion/startup fault. Time and counters are unsigned 32-bit values. Raw is the current inverted GPIO vector; follower is its qualified fourth channel. Travel describes the current trial, while duration, closed, transitions and bins describe the last complete trial. The latter fields are all zero before any completion. This intentional separation prevents a partially acquired map from looking like a complete result.

The Linux Python/Tk companion uses only the standard library. Connect / Replay discovers ACM and USB serial devices, allows a manually entered device path and opens raw 115200-baud transport. Angular map displays the last completed forty-bin chart alongside live acquisition and rejection status. Turns / Export retains the latest hundred observed completions, displays their durations and counts, and writes a new private CSV with provenance. Export refuses overwrite rather than silently replacing a baseline. There is no phone pairing, network listener, telemetry upload or account.

A live session freezes on backwards device time, decreasing cumulative counters, contradictory duplicate timestamps, changing completed data without a new identity, or impossible completion rate. Duplicate identical frames do not refresh freshness. A receipt pause longer than one second requires explicit reconnect; a developing backlog is also rejected when host elapsed time exceeds accumulated device progress by 750 ms plus a two-percent drift allowance. These are conservative observation checks, not authentication. The first buffered frame cannot prove its own freshness, and a reset with forward-looking time and counters can evade inference without a persistent boot identifier.

The parser validates CRC, dimensions, field ranges, map conservation and lower/upper transition bounds from each bin and forced homogeneous boundaries. Those bounds reject grossly impossible maps; they do not prove every accepted mixed-bin combination has a realizable exact sample order. Historical snapshots retain their original live or replay provenance after disconnect. CSV includes connection state, warning, parser errors, missed-turn count and host age even when no complete result exists. Device snapshots repeat only the newest completed turn, so attachment after several turns records missing prior identities rather than inventing their maps.

Replay accepts only regular files up to two MiB. Comment lines beginning with a hash may identify synthetic fixtures. Empty files, malformed frames, incomplete final frames, FIFOs and oversized files are rejected. The supplied test fixture is produced by the real C encoder using synthetic inputs and is explicitly not a hardware capture. No untrusted file causes code execution. Local device paths and captures remain trusted operator choices; CRC protects against accidental corruption, not malicious peers.

## Build, verification and first physical commissioning

From this directory run `make -C firmware all test`, then `python3 tests/test_app.py`, and `xvfb-run -a python3 tests/test_gui.py`. The target build requires GNU Arm Embedded tools and produces an ELF, map and binary under ignored firmware/build. Host tests use GCC address and undefined-behavior sanitizers. Python requires Tk; Xvfb is only needed for headless GUI testing. Start the actual app with `python3 app/cam.py`. Replay `firmware/build/fixture.txt` to inspect the synthetic half-closed angular map.

KiCad generation uses `/usr/bin/python3 tools/generate_hardware.py` with the distribution's pcbnew bindings and installed footprint libraries. Run `/usr/bin/python3 tools/check_hardware.py` to execute ERC, DRC and exact schematic-export/PCB/manifest comparison. Fresh XML netlists are generated during this check, not merely inferred from text labels. The supplied project also includes a native exported netlist. All generated design metadata credits jayis1. See VERIFICATION.md for the actual run results and limitations.

Before connecting sensors, have a competent engineer review the schematic, connector orientation, supply arrangement, part substitutions and physical clearances. Inspect resistance and continuity with power removed. Power the Nucleo alone, measure 3.3 V, then test each input with a known isolated contact. Add the separately powered encoder only after confirming grounds and collector topology. Verify A/B sequence and Z width with an independent oscilloscope or analyzer while turning slowly. Confirm that exactly four hundred transitions occur per index interval and that reversing produces rejection.

Finally, test known all-open, all-closed and half-turn contact patterns, compare an independent angular reference, deliberately unplug a signal, interrupt USB and induce pauses. Record measured timing and repeatability rather than copying synthetic expected values into a calibration report. Human approval remains required before fabrication and deployment. This is not a safety device, engine calibration tool, certified measuring instrument or production-ready product; its delivered value is a coherent, inspectable prototype with real software and CAD validation.
