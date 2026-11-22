# Sleeve Order — nested telescoping-stage sequence notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original design, documentation, firmware, application and tests are MIT licensed; see LICENSE. Startup, clock, GPIO, UART, passive carrier and serial framing infrastructure reuse jayis1's Aisle Pair platform at commit 3b19f2a. The trajectory interpretation, SV1 contract and workbench are specific to Sleeve Order. Installed KiCad footprint geometry retains its upstream licensing. The import script records provenance and refuses overwrites; it is not a normal build step.

Revision A is an unbuilt engineering prototype, not a validated product. The custom PCB is a passive interface to a separately purchased NUCLEO-F401RE. It does not include the processor, USB bridge, regulator or debugger. Compilation, host simulation, desktop workflows and CAD checks have been exercised; fabrication, programming, electrical measurements, mechanical calibration and user benefit have not. This device cannot prevent movement and must never serve as a protective interlock, machinery controller or certification instrument.

## Purpose and practical utility

A two-stage telescoping demonstrator can extend in a particular order: first the intermediate sleeve moves relative to the fixed base, then the inner slider moves relative to that sleeve. Reinsertion may require the opposite order. A worn sequencing tab, incorrectly adjusted stop or operator shortcut can change that sequence without changing the final fully closed position. Counting closed-to-closed cycles therefore hides the very behavior a designer wants to investigate.

Sleeve Order records whether each observed outer excursion followed a specified nested endpoint trajectory. Four switches identify outer home, outer far, inner home and inner far. It distinguishes an outer-only cycle from a cycle containing one or more complete inner round trips, and retains separate counts for abandoned cycles, ordering diagnostics, sampling gaps and timeouts. The operator sees the current trajectory phase, working age, last result and longest completed cycle while adjusting an unloaded tabletop model.

The target audience is a maker developing a small nested enclosure, an educator demonstrating sequencing mechanisms, or a repairer examining a detached low-energy slide sample. The instrument answers a narrow question: did the contacts witness outer extension before inner travel, inner return before outer retraction, and the required clear regions between opposing endpoints? It does not measure extension distance, force, friction, load capacity, alignment or physical wear. An accepted cycle is an accepted observation sequence, not a declaration that the mechanism is safe or good.

One useful experiment compares two removable sequencing-tab geometries. Use the same captive fixture, switch positions, unloaded parts and prescribed hand motion for both. Collect an equal number of deliberate outer excursions, export each session, and retain the aborted outcomes rather than removing inconvenient trials. Compare nested completions and observed order diagnostics. Repeat with independent video or a written reference to determine whether contact placement is hiding motion. Differences motivate further physical testing; the counts alone do not establish causation or reliability.

## Originality and repository comparison

The repository directories and README descriptions were inspected, including a search for telescoping, inner/outer and nested-slide concepts. The closest existing designs are Aisle Pair, which records continuous occupancy of two independent opposing drawers, and Latch Reach, which relates opening checkpoints to a later latch indication. Neither implements the ordered nesting contract used here. Reusing their passive electrical interface is intentional. A different resistor arrangement would not make the experiment more useful.

Sequenced telescopic slides are established prior art, not an invention claimed by this project. A bounded search found Accuride's patent US5551775, “Telescopic drawer slide with mechanical sequencing latch,” describing intermediate and distal members whose motion is mechanically sequenced. See https://www.freepatentsonline.com/5551775.html . Commercial slide installation guidance also discusses aligned end positions and correct mounting; see https://elesa-ganter.be/image/pdf/en/Mounting_Information_Telescopic_Slides.pdf . Those sources demonstrate why motion sequence can matter, but they do not validate this instrument.

The proposed contribution is a compact, local, receive-only experimental notebook with explicit observed-path rules, repeated inner-trip accounting and conservative rejection of insufficient evidence. No worldwide novelty, commercial absence, patentability or freedom-to-operate claim is made. A camera, displacement sensors or a proper mechanical test stand may be better when continuous motion or calibrated loads are required. Practical value remains a hypothesis until the physical prototype is compared with those alternatives.

