# Tooth Census — indexed tooth-follower comparison fixture

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint material retains its upstream licensing. Shared acquisition and CAD infrastructure is reused from jayis1's Sector Soak at e8a0f67; the tooth-count engine, protocol contract, application screens and experiment tests are specific to this project.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive interface for a separately purchased NUCLEO-F401RE, not a standalone MCU board. Firmware linking, synthetic-input behavior, local application operation and CAD connectivity are verified separately from fabrication, flashing, mechanical assembly, contact performance and useful measurement accuracy. None of those physical activities has been performed for this design.

## Purpose, practical value and target audience

Tooth Census helps a maker of wooden or printed teaching gears compare the number of follower pulses observed on repeated slow, hand-turned revolutions. It combines a shaft encoder, an independently positioned once-per-turn index and a lightly loaded tooth follower. Every accepted capture must contain exactly ninety-six qualified encoder transitions in one observed direction between index rising passages. The first accepted pulse count becomes the reference for that powered session. Subsequent accepted counts update the minimum, maximum, total and mismatch tally. Reversed, incomplete and boundary-ambiguous captures are rejected rather than mixed into the comparison.

The intended user is a museum workshop, automaton class or small fabrication studio making low-energy plastic, cardboard or wooden demonstrators. A printed gear can have a damaged crest, an obstructed valley or a follower adjustment that reads inconsistently. A person can count teeth by eye, but remembering whether the electrical follower actually responded to every feature on several unevenly hand-cranked turns is more difficult. This fixture records that limited repeatability question without a camera or a cloud service. It does not measure involute profile, backlash, pitch error, tooth strength, torque or dimensional accuracy.

A useful first experiment begins with a deliberately coarse, known-count teaching wheel. Mark its physical teeth independently. Record several turns with a low-force follower adjusted to lift at each crest and return in each valley. Change only the follower bracket offset, repeat, and compare the accepted counts. A mismatch indicates that electrical responses differed from the first accepted turn; it does not identify which tooth changed or why. Inspect the mechanism and correlate with slow-motion video before attributing the difference to a damaged gear. A consistent wrong count remains consistent, not correct.

The instrument is deliberately observational. It has no motor driver, relay, machine-enable output, buzzer interlock or remote-control command. Keep the mechanism disconnected from power and move it by hand. It is unsuitable for installed machinery, engines, heavy flywheels, protective equipment or safety-critical gearing. A successful acquisition is called “accepted” in the application; the protocol's compact field name `good` means only accepted acquisition, never acceptable product quality. This distinction is essential when exporting results for someone who did not attend the experiment.

## Distinction from existing work and prior art

The configured hardware-design-lab checkout was refreshed and its device directories and README titles inventoried before selection. Related purpose sections were reviewed. Cam Dwell Map uses an encoder, index and follower to create angular occupancy bins. Tooth Census instead counts complete independently qualified follower pulses, including pulses that begin and end while the encoder remains in one coarse state, and compares complete-revolution totals against a learned reference. It does not approximate tooth count from angle-weighted bin changes. Ratchet Audit measures rollback and pawl association inside handle strokes; it does not compare tooth-response counts between shaft index passages. Pattern Sled learns a repeated three-bit strip word at separate reading clocks rather than counting pulses on an indexed gear.

Gear tooth sensing and inspection already exist. A bounded search found Sensor Solutions gear-detection sensors, including single-channel pulse outputs and speed/direction variants, and EyeVisionTechnology's EyeSens gear-tooth inspection system, which inspects tooth presence, width and spacing optically. Those are established alternatives, not evidence that this project invented tooth counting. The proposed differentiated contribution is a small contact-based comparison fixture for nonmetal teaching gears, with conservative revolution acceptance, explicit rejected captures and a private local notebook. Worldwide novelty, patentability and freedom to operate are not established.

An ordinary logic analyzer can capture richer electrical detail. A camera-based inspection system can answer dimensional questions this fixture cannot. The product hypothesis is that a simple automatically bounded count notebook is easier to use repeatedly during follower adjustment than manually reviewing logic traces. That hypothesis needs a real workshop trial. If the follower's spring force changes the gear motion materially, if the index cannot be placed in a clear valley, or if the pulses are too narrow, use a different sensor architecture rather than pretending this software fixes unsuitable mechanics.

