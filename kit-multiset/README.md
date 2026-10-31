# Kit Multiset — unordered kitting-stroke evidence fixture

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and locally authored hardware design sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is a build-tested engineering prototype, not an inspected-parts counter or production quality system. The PCB is a routed passive carrier for an external NUCLEO-F401RE. No prototype has been assembled, fabricated, flashed or physically calibrated. Software and CAD validation cannot establish electrical robustness, switch repeatability or practical benefit to a learner.

## Purpose and the ordinary-day test

Kit Multiset is a small teaching fixture for preparing the same simple kit repeatedly without prescribing the order of picks. Three storage bins have spring-return declaration paddles, and one receiving tray has a presence switch. The example recipe is two declarations from bin A, one from bin B and one from bin C. The operator may visit the bins in any order, including returning to A after visiting another bin. Removing the tray closes the observed batch. The device retains its three stroke counts and classifies the batch as recipe-matching, mismatching or abandoned because observation became unreliable.

On an ordinary Tuesday, a makerspace instructor prepares classroom construction kits containing two washers, one spacer and one sleeve. A learner who finds an assembly instruction difficult can use a physical station rather than repeatedly touching a tablet. The instructor first demonstrates taking one item and pressing that bin's paddle once. The learner prepares a tray, removes it, and compares the physical contents with the recorded declaration. After several trials the instructor rearranges the bins and observes whether extra or missed declarations become less common. The point is to compare a workflow, not score a person or certify a finished assembly.

This is deliberately a multiset rather than a sequence checker. A,A,B,C and C,A,B,A are equally valid declarations. A,B,C is short; A,A,A,B,C is excessive. A held paddle is one declaration, not repeated credit. Two paddles can qualify together and both are counted when a previously established tray remains present. There is no imposed fastest route, no enforced hand order, and no motor or lock. These choices make the device suitable for instruction where multiple comfortable movement patterns are acceptable.

The hardware is necessary because the evidence comes from a physical tray boundary and three separate handles. An application alone cannot observe those contacts. Equally, these contacts cannot tell whether the operator actually picked a washer, picked two washers at once, returned an item, or filled the bins correctly. Every matching result means only that the observed declaration counts matched the fixed recipe. Physical inspection remains separate and mandatory. Do not use this revision for medicines, food allergens, hazardous substances, regulated manufacturing or safety-critical assemblies.

## Originality, alternatives and explicit limits

The repository directories and device README summaries were inventoried before implementation. The nearest local concepts were read more closely. Scoop Window records overlap between absent utensils and the opposite bin's open lid; it does not count recipe membership. Teardown Stack retains a dynamic last-in-first-out order of uniquely docked cups; it deliberately rejects many reorderings that Kit Multiset permits. Pattern Sled compares ordered mechanical program words against a learned reference. This device instead combines a physical tray delimiter, repeated same-bin declarations, order-independent multiplicity and conservative abandonment after loss of acquisition continuity.

