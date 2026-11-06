# Age Rack — cohort-aware retrieval-rank notebook for four sample pockets

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, tests and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive interface for a separately purchased NUCLEO-F401RE. The MCU, regulator, reset network, decoupling, USB bridge and debugger belong to that development board. Software execution and electrical CAD checks are described in VALIDATION.md. No physical assembly, flashing, environmental qualification, specimen testing or manufacturing readiness is claimed.

## Purpose and practical utility

Age Rack is a four-pocket rack that records which sample carrier was retrieved relative to the other carriers still present. It is meant for small teaching experiments where retrieval order matters more than absolute elapsed time: a paper-coating demonstration, staged model-paint handling with already dry samples, or a workshop exercise comparing explicit oldest-first selection against informal selection. The instrument measures a narrowly defined fact: at the observation of a removal, how many carriers had earlier observed placements, and how many were indistinguishably tied with the removed carrier?

The central result is an age-rank interval. Rank zero means oldest among the inventory immediately before the removal group, not a minimum elapsed age. A removal with rank two had two strictly older observed carriers ahead of it. If two carriers arrived in the same qualified observation, their relative chronology is unknown. The notebook retains that uncertainty instead of assigning order according to a GPIO pin number. A tied removal can therefore be reported as rank zero through one. Unambiguous removals accumulate in four rank bins; ambiguous removals accumulate separately rather than being silently rounded toward oldest-first behavior.

A practical demonstration starts with empty pockets A through D. Insert A, then B, then C. Retrieve B while A and C remain. The notebook records rank one: A was older and C was younger. Put another carrier in B and retrieve A. That removal has rank zero despite B being numerically earlier than C in a label list. The age ordering comes from observed placements, not fixed pocket numbers, paper labels or a remembered schedule. A pocket may be reused while others remain occupied; a full drain is not necessary between observations.

The target audience is a maker-space educator, a small materials-workshop instructor, or a fixture designer studying how people select from a small work-in-progress rack. The output can reveal whether physical pocket arrangement influences selection. It must not be used to score employees or infer that a participant behaved incorrectly. A younger sample may have a perfectly legitimate reason to be selected. This is an observational notebook with transparent rules, not a policy enforcement system.

## Originality and limits of the concept

The repository directories and README titles were inventoried before choosing Age Rack. The complete purpose and measurement contracts of the closest designs were then inspected. Teardown Stack requires reverse-order retrieval of uniquely docked parts cups and abandons its sequence on a wrong removal or simultaneous change. Age Rack permits every removal, preserves continued rolling inventory, and reports cohort-aware rank intervals rather than passing or failing a stack policy. Rest Quartet measures independent residence durations against a fixed interval and can recover individual empty pockets after gaps. Age Rack measures relative placement chronology, has no cure threshold or residence timer, and halts the whole rank ledger when acquisition continuity is lost.

Ordinary FIFO racks, numbered trays, paper logs, stopwatches and camera annotation are existing alternatives. A logic analyzer plus a custom program could implement this experiment. The original contribution here is the specific removable-carrier workflow, cohort interpretation of simultaneous placement, pre-group interpretation of simultaneous removal, portable rank engine, and local interface. Reusing resistor networks and processor support is intentional and not itself an invention. No exhaustive commercial survey, patent search, worldwide novelty claim or freedom-to-operate opinion is supplied. Before investing in production, test whether the extra automatic observations improve an actual workshop decision over simply labeling the carriers.

The electronics observe pocket contacts, not specimens. They cannot tell whether a carrier is empty, whether a coating was applied, whether two carriers were exchanged without opening a switch, or whether an item is safe to handle. Rank is not cure, temperature, drying, sterility, specimen identity, compliance, or elapsed residence. In particular an apparently oldest carrier may have been present only briefly; the system does not even calculate the duration. Keep independent sample labels and material-specific procedures alongside the notebook.

## Hardware specification

