# Nest Settle — four-corner seating and reseating notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original source and locally authored designs are MIT licensed; see LICENSE.

Revision A is an engineering prototype. The delivered custom PCB is a fully routed passive dry-contact interface, used with a separately purchased NUCLEO-F401RE. It is not a standalone microcontroller board. Fabrication, flashing, physical timing accuracy, switch mechanics and fixture repeatability remain unverified. No output authorizes a machine to start. The word “settled” describes a qualified electrical observation, not a claim that a workpiece is clamped, flat, supported evenly or safe.

## Purpose, audience and practical utility

A small workshop making printed enclosures often uses a simple locating nest to hold each shell for marking, inspection or manual assembly. A slightly warped shell can touch one corner, rock onto the opposite pair, and eventually rest somewhere that looks plausible. The fixture designer wants to compare two nest geometries and determine whether repeated placement reaches all intended locating points promptly and stays there until removal. Looking at the shell from above is not enough to see the sequence at the hidden corners.

Today that designer may use feeler gauges, carbon paper, a multimeter or phone video. Those are useful tools, but a meter shows only one contact at a time, carbon paper records accumulated contact without its timing, and video often cannot see the underside. A commercial pneumatic seating sensor solves a different, higher-precision industrial problem. Nest Settle proposes a modest bench alternative: four spring-loaded, low-force witness contacts in a removable insert, a shared acquisition clock, and a local notebook of seating delay, full-contact dwell and partial corner dropouts.

The concrete experiment is to place the same sample into insert A ten times, remove it completely between trials, then repeat with insert B. The operator changes only the insert, not the witness heights or switch types. The app records whether all four corners stayed qualified closed through a 250 ms milestone, how long that first milestone took, how much of the trial was fully seated, the longest full interval, and which corners opened again before complete withdrawal. This can reveal that a chamfer improves initial placement but does not eliminate subsequent rocking. It can also reveal that the contacts themselves are disturbing the sample.

The target audience is fixture makers, repair workshops, makerspaces and engineering educators. This is not a product-inspection certificate. An outcome may be deciding to use three locating points instead of four, reducing spring force, or abandoning contact witnesses in favor of a noncontact method. The practical benefit is a reproducible comparison record, not an automatic pass/fail assertion about manufactured parts. That benefit is a design hypothesis awaiting workshop evaluation; no customer trial is claimed.

Hardware is justified because four underside contacts need synchronized sampling independent of desktop scheduling. A phone-only application cannot observe these disconnected electrical contacts. The custom board provides bias resistors, series resistance, labeled connectors and reproducible wiring. The development board supplies the microcontroller, USB bridge, regulator, decoupling and debugger; there is no reason to duplicate those functions on a first low-volume fixture interface.

## Originality, alternatives and prior art

The repository's device directories and README purpose paragraphs were inventoried before selecting this concept. QuadraAlign concerns force distribution beneath a machine's supporting feet. Nest Settle does not measure force or level; it studies finite placement and removal trials in a small manual locating fixture. Chord Window studies overlapping human switch gestures, not repeated seating/dropout intervals. Contact Atlas studies rotary-selector transitions, while Fold Trace records an intended sequence of folding leaves. The passive resistor interface and board support are reused from Chord Window, with attribution preserved. Reusing a sound electrical platform is intentional; the new contribution is the seating experiment, finite-state observer, evidence contract and application.

Alternatives considered included a detachable dispenser double-feed recorder, a drawer-latch release notebook and this seating insert. The dispenser was too close to Route Census's object-passage experiments; the latch overlapped HingeScribe's door mechanism domain. Nest Settle offers a clearer local distinction and can be exercised without motors, compressed air, batteries or connection to an operating machine.

A bounded external search found METROL's DPA air-gap seating sensors and a published seat-check-module review. Industrial seating confirmation is established prior art. METROL describes pneumatic gap detection, multiple point configurations and machine-tool applications. Nest Settle does not claim to invent seating detection, compete with micron-scale air gauging, or prove that no similar hobby instrument exists. Its proposed distinction is a low-cost, removable four-contact insert with repeatable placement episodes, explicit incomplete trials, per-corner dropout evidence and offline comparison for manual fixtures. This is an original repository implementation and product proposal, not patent clearance or a worldwide novelty determination.

References:

- METROL DPA family: https://metrol-sensor.com/air-gap-sensor/
- METROL single-point sensor specifications: https://metrol-sensor.com/products/air-gap-sensor-sr1/
- Seat-check-module review: https://doi.org/10.55041/ijsrem16385
- Nucleo connector and virtual-COM reference, UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- MCU reference: STM32F401 RM0368; the register names and addresses are collected in firmware/registers.h.

