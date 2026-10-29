# Rest Quartet — four-pocket residence notebook for adhesive test coupons

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is an engineering prototype. Its custom PCB is a routed passive contact carrier for a separately purchased NUCLEO-F401RE. The MCU, regulator, decoupling, debugger and USB bridge are on that development board. Target builds, software tests and CAD checks are reproducible; fabrication, programming, physical switch measurements, adhesive experiments and usability have not been performed. Residence is not cure quality.

## Purpose, user and practical outcome

Rest Quartet helps a bookbinding or paper-repair workshop compare how small adhesive test coupons are handled while resting. Four labeled pockets independently record insertion, uninterrupted qualified occupancy, removal before a fixed observation threshold, removal after that threshold, and interruptions that invalidate timing. Its primary user is an instructor preparing several practice bonds while students repeatedly pick up samples to inspect them. The measurable outcome is a per-pocket placement ledger rather than a memory-based claim that every coupon rested for the same interval.

A conventional kitchen timer measures time since somebody pressed a button. It does not know that one sample was lifted, that another was returned to the wrong place, or that a sample already occupied its holder when power was applied. Rest Quartet makes a narrower but more defensible observation: this pocket was known empty, then its witness contact closed, then remained qualified closed until removal. It refuses to invent a placement time for a pocket first seen occupied. Simultaneous placements in other pockets do not interfere with its accounting.

The initial experiment is deliberately modest. Prepare four dry paper practice coupons or use already bonded, chemically safe samples. Mark them A through D, assign one pocket to each, and compare a supervised hands-off run with a run allowing repeated inspection. Observe the number of early removals and the shortest, longest and most recent completed residence. The fixed sixty-second threshold is an experimental handling interval, not an adhesive manufacturer's setting time. This instrument cannot tell whether glue was mixed, whether a bond was pressed, or whether anything cured.

Hardware is useful because the pocket switches observe handling without requiring a user to tap a phone for every movement. The computer supplies a readable notebook and export, while the microcontroller continues accumulating totals even when the app is absent. The electronics do not lock a sample in place, sound a safety alarm, heat adhesive or authorize removal. The proposed benefit is better handling evidence for small comparative experiments. It remains a product hypothesis until a workshop tests whether it changes decisions.

## Originality and bounded comparison

The existing repository device directories and titles were inventoried before choosing this concept, followed by purpose review of related fixtures and searches for adhesive and residence workflows. Nest Settle observes four corners of one locating nest and studies seating/dropout dynamics. Rest Quartet observes four independent pockets and conserves placement episodes separately. Rinse Ledger allocates basket immersion across declared water batches, while this design classifies completed uninterrupted pocket residences and refuses to resume an occupied pocket after a gap. Peel Front studies spatial propagation during adhesive-strip lifting, not pre-test handling history.

Stopwatches, occupancy detectors and curing racks already exist. A bounded external search found commercial gel timers, which detect increasing viscous drag, and UV curing monitors that investigate resin cure. Those are different measurement goals. This proposal does not invent timing, adhesion testing or sample racks, and no worldwide novelty or patent clearance is claimed. The original contribution is the pocket-specific experiment, conservative reset/gap semantics, placement conservation, firmware implementation and local evidence notebook. Reusing a known electrical carrier rather than inventing another unverified microcontroller board is intentional.

Why might this combination be useful despite simple alternatives? A cheap multi-channel timer still relies on correct manual starts and stops. A camera provides stronger contextual evidence but introduces recording, frame review and potential privacy issues. This tray offers automatic binary handling records with no camera and no cloud. Conversely, when specimen identity, actual cure, temperature or force matters more than residence, this design is not the appropriate instrument. Use independent labels and appropriate material tests instead of extending its claims.

## Hardware specifications and architecture

| Function | Revision A design |
| --- | --- |
| Processor | STM32F401RET6 on NUCLEO-F401RE MB1136 |
| CPU and memory | Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI restored to nominal 16 MHz |
| Sensors | Four external normally-open voltage-free pocket switches |
| Acquisition | Nominal 1 ms polling; independent 8 ms qualification |
| Input bias | Four 10 kohm pullups to Nucleo 3.3 V |
| Series resistance | Four 1 kohm resistors between sense nodes and switches |
| Connectivity | USART2 TX, 115200 baud, 8N1, ST-LINK virtual COM |
| Power | USB to Nucleo; carrier takes its 3.3 V and ground |
| Carrier PCB | 80 by 70 mm, two copper layers, 1.6 mm nominal thickness |
| Assembly | Through-hole axial resistors and 2.54 mm headers |
| Application | Python 3 with Tk, Linux termios, no pip packages |

