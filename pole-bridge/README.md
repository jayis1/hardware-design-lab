# Pole Bridge — paired-pole transfer-topology notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software, tests, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint-library geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier for a separately purchased NUCLEO-F401RE. Software builds, synthetic contact histories and electrical CAD checks are reproducible. No physical carrier has been fabricated, MCU programmed, switch measured, or electrical timing calibrated. This is not a mains tester, contact-rating instrument, relay driver, safety interlock or production inspection certificate.

## Purpose and practical problem

Pole Bridge helps a repair-cafe technician or teaching laboratory examine a disconnected, manually operated double-pole double-throw switch. Two switches with the same static truth table can have very different intermediate behavior. One pole may move before the other; one may visit an all-open state while the other temporarily connects both throws; the operator may start a movement and return to the original endpoint. A continuity buzzer generally loses that relationship. Four separate traces can capture it, but interpreting each transfer by hand is repetitive and error-prone.

The device makes a deliberately narrow notebook of those transfers. Each pole has four observed topologies: neither throw connected, A only, B only, or both throws connected. For each completed movement it retains the acquired duration in each topology, the duration that could not be assigned confidently because contacts were changing, and the time when the two poles selected opposite sources. It distinguishes a settled transfer to the opposite endpoint from a settled return to the starting endpoint. An unfinished movement receives a timeout rather than a misleading successful result.

The practical value is in explaining slow mechanical transfer behavior and comparing repeated manual operations under the same fixture conditions. A technician can document that a repaired linkage still spends substantial observed time in opposite-source configurations even though both endpoint continuity checks pass. A teacher can show why same-pole overlap and cross-pole disagreement are different facts. A mechanism designer can compare two hand-operated contact arrangements without building a separate data decoder for each trial.

The prototype intentionally does not measure contact resistance, current capacity, insulation, dielectric strength, arcing, or bounce shorter than its qualification window. A zero overlap count does not establish break-before-make behavior at a finer timescale. Fast snap switches may complete their entire internal transfer before qualification; that is a limitation, not evidence that their contacts transferred simultaneously. Initial experiments should use slow teaching mechanisms with visible contacts and adjustable travel.

## Originality and comparison with existing work

The repository's device directories and README titles were inventoried before creating this folder. The closest existing project is ContactAtlas, whose complete measurement contract was reviewed: it discovers arbitrary stable four-bit signatures and records a directed graph between them, with whole-vector zero and multiple-contact durations. Those durations deliberately cannot be interpreted as per-pole overlap. Pole Bridge instead fixes two separately interpreted changeover poles, permits neither/both states within each pole, partitions every acquired interval independently for each pole, and distinguishes cross-pole opposite-source occupancy from same-pole bridging. It also retains return and timeout outcomes rather than treating every stable signature as another graph node.

Pedal Window was reviewed as another nearby four-contact timing design. It records first-event offsets between two piano keys and pedal endpoints; it does not integrate changing contact topologies or compare two changeover poles. The common electrical platform, startup code, serial driver, transport lifecycle and CAD tooling are intentionally reused from jayis1's Age Rack design at commit 54e1577. The new contribution is the measurement engine, two-pole contract, validation rules and companion presentation, not a claim that resistor networks are novel.

A limited external search found Tektronix's Switching Handbook, which explains SPDT and DPDT contact configurations, and Analog Devices' break-before-make timing considerations. Conventional switching and timing analysis are established prior art. The original proposal here is an inspectable, inexpensive, gap-aware two-pole teaching notebook. No worldwide uniqueness, patent novelty or absence of similar commercial instruments is asserted. A general logic analyzer remains the better choice when microsecond-scale transitions, analog levels or complete waveform retention matter.

## Hardware specifications

| Item | Revision A specification |
| --- | --- |
| Controller assembly | NUCLEO-F401RE, MB1136 family |
| MCU | STM32F401RET6, Cortex-M4, LQFP64, on purchased board |
| Clock | Internal HSI, nominal 16 MHz; no PLL required |
| Memory | Target linker maps 512 KiB flash and 96 KiB SRAM |
| Sensing | Four voltage-free contact paths, active low at GPIO |
| Carrier components | Four 10 kohm pull-ups, four 1 kohm series resistors, six headers |
| Carrier form | 80 by 70 mm, two copper layers, 1.6 mm FR-4 |
| Routing | 0.3 mm tracks; through-hole axial resistors and headers |
| Link | USART2 TX via ST-LINK virtual COM, 115200 baud, 8N1 |
| Power | USB into Nucleo; its regulated 3.3 V supplies the carrier |
| Acquisition | Nominal one-millisecond samples; any skipped tick halts correlation |
| Qualification | Five consecutive samples independently per contact |
| Endpoint settlement | Twenty clean acquired intervals |
| Trial deadline | 5000 acquired intervals; timeout wins at equality |
| Storage | Volatile counters plus working and last classified capture |
| Host | Local Python 3/Tk desktop on Linux; no pip dependencies |

