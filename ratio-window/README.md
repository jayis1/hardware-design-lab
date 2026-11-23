# Ratio Window — input-turn apertures for hand-cranked gear experiments

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software, tests, documentation and locally authored electrical design sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream library licensing. The acquisition platform, passive carrier generator and POSIX transport are explicitly reused from jayis1's Foot Rock at commit 99853fddaaf648c8ee5f573a069e02d46dbbee5f. The quadrature-window engine, RW1 protocol, comparison interface and experiment tests are specific to this device.

Revision A is an unbuilt engineering prototype. The supplied PCB is a routed passive carrier for an external NUCLEO-F401RE, not a standalone processor board. Target compilation, synthetic electrical tests, desktop application operation and electrical CAD checks have been exercised. Fabrication, programming a physical board, mechanical assembly, contact performance, timing accuracy and workshop usefulness remain unverified. Nothing here establishes industrial transmission-error accuracy, product safety certification or worldwide patent novelty.

## Purpose, audience and practical utility

Ratio Window helps an educator or mechanism designer investigate a slow hand-cranked gear train one input turn at a time. An encoder on the input shaft defines successive windows of ninety-six qualified transitions. A second encoder counts output motion inside each window. The instrument retains the latest output count, its direction relative to the input, the range across accepted windows, and separate rejected-window evidence. A reversal on either shaft, an ambiguous quadrature jump, an excessive window duration or an acquisition interruption is not silently folded into an average ratio.

The useful question is deliberately modest: when this input shaft advanced through one encoder-defined revolution without an observed reversal, how many monotonic output transitions were observed? A handmade compound gear train can feel irregular because of binding, coupling slip, clearance, assembly mistakes or intermittent operator reversal. Watching only the two final shaft marks hides much of the history. A handheld tachometer averages over time, while the user is changing crank speed. Ratio Window bounds its observations by input displacement instead, keeping variable-speed manual experimentation understandable.

A representative teaching exercise uses a guarded pair of coarse plastic gears. First compare nominal equal-size gears, then install a larger driven gear, and finally add an idler. The notebook should expose the changing output count and relative sign without requiring students to maintain a stopwatch or annotate every encoder edge. The output may alternate between nearby counts when a noninteger ratio crosses quantized boundaries; that is not automatically a faulty gear. Deliberately reverse the crank inside a window and observe a rejection rather than a net count that conceals the reversal.

The primary audience is a makerspace class, museum exhibit developer or small model-mechanism workshop. The target is low-energy manually moved fixtures, not installed industrial gearing. There is no motor, brake, relay, automatic adjustment or machine-enable output. Accepted means the acquisition met this software contract; it never means that the gear is acceptable, that all teeth are intact, or that the mechanism is safe. A stalled output can legitimately produce a zero count, but that result requires investigation rather than a green quality indication.

## Distinction and bounded originality assessment

The repository was pulled and its device directories and README titles inventoried before selecting the concept. The closest existing designs were read in detail. WebSlack Ledger compares two material-contact wheel positions and records reversal-indexed differential travel. Tooth Census counts follower pulses between index passages while enforcing a single encoder revolution. Ratio Window instead uses two independently qualified quadrature bundles, divides input motion into contiguous ninety-six-edge apertures, and records output magnitude and relative sign within those cuts. It has neither a material-travel scale nor a gear-tooth follower or physical index sensor.

Dual-encoder gear measurement is established technology. A bounded search found ROTEC's transmission-error measurement system and Siemens Simcenter gear-test-rig examples using encoders on both shafts. Those instruments address substantially richer and more precise measurements. This project does not claim to have invented shaft comparison. Its proposed contribution is a small, inspectable, low-cost manual teaching workflow with explicit rejection and boundary semantics, a local evidence notebook, and a reproducible passive interface. The usefulness of that combination must still be tested with real builders.

