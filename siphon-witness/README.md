# Siphon Witness — two-vessel threshold-transfer recorder

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application and locally authored design sources: MIT.

Revision A is a build-tested engineering prototype, not a physically validated flow instrument. The custom PCB is a fully routed passive float interface. Its processor, debugger, USB interface, regulator and indicator live on a separate NUCLEO-F401RE. The carrier is not a standalone microcontroller board.

## Problem, purpose and audience

A small irrigation designer or teaching-laboratory technician can build a working siphon in minutes and still spend hours determining why nominally identical transfers behave differently. Priming can be delayed, tubing can admit bubbles, a receiver can be accidentally raised, and a float can snag on the wall. Watching the source vessel alone misses what happened at the receiver. Timing an entire pour with a stopwatch merges the delay before motion with the transit through the useful level band. A pump controller often hides this evidence by immediately attempting recovery.

Siphon Witness answers a deliberately narrower question: did water move monotonically through two marked level bands, and how long did each band traversal take? Four external dry reed float switches observe a lower and upper threshold in each of two vessels. The firmware qualifies the contact states, automatically arms when the source is full and the receiver is empty, records source-band drain time and receiver-band fill time, and distinguishes completion from a timeout, reversed level movement, impossible float ordering, skipped band or sampling interruption. It does not actuate a valve, start a pump, or claim that the hose contains a continuous column of liquid.

The primary user is a person comparing short siphon configurations on a supervised clean-water bench: changing hose diameter, bend radius, crest height, inlet position or priming technique while retaining the same vessels and threshold brackets. A secondary audience is an educator showing that source departure and receiver arrival are different observations. Hardware matters because the timing evidence originates at four physical heights, not in a phone interface. There is no account, camera, radio or cloud service. The onboard indicator offers standalone trial feedback; the desktop application is optional for numerical results and export.

## Differentiation and alternatives

The repository inventory was inspected before selecting this concept. DrainVeil and PipeWhisper address installed plumbing health through multi-modal sensing. FoldTrace records ordered jig contacts, ContactAtlas maps detached rotary-switch transitions, and WebSlack Ledger measures differential encoder travel. This proposal instead uses a physically constrained two-vessel topology: an upper submerged float requires its lower float to be submerged, source levels must fall, and receiver levels must rise. Its useful output is paired band-transit evidence with explicit invalid-trial reasons. The underlying passive resistor interface is intentionally reused as an electrical pattern; the measurement semantics are new.

A stopwatch and two observers remain the cheapest alternative. Video is richer but needs frame review, a visible scale, and an unobstructed view of both vessels. A generic logic analyser records contacts at higher speed but leaves threshold ordering, rearming, unknown intervals and the export interpretation to the user. Commercial float controllers generally make control decisions rather than preserve this particular paired-band experiment. None of those comparisons proves worldwide novelty.

