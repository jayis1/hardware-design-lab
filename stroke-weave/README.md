# Stroke Weave — full-stroke ordering notebook for twin-piston teaching pumps

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, tests, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint-library geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The supplied custom PCB is a routed passive contact interface for a separately purchased NUCLEO-F401RE. The processor, regulator, debugger, USB bridge and processor decoupling are on that purchased board, not duplicated on this carrier. Target linking, simulated contact histories, desktop operation and electrical CAD checks are recorded separately in VALIDATION.md. No physical fabrication, target programming, pumping performance or calibrated measurement is claimed.

## Purpose and practical utility

Stroke Weave helps a workshop educator investigate how two independently hand-driven pistons are coordinated. A transparent twin-cylinder air-pump model can produce an irregular rhythm even when both handles visibly move. Counting handle motions confuses full strokes with short reversals. Merely counting arrivals at the delivery end also accepts a piston that was already there at startup, or one that left and returned without reaching its refill end. The useful question is narrower: which cylinder completed each witnessed full delivery-direction excursion, and did that sequence alternate or repeatedly use the same cylinder?

Four removable magnetic contacts mark the home and delivery endpoints of the two pistons. The electronics independently recognize each cylinder's excursions through the clear region between its endpoints. They count complete delivery strokes, complete refill strokes, returns to origin and excursions abandoned by a fault. A shared-clock ordering stage then counts alternating and repeated-cylinder delivery pairs, retains the current same-cylinder streak and its peak, and explicitly marks simultaneous qualified deliveries without choosing an artificial order.

The intended user is a maker-space instructor, museum workshop technician or mechanical teaching-model builder. The instrument is useful when comparing a free-hand operating procedure with a simple mechanical coordination linkage. A student can perform a slow sequence, export the evidence and compare repetition and cancellation counts before and after changing the procedure. The output is a notebook of electrical endpoint observations, not a judgment of operator skill or an automatic diagnosis of a pump fault.

A teaching example uses two independent sliders representing pistons, initially without tubing or pressure. Ask the operator to alternate delivery strokes while allowing either refill order. An A delivery, B delivery, B delivery sequence produces one alternating pair and one repeated pair. A half movement followed by return to its own origin produces a cancellation but not a delivery. This distinction is the reason to use two endpoint witnesses per cylinder instead of a single stroke counter.

The target device has no actuator, solenoid, relay, control loop or command receiver. It does not start a pump or inhibit a handle. It is not for medical dosing, breathing apparatus, fuel transfer, pressure testing, industrial maintenance or safety certification. “Delivery” is a direction label defined by the fixture; it does not establish that fluid moved, a valve sealed or a particular quantity was dispensed. Dry, low-energy, manually operated teaching mechanisms are the only intended environment.

## Originality, alternatives and provenance

The repository's device directories and README purpose summaries were inspected before selecting this concept. Bellows Phase observes valve positions during the strokes of one bellows; it does not reconstruct independent paired piston strokes or their delivery word. Rail Skew compares maintained ordinal position bands on two mechanically related drawer rails; it does not permit independent piston refill cycles and classify full delivery ordering. Pallet Census attributes wheel arrivals to pallet positions; its arrivals are strobes, not independently proven origin-clear-destination excursions. Stroke Weave changes the physical experiment, state machine, retained evidence and user interpretation.

A generic logic analyzer and custom script could perform this experiment. Video annotation could recover more mechanical detail. The proposed contribution is an inspectable, removable four-contact fixture and a bounded full-stroke/order workflow, including explicit cancellation and tie semantics. No worldwide novelty, patentability or freedom-to-operate claim is made. A bounded public search for twin-piston hand-pump stroke-logging equipment returned general pump-testing prior art, including US7047802, which describes instrumented pump actuation and performance recording. That establishes that pump instrumentation itself is not new; it does not establish that this particular implementation is commercially unique. The search is not a patent clearance review.

