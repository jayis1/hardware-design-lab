# Rail Skew — paired drawer progression notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application and locally authored hardware design sources are MIT licensed; see LICENSE. Installed KiCad footprint material retains its upstream licensing.

Revision A is a build-tested engineering prototype. Its custom PCB is a routed passive interface for a separately purchased NUCLEO-F401RE. The microcontroller, regulator, decoupling, debugger and USB bridge are on that development board, not on the carrier. No PCB has been fabricated, target flashed, drawer fixture assembled or physical measurement calibrated during this design run. Threshold disagreement is not a measurement of force, dimensional squareness or safe operation.

## Purpose, user and practical outcome

A furniture prototype maker adjusting a wide, light drawer can feel a hesitation without knowing whether the left rail consistently reaches the next region first or whether the pull itself is asymmetric. Looking at the front hides the rails. Looking at one rail misses the other. A tape measure can reveal static alignment but cannot preserve how the two sides progress during a complete hand-operated opening and closing excursion. Rail Skew provides a removable contact fixture and a shared-clock notebook for that narrow workshop experiment.

Two maintained threshold witnesses attach to each side. The first reports that its rail has passed an earlier reference position; the second reports a later reference position. Each side therefore occupies one of three stages: before both thresholds, between thresholds, or beyond both. Firmware measures how long the sides are observed in matching stages, one stage apart, or two stages apart, separating opening-origin and closing-origin traverses. It also records completed excursions, returns to origin, peak stage difference and the most recent completed duration.

The intended user is a cabinet prototyper or woodworking educator comparing rail placement or pull-handle location on an unpowered bench mockup. A useful experiment is ten slow excursions with a centered pull followed by ten with an off-center pull, resetting and exporting between conditions. The quantitative output is six millisecond coverage bins and separate opening, closing and abandonment counts. The design target is to replace manual annotation of those six categories, not to promise a measured time saving or declare a drawer correctly installed.

Hardware is justified because four hidden contact states need simultaneous acquisition without requiring the operator to touch a phone at every threshold. A camera can recover richer information and remains the preferred commissioning reference, but needs sight lines to both sides and subsequent interpretation. The custom carrier makes wiring repeatable and accessible for continuity testing. The companion makes the evidence understandable and exportable without a cloud service, account, camera, microphone or wireless pairing procedure.

## Originality and alternatives

The repository directories and README descriptions were inventoried before choosing this design. Crossover Track separately times two carts and guides an ABBA lane-swap experiment. Rail Skew instead observes two sides of the same constrained drawer and integrates disagreement between maintained position stages in both directions. Nest Settle records placement at four corners, not progression along two rails. Bellows Phase conditions valve positions on stroke direction; it has no paired ordinal position code. HingeScribe studies hinge movement and force, while CrisperCue concerns food conditions inside a refrigerated drawer. None is this particular rail-stage comparison workflow.

Three candidates were considered. A simple drawer-open counter was rejected as too generic to justify another instrument. An encoder-based rail displacement difference meter was rejected because the repository already contains differential travel tools and because it would imply a stronger dimensional calibration requirement. The selected maintained-threshold approach is deliberately coarse, but supports a reproducible comparative experiment with visible uncertainty and readily inspected sensors.

Position switches, data loggers, synchronized video and drawer test fixtures are established technologies. No global novelty, patentability or freedom-to-operate claim is made. The original contribution here is the bounded rail-stage experiment, its bundled qualification policy, conservative coverage accounting, evidence protocol and local application. Shared processor support and passive circuitry are intentionally reused from jayis1's bellows-phase at commit 0192ec5; tools/import_platform.py records the exact source set and refuses overwrite. Circuit reuse is not represented as an electrical invention.

## Hardware specifications

The processor is STM32F401RET6 on ST NUCLEO-F401RE: Cortex-M4, 512 KiB flash and 96 KiB SRAM. The firmware uses the internal nominal 16 MHz HSI clock, explicitly restoring the reset clock tree before starting peripherals. SysTick provides nominal millisecond acquisition timing. USART2 transmits at 115200 baud, eight data bits, no parity and one stop bit through PA2 alternate function seven to the ST-LINK virtual COM port. Receive is disabled. There is no radio, FPGA, battery, charger, actuator, removable storage or external ADC.

The custom board is 80 by 70 mm, nominally 1.6 mm FR-4 with two copper layers. Through-hole axial resistors and 2.54 mm headers support manual assembly and repair. Routes use 0.30 mm tracks, the default clearance is 0.20 mm, and the single ground via has a 0.80 mm diameter and 0.40 mm drill. The supplied KiCad schematic has real locally authored symbols and numbered pins; PCB footprints contain real connected pads. The board is an interface, not a standalone computer.

