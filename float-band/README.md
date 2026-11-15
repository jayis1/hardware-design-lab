# Float Band — reference-bracket switching notebook for detached float switches

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. The acquisition drivers and passive carrier infrastructure are reused from jayis1's Label Veil at commit 98f7d56; the reference-bracket engine, protocol, application model, screens and experiment tests are specific to Float Band.

Revision A is an unbuilt engineering prototype. Target compilation, simulated acquisition, desktop workflows and native electrical CAD checks are reproducible. No PCB has been fabricated, MCU flashed, float assembly calibrated or water experiment performed. The custom PCB is a passive interface for a separately purchased NUCLEO-F401RE, not a standalone microcontroller board. This instrument does not control a pump and must not be used to certify an overflow alarm or safety interlock.

## Purpose, audience and practical utility

A float switch can appear functional when moved by hand but behave differently when mounted in a small tank. Its moving element may rub against the wall, a mounting nut may distort its stem, surface movement may repeatedly reopen the contact, and a different orientation may change the switching polarity. A continuity meter tells the operator whether the contact is currently closed. It does not conveniently retain where repeated openings and closings occurred relative to a set of reference levels. Video is useful, but reading a small meter and a moving waterline from video requires deliberate framing and subsequent review.

Float Band is a supervised clean-water bench fixture for comparing disconnected float switches before placing them in noncritical aquarium, model irrigation or teaching assemblies. Three independent reference float contacts define four ordinal water-level regions. A fourth contact is the device under test, abbreviated DUT. Every qualified DUT opening or closing is assigned to a stable reference region, or to an explicit unknown category if the reference evidence was disturbed during that edge. The companion shows separate opening and closing distributions, short rejected contact excursions, invalid reference observations, skipped reference bands and reference-direction reversals.

This distinction matters. Twenty closings clustered between the middle and upper references suggest a repeatable bracket under the particular test conditions. Ten closings in that bracket and ten marked unknown instead suggest that the fixture or fill rate needs attention. Repeated closings and openings in a stationary bracket can reveal switching instability without pretending to distinguish electrical contact bounce from genuine mechanical float motion. The software preserves what the contacts established rather than guessing the physical cause.

The primary audience is a small workshop or educator who already has a tank, a set of low-energy reed floats and a processor development board. An aquarium hobbyist comparing bracket positions is another intended user, provided no installed controller remains connected. A maintenance technician can use it for exploratory bench comparison, but a calibrated industrial test procedure remains necessary for service qualification. There are no cameras, microphones, accounts, cloud uploads or autonomous actuators. The design intentionally trades continuous height resolution for inexpensive, explicit spatial brackets and auditable uncertainty.

## Originality and nearby designs

The repository device inventory was inspected before selecting this concept. Siphon Witness measures paired source and receiver band-transit times during a transfer. Its purpose is not switch characterization. Rail Skew compares two ordinal drawer progressions, while ContactAtlas maps detached rotary-switch contact topology. Float Band instead treats three channels as one ordered reference word and the fourth as an independently qualified DUT; its principal result is a polarity-separated spatial switching distribution with uncertainty attached to each edge. Reusing the passive carrier does not imply that those experiments or their firmware are interchangeable.

Float-switch hysteresis, continuity testing and rising/falling water tests are established techniques. A limited prior-art search found WIKA's discussion of density-dependent switching and reed hysteresis, Madison Sensor's float-switch test guidelines, and supplier instructions for checking rising and falling levels. The contribution proposed here is this small reference-bracket recorder and its conservative attribution contract, not the invention of float testing itself. No worldwide novelty, patentability or freedom-to-operate claim is made.

