# Touch Tour — four-station coverage rounds for dry workflow practice

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype, not a cleaning verifier, production inspection station, medical device or safety interlock. Compilation and electrical CAD checks do not establish physical performance.

## Purpose and useful distinction

Touch Tour is a tabletop practice fixture with four broad, spring-return contact pads. It records a coverage round when each pad has been visited at least once, in any order, with an observed release between visits. It separately records repeated visits to already covered stations and interrupted partial rounds. This makes it possible to compare two physical layouts without prescribing a single correct movement sequence. A total button counter cannot distinguish four different stations from four presses at the same station. A rigid sequence checker incorrectly rejects comfortable alternative routes. Touch Tour retains that difference explicitly.

The intended first application is a dry mock-up of a four-location inspection or wipe route. Place four labelled pads where a future task would have four attention points. A participant uses a dry, blunt, nonconductive practice wand or a finger to press and release the pads. The pad movement acknowledges a visit; it does not detect a wiped area, a visual inspection or completed work. An instructor can compare repeated first-visit orders and repeat counts after changing the physical layout. A human observes the exercise and interprets the notebook alongside comments from the participant.

For example, the route A, B, A, D, C completes one coverage round with one repeat. The first-visit order is A, B, D, C. The route D, C, B, A also completes a round and is not classified as inferior. Holding C indefinitely produces only one visit, not an unlimited stream of coverage. Pressing two pads, or changing directly between pads without a sufficiently observed clear interval, makes the physical correspondence ambiguous and abandons the unfinished round. The participant must release all pads before beginning again.

A workshop can use this instrument to investigate whether labels or spacing encourage returning to the same location while overlooking another. A classroom can use it to teach the difference between counting events, set membership, and completing a coverage cycle. A fixture designer can compare rounded and rectangular layouts. Do not use its numbers to rank employees or diagnose a participant. Shorter routes and fewer repeats are not inherently better; repeats may be intentional and the switches may be awkwardly adjusted.

## Originality, prior art and reuse

The repository was pulled on its configured main branch and all existing device directories and top-level README titles were inventoried before selection. The nearest designs were inspected more closely. Kit Multiset compares three bin declaration multiplicities inside an externally delimited tray episode. Duplex Witness requires exactly one press on each of two captive faces. Age Rack measures relative retrieval rank among occupied pockets, and Grip Lattice measures gated contact-combination durations. Touch Tour instead closes rolling four-identity coverage rounds automatically, allows arbitrary repeat visits without invalidating the round, and retains the first-visit permutation and duration of the latest completion. No tray, captive part, fixed recipe multiplicity or simultaneous-contact dwell measurement is involved.

Coverage counters, pick-to-light acknowledgments and task checklists already exist. A bounded public search also found Hygiena's EnSURE Touch ATP Cleaning Verification System and CDC environmental-cleaning evaluation guidance. These are important contrasts: biochemical surface testing and objective cleaning evaluation measure something that these contact pads do not. Touch Tour does not replace those methods. Its proposed original contribution is a transparent, local, dry-practice coverage notebook with conservative handling of mixed contact envelopes and interrupted acquisition. Worldwide uniqueness, patentability and freedom to operate have not been established.

The processor support, passive carrier topology, serial framing infrastructure, license and driver tests reuse jayis1's Parity Sled platform at repository revision `6aa21c2b75a58425b7f0abe1a00320dfc43e98f9`. `tools/import_platform.py` identifies exactly which files were imported and refuses overwrites. It is an initial provenance record, not a command to run over an existing checkout. The coverage engine, TT1 fields, model, desktop workflow, coverage tests and mechanical proposal are specific to Touch Tour. Electrical reuse is intentional and is not represented as a novel resistor circuit.

Relevant references:

- ST UM1724, Nucleo-64 MB1136 board manual, connector table 16 and virtual-COM solder bridges: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual; the exact addresses used are in `firmware/registers.h` and `firmware/drivers/clock.c`.
- ST PM0214, Cortex-M4 programming manual, SysTick and exception vectors.
- Hygiena ATP system guide: https://www.hygiena.com/documents/104646/healthcare-implementation-guide-for-ensure-touch.pdf
- CDC environmental evaluation guidance: https://www.cdc.gov/infection-control/media/pdfs/Toolkits-Environmental-Cleaning-Evaluation-2010-P.pdf

The ST connector contract is inherited from the inspected platform and manual reference. A fresh download of UM1724 was attempted during this run but the publisher connection failed and then timed out. Do not confuse that failed retrieval with a newly checked manufacturer revision; verify the purchased board against its current manual during physical bring-up.

## Hardware specification

