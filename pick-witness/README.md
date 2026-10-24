# Pick Witness — handloom shuttle, shed and beat sequence recorder

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored hardware sources: MIT.

Revision A is a build-tested engineering prototype. No physical loom trial, fabricated-board inspection, electrical qualification, or accuracy certification has been performed. The custom PCB is a passive contact interface; the processor, regulator, debugger and USB interface are on a separate NUCLEO-F401RE. It is not a standalone MCU board.

## Purpose, user and practical value

Pick Witness is a removable witness for a small manual, two-shed plain-weave loom. Its primary user is a weaving instructor helping a beginner establish a consistent throw, arrival, beat and shed-change rhythm. A beginner may lose track of whether a shuttle reached the opposite side, beat twice without another throw, or change the shed before finishing the current pick. Looking at the cloth afterwards does not always tell the instructor when the questionable motion occurred. Counting beater strokes alone is also misleading: a stroke is not necessarily a completed, properly ordered pick.

The device observes four deliberately simple physical states: shuttle resting in the left dock, shuttle resting in the right dock, beater at its forward beat position, and shed B selected. It asks a narrower question than a camera or a fabric inspector: did the observed controls follow the selected plain-weave sequence? A left-to-right or right-to-left arrival becomes pending only after an observed departure. A forward beat consumes that arrival. Accepted beats must alternate the qualified shed bit. The shed must remain unchanged from departure through arrival and beat. Questionable observations become explicit reasons rather than silently increasing the accepted count.

An instructor can therefore compare repeated-shed attempts, beats without a witnessed pass, and premature departures against a lesson plan. The elapsed arrival-to-beat interval gives a modest coordination measurement, not a speed score. The companion application retains the recent status stream and exports a private CSV that includes acquisition gaps, serial errors and provenance. This is useful when discussing an interrupted lesson: an unexplained count is less helpful than a count accompanied by evidence that a cable disconnected or a switch was adjusted.

The minimum useful assembly is one Nucleo board, the passive interface, four reed contacts with magnets, and a Linux laptop. There is no cloud subscription, camera recording, radio pairing, motor output or loom modification requiring permanent wiring. Mounts must be removable and mechanically safe. This revision deliberately trades broad compatibility for a clear experimental contract: slow manual plain weave with visible stops at both shuttle docks. It is unsuitable for flying-shuttle, powered, multi-shaft-pattern or continuous high-speed weaving.

## Originality and adjacent work

The repository was inventoried before implementation, including device directory names and README purpose sections. StitchScope concerns stitch formation on sewing machines; WebSlack Ledger concerns differential strip travel; FoldTrace concerns folding-jig states; ContactAtlas maps detached rotary switch contacts. None provides the shuttle-departure, opposite-arrival, stable-shed and consumed-beat contract implemented here. The reusable passive contact electronics and STM32 platform are intentionally derived from jayis1's ContactAtlas design; the loom-specific engine, application, tests and documentation are new.

The proposed novelty is the removable teaching workflow and conservative evidence semantics, not invention of a loom counter. A limited web search found AVL loom literature describing computer-assisted loom controls and coordinated shuttle, beater and shed functions. Such adjacent systems mean this project must not claim that electronic weaving supervision is new. A basic tally counter and a phone video are also credible alternatives: the counter lacks ordering evidence, while video requires manual review. Pick Witness is a design hypothesis for a small, inspectable middle ground. No worldwide originality, patent novelty, market absence, or commercial superiority has been established.

## Mechanical sensing and operating contract

Use two shallow shuttle parking docks outside the warp edges. Each contains a normally open reed switch; a small mechanically retained magnet on the shuttle closes the switch only when fully parked. The left and right detection regions must not overlap. A third contact closes near the end of the forward beater stroke. A fourth closes when the mechanism is positively in shed B and remains open in shed A. The switch arrangement must not impede moving parts or change the loom's original stops.

The fourth sensor is only one binary observation. It cannot distinguish shed A from a neutral position or a disconnected lead. Fit it to a mechanism with two stable positions, and independently verify that the open position actually produces shed A. Do not infer successful warp separation from the input bit. Likewise, a magnet moved by hand can imitate a shuttle; no sensor here verifies that thread passes through the shed, that weft is present, or that the resulting fabric is correct.

Keep every meaningful state steady for at least 20 ms. In practice, use a clear pause at each dock and separate arrival from beat. Changes that qualify together have no reliable ordering; simultaneous arrival and beat are rejected as ambiguous. A departure with a simultaneous shed change is also ambiguous. A shed change during shuttle flight invalidates the movement. Each flight and each arrival-to-beat waiting period is separately limited to 10 seconds, preventing unrelated movements after a long interruption from being joined into a pick.