## Hardware specification and block diagram

| Item | Revision A specification |
| --- | --- |
| MCU | STM32F401RET6 on NUCLEO-F401RE MB1136 |
| Processor | Cortex-M4, nominal 16 MHz internal HSI |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Encoder | Bourns PEC11R-1015F-N0024, contact quadrature, 24 pulses/revolution |
| Index | External normally-open reed and one shaft magnet |
| Tooth sensor | External normally-open reed operated by a magnet on a mechanical follower |
| Acquisition | Nominal 1 ms polling, five consecutive samples per filter group |
| Connectivity | USART2 transmit only, 115200 baud, 8 data bits, no parity, one stop bit |
| Host connection | Nucleo ST-LINK virtual COM over USB |
| Carrier | 80 × 70 mm, two copper layers, 1.6 mm board thickness |
| Input circuit | Each channel: 10 kohm pullup and 1 kohm series resistor |
| Power | USB-powered Nucleo; carrier uses its 3.3 V and ground |
| Companion | Linux Python 3.13 and Tk; standard library only |

Signal architecture:

    Gear shaft ---- flexible coupling ---- 24-pulse contact encoder A/B
         |                                    |                 |
         +---- one magnet / index reed ------ | ---------------- | --- J5
         |                                    |                 |
         +---- gear crests / follower          J3                J4
                         |
                    magnet / reed ----------------------------------- J6

    J3 ENC_A -- 1k -- SENSE0 -- PA0 / A0 --+
    J4 ENC_B -- 1k -- SENSE1 -- PA1 / A1 --+-- independent filter groups
    J5 INDEX -- 1k -- SENSE2 -- PA4 / A2 --+-- revolution capture engine
    J6 TOOTH -- 1k -- SENSE3 -- PB0 / A3 --+-- TC1 CRC snapshots
                      |                            |
                    10k to 3V3                USART2 / ST-LINK
                                                   |
                                      local comparison / notebook / CSV

Every sensor connector has signal on pin 1 and ground on pin 2. The encoder common terminal goes to sensor ground; A and B go to J3.1 and J4.1. These contacts belong exclusively to this instrument. J1.1 connects to Nucleo CN6.4, 3.3 V; J1.2 connects to CN6.6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4, PA0, PA1, PA4 and PB0. Do not interchange two-pin power and sensor headers. Label harnesses before applying power.

USART2 TX uses PA2 alternate function 7. The normal MB1136 ST-LINK virtual-COM arrangement requires SB13 and SB14 fitted and SB62 and SB63 open. The code and pin map follow the existing jayis1 platform and ST UM1724; a fresh ST manual download failed during this run, so confirm the actual board revision and solder bridges against the manufacturer manual before assembly. Register addresses are documented against STM32F401 RM0368. No vendor HAL or guessed cross-family peripheral driver is included.

## Electrical limits, power and bill of materials

The pullup and series resistor form a divider when a contact closes. At nominal 3.3 V and an ideal closed contact, current is 0.300 mA and the MCU node is approximately 0.300 V. Four closed channels draw approximately 1.200 mA from the Nucleo 3.3 V rail. These are circuit calculations, not measured supply or contact-current results. The development board and ST-LINK dominate USB consumption. Use a current-limited USB supply with sufficient headroom for the purchased board; verify actual current before enclosure work. There is no battery charger or independent carrier supply regulator.

| References | Quantity | Proposed procurement item |
| --- | --- | --- |
| R1, R3, R5, R7 | 4 | Yageo MFR-25FBF52-10K, axial 10 kohm, 1%, 0.25 W |
| R2, R4, R6, R8 | 4 | Yageo MFR-25FBF52-1K, axial 1 kohm, 1%, 0.25 W |
| J1, J3–J6 | 5 | Samtec TSW-102-07-G-S, straight 2.54 mm two-position header |
| J2 | 1 | Samtec TSW-104-07-G-S, straight 2.54 mm four-position header |
| External MCU assembly | 1 | ST NUCLEO-F401RE, MB1136 family |
| External encoder | 1 | Bourns PEC11R-1015F-N0024, no-detent, no push switch |
| External reeds | 2 | Littelfuse MDSR-7-10-15, normally open, protected in nonconductive holders |
| Mechanical hardware | As required | Two shaft supports, light coupling, captive crank, follower pivot, magnets, guards |