| Function | Revision A choice |
|---|---|
| MCU | STM32F401RET6 Cortex-M4 on external ST NUCLEO-F401RE MB1136 |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz, restored explicitly at startup |
| Sensors | Four normally-open, voltage-free, low-level mechanical contacts |
| Suggested switches | Four Omron D2F-01L lever switches, subject to purchased rating verification |
| Acquisition | Foreground sample at each new nominal 1 ms SysTick |
| Qualification | Five unchanged intervals, six observations per whole-word qualification |
| Connectivity | USART2 TX on PA2 AF7, 115200 baud, 8N1, ST-LINK USB virtual COM |
| Power | USB to Nucleo; its regulated 3.3 V feeds the passive carrier |
| PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Host | Linux desktop, Python 3 with Tk and POSIX serial support |
| Actuation | None; no motors, relays, heater, wireless link or permission output |

The custom PCB is a cable-connected passive carrier, not a complete processor motherboard or mechanically mating shield. The MCU, regulator, decoupling, debugger, reset components and USB bridge are on the separately purchased Nucleo. The KiCad project accurately contains the custom resistors and connectors rather than an unconnected decorative processor. No FPGA is needed for human-paced contacts. No battery, charger, flash card or cloud account is required.

## Architecture and block diagram

    Four spring-return practice pads A / B / C / D
                 | voltage-free contacts to GND
                 v
          J3 / J4 / J5 / J6, pin 1 signal, pin 2 ground
                 | 1k series resistors
                 v
          SENSE0..3, each pulled to 3V3 through 10k
                 | J2, short insulated harness
                 v
       External NUCLEO-F401RE PA0 / PA1 / PA4 / PB0
                 |
       1 ms sampling + raw-envelope ambiguity guard
                 |
       whole-word qualification + all-clear rearming
                 |
       distinct-coverage set / repeat ledger / timeout
                 |
       transactional counters and latest first-visit order
                 |
       TT1 status + CRC-16, USART2 TX -> ST-LINK VCP
                 |
       Linux desktop: coverage / snapshots / wiring
                 |
       owner-only local JSON evidence export

J1 pin 1 is VCC from Nucleo CN6 pin 4, regulated 3V3; J1 pin 2 is ground from CN6 pin 6. J2 pins 1 through 4 connect to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled pins operate as digital inputs. J3 is PAD_A, J4 PAD_B, J5 PAD_C and J6 PAD_D. Every pad connector has signal on pin 1 and ground on pin 2. Logical one means closed switch, despite the electrically active-low input.

The inherited virtual-COM arrangement assumes SB13 and SB14 connected and SB62 and SB63 open. Verify this on the actual board. The UART receiver is disabled, so the companion cannot remotely modify the acquisition state or write firmware. Reset is a physical development-board action and clears all volatile history. SWD programming is through the board's ST-LINK interface, not through the carrier input headers.

## Electrical limits and bill of materials

R1, R3, R5 and R7 are 10k pullups. R2, R4, R6 and R8 are 1k series resistors. Use 1% axial parts fitting the DIN0207 7.62 mm footprint, for example Yageo MFR-25FBF52-10K and MFR-25FBF52-1K. J1 and J3 through J6 are two-pin 2.54 mm headers; J2 is a four-pin header. Samtec TSW-102-07-G-S and TSW-104-07-G-S are possible procurement choices. Confirm lead dimensions and assembly orientation against the footprint before ordering. Four external switches, the Nucleo, insulated harnesses and mechanical pad assemblies are additional system parts, not invisible carrier components.

At nominal 3.3 V, each closed switch draws 3.3 V divided by 11k, approximately 0.300 mA. The MCU-side sense node is approximately 0.300 V. All four contacts closed draw approximately 1.200 mA and dissipate 3.96 mW in the resistor networks. These are ideal calculations, not measured thresholds, wetting-current guarantees, total USB consumption or a noise-margin certification. Use a protected USB source suitable for the Nucleo, reserving 500 mA capacity for the development setup; measure actual assembled consumption before specifying a product supply.

The carrier has no galvanic isolation, TVS protection, fuse or reverse-power protection. Its series resistors do not make external powered signals safe. Connect only isolated, dedicated voltage-free contacts. Do not connect mains, machine wiring, PLC outputs, live appliances or sensors powered from another supply. USB ground becomes common with the fixture. An open lead resembles a released pad and a short resembles a held pad; neither fault can always be identified from the four bits. Use short signal/ground pairs below 0.5 m initially, with strain relief and no exposed conductors near hands.

## Mechanical design and build sequence