## Hardware specifications and boundaries

| Item | Revision A |
|---|---|
| Processor assembly | ST NUCLEO-F401RE, MB1136 family |
| MCU | STM32F401RET6, Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz |
| Observation cadence | Nominal 1 ms, full-vector qualification over five unchanged intervals |
| Sensors | Four disconnected voltage-free endpoint contacts |
| Carrier | 80 by 70 mm, two copper layers, 1.6 mm FR-4 |
| Power | USB-powered Nucleo; carrier takes its regulated 3.3 V |
| Host connection | USART2 TX through ST-LINK virtual COM, 115200 baud, 8N1 |
| Programming | Onboard ST-LINK SWD; OpenOCD flash target |
| Companion | Linux/POSIX desktop Python 3 with Tk, standard library only |
| Outputs | Telemetry only; no actuators, relays or permissive signal |

The board contains fourteen components: six headers and eight axial resistors. Each input has a 10 kohm pullup to 3.3 V and a 1 kohm series resistor to the field switch. Closing the switch connects the field node to ground. At nominal values the current is 0.300 mA and the MCU sensing voltage is 0.300 V. Four simultaneously closed paths would draw 1.20 mA and dissipate 3.96 mW in the resistor networks. These are calculations, not measurements; valid mechanics should not close opposing endpoints simultaneously. Development-board consumption dominates and must be measured separately. Use 150 mA at USB 5 V only as an unmeasured initial planning allowance, not a guaranteed operating specification.

The MCU, bypass capacitors, voltage regulation, reset circuit and USB circuitry remain on the purchased board. The carrier has no regulator, isolation, reverse-polarity protection, battery or charger. Its series resistors are not cable-surge protection or a safety barrier. Use only short paired signal/ground leads on a dry supervised bench. Do not attach powered sensors, PLC outputs, live machine switches, solenoids or mains wiring. USB ground is common with the contacts. An open cable looks like an open contact and a short looks like a closed contact; software cannot distinguish those faults reliably.

### Connector map and bill of materials

| Logical weight | Meaning | Field header pin 1 | J2 pin | Nucleo header | MCU pin |
|---|---|---|---|---|---|
| 1 | Outer home | J3 | 1 | CN8.1 / A0 | PA0 |
| 2 | Outer far | J4 | 2 | CN8.2 / A1 | PA1 |
| 4 | Inner home | J5 | 3 | CN8.3 / A2 | PA4 |
| 8 | Inner far | J6 | 4 | CN8.4 / A3 | PB0 |

Every field header pin 2 is GND. J1.1 connects to CN6.4, regulated 3V3; J1.2 connects to CN6.6, GND. Never substitute the adjacent 5 V pin. Confirm pin-one orientation by continuity measurement, not by guessing the view of the mating connector. J2 is four sensing signals, not a powered peripheral bus.

Populate R1, R3, R5 and R7 with 10 kohm one-percent resistors; R2, R4, R6 and R8 are 1 kohm one-percent. The footprints are DIN0207 axial bodies, 7.62 mm lead pitch; use at least 0.125 W parts. J1 and J3 through J6 are two-pin 2.54 mm headers, while J2 is four-pin. The PCB uses 0.30 mm tracks, a nominal 0.20 mm design clearance and a through via with 0.80 mm diameter and 0.40 mm drill. The supplied native project contains real footprints, pads, routed copper and an outline, not a decorative diagram.

External items include one Nucleo, a USB data cable, four low-force dry-contact switches, insulated connectors and harnesses, a fixed base, captive outer carriage, captive inner carriage, adjustable cams, independent stops and an insulating enclosure. An Omron D2F-01L low-level switch family is a candidate, not a qualified procurement choice. Confirm the exact purchased suffix, low-current switching suitability, operating travel and force. Contact wetting requirements and switch-induced friction are physical acceptance gates. The carrier has no mounting holes; use insulating edge rails rather than drilling through routed copper.