These are proposed part selections, not claims of current stock, supplier quotation or verified assembled fit. Resistor footprints are axial DIN0207 with 7.62 mm lead spacing. Header footprints use 2.54 mm pitch. Confirm manufacturer mechanical drawings before ordering. Sensors, coupling and MCU are external assemblies and therefore appear as connectors on the carrier schematic, not as fictitious on-board ICs. The KiCad project contains the actual passive carrier and no hidden regulator or microcontroller.

The freshly retrieved Bourns PEC11R datasheet specifies two-bit quadrature, contact bounce at its stated 15 RPM test condition, and recommends noise reduction filtering. It also says hand soldering is not recommended. Use a professionally assembled encoder breakout or a manufacturer-compatible assembly process; do not assume direct hand-soldered wires are validated. This design uses software qualification rather than the manufacturer's suggested hardware filter, and that substitution requires bench characterization. Its 0.3 mA contact current likewise needs assembled reliability testing. A contact maximum rating does not establish minimum wetting-current performance.

The carrier is not isolated and contains no TVS, fuse or reverse-polarity protection. USB ground is shared with the experiment. Use short leads, initially no longer than 0.5 m, in a dry indoor bench setting. Never connect driven outputs, energized machine switches, mains, inductive coils or externally biased contacts. The 1 kohm resistor is not a universal overvoltage protector. Disconnect power for rewiring and keep the PCB in a nonconductive enclosure with strain relief. Environmental, ESD and EMC compliance are explicitly unverified.

## Mechanical fixture and setup

Build a rigid base approximately 220 by 140 mm with two bearing supports for a freely turning specimen shaft. The suggested size is an assembly envelope, not a fabricated drawing tolerance. Mount the test gear between the supports, retain it axially, and use a small captive hand crank outside the guarded gear perimeter. Couple the encoder coaxially using a light flexible coupling so its shaft does not carry gear radial load. Keep fingers away from meshing parts and use no second driven gear during initial characterization. The encoder's friction and coupling compliance can affect the experiment and must be assessed independently.

Mount one small magnet on a removable index collar fixed to the same shaft. Mount the index reed in an adjustable nonconductive bracket. The collar must not slip relative to the specimen during a session. There must be exactly one index closure per physical turn, with a sufficiently wide clear interval between closures. Verify this by manually making one complete turn while watching the raw input, and repeat in both directions. Firmware checks encoder transitions but cannot detect all coupling slip or a misleading index installation.

Use a low-force pivoted follower whose rounded tip follows the gear crests and valleys. Put a small magnet on the follower arm and mount the tooth reed so the contact closes at a crest and opens fully in a valley. The reed must not bear mechanical load. Protect its glass body and use a separate return spring. Keep the follower tip broad enough not to snag, but narrow enough to return completely between the smallest intended teeth. Adjust overtravel with a mechanical stop rather than bending the reed leads. Begin with broad, slow teaching-wheel features, not fine machine gears.

Place the index window inside a tooth valley so TOOTH stays raw and qualified low throughout that window. This is an intentional excluded boundary region. A pulse straddling the boundary cannot be assigned confidently to one capture and is rejected. Keep A/B stable around the qualified index rising endpoint as well; otherwise a delayed encoder transition could cross the cut. Either rotation direction may be used, but choose one and hold it for each capture. Reversal invalidates that capture even if later forward motion restores the same net encoder count.

Start with about five RPM or slower, and confirm every encoder state and tooth high/low plateau is comfortably longer than the five-sample filter window. This is a starting operating recommendation, not a guaranteed speed rating. Tooth geometry, contact bounce, magnetic hysteresis and sampling phase set the usable speed. A plateau shorter than the qualification window can disappear entirely. Five consecutive 1 ms samples span four nominal intervals; do not describe the filter as proving five milliseconds of continuous electrical stability. Stop and rework the fixture if pulses are marginal.

## Firmware behavior and design decisions

