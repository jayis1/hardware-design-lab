# Merge Rhythm — source-streak notebook for slow two-chute interleavers

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. The supplied PCB is a routed passive contact carrier for an external NUCLEO-F401RE, not a standalone MCU board. Target compilation, host software tests and CAD connectivity checks are reproducible. No board has been fabricated, target programmed, gate fixture assembled, or mechanical experiment performed during this design run. Timing accuracy, contact disturbance, electrical protection and actual usefulness require bench validation.

## Purpose, scenario and overview

Mara builds hand-operated marble and wooden-token exhibits for a small science museum. A two-chute mechanism appears to alternate its inputs, yet occasionally empties one side in a long streak while the other side waits. Merge Rhythm records the observed source sequence and downstream arrival timing during a supervised slow trial, while making ambiguous inputs, missing arrivals and operator restarts visible instead of silently turning them into plausible results.

The instrument uses three voltage-free passage contacts and an explicit CLEAR pushbutton. LEFT and RIGHT contacts sit at the two final admission throats, immediately adjacent to the confluence; EXIT sits downstream in the constrained single-file channel. A qualified source closure appends that source to a small FIFO. A qualified exit closure retires the oldest admission, contributes its transit time, and updates source-run statistics. Simultaneous source admissions, an unpaired exit, too many pending objects or an overdue arrival invalidate correlation. The operator must physically empty the channel and deliberately establish a new baseline before collecting another segment.

The specific value is a source-streak and transit notebook for testing a passive interleaver, not a controller that creates alternation. It answers questions such as whether changing a gate spring reduces long same-source runs, whether a gentler slope increases downstream residence, and how often a trial becomes unresolvable. It deliberately does not label a feed mechanism fair, random, safe or production-ready. Source demand is not sensed, so repeatedly serving an empty opposite chute is not evidence of unfairness. Use comparable loading conditions and document them externally.

A phone app alone cannot observe three hidden passage points on a common clock. Video remains a valuable independent reference, but manually reviewing long test sequences is inconvenient. The hardware offers immediate cumulative feedback without recording visitors, faces or audio, and continues its embedded accounting if the companion is closed. The application preserves a bounded local history and exports evidence with transport warnings. There are no cloud accounts, radio links, remotely configurable policies or moving outputs.

## Originality and alternative concepts

Repository inventory preceded implementation and covered device folders and README purpose summaries. The closest internal designs were reviewed in more detail. Route Census associates one inlet release with one of three outlets and rejects concurrent releases; Merge Rhythm instead records two-source ordering through a bounded shared FIFO and measures same-source streaks. FoldTrace checks a fixed sequence of folding leaves with dwell requirements; this design accepts arbitrary source sequences and reports them rather than prescribing an order. Teardown Stack records reverse retrieval of uniquely docked cups; this FIFO contains transient source tags, permits repeated tags, and has downstream transit deadlines. Sand Stroke records face-separated endpoint traversals and supplies reusable low-level board support, not the interleaving algorithm.