The supplied schematic and PCB are the complete passive interface, not a standalone processor motherboard. The regulator, decoupling capacitors, reset network, debugger, USB connector and processor belong to the Nucleo. No hidden radio, battery, charger or relay coil driver is claimed. Keeping those functions on a documented purchased assembly makes the prototype bounded and buildable without pretending that a bare MCU needs no supporting circuit.

## Architecture and block diagram

    Detached pole 1 common ------------------------------ GND
    Detached pole 2 common ------------------------------ GND
    Pole 1 A -> J3 -> 1k -> SENSE0 -> J2.1 -> PA0
    Pole 1 B -> J4 -> 1k -> SENSE1 -> J2.2 -> PA1
    Pole 2 A -> J5 -> 1k -> SENSE2 -> J2.3 -> PA4
    Pole 2 B -> J6 -> 1k -> SENSE3 -> J2.4 -> PB0
                              |
                        each through 10k
                              |
    USB -> NUCLEO-F401RE -> 3V3 -> J1.1
               |
               +-> SysTick -> independent qualification -> transfer engine
               |                          |
               |             per-pole bins / unknown / mixed / outcome
               |                          |
               +-> USART2 TX -> ST-LINK VCP -> Linux Tk companion
                                                    |
                                        topology / trace / CSV export

The two switch commons are deliberately joined to the USB host's ground. This is acceptable only for a completely detached switch. It is not a differential probe and cannot preserve electrical isolation between installed poles. Remove all original wiring, external loads and stored-energy connections before attaching the fixture. Do not energize a relay coil through this carrier or assume that an unpowered machine is disconnected from every hazardous source.

## Pin map and assembly contract

J1 pin 1 is VCC and connects to Nucleo CN6 pin 4, 3V3. J1 pin 2 connects to CN6 pin 6, ground. Never connect J1 to the Nucleo 5 V pin. J2 pins 1 through 4 connect to CN8 pins 1 through 4, respectively A0/PA0, A1/PA1, A2/PA4 and A3/PB0. J3 through J6 pin 1 carries P1_A, P1_B, P2_A and P2_B, in that order; pin 2 is ground on every contact connector. Connect the two pole commons to ground and each throw to its named signal.

A and B are operator-applied labels, not automatically identified normally-open and normally-closed positions. Align labels so that one intended extreme closes P1_A and P2_A, producing mask 5, and the other closes P1_B and P2_B, producing mask 10. Bits zero through three correspond to J3 through J6. If the switch is cross-wired, the firmware will not invent a valid common endpoint; correct the labeling before collecting evidence. A center-off detent is observed as part of a transfer or timeout, not a third accepted endpoint.

USART2 transmit uses PA2 alternate function AF7. The Nucleo's documented virtual-COM bridge arrangement is required: SB13/SB14 connected and SB62/SB63 open on the relevant MB1136 configuration. No application receiver is enabled. Before assembly, confirm the actual board revision, connector orientation and solder bridges against ST UM1724. Register addresses and clock behavior derive from RM0368. Direct attempts to download the current ST manual during this run failed; this revision reuses the repository's existing platform mapping and does not claim a fresh manufacturer-document audit.

The BOM is five two-pin 2.54 mm vertical headers, one four-pin header, four 10 kohm quarter-watt axial resistors and four 1 kohm quarter-watt axial resistors. The resistor footprint uses 7.62 mm lead spacing. An exact reference/value/footprint/pin manifest is in kicad/connectivity.json. KiCad contains locally authored resistor and header symbols, connected wires and labels, actual library footprints, assigned pad nets, routed copper and a closed board outline. Mechanical mounting uses insulated edge supports; no mounting holes are claimed.

## Electrical limits, power and enclosure

The intended closed path is 3.3 V through 10 kohm, then the GPIO sense node, then 1 kohm and a closed contact to ground. Nominal current is 0.300 mA and the sense voltage is 0.300 V. Four closed contacts draw approximately 1.200 mA, or 3.960 mW from the carrier supply. These calculated values exclude resistor tolerances, leakage and cable effects. Verify actual voltages and logic thresholds on the assembled board. Low wetting current may not penetrate oxide on power contacts; use clean low-level signal contacts for initial tests.