The passive carrier, STM32 board support, serial transport and CAD verification utilities reuse jayis1 sources reviewed from pallet-census at repository commit 182df31. tools/import_platform.py records the source file set and substitutions, and refuses to overwrite existing destinations. The new engine, TW1 field contract, companion model, application presentation and pump fixtures are local to Stroke Weave. Reusing a known electrical interface is intentional; this design does not claim to invent pullup resistors or magnetic endpoint sensing.

## Hardware specification and architecture

The processor is STM32F401RET6 on ST NUCLEO-F401RE MB1136: Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware explicitly restores the internal nominal 16 MHz HSI clock tree. A SysTick-derived millisecond clock drives foreground acquisition. USART2 TX on PA2 alternate function seven transmits ASCII snapshots through the ST-LINK virtual COM bridge at 115200 baud, eight data bits, no parity and one stop bit. USART reception is disabled. There is no radio, FPGA, battery, charger, external memory or sensor bus requiring additional software dependencies.

The custom board is 80 by 70 mm, nominally 1.6 mm FR-4 with two copper layers. It contains four 10k pullups, four 1k series resistors, five two-pin headers and one four-pin header. The resistors use axial DIN0207 footprints with 7.62 mm lead spacing. Headers are straight 2.54 mm pitch. Tracks are 0.30 mm wide with a default 0.20 mm clearance. There is one through via in the ground route. The carrier is not a mechanically pin-compatible Nucleo shield: connect it with short labeled insulated leads.

```text
Cylinder A home ---- dry reed ---- J3 --- 1k --- SENSE0 / PA0
Cylinder A end ----- dry reed ---- J4 --- 1k --- SENSE1 / PA1
Cylinder B home ---- dry reed ---- J5 --- 1k --- SENSE2 / PA4
Cylinder B end ----- dry reed ---- J6 --- 1k --- SENSE3 / PB0
                                                 |
                                    each SENSE has 10k to 3V3
                                                 |
USB power -> NUCLEO-F401RE -> independent 5-sample qualification
                                                 |
                                  two independent excursion machines
                                                 |
                             full-delivery grouping and order/streaks
                                                 |
                           CRC snapshots -> USART2 -> ST-LINK USB VCP
                                                 |
                         Linux/Tk evidence / trace / wiring screens
                                                 |
                              strict replay and private CSV export
```

### Fixed connector and pin contract

| Connector | Connection | Meaning |
|---|---|---|
| J1 pin 1 | Nucleo CN6 pin 4 | 3V3 source, net VCC |
| J1 pin 2 | Nucleo CN6 pin 6 | Common GND |
| J2 pin 1 | CN8 pin 1 / A0 / PA0 | SENSE0, A_HOME, logical bit 0 |
| J2 pin 2 | CN8 pin 2 / A1 / PA1 | SENSE1, A_END, logical bit 1 |
| J2 pin 3 | CN8 pin 3 / A2 / PA4 | SENSE2, B_HOME, logical bit 2 |
| J2 pin 4 | CN8 pin 4 / A3 / PB0 | SENSE3, B_END, logical bit 3 |
| J3, J4, J5, J6 pin 1 | Corresponding endpoint contact | A_HOME, A_END, B_HOME, B_END |
| J3, J4, J5, J6 pin 2 | Other terminal of contact | GND |

The square pad indicates pin one. Verify pad numbering against the actual mating harness rather than assuming a wire color. A closed contact pulls the input low and maps to logical one. The MCU's internal pull resistors are disabled. Each pullup is on the MCU side of its series resistor. At ideal nominal 3.3 V, a closed contact draws 3.3/11000 A, approximately 0.300 mA, and the MCU-side node is approximately 0.300 V. All four closed contacts draw approximately 1.20 mA. These are nominal circuit calculations, not measured guarantees of logic margin or wetting reliability.