Industrial merging is established technology. A limited prior-art search found [US20100252398A1](https://patents.google.com/patent/US20100252398A1/en), describing a timing conveyor and high-friction merge conveyor, and [EHS merge systems](https://www.ehs-ct.com/merge-systems/), describing zipper merging and product detection. Those precedents rule out any claim that two-lane merging or alternation itself is new. The proposed contribution is a low-energy removable observer with explicit lost-correlation accounting, deliberate empty-channel baselines and a local source-streak notebook. This is an original repository implementation, not a patent search or a guarantee of worldwide novelty.

Three candidate scopes were considered. A motorized corrective merger would be useful but would add pinch hazards and control responsibility, so it was rejected. A camera-only source classifier would avoid contact drag but require lighting, calibration and image processing beyond this bounded instrument. The chosen passive observer makes its sequencing assumptions inspectable and is plausible for a maker to assemble below a roughly USD 100 planning budget. Mechanical feasibility remains the main uncertainty, not processor capacity.

## Measurement contract and essential assumption

The FIFO is an inference, not object identification. Its validity requires that physical objects cross the source contacts in the same order that they later cross EXIT. Place source contacts at the final entry throats, not far upstream where one branch can queue while the other overtakes. Use a narrow shared channel that prevents side-by-side travel, reversal and overtaking. Source selection must be mechanically resolved before the admitted object becomes invisible. If the mechanism cannot satisfy this condition, this revision is unsuitable; use tagged objects or an independent tracking camera instead.

For a first fixture, use slow, hand-fed wooden sliders or large opaque tokens in an enclosed tabletop guide. A lightweight hinged flap can carry a retained magnet past a low-current reed switch; alternatively use a suitably light gold-contact lever switch. Gate geometry, magnet retention, lever return and token clearance must be designed for the actual exhibit. The supplied electronics are complete for the passive carrier, but this repository does not include a universal chute CAD model. Fixture dimensions cannot be universal because the tested mechanism is the experimental subject.

Each closure and separating opening should last at least twelve milliseconds during initial experiments. Firmware accepts eight consecutive nominal one-millisecond samples per channel, so shorter pulses can disappear. A held contact produces only one admission; it does not count a queue of touching objects. The device therefore cannot measure packed flow without a separating gate mechanism. Bounce asymmetry can change the apparent ordering of close events. Keep feeds slow and separated, then compare every admission against video before trusting source labels. Contact force may itself change the interleaver's behavior and must be checked with instrumented and uninstrumented trials.

At most eight admissions may be pending and every accepted arrival must occur strictly before five seconds from its qualified admission. An arrival exactly at five seconds is overdue. A source event and exit in the same qualified sample may retire an already pending object and then append the new source. When no older object is pending, that combination is an orphan exit and the new source is rejected. Two simultaneous qualified source rises are unordered and invalidate the segment. No guess based on left/right priority is made.

## Hardware specifications and block diagram

| Item | Revision A design |
|---|---|
| MCU assembly | NUCLEO-F401RE, STM32F401RET6 Cortex-M4 |
| Processor clock | Internal HSI, nominal 16 MHz; no PLL requirement |
| Address space | 512 KiB flash and 96 KiB SRAM |
| Acquisition | Three dry-contact gates plus one dry-contact CLEAR button |
| Bias network | Four 10 kohm pull-ups, four 1 kohm series resistors |
| Host transport | USART2 TX, 115200 baud, 8N1, ST-LINK virtual COM |
| Embedded power | Nucleo USB; carrier receives regulated 3.3 V from Nucleo |
| PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Assembly | Through-hole resistors and 2.54 mm pin headers |
| Companion | Python 3, Tk, Linux termios; standard library only |

    LEFT throat ---- J3 -- 1k -- SENSE0 -- PA0 / A0
    RIGHT throat --- J4 -- 1k -- SENSE1 -- PA1 / A1
    Shared EXIT ---- J5 -- 1k -- SENSE2 -- PA4 / A2
    CLEAR button --- J6 -- 1k -- SENSE3 -- PB0 / A3
                                |
                           10k each to 3V3
                                |
    USB --> external NUCLEO-F401RE --> J1 3V3/GND
                   |
             SysTick / qualifier
                   |
           bounded FIFO / diagnostics
                   |
       CRC snapshot --> USART2 PA2 --> ST-LINK USB
                                              |
                                     local Tk notebook

The processor, regulator, MCU decoupling, reset circuitry, SWD programmer and USB bridge are on the purchased Nucleo assembly. They are not fictitious parts hidden in the carrier schematic. The custom board has fourteen physical components and ten nets. Real resistor/header symbols, numbered pins, shared net labels, matching footprints, routed copper, a via and a closed Edge.Cuts outline are included. ERC checks the carrier only; an all-passive schematic cannot certify the external supply or jumper wiring.

## Pin allocation and wiring

| Carrier connection | External destination |
|---|---|
| J1 pin 1 VCC | Nucleo CN6 pin 4, regulated 3V3 |
| J1 pin 2 GND | Nucleo CN6 pin 6, ground |
| J2 pin 1 SENSE0 | CN8 pin 1, A0, PA0, LEFT |
| J2 pin 2 SENSE1 | CN8 pin 2, A1, PA1, RIGHT |
| J2 pin 3 SENSE2 | CN8 pin 3, A2, PA4, EXIT |
| J2 pin 4 SENSE3 | CN8 pin 4, A3, PB0, CLEAR |
| J3 through J6 pin 1 | Corresponding isolated contact signal |
| J3 through J6 pin 2 | Other contact terminal, common ground |
| R1/R3/R5/R7 | 10k pull-ups from VCC to SENSE0/1/2/3 |
| R2/R4/R6/R8 | 1k from corresponding SENSE to contact signal |

A closed contact reads low electrically; the GPIO driver inverts it so a one bit means closed. Do not connect J1 to the Nucleo 5 V terminal. Use the board's documented factory virtual-COM path: SB13/SB14 fitted and SB62/SB63 open. PA2 is configured for USART2 AF7 transmit. The receiver is disabled, and sending bytes from a host cannot rearm the instrument or change its timing policy. SWD through onboard ST-LINK is the programming route.

The platform references are ST's [UM1724 Nucleo-64 manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf) and STM32F401 RM0368 reference manual. Register definitions are the small peripheral subset actually used, not an invented general HAL. The latest online UM1724 download could not be retrieved during this run; the pin/bridge map follows the repository's existing reviewed platform. Verify your physical MB1136 revision against the manufacturer's manual before wiring. No firmware build substitutes for that inspection.

## Power, components and packaging

At nominal 3.3 V an ideally grounded contact draws 3.3/11000 A, or 0.300 mA. The MCU SENSE node is approximately 0.300 V. Four simultaneously closed channels draw 1.200 mA; this is a circuit calculation, not a measured current. Specify 1% resistors and contacts suited to low-level switching. A large oxidized power switch may be unreliable at this wetting current. A candidate lever contact is Omron D2F-01L, subject to checking its travel, actuation force and low-level contact specification for the fixture. A reed assembly may reduce drag but requires secure magnets and careful hysteresis testing.

| Quantity | Item | Planning cost, USD |
|---|---|---:|
| 1 | NUCLEO-F401RE | 25 |
| 1 | Small two-layer carrier PCB | 8 |
| 8 | Four 10k and four 1k axial resistors | 2 |
| 6 | Five 1x2 and one 1x4 headers | 3 |
| 3 + 1 | Passage contacts and momentary CLEAR button | 12 |
| allowance | Cable, enclosure and removable brackets | 20 |

These are planning allowances, not live supplier quotations. Shipping, tools and the experimental chute are excluded. Reserve 150 mA at the Nucleo USB input as a conservative assembly allowance and use a reputable regulated USB supply capable of at least 500 mA. Actual Nucleo, debugger and LED current must be measured. Firmware busy-polls between ticks, so this is not a battery-optimized design. There is no charger, lithium cell or claimed runtime.

Mount the carrier and Nucleo in an insulated bench enclosure with separate retention and cable strain relief. The carrier has no mounting holes; use edge supports rather than drilling through unknown copper. A provisional enclosure envelope of 120 by 100 by 45 mm is a packaging target, not supplied mechanical CAD. Use short twisted signal/ground pairs, initially under half a metre. Keep cables away from the moving chute and loose objects. The design has no isolation, TVS network, certified ESD immunity or external-voltage protection. Never attach it to powered machinery switches, industrial 24 V sensors, mains circuits or building wiring. The series resistors do not make those connections safe.

## Firmware implementation and decisions

The firmware is freestanding C11 with startup code and a linker script for the actual MCU memory map. `main.c` establishes the reset HSI clock, initializes GPIO/UART/SysTick, checks configuration and starts the independent watchdog. Acquisition processing feeds the watchdog; UART draining does not block acquisition. Clock setup waits have finite iteration budgets before SysTick exists, and watchdog initialization has bounded tick-based waits. A clock-setup failure stops before serial initialization. Configuration or watchdog failure yields a terminal fault snapshot rather than healthy-looking measurements.

Each input qualifies independently. Missing even one expected sample invalidates acquisition continuity: a delta other than one millisecond discards the current FIFO into abandoned admissions, restarts qualification and returns to WAIT. Repeated calls at the same timestamp do nothing. Unsigned subtraction handles ordinary millisecond wrap. No sampling gap is stretched into apparently continuous debounce stability. A gap does not erase cumulative deliveries, latencies or diagnostic counters.

WAIT requires a fully qualified all-open baseline, followed by CLEAR alone held for one hundred qualified samples. The suggested operator press of at least 120 ms includes debounce margin. Arming increments `segments`, but CLEAR must be released before feeds begin. A qualified gate during that held CLEAR invalidates the baseline. A new CLEAR rise while already ARMED abandons pending admissions and returns to WAIT; holding it indefinitely does not repeatedly rearm. Release everything, empty the channel, then press again. HALT cannot be cleared by the button and requires a new firmware run after recording the reason.

The FIFO stores eight source tags and their qualified admission timestamps. It never uses dynamic allocation. At an accepted exit the engine accumulates a positive latency strictly below 5000 ms, increments the appropriate source total and updates consecutive-source streaks. Streaks continue through an idle but still correlated period; they restart after correlation loss. Alternations count source changes between successive deliveries within correlated segments, not across resets or rearming. The longest streak is retained cumulatively. This prevents an unknown interruption from inventing a new alternation.

The invariant is `admitted = left + right + abandoned + pending`. Rejected source edges were never admitted. Activity while WAIT is outside coverage and is not claimed as observed flow. Diagnostics describe causes rather than an exhaustive event log: clear has precedence over timeout, timeout over outlet acceptance, and ambiguous dual input over ordinary retirement. Counters stop before one billion. Coupled outlet-plus-inlet updates are staged on a temporary engine copy; exhaustion rolls back the entire sample and abandons the original pending FIFO exactly once. This matters because incrementing delivery before discovering admission overflow would otherwise publish half a transaction.

All accounting is volatile. There is no persistent boot identifier, flash journal or device-side archive. Power loss can erase evidence before the host receives it. A new run cannot prove continuity with an older one. The companion detects many resets through timestamp and counter regressions, but a reset whose values advance beyond the last received snapshot can remain indistinguishable. Export before resetting and start a clearly separate experiment afterward.

## Protocol and companion application

Every periodic line starts with `MR1,`, carries twenty-three decimal unsigned fields, then an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT uses initial value 0xffff and polynomial 0x1021 over bytes before the asterisk. The marker is a policy version repeated in every snapshot; it is not a boot notification. Field order is defined identically in `firmware/wire.c` and `app/protocol.py`:

    now state mask known pending admitted left right abandoned rejected
    alternations longest streak latency_sum latency_max segments ambiguous
    orphans full timeouts gaps tx fault

State values are WAIT=0, ARMED=1 and HALT=2. Contact bit order is LEFT, RIGHT, EXIT, CLEAR. Fault bits are configuration=1, watchdog=2, exhaustion=4 and internal invariant=8. Frames are emitted approximately every 100 ms when transmission is available. The UART has a bounded buffer and a 100 ms drain deadline; transport failure increments `tx`. The receiver resynchronizes on the complete marker after truncated transmissions. CRC detects corruption, not a malicious local sender, and the serial port should remain under the operator's control.

The Linux desktop has three functional tabs. Connection/baseline opens one selected serial device at 115200 8N1 and explains wiring and physical clearing. Interleaving/transit displays source totals, pending count, current and longest streak, accepted latency mean/maximum, and diagnostics. History/export retains the most recent 500 accepted snapshots and writes a new owner-only CSV without overwriting an existing file. Export metadata includes author, provenance, connection/freshness, parser errors, evicted rows and warnings even when no data rows exist.

Replay accepts a regular MR1 recording no larger than two MiB and rejects empty files, corruption, nonregular files and partial tails. Replay never labels data live. CSV exports are analysis records, not MR1 input; do not feed an export back into the replay selector. A synthetic recording can be generated from the C test encoder for demonstration, explicitly separate from physical observations. Failed replay or failed connection setup preserves prior evidence. A successful new connection starts a new notebook and clears stale numeric labels. Disconnect keeps received data and provenance while revoking live status.

Two advancing snapshots are required before showing live progress. Duplicate timestamps do not refresh it. A one-second lack of forward progress marks the display stale. Accumulated host/device elapsed-time disagreement beyond a small bounded allowance freezes the session and requires reconnect, protecting against serial backlog and clock jumps. This is conservative: internal oscillator drift can eventually force reconnect during long sessions. An initially queued but internally consistent stream may still be indistinguishable from live traffic. Fresh transport also does not establish valid mechanical correlation; always read WAIT/ARMED/HALT and the diagnostic counters.

## Build and reproducible checks

From the repository root, with GNU Arm Embedded GCC, host GCC, Python 3/Tk, Xvfb and KiCad 9 installed:

    make -C merge-rhythm/firmware all test
    xvfb-run -a python3 -m unittest discover -s merge-rhythm/tests -p 'test_*.py' -v
    /usr/bin/python3 merge-rhythm/tools/check_hardware.py
    python3 merge-rhythm/tools/check_quality.py
    python3 merge-rhythm/app/workbench.py

The Makefile cross-links the actual Cortex-M4 image and produces an ELF, map and binary under ignored `firmware/build/`. Host engine tests use address and undefined-behavior sanitizers. They exercise all 256 eight-admission source patterns, timeout boundaries, independent qualification, rearming, FIFO conservation and exhaustion. Register-storage tests cover clock masks and nonblocking UART behavior; they are not electrical emulation. GUI tests use an actual Tk event loop with Linux pseudo-terminals and records produced by the real C encoder. They are synthetic transport tests, not a flashed-device demonstration.

`tools/generate_hardware.py` reproduces the carrier from authored symbols and installed KiCad footprints. `tools/check_hardware.py` runs ERC and DRC, exports a fresh XML netlist and compares every reference/pin/net, value and footprint against PCB pads and `connectivity.json`. A schematic render was inspected for legibility. The hardware generator intentionally reuses the reviewed carrier architecture. `tools/import_platform.py` documents source provenance and refuses existing destinations; it is not a normal build step. No GitHub Actions or repository-wide automation is added.

To generate an explicitly synthetic replay after tests, redirect `merge-rhythm/firmware/build/test-engine --fixture` to a file outside the repository, then select that file in the application. To program a physically inspected assembly, use the Makefile `flash` target with OpenOCD and ST-LINK. Flashing was not performed here. Use normal permissions for the serial device; avoid running the desktop as root in a deployed bench installation.

## Bring-up, experiment design and remaining work

Before power, check resistor values, contact polarity, continuity, adjacent-pad shorts and the 3V3/GND harness. With all contacts disconnected, power the Nucleo and measure its supply. Connect the carrier, close each contact independently and measure the corresponding SENSE voltage; verify that no other bit changes. Confirm CLEAR cannot operate a mechanism because there is no actuator path. Then compare a known-duration contact stimulus with a scope or logic analyzer to establish actual sampling, debounce and UART behavior.

Start mechanical trials one object at a time and compare source labels, exit counts and transit times with video. Next use alternating and repeated sources, deliberately obstruct an exit, trigger two source gates together and exceed the pending limit. Every rejection should remain visible and require deliberate physical clearing. Check that contact springs do not alter which branch wins. Test slow rollback, bouncing tokens, a stuck switch and a disconnected lead; document cases that cannot be distinguished electrically. An open wire can look like an idle gate, so the instrument is not self-diagnosing.

Only after those checks should a technician compare gate designs under repeatable loading. Keep the same objects, slope, contact locations and feed protocol, and report rejected episodes alongside accepted statistics. Do not divide alternations by delivered count and call it fairness: absent demand and censored failures bias such an interpretation. Useful future work includes a demand-aware fixture, tagged-object verification and measured low-force gates, but none is claimed implemented. This revision is a transparent design and executable notebook for a narrow experiment, not a certified instrument or a finished commercial product.