## Architecture and block diagram

```text
Fixed base <---- outer carriage <---- inner carriage
   |              |                    |
outer HOME/FAR    inner HOME/FAR referenced to outer carriage
   |              |
four voltage-free switches, each pin 2 -> GND
   |
J3..J6 pin 1 -> 1k -> SENSE0..3 -> PA0 / PA1 / PA4 / PB0
                         |
                       10k each -> Nucleo 3V3
                         |
                 nominal 1 ms acquisition
                         |
        whole-vector qualifier + raw movement guards
                         |
      ordered nested-trajectory engine + counters
                         |
           bounded SV1 snapshot + CRC-16
                         |
 USART2 PA2 AF7 -> ST-LINK USB VCP -> Linux Tk workbench
                               status / history / replay / CSV
```

The inner switches must measure inner position relative to the moving outer carriage, not relative to the bench. Otherwise outer movement can imitate an inner stroke. Route their cables so loops cannot pull the carriage or become pinched. The instrument records endpoint contact states; it never reconstructs an unobserved continuous position from the apparent geometry.

A proposed fixture uses two captive low-mass stages with approximately 80 mm travel each. Provide disjoint home and far contact windows, with a clear middle region on each stage. Start with about 5 mm of endpoint cam engagement and slow hand motion, holding each observed region for at least 20 ms during commissioning. These dimensions are adjustable construction guidance, not released tolerances. Independent mechanical stops must absorb travel limits; switch levers are not stops. Keep fingers outside pinch points and use unloaded components without stored spring energy. No enclosure CAD or physically validated mechanical assembly is supplied.

## Firmware contract and design decisions

The firmware is freestanding C11 with an explicit vector table, data/BSS initialization, linker script, clock restoration, GPIO setup, watchdog control and nonblocking UART transmitter. The linker targets flash at 0x08000000 and SRAM at 0x20000000. Register addresses follow STM32F401 RM0368. No vendor SDK download or guessed library dependency is required. Only SysTick interrupts are enabled; all trajectory and transport decisions occur in the foreground.

Clock restoration uses bounded iteration waits before telemetry is initialized. Failure stops the program before a possibly incorrect baud rate can be treated as evidence. GPIO setup disables internal pulls because the external networks establish bias. PA2 uses USART2 AF7, and no receive path is enabled. Watchdog startup uses bounded millisecond waits; failure places the engine in HALT. The watchdog is fed after observed tick progress, including terminal diagnostic operation. Reset causes are not retained in a durable boot identifier, so a watchdog reset starts fresh volatile accounting rather than reconstructing an interrupted experiment.

The main loop observes each change of the millisecond tick once. It does not fabricate missing samples by replaying the current input across elapsed time. Each sample updates the full four-bit vector. A changed vector seeds a candidate; five subsequent identical one-millisecond intervals qualify it. Thus a newly changed candidate requires six acquired observations including its first endpoint. Whole-vector qualification intentionally refuses to assemble independently delayed endpoints into an apparent mechanical sequence. Chatter can postpone qualification, and that limitation must be considered when interpreting missed endpoints.

The phase sequence is WAIT, READY, OUTWARD, EXTENDED, INNER_OUT, INNER_FAR, INNER_BACK, OUTER_BACK and HALT, encoded zero through eight. WAIT enters READY only at qualified and raw mask 5, meaning both stages home. Outer departure must first produce mask 4, with the inner stage still home. Outer far is mask 6. Reaching far directly from both-home without a separately witnessed middle region is not an accepted traverse.

At EXTENDED, the operator may retract the outer stage without an inner trip, or may move the inner stage. An inner round trip is 6 -> 2 -> 10 -> 2 -> 6, representing inner departure, inner far, inner return through the clear region, and inner home. Every completed inner round trip increments its lifetime count immediately. Multiple such trips may occur while the outer stage remains extended. Consequently inner trips can be retained even if that enclosing outer cycle is later aborted; they are not a count limited to successfully completed outer cycles.