Supply the Nucleo through its normal USB arrangement. Reserve 150 mA at USB 5 V as an initial system planning allowance and measure the actual purchased assembly. The carrier has no independent regulator, power switch or supply sequencing circuit. Feed J1 only from the Nucleo regulated 3.3 V output; never connect a second supply to that node. The external assembly supplies its own processor decoupling and reset network, so the carrier must not be interpreted as a complete bare-MCU design.

Only disconnected voltage-free contacts are supported. Never attach powered pump wiring, PLC signals, 5 V modules, motor terminals, mains wiring or unknown transducers. The board offers no galvanic isolation, TVS network, qualified ESD immunity or industrial hot-plug protection. USB makes computer ground common with the contacts. The series resistor is not an isolation barrier. Begin with cables shorter than half a metre on a dry indoor bench, and inspect interference with an oscilloscope before extending them.

Manufacturer references for the reused processor platform are ST UM1724 for the NUCLEO-64 MB1136 assembly, RM0368 for STM32F401 registers, and PM0214 for Cortex-M4 behavior. Confirm the purchased board revision and ST-LINK VCP routing; the platform assumes SB13/SB14 connected and SB62/SB63 open. These references do not replace a board-level bring-up test. No supplier availability or manufacturer approval is asserted.

## Bill of materials and fixture design

The electrical BOM is one NUCLEO-F401RE; one supplied carrier PCB; four 10k and four 1k, 1% axial resistors rated at least 0.125 W; five 1x2 headers; one 1x4 header; insulated mating leads; and a USB data cable. Four Littelfuse MDSR-7-10-15 normally open reed contacts are candidate endpoint sensors. They require protective holders, captive magnets and characterization at the actual low contact current. Equivalent signal-rated dry contacts may be used after testing; oxidized power switches are not assumed reliable at this excitation current.

Plan approximately USD 80 for the electronics, small-quantity carrier, sensors and ordinary harness materials, excluding labor, shipping, tools and the pump model. This is a budgeting assumption rather than a current supplier quote. An insulating enclosure approximately 160 by 110 by 45 mm can contain the Nucleo and carrier side by side. The carrier has no mounting holes; use insulated edge supports, not improvised drilling through routed copper. Provide strain relief and visible endpoint labels.

For initial validation, use two parallel captive sliders on a tabletop base, each with about 60 mm available hand travel. Put separate hard stops at the two ends. A small retained magnet moves with each slider, while fixed protected reeds mark its home and end zones. Set narrow but repeatable closure regions close to the hard stops, leaving a broad interval where neither contact closes. The contacts must not act as structural stops. The exact travel and closure-zone dimensions are fixture assumptions to be measured, not supplied mechanical manufacturing tolerances.

Mount cylinder A and B sensors far enough apart to avoid cross-triggering. Confirm every combination with the other piston stationary before recording. If a future demonstration adds transparent cylinders, operate only freely vented room-temperature air at low hand force. Do not seal outlets or use a reservoir to build pressure. The electronics are outside the moving mechanism, with wires retained clear of handles and pinch regions. Glass reeds need encapsulation and lead strain relief; small magnets must not become loose parts accessible to children.

Before each experiment, move each piston from home through the middle to end and back while observing all four raw and qualified bits. Both contacts on one cylinder must never indicate closed together. At the selected slow motion speed, hold every endpoint and the middle region for at least twenty milliseconds; initial complete strokes should take around one second. Sensor magnets and cables must not materially change motion. If instrumentation adds drag or attraction, redesign the bracket before interpreting comparisons.

## Firmware and measurement decisions

Each of the four contacts qualifies independently after five consecutive matching raw samples. The first matching sample counts as one, giving four observed intervals across the qualification window. Independent qualification prevents chatter on cylinder B from postponing a sustained endpoint on cylinder A. Pulses shorter than the qualification window may be rejected; transitions entirely between samples are invisible. Contact edges are electrical proxies with hysteresis and bounce, not precise physical coordinates.