## Hardware specifications and block diagram

| Function | Revision A specification |
| --- | --- |
| Processor | STM32F401RET6 on NUCLEO-F401RE, Cortex-M4 |
| Clock | Internal HSI, nominal 16 MHz, restored explicitly before peripheral setup |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Sensors | Four external normally-open, voltage-free low-force switches |
| Acquisition | Nominal 1 ms polling, independent 5 ms contact qualification |
| Input network | 10 kohm pullup and 1 kohm series resistance per channel |
| Connectivity | USART2 transmit, 115200 baud, 8N1, onboard ST-LINK virtual COM |
| Power | Nucleo USB; carrier uses Nucleo 3.3 V and ground only |
| PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm thickness |
| Assembly | Axial through-hole resistors and 2.54 mm pin headers |
| Companion | Python 3 / Tk desktop app for Linux, no pip dependency |

Architecture:

    Contact A -- J3 -- SEAT_A -- 1k -- SENSE0 -- PA0 / A0
    Contact B -- J4 -- SEAT_B -- 1k -- SENSE1 -- PA1 / A1
    Contact C -- J5 -- SEAT_C -- 1k -- SENSE2 -- PA4 / A2
    Contact D -- J6 -- SEAT_D -- 1k -- SENSE3 -- PB0 / A3
                                        |
                                  10k each to 3V3

    USB -> NUCLEO-F401RE -> qualification -> placement trial engine
                         -> CRC-framed transmit queue -> ST-LINK VCP
                         -> Linux app: live contacts / trials / comparison

J1 pin 1 connects to Nucleo CN6 pin 4, 3.3 V. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect to Nucleo CN8 pins 1 through 4, respectively PA0, PA1, PA4 and PB0. These pins are used as digital inputs despite being on the analog header. J3 through J6 have their corresponding seat signal on pin 1 and ground on pin 2. A closed contact reads low electrically and becomes a one in the protocol. A, B, C and D have mask weights 1, 2, 4 and 8.

The default virtual-COM solder-bridge arrangement is required: SB13 and SB14 fitted; SB62 and SB63 open. PA2 uses alternate function AF7 for USART2 TX. Receive is disabled. Verify the board revision against UM1724 before assembly; its connector table and USART section were consulted during design. No carrier connection goes to 5 V, VIN, reset or a powered machine input. Flash through the Nucleo's onboard ST-LINK SWD interface.

## Power, BOM and electrical restrictions

Use one known-good USB supply for the Nucleo. Reserve a conventional 5 V, 500 mA source budget for the development board and debugger rather than claiming a measured operating current. The carrier has no battery, charger, regulator or secondary power inlet. Never connect an external 3.3 V supply while the Nucleo rail is connected; source contention is outside the design.

At nominal 3.3 V, one closed contact draws 0.30 mA through 11 kohm, yielding a nominal MCU-side low voltage of 0.30 V. Four closed contacts draw 1.20 mA and dissipate 3.96 mW in the resistor networks. These are ideal resistor calculations, not physical measurements. Confirm actual resistor tolerances, MCU input thresholds, contact resistance and rail voltage during bring-up. Signal-rated gold contacts are preferable to high-current power switches that need greater wetting current.

| Item | Quantity | Planning guidance |
| --- | --- | --- |
| NUCLEO-F401RE | 1 | Obtain the actual MB1136 board, not a pin-compatible assumption |
| 10 kohm axial resistor | 4 | 1%, quarter-watt class, 7.62 mm lead pitch |
| 1 kohm axial resistor | 4 | Same body and lead-pitch class |
| Two-pin 2.54 mm header | 5 | J1 and J3 through J6 |
| Four-pin 2.54 mm header | 1 | J2 |
| Low-force signal switch | 4 | Choose travel and force for the particular sample |
| Carrier PCB | 1 | Two-layer board with the supplied outline |
| Insulated tray and wiring | 1 set | Add strain relief and protect the underside |

A planning allowance of USD 50–100 for a reusable bench assembly is reasonable as a budgeting assumption, but this is not a supplier quote or an availability claim. Switch mechanics, fixture manufacture and enclosure work can dominate the cost. Use identical switches from the same series for comparative experiments. KiCad's installed footprint-library material retains its upstream license; the authored schematic symbols, connectivity manifest and routing logic are credited to jayis1.