Use a rigid dry tabletop base approximately 320 by 240 mm. Mount four separate pad modules on adjustable slots, starting with centres on a 160 by 120 mm rectangle. Each pad can be approximately 45 mm square with rounded corners and a compliant top. Leave enough clearance that a broad finger or wand cannot unintentionally press two neighbours. Slot adjustment is the experimental variable, not a claim of ergonomic optimization. These dimensions are a prototype construction specification, not a supplied mechanical CAD model or a validated human-factors recommendation.

Each module has a pivoting or vertically guided paddle that operates one D2F-01L lever switch. Provide a hard mechanical stop independent of the switch so hand force cannot overload the lever. Keep pinch regions guarded and fasteners captive. Adjust actuation within the manufacturer's permitted travel; do not bend the lever casually to compensate for a bad bracket. Verify that the pad returns fully and its contact opens without a participant lifting it manually. Number both ends of every cable and keep leads clear of moving parts.

Enclose the carrier in an insulating edge-retention housing with at least 100 by 90 mm internal clearance. The PCB has no mounting holes; do not drill through routed copper. Put the Nucleo in a separate ventilated insulating enclosure with USB and reset accessible. Connect ground and power only after checking harness continuity with the electronics unpowered. Verify 3V3 polarity, resistor placement and switch isolation before inserting the USB cable. No ingress, cleaning-chemical, drop, ESD or environmental rating is claimed.

This is dry practice only. Do not apply disinfectants, water, solvent, abrasive dust or food residues to the pads. A practice wand should be blunt, light and nonconductive. Use no powered tool. The four contacts do not establish a swept path or area coverage: a point press is enough to register a visit even if the rest of a simulated surface was untouched. Record this limitation in every experiment description.

## Firmware measurement contract

The engine accepts one four-bit contact observation with a monotonically advancing modular 32-bit timestamp. A whole-word qualifier requires five unchanged intervals after the candidate first appears. Thus six observations establish a new endpoint. Qualification is deliberately bundled because the identity is the entire mutually exclusive pad word. Chatter in another pad may prevent a usable visit; it must not synthesize a cleaner identity than the actual raw envelope.

At boot, held pads earn nothing. A qualified zero word establishes all-clear and arms one visit. The firmware accumulates the union of every raw pad bit seen since that clear baseline. If more than one identity appears before another complete clear window, the envelope is ambiguous even when the two pads were never closed simultaneously. One disruption is counted for that envelope, any unfinished round is abandoned, and no new visit is possible until all-clear qualifies again. A brief rejected zero glitch is not a release window and does not erase the union.

An armed, unblocked, qualified one-hot word adds one visit to that pad. The first such visit starts a round with age zero. If the pad was already present in the round's membership set, repeats increases and the set stays unchanged. Otherwise the pad's identity is appended to a compact first-visit order and its membership bit is set. Once all four bits are present, rounds increases and the final age and permutation are retained. The active set, working order and age then clear, but the held final pad cannot immediately start another round. It too requires a new all-clear window.

Active age counts acquired one-millisecond intervals from the first accepted visit. A rejecting deadline of 60000 ticks abandons the round before processing any new qualifying visit at that same timestamp. A completion just before that boundary is allowed; a completion exactly on it is not. A gap in acquisition abandons the active round, records missed ticks, restarts qualification and requires a new clear baseline. Unobserved time is never silently credited as continuous contact. Completed history survives gaps; partial coverage does not.

Round accounting obeys `starts = rounds + aborted + active`. Visit accounting obeys `sum(visits) = 4*rounds + popcount(mask) + discarded + repeats`. Discarded counts distinct first visits lost with abandoned rounds, not repeats. Every abandoned active round has one to three such visits. Counters are updated on a temporary engine copy and committed together only if all additions fit. Overflow terminates acquisition with a sticky fault and conservatively abandons the prior active round. Time, input and initialization faults have the same terminal-accounting behavior.

The startup vector and linker script target 512 KiB flash at 0x08000000 and 96 KiB SRAM at 0x20000000. The code restores the HSI clock, configures GPIO and USART2, enables SysTick and starts an independent watchdog. The nonblocking serial driver uses a fixed buffer and a bounded transmission timeout. Firmware has no dynamic allocation, operating system or vendor HAL dependency. Fault codes are configuration 1, watchdog setup 2, time 4, overflow 8 and invalid input 16. They combine as sticky bits. A failed early clock restoration stops before interface initialization.

## Protocol and application