Each cylinder has five phases: UNKNOWN, HOME, END, OUTBOUND and RETURNING. An initially unknown lane acquires an origin only when its qualified endpoint is unique and agrees with raw input. Starting with neither endpoint closed cannot invent a prior stroke. Starting at END is legitimate; the first complete excursion back to HOME is a refill, not a delivery. There is no global RUN contact and no requirement that both lanes acquire their origins simultaneously.

A lane at HOME enters OUTBOUND when its qualified endpoints become clear and the raw pair is also clear. This increments starts and sets its active age to zero. A lane at END similarly enters RETURNING. An opposite endpoint appearing without an observed clear middle region is a terminal boundary fault, not a full stroke inferred from missing evidence. While traveling, age increases once per consecutive acquired sample. Arrival must have a unique qualified endpoint agreeing with raw input.

An OUTBOUND arrival at END increments deliveries; a RETURNING arrival at HOME increments refills. Either stores its full-stroke duration in last_ms. Returning to the same origin increments canceled instead, leaves last_ms unchanged, and reestablishes that endpoint as the origin for another attempt. Cancellations are real witnessed excursions, but do not change the full-delivery sequence. The counters obey starts = deliveries + refills + canceled + abandoned + one if traveling, independently for each cylinder.

A travel age reaching 60000 milliseconds rejects the excursion before evaluating an arrival on that sample. Timeout is terminal for the entire recorder because subsequent delivery ordering would otherwise bridge lost correlation. Qualified simultaneous home and end indications are a geometry fault. A skipped sample is a continuity fault. Configuration failure, watchdog-start failure and counter exhaustion also halt acquisition. Every pending lane is abandoned exactly once, and phases become UNKNOWN. Retained counts, ordering, active ages and last durations freeze after halt; wall time and UART diagnostics may advance. The rejected sample's speculative age is rolled back, so a timeout snapshot may retain age 59999 rather than 60000.

Both lane updates and their delivery grouping are transactional. If B fails on the same sample as a potential A completion, neither speculative success is published; all previously active lanes are abandoned. This conservative policy avoids claiming a valid order across an ambiguous boundary. Each lane permits at most one million starts. Counter limits are checked before accepting the staged state. Sample-sequence exhaustion also halts rather than wrapping silently, whereas the millisecond timestamp is interpreted modulo 32 bits.

When exactly one lane completes a delivery, it becomes the ordering anchor. If it matches the previous anchor, repeated increments and the current streak grows. If it differs, alternating increments and the streak becomes one. The first delivery has no preceding pair. The largest observed streak is retained in peak. Refills and cancellations do not break this sequence. When both deliveries qualify in the same sample, both per-lane delivery counts increment and ties increments once; anchor and streak clear. The next single delivery begins a new chain. A tie means equal qualified sampling endpoints, not proof of physically simultaneous motion.

Startup initializes data and BSS using a supplied vector table and linker script. Clock waits are bounded. GPIO, RCC, SysTick, USART and independent-watchdog access use actual STM32F401 registers. The target has no heap, operating system, floating-point requirement or synthetic input path. UART transmission is nonblocking, bounded to a complete frame buffer and expires after 100 ms if stalled. The foreground feeds the watchdog after sampling. If reset clock restoration fails, firmware stops before starting communication. Reset discards all volatile evidence; no persistence or automatic experiment resumption is promised.

## Protocol and desktop application

TW1 is a versioned fixed-policy ASCII snapshot emitted approximately every 100 ms. Each record starts with TW1 followed by comma-separated unsigned decimal values. The fields are: sequence, now, state, fault, raw, qualified, valid; then for A and B in that order, phase, starts, deliveries, refills, canceled, abandoned, age, last_ms; finally alternating, repeated, ties, anchor, streak, peak, tx_errors, gaps. Values have canonical decimal spelling. State is zero for RUNNING and one for HALT. Phase values zero through four follow the phase order above. Fault bits are configuration 1, watchdog 2, gap 4, deadline 8, boundary 16, counter 32 and geometry 64; exactly one terminal cause is retained.