The series resistors do not provide certified ESD, overvoltage or industrial transient protection. There is no galvanic isolation or dedicated TVS network. Limit leads initially to half a meter in a dry indoor environment. All switches must be disconnected from other equipment. The carrier and computer share ground through USB. Do not attach PLC outputs, machine interlocks, mains wiring, powered keyboard contacts or unknown field wiring.

## Mechanical fixture and experimental controls

Install the witness switches under a removable insulating locating insert. Use hard mechanical stops to carry the sample; the switches should report arrival at the chosen threshold rather than become the four load-bearing feet. Adjust switch heights with a reference sample and lock the adjustments. Keep wires below the work area, secure each cable, and label the four corners clockwise so a dropout mask has a consistent physical meaning.

The supplied PCB has no mounting holes; use an insulating board tray or edge supports, not loose metal spacers. A roughly 110 by 100 by 45 mm electronics enclosure allowance is a starting envelope, with the Nucleo adjacent and USB access preserved. The locating insert must be designed for the actual sample. No universal CAD model is supplied for that application-specific insert, and the board outline is not an enclosure fit certification.

Before an experiment, test each contact individually in the Live seating screen. Remove the sample and verify READY. Press all four witnesses and verify the correct bit pattern. Try a known shim beneath one locating stop and verify that its intended corner actually remains open. A disconnected wire is indistinguishable from an open contact; this instrument cannot certify cable health. A stuck closed input prevents the required empty baseline, but it is not diagnosed to a particular component automatically.

Record sample identity, insert revision, switch part, threshold adjustment, operator procedure and environmental conditions alongside exports. Use the same placement motion and do not push harder on one design to make it succeed. Stop if a spring or fixture damages the sample. Contact force can change the very rocking being studied; if results change substantially with switch force, use a lower-force or noncontact method rather than interpreting these measurements as flatness.

## Firmware design and exact timing semantics

The firmware is freestanding C11, with startup vectors, linker script, clock initialization, GPIO input driver, SysTick clock, independent watchdog and bounded nonblocking UART transmission. It does not depend on a vendor SDK, heap allocator, operating system or floating-point library. The Makefile enables warnings as errors and links a real Cortex-M4 image. Clock waits have finite iteration budgets; watchdog initialization waits use bounded millisecond intervals. A failed initialization enters FAULT or stops before initializing unreliable transport.

The observer begins in WAIT. All four contacts must qualify open and remain in the empty baseline for 100 ms before READY. A qualified nonzero vector begins ACTIVE. Each channel requires 5 ms at its candidate raw level; chatter on one input does not postpone another channel. State and durations describe qualified levels, not every electrical edge. A raw glitch shorter than qualification can be invisible to the trial and does not break the measured full-contact interval.

During ACTIVE, the observer accumulates the time spent at mask 15 and the longest contiguous full interval. The first time that interval reaches 250 ms while the incoming vector is still full, it records first_settle as elapsed time from trial start. This milestone is conservative at a simultaneous opening: if a contact qualifies open exactly at the deadline, the interval can measure 250 ms but does not earn the milestone. This explicit ordering policy avoids asserting that seating preceded an opening observed on the same tick.

After the first full vector, qualified falling edges in a nonzero vector count as partial dropouts. Each falling corner increments the count, and its bit is retained in drop_mask. Thus a two-corner loss counts twice. Complete withdrawal to mask zero ends the trial and is not counted as four dropouts. Sequential withdrawal may produce partial dropouts before the final zero; these are genuine recorded contact changes but cannot be distinguished from unintended rocking. Interpret them with the handling procedure, not as automatic defect counts.

A completed withdrawal is SETTLED if the milestone occurred, otherwise UNSETTLED. A 30,000 ms active trial ends as TIMEOUT even when fully seated. At the exact timeout boundary the prior qualified full interval is accounted, but no new milestone or normal removal result overrides timeout. Any sample interval greater than 1 ms invalidates continuity, increments gaps, and ends an active trial as GAP. It requires fresh qualification and a new empty baseline afterward. Unobserved time is never credited as full seating. Duration for a gap result is capped at the trial limit; full_ms remains only accumulated observed time.

Unsigned modular time arithmetic permits ordinary millisecond wrap. A full timer cycle without execution is not distinguishable from shorter elapsed time; the watchdog is intended to reset such a stalled system. Event identity and gap-counter exhaustion enter terminal FAULT rather than wrap silently. All evidence is volatile. Resetting the board clears the trial state and requires an empty baseline; it does not resume a partially observed placement.