A logic analyzer can retain much richer electrical information than this notebook. High-resolution industrial encoders and synchronized acquisition are appropriate when angular transmission error matters. A camera may be preferable when the important uncertainty is shaft coupling slip. Ratio Window intentionally does not produce micrometre error, backlash angle, torque, efficiency, tooth geometry, speed certification or an automatic diagnosis. Do not substitute its count ratio for those measurements.

## Architecture and hardware specifications

    hand crank -> input shaft -> guarded specimen gear train -> output shaft
                      |                                          |
                24-pulse contact encoder                  24-pulse contact encoder
                      A/B                                        A/B
                       +---------------+--------------------------+
                                       |
                       four 10k pullups / four 1k series resistors
                           custom two-layer passive carrier
                                       |
                          PA0 / PA1 / PA4 / PB0 digital inputs
                            external NUCLEO-F401RE module
                                       |
                       independent A/B qualification, quadrature decode
                          contiguous input-turn window accounting
                                       |
                            USART2 TX / ST-LINK USB VCP
                                       |
                         Linux Tk comparison, notebook and CSV

| Item | Revision A specification |
| --- | --- |
| Processor | STM32F401RET6 Cortex-M4 on NUCLEO-F401RE MB1136 |
| Clock | Internal HSI, nominal 16 MHz; no external crystal assumed |
| Target memory | 512 KiB flash, 96 KiB SRAM |
| Sensors | Two Bourns PEC11R-1015F-N0024, no-detent contact encoders |
| Encoder resolution | 24 quadrature pulses, 96 transitions per nominal revolution |
| Acquisition | Nominal 1 ms polling, five consecutive samples per two-bit bundle |
| Communication | Transmit-only USART2, 115200 baud, 8N1 through ST-LINK VCP |
| Carrier | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Power | Nucleo USB; carrier powered from Nucleo 3.3 V |
| Application | Linux Python 3 with Tk; no pip, Node, network or cloud dependency |

An FPGA and high-speed radio would add cost without improving this deliberately slow experiment. The MCU module supplies regulation, reset, decoupling, debug access and USB bridging. Those circuits are not falsely represented as components on the custom carrier. The delivered schematic describes the actual fourteen-component passive board, and the PCB contains footprints and routed nets for those components.

## Wiring and electrical constraints

| Carrier connector | Signal / MCU destination | External connection |
| --- | --- | --- |
| J1 pin 1 | VCC, Nucleo CN6 pin 4 | 3.3 V only |
| J1 pin 2 | GND, Nucleo CN6 pin 6 | Shared ground |
| J2 pin 1 | SENSE0, CN8 pin 1, PA0 / A0 | Input phase A |
| J2 pin 2 | SENSE1, CN8 pin 2, PA1 / A1 | Input phase B |
| J2 pin 3 | SENSE2, CN8 pin 3, PA4 / A2 | Output phase A |
| J2 pin 4 | SENSE3, CN8 pin 4, PB0 / A3 | Output phase B |
| J3 pin 1 | INPUT_A | First encoder A contact |
| J4 pin 1 | INPUT_B | First encoder B contact |
| J5 pin 1 | OUTPUT_A | Second encoder A contact |
| J6 pin 1 | OUTPUT_B | Second encoder B contact |

Every J3–J6 pin 2 is ground. Connect each encoder common terminal to ground. The encoder contacts are excited only by this board's pullups; they must not remain attached to another controller. Square pads identify connector pin 1. J1 is a power connector, not a phase input. Label the harness and verify polarity with power removed before attaching the Nucleo.

ST UM1724's NUCLEO-F401RE connector table confirms this A0–A3 mapping. Its virtual-COM description specifies SB13 and SB14 closed, with SB62 and SB63 open. USART2 TX uses PA2 alternate function 7. Verify the actual purchased board revision and solder-bridge arrangement before powering or flashing it. The register definitions follow STM32F401 RM0368; they are not generic addresses for arbitrary STM32 families.