The payload is followed by an asterisk, four uppercase hexadecimal CRC digits and LF. CRC is CRC-16/CCITT-FALSE with polynomial 0x1021, initial 0xFFFF and no final XOR, covering TW1 through the final decimal digit. The encoder and companion validate accounting and ranges in addition to CRC. CRC detects accidental corruption, not forgery. There is no authentication or remote update endpoint. Use trusted local serial access and trusted recordings.

The Python standard-library Tk application provides Stroke evidence, Trace, and Wiring and limits screens. Connect opens a Linux serial device at 115200 8N1, flushes old input and begins a new live session. Disconnect preserves retained evidence and provenance. A second connection is refused while a port is owned. Failed opening does not erase prior evidence. The driver restores original terminal settings on close and detects hangup. Replay accepts regular files up to two MiB, rejects incomplete trailing frames, and never marks synthetic or recorded data live.

The model retains the latest 512 snapshots, validates monotonic counters and necessary inter-frame sample budgets, and freezes on contradictory history. These checks are not a reconstruction or proof of every hidden event between snapshots. Acquisition is labeled advancing only after sequence progress, while connected, without a warning or halt, and with recent progress. A fixed two-second host/device progress discrepancy blocks a live session; an uncalibrated oscillator can exhaust that budget during a long run, requiring export and reconnect. Receive-only telemetry without a persistent boot identifier cannot distinguish every possible reset from valid forward traffic.

CSV export uses exclusive creation and mode 0600, refusing to overwrite an existing file. It includes author, provenance, connected state, freshness, parser-error count and warning before measurement rows. Export before resetting the observer or changing experimental conditions. The application is a live notebook, not an unattended archival service. A disconnected session keeps its original live provenance but explicitly exports connected=false and fresh=false. No cloud account, phone permissions, microphone, camera or online service is needed.

## Build, reproduce and commission

From this directory run `python3 tools/verify.py`. The verification tool rebuilds freestanding Cortex-M4 ELF and binary images, runs sanitizer-enabled C tests, builds a synthetic history using the real encoder, exercises the Python model and GUI under Xvfb, and checks KiCad ERC, DRC and exact schematic-to-PCB pin mapping. It also counts README words, substantive production C lines, required files and authorship. Dependencies are GNU make, host GCC, arm-none-eabi-gcc/binutils, Python 3 with Tk, Xvfb, and KiCad 9 including system Python pcbnew bindings. No pip or npm dependencies are required.

Run `/usr/bin/python3 app/workbench.py` to open the desktop application. For an explicitly synthetic demonstration, build with `make -C firmware fixture` and replay firmware/build/demo.sw1. The file extension is a local recording convention; its actual wire marker is TW1. To program a purchased target after inspecting wiring and confirming its identity, use `make -C firmware flash`, which invokes OpenOCD with ST-LINK and stm32f4x configuration. That target is provided for the assembler and has not been executed against physical hardware in this design run.

Commissioning must begin unpowered: inspect orientation, continuity, resistance and shorts. Power the Nucleo alone, verify its regulated output, attach the carrier, then operate each isolated sensor. Check serial bit mapping and logic margins with instruments. Repeat complete, canceled and tied synthetic actions on the physical fixture while recording synchronized video. Compare counts and durations against annotated video, investigate bounce and threshold hysteresis, test USB disconnection and reset, and measure current consumption. None of those physical checks can be replaced by a successful C build or clean CAD rules.

The appropriate release interpretation is a complete, reproducibly checked engineering design package with explicit physical validation still outstanding. It is not a production-approved instrument, a calibrated flow meter or a safety device. Record fixture dimensions, magnet positions, contact part numbers, test conditions and operator procedure alongside every real experiment so future comparisons have an interpretable physical basis.