Outer retraction is 6 -> 4 -> 5. A completed outer cycle increments completed, and increments nested if at least one inner round trip occurred in it. Otherwise it is classified outer-only. Returning before reaching the prescribed far endpoint is a rejected sequence, not a partial success. Simultaneous qualified changes cannot supply missing intermediate states. The observer's contract is deliberately stricter than every mechanically possible way of moving two sliders.

During outer travel the raw inner pair must remain exactly home. During inner travel the raw outer pair must remain exactly far. These guards operate before qualification, so even a one-sample observed loss rejects authorization rather than being hidden by debounce. At other phases, endpoint changes are judged through the qualified vector. Firmware does not claim to detect electrical pulses between samples. Both endpoints of one stage qualifying together are an overlap diagnostic. Diagnostics can occur while no cycle is active, so their total need not equal aborted cycles.

Cycle age starts at zero on qualified outer departure. Completion must occur strictly before 60000 acquired milliseconds. The rejecting deadline wins over an otherwise simultaneous completion. A skipped or repeated acquisition timestamp abandons an active cycle, increments gaps, clears qualification validity and returns to WAIT. A fresh both-home qualification can rearm; previous counts remain. Age on gap abandonment is the retained acquired age, not an estimate of motion during missing wall time. Normal unsigned timestamp wrap is supported.

Counter increments are checked on a private engine copy. If any required update would overflow, the entire speculative sample is discarded and the prior active cycle is abandoned once under terminal overflow fault. Fault bits are configuration 1, watchdog 2 and overflow 4. HALT freezes trajectory accounting while timestamps and transmission diagnostics can still advance. All evidence is volatile and lost on reset. UART snapshots are attempted every 100 ms and transmission is abandoned after a 100 ms stall; recovery never blocks input acquisition waiting for a host.

## SV1 protocol and application

Each frame contains ASCII `SV1`, then 23 comma-separated canonical unsigned decimal integers, an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE covers every byte before the asterisk, with polynomial 0x1021, initial value 0xFFFF and no final XOR. The standard check string `123456789` produces 0x29B1. The frame writer is capacity checked against a 768-byte buffer. CRC detects accidental corruption, not hostile modification or truthful sensing.

Field order is: sequence, now, phase, fault, raw, qualified, valid, starts, completed, nested, aborted, inner_trips, violations, timeouts, gaps, tx_errors, age, maximum, last, last_age, cycle_trips, qualify, limit. Sequence counts acquisition calls committed before HALT, not UART packets. Raw and qualified use the bit weights in the wiring table. Valid is a whole-vector flag. Last result values are none 0, outer-only 1, nested 2, order rejection 3, endpoint overlap 4, gap 5, timeout 6 and terminal fault 7. Durations are nominal acquired milliseconds, not calibrated time.

The companion checks CRC, framing, field count, canonical decimals, ranges, phase topology, fixed policy and conservation: starts equals completed plus aborted plus an active-cycle indicator. It also checks cumulative-counter rollback, contradictory repeated timestamps, terminal evidence freezing, sticky faults, plausible event growth and duration bounds. Sparse snapshots are permitted. These checks are necessary consistency conditions, not proof that every unreported motion occurred or a reconstruction of all hidden input samples.

Launch `python3 app/workbench.py`. The trajectory screen displays current phase, totals, masks, last outcome and durations. The history screen contains every retained accepted snapshot up to a 2000-row bound. Setup and limits describes wiring and the fixed movement policy. The port entry is local host configuration; acquisition policy cannot be changed by a command. This avoids remote resets and partial configuration writes entirely. Change firmware deliberately and rebuild if a different experiment is needed; a changed policy requires compatible protocol validation.

Connect opens the selected POSIX serial port at 115200 8N1, saves terminal attributes and flushes pending input. Only successful opening replaces the previous session. A second simultaneous connection is refused. Disconnect restores attributes and retains evidence. Hangup is detected with poll flags as well as read errors. A partially received leading frame is discarded and counted; live decoding can resynchronize at the next full SV1 marker. Replay is strict, regular-file-only, capped at 2 MiB, and rejects empty files and partial tails. Replay is never labeled live hardware.