Each channel uses a 10 kohm pullup to 3.3 V and a 1 kohm series resistor between the MCU sense node and the encoder contact. For an ideal closed contact, the calculated current is 0.300 mA and the sense voltage is 0.300 V. Four closed channels draw 1.200 mA in the resistor network. These are nominal circuit calculations, not measured contact performance or total system consumption. USB consumption is dominated by the development board. Reserve appropriate supply capacity according to its manual and measure actual current during bring-up.

The carrier has no galvanic isolation, TVS protection, fuse, reverse-power protector or industrial input conditioning. Series resistance does not make it safe to connect arbitrary voltages. Use only disconnected voltage-free contacts, short indoor leads and a common low-voltage ground. Start with leads shorter than half a metre and keep them away from motors and inductive wiring. Never attach energized machinery, mains switches, 24 V outputs, externally biased contacts or an unknown electronic encoder variant.

## Bill of materials and physical construction

| References | Quantity | Selected part or specification |
| --- | --- | --- |
| R1, R3, R5, R7 | 4 | Yageo MFR-25FBF52-10K, 10 kohm, 1%, axial |
| R2, R4, R6, R8 | 4 | Yageo MFR-25FBF52-1K, 1 kohm, 1%, axial |
| J1, J3–J6 | 5 | Samtec TSW-102-07-G-S, two-position 2.54 mm header |
| J2 | 1 | Samtec TSW-104-07-G-S, four-position 2.54 mm header |
| External processor module | 1 | ST NUCLEO-F401RE |
| External encoders | 2 | Bourns PEC11R-1015F-N0024 |
| External mechanics | Set | Independent shaft bearings, flexible couplings, crank, guards, base and brackets |

Part choices are a procurement specification, not a stock or price claim. Check current mechanical drawings before ordering. The resistor footprint is DIN0207 with 7.62 mm lead spacing. The carrier uses 0.30 mm tracks, 0.20 mm nominal clearance and one 0.80 mm through-via with a 0.40 mm drill. Use an insulating enclosure with edge retention; there are no mounting holes, and drilling through the released layout is not supported. The encoders mount off-board on manufacturer-compatible assembled breakouts, not directly into the carrier.

The indexed Bourns datasheet identifies the two-bit quadrature output, no-detent option and 24-pulse resolution. It specifies bounce under a particular test condition and recommends noise reduction filtering; indexed older documentation also warns against hand soldering. Revision A uses software qualification rather than claiming equivalence to the manufacturer's analog filter circuit. Low-current contact reliability and the selected assembly method require physical evaluation. A maximum contact-current rating does not establish a minimum reliable wetting current.

A suitable initial fixture uses a rigid base roughly 250 by 180 mm, two independently supported specimen shafts, a light removable gear pair and transparent guards over the meshing region. Couple an encoder coaxially to each shaft using a low-compliance flexible coupling. Do not hang gear loads from encoder bearings. Keep the crank outside the guard and retain all rotating parts axially. The dimensions describe a construction envelope, not a released machined-part tolerance drawing. Adjust shaft spacing to the chosen gear geometry and independently inspect free rotation before coupling sensors.

Encoder drag can change the mechanism being studied. Compare the fixture with and without encoders before interpreting counts. Mark each coupling and inspect for slip. Begin around five RPM or slower on the faster shaft, and verify every raw quadrature plateau is comfortably longer than the qualification interval. This is a conservative starting recommendation, not an established maximum speed. Input rate alone is insufficient: a speed-increasing train can overspeed the output sensor while the crank remains slow.

## Firmware and exact observation contract

The firmware is freestanding C11 with its own startup, linker script, register map, clock restoration, GPIO setup, watchdog and bounded polled UART. There is no hidden HAL generation step, heap allocation, RTOS or downloaded binary dependency. Clock waits are bounded before normal board initialization. A configuration check verifies input modes and transmit-only serial configuration. Failed platform initialization produces a halt rather than trustworthy-looking measurement data.