| Function | Revision A implementation |
|---|---|
| Processor | STM32F401RET6 on ST NUCLEO-F401RE MB1136 |
| Core and memory | Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Restored internal HSI, nominal 16 MHz |
| Contacts | Four normally open, voltage-free pocket switches |
| Suggested sensor | Omron D2F-01L signal-level lever switch; verify purchased rating |
| Sampling | Nominal one millisecond, foreground acquisition |
| Qualification | Five consecutive identical samples independently per contact |
| Host link | USART2 TX, PA2 alternate function 7, ST-LINK virtual COM |
| Serial parameters | 115200 baud, eight data bits, no parity, one stop bit |
| Power | USB to Nucleo; Nucleo regulated 3.3 V to passive carrier |
| Custom PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Carrier assembly | Through-hole resistors and 2.54 mm headers |
| Application | Linux Python 3/Tk, standard library, no Internet service |
| Outputs | Telemetry only; no motor, relay, heater, lock or permission signal |

The board is not a mechanically mating Nucleo shield. Short insulated leads connect the development board headers to the carrier. Firmware treats the analog-header pins as ordinary digital inputs and disables their internal pulls because external resistors establish the bias. The UART receiver is disabled. There is no radio, battery, charger, FPGA, external storage, sensor bus or remotely writable configuration.

### Connector and net contract

| Connector | Pin / net | Destination or meaning |
|---|---|---|
| J1 | 1 VCC | Nucleo CN6 pin 4, regulated 3V3 |
| J1 | 2 GND | Nucleo CN6 pin 6, common ground |
| J2 | 1 SENSE0 | CN8 pin 1, PA0 / A0 |
| J2 | 2 SENSE1 | CN8 pin 2, PA1 / A1 |
| J2 | 3 SENSE2 | CN8 pin 3, PA4 / A2 |
| J2 | 4 SENSE3 | CN8 pin 4, PB0 / A3 |
| J3 | 1 RACK_A; 2 GND | Pocket A switch |
| J4 | 1 RACK_B; 2 GND | Pocket B switch |
| J5 | 1 RACK_C; 2 GND | Pocket C switch |
| J6 | 1 RACK_D; 2 GND | Pocket D switch |

Each SENSE node has a 10k resistor to 3V3 and a 1k series resistor to its external RACK contact. A closed switch connects RACK to ground. The MCU inverts the active-low electrical input so a one bit means occupied; A is bit zero and D is bit three. The 10k is on the processor side of the 1k. At ideal nominal 3.3 V, the resulting closed-channel current is approximately 0.300 mA and the processor-side node approximately 0.300 V. Four closed channels draw approximately 1.20 mA. These are nominal calculations, not measured logic margin or wetting-current guarantees.

The design basis is ST UM1724 for Nucleo-64 MB1136 connectors and ST-LINK routing, RM0368 for STM32F401 registers, and PM0214 for the Cortex-M4 core. Check the exact purchased board revision and manufacturer documents before assembly. The inherited VCP arrangement assumes SB13/SB14 connected and SB62/SB63 open. The freestanding register definitions and direct drivers are included rather than relying on a vendor HAL download. They have software tests but still require physical bring-up on the selected target.

## Architecture and block diagram

```text
Carrier A tab -> dry contact J3 -> RACK_A -> 1k -> SENSE0 -> PA0
Carrier B tab -> dry contact J4 -> RACK_B -> 1k -> SENSE1 -> PA1
Carrier C tab -> dry contact J5 -> RACK_C -> 1k -> SENSE2 -> PA4
Carrier D tab -> dry contact J6 -> RACK_D -> 1k -> SENSE3 -> PB0
                                                     |
                                              each: 10k to 3V3
                                                     |
USB power -> NUCLEO-F401RE 3V3/GND -------------------- J1
                 |
                 +-> millisecond sampling -> four independent qualifiers
                 |                                 |
                 |                       empty startup baseline
                 |                                 |
                 |                      live cohort stamp inventory
                 |                                 |
                 |                   pre-change removal rank intervals
                 |                                 |
                 +-> bounded CRC UART -> ST-LINK VCP -> Linux/Tk notebook
                                                       ranks / trace / wiring
                                                       strict replay / CSV
```