Relevant reading includes [WIKA's switch-point explanation](https://blog.wika.com/en/products/level-products/how-does-one-specify-the-switch-point-of-a-float-switch/) and [Madison Sensor test guidelines](https://files.plytix.com/api/v1.1/file/public_files/pim/assets/ea/8a/ec/64/64ec8aea3a090800015845ef/texts/9d/8f/57/6a/6a578f9d5ea4f212d76731e8/Float%20Switch%20Test%20Guidelines.pdf). These sources support treating mounting, contact action and bidirectional operation as real engineering considerations. They do not validate the accuracy of this proposed fixture.

## Hardware specifications and electrical contract

| Item | Revision A specification |
|---|---|
| MCU assembly | ST NUCLEO-F401RE, STM32F401RET6 Cortex-M4 |
| Clock | Internal HSI, nominal 16 MHz; no external crystal required by firmware |
| Memory | 512 KiB flash, 96 KiB SRAM; static allocation only |
| Sensors | Three reference dry-contact floats and one disconnected DUT contact |
| Carrier | 80 × 70 mm, two copper layers, 1.6 mm FR-4 |
| Supply | Nucleo 3.3 V output and common ground; no local regulator |
| Host connection | USART2 TX, PA2 AF7, through ST-LINK virtual COM |
| Serial format | 115200 baud, 8 data bits, no parity, one stop bit |
| Acquisition | Nominal 1 ms observations, five consecutive samples to qualify |
| Programming | Onboard ST-LINK SWD; flash image at 0x08000000 |
| Companion | Linux Python 3 desktop with Tk; no pip dependencies |
| Use environment | Supervised indoor clean-water bench, electronics kept dry |

The MCU, reset circuit, supply regulation, decoupling, USB bridge and debugger are on the purchased development board. The custom board contains only the connectors and resistors documented below. Its project files do not pretend that the external processor is soldered onto the passive carrier.

| Bit | Signal | Field connector | J2 pin | Nucleo Arduino pin | MCU pin |
|---|---|---|---|---|---|
| 0 | LOW | J3.1 | 1 | A0 | PA0 |
| 1 | MID | J4.1 | 2 | A1 | PA1 |
| 2 | HIGH | J5.1 | 3 | A2 | PA4 |
| 3 | DUT | J6.1 | 4 | A3 | PB0 |

Every field connector pin 2 is ground. J1 pin 1 connects to Nucleo 3V3, CN6.4; J1 pin 2 connects to ground, CN6.6. J2 maps to CN8 pins 1 through 4. Check the actual board revision, connector orientation and ST-LINK serial solder bridges against ST UM1724 and the MB1136 documentation before wiring. Peripheral registers follow STM32F401 RM0368; SysTick follows the Cortex-M4 programming model. Existing driver tests and target linking do not replace checking the physical board.

Each input has a 10 kohm pull-up from SENSE to 3V3 and a 1 kohm resistor between SENSE and its field connector. A closed switch reads logically one after firmware inversion. At nominal values the contact current is 0.300 mA and the low input is approximately 0.300 V. Four closed contacts draw 1.20 mA; carrier resistor dissipation totals approximately 3.96 mW. These are resistor-network calculations, not measured system consumption. Budget the development board and USB bridge separately using its actual board configuration and measurements.

There is no battery, charger, radio, isolation barrier, reverse-polarity circuit or field surge protector. USB common ground is shared with every contact. The 1 kohm resistor limits ordinary low-voltage contact current; it is not permission to attach a powered signal. Use only voltage-free switches completely disconnected from their former circuits. Never attach mains, a PLC output, a pump, a powered level transmitter or an unknown installed alarm loop. A cable break reads open and can be indistinguishable from a genuinely open switch.

## Architecture and block diagram

```text
LOW float  ---- J3 -- 1k -- SENSE0 -- A0 / PA0 --+
MID float  ---- J4 -- 1k -- SENSE1 -- A1 / PA1 --+-- bundled reference qualifier
HIGH float ---- J5 -- 1k -- SENSE2 -- A2 / PA4 --+       |
DUT contact --- J6 -- 1k -- SENSE3 -- A3 / PB0 -------- independent DUT qualifier
                           |                           |
                  each SENSE: 10k to 3V3        edge envelope attribution
                                                       |
USB host <--- ST-LINK VCP <--- USART2 TX <--- FB1 cumulative snapshots
   |
Tk companion: switching-band screen / setup screen / strict replay / CSV
```

The schematic uses actual numbered connector and resistor symbols, connected by named nets. The PCB carries corresponding footprints, pads, traces, a board outline and labelled interfaces. Generation is deterministic in identifiers, but output formatting can vary with KiCad versions. Verification exports a fresh schematic netlist and compares every component value, footprint identity and pin/net connection with the PCB and the connectivity manifest. Passive-only ERC cannot establish that an external Nucleo power source is connected correctly or that a wet harness is safe.

## Bill of materials and mechanical construction

Populate R1, R3, R5 and R7 with 10 kohm, one-percent axial resistors. Populate R2, R4, R6 and R8 with 1 kohm resistors of the same tolerance. All use DIN0207 bodies and 7.62 mm lead spacing; a rating of at least 0.125 W is sufficient for the intended network. J1 and J3 through J6 are two-pin 2.54 mm headers. J2 is a four-pin header of the same pitch. Use matching insulated harness connectors, clear labels and separate strain relief. The board has no mounting holes; insulating edge rails are the intended retention method.

External components are one NUCLEO-F401RE, a data-capable USB cable, three sealed low-energy reference float switches, a disconnected DUT, a nonconductive test vessel, a rigid bracket rail and a spill tray. Reference floats must close when submerged. Select them for reliable switching near 0.3 mA; a large maximum current rating alone does not prove suitability for this low wetting current. The BOM deliberately specifies required contact behavior rather than asserting an unverified particular float part number. Procurement and assembled calibration remain release gates before building a useful physical instrument.

Mount the three reference floats at progressively increasing heights with enough separation that their switching regions cannot overlap after accounting for float hysteresis and slosh. As an initial mechanical experiment, use a slotted rail that allows independent height adjustment and lock each bracket against rotation. Record the actual rising and falling switching heights with a ruler and video before relying on the ordinal regions. Place the DUT on a separate adjustable bracket so moving it cannot move the references. Keep all floats clear of walls, hoses and one another.

Use manual slow filling and draining, with the vessel in a spill tray below the electronics. Do not use electrically driven pumps as part of this revision. Keep signal/ground pairs short, preferably below one metre, and route them away from power equipment. There is no enclosure drawing, ingress rating or external ESD qualification. An insulating dry enclosure above the tray must accommodate both the carrier and the purchased processor board. The mechanical instructions define an experimental arrangement, not a production waterproof product.

## Firmware behavior and design decisions

At boot the clock driver restores HSI operation with bounded waits before setting up GPIO, USART2 and SysTick. It checks that the sensing pins remain inputs and that the transmitter configuration matches the receive-only protocol. Watchdog initialization failure latches a terminal fault. Main polls serial output without blocking, samples when the millisecond tick advances, and requests a cumulative snapshot every 100 ms when the transmitter is available. The transmit buffer is bounded and an undrained frame times out after 100 ms. Partial frames can therefore occur and the companion explicitly resynchronizes on the complete FB1 marker.

Reference bits are qualified as a bundle. Legal words, listed in numeric order, are 0, 1, 3 and 7, meaning below LOW, between LOW and MID, between MID and HIGH, and above HIGH. Other words mean an impossible or unusable reference arrangement. Five identical consecutive observations establish a qualified word. A sustained invalid word contributes invalid-reference samples. This is an observation count, not a calibrated physical exposure duration. Unrelated DUT chatter does not delay reference qualification, and reference chatter does not suppress sustained DUT edges.

The DUT has its own five-sample qualifier. Its first qualified state is a baseline, not an opening or closing. When raw DUT state first differs from that baseline, an edge envelope begins. The engine captures the previously qualified reference band. Every sample through DUT qualification must retain that raw reference word and a valid prior bracket. Any reference disturbance taints the envelope permanently, including a one-sample glitch that would not survive reference qualification. A DUT edge qualifying on the same sample as a reference change is therefore unknown, not assigned to whichever band happens to be processed first.

If raw DUT returns to its qualified state before five samples, the envelope is rejected and the noise counter increments. That counter describes short sampled excursions; it cannot establish electrical bounce frequency or detect pulses between observations. If the alternate state persists for five samples, the engine increments exactly one opening or closing cell, using the unknown cell if tainted. Repeated qualified binary edges must alternate. The initial level and both switch polarities are supported without interpreting closed as intrinsically good or bad.

Adjacent reference changes set a retained upward or downward direction. A later opposite adjacent change increments reversals. A jump over a band increments skips and clears direction. Invalid reference words also clear direction. Direction means the latest qualified adjacent reference movement, not a promise that water is still moving in that direction. The application does not pair edges into automatic fill/drain cycles or calculate millimetre hysteresis: doing so would require stronger motion and reference-calibration evidence than four contacts provide.

Sampling discontinuity is terminal for that run. The first gap increments a diagnostic and latches F_GAP without attributing new contact evidence. Ordinary unsigned timestamp wrap is supported; repeated or skipped acquisition timestamps are not treated as elapsed stable samples. Statistics use bounded counters capped at one billion. Updates are staged transactionally so an exhausted counter cannot publish half an edge. A terminal state continues to transmit heartbeat time but freezes acquisition evidence. Resetting the MCU begins a new volatile history. There is no flash logging, remote update, network service or host command receiver.

## Protocol and application operation

FB1 is ASCII with comma-separated unsigned decimal values, followed by an asterisk, four uppercase hexadecimal CRC digits and LF. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xFFFF, covering the entire body before the asterisk. The schema is fixed by `app/model.py`: sequence, milliseconds, fault, raw bits, reference-valid, DUT-valid, band, DUT state, direction, total edges, five opening bins, five closing bins, noise, invalid samples, skips, reversals, gaps and transmission errors. A version change requires a new marker rather than silently changing policy under FB1.

The desktop has a switching-bands screen and a setup/interpretation screen. Enter a serial device such as `/dev/ttyACM0`, connect, and wait for advancing records. Connection does not reset device history. Disconnect retains the last evidence. Replay requires disconnection and validates an entire regular file before replacing the current session. Replay reads are capped at two MiB; special devices and FIFOs are rejected. CSV export creates a new file exclusively, includes jayis1 attribution and session provenance, and refuses to overwrite an existing record. Only the latest ten thousand snapshots are retained; cumulative firmware totals remain available even when older display snapshots have aged out.

A received CRC is not sufficient validation. The model checks field ranges, edge conservation, alternation, qualification startup bounds, monotonic counters, acquisition opportunity, immutable halted evidence and unchanged data on duplicate timestamps. Errors latch the session and require an explicit new connection or replay. These necessary checks cannot reconstruct all hidden acquisition samples or authenticate a sender. Local serial data is trusted only as diagnostic evidence, not a security boundary.

Live freshness requires multiple advancing records and recent progress, rather than mere reception of duplicates. A one-second progress pause marks evidence stale. A later reception pause or cumulative host/device clock discrepancy exceeding two seconds blocks the session. This intentionally conservative rule can require reconnecting during a long run because HSI is uncalibrated. Reconnect flushes the local serial input, but there is no persistent boot identity; some MCU resets with compatible forward counters remain indistinguishable. Exported provenance must be retained when comparing runs.

## Building, tests and first physical bring-up

From this device directory run `make -C firmware all test fixture`, then `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'`. Run `python3 tools/check_hardware.py` using Python with KiCad pcbnew installed. Launch the desktop with `python3 app/workbench.py`; use `--replay firmware/build/demo.fb1` for the explicitly synthetic fixture. Headless export is available with `--replay firmware/build/demo.fb1 --export /path/to/new.csv`. The application needs Python Tk and Linux termios, not React Native or a package-manager download.

The firmware build requires GNU Arm Embedded tools and GNU Make. Host tests use GCC with AddressSanitizer and UndefinedBehaviorSanitizer. Native CAD generation and checks were exercised with KiCad 9; older schemas are not promised. `tools/import_platform.py` is a one-time provenance helper and refuses to overwrite imported files. It is not a normal build prerequisite. `tools/generate_hardware.py` regenerates the custom CAD sources; inspect diffs and rerun ERC, DRC and connectivity checks afterward.

Before connecting a vessel, inspect the populated carrier, check for shorts, verify the supply, and exercise each input with a known dry switch. Confirm the application sees the correct bit and that invalid reference combinations remain unknown. Only then connect independently checked floats. Record reference rising and falling heights, inspect mechanical interference, and compare a slow fill/drain recording with video. Test cable removal and MCU reset deliberately. Do not claim calibrated hysteresis, physical reliability, regulatory compliance or fabrication readiness solely from the software and CAD checks documented in VALIDATION.md.