## Wire protocol and application interface

Every 50 ms, when the bounded transmit buffer is available, the main loop creates one ASCII snapshot. There is no command receiver, provisioning service, radio or network API. A host cannot start a machine or change firmware settings through this channel. Frames have this format:

    NS1,time,state,mask,gaps,tx,id,reason,duration,first_settle,full_ms,longest,drops,drop_mask,seen*CCCC\n

All numeric fields are unsigned decimal. Time and durations are milliseconds. State is 0 WAIT, 1 READY, 2 ACTIVE or 3 FAULT. Reason is 0 NONE, 1 SETTLED, 2 UNSETTLED, 3 TIMEOUT or 4 GAP. The CRC is CRC-16/CCITT-FALSE, initial value 0xffff, polynomial 0x1021, over the bytes before the asterisk. CCCC contains four hexadecimal digits. Each snapshot repeats the last finished result; id zero means no result yet. There is no boot identifier or durable event archive.

The Python app verifies CRC, version, field count, integer ranges, mask/state consistency and result relationships. It resynchronizes on the complete NS1 marker after truncated transmissions. Duplicate snapshots do not refresh freshness. Counter rollback, time rollback or contradictory repeated results freezes acceptance and requests reconnect. Some resets with lost initial frames and forward-looking counters remain fundamentally indistinguishable; do not treat a receive-only stream as proof of session identity.

The Live seating tab shows qualified corner states and device diagnostics. Trial history retains at most 500 results. Comparison applies a local maximum first-settle delay and requires no partial dropouts; it does not change the fixed firmware dwell requirement. This comparison is an exploratory grouping, not a safety verdict. Replay accepts regular files up to one MiB, rejects device files and FIFOs, and reports incomplete final frames. Replays are always labeled offline and never presented as live measurements.

A reception pause over one second prevents buffered snapshots from restoring LIVE status; explicitly reconnect to begin a new session. The serial port is flushed on open. Disconnect preserves retained evidence and its source label. Reconnect clears the old numeric display and trial history. CSV export exclusively creates a new private file, refuses overwrite, and includes author, provenance, connection/freshness state, parser errors, reset blocking, missing results and evictions even when no trial completed. Export before reconnecting if the previous evidence matters. The host cannot recover intermediate results lost while disconnected because only the latest result is repeated.

## Building, running and verification

From the repository root, run:

    make -C nest-settle/firmware all test
    python3 -m py_compile nest-settle/app/nest.py
    python3 nest-settle/tests/test_app.py
    xvfb-run -a python3 nest-settle/tests/test_gui.py
    /usr/bin/python3 nest-settle/tools/check_hardware.py
    python3 nest-settle/app/nest.py

Install an ARM GNU embedded toolchain, native GCC, Python with Tk, KiCad 9 with its pcbnew Python module, and Xvfb for headless GUI tests. No pip packages are required. Use /usr/bin/python3 for CAD scripts so the system KiCad bindings resolve. The GUI uses Linux termios and is not advertised as a Windows or macOS application. Serial access may require membership in the local serial-device group; do not run the GUI as root merely to bypass device permissions.

The target output is firmware/build/nest-settle.elf and its binary image. With only the intended Nucleo connected, make -C nest-settle/firmware flash uses OpenOCD and ST-LINK to program, verify and reset it. That command has not been executed in this run because no physical device is available. Do not flash another project's board accidentally. Start at the bench with the carrier disconnected, inspect rails, then add each contact channel individually.

The hardware generator is tools/generate_hardware.py. It creates real embedded symbols, library references, net labels, footprint pads, board outline and routed tracks. Run it only when deliberately regenerating the CAD sources. The checker executes KiCad ERC and DRC, exports a fresh schematic XML netlist, and compares every reference, value, footprint and pin/net assignment against the PCB and connectivity manifest. Passive-only ERC cannot verify an external supply, switch mechanics or the Nucleo assembly. The custom schematic intentionally does not pretend the MCU and regulator are on the carrier.

See VERIFICATION.md for the actual tool versions, build sizes and checks from this revision. Host tests use synthetic switch sequences and a real firmware encoder fixture; GUI tests exercise Tk through Linux pseudoterminals. Those are executable software checks, not physical switch captures. Fabrication review, continuity testing, GPIO voltages, oscillator error, switch hysteresis, mechanical disturbance, USB unplug behavior on real hardware, ESD, EMC and regulatory qualification remain future bench work. Do not release fabrication or make precision claims solely because software builds and CAD checks pass.
