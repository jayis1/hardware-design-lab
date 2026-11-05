# Toggle Hold — unloaded-retention notebook for bistable mechanisms

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The supplied PCB is a routed passive contact carrier for a separately purchased NUCLEO-F401RE, not a standalone processor board. Target linking, simulated experiments, desktop application workflows and electrical CAD checks are reproducible. Fabrication, programming a physical target, mechanical assembly, contact adjustment, electrical bring-up, repeatability and practical usefulness have not been physically verified. This is not a force gauge, fatigue tester, safety interlock or certification instrument.

## Purpose, audience and useful result

Toggle Hold helps a maker compare small bistable printed mechanisms, such as an over-center flexure in a model display, a reversible desktop indicator, or a two-position educational linkage. Such a mechanism can appear to have snapped into its second position while a finger is still helping it stay there. Once the finger leaves, it may immediately return, float between the endpoints, or settle successfully. Counting endpoint arrivals alone merges these very different observations. Toggle Hold records a more specific sequence: starting endpoint, directed hand-pressure indication, opposite endpoint reached while pressure remains indicated, pressure withdrawal, and an uninterrupted unloaded observation window.

The intended user is a workshop designer experimenting with printed hinge thickness, clearances or an over-center geometry. They do not need an automated production tester for the first comparison. They need a repeatable notebook that distinguishes an apparent transition from short-term retention after unloading. The operator can compare A-to-B and B-to-A trials separately instead of assuming the mechanism behaves symmetrically. The device produces named outcomes, relevant milestone times and the underlying input snapshots, rather than an unsupported overall quality score.

A representative trial starts with the specimen resting at endpoint A and both hand plungers released. The desktop shows READY after a short observed baseline. The user depresses the plunger toward B, moves the specimen to B, and waits for LOADED. After releasing the plunger they watch the HOLD interval. If B remains indicated with no pressure witness active for 500 acquired milliseconds, the result is retained. If the endpoint drops out, or pressure is reapplied during that interval, the result is rebound / recontact. A mechanical rebound and an electrical contact interruption cannot be distinguished from these signals alone.

That explicit uncertainty is part of the product. A retained result does not mean indefinite stability, adequate holding force or a safe mechanism. A rejected result does not prove a defective print. Misadjusted switches, noisy leads, a sticky plunger or inadequate acquisition can produce rejection. The instrument is useful when it makes these limitations easier to see and compare than informal notes. A camera or a force-displacement test stand remains the appropriate alternative when continuous motion, snap force, energy or material fatigue matters.

## Differentiation and limits of novelty

The repository's device directories and concept titles were inventoried before selecting this design. Catch Witness was examined in detail: it classifies reopening of a settled panel latch using handle and challenge associations. Detent Loop was also examined in detail: it records analog reaction force and crosshead travel during outward and return legs. Snap History concerns reversible engagement groups across multiple fasteners, and Flap Return compares closures after a shared release. None of those contracts is this two-direction bistable transition followed by an explicitly unloaded retention window.

The proposed contribution is the particular bench fixture and conservative event contract, not the invention of bistability or switch-based measurement. It separates reaching the destination under pressure from remaining there after pressure withdrawal, supports either origin without a fixed home, rejects simultaneous or missing evidence, and freezes one result until an intentional reset. It reuses jayis1's inspected passive carrier, clock, UART and startup platform. Reusing the electrical interface does not make the experimental state machine or the interpretation identical to another device.

This is repository-level differentiation, not proof of worldwide uniqueness, commercial demand, patentability or freedom to operate. Mechanical testing of snap-through and over-center devices already exists. No comprehensive patent or commercial prior-art search was performed. The design should be treated as a proposed open workshop instrument whose value must still be evaluated in a real comparative experiment.

## Hardware specifications