Each encoder's A/B word is qualified as a bundle independently of the other encoder. Five consecutive identical samples qualify a word; they span four nominal intervals and do not prove continuous analog stability between samples. Bundling avoids synthesizing a code from separately delayed phase bits. Independence prevents chatter on one encoder from indefinitely postponing a clean transition on the other. The positive sequence is 00, 01, 11, 10, 00 in the logical closed-contact bits. Physical clockwise direction depends on mounting and A/B wiring.

Startup and ordinary rejection enter WAIT. Both raw and qualified words must agree and remain unchanged over one hundred credited intervals before ACTIVE begins. Credit requires agreement at both endpoints; a short rejected raw disturbance restarts the quiet window. Rearm creates an arbitrary electrical origin, not a physical zero-angle reference. No index sensor is present. Within ACTIVE, the first transition on each shaft establishes its direction for that window. Opposite subsequent motion rejects the window even if the net displacement would later recover.

Ninety-six monotonic input transitions complete a window. Both qualified lane deltas are processed before evaluating that cut. An output transition qualifying on the same sample belongs to the ending window; no within-sample chronology is claimed. A new ACTIVE window starts immediately at the cut, so output movement during a pause in input motion is not dropped. Directions reset for each new window. Consequently, a reversal exactly between windows can establish the next window's opposite direction without an intra-window reversal rejection.

A two-bit quadrature jump rejects the pending window. A window reaching 120000 acquired milliseconds rejects before any same-sample completion. More than 4096 output transitions also rejects. Ordinary rejected windows discard only pending evidence and return to quiet rearm. Previously accepted results remain intact. Zero output transitions are permitted and classified separately from same-direction and opposite-direction results. All counts describe electrical observations; valid-looking overspeed aliasing, long bounce, coupling slip or disconnected contacts can still produce misleading results.

A missing or repeated acquisition timestamp permanently halts the session. Normal unsigned timer wrap is supported. The sample counter is capped at one billion; the following attempted sample halts before measurement counters can wrap. Platform faults abandon one active window exactly once, preserving starts = accepted + rejected + active. After HALT, acquisition fields freeze while wall timestamps and transmitter diagnostics may continue. A reboot resets volatile history; it is not an authenticated recalibration or a durable experiment identifier.

## Wire protocol and companion application

Snapshots are attempted approximately every 100 ms. The UART queues one bounded frame and advances without blocking acquisition; a transfer stuck for 100 ms is abandoned and counted. The next full marker lets the host recover framing. Each record is ASCII `RW1,` followed by these unsigned decimal fields in order:

    samples now state fault raw qualified valid starts good bad same opposite still
    minimum maximum total last last_ms last_input last_output steps output age
    input_dir output_dir reason quiet tx_errors

An asterisk, four uppercase hexadecimal CRC digits and newline finish the frame. CRC-16/CCITT-FALSE covers all bytes before the asterisk, using polynomial 0x1021 and initial value 0xffff. The standard `123456789` check produces 0x29b1. CRC detects corruption, not malicious senders. The firmware does not enable a command receiver, remote update endpoint or configuration write path. Programming remains a physical ST-LINK operation.

States are WAIT=0, ACTIVE=1 and HALT=2. Directions are unknown=0, positive=1 and negative=2. Rejection reasons are none=0, reversal=1, quadrature jump=2, timeout=3, capacity=4 and platform fault=5. Fault bits are configuration=1, watchdog=2, acquisition time=4, counter limit=8 and invalid raw input=16. The latest rejection reason remains retained after successful rearm; it is not a current alarm state. Validity bits refer to the two encoder bundles.

Run `python3 app/workbench.py` on a Linux desktop with Tk. The Turn windows screen shows pending progress, accepted/rejected totals, relative-direction counts, latest ratio and range. The Snapshot notebook lists every retained snapshot, up to 512 rows. Wiring and limits explains the experiment contract. The plot displays the latest result repeated across received snapshots, not a unique-turn time series. Faster completions can occur between telemetry transmissions; aggregate counters survive, but the notebook does not reconstruct every hidden result.