Reserve 100 mA at 5 V as a conservative planning allowance for the development board and debugger; this is not a measured consumption specification. There is no battery runtime promise. USB power loss erases working and retained device evidence, so export useful host observations before disconnecting. A regulated USB supply suitable for the purchased board is required. No independent external supply should be connected to the carrier.

The enclosure should support the Nucleo and carrier separately, protect exposed solder joints, retain the USB cable, and label every test lead. Keep signal leads short, initially below half a meter, and pair signals with returns. That length is a prototyping constraint rather than a tested EMC rating. Avoid loose clips near moving contacts. Do not alter a valuable switch or machine to fit the test fixture before checking whether the proposed observation is actually useful.

The resistors do not provide galvanic isolation, surge suppression, reverse-power protection or a certified measurement category. Never attach mains, industrial 24 V, powered sensor outputs or installed electrical contacts. The entire device is a supervised dry indoor bench prototype. It does not determine whether a switch is safe to return to service.

## Firmware behavior and measurement semantics

The firmware is freestanding C11 with a real Cortex-M4 target link, vector table, data/BSS initialization, linker script and nonblocking UART driver. There is no RTOS, dynamic allocator, HAL download, cloud service or persistent settings store. Clock startup restores HSI with bounded waits before configuring the interfaces. GPIO configuration is checked, and watchdog initialization failure becomes an explicit terminal fault. The watchdog is serviced only after sampling progress.

WAIT requires a fully qualified common endpoint, either mask 5 or mask 10, and twenty adjacent intervals whose raw and qualified values both agree. The first qualifying endpoint starts settlement credit at zero. READY remembers that endpoint as the anchor. Raw glitches that never change the qualified vector do not start a trial. The first qualified departure starts ACTIVE with age zero. This means the instrument does not count the predeparture raw debounce period as measured transfer time.

During ACTIVE, every new sample adds one interval to age. For each pole independently, the preceding and current raw two-bit codes must agree, and both corresponding qualified codes must agree with that code. If so, the interval goes into that pole's open, A-only, B-only or overlap bin. Otherwise it goes into that pole's unknown bin. Each pole's four bins plus unknown therefore equal the same capture age. Do not add durations from both poles as if they were disjoint wall time.

Mixed duration is incremented only when both adjacent raw vectors and both qualified vectors are identical mask 6 or 9. These are opposite-source selections across different poles. Mask 15 is same-pole overlap on both poles, not mixed-source occupancy under this definition. Mixed is a subset of each pole's combined A-only/B-only duration; it is not an additional duration that should be added to the partition.

A stable opposite endpoint held for twenty clean intervals completes TRANSFER. A stable original endpoint completes RETURN, preserving a real excursion rather than calling it a successful switch. Any raw endpoint disturbance resets settlement, even when too short to qualify. At age 5000 the capture becomes TIMEOUT before settlement is considered. Thus completion exactly on the deadline is rejected. After every classified result the engine returns to WAIT and requires a new settlement baseline; held contacts do not manufacture more transfers.

Working coverage is separate from the last classified result. A new start does not erase the previous result. An acquisition gap, invalid input mask, startup fault or counter exhaustion causes terminal HALT. An active trial is counted once as aborted; its partial working capture is cleared without replacing the last result. The invariant is starts equals transfers plus returns plus timeouts plus aborted plus the active indicator. Counters never silently wrap, and staged sample updates prevent partial changes at exhaustion. A host must export, inspect and explicitly restart after HALT; quiet inputs cannot prove what happened during a gap.

## Protocol and companion application

Every ASCII line is PB1 followed by 45 comma-separated unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses initial value 0xffff and polynomial 0x1021 over the payload before the asterisk, without reflection or final XOR. The standard check string 123456789 yields 0x29b1. PB1 identifies the fixed five-sample, twenty-interval, 5000-interval policy on every record. Changing that policy requires a new compatible protocol contract.

The first fields are sequence, now, state, fault, raw, qualified, valid, starts, completed, returned, timeouts, aborted, tx_errors, gaps, anchor, target and settle. They are followed by a working capture and a retained capture. Each capture contains origin, outcome, age, bin0 through bin7, unknown0, unknown1 and mixed. Bins zero through three belong to pole one, and four through seven to pole two. Within each group the code order is open, A, B, overlap. State codes are WAIT=0, READY=1, ACTIVE=2 and HALT=3; outcomes are none=0, transfer=1, return=2 and timeout=3. Fault bits used as exclusive reasons are configuration=1, watchdog=2, gap=4 and exhaustion=8.