Every nominal 100 ms the production loop attempts one `TT1` status line. The body is the marker followed by 26 comma-separated unsigned decimal values. Fields, in order, are now, samples, missed, tx_errors, fault, raw, qualified, valid, armed, starts, rounds, aborted, repeats, discarded, disruptions, timeouts, active, mask, age, order, last_age, last_order, visit_a, visit_b, visit_c and visit_d. The suffix is an asterisk, four uppercase hexadecimal CRC digits and LF. CRC is CRC-16/CCITT-FALSE over the body before the asterisk, initialized to 0xffff with polynomial 0x1021. No NUL terminator is sent.

The compact order stores each first-visited pad as a two-bit identity, earliest in the least-significant bits. A=0, B=1, C=2 and D=3; use membership population to decode the partial order, and exactly four identities for a completed order. The complete alphabetic permutation encodes as 228. A maximum-width frame remains below the 1024-byte transport buffer. CRC detects accidental corruption, not malicious substitution or authenticated origin. There is no command channel, encryption, user identity or trusted boot identifier.

Run the companion with `/usr/bin/python3 touch-tour/app/workbench.py` from the repository root. Choose a serial device such as `/dev/ttyACM0` and connect. Linux serial permissions must permit access; do not run the GUI as root merely to bypass permissions. The Coverage screen shows rounds, repeats, partial membership, latest complete order, nominal duration and interruption diagnostics. Snapshots shows every retained row, up to the newest 1000. Wiring and limits provides the connector map and measurement warnings. Disconnect preserves evidence; connecting successfully starts a new host notebook. A failed connection does not erase old evidence.

Replay accepts a regular raw TT1 recording no larger than 2 MiB and rejects malformed frames, invalid accounting and incomplete trailing data. Replay is visibly not live. Export writes a new owner-only JSON file, refuses overwrite, and includes author, source provenance, connection state, warning state, parser errors and retained snapshots. Export JSON is an analysis artifact, not a raw TT1 replay file. No network service receives data. Delete local exports using normal operating-system file management when no longer needed. The application does not claim secure deletion or encrypted storage.

Live acquisition requires advancing sample evidence, not just newly received bytes. More than one second without acquisition progress latches a reconnect requirement. A cumulative host/device progress difference greater than 0.75 seconds also blocks the session, conservatively detecting backlog or clock jumps; oscillator drift can eventually trigger this check during long sessions. A reset with apparently forward timestamps and counters may remain indistinguishable without a persistent boot identity. Disconnect and reconnect after any known reset. Sparse snapshot checks impose necessary conservation and rate bounds, not proof of every hidden visit's chronology.

## Reproduction, validation and remaining engineering work

From the repository root, run `make -C touch-tour/firmware all test fixture`. The target is compiled and linked with arm-none-eabi-gcc, warnings are errors, and host engine tests use address and undefined-behavior sanitizers. The fixture generator emits explicitly synthetic contacts into `firmware/build/demo.tt1`. Open that recording with the desktop's Replay action or pass `--replay touch-tour/firmware/build/demo.tt1`. To exercise the real Tk event loop without a display, run `xvfb-run -a /usr/bin/python3 -m unittest discover -s touch-tour/tests -p 'test_*.py'`.

Generate the native board and schematic with `/usr/bin/python3 touch-tour/tools/generate_hardware.py`, using KiCad 9 Python bindings and installed standard footprint libraries. Run `/usr/bin/python3 touch-tour/tools/check_hardware.py` for ERC, DRC, unconnected-item checks and exact schematic-to-PCB-to-manifest pin comparison. The project includes authored passive component symbols, actual standard footprints, routed copper and a closed outline. Passive ERC does not verify the external regulator, switch mechanics or the purchased module. Render the schematic and inspect label readability in addition to electrical checks.

The firmware tests enumerate every eight-visit word against an independent set-membership oracle, exercising repeated identities, arbitrary first-visit order and successive rounds. Boundary tests cover held startup, held completion, raw mixed envelopes, sequential hand exchange, gaps, exact timeout, clock wrap, counter exhaustion and bounded serialization. Application tests consume the actual C encoder's fixture, including sparse subsequences, corrupt and truncated frames, rollback, stale reception, private export and overwrite refusal. GUI tests exercise all screens, serial data through a Linux pseudo-terminal, replay replacement and hangup. These are synthetic software tests, not physical trials.

Before using an assembled prototype, verify supply voltage and current, contact low/high levels, USB common ground, switch repeatability, watchdog behavior and actual sample timing with appropriate instruments. Compare at least several complete and interrupted routes against an independently annotated video, with participant consent and a deletion plan. Test sensitivity to pad spacing, cable movement and switch bounce. Inspect a fabrication output against the board before ordering. Physical fabrication, flashing, mechanical repeatability, field robustness and actual user benefit remain unverified; this repository delivers the design and executable software, not a manufactured or certified instrument.