For initial synchronization, park the shuttle in exactly one dock and release the beater. If both docks close, the engine discards motion context. Recovery requires a newly qualified single-dock, released-beater pose. An acquisition gap also discards context and restarts qualification. Cumulative counts remain, but alternating-shed history resets after invalidation; the first subsequent valid pick establishes a fresh parity baseline. Inspect the gaps and anomalies before comparing counts across such an interruption.

## Hardware specifications and interface contract

| Function | Part or implementation | Connection |
|---|---|---|
| Processor board | ST NUCLEO-F401RE, MB1136; STM32F401RET6 | External board, reset HSI clock at 16 MHz |
| Contact sensors | Four Littelfuse MDSR-7 normally open reed contacts, sensitivity selected for the mount | External to the PCB, magnet actuated |
| Left dock | J3 pin 1 CONTACT0, pin 2 GND | SENSE0 to PA0, Arduino A0 |
| Right dock | J4 pin 1 CONTACT1, pin 2 GND | SENSE1 to PA1, Arduino A1 |
| Forward beat | J5 pin 1 CONTACT2, pin 2 GND | SENSE2 to PA4, Arduino A2 |
| Shed B | J6 pin 1 CONTACT3, pin 2 GND | SENSE3 to PB0, Arduino A3 |
| MCU signal header | J2 pins 1, 2, 3, 4 | SENSE0, SENSE1, SENSE2, SENSE3 |
| Carrier power | J1 pin 1 VCC, pin 2 GND | Nucleo 3V3 and common ground, never 5 V |
| PC link | USART2 TX PA2, AF7 | ST-LINK virtual COM, 115200 baud, 8N1 |
| Board geometry | Two copper layers, 1.6 mm thickness | 80 × 70 mm; edge coordinates 10,10 to 90,80 mm |

J1 through J6 are 2.54 mm through-hole headers. Every contact channel has a 10 kΩ pull-up from VCC to SENSE and a 1 kΩ resistor between SENSE and CONTACT. The closed-contact current is approximately 0.3 mA at 3.3 V, and the closed SENSE level is approximately 0.3 V. Four closed inputs draw approximately 1.2 mA. These values are calculations, not measurements. Use low-level contacts; a switch requiring high wetting current is inappropriate.

There is no galvanic isolation, dedicated ESD protection, battery charger, reverse-polarity protection or external-voltage tolerance claim. Connect only voltage-free contacts with short internal fixture wiring, initially no longer than 0.5 m per channel. Do not connect powered industrial sensors, mains circuits, machine controller terminals, or long cables exposed to static discharge. USB shares ground with the laptop. The series resistor is not a substitute for isolation or a rated protection network.

The Nucleo supplies local decoupling, power regulation and programming support. The passive PCB has no active supply consumer other than its pull-up paths and therefore no active-device bypass network to invent. Keep the Nucleo's standard USART2-to-ST-LINK bridge configuration: SB13/SB14 on and SB62/SB63 off, as documented in UM1724. Verify the actual board revision before assembly. The carrier connects by keyed or clearly labelled flying leads, not by stacking mechanically onto Arduino headers.

## Architecture and block diagram

```text
left dock reed ----- J3 -- 1k -- SENSE0 -- PA0
right dock reed ---- J4 -- 1k -- SENSE1 -- PA1
beater reed -------- J5 -- 1k -- SENSE2 -- PA4
shed B reed -------- J6 -- 1k -- SENSE3 -- PB0
                                      |
                         each SENSE has 10k to 3V3
                                      |
                      STM32F401RE on NUCLEO-F401RE
                    1 ms sampling / 20 ms qualification
                       conservative pick state machine
                                      |
                    PA2 USART2 TX -> ST-LINK USB VCP
                                      |
                       Linux Tk workbench -> private CSV
```

All four reed returns connect to GND. The firmware inverts the GPIO inputs so a closed contact is represented by one. Bits zero through three are left, right, beat and shed B respectively. The schematic, PCB and connectivity manifest use the same SENSE and CONTACT names. The CAD checker exports a fresh KiCad XML netlist and compares every reference, numbered pin, value, footprint identifier and net against the actual PCB pads and manifest. It is not merely checking that parentheses balance.

## Firmware implementation and design decisions

The target is freestanding C11 for Cortex-M4 with no vendor runtime or floating-point dependency. Startup copies initialized data, clears BSS and installs a minimal vector table. Only SysTick interrupts are enabled. Unexpected faults enter a non-returning handler; once started, the independent watchdog provides recovery. The linker script places code in the STM32F401RE's 512 KiB flash and data in its 96 KiB SRAM, reserving stack headroom with a link-time assertion.