Signal architecture:

    Pocket A contact -- J3 -- POCKET_A -- 1k -- SENSE0 -- PA0 / A0
    Pocket B contact -- J4 -- POCKET_B -- 1k -- SENSE1 -- PA1 / A1
    Pocket C contact -- J5 -- POCKET_C -- 1k -- SENSE2 -- PA4 / A2
    Pocket D contact -- J6 -- POCKET_D -- 1k -- SENSE3 -- PB0 / A3
                                               |
                                          10k to 3V3

    USB -> NUCLEO-F401RE -> independent qualification -> four pocket ledgers
                          -> bounded CRC-framed UART -> ST-LINK VCP
                          -> Linux pocket screen / snapshot notebook / CSV

J1 pin 1 connects to Nucleo CN6 pin 4, 3.3 V. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: PA0, PA1, PA4 and PB0. These analog-header pins are configured as digital inputs. J3 through J6 use pin 1 for the corresponding pocket signal and pin 2 for ground. A closed switch pulls its signal low; firmware inverts this to occupied. Never exchange the power header with a switch header.

USART2 transmit uses PA2 in AF7. The standard MB1136 virtual-COM arrangement requires SB13 and SB14 fitted and SB62 and SB63 open. The receive peripheral is disabled; the application cannot change firmware policy or send actuation commands. The connector map and platform follow the repository's existing jayis1-authored Nucleo designs and ST UM1724. A fresh manufacturer PDF download failed during this run; verify your actual board revision against that manual before wiring. The inherited register definitions reference STM32F401 RM0368 rather than a guessed device family.

## Mechanical tray and sensor installation

Build a nonconductive four-pocket rack for dry, individually labeled coupon carriers. An initial mechanical layout uses four removable 45 by 65 mm inserts arranged in two rows, with enough finger clearance that lifting one insert does not touch its neighbors. These dimensions are a starting fixture specification, not a universal coupon standard. Provide a rigid rear stop and two side guides in each pocket so insertion position is reproducible. A lightweight carrier tab operates a low-force switch near the rear stop. The coupon itself should not press on the switch or carry its spring force.

Each insert should have a raised dry gripping end and a replaceable nonstick support surface appropriate to the adhesive procedure. Keep uncured adhesive away from the switch, guides, electronics and USB cable. The rack must not squeeze the bond or act as an undocumented clamp. If the experiment requires clamping, use a separate specified clamp and recognize that pocket presence says nothing about clamp force. Use identical carriers and switch geometry across all four positions so the contact threshold is not a hidden variable.

Mount switches on slotted insulating brackets and adjust them with an empty reference carrier. Arrange overtravel against a hard mechanical stop, not against the switch housing. Choose signal-rated contacts with low-current suitability and documented operating force. Label A through D on both rack and cable ends. Use strain relief and short paired signal/ground wiring; keep initial lead lengths below half a meter. Do not connect the switches to any other powered apparatus.

The PCB has no mounting holes. Retain it using insulating edge rails in an electronics compartment, with the Nucleo adjacent and its USB and reset accessible. Allow at least a roughly 110 by 100 mm area for the carrier enclosure alone and determine the final combined housing from the actual Nucleo dimensions and cable clearance. No printable enclosure or universally fitted rack model is claimed. The mechanical design is a dimensioned starting arrangement whose actuation repeatability and material compatibility require local fabrication trials.

## Power, BOM and restrictions

The component manifest in kicad/connectivity.json records fourteen populated carrier components. It contains four 10 kohm axial resistors, four 1 kohm axial resistors, five two-pin headers and one four-pin header. The resistor footprint is a DIN0207 axial body with 7.62 mm lead pitch. Select one-percent, quarter-watt resistors and appropriate mating cable housings. Add the external Nucleo, four low-force signal switches, USB cable, insulated rack, replaceable inserts and strain-relieved wiring to the purchase list.

At nominal 3.3 V, a closed channel draws 3.3 V through 11 kohm, giving 0.300 mA and an MCU-side voltage of approximately 0.300 V. All four closed channels draw 1.200 mA. The resistor network then dissipates approximately 3.96 mW. These are ideal calculations, not measured consumption. Verify tolerances, switch resistance and MCU thresholds during electrical bring-up. The development board, debugger and LEDs consume additional power not measured here.

Use a regulated USB source with at least a conventional 5 V, 500 mA capacity and follow the Nucleo's documented power-jumper arrangement. There is no battery, charging circuit or external carrier regulator. Do not connect a second supply to the carrier's 3.3 V rail. As a planning target rather than a supplier quote, budget under USD 100 for electronics and simple hand-built fixtures, excluding labor and shipping. The likely buyer is a teaching workshop reusing the platform across experiments; elaborate precision mechanics could defeat this cost goal.