| Subsystem | Revision A selection and contract |
|---|---|
| Processor assembly | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Processor operation | Cortex-M4, reset HSI at nominal 16 MHz |
| Memory map | 512 KiB flash, 96 KiB SRAM |
| Endpoint sensors | Two normally-open, low-level dry-contact switches |
| Pressure witnesses | Two independent normally-open plunger switches |
| Candidate switch | Omron D2F-01L, subject to actual force and travel suitability |
| Input qualification | Five consecutive 1 ms acquired samples per channel |
| Baseline | 50 observed intervals at one endpoint, both plungers released |
| Unloaded hold | 500 observed intervals after qualified pressure release |
| Rejecting deadline | 10,000 ms from the qualified start |
| Communications | USART2 TX on PA2 AF7, 115200 baud, 8N1 |
| Host link | Nucleo ST-LINK virtual COM over USB |
| Custom carrier | 80 × 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Application | Linux, Python 3 and Tk, standard library only |
| Actuation | None: no motors, locks, relays, heaters or drive outputs |
| Battery and radio | None |

The processor, USB bridge, debugger, regulator, reset circuitry and MCU decoupling belong to the purchased Nucleo assembly. They are not hidden omissions from an alleged standalone custom MCU board. The native KiCad files depict the actual passive carrier: six headers and eight resistors with real symbols, pin connectivity, footprints and routed copper. A short cable harness connects the carrier to the Nucleo; this is not a mechanically mating shield.

### Wiring contract

| Carrier connection | Nucleo or specimen connection |
|---|---|
| J1.1 VCC | CN6.4 regulated 3V3 |
| J1.2 GND | CN6.6 ground |
| J2.1 SENSE0 | CN8.1 A0 / PA0, END_A |
| J2.2 SENSE1 | CN8.2 A1 / PA1, END_B |
| J2.3 SENSE2 | CN8.3 A2 / PA4, PUSH_B |
| J2.4 SENSE3 | CN8.4 A3 / PB0, PUSH_A |
| J3.1 / J3.2 | Endpoint A signal / ground |
| J4.1 / J4.2 | Endpoint B signal / ground |
| J5.1 / J5.2 | Pressure toward B signal / ground |
| J6.1 / J6.2 | Pressure toward A signal / ground |

PUSH_B means a plunger requesting motion toward B, not a switch physically located at B. Label both plungers with destination arrows to avoid reversing their meaning. A closed switch is represented as a one bit after firmware inverts the low-active GPIO. Each SENSE net has a 10 kΩ pull-up to VCC and a 1 kΩ series resistor between the MCU-side node and the external contact signal. The resistor pairs are R1/R2 through R7/R8 in channel order.

At nominal 3.3 V a closed contact draws 0.30 mA and leaves the sense node at 0.30 V. Four closed paths draw 1.20 mA, or 3.96 mW from the regulated supply. These are ideal resistor calculations, not measured electrical performance. Use signal-rated contacts rather than switches requiring large wetting currents. The 1 kΩ series resistor does not provide galvanic isolation, industrial transient suppression or meaningful protection against powered field wiring.

Allow a current-limited 5 V USB supply with a 500 mA budget for the entire Nucleo/debugger assembly. Actual assembly consumption was not measured. Power the Nucleo from one supply only and take regulated 3.3 V from the indicated header. Never feed USB voltage into the carrier VCC pin, connect a second supply to the carrier, or attach an externally powered sensor output. The host computer, carrier and switches share ground. Keep initial sensor leads shorter than half a meter, route signal and return together, and provide strain relief.

### Bill of materials and assembly envelope

The electrical BOM is one NUCLEO-F401RE, four 10 kΩ and four 1 kΩ axial resistors, five two-pin headers, one four-pin header, four low-level switches and the two-layer carrier PCB. Resistor footprints use a 7.62 mm lead pitch. Header pitch is 2.54 mm. The machine-readable connectivity manifest records each board reference, value, footprint and ordered pin-net list. External switches and the Nucleo are separate purchased assemblies, not PCB-mounted components in that manifest.