Each external sensor is a maintained, normally open, voltage-free contact. A candidate sensing element is Littelfuse MDSR-7 with a captive magnet on a low-force mechanical follower. The follower rides an insulated cam that moves the magnet into the reed's qualified closure region after the threshold and keeps it there until reverse travel passes that threshold. A narrow magnet simply passing a reed produces a pulse and is not suitable: the engine requires a maintained thermometer code. The switch, magnet, cam height and follower return spring require assembled characterization before selecting final production parts.

## Architecture and fixed pin contract

    left early cam  -> LEFT_1  -> J3 -> 1k -> SENSE0 -> PA0
    left late cam   -> LEFT_2  -> J4 -> 1k -> SENSE1 -> PA1
    right early cam -> RIGHT_1 -> J5 -> 1k -> SENSE2 -> PA4
    right late cam  -> RIGHT_2 -> J6 -> 1k -> SENSE3 -> PB0
                                      each SENSE has 10k to 3V3
                                      each field pin 2 is GND

    USB -> NUCLEO-F401RE -> 1 ms sampling -> per-rail qualification
          -> traversal state -> six coverage bins + retained result
          -> CRC ASCII records -> ST-LINK USB virtual serial
          -> local Linux/Tk: setup / progression / history and CSV

J1 pin one is VCC, connected only to the Nucleo 3V3 source at CN6 pin four. J1 pin two is ground, connected to CN6 pin six. J2 pins one through four connect to Nucleo CN8 pins one through four: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. Those analog connector labels identify digital inputs here. The square pad identifies pin one on each header; never infer orientation from a cable's color alone.

| Bit | Field connector | Net | MCU input | Resistive path |
|---|---|---|---|---|
| 0 | J3 pin 1 | LEFT_1 | PA0 | R1 10k pullup, R2 1k series |
| 1 | J4 pin 1 | LEFT_2 | PA1 | R3 10k pullup, R4 1k series |
| 2 | J5 pin 1 | RIGHT_1 | PA4 | R5 10k pullup, R6 1k series |
| 3 | J6 pin 1 | RIGHT_2 | PB0 | R7 10k pullup, R8 1k series |

The processor-side SENSE net connects to the pullup and the series resistor. An ideal closed field contact draws 3.3/11000 amperes, nominally 0.300 mA, leaving approximately 0.300 V at the MCU pin. Four closed contacts draw approximately 1.20 mA. These are ideal circuit calculations rather than measurements. Internal MCU pull resistors are disabled. No unmatched filtering capacitors are used to avoid silently creating channel-dependent delay.

## Power, bill of materials and restrictions

Power the Nucleo through its normal USB connection. Reserve 150 mA at 5 V as a planning allowance for the development board and add the small carrier load on its 3.3 V rail. Measure the assembled current before choosing a final supply; this is not a battery-life specification. The carrier has no regulator or second intended source. Never connect an external supply to J1 while the Nucleo drives that rail.

The bill of materials is one NUCLEO-F401RE, four 10k and four 1k axial resistors, five two-pin headers, one four-pin header, one carrier PCB, four witness assemblies, insulated wiring, a USB data cable and an enclosure. Resistors are 1%, at least 0.125 W, in the supplied DIN0207 body and 7.62 mm pitch footprint. Samtec TSW-102-07-G-S and TSW-104-07-G-S are candidate header families; verify mating height and footprint dimensions before purchasing. The MDSR-7 glass reed needs a protective holder and strain relief, not an exposed installation beside a moving rail.

Allow roughly USD 50–100 for controller, small-quantity carrier, passives, harness and contacts as a budgeting assumption, not a current quotation. The drawer, cam strips, brackets, enclosure fabrication, tools, labor and shipping are excluded. Sensor mechanics may dominate both cost and uncertainty. The low contact current means ordinary oxidized power switches are not interchangeable with signal-rated contacts. Final switch reliability must be tested at the actual excitation current.

Only disconnected voltage-free switches are allowed. Never connect powered machine inputs, 5 V modules, PLC wiring, mains, motor terminals or existing cabinet lighting circuits. There is no isolation, TVS protection or certified hot-plug behavior. A 1k resistor is not an overvoltage barrier. USB and field grounds are common. Begin with contact cables shorter than half a metre and a dry indoor bench around 10–35 degrees Celsius; these are test conditions, not a qualified environmental rating.