Commercial pick-to-light systems already exist. Banner Engineering's [kitting and sequencing overview](https://www.bannerengineering.com/us/en/solutions/pick-to-light/kitting-and-sequencing.html) describes quantity displays, touch confirmations and optical pick detection. Its [PTL110 family](https://www.bannerengineering.com/us/en/products/lighting-and-indicators/pick-to-light/ptl110-series.html) is a relevant industrial alternative. Sorion's [kitting description](https://www.sorion-group.com/pick-to-light-for-kitting/) likewise describes button or optical confirmation. Those are genuine prior art, not systems this project claims to have invented. The proposed contribution is this small, transparent, non-guiding educational fixture and its explicit distinction between matching declarations and verified contents. Worldwide novelty, patentability and commercial absence have not been established.

Three candidate directions were considered before committing the concept. Scores below are engineering judgments from one design pass, with five meaning stronger fit; they are not market research results.

| Candidate | Repo distinctness | Prior-art differentiation | Practical utility | Buildability | Demonstrability |
|---|---:|---:|---:|---:|---:|
| Tray-delimited unordered recipe declarations | 4 | 2 | 4 | 5 | 5 |
| Another reverse-order parts organizer | 1 | 2 | 3 | 5 | 5 |
| Camera-based automatic tiny-part inspection | 4 | 2 | 4 | 2 | 2 |

The selected concept is intentionally narrower than an industrial picking controller. It avoids camera occlusion, identity databases, cloud services, powered machine interfaces and an expensive mechanical singulator. Its reusable MCU platform is derived from jayis1's Rebound Rungs revision 52a3bd1. The import script records exactly which startup, register, GPIO, serial, clock, carrier-generator and transport sources were reused. The recipe engine, protocol encoder, evidence model and presentation are specific to Kit Multiset. Reusing an electrical platform is not presented as electrical novelty.

## Mechanical assembly specification

Use a rigid, dry tabletop base approximately 420 mm wide and 280 mm deep. Reserve the rear row for three labelled parts bins and the front centre for a removable tray. Suggested bin footprints are 100 by 100 mm with a 20 mm gap between neighbouring bins. Reserve a 160 by 100 mm shallow locating recess for the tray. These dimensions are mock-up targets, not fabricated CAD or a claim of compatibility with a particular purchased organizer. Scale the base to actual bins before drilling.

Each bin gets a separate insulated spring-return declaration paddle mounted just in front of its lip. The paddle is a button-like acknowledgement, not a dispensing gate. Fit a low-level switch such as an Omron D2F-01L so the paddle presses the lever within the manufacturer's permitted travel; add an independent mechanical hard stop so a hand cannot overload the switch body. Keep the normally-open contact open at rest and closed while pressed. Use broad, rounded paddles, captive fasteners, guarded pinch regions and a finish that can be wiped without liquid reaching electronics. The tray operates the fourth normally-open switch only when seated at its locating stop.

Avoid mounting the tray switch where an empty hand normally rests. A tray that rocks can create apparent removals and new batches. Use three stable support pads and a dedicated switch-actuating tab. Adjust the tab rather than bending a switch lever beyond its specification. Mark both tray and recess with the same orientation arrow. The design does not identify the tray; swapping trays without an observed absence cannot establish a new batch. If the activity needs identity or several simultaneous trays, this four-contact instrument is the wrong architecture.

Enclose the 80 by 70 mm carrier in an insulating edge-retention housing of at least 100 by 90 mm internal clearance. The PCB has no mounting holes; do not drill arbitrary holes through copper. Put the Nucleo in a separate ventilated insulating enclosure, with USB and reset accessible. Use strain relief on all sensor leads and label both ends. Initially limit each contact lead to 0.5 m and keep it paired with ground. Do not route leads through a paddle hinge or under a movable bin. No ingress, drop, cleaning-chemical or electrostatic-discharge rating is claimed.

## Hardware, power and connector contract

| Function | Revision A implementation |
|---|---|
| Processor | External ST NUCLEO-F401RE MB1136, STM32F401RET6 Cortex-M4 |
| Memory | 512 KiB flash, 96 KiB SRAM on the purchased MCU module |
| Clock | Reset HSI, nominal 16 MHz; no external oscillator dependency |
| Sensors | Four external low-level normally-open voltage-free switches |
| Supply | Nucleo USB supply; carrier takes regulated 3.3 V and common ground |
| Host link | USART2 TX on PA2 AF7 through ST-LINK virtual COM, 115200 8N1 |
| Sampling | Nominal 1 ms; five consecutive matching observations per channel |
| Carrier | Two copper layers, 1.6 mm nominal thickness, 80 by 70 mm outline |
| Custom-board components | Six headers, four 10k pull-ups, four 1k series resistors |
| Connectivity | Local USB serial only; no radio, network stack or cloud account |
| Actuators | None; results never enable a machine or release a lock |

J1 pin 1 is VCC from Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 is ground from CN6 pin 6. Never connect 5 V or VIN to the carrier's VCC. J2 pins 1 through 4 are SENSE0 through SENSE3 and connect to CN8 pins 1 through 4 respectively. These are A0/PA0, A1/PA1, A2/PA4 and A3/PB0. The board is a wired carrier, not a mechanically mating Arduino shield.

J3 is BIN_A, J4 is BIN_B, J5 is BIN_C and J6 is TRAY. On each sensor connector, pin 1 is the named signal and pin 2 is ground. Each SENSE net has a 10k pull-up to VCC and a 1k resistor toward its sensor signal. Switch closure pulls SENSE low; the GPIO driver inverts it into an asserted logical bit. Bits zero, one and two represent A, B and C paddles; bit three represents tray presence. A closed contact therefore produces a telemetry one, not zero.

At nominal 3.3 V, a closed 11k path draws 0.3 mA and SENSE sits at 0.3 V. Four closed paths draw 1.2 mA. These are circuit calculations, not measurements. Select switches suitable for low-level contact currents; a switch that requires substantial wetting current is not interchangeable simply because it fits the bracket. Budget 150 mA at 5 V for the complete development-board assembly as an unmeasured planning allowance. Measure actual USB consumption before choosing a hub or power supply. There is no battery or battery-life estimate.

The MCU, regulator, reset network, decoupling and debugger reside on the purchased Nucleo. The passive carrier adds no active IC whose decoupling is omitted. There is no isolation, reverse-polarity protection or dedicated external-cable ESD protection. Series resistors are not safety barriers. Only disconnected voltage-free contacts are permitted. Never connect PLC outputs, industrial sensors with powered outputs, live machine switches or mains circuits. USB joins the fixture ground to the host. Cable shorts and open circuits can impersonate physical states; the firmware cannot diagnose them uniquely.

ST's UM1724 Nucleo-64 manual, table 16 in revision 14, establishes the connector map. The default USART2 bridge uses SB13/SB14 on and SB62/SB63 off; verify the actual board revision before use. RM0368 is the register reference for GPIO, RCC, USART2 and watchdog definitions. Sources: [UM1724](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf), [RM0368](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf), and the [university-hosted UM1724 mirror](https://webcms3.cse.unsw.edu.au/static/uploads/course/DESN2000/24T2/1a84764cb0b54566f849cb73193efab006c77744a5231bc55583a0df199b08d3/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf).

## Architecture and block diagram

```text
Bin A paddle -- J3 -- 1k -- SENSE0 -- PA0
Bin B paddle -- J4 -- 1k -- SENSE1 -- PA1
Bin C paddle -- J5 -- 1k -- SENSE2 -- PA4
Tray switch --- J6 -- 1k -- SENSE3 -- PB0
                              | each SENSE has 10k to 3V3
USB --> external NUCLEO-F401RE --> J1 3V3 / GND
                |
          1 ms SysTick + independent contact qualifiers
                |
       tray-delimited multiset engine, fixed 2:1:1 recipe
                |
       bounded KM1 snapshot encoder + CRC16
                |
       nonblocking USART2 --> ST-LINK --> USB serial
                |
       Linux Tk connection / evidence / recipe-export screens
```

The MCU owns all classifications. The desktop does not infer missed paddle events from snapshots transmitted at roughly ten per second. A complete frame carries current batch state, cumulative outcomes and the latest retained disposition. Missing several frames can hide intermediate batch details even though cumulative totals remain visible. There is no persistent log in flash and no claim that every completed batch can be reconstructed after late attachment. At most the latest 2000 accepted snapshots are retained by the companion.

## Firmware semantics and failure behaviour

The engine begins in WAIT. It requires all four inputs to qualify and both raw and qualified inputs to be empty before entering READY. An established empty baseline followed by tray-only assertion begins ACTIVE. A paddle held during docking, or a paddle asserted in the same qualified observation as docking, does not receive credit and cannot establish a valid start. Remove the tray and release all paddles to recover. Starting the MCU with a tray already present never invents a preceding empty baseline.

During ACTIVE, each independently qualified paddle rising edge increments that bin's count. Releasing the paddle rearms it; holding it cannot multiply counts. Independent qualification prevents chatter on one paddle from blocking another. Five matching one-millisecond observations are a sampled qualifier, not proof of five milliseconds of uninterrupted electrical stability. Meaningful manual states should remain stable at least 20 ms during commissioning. Pulses entirely between observations may be missed.

Qualified tray removal closes the batch. Exactly 2,1,1 counts without attribution taint is matching; every other completed batch is mismatching. Removal while a paddle is raw or qualified asserted taints the batch, including a short raw tray glitch that never qualifies. This rule is intentionally conservative: a paddle associated with uncertain tray presence cannot silently produce a clean result. A simultaneous final paddle and tray removal cannot establish that the declaration preceded removal. Matching classification does not imply physical contents were inspected.

The maximum active duration is 60000 acquired milliseconds. Timeout wins over completion on the same observation and abandons the batch. A skipped acquisition tick also abandons an active batch, clears qualification and requires another observed empty baseline. No interpolation is performed across the gap. The retained age on gap abandonment is acquired duration through the previous valid observation, not wall-clock time through detection. READY and WAIT can recover after fresh qualification; terminal HALT requires physical MCU reset.

Cumulative accounting preserves starts = matching + mismatching + abandoned + active. Before increments that could exhaust a counter, the engine checks available range and enters HALT rather than wrapping. An active batch is disposed as abandoned before terminal fault. Configuration or watchdog-start failure likewise creates a visible fault. Serial transmission is bounded and nonblocking; transmitter stalls increment a diagnostic rather than blocking sampling forever. The independent watchdog is fed only after acquisition progress. Clock establishment failure stops before trusting UART timing.

Power loss clears volatile counters. The receive-only protocol has no persistent boot identity, so host reset detection is heuristic: decreasing counters or backwards timestamps freeze the session, but a reboot that produces plausible forward values may be indistinguishable. For controlled experiments, reset the MCU with an empty fixture and reconnect explicitly. Do not merge runs based solely on similar counts or host timestamps. No remote command can change the recipe, reset evidence or actuate anything.

## Wire protocol and application

Each LF-terminated line is ASCII `KM1,<21 unsigned decimals>*<four uppercase hexadecimal CRC digits>`. CRC is CRC16-CCITT-FALSE, initial value 65535, polynomial 0x1021, no final XOR, over the body through the last decimal and excluding the asterisk. Decimal values are canonical, without leading zeros except zero itself. The field order is:

```text
now,state,levels,valid,starts,good,bad,aborted,gaps,faults,tx_errors,
age,tainted,a,b,c,last_a,last_b,last_c,last_result,last_age
```

State codes are WAIT=0, READY=1, ACTIVE=2 and HALT=3. Last-result codes are none=0, matching=1, mismatching=2 and abandoned=3. Fault bits are configuration=1, watchdog startup=2 and counter exhaustion=4. Counts refer to declarations, and ages are milliseconds. The protocol identifier defines the fixed recipe, qualifier and timeout policy. Changing these policies requires a protocol revision rather than silently mixing incompatible evidence.

Run `python3 kit-multiset/app/workbench.py` on Linux with Python 3 and Tk installed. The application uses only the standard library. The Connection screen opens an explicit serial path, refuses a second connection while the port is owned, and can load strict local recordings. The evidence screen shows current counts against the fixed recipe, cumulative classifications, taint, gaps and retained outcomes. The Recipe / export screen explains the policy and accepts a printable local fixture label. Its CSV exporter exclusively creates a new file with owner-only permissions; it never overwrites previous evidence.

The transport bounds input chunks and frame lengths, checks CRC and validates accounting. Replay files must be regular files no larger than 2 MiB; FIFOs and devices are rejected before reading. A partial final frame or empty replay is rejected. Live late attachment can discard a leading fragment, but corruption counts remain visible in exported metadata. Repeated identical timestamps do not refresh liveness, and contradictory duplicates freeze the session. At least two advancing snapshots are required before LIVE is shown.

Two seconds without progress makes evidence stale. A prolonged reception pause followed by queued data requires reconnect instead of renewing freshness from backlog. Cumulative host/device drift beyond two seconds also freezes capture; this deliberately conservative bound may require reconnect on long runs with an uncalibrated HSI. Disconnect retains existing evidence but marks it nonlive. Replays remain labelled replay and never become live. Exports include provenance, connection state, warning text, parser-error count and retained/received totals even when no batch completed. Free-form CSV metadata is prefixed to prevent spreadsheet formula interpretation.

## Bill of materials and cost envelope

Use four 10k and four 1k through-hole metal-film resistors, 1% tolerance, 0.25 W, DIN0207 bodies with 7.62 mm lead pitch. Yageo MFR-25FBF52-10K and MFR-25FBF52-1K are example ordering families; confirm actual supplier suffixes, dimensions and availability. Use five 1x2 and one 1x4 vertical 2.54 mm headers, such as cut lengths of a suitable Samtec TSW strip. Purchase one NUCLEO-F401RE and four D2F-01L switches separately from the carrier BOM. Wires, mating connectors, base, bins, tray, brackets and enclosure are assembly items rather than hidden PCB components.

Planning estimates in US dollars are deliberately rough, not supplier quotes: Nucleo $20 at quantity one or $17 per unit at one hundred; four switches $12 or $8; resistors and headers $4 or $2; bare carrier $6 or $2; harness $6 or $4; mechanical fixture and enclosures $20 or $12. Labour, shipping, tax, tooling and a host computer are excluded. Requote all items before procurement. A production redesign could reduce module cost but would need a genuine MCU power, clock, debug, USB and protection design rather than removing the development board from this schematic without replacement.

## Build, CAD validation and commissioning

From the repository root, run `make -C kit-multiset/firmware all test`. The target uses arm-none-eabi-gcc for Cortex-M4/Thumb, warnings as errors, a supplied startup/vector file and linker script, and no vendor SDK or downloaded dependency. The host engine, clock and serial tests use GCC with address and undefined-behaviour sanitizers. Run `xvfb-run -a python3 -m unittest discover -s kit-multiset/tests -p 'test_*.py'` for the model, real C-encoder fixture, POSIX serial tests and Tk event-loop tests over a pseudo-terminal. Synthetic contact traces are software fixtures, not hardware measurements.

The KiCad files are `kicad/device.kicad_sch`, `device.kicad_pcb` and `device.kicad_pro`. A local symbol library contains actual two-pin resistors and connectors. The PCB uses installed library footprints, 0.3 mm tracks and a routed two-layer topology. Recreate generated CAD with `/usr/bin/python3 kit-multiset/tools/generate_hardware.py` under KiCad 9. Validate with `/usr/bin/python3 kit-multiset/tools/check_hardware.py`; it runs ERC and DRC, exports a fresh schematic netlist, and compares all pins, values and footprint identifiers against the PCB and connectivity manifest. Passive ERC does not verify the external Nucleo supply or harness.

Before physical use, inspect the assembled board unpowered for solder bridges and connector orientation. Check every resistor value, continuity from J2 to the intended Nucleo pin, ground continuity and absence of a supply short. Connect only USB power initially and measure the regulated rail. Exercise each switch separately and verify logical bit polarity. Test a held paddle, a short rejected bounce, all recipe orders, tray removal before the final declaration, a cable unplug and MCU reset. Photograph and document the fixture before comparing layouts. Measure switch travel and actual acquisition timing with an independent instrument if timing matters.

Only after those steps should a supervised learner trial be considered. Compare recorded declarations to manually inspected tray contents and explicitly record false matches and false mismatches. If a learner reasonably performs several picks per paddle press, change the teaching workflow or abandon this instrument; do not reinterpret its counts as automatic part sensing. Remaining work includes fabrication review, assembled electrical testing, mechanical endurance, ESD evaluation, usability evaluation and any necessary product compliance. The current deliverable establishes a reproducible software-and-CAD prototype, not a production release or validated measurement instrument.