The carrier is not isolated from the computer. There is no certified overvoltage, ESD or industrial transient protection. A series resistor is not a safety barrier. Never connect mains, machine controls, PLC outputs, powered sensors or unknown field wiring. There is no ingress rating. Keep the assembly dry and away from flammable vapors, hot processes and conductive debris. Follow adhesive safety documentation independently; this is not an occupational-safety, food, medical or structural-bond certification device.

## Firmware behavior and design decisions

The firmware is freestanding C11 with explicit startup vectors, data/BSS initialization, linker memory layout, clock restoration, GPIO configuration checks, SysTick acquisition, watchdog setup and bounded UART transmission. It links for Cortex-M4 with the real ARM embedded toolchain and no vendor SDK or heap. The core acquisition policy is separate from hardware access so it can be exercised with synthetic inputs on a host. The board support and passive carrier topology are reused from pipette-path, preserving jayis1 attribution; the pocket engine, protocol and app are device-specific.

Each pocket has WAIT, READY and OCCUPIED states. WAIT requires a qualified open contact before becoming READY. Closing a READY pocket starts one new placement with age zero. Every subsequent consecutive one-millisecond sample credits the preceding qualified occupied interval. A qualified opening completes the placement, records the duration and returns to READY. A duration below 60,000 ms increments early; a duration at or above 60,000 ms increments rested. The equality boundary is explicit: removal at exactly the threshold is classified rested. That word describes an observed interval only.

The duration limit is 3,600,000 ms. At that boundary timeout takes precedence over a simultaneous removal. The placement is aborted, timeouts increases, and the pocket must be observed empty before another start. If the contact is already qualified open on the timeout sample, READY can be established immediately; it need not manufacture a new edge. A held occupied switch cannot create repeated timed-out placements. Completed-only totals exclude aborted and timed-out durations, while starts and aborted preserve their existence.

Each contact qualifies independently. A changed raw level becomes a candidate with age zero; eight further consecutive samples are required before accepting it. Thus another pocket's chatter cannot postpone a stable transition here. Short glitches can be invisible to the qualified ledger. Its durations include debounce effects and are not guaranteed lower bounds on physical residence. Pocket identity is not specimen identity: removing one coupon and inserting a different coupon starts a new episode, but replacement without opening the switch is undetectable.

Any acquisition interval other than one millisecond invalidates continuity. A gap increments the gap counter, aborts every active pocket, clears qualification, and requires each pocket's own empty baseline. Previously completed totals remain. Unknown time is never credited. A closed pocket following a gap remains WAIT indefinitely until emptied. An empty pocket can recover even if a different pocket remains occupied. Reboot clears all volatile state and likewise cannot infer earlier placement times.

For every pocket the invariant is starts = early + rested + aborted + active, where active is one only in OCCUPIED. All updates for an acquisition sample are staged transactionally. If any counter or completed-duration total would overflow, the staged update is discarded and the previous active placements are abandoned exactly once before terminal fault. Counters do not silently wrap. Fault bits identify overflow, invalid input/configuration and watchdog initialization failure. Ordinary millisecond uptime rollover uses modular arithmetic; a completely unobserved full timer cycle is inherently ambiguous, with the watchdog intended to prevent such a stall.

## Protocol and companion application

A snapshot is emitted every 100 ms when the nonblocking transmitter is available. The frame begins RQ1, contains 51 unsigned decimal fields separated by commas, and ends with an asterisk, four hexadecimal CRC characters and LF. CRC-16/CCITT-FALSE covers the entire prefix before the asterisk, with polynomial 0x1021 and initial 0xffff. There is no reflection or final XOR. The encoder and driver use a 768-byte buffer; an incomplete transmission is abandoned after 100 ms and increments tx.

The first seven fields are now, gaps, tx, fault, qualify, goal and limit. The last three repeat 8, 60000 and 3600000 on every snapshot so late attachment can validate policy without a one-shot boot record. These are followed by eleven fields for A, then B, C and D: state, starts, early, rested, aborted, age, last, minimum, maximum, total and timeouts. State numbers are WAIT=0, READY=1 and OCCUPIED=2. Durations are milliseconds. Last, minimum, maximum and total describe completed placements only and remain zero before the first completion.

