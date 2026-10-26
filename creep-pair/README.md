# Creep Pair — paired compression-and-recovery notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is an engineering prototype, not a material-testing certificate. The supplied custom PCB is a routed passive resistive-sensor interface for a separately purchased NUCLEO-F401RE. The MCU, voltage regulator, decoupling, debugger and USB bridge are on that development board. Firmware has a real target build and host tests; the application has simulated serial and GUI tests. No fabricated assembly, flashed target, displacement calibration, repeatability study or physical foam experiment is claimed.

## Purpose and overview

Creep Pair helps a small packaging workshop compare two cushioning offcuts under removable dead weights. A person making a reusable instrument case wants to know whether one candidate slowly collapses under a supported object and whether its thickness returns after that object is removed. A finger squeeze is not repeatable. A ruler and stopwatch are workable, but recording two samples at comparable elapsed times while lifting weights is awkward. The proposed instrument supplies two synchronized displacement channels, independent load-presence witnesses, and a local record of displacement at one, ten and sixty seconds after loading and after unloading.

The primary user is a packaging prototyper comparing noncritical foam inserts, not a mattress certification laboratory. A concrete trial compares equal-size coupons from two candidate sheets using equal removable masses, equal follower preload and the same conditioning history. Each lane has its own time origin, so the operator does not have to place both weights simultaneously. The result is a paired record, including incomplete and invalid trials, rather than an unexplained single quality score. The measurable benefit to evaluate in a workshop trial is fewer missed timestamped readings than a manually transcribed two-coupon experiment. That benefit is a hypothesis, not a measured productivity claim.

The novelty proposed here is the combination of a small paired manual fixture, independent load timing, explicit ambiguity rejection and late-attachable evidence snapshots. It does not invent creep testing. It also does not claim that no equivalent instrument exists anywhere. A phone alone cannot read two obscured plunger positions and the two physical weight-seat contacts without additional sensing. The dedicated controller keeps acquisition timing independent of desktop scheduling; the companion app handles comparison and local exports without cloud accounts.

## Originality and neighboring designs

The live repository directories and README introductions were inventoried before selecting this concept. The closest repository comparisons are:

| Existing device | Measurement and user | Difference here |
| --- | --- | --- |
| QuadraAlign | Four-foot load distribution and leveling of supported machines | Creep Pair follows specimen displacement under deliberately imposed bench loads; it measures neither force nor level |
| Nest Settle | Contact seating and corner dropouts in a locating nest | Creep Pair records analog deformation and recovery, not whether a workpiece touches locating points |
| Wick Escape | Conductive liquid arrival through or around paper barriers | Only the passive electrical platform and board support are reused; no wetting, electrodes or barrier classification exists here |
| HingeScribe | Door hinge motion, sag and operating health | This fixture compares coupons, not installed doors or hinge condition |

External prior art is substantial and useful. The Polyurethane Foam Association describes creep as thickness change under constant loading and distinguishes recovery and compression-set tests. Shimadzu describes force/displacement-controlled foam tests with specified prestressing and dwell periods. Rogers explains why test conditions and calculation conventions matter when comparing compression-set figures. Creep Pair deliberately lacks force feedback, controlled temperature and certified dimensional metrology. Its short bench experiment must not be labeled ASTM D3574 compliance, compression set under that standard, or a predictor of shipping damage.

References consulted for the design and its limitations:

- https://www.pfa.org/wp-content/uploads/2022/05/InTouchv4.2new.pdf
- https://www.ssi.shimadzu.com/sites/ssi.shimadzu.com/files/pim/pim_document_file/ssi/applications/application_note/16850/MT-006-ASTM-D3574-Foam.pdf
- https://www.rogerscorp.com/blog/2024/everything-you-need-to-know-about-compression-set-for-elastomeric-foam-materials
- Bourns PTA mechanical and electrical data: https://www.bourns.com/docs/Product-Datasheets/pta.pdf
- ST Nucleo connector and virtual-COM manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 register reference: RM0368, with register definitions collected in firmware/registers.h.

## Hardware specifications