Freshness requires advancing acquisition, a recent advance and a connected live source without a terminal fault or latched inconsistency. Cumulative device/host drift is bounded to two seconds, and a long reception pause blocks continuation. Duplicate records do not renew progress. Nevertheless there is no persistent boot identity, authentication or clock synchronization: some resets and preexisting transport delays remain indistinguishable from plausible forward traffic. A long session with uncalibrated oscillator drift can conservatively require reconnecting. Never use the live label as proof of uninterrupted physical observation.

CSV export creates a new mode-0600 file and refuses to overwrite an existing path. Metadata records jayis1, source, connection, freshness, parser errors, blocking reason and eviction count, even when there are no observations. Disconnecting does not relabel previously live evidence as replay. Keep physical setup notes and specimen identity separately; the device does not identify the mechanism or the operator. There is no cloud service or network dependency.

## Build and verification

Required tools are GNU Make, host GCC, GNU Arm Embedded GCC and binutils, Python 3 with Tk, and KiCad 9 CLI plus pcbnew bindings. On this host the bindings are available through `/usr/bin/python3`. No Python package dependencies are downloaded. From this directory:

```sh
make -C firmware clean all test fixture
xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 app/workbench.py
```

The fixture target emits synthetic observations using the real C engine and encoder into `firmware/build/demo.sv1`. It exercises a nested completion, an outer-only completion and an aborted inner movement. It is not captured hardware data. Host C tests use address and undefined-behavior sanitizers; clock and serial tests use modeled peripheral registers. Desktop tests invoke actual Tk callbacks with Linux pseudo-terminals, including connection, reconnection, hangup, replay, screen selection and private export. They do not prove operation of physical USB hardware.

The CAD checker runs native ERC and DRC, exports a fresh schematic XML netlist, and reconciles every reference, value, footprint and numbered pin/net against actual PCB pads and the connectivity manifest. Passive-only ERC does not verify external power, development-board wiring or mechanical safety. The schematic has also been rendered for readability inspection. See VALIDATION.md for the executed outcomes and explicit remaining gates.

To regenerate authored CAD, run `/usr/bin/python3 tools/generate_hardware.py` and repeat the checks. Do not run the historical import script over this completed project. `make -C firmware flash` invokes OpenOCD against a deliberately connected ST-LINK target; it was not run during software validation. Verify debugger identity and disconnect unrelated targets before physical programming.

## Physical bring-up and remaining release gates

Inspect resistor values, solder joints, copper clearance and connector polarity unpowered. Check for shorts and compare continuity with the schematic. Power and identify the Nucleo alone, then attach the carrier with all switches disconnected. Measure 3.3 V and each sense node. Close each loose switch separately and confirm exactly the expected raw bit. Confirm input-low margin, serial baud reliability, watchdog operation and current consumption on the actual board revision.

Only then mount the switches on the captive fixture. Verify inner sensing moves with the outer reference. Compare the full mask path with independent observation, including deliberate premature inner departure, outer motion during inner travel, skipped endpoints, broken leads and overlapping cams. Test near the rejecting deadline and examine switch bounce at practical hand speeds. Remove the followers and compare mechanical motion to detect added drag. Export every session before resetting or changing the arrangement.

Manufacturing, physical timing, contact lifetime, calibration, enclosure protection, cable fatigue, ESD, EMC, temperature behavior and practical usefulness remain unverified. No production release, load rating or regulatory compliance is claimed. The correct next step is a supervised low-energy bench prototype, not installation on a real machine or safety-related slide.

Manufacturer references: ST UM1724 Rev 17, Table 16 and the MB1136 virtual-COM solder-bridge guidance, https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf ; STM32F401 RM0368, https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf . Confirm the actual board revision and VCP bridge arrangement before wiring. The documented connector mapping was checked against the manufacturer manual; physical board correspondence remains a bring-up check.