The Pocket residence screen displays all four independent ledgers. Snapshot notebook shows up to 500 accepted snapshots and aggregate placement counts. Setup and limitations explains the connection map and interpretation. Connect opens a Linux serial path and configures 115200 8N1; a second Connect cannot replace an already owned port. Disconnect preserves evidence and provenance. Replay loads a regular file no larger than one MiB and refuses FIFOs, device files, empty data and malformed or incomplete recordings. It is always labeled offline.

The parser bounds pending frames and resynchronizes on the complete RQ1 marker after a truncated predecessor. It validates CRC, policy, field count, integer bounds, placement conservation, state constraints and necessary relationships among classification counts and duration extrema/totals. Session checks reject cumulative rollback, contradictory same-time snapshots and changes to completed evidence without a completion. These are necessary consistency checks, not a proof that every conceivable valid-CRC stream arose from a real fixture.

LIVE means the transport has shown advancing device time, not that the material is ready. Duplicate timestamps do not refresh freshness. A live reception pause exceeding one second freezes acceptance until reconnect, preventing a backlog from relabeling old evidence as fresh. A fixed half-second bound on cumulative host/device progress difference also detects slower developing backlog and forward clock jumps. This intentionally conservative bound can require reconnect during long runs because the uncalibrated MCU oscillator differs from the host clock. Export before reconnecting; reconnect clears the desktop notebook, not MCU totals.

CSV export exclusively creates a new file with private permissions and refuses overwrite. Its metadata row includes author, provenance, connection state, freshness, age, parser errors, blocked reason and notebook evictions even when no snapshots exist. Reset inference remains heuristic because there is no persistent boot identity; a lost reset followed by forward-looking counters can be indistinguishable. CRC is error detection, not authentication. The local serial stream is untrusted, no cloud service is used, and the application does not control machinery.

## Build, verification and physical acceptance

From the repository root:

    make -C rest-quartet/firmware all test
    xvfb-run -a python3 rest-quartet/tests/test_app.py
    python3 -m py_compile rest-quartet/app/protocol.py rest-quartet/app/quartet.py
    /usr/bin/python3 rest-quartet/tools/check_hardware.py
    python3 rest-quartet/tools/check_quality.py
    python3 rest-quartet/app/quartet.py

Required tools are GNU Make, ARM embedded GCC/binutils, native GCC, Python with Tk, KiCad 9 and its standard footprint libraries. Xvfb is needed only for headless GUI tests. CAD scripts use the distribution Python providing pcbnew. No pip or npm dependency installation is required. The Linux termios transport is not advertised as portable to Windows. Use appropriate serial-device permissions rather than running the application as root merely to access a port.

The hardware checker runs ERC and DRC with violation exit codes, exports a fresh schematic netlist and compares all reference/pin/net assignments, values and footprint identities against actual PCB pads and the manifest. Generated local symbols have actual numbered passive pins and footprint assignments. The PCB has routed copper, a ground via and a closed outline, not just component placeholders. Passive-only ERC cannot verify the external development board or physical switch wiring. Standard KiCad footprints retain their upstream library licensing; jayis1 is credited for original sources, not as the creator of upstream geometry.

Firmware tests exercise qualification independence, exact threshold boundaries, gaps, held inputs, timeout recovery, counter overflow, fault conservation, uptime wrap, encoder bounds and CRC. Reused platform tests exercise clock/configuration checks and UART queue/timeout behavior with simulated registers. Application tests consume fixtures from the actual C encoder and exercise Tk through Linux pseudoterminals. These tests do not demonstrate physical USB delivery or contact mechanics. See VERIFICATION.md for executed results and tool versions.

Build products are firmware/build/rest-quartet.elf and its binary. With the intended board positively identified, make -C rest-quartet/firmware flash invokes OpenOCD and ST-LINK programming, verify and reset. That operation has not been performed here. Before flashing, inspect the unpowered carrier, check resistor values, meter shorts and confirm the connector orientation. Power the Nucleo alone, then connect the carrier and verify open/closed sense voltages. Exercise one jumper at a time and compare timestamps against a logic analyzer.

Finally assemble a dry fixture, repeat insertions across every pocket, deliberately lift and reinsert, start with occupied pockets, introduce a broken wire and interrupt USB. Record switch part, geometry, sample identity and environmental conditions independently. Compare the ledger against video or a reference instrument before using it for an experiment. Fabrication, timing calibration, mechanical repeatability, EMC, ESD, material compatibility and human usefulness remain unverified. Passing the software and CAD gates is a reason to proceed to supervised bench evaluation, not a reason to claim a finished measurement product.

## References

- Nucleo MB1136 manual, UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 register reference, RM0368: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Existing gel-time instruments: https://www.worldoftest.com/gel-timer/
- Existing curing-process monitor: https://acroedge.org/products/uv-curing/