| Function | Revision A |
| --- | --- |
| Processor | STM32F401RET6 on NUCLEO-F401RE, Cortex-M4 |
| Clock and memory | Nominal 16 MHz internal HSI; 512 KiB flash; 96 KiB SRAM |
| Analog sensing | Two externally mounted Bourns PTA4543-2015CPB104 linear 100 kohm slide potentiometers, used as rheostats |
| Load witnesses | Two external low-current, normally-open dry contacts beneath removable mass seats |
| ADC | ADC1, 12 bits, four sequential channels, discard plus eight-sample mean |
| Sampling | Nominal 20 ms sample interval; observed gaps outside 20–25 ms invalidate both trials |
| Connectivity | USART2 transmit-only, 115200 baud, 8N1, onboard ST-LINK virtual COM |
| Power | Nucleo USB; passive PCB receives only Nucleo 3.3 V and ground |
| PCB | 80 by 70 mm, two copper layers, 1.6 mm nominal FR-4 |
| Components | Eight axial resistors, five two-pin headers, one four-pin header |
| Companion | Python 3 / Tk desktop application for Linux; no pip packages required |

The two slide sensors are external fixture components, not footprints silently omitted from the carrier. Each sensor is wired between its signal and ground. Identify the two end terminals and wiper from the manufacturer's drawing and confirm them with an ohmmeter. Tie the wiper to the unused end terminal so an intermittent wiper tends toward the full track resistance rather than a completely floating input. This does not create complete fault coverage: a damaged sensor can still produce a plausible resistance.

## Architecture and block diagram

    Specimen follower -> rheostat A -> J3 -- 1k -- SENSE0 -> PA0 / A0
    Reference follower -> rheostat B -> J4 -- 1k -- SENSE1 -> PA1 / A1
    Removable mass A -> dry contact -> J5 -- 1k -- SENSE2 -> PA4 / A2
    Removable mass B -> dry contact -> J6 -- 1k -- SENSE3 -> PB0 / A3
                                                    |
                                           100k per input to 3V3

    USB -> NUCLEO-F401RE -> ADC averaging -> independent lane engines
                        -> bounded CRC encoder -> UART -> ST-LINK VCP
                        -> Linux live view / comparison / evidence export

J1 pin 1 is 3.3 V from Nucleo CN6 pin 4; J1 pin 2 is ground from CN6 pin 6. J2 pins 1–4 connect to CN8 pins 1–4, corresponding to PA0, PA1, PA4 and PB0. J3 through J6 have signal on pin 1 and ground on pin 2. The firmware uses ADC channels 0, 1, 4 and 8 in that same order. USART2 uses PA2 alternate function AF7. Verify the board revision and default ST-LINK virtual-COM solder bridges against UM1724 before wiring. Receive is disabled; the app cannot command an actuator or change firmware policy.

The nominal conversion is external resistance = 100000 times ADC / (4095 minus ADC), minus 1000 ohms. Negative low-end results clamp to zero; full-scale returns an unavailable sentinel. This corrects the divider's nonlinear voltage response but does not calibrate resistor tolerance, ADC error, contact resistance, potentiometer taper or mechanics. Analog readings outside 80–2300 counts invalidate the experiment. Those bounds intentionally exclude the shorted and open extremes; they are not a complete sensor diagnostic.

## Mechanical fixture and measurement limits

Build two independent vertical follower stations on an insulating base, with a planning envelope of 250 by 180 mm. Each station holds a nominal 25 by 25 mm foam coupon, approximately 20 mm thick, beneath a flat 25 by 25 mm follower platen. Use smooth guided plungers and hard travel stops so the linkage cannot crash into the sensor's end stops. Keep the removable masses captured in shallow cups with side walls. Limit the initial prototype to a 200 g removable mass per lane, plus a nominal 100 g follower assembly. Do not stack loose weights or put fingers beneath a raised platen.

Mount each 45 mm slide sensor horizontally, outside the load path. A pivoted linkage should move the sensor one unit for five units of follower travel. An example lever has a 50 mm follower radius and a 10 mm sensor radius, operated near its centered position; use articulated links, not rigid side-loading. The ratio is an approximation that must be calibrated over the actual small travel range. Set the unloaded sensor near 30–50 kohm with adjustment slots, and orient increasing compression to increase resistance. Mechanical stops should keep the useful experiment away from either track endpoint.

The Bourns data specifies operating force of 30–250 gram-force. This is not a precision low-force displacement transducer. A five-to-one motion reduction reduces the reflected sliding force, but does not remove friction, linkage hysteresis or follower preload. Recovery in this fixture means recovery under the remaining follower and sensor loading, never unloaded free recovery. Before interpreting any foam result, replace the coupon with a rigid spacer, cycle the follower by hand and measure repeatability and hysteresis with an independent dial indicator. If drag is a significant fraction of the coupon response, abandon this sensor arrangement or redesign the mechanics rather than correcting the result with an invented constant.