Manufacturer references for the reused platform are ST UM1724 for NUCLEO-64 MB1136, RM0368 for STM32F401 registers, and PM0214 for the Cortex-M4 core. The VCP assumption is SB13/SB14 connected and SB62/SB63 open; verify the actual purchased board revision before wiring. This run rechecks target compilation and simulated register behavior, not the external board's manufacture or supply integrity.

## Mechanical arrangement and onboarding

Use a lightweight captive drawer with hard end stops and no powered movement. Attach two parallel cam strips to each moving side, with early and late transitions separated far enough to create a clearly visible middle region. Each cam must maintain its raised state through the rest of opening travel. Both threshold positions should be matched across the drawer using a physical reference, not by adjusting them until the app displays a desired answer. On closing, the late contact releases first and the early contact releases second.

Put the sensing assemblies on the fixed frame, outside pinch points. Low-force followers carry magnets while fixed reeds remain protected. Hard stops carry the drawer load; switches and followers must not become structural stops. Retain every magnet and fastener, keep cables out of the rail path, and stop if mounting changes the drawer motion noticeably. An enclosure approximately 160 by 110 by 45 mm can hold carrier and Nucleo adjacent; this is a packaging allowance, not released enclosure CAD. The carrier has no mounting holes, so use insulated edge supports.

First-use flow has four steps. First, inspect the unpowered board, continuity and resistor values. Second, connect only the Nucleo supply and test every contact individually on the application. Third, move each rail slowly and confirm its local code sequence is 00, 01, 11 and the reverse, with no sustained 10. Fourth, place both sides before both thresholds, confirm READY, and record slow full excursions while filming the witnesses. Use contact regions held for at least 20 ms and initially take at least one second per excursion.

A broken wire can look like a legitimately open contact. A stuck contact may prevent an endpoint or produce an invalid code, but there is no complete wiring diagnostic. Compare every channel with visual motion before each experiment. Record cam spacing, threshold positions, rail installation, load, pull position and operator procedure separately. Do not compare records made with different threshold geometry as though they share a calibrated coordinate system.

## Firmware decisions and measurement semantics

Each rail is a two-bit bundle. Legal local codes are binary 00, 01 and 11, mapped to stages zero, one and two; binary 10 is invalid. Bits zero and one belong to the left rail; bits two and three belong to the right. Five consecutive identical millisecond samples qualify each bundle independently. The first sample counts as one, so the observed qualification span is four intervals. Bundling prevents different per-bit debounce histories from synthesizing a forbidden code, while independence prevents chatter on one rail from delaying the other rail's qualification.

WAIT is state zero. Once both rails are known and raw agrees with qualified data, all-zero or all-one input establishes READY, state one. Anchor one means the closed reference, anchor two the open reference. A changed qualified vector begins ACTIVE, state two, increments starts and captures the origin direction. A direct change to the opposite endpoint counts an abandoned attempt because no intermediate progression was observed. The instrument does not infer a missing traverse from endpoint changes alone.

During ACTIVE, duration advances each consecutive acquisition millisecond. Absolute stage difference selects one of three bins for the captured direction. A bin earns an interval only when both adjacent raw vectors equal their qualified vectors, both qualified vectors are identical, and the vector is neither complete endpoint. This excludes qualification boundaries and sampled raw glitches rather than crediting them retrospectively. Bins are lifetime observed intervals including attempts later abandoned. They are not the entire wall duration or exclusively completed-traverse statistics.

Peak stage difference is the maximum qualified difference during the current traverse, including its departure vector. It is ordinal, not millimetres. Reversal within the middle region is allowed; direction continues to mean the origin of the excursion, not instantaneous velocity. Returning to the origin abandons the attempt. Reaching the opposite endpoint completes it only if the raw vector agrees, records duration, direction and peak, and permits immediate rearming at that endpoint. Thus an opening can be followed immediately by a closing.

A duration of 60000 ms abandons the attempt; timeout takes precedence over an arrival on the same sample. A sustained invalid code or any skipped acquisition millisecond enters HALTED, state three. An active attempt is abandoned exactly once before halting. A short invalid raw glitch below qualification need not halt but removes coverage. Counter updates are transactional: exhaustion discards tentative changes before terminating, preserving starts = openings + closings + abandoned + one if ACTIVE. Halted accounting remains frozen while time, raw inputs and UART diagnostics may advance.