Sequence advances per acquired sample, not per transmitted record. Normal target snapshots are emitted at roughly 100 ms intervals, so sequence differences usually represent unreported acquisition samples, not lost packets. Only the latest classified capture is transmitted; multiple transfers between received records cannot be reconstructed. This is not an archival event logger. UART buffering is bounded, transmission is nonblocking and a transmit deadline reports loss without trapping acquisition.

Run `python3 pole-bridge/app/workbench.py` from the repository root. The three real screens show current and last per-pole topology, the retained snapshot trace, and wiring/interpretation guidance. Connect opens a selected Linux serial path; replay loads a bounded local PB1 recording; export creates a private CSV without overwriting an existing file. Reconnect clears prior session displays only after the new port opens successfully. Disconnect retains evidence and original provenance.

The companion validates CRC, framing, field ranges, capture conservation, endpoint settlement coverage, monotonic counters and necessary sparse transition bounds. Contradictory duplicates or reset-like regressions latch a warning instead of silently relabeling old evidence. The parser can recover at a complete PB1 marker after a truncated live frame. Replay is stricter: it rejects malformed input, partial tails, empty recordings, special files and files larger than two MiB. The application retains the latest 512 accepted snapshots and makes no claim to recover unreported history.

Live freshness requires advancing acquisition, an open connection and recent host receipt. Duplicate records do not refresh it; HALT is never live acquisition. A fixed two-second cumulative host/device progress budget detects backlog or excessive clock disagreement, rather than granting an ever-growing lag allowance. Reset detection remains heuristic because the device has no persistent boot identifier. CRC detects accidental corruption, not forgery. There is no incoming command channel, authentication secret, network endpoint or remote update path.

## Build, verification and physical acceptance

Required tools are GNU make, host GCC with sanitizers, Arm bare-metal GCC, Python 3 with Tk, Xvfb for headless GUI tests, and KiCad 9 with distribution Python bindings and standard footprint libraries. No package-manager download is needed for the companion. From the repository root, run:

    python3 pole-bridge/tools/verify.py
    python3 pole-bridge/app/workbench.py

The verification script target-links firmware, runs sanitized host tests, produces clearly synthetic frames through the real C encoder, exercises parser and real Tk/PTY workflows, and runs schematic ERC, PCB DRC and exact schematic/PCB/manifest connectivity comparison. README size and substantive production C line counts are checked separately; those counts are not substitutes for functional tests. tools/import_platform.py records source provenance and refuses overwrite; it is not required for normal builds. Regenerate CAD only deliberately with the included generator, then repeat all electrical checks.

Programming is a trusted local ST-LINK/OpenOCD action through `make -C pole-bridge/firmware flash`. No physical target was connected during this design run, so flashing is not among the verified outcomes. Before programming, inspect the carrier unpowered, check resistance and continuity, verify the actual board revision and pin map, then measure open and closed SENSE voltages. Exercise all sixteen contact masks with safe isolated jumpers. Confirm mask 5 and mask 10 baselines and introduce deliberate wrong wiring to ensure the display remains interpretable.

A first physical experiment should use a slow transparent DPDT teaching mechanism. Record independent logic-analyzer traces alongside UART output, test both directions, pause in center-off, introduce overlap, return to origin, and disturb a settling endpoint. Check short pulses, timing uncertainty, cable movement, USB disconnect and MCU reset. Retain raw reference traces and mounting notes. Only after these tests should anyone evaluate practical repair usefulness. Fabrication readiness, electrical bring-up, calibration, ESD/EMC performance and physical switch behavior remain unverified; the delivered package is a reproducible engineering design, not a certified instrument.

## References

- ST UM1724, Nucleo-64 MB1136 manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Tektronix Switching Handbook: https://www.tek.com/en/documents/product-article/switching-handbook
- Analog Devices timing considerations: https://ez.analog.com/switches_multiplexers/w/documents/15262/switches-and-multiplexers-break-before-make-timing-considerations

All original deliverables credit jayis1 through valid source comments, project metadata, title blocks or document attribution. Generated build products, caches and local editor preferences are ignored. Validation outcomes and remaining limitations are recorded in VALIDATION.md.