Plan a low-volume materials allowance of roughly USD 50–100 including a development board, switches, wiring, carrier, insulating supports and simple manual fixture hardware. This is a provisional budgeting range, not a supplier quotation. Machining, printing labor, independent instrumentation and shipping are excluded. Reusing an existing Nucleo can reduce incremental cost, but cannot eliminate the need to evaluate the sensor forces and fixture geometry.

## Architecture and block diagram

    Low-energy bistable specimen on guarded manual fixture
        endpoint A ----- J3 END_A -----\
        endpoint B ----- J4 END_B ------+-- 1k series + 10k pull-up
        B plunger ------ J5 PUSH_B -----+             |
        A plunger ------ J6 PUSH_A ----/              v
                                               J2 GPIO harness
    USB -> NUCLEO-F401RE -> regulated 3V3/GND -> J1 carrier
                |
                +-- 1 ms input acquisition
                +-- independent qualification
                +-- baseline -> push -> loaded -> unloaded hold -> frozen result
                +-- bounded CRC snapshot encoder -> USART2 -> ST-LINK VCP
                                                           |
                                                Linux local application
                                                Trial / History / Setup
                                                strict replay / private CSV

There is deliberately no command path from the companion into the experiment. Closing the app cannot move the specimen. Firmware has no incoming command parser and does not enable the USART receiver. All sensing pins remain inputs. Serial transmission is buffered and polled separately from acquisition, so a busy transmitter does not deliberately block sampling. Transmission timeouts are counted; acquisition discontinuities invalidate the experiment rather than being interpreted as a long stable contact.

## Mechanical fixture design

Use an insulating base approximately 220 × 140 mm for a small specimen, with a rigid central mounting clamp and adjustable stops at its two intended positions. Fit one endpoint switch on each side using slotted brackets. The specimen must operate the switches through lightweight followers without resting its retention load on the switch bodies. Provide independent travel stops so neither the endpoint switches nor the pressure witnesses serve as structural stops. Leave clearance for the specimen's full transition path.

Two opposing hand plungers provide a visible and repeatable interaction method. Each has a spring return, an insulated contact tip, and a separate switch that closes during initial plunger travel before the tip pushes the specimen. The witness should remain closed while the tip can apply force and open only after a visible clearance is restored. A proposed starting clearance is 2 mm at rest, to be adjusted after assembly. The firmware cannot verify that clearance or detect a disconnected witness. In particular, a switch opening does not prove zero force if a plunger sticks or a finger touches the specimen elsewhere.

Locate the endpoint switches so A and B cannot normally be closed simultaneously. Use loose cable loops and low operating-force followers; a switch that helps hold the mechanism in position defeats the purpose of the experiment. Compare behavior with and without the sensors installed before trusting any retention result. Mark bracket positions after adjustment. Record specimen geometry, print material, print orientation, conditioning and ambient conditions alongside exported data in a separate notebook.

Protect fingers from the moving region with a clear guard and operate only low-energy specimens. Do not use prestressed metal springs capable of hazardous release, powered mechanisms, lifting hardware or any safety-critical latch. The board has no mounting holes; retain it in insulated edge supports with clearance below solder joints. An electronics enclosure should allow at least 160 × 110 × 45 mm for the Nucleo, carrier and cable bends. These are dimensional planning recommendations, not supplied manufacturing drawings or validated enclosure CAD.

## Firmware behavior and design decisions

The target uses freestanding C11, explicit STM32F401 register definitions, a supplied linker script and startup vector table. There is no vendor SDK, dynamic allocation or network stack. The clock driver restores and checks the reset HSI clock tree with bounded iteration waits before initializing GPIO and UART. The board driver configures SysTick, samples PA0/PA1/PA4/PB0 and starts the independent watchdog using bounded waits. The serial driver rejects oversized or overlapping submissions and abandons a transmission after 100 ms rather than blocking acquisition indefinitely.

Each input qualifies independently after five matching samples. The qualification timestamp is the fifth sample, not a backdated estimate of first contact. Chatter on one pressure switch cannot prevent an endpoint from qualifying. Pulses entirely between acquisitions are invisible, and short pulses may be deliberately filtered. Sampling is nominally every millisecond, not a claim of calibrated timing accuracy. The internal oscillator, main-loop scheduling and physical switch latency all affect the actual observation.