The implementation is freestanding C11 with startup vectors, a linker script, register definitions, GPIO configuration, SysTick scheduling, watchdog startup and a bounded polled UART. It uses the internal HSI rather than an assumed external crystal. Clock-restoration waits are bounded before SysTick starts. Failure prevents normal interface startup. A hardware input-configuration check verifies digital input modes, external-bias settings and transmit-only USART configuration. Watchdog initialization failure produces a terminal fault rather than normal-looking acquisition.

Three filter groups run independently: A/B as a two-bit bundle, INDEX alone, and TOOTH alone. Bundling A/B avoids constructing a quadrature state from independently delayed bits; separating the other signals prevents tooth chatter from postponing the index observation. The engine initially seeks a qualified low index followed by a rising passage. An index held high at startup is not a passage and cannot seed a turn. On a clean rising boundary it starts an ACTIVE capture with zero edge and pulse counters.

During ACTIVE, every legal A/B transition establishes or agrees with the direction. An opposite transition rejects the capture as a reversal. A two-bit jump rejects it as ambiguous quadrature. More than ninety-six transitions rejects it immediately; fewer than ninety-six at the next qualified index passage rejects it as an incomplete or misconfigured turn. Exactly ninety-six with a clean tooth-low boundary completes an accepted capture. Either sign is allowed for the initial direction. The count is a wiring-defined direction, not a claim of clockwise orientation from an unspecified viewing side.

A tooth pulse is counted only after a qualified rise followed by a qualified fall during the capture. Counting completion rather than rising edges makes the boundary condition explicit. A still-high pulse at the index is rejected; a pulse transition while INDEX is raw or qualified high is also rejected. A short raw tooth disturbance at the exact index cut prevents acceptance even if it never qualified. Electrical disturbances between samples remain undetectable, and long bounce can resemble multiple pulses. The result is observed qualified pulse count, not independently proven physical tooth count.

The first accepted capture establishes a volatile reference, even when its pulse count is zero. Zero is valid evidence of no observed responses, not a successful gear inspection. Later counts update minimum, maximum, total, latest count and latest duration. Mismatch counts compare only accepted captures to that immutable reference. A clean completion seeds the following capture on the same index passage, supporting rolling observation. A clean boundary with the wrong encoder count rejects the old capture and can seed a new one. Other same-sample rejection paths do not immediately restart. Subsequent clean index passages can recover ordinary rejected captures.

A 120000-sample age limit rejects an unfinished capture. The rejecting deadline wins over a completion on that exact sample. Any acquisition timestamp gap, repeated timestamp or unexpected interval permanently halts measurement; advancing status timestamps after HALT do not resume acquisition. Normal 32-bit timer wrap is supported by unsigned subtraction. Configuration, watchdog and counter-limit faults also halt. A stopped capture remains accounted for as rejected, maintaining starts = accepted + rejected + active. Sequence is bounded at one billion samples to prevent cumulative measurement arithmetic from wrapping. Reboot explicitly starts a new experiment and clears volatile history.

## Protocol and application interface

Firmware emits a snapshot approximately every 100 ms when its UART queue is free. Frames begin `TC1,`, contain twenty-four unsigned decimal fields in the order listed below, and end with `*`, four uppercase hexadecimal CRC digits, and newline. CRC is CCITT-FALSE, polynomial 0x1021, initial value 0xffff, over bytes before the asterisk. There is no command receiver, configuration write, update endpoint or network authentication surface. CRC detects accidental corruption; it does not authenticate an adversarial serial source.

    sequence now state fault raw qualified valid starts good bad mismatch reference
    minimum maximum total last last_ms reason steps teeth age direction tx_errors gaps

State values are SEEK=0, ACTIVE=1 and HALT=2. Validity bits describe the three filter groups, not four physical channels. Direction is unknown=0, positive=1, negative=2. Rejection reasons are none=0, reversal=1, quadrature jump=2, edge count=3, boundary=4, timeout=5, acquisition gap=6 and platform fault=7. Fault bits are configuration=1, watchdog=2, acquisition gap=4 and counter limit=8. The marker fixes encoder resolution and qualification policy; incompatible future policy requires a different protocol version, not silent app-side reinterpretation.