The MCU owns the measurement history. The app is a receiver and validator, not an alternative engine controlling the rack. Disconnecting the computer application does not stop firmware accounting while USB power remains. Resetting or losing power destroys volatile history. There is no persistent experimental identity, no flash journal and no hidden attempt to reconstruct chronology after a restart. Export evidence before deliberately resetting.

## Mechanical fixture, BOM and power

Use four shallow, individually labeled pockets on an insulating base. A starting arrangement is two rows of two pockets for approximately 45 by 65 mm removable carriers with generous finger clearance. Give every pocket a rigid rear stop and side guides. A small carrier tab operates its switch close to the fully seated position. The rigid stop, not the switch, must carry insertion force. Slotted brackets allow adjustment so contacts close repeatably without overtravel. These are prototype fixture dimensions and construction requirements, not a fabricated or tolerance-validated enclosure model.

Use identical tabs and switch geometry across pockets. Differences in actuation position or bounce can change which placement qualifies first. The contact must sense the carrier, not apply force to a wet coating or fragile specimen. Keep electronics and wiring in a separate dry compartment. Use inert practice coupons for commissioning. No heating, solvents, reactive chemicals or special exposure process is needed to demonstrate the instrument. Material safety remains governed by the relevant procedures, never by this notebook.

The electrical BOM contains fourteen carrier components: four 10k resistors, four 1k resistors, five two-pin headers and one four-pin header. Use one-percent axial resistors rated at least 0.125 W with DIN0207, 7.62 mm pitch footprints. Add a NUCLEO-F401RE, four low-level switches, a USB data cable, insulated mating leads, strain relief and the hand-built pocket assembly. The machine-readable component list and exact pin nets are in kicad/connectivity.json. The KiCad schematic uses locally authored resistor and connector symbols, actual footprint identifiers, named wires and net labels; the PCB routes the corresponding nets on both copper layers.

The carrier has no mounting holes. Retain it using insulating edge rails or supports, without drilling through routed copper. Allow room next to it for the Nucleo and cable bends; a preliminary electronics enclosure envelope of about 160 by 110 by 45 mm is a planning target, not a fit guarantee. Initial paired signal/ground cables should be shorter than half a metre and secured against pulls. Label both harness ends, protect solder joints, and verify pin one using the square pad rather than trusting a wire color.

Power the Nucleo by its normal USB arrangement from a suitable regulated source. Reserve 150 mA at 5 V as a planning allowance and measure the actual assembly before finalizing a supply budget. Only the Nucleo 3.3 V rail may feed J1; never parallel it with another supply. The passive board does not need a duplicate processor regulator or reset network because those are on the purchased module. Budget roughly USD 80–100 for low-volume electronics and simple fixture materials, excluding labor, shipping and tools; this is an assumption, not a supplier quotation.

USB makes the rack contacts electrically common with the computer. There is no galvanic isolation, qualified ESD protection, industrial transient protection, ingress rating or safety certification. The series resistors are not isolation barriers. Connect only dedicated, disconnected voltage-free switches. Never connect PLC outputs, machine controls, powered sensors, mains wiring or unknown circuits. A low-current switch rating and assembled contact resistance must be checked; oxidized power switches are not automatically suitable at the stated excitation current.

## Firmware measurement contract

The portable engine has WAIT=0, RUNNING=1 and HALT=2. WAIT persists until all channels are qualified and both the raw and qualified occupancy vectors are empty. Starting with carriers already present cannot invent their original placement order. The operator must clear every pocket before acquisition history can begin. Contacts qualify independently: a new level is sample one, and sample five accepts it. A brief glitch shorter than this window can disappear from the qualified history. Qualification is a sampling convention, not proof that physical events were simultaneous or electrically clean.

In RUNNING, every group of newly occupied pockets advances the cohort ordinal once. Every inserted pocket receives that same ordinal. Unchanged occupied pockets retain their ordinal. Removing and later reusing a pocket assigns a new ordinal, even if all other pockets drained in between. The ordinal is an ordering stamp, not milliseconds or a specimen number. A cohort can contain between one and four insertions. The engine does not sort by pocket label, enforce FIFO or require a fixed sequence.