WAIT looks for exactly one endpoint with both pressure inputs released, with raw and qualified vectors agreeing. Baseline credit requires that condition at both ends of each acquired interval. Fifty intervals produce READY and establish A or B as the origin. A paddle held at startup cannot invent a trial. Loss of the origin or appearance of the wrong-direction paddle in READY returns to WAIT and requires a new baseline. The correct paddle must qualify while the origin remains physically indicated; move slowly enough to make that ordering observable.

PUSH begins at the correct paddle's qualified rising edge. The age starts at zero. A qualified release before the opposite endpoint was reached under raw pressure produces early release. To establish LOADED, the raw and qualified endpoint identities must both show the destination while the correct raw pressure witness remains closed. Consequently simultaneous arrival and raw release cannot be interpreted as an ordered arrival-under-load. LOADED waits for qualified release of both pressure witnesses while the destination remains present.

HOLD begins with zero credit at that release endpoint. Every subsequent millisecond must have the destination present and both pressure witnesses absent in both raw and qualified states. Any raw loss or recontact rejects retention immediately, even if debounce would later filter it out. This deliberately favors rejecting uncertain experiments over claiming continuous unloaded retention through a hidden interruption. At 500 credited intervals the result is retained. A disqualifying raw sample on that exact boundary wins over completion.

The ten-second trial deadline wins over ordinary events, including completion, on the same sample. During an active trial simultaneous raw endpoints or the opposite-direction raw pressure witness produce conflict. A sampling gap halts the instrument and abandons an active trial; the missing time is not added to acquired age. Invalid input masks, clock-independent configuration checks, watchdog setup failure and counter exhaustion also produce explicit faults. A sequence counter at its limit halts rather than wrapping. Terminal acquisition evidence freezes, although transport timestamps continue to advance.

This revision records exactly one trial per MCU reset. That limitation avoids accidentally treating a failed specimen position or a held plunger as the beginning of a new comparison. Export the result, disconnect the app, release both plungers, reset the Nucleo and reconnect for another trial. Reset destroys volatile evidence; there is no persistent session identifier, flash log, automatic calibration, authenticated update system or remote reset feature.

## Application and wire protocol

Launch `python3 app/workbench.py` from this directory on Linux with Tk installed and serial-device permission. The Connect / Trial screen accepts an explicit serial path and displays phase, direction, result, hold credit, arrival and release times, faults and transport diagnostics. History / Export displays all retained snapshots up to the 2,000-row bound. Setup / Limits explains wiring and procedure in the application itself. A successful reconnect replaces the previous host session; failed connection setup preserves existing evidence. Disconnect also preserves evidence.

Snapshots are attempted every 100 ms. Each ASCII line contains `TH1`, seventeen comma-separated unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits and LF. Fields are ordered as follows:

    sequence now state fault raw qualified valid origin age baseline hold
    reached_ms released_ms finish_ms result tx_errors gaps

CRC-16/CCITT-FALSE covers the payload before the asterisk, using initial value 0xffff and polynomial 0x1021. The standard `123456789` result is 0x29b1. Values are canonical unsigned decimal; absent milestone times use 4294967295. Origin is 0 for unassigned, 1 for A-to-B and 2 for B-to-A. Input bits are END_A=1, END_B=2, PUSH_B=4 and PUSH_A=8. The valid mask identifies channels that have completed initial qualification.

States are WAIT=0, READY=1, PUSH=2, LOADED=3, HOLD=4, DONE=5 and HALT=6. Results are none=0, retained=1, early release=2, rebound/recontact=3, conflict=4, timeout=5 and aborted=6. Fault bits are configuration=1, watchdog=2, sequence/diagnostic exhaustion=4, invalid input=8 and acquisition gap=16. Fixed timing and interpretation are part of TH1; changes require a protocol revision, not silent reinterpretation of old files.