The supplied PCB has no mounting holes; retain it in an insulating edge-support tray with cable strain relief. Keep the Nucleo next to it in a roughly 110 by 100 by 45 mm electronics enclosure allowance. No certified enclosure fit or ingress rating is claimed. The application-specific mechanical fixture is dimensioned here but has no fabrication drawing or validated bearing selection. The hard deliverables are the electronics, software and experimental contract; physical fixture qualification remains a prototype task.

## Power budget, BOM and safety

Reserve a 5 V, 500 mA USB source budget for the Nucleo and debugger. This is a supply allowance, not a measured consumption claim. There is no battery, charging circuit, radio or power-management runtime estimate. The MCU polls continuously, so this is not an energy-harvesting or long-life battery instrument. Carrier excitation is bounded by four 101 kohm paths: at nominal 3.3 V the maximum total is approximately 0.131 mA. A shorted input produces about 40.5 ideal ADC counts. Resistor tolerances and actual rail voltage must be checked during bring-up.

| Item | Quantity | Procurement/assembly note |
| --- | --- | --- |
| NUCLEO-F401RE | 1 | Actual MB1136 assembly, including ST-LINK |
| 100 kohm axial resistor | 4 | 1%, quarter-watt, 7.62 mm lead pitch |
| 1 kohm axial resistor | 4 | Same footprint family and tolerance |
| Two-pin 2.54 mm header | 5 | J1 and J3–J6 |
| Four-pin 2.54 mm header | 1 | J2 |
| PTA4543-2015CPB104 | 2 | External linear 100 kohm, 45 mm slide sensors |
| Signal-rated dry contact | 2 | Low-force mass-seat witness; choose for low wetting current |
| Passive PCB | 1 | Routed two-layer design supplied |
| Guided followers, levers, cups, base | 2 stations | Fabrication and friction characterization required |

A quantity-one planning budget is USD 80–160 including the development board, sensors, interface and simple fixture materials. At one hundred assemblies, a preliminary allowance is USD 60–120 each while retaining the purchased development board. These are budgeting assumptions, not current quotes, and machining labor may dominate. The manual incumbent is a ruler, timer and notebook; a professional force-controlled tester is not a price-equivalent substitute and provides capabilities this design lacks.

Keep leads under half a meter during initial tests, use only disconnected passive sensors and contacts, and never connect powered industrial outputs. USB shares ground with the computer. There is no isolation, TVS network, industrial transient qualification or mains protection. The series resistor is not an overvoltage safety barrier. This device is not for medical cushioning approval, fall protection, load-bearing structures or acceptance of safety-critical packaging. Do not infer force from the contact state: the contact witnesses the mass seat, not mass value or actual applied stress.

## Firmware details and design decisions

The target build is freestanding C11 with explicit startup vectors, data initialization and linker regions for the STM32F401RE. No SDK shim or host-only substitute is used for the target image. Clock setup restores HSI and prescalers before peripheral configuration. ADC acquisition uses the longest programmed sampling interval, discards the first conversion after multiplexer selection, then averages eight conversions. Conversion waits and clock waits are bounded. The watchdog uses LSI with bounded readiness checks. The UART has a bounded 512-byte transmit buffer and a 100 ms transmit timeout.

Each lane starts in BASELINE. Its contact must qualify open, and fifty samples of nominal resistance must fit a 500 ohm peak-to-peak band before READY. Drift restarts this collection; no software automatically retare-adjusts an already established baseline. The tolerance is deliberately coarse and is not a promised displacement resolution. With the reduction linkage, even a small resistance variation can correspond to substantial travel; characterize it before relying on comparisons.

Contacts qualify independently using five consecutive samples, with closed at or below 200 counts and open at or above 3800. Values in between reset qualification. A qualified loading transition starts LOADED at zero elapsed time. Loaded resistance is captured at the first accepted sample at or after one, ten and sixty seconds. Release must occur after all loaded milestones and no later than 180 seconds. A release whose raw qualification overlaps a milestone invalidates the lane rather than recording unloaded displacement as loaded evidence. The release sample does not earn a missing loaded milestone.

After valid release, RECOVERY records the same three time points. A new raw loading indication or ambiguous contact qualification invalidates recovery conservatively. At sixty seconds the lane becomes DONE and freezes its final resistance. An early release, excessive hold or reload invalidates that lane; ADC, timing, UART or watchdog faults invalidate both. Results do not automatically restart: remove the weights, export evidence, reset the Nucleo deliberately and reconnect for another pair. No flash writes are performed and reset loses volatile trial data.