Connect accepts an explicit serial path and refuses a second port until disconnected. Replay accepts a bounded regular RW1 recording and is always labelled NOT LIVE. Export creates a new owner-readable CSV and refuses overwriting existing files. It includes author, provenance, connection state, status, corruption count, warning text and raw fields. Disconnect retains evidence; successful replacement starts a new session. Failed connection construction or replay preserves the prior evidence. No network permissions or credentials are needed.

The parser checks CRC, numeric bounds and accounting identities. Session checks reject regression, contradictory duplicates, impossible sparse completion rates, unexplained latest-result changes and halted evidence mutation. These are necessary consistency bounds, not reconstruction or authentication of hidden motion. Duplicates do not refresh freshness. A receive pause or accumulated host/device timing discrepancy beyond one second blocks live evidence until reconnect. The fixed allowance is deliberately conservative and can eventually reject a healthy but drifting oscillator. Without a persistent boot identifier, some resets can still resemble forward traffic; reset the device and reconnect explicitly for each new setup.

## Reproduction, verification and physical acceptance

From this directory, run:

    make -C firmware all test fixture
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py

The target toolchain is GNU Arm Embedded GCC; native tests use GCC with address and undefined-behavior sanitizers. The app uses Python's standard library and OS-provided Tk. KiCad generation and checking use the system Python with pcbnew. Generated binaries, caches and local KiCad preferences are ignored. To regenerate the carrier, run `/usr/bin/python3 tools/generate_hardware.py`. The import script documents initial platform provenance and deliberately refuses to overwrite an existing import.

The fixture file under firmware/build is explicitly synthetic. It exercises one same-direction equal-count window and one opposite-direction half-count window through the actual C encoder and Python reader. GUI tests use a real Tk event loop and Linux pseudo-terminal, visit all screens, verify CSV permissions, close the serial peer and replay retained data. They do not demonstrate physical USB operation. CAD checks run native ERC and DRC and compare every schematic pin, component value and footprint against actual PCB pads and the connectivity manifest. A rendered schematic was inspected for readability. Passive-only ERC cannot validate the external module, wiring harness or mechanics.

Before physical use, inspect resistor values, solder joints, connector polarity and supply isolation with power removed. Power the Nucleo alone, then connect the carrier and measure every open/closed sense level. Flash only the specified target, using the provided `make -C firmware flash` command after reviewing OpenOCD/ST-LINK access. That command has not been exercised against hardware here. Verify serial timing, watchdog recovery, actual ninety-six transitions per marked shaft revolution, and both direction signs independently before attaching the specimen.

Then compare known gear arrangements in both directions, deliberately reverse mid-window, disconnect a phase lead and turn faster than intended while capturing a reference logic trace. Confirm which faults are detected and which alias into plausible data. Compare marked physical turns against encoder-defined windows and evaluate quantization near cuts. Retain rejected observations and compare results before and after coupling changes. These physical acceptance steps remain outstanding; passing software tests does not substitute for them.

## References

- ST UM1724, MB1136 connector and virtual COM documentation: https://www.st.com/resource/en/user_manual/um1724-.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Bourns PEC11R series datasheet: https://www.bourns.com/docs/product-datasheets/PEC11R.pdf
- Established dual-encoder alternative: https://rotec-munich.de/en-gb/measurement-technology-applications_gearbox
- Gear-test-rig measurement context: https://blogs.sw.siemens.com/simcenter/what-type-of-measurments-are-possible-with-gear-test-rig-part-2/

Manufacturer manual and datasheet search extracts were available during this run; direct lowercase Bourns PDF retrieval returned HTTP 403. Confirm current complete documentation before procurement. See VALIDATION.md for executed results and remaining physical limitations.