The parser bounds chunks and frame lengths, checks CRC and numeric ranges, and rejects contradictory milestones or terminal evidence changes. It resynchronizes on the full TH1 marker after a truncated live frame. Replay is stricter: it accepts only regular files up to two MiB, rejects partial tails and requires at least one accepted snapshot. CRC is corruption detection, not authentication. Consistency checks are necessary constraints, not a proof of every hidden physical event between snapshots.

LIVE requires advancing device timestamps and an active acquisition state. Duplicate data cannot renew freshness; DONE and HALT are retained evidence, never live acquisition. A host reception pause beyond two seconds or cumulative host/device progress mismatch beyond two seconds blocks further interpretation until reconnect. This fixed discrepancy limit can conservatively reject long captures with oscillator drift. Neither it nor a serial timestamp proves absolute acquisition freshness or excludes an initial transport delay. Some resets remain indistinguishable without a persistent boot identity.

CSV export creates a new file with private permissions and refuses overwrite, including symlink targets. Metadata includes jayis1 attribution, source, connection state, warnings, parser errors, unreported sample count and reception age even if no trial completed. Sequence advances per acquisition call, not per transmitted packet: intervening samples are normal and are not labeled lost frames. Replay remains labeled replay. Export before reconnecting or loading another file. The application stores no cloud credentials, opens no network service and transmits no serial commands. Synthetic test recordings are not physical measurements.

## Build, verification and physical bring-up

Required tools are GNU Arm Embedded GCC and binutils, native GCC, Python 3 with Tk, Xvfb for headless application tests, and KiCad 9 with its Python bindings and footprint libraries. There are no pip or npm dependencies. Run these commands from the device directory:

    make -C firmware all test fixture
    xvfb-run -a python3 tests/test_app.py
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/verify.py

The firmware build links a real Cortex-M4 ELF and produces a binary using the supplied startup code and memory map. Host tests exercise the production engine and wire encoder, plus simulated register-level clock and UART behavior under address and undefined-behavior sanitizers. Application tests feed real C-generated synthetic frames through parser, replay, Tk and Linux pseudoterminal paths. They exercise both directions, export protection, malformed data, disconnect and retained-evidence behavior rather than merely importing modules.

The CAD checker runs schematic ERC and PCB DRC with violations treated as failures, exports a fresh schematic XML netlist, and compares every reference/pin/net, value and footprint identifier with actual board pads and the manifest. The hardware generator is included for reproducibility; ordinary firmware builds do not regenerate CAD. Passive-only ERC cannot validate the external Nucleo, harness polarity or switches. Zero DRC violations does not prove assembly quality or manufacturing readiness. See VALIDATION.md for executed results and limitations.

Before physical use, inspect the unpowered assembly and test continuity and supply isolation. Verify resistor values and every header pin. Power from a current-limited source, measure the regulated supply, and exercise each channel individually with a known dry contact before mounting a specimen. Review the NUCLEO-F401RE connector map and ST-LINK virtual-COM routing against ST UM1724 for the purchased MB1136 revision. Firmware register behavior is based on STM32F401 reference manual RM0368. Do not assume a different Nucleo model is pin-compatible.

The optional `make -C firmware flash` command invokes OpenOCD's ST-LINK interface and STM32F4 target scripts, then programs, verifies and resets the target. It is not part of automated validation and has not been executed on physical hardware for this design. Compare acquisition and serial timing against independent instrumentation after programming. Deliberately exercise wrong-direction pressure, overlapping endpoints, early release, repeated pressure and contact noise before interpreting a retained result.

Finally compare several known stable and deliberately unstable low-energy specimens using direct observation or video. Establish whether sensor forces perturb their behavior, whether the pressure witness truly opens after mechanical separation, and whether bracket adjustment is repeatable. If the measurement setup changes the phenomenon it is meant to observe, redesign the fixture rather than hiding the problem with longer debounce or a more favorable result label. These physical checks remain necessary work; passing file-level verification is not a substitute for them.