The board driver uses the reset HSI clock, configures the four GPIOs as inputs without internal pulls, sets PA2 to AF7, and initializes USART2 transmit-only. Register definitions correspond to STM32F401 RM0368 and Cortex-M SysTick, not a fictional generic sensor interface. No external sensor register map is needed for dry contacts. The independent watchdog uses the LSI source; readiness waits are bounded. If watchdog initialization fails, normal telemetry is withheld rather than implying normal operation.

The acquisition engine sees actual elapsed milliseconds. A skipped millisecond is not treated as a repeated sample. Instead it increments the gap count, clears pending motion, and requires a fresh 20-sample qualification window. Whole-vector qualification avoids inventing order through independent channel debounce delays. It also means that unrelated contact chatter can postpone all four inputs; this conservative trade-off is intentional and should be evaluated on a real loom.

A valid departure remembers both origin and shed. Opposite arrival sets a pending beat. Returning to the original dock records an anomaly, not a completed pass. Starting another departure while an arrival is pending records an unbeaten pass. A forward beat without pending arrival increments rejected beats. A repeated shed also rejects the beat. Accepted beats update cumulative count and last/minimum/maximum arrival-to-beat intervals. These intervals describe qualified switch times, including sampling and qualification effects; they are not calibrated shuttle-flight timing.

Counters saturate rather than wrapping silently. The saturation bit remains visible even if a subsequent event changes the sticky reason. Ordinary 32-bit millisecond wrap is handled by unsigned elapsed differences. Flight and pending ages use saturating accumulation rather than ancient timestamp subtraction. There are no flash writes during operation, no settings persistence, no remote update mechanism and no serial command parser. Power loss erases RAM history. Export the host session before resetting the board.

Transmit processing is nonblocking and bounded. One record buffer is copied into the serial driver, then drained while the foreground loop continues sampling. A 100 ms transmit timeout discards the remainder and increments a transport-loss counter. Every status frame starts with a full marker, so the receiver can resynchronize after a truncated frame even if no newline arrived. Status is emitted approximately every 250 ms; a sticky last reason is a summary, not a guaranteed lossless event log. Multiple events between reports are reflected in counters but their individual reasons may not survive.

## Application and serial protocol

Run `python3 app/app.py` on Linux with Python 3 and Tk installed. The application requires no pip dependencies. Its Connection / setup view opens a chosen serial character device at 115200 baud, obtains an exclusive TTY claim, and displays setup instructions. The Live witness view shows accepted sequences, rejected beats, anomalies, contacts, pending arrival, timing and the latest reason. The History / export view displays up to 200 recent received records and writes the retained session to CSV. Up to 4096 records are retained; dropped-history counts make this bound explicit.

Frames use ASCII, LF termination, unsigned decimal fields and uppercase hexadecimal CRC. The format is `@PW1,<18 decimal fields>*<four CRC digits>\n`. CRC-16/CCITT-FALSE uses polynomial 0x1021, initialization 0xFFFF and no final XOR; coverage includes the initial @ through the last decimal field, excluding the star. The standard check string 123456789 produces 0x29B1. Maximum accepted frame length is 256 bytes. The app rejects future protocol markers, wrong field counts, negative or oversized integers, invalid booleans, unsupported policy values and bad checksums.

Fields in order are sequence, milliseconds, contact mask, ready, pending, accepted, rejected, anomalies, gaps, events, reason, last interval, minimum interval, maximum interval, transmit losses, saturated, qualification milliseconds, and phase timeout milliseconds. The policy fields are repeated in every status so late attachment does not depend on receiving a one-time boot announcement. Reason codes 0 through 12 are none, accepted, beat without pass, repeated shed, both docks, unbeaten pass, shed moved, arrival expired, acquisition gap, ambiguous order, returned to origin, counter limit, and flight expired.

The application marks live data stale after 1.5 seconds without a valid frame. Disconnection retains values and their source but labels them not live. Sequence, timestamp or cumulative-counter rollback blocks further acceptance until a new session is explicitly started. This is a reset heuristic, not a unique boot identity: an unseen reboot whose new counters and times happen to exceed previous values can evade detection. Long outages and timestamp ambiguity require an explicit new session. No serial authentication is provided; CRC detects corruption, not deliberate forgery. Keep physical USB access within the trusted teaching setup.

Replay is clearly labelled and never considered live. Replay files are capped at 1 MiB. CSV exports preserve source, connection state, suspected-reset state, parser errors, missing frame count, history truncation and export-time freshness along with observations. Files are created mode 0600 and existing paths are never overwritten. There is no network service, analytics or automatic upload. The app's main decision support is whether to inspect a questionable sequence or repeat a lesson segment, not whether a machine is safe to operate.