A limited prior-art search for two-reservoir siphon float-switch timing returned primarily large-reservoir siphon engineering references, including the [CIRIA siphon design guide](https://www.ciria.org/ci/iCore/Store/StoreLayouts/Item_Detail.aspx?iProductCode=C813D&Category=DOWNLOAD) and [Simple Steps to Siphoning](https://www.damtoolbox.org/images/f/f2/Simple_Steps_to_Siphoning.pdf). These establish that siphon management is existing engineering, not a newly invented physical principle. Search coverage was limited; follow-up component queries returned no results. Treat originality as a differentiated bench-device proposal relative to the reviewed repository, not a patentability or market-exclusivity claim.

## Hardware specification and interface contract

| Item | Revision A contract |
|---|---|
| Processor module | ST NUCLEO-F401RE, STM32F401RET6 Cortex-M4, reset HSI 16 MHz |
| Program memory | 512 KiB flash, 96 KiB SRAM; no dynamic allocation in firmware |
| Sensing | Four independent voltage-free reed contacts, closed when submerged |
| Carrier | 80 by 70 mm, two copper layers, 1.6 mm FR-4 |
| Logic supply | 3.3 V from the Nucleo 3V3 pin, common ground |
| Data transport | USART2 TX PA2 through onboard ST-LINK virtual COM port |
| Serial framing | 115200 baud, eight data bits, no parity, one stop bit |
| Feedback | Nucleo LD2 on PA5; no field actuator outputs |
| Programming | Existing ST-LINK SWD; flash image starts at 0x08000000 |
| Application | Local Linux desktop, Python 3 and Tk; no pip packages |
| Environment | Indoor, supervised, nonconductive clean-water vessels |

The board pin assignment is fixed across the generator, connectivity manifest, firmware and this table:

| Bit | Vessel threshold | Field header | Carrier J2 | Nucleo Arduino pin | MCU pin |
|---|---|---|---|---|---|
| 0 | SOURCE_LOW | J3 pin 1 | pin 1 | A0 | PA0 |
| 1 | SOURCE_HIGH | J4 pin 1 | pin 2 | A1 | PA1 |
| 2 | RECEIVER_LOW | J5 pin 1 | pin 3 | A2 | PA4 |
| 3 | RECEIVER_HIGH | J6 pin 1 | pin 4 | A3 | PB0 |

Every field header pin 2 is ground. J1 pin 1 is 3V3 and J1 pin 2 is ground. Each SENSE input has a 10 kilohm pull-up to VCC and a 1 kilohm resistor to the corresponding field signal. Closing a reed therefore pulls the input down through the series resistor rather than shorting the supply. There are no electrodes in contact with water: the sealed float assembly provides galvanically separate reed contacts, but the electrical instrument itself is not isolated from the USB host. Do not connect a powered level transmitter, industrial PLC output, mains wiring or an unknown existing switch circuit.

References for board integration are ST UM1724, the MB1136 Nucleo-64 documentation, STM32F401 RM0368 for RCC/GPIO/USART/IWDG, and PM0214 for Cortex-M4 SysTick. The firmware uses these register families directly instead of claiming a dependency on an unprovided HAL. The current ST manual download could not be retrieved during this run; inspect the specific board revision and its ST-LINK USART solder bridges before physical assembly. An actual electrical bring-up remains necessary even though the target image links and CAD checks pass.

## Architecture and block diagram

```text
Source lower reed ---- J3 -- 1k -- SENSE0 -- A0 / PA0
Source upper reed ---- J4 -- 1k -- SENSE1 -- A1 / PA1
Receiver lower reed -- J5 -- 1k -- SENSE2 -- A2 / PA4
Receiver upper reed -- J6 -- 1k -- SENSE3 -- A3 / PB0
                                      |
                          each input: 10k to 3V3
                                      |
USB host -- ST-LINK -- NUCLEO-F401RE --+-- SysTick / qualifier
                         |                topology + trial engine
                         +-- PA5 LD2      CRC-framed serial snapshots
                                               |
                                      desktop live / replay / CSV
```

The schematic contains the complete custom carrier, including all resistors and headers. External vessels, floats, Nucleo and USB cable are system assemblies, not fictitious on-board parts. The generated symbol library embeds real two-terminal resistor symbols and numbered connector symbols; actual PCB pads carry the corresponding nets. The manifest is not treated as proof by itself: verification exports a fresh schematic netlist and compares every reference, pin, net, value and footprint against the loaded PCB and the manifest.

## Bill of materials and mechanical arrangement

| References | Quantity | Specification or procurement choice |
|---|---|---|
| R1, R3, R5, R7 | 4 | 10 kohm, 1%, axial DIN0207, 7.62 mm lead pitch, at least 0.125 W |
| R2, R4, R6, R8 | 4 | 1 kohm, 1%, same axial package and lead pitch |
| J1, J3 through J6 | 5 | 1 by 2 straight 2.54 mm pin header; matching insulated crimp harness |
| J2 | 1 | 1 by 4 straight 2.54 mm pin header; keyed/labelled harness strongly recommended |
| External MCU assembly | 1 | NUCLEO-F401RE, not a generic unspecified STM32 board |
| External level assemblies | 4 | Sealed reed float switches, voltage-free, closed when wet, low-current switching suitable for approximately 0.3 mA |
| Bench fixture | 1 set | Two nonconductive vessels, hose, four rigid threshold brackets and spill tray |

The external float procurement specification is functional rather than an unverified manufacturer part number. Select contacts suitable for low-energy service; a high-current float rating alone does not establish minimum reliable switching current. Verify material compatibility and normally-open orientation with a meter before connecting it. Some reversible floats change polarity when their moving element is inverted. The instrument has no software polarity option, so an incorrectly installed switch must be corrected mechanically rather than hidden in configuration.

Place each pair of thresholds far enough apart that a normal trial traverses each band in at least one second. Secure brackets to a rigid rail outside the flow jet. Use a stilling shield if necessary, but do not trap the float. Source and receiver threshold heights need not match; they represent different volumes unless the geometry and spacing are deliberately calibrated. The receiver should start below its lower switch and have enough freeboard beyond its upper switch for the remaining transfer. A complete electronic trial does not stop the water.

The 80 by 70 mm carrier has no mounting holes. Retain it in insulating edge guides or PCB rails, not loose on a metal bench. Provide separate strain relief for every cable and place the enclosure above the spill tray. A practical enclosure must also accommodate the separate Nucleo, approximately 100 by 140 mm internal floor area as an initial layout allowance; this is an enclosure recommendation, not a supplied injection-moulded mechanical design. Route short paired signal/ground leads, preferably below one metre. There is no external ESD suppressor or surge certification, so long building wiring and outdoor lightning exposure are excluded.

Planning cost, not a supplier quotation, is approximately USD 15–30 for a small PCB batch and passive population, USD 15–30 for the Nucleo, and USD 20–60 for four suitable floats. Harnesses, vessels, brackets and enclosure can add USD 20–50. Reusing lab vessels and the processor board reduces incremental expense. None of these estimates implies current stock or an approved purchase list.

## Power and electrical limits

Use the Nucleo's normal USB supply arrangement, not a second parallel regulator feeding J1. The carrier draws only the pull-up current plus negligible input leakage. At nominal resistance, a closed contact draws 3.3 V / 11 kohm = 0.300 mA; all four closed draw 1.20 mA. The input low level is approximately 0.300 V and total resistor dissipation is 3.96 mW. Verify voltage levels with the assembled harness, especially if contact resistance is unstable. A disconnected wire reads dry and can be indistinguishable from a genuine dry float; topology checks do not diagnose every open circuit.

Budget 150 mA at the 5 V USB input for the whole development-board-based instrument, or 0.75 W. This is a conservative planning allocation, not measured consumption or a USB enumeration claim. The firmware continuously samples and services serial output; there is no low-power sleep state, battery charger, energy-harvesting subsystem or battery-life promise. Removing USB power stops measurements and clears volatile counters. The small carrier cannot protect the host from gross miswiring, and a series resistor is not an isolation barrier.

## Firmware state machine and timing decisions

The firmware is a freestanding C11 image with startup vectors, data/BSS initialization, a linker script, explicit register definitions, GPIO and nonblocking serial drivers, an indicator driver, measurement engine and bounded protocol encoder. No vendor SDK download is necessary. The main loop processes one sample per observed millisecond tick. A gap greater than two milliseconds increments a saturating gap counter, invalidates an active trial, and restarts qualification. Missing time is never silently treated as stable contact time.

The complete four-bit mask must remain unchanged for 30 milliseconds before qualification. This whole-mask debounce favours coherent topology over independent edge speed. Several simultaneously changing contacts can extend qualification; the system is intended for slow water-level transitions, not sub-millisecond contact characterisation. At startup, or after a sample gap, telemetry reports mask 255 until fresh qualification completes. The previous qualified mask is retained internally across gaps, but an active trial is already faulted.

A qualified mask of 3 means both source floats are wet and both receiver floats are dry. Holding this ready condition continuously for one second arms a new trial, increments its identifier and clears per-trial flags and band durations. The one-second preparation period is not included in elapsed trial time. The armed state waits for source upper opening or receiver lower closing; either begins transfer. Source upper opening starts the drain-band clock, source lower opening stops it. Receiver lower closing starts the fill-band clock, and receiver upper closing stops it. These clocks are independent, allowing either vessel's band to begin first.

A source rise or receiver fall during an active trial is a reversal fault. An upper wet float with its lower dry float is a topology fault. Both source thresholds opening together or both receiver thresholds closing together is a skipped-band fault, not a zero-duration success. An active trial lasting 120 seconds faults even if no water has moved. Completion requires mask 12 and both band durations known. Completion and fault hold the result until the physical setup returns to ready for another second. Trial counters saturate instead of wrapping; elapsed age accumulation also saturates. The millisecond clock itself wraps naturally, and subtraction handles ordinary wrap between samples.

LD2 gives a short heartbeat every two seconds while settling/ready, a half-second blink while armed, a faster equal-duty blink during transfer, steady light after completion, and short rapid pulses on fault. Light patterns do not convey all numerical diagnostics. A watchdog is started before any measurements are emitted and is fed only after sampling progresses. If the watchdog oscillator does not become ready within the bounded initialization wait, the instrument emits no measurement stream and flashes a rapid startup error. The watchdog period depends on the internal low-speed oscillator and is not a precision timing reference.

## Protocol and companion software

Each ASCII line contains SW1 followed by thirteen unsigned decimal fields: sequence, uptime milliseconds, trial identifier, phase, mask, flags, elapsed milliseconds, drain milliseconds, fill milliseconds, completed count, failed count, sample-gap count and transmit-drop count. A star and four uppercase hexadecimal CRC digits precede LF. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xFFFF, no reflection, no final XOR, calculated over every byte before the star. The standard test string 123456789 produces 0x29B1. Unknown durations are 4294967295, never zero.

Phases are 0 settling, 1 ready, 2 armed, 3 transfer, 4 complete and 5 fault. Flag bits are 1 topology, 2 reversal, 4 timeout, 8 sampling gap, 16 skipped band and 32 serial transmit loss. The transmitter emits periodic snapshots every 250 milliseconds and tries to report phase changes promptly. It does not promise a lossless event journal; very brief intermediate phases may be coalesced while a frame is being sent. A transmit attempt has a 100 millisecond deadline. Failure is counted, and the application resynchronizes on the entire SW1 marker after partial frames. Sequence numbers describe submitted frames, not acknowledgements from a host.

Run `python3 app/witness.py` on a Linux desktop with Tk installed. The Connect / Replay screen selects a serial TTY or opens a saved SW1 stream. The Live trial screen displays phase, float mask, band times, elapsed time, counters and decoded fault reasons. Evidence displays the most recent 200 accepted snapshots; the retained export buffer holds up to 10,000, with eviction count disclosed. Export creates a new private CSV and refuses to overwrite an existing file. Starting a new connection or replay intentionally clears the old session, so export first.

The serial port is opened read-only and raw at 115200 baud. There are no configuration commands, network sockets, shell commands, remote updates or dependency-install hooks. After 1.5 seconds without a valid accepted frame, live readings are marked stale. Disconnect retains historical evidence but not a live indication. Session provenance remains live or replay even after disconnection; CSV records the current connection state separately. Parse errors, restart blocking, warning text, freshness, frame loss, row eviction and row age accompany the exported measurement fields. These are session qualifications at export time, not per-row historical diagnoses.

Backward sequence/time movement blocks ingestion until reconnect rather than silently merging two device runs. This is a heuristic because no persistent boot identifier exists. A reset whose first observed counters happen to move forward cannot always be detected. Evidence therefore belongs to a supervised session, not an audit-certified continuous recorder. Replay is explicitly labelled; fixtures generated by host tests are synthetic and are not physical measurements. All storage is local and ephemeral until export. CRC detects accidental corruption, not malicious forgery; do not trust an unknown USB device to provide authentic scientific data.

## Build, verification and first physical trial

From this directory, run `make -C firmware clean all test`. This builds the Cortex-M4 ELF and binary using arm-none-eabi-gcc, links against the supplied memory map, and runs the production engine and encoder under host address/undefined-behaviour sanitizers. Use `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v` for parser, evidence and real Tk event-loop tests. The GUI test uses a Linux pseudo-terminal and real C-encoded fixture, not a physical ST-LINK connection.

For hardware regeneration use `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9 Python bindings and the standard footprint libraries. Then run `/usr/bin/python3 tools/check_hardware.py`. It requires zero ERC violations, zero DRC violations and zero unconnected items, and verifies the fresh schematic-to-PCB connectivity. The project file stores authorship in supported text variables. Each source file credits jayis1; the generic installed KiCad footprint geometry is not claimed as a newly invented footprint. See VERIFICATION.md for observed results rather than assuming these commands were successful on another workstation.

Before attaching water, inspect solder joints and meter J1 polarity. Check all four open input voltages and all four closed levels with a jumper. Flash the binary at 0x08000000 through ST-LINK, reset, and manually simulate mask sequence 3, 1, 5, 4, 12 with several seconds at each step. Verify the desktop agrees and that deliberately impossible mask 2 faults. Then disconnect USB and install the float harnesses, keeping electronics dry. Lift each float separately to verify the expected bit and polarity before mounting it at a threshold.

For the first water trial, use a shallow spill tray and enough receiver freeboard for the entire source volume. Fill the source above its upper switch, empty the receiver below its lower switch, wait for the armed indication, and prime the siphon by a clean-water-safe method without mouth suction. Observe rather than leave the bench. Compare recorded band times with independent video or a stopwatch over repeated trials. Record threshold spacing, water temperature, hose geometry and float mounting in separate notes; the app does not automatically measure them.

## Limits and release status

This release supplies the detailed design, complete target-buildable firmware, routed carrier PCB, application and repeatable software/CAD checks. It has not been fabricated, programmed on real hardware, hydraulically calibrated, EMC/ESD tested or certified. No flow rate, pressure, transferred volume, leak tightness, drinkability, flooding protection or guaranteed siphon reliability can be inferred from a passing threshold sequence. A stuck contact can produce plausible states, and an unobserved process can continue after a completed trial. Use independent containment and supervision. The device is for small clean-water bench experiments, not dams, life-support aquariums, medical equipment, unattended irrigation, fuel, solvents, hot liquids or safety-critical process control.