The firmware stores nominal ohms, not calibrated millimeters. The app subtracts the unloaded baseline and divides by an operator-supplied signed ohms-per-millimeter scale. Its starting scale is only an editable assumption. Calibrate both complete mechanical channels using several known displacement spacers; the simple app currently uses one common scale, so unequal channel sensitivities require comparison in raw ohms or an externally documented per-channel analysis. Do not present the app's four displayed decimal places as accuracy.

## Application/software interface

Run `python3 app/creep.py` from this folder on Linux with Tk installed and serial-port permission. The Connection / live screen opens a chosen device path at 115200 baud, flushes queued input and shows both lane states, sample validity and fault flags. The Comparison screen accepts the displacement scale and reports completed residuals and A-minus-B when both lanes complete. The Evidence screen retains up to 2,000 snapshots, displays the latest hundred and exports all retained snapshots with operator notes, provenance, error count, status, age and scale.

The protocol is ASCII CSV with a CRC-16/CCITT-FALSE suffix. A frame contains marker C1, sequence, device milliseconds, sticky fault mask, acquisition-valid flag, four ADC counts, then fourteen fields for each lane: state, qualified contact, baseline resistance, hold milliseconds, recovery milliseconds, loaded milestone mask, recovery mask, final resistance, three loaded resistances and three recovered resistances. A comma and four uppercase hexadecimal CRC digits precede LF. CRC covers all bytes before the final comma. The unavailable resistance sentinel is 4294967295. States are BASELINE=0, READY=1, LOADED=2, RECOVERY=3, DONE=4 and INVALID=5. Fault bits are range=1, sampling gap=2, ADC=4, UART=8 and watchdog=16.

Full periodic snapshots permit late attachment without requiring a boot banner. This does not prove session identity: a reset with plausible forward sequence and timestamps can remain indistinguishable. The app rejects contradictory duplicates, timestamp regression, disappearing sticky faults, regressing lane histories and changing established baselines or earned milestones. Stale live reception, a long host pause, and excessive host/device timing divergence freeze capture and require explicit reconnect. CRC detects accidental corruption, not malicious forgery; the USB port is a trusted local interface, with no remote configuration service.

Replay accepts only regular files up to two MiB, rejects malformed records and truncated tails, and labels retained data as replay rather than live hardware. Synthetic fixtures are generated by the C test encoder. Exports create a new private file and refuse overwrite. They contain a metadata record even when no measurements were captured. Disconnect retains evidence; successful reconnection clears it for the new session, so export first. A failed connection preserves the prior capture. Nothing is uploaded, persisted automatically or transmitted back to the MCU. Treat operator notes as untrusted text when importing CSV into spreadsheet software.

## Building, testing and bring-up

From this folder, run `make -C firmware all test` to cross-link the target and run host engine tests with address/undefined-behavior sanitizers. Run `xvfb-run -a python3 tests/test_app.py` for parser, replay, GUI, serial PTY, disconnect and export tests. Run `/usr/bin/python3 tools/check_hardware.py` for KiCad ERC, PCB DRC and exact schematic/PCB/manifest connectivity comparison. `python3 tools/verify.py` combines these checks with authorship and content-size gates. The hardware generator requires the system Python with KiCad pcbnew bindings; ordinary firmware and application builds do not regenerate CAD.

The documented flash command is `make -C firmware flash`, using OpenOCD's ST-LINK interface and STM32F4 target scripts. Flashing was not performed in this run. Inspect the PCB for bridges, verify power polarity and test input networks using fixed resistors before attaching fixture sensors. Confirm the four channel identities individually. Validate contact thresholds with a jumper, deliberately disconnect a rheostat, and confirm invalidation instead of a plausible final result. Then compare timestamps against an independent instrument and investigate oscillator error before making timing claims.

ERC on a passive carrier cannot validate the external MCU board, fixture sensor wiring or suitability of the mechanical follower. DRC confirms the authored copper meets configured geometric rules, not that manufacturing and assembly succeeded. The generated native schematic contains actual connected resistor and connector symbols, and the PCB contains matching library footprints, routed nets and an outline. All pad/net relationships are checked against a fresh schematic export. See VERIFICATION.md for the actual software and CAD results and remaining physical work.