Run `python3 app/workbench.py` from this directory on a Linux desktop with Tk available. The Gear comparison tab shows accepted/rejected counts, learned reference, count range, mismatches and pending capture status. The Snapshot notebook tab shows retained telemetry rows. Setup and limits explains wiring and interpretation. Connect accepts an explicit serial path and refuses a second connection until disconnected. The application does not transmit bytes or change experiment parameters. Reboot the target and start a new host session to compare a different gear.

Replay loads a bounded regular TC1 file and is always labeled replay. The parser checks framing, CRC, field ranges, trial conservation and aggregate relationships. Session checks reject counter regression, changed references, unexplained result changes and impossible sparse duration growth. These are necessary consistency bounds, not reconstruction of every hidden contact event. Malformed serial fragments are counted and the decoder resynchronizes at the complete marker. A partial tail is an error in a recording. Empty files, device files, FIFOs and files above two MiB are refused before unbounded allocation or blocking reads.

The application retains at most 512 snapshots. Export creates a new owner-readable CSV without overwriting an existing path and includes jayis1 attribution, provenance, connection state, advancing status, parser-error count and warning text. Disconnect preserves the current evidence. A failed replacement connection or replay preserves the previous session. Advancing status requires distinct forward records, a recent receive time and no terminal fault. A receive pause or cumulative host/device drift beyond the conservative allowance blocks the session until reconnect; duplicates do not refresh freshness. This is not proof of absolute live latency or boot identity. Some MCU resets can resemble forward traffic because TC1 has no persistent boot identifier.

## Build, validation and physical acceptance plan

The tested toolchain is GNU Arm Embedded GCC 14.2.1 with GNU make, host GCC with address/undefined-behavior sanitizers, Python 3.13.5 with Tk, and KiCad 9.0.2. No pip or Node dependencies are needed. Build with `make -C firmware all`; run host firmware tests with `make -C firmware test`; generate the explicitly synthetic recording with `make -C firmware fixture`. Open it with `python3 app/workbench.py --replay firmware/build/demo.tc1`. Run the complete verification entry point using `python3 tools/verify.py`. Build outputs and KiCad local preferences are ignored.

Hardware generation uses the installed KiCad Python module through `/usr/bin/python3 tools/generate_hardware.py`. It writes actual local symbols, wired schematic nets, matched footprints, routed copper, board outline and project metadata. `tools/check_hardware.py` runs native ERC and DRC, exports a fresh schematic XML netlist and compares every reference, pin, value, footprint and net with the PCB and connectivity manifest. A passive-only ERC cannot validate the external Nucleo supply or harness. Zero CAD violations is not fabrication or electrical qualification.

Before any physical use, inspect all resistor values and harness polarity with power removed. Confirm no unintended continuity between 3.3 V and ground. Power the Nucleo alone, then the carrier, and measure supply current and each open/closed input voltage. Flash only the intended NUCLEO-F401RE using `make -C firmware flash` with a correctly configured ST-LINK/OpenOCD installation; that command is provided but has not been exercised on hardware here. Verify the actual image, reset behavior, serial timing and input mapping. Then test a known electrical stimulus before coupling the gear.

Physical acceptance requires repeatable ninety-six encoder transitions per turn, one index passage, clearly separated tooth pulses, no bracket slip and no excessive follower loading. Compare at least several manual turns against an independent count and video, deliberately introduce a reversal, obstruct one follower return, and test both index-boundary directions. Unplug the USB connection and confirm the app stops indicating advancing acquisition. Compare unloaded and instrumented mechanism behavior. Keep all findings, including rejected runs; the delivered synthetic tests are not a substitute for these measurements.

## References and evidence limits

- ST UM1724, STM32 Nucleo-64 MB1136 user manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Bourns PEC11R manufacturer datasheet, retrieved during this run: https://www.bourns.com/docs/product-datasheets/pec11r.pdf
- Established gear sensing alternatives: https://sensorso.com/gear-detection-sensors.html
- Established optical inspection alternative: https://www.evt-web.com/products/ready-to-use/gear-checking/

See VALIDATION.md for executed results. Electrical and mechanical assumptions are stated explicitly so a future builder can reject the design if those assumptions fail, rather than inheriting an unsupported claim of a finished measuring instrument.