For each removal the lower rank is the count of strictly older carriers in the inventory immediately before the qualified change group. The upper rank includes all other carriers with an equal or older stamp. If lower and upper agree, the corresponding rank bin increments. Otherwise ambiguous increments. Both bounds are zero-based and cannot exceed three. The latest removal group retains its mask plus one low/high pair for each participating pocket; all nonparticipating entries are zero. That result stays frozen through later insertions and idle observations until another removal occurs.

Simultaneous removals are evaluated against the same previous inventory. Suppose A was inserted before B and both disappear in one observation: A records rank zero and B rank one. Removing A first in a loop and then ranking B against the reduced inventory would incorrectly make both oldest. Conversely two same-cohort carriers removed together both retain the interval zero through one. A same-observation insertion in a different pocket does not affect departure ranks; it is added after the departure group. These are explicit observation-boundary semantics, not a recovered ordering inside a millisecond.

The primary identities are insertions = removals + abandoned + occupied population, and removals = ambiguous + the sum of four unique-rank bins. Epoch is at most insertions, and insertions is at most four times epoch. A nonzero live stamp corresponds exactly to an occupied pocket. Completed removal evidence remains after a fault; active inventory is abandoned and cleared without falsely recording a retrieval. Abandonment is at most four because loss is terminal for this boot.

Any acquisition delta other than one millisecond after the first sample latches F_GAP. Unknown time is not filled with repeated samples and chronology is never resumed from a nonempty rack. Ordinary unsigned uptime wrap works with consecutive samples. Counter exhaustion latches F_COUNTER, bad input/configuration latches F_CONFIG, and failed watchdog startup latches F_WATCHDOG. The fault values are respectively 4, 8, 1 and 2. Only the first terminal reason is retained. Most counters and cohort stamps are bounded at one million; acquisition sequence is bounded at the unsigned 32-bit maximum. Transmission errors saturate independently.

Every ordinary sample is staged in a temporary engine. If any insertion or removal update would exhaust a bound, none of that sample's accounting commits. The old live inventory is abandoned exactly once. HALT freezes measurement fields, raw/qualified evidence and sequence while uptime and transmission diagnostics may continue. The app must not label those timestamps advancing acquisition. Recover by exporting, clearing the physical rack, inspecting the failure, and resetting the observer; no software command clears it.

Platform code restores the nominal HSI clock before serial use, checks input and UART configuration, provides startup vectors and RAM initialization, samples via SysTick, and runs a bounded nonblocking transmitter. The independent watchdog is initialized with bounded waits after SysTick starts. The code uses no heap or SDK. UART service returns immediately if not ready and abandons a pending frame after 100 ms. These implementation checks do not establish real oscillator accuracy, watchdog recovery, debounce quality or electromagnetic immunity.

## Protocol and companion software

AR1 is a fixed-policy, receive-only ASCII protocol. Every snapshot contains the marker, 32 unsigned decimal fields separated by commas, an asterisk, four uppercase CRC hexadecimal digits and LF. Decimal values have no leading zeros except zero itself. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial 0xFFFF over the marker and comma-separated payload before the asterisk, without reflection or final XOR. The marker appears on every record so a late attachment does not depend on catching a boot packet.

Field order is sequence, now, state, fault, raw, qualified, valid, epoch, occupied, insertions, removals, abandoned, ambiguous, last_mask; then rank0 through rank3; stamp0 through stamp3; low0 through low3; high0 through high3; tx_errors and gaps. Array suffixes indicate pockets A through D for stamps and intervals, but indicate rank bins for rank0 through rank3. now is modular millisecond uptime; none of the rank fields is a duration. A snapshot is attempted every 100 ms when the transmitter is available. The bounded 768-byte firmware buffer exceeds the largest legal frame.

The Linux/Tk application has Retrieval ranks, Trace, and Wiring and limits screens. The first shows current live cohort rank intervals, cumulative classifications and latest removal evidence. The trace displays every retained accepted snapshot, not just the last row in a serial batch. Connect opens a local 115200 serial device; Disconnect restores terminal attributes and preserves evidence. Replay replaces a disconnected session only after strict parsing succeeds. CSV export is exclusive-create with private file permissions and refuses to overwrite an existing file. No package installation, cloud account or network service is required.