The target is freestanding C11, with real startup, linker script, clock, GPIO, SysTick, USART and independent-watchdog support. There is no heap, operating system, float dependency or simulated target input. UART submission is bounded, nonblocking and expires after 100 ms; transport failures increment a diagnostic rather than pretending the host received a result. Initialization faults halt measurement. Watchdog timing depends on the uncalibrated LSI oscillator. Reset clears all volatile evidence and does not resume an earlier excursion.

## Protocol and companion application

RS1 appears on every periodic record, so a lost boot line does not prevent late attachment. Records are emitted approximately every 100 ms. Each starts with RS1 and comma-separated unsigned decimal values in this exact order:

    time,raw,levels,known,state,anchor,starts,opening,closing,aborted,active_ms,
    bin0,bin1,bin2,bin3,bin4,bin5,peak,last_ms,last_dir,last_peak,flags,gaps,tx

Append an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection or final XOR, over all bytes before the asterisk. Time is modular uint32 milliseconds; durations and bins are milliseconds. Bins zero through two describe opening, three through five closing, with stage differences zero, one and two respectively. Last direction zero means opening and one closing. Flag bits are one acquisition gap, two invalid code, four exhaustion, eight configuration failure and sixteen watchdog initialization failure. The complete line fits below 512 bytes.

The appropriate companion is a Linux Python/Tk desktop application with no third-party Python dependencies. Connect / Setup discovers common USB ports, accepts a path, connects, disconnects and loads replay. Rail progression displays raw and qualified bits, state, counters, bins and the retained result. History / Export shows the latest 200 snapshots and writes retained evidence to a new CSV. Fixed policy and physical thresholds make a software configuration screen misleading, so the setup screen explains installation rather than sending unsupported commands.

Launch python3 app/workbench.py from this directory. Use a normal desktop account with serial-device permission, not root to bypass permissions. The serial transport configures 115200 8N1, flushes pending input on connect and restores original terminal settings on close where possible. A second connect request cannot silently reopen the owned port. Failed replacement connections preserve evidence. Reconnect clears the previous numerical display. Validated replay replaces a session only after the whole input has passed checks.

Replay is restricted to regular files at most two MiB, rejects empty input and partial final records, and never receives a LIVE label. Framing resynchronizes on the full RS1 marker. The model rejects CRC-valid impossible counts, invalid codes without faults, disappearing flags, counter rollback and changed retained results without completion. These are necessary consistency checks, not a proof of every unseen input history. Storage retains 6000 snapshots with eviction metadata; the latest completed result is not a lossless event archive.

LIVE requires a connected port and two advancing device timestamps. Duplicate data does not refresh progress. More than one second without progress is stale; a reception pause over one second or cumulative host/device disagreement over 750 ms blocks resumed interpretation until reconnect. HSI drift can conservatively reject long captures. There is no persistent boot identifier, so a reset with compatible forward-looking counters may remain undetectable. Known reset or wiring changes require a deliberate new session. CRC detects corruption, not malicious input or authenticity.

CSV export uses exclusive creation and mode 0600, refusing overwrite. Even an empty export includes author, original live/replay provenance, connection state, freshness, parser-error count, warning, eviction count and age. Disconnect retains evidence and provenance rather than relabeling it current. Export before replacing a session. Data remain local unless the operator explicitly shares the file.

## Build, verification and remaining physical work

Run make -C firmware clean all test to link the actual Cortex-M4 ELF and binary and execute engine, clock and UART tests under AddressSanitizer and UndefinedBehaviorSanitizer. Run xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v for protocol, replay, export and real Tk/pseudoterminal tests. Synthetic fixtures are produced by the actual C encoder, not substituted for physical measurements. Build products are ignored.

With KiCad 9 and its system Python bindings, /usr/bin/python3 tools/generate_hardware.py regenerates the carrier. /usr/bin/python3 tools/check_hardware.py runs ERC, DRC, fresh schematic netlist export and exact reference/pin/net/value/footprint comparison against the PCB and manifest. Passive-only ERC cannot validate an external supply or cam fixture. The source generator must remain consistent with any deliberate CAD edits.

The explicit operator command make -C firmware flash uses OpenOCD and the Nucleo's ST-LINK. It has not been run in this design session. Before assembly release, review footprints and connector orientation, inspect fabrication output, measure open/closed voltages, compare timing against a reference source, and verify watchdog reset and real USB unplug behavior. Then compare every stage transition with synchronized video and test threshold hysteresis, cam wear, sensor drag and cable noise. ESD, EMC, physical accuracy, durability and regulatory qualification remain unverified. VALIDATION.md separates executed software/CAD checks from those future bench activities.