## Bill of materials, power and enclosure

Budgetary estimates are planning allowances, not supplier quotations. Allow approximately US$20–30 for a NUCLEO-F401RE, US$4–10 for a small passive PCB, US$2 for eight through-hole resistors and headers, US$8–16 for four low-level reed contacts and magnets, and US$10–20 for wiring and removable mounts. A working prototype budget is roughly US$45–80 before the laptop, labour and shipping. The modest target is credible for a shared teaching fixture; it is not a price promise or a proven commercial business case.

Use four 10 kΩ and four 1 kΩ metal-film axial resistors with the provided 7.62 mm pitch footprints, for example Yageo MFR-25 series parts selected to those values. Use 1×2 headers for power and contacts and a 1×4 header for signals. The selected MDSR-7 reed family is an external assembly item, not a surface-mount part fitted into a header footprint. Magnet size, sensitivity bin, holding method and clearance must be selected together during mechanical validation. Protect glass capsules from bending forces with insulated mounting and strain relief.

Power comes solely from the Nucleo's USB supply. Allocate 100 mA at 5 V for the combined development board and passive interface as a conservative prototype allowance, subject to measurement. This corresponds to 0.5 W and 2 Wh for a four-hour lesson. The 3.3 V contact contribution ranges from near zero when all contacts are open to approximately 1.2 mA when all are closed. There is no implemented sleep state or claimed battery runtime: acquisition is continuous while powered, including when the loom is idle.

Enclose the passive board and Nucleo in a nonconductive case with separate retention and cable strain relief. The carrier has no mounting holes; use edge supports that do not touch copper and verify clearances before closing the case. Do not hang the assembly from a sensor lead. Keep magnets mechanically retained and inaccessible to young children. Never place cables across the shuttle path or attach a sensor where it could be drawn into the warp or beater mechanism.

## Build, verification and commissioning

From this directory, run `make -C firmware all test`. The target build produces an ELF and binary with the ARM embedded GCC toolchain. Host tests run the same engine and encoder with address and undefined-behavior sanitizers. `make -C firmware flash` uses OpenOCD and ST-LINK; it is an explicit physical programming action and was not exercised without a connected board. Do not interpret successful linking as proof that a particular assembled board boots.

Run `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9's Python bindings to regenerate the CAD. Run `/usr/bin/python3 tools/check_hardware.py` for ERC, DRC and pin-level agreement. Run `/usr/bin/python3 tests/test_app.py` and `xvfb-run -a /usr/bin/python3 tests/test_gui.py` for parser/model and actual Tk/PTY tests. The C fixture is explicitly synthetic and exercises two accepted sequences followed by a repeated-shed rejection. It is not recorded loom data.

Before attaching to a loom, inspect component orientation, continuity, ground routing and absence of solder bridges with power removed. Power the Nucleo independently, verify its 3.3 V rail, then connect J1 and J2. Short each contact input to its paired ground through a temporary voltage-free switch and check the displayed bit. Verify closed/open thresholds with a meter. Check that sensor movement yields at least the required dwell, that both docks cannot legitimately close together, and that the shed bit stays stable for a full throw.

Commission with slow deliberate sequences: a valid pass and beat, an empty beat, a repeated shed, a return to the same dock, a shed change mid-flight, a simultaneous arrival/beat, a stopped shuttle exceeding the timeout, and a disconnected serial cable. Compare observations with written notes and an independent timing reference. Adjust mechanical mounts rather than weakening qualification to hide errors. Only after these checks should it be used as experimental teaching feedback, still never as an interlock or fabric-quality guarantee.

## References and remaining limits

Manufacturer references: ST UM1724 for NUCLEO-F401RE connector and solder-bridge routing; ST RM0368 for STM32F401 register definitions; ST STM32F401xD/xE datasheet for electrical limits; Littelfuse MDSR-7 product documentation for reed-contact handling and sensitivity selection. UM1724 was retrieved through the Harvey Mudd College mirror after the manufacturer's PDF endpoint failed. Useful URLs are https://www.st.com/en/evaluation-tools/nucleo-f401re.html and https://pages.hmc.edu/brake/class/e155/fa20/assets/doc/Nucleo-64_User_Manual_UM1724.pdf. Adjacent loom literature is at https://www.avlusa.com/downloads/Brochure%20V1.pdf.

Remaining physical work includes mount design for a specific loom, reed operating margins, cable-noise susceptibility, contact ageing, actual current measurement, oscillator timing error, watchdog recovery, assembly inspection and user trials. ERC and DRC cannot establish these properties. No electromagnetic compatibility, ingress, child-safety or regulatory certification is claimed. See VERIFICATION.md for the exact checks actually executed and the limits of that evidence.