The model rejects bad CRCs, malformed decimals, invalid field counts, impossible inventory conservation, out-of-range intervals, decreasing counters and incompatible transitions. Sparse snapshots receive necessary endpoint checks, including new stamp and removal budgets and frozen terminal evidence. They are not a complete proof of all hidden events. Snapshot transmission may omit multiple contact changes; absence of a row is not absence of activity. The app retains only the latest 512 snapshots, while device counters summarize the current volatile boot history.

Live freshness needs a connection, at least two advancing acquisition observations, no latched validation warning and progress within one host second. A fixed two-second cumulative host/device progress budget detects growing backlog and clock jumps. It can also force reconnection during a long run with ordinary HSI drift. Replayed data is always labeled replay and never fresh. CRC provides corruption detection, not authentication; a malicious local device can forge evidence. Some resets with forward-looking counters and timestamps cannot be distinguished without persistent boot identity. Do not use this stream for adversarial audit or safety decisions.

## Build, test and operate

Required build tools are GNU Arm Embedded GCC and binutils, host GCC, Python 3, Tk, Xvfb, KiCad 9 with its Python pcbnew module, and installed KiCad footprint libraries. On this host pcbnew and Tk are available through /usr/bin/python3. From the device directory run:

```sh
make -C firmware clean all test fixture
xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py'
/usr/bin/python3 tools/check_hardware.py
/usr/bin/python3 tools/verify.py
/usr/bin/python3 app/workbench.py
```

The optional CAD regeneration command is /usr/bin/python3 tools/generate_hardware.py. It overwrites generated electrical files, so review any manual CAD changes first. tools/import_platform.py is provenance tooling for an empty destination, not a normal build step; it deliberately refuses overwrites. Shared support comes from jayis1's stroke-weave at repository commit 2d98d37. The engine, AR1 fields, model, rack presentation and rank-specific tests are new. The common transport and UI lifecycle are adapted from that reviewed platform.

An optional make -C firmware flash target invokes OpenOCD using ST-LINK and STM32F4 configuration and verifies the programmed image. It has not been exercised against physical hardware here. Before programming, check board jumpers and remove the rack harness. Verify power and each input with a meter, then attach one switch at a time and confirm raw/qualified display bits. Begin slow manual trials with each state held for at least 50 ms. Compare handwritten placement and retrieval histories with exported ranks, including tied groups and reused pockets. Observe real bounce using suitable test equipment before increasing handling speed.

The automated engine test compares every four-vector occupancy word against an independently expressed cohort-array oracle, including ties, rolling reuse and simultaneous changes. Additional tests exercise held startup, qualification boundaries, unrelated chatter, sampling loss, timestamp wrap, exhaustion, invalid input, CRC and buffer bounds. Driver tests cover clock configuration and serial timeout paths. Python tests use real C-encoder fixtures, CRC-valid contradictory records, bounded replay, serial restoration, private export and an actual Tk event loop with a pseudo-terminal. Synthetic fixtures are not measurements from an assembled rack.

## Acceptance, remaining work and responsible use

Electrical acceptance requires native KiCad ERC and DRC plus an exact comparison of schematic XML pin nets, PCB pad nets, component values, footprint identities and the manifest. The passive carrier cannot validate the external Nucleo power tree or a hand-wired harness. Zero rule violations is useful evidence, not an assertion of fabrication readiness. Inspect the rendered schematic and the assembled board polarity separately, and preserve the explicit board boundary when reviewing the design.

Remaining work includes fabrication inspection, continuity testing, actual MCU flashing, supply and logic-level measurements, timing and watchdog tests, switch endurance and bounce characterization, mechanical repeatability, enclosure fit, cable interference, and a supervised usability trial. No regulatory, environmental or material-performance qualification is supplied. Stop if a switch becomes intermittent or the rack fixture affects the sample. Retain the uncertainty in exports rather than replacing it with a favorable interpretation. The useful final product is a reproducible statement about observed selection chronology, not a declaration that a process or specimen is good.
