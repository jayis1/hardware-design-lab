# Pallet Census — alternating-lock arrival notebook for slow escapement demonstrators

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion application, documentation and locally authored electrical designs are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering design. The supplied PCB is a fully routed passive input carrier for a separately purchased NUCLEO-F401RE. The processor, regulator, debugger, reset network, USB bridge and MCU decoupling are on that purchased board, not this carrier. Target linking, synthetic acquisition, desktop workflows and electrical CAD checks are recorded separately in VALIDATION.md. No fabricated board, programmed target, assembled mechanism or physical measurement is claimed.

## Purpose, usefulness and intended user

Pallet Census helps a clock-mechanism educator or automaton builder compare the behavior of two alternating stop fingers on a slow, hand-driven demonstration wheel. Its specific question is: when the wheel reaches a repeatable index window, which pallet-position contact is already asserted, and does that attributed side alternate from the preceding attributable arrival? It preserves repeated-side arrivals, arrivals with neither or both contacts, and arrivals whose electrical ordering is unresolved. It does not collapse all of them into a single wheel count.

A typical experiment compares two removable pallet profiles on a large tabletop escapement model. The builder fits one magnetic contact to each pallet's nominal catching position, fits a repeatable index contact to the wheel, and holds a separate RUN switch while moving the mechanism slowly by hand. A local desktop view shows A and B arrivals, their directed adjacency counts AA, AB, BA and BB, and the three nonattributed categories. After stopping with the index contact open, the builder releases RUN and exports the observation before changing a profile.

The instrument is useful when the operator can see that the wheel moved but cannot remember which contact indication accompanied each arrival. For example, a profile may produce a repeated A indication after adjustment, or an index position may lie between the two contact zones and consequently produce many neither observations. Those are prompts to inspect geometry and sensor alignment. They are not automatic diagnoses of double escape, tooth skipping, poor efficiency or clock rate. The independent reference is annotated video and electrical probing, not the assumption that the model must alternate.

Target users are small museum workshops, clockmaking classes, mechanical-computing clubs and makers of low-energy demonstration mechanisms. The device neither controls a clock nor regulates an escapement. It has no motor, relay, solenoid, protective interlock, alarm output or radio. There is no user-account service, remote database or cloud dependency. It is deliberately a local bench notebook that makes its limited measurement definition inspectable.

## Originality and scope comparison

The repository's top-level directories and device README titles were inventoried before choosing this concept. The closest contracts were then read: Ratchet Audit measures forward and reverse encoder movement inside handle strokes, including rollback while a pawl indication is present; Beat Aperture partitions comparison-pendulum returns into reference windows; Merge Rhythm infers source ordering through a shared FIFO. Pallet Census does none of those. It samples one wheel-arrival strobe against two independent pallet-position indications and retains the directed adjacency of only unambiguous side attributions.

A generic logic analyzer could reproduce this analysis with custom software. Escapements, contact instrumentation and adjacency matrices are established techniques. The proposed contribution is the combination of a removable low-energy fixture, conservative endpoint attribution, explicit chain breaks, bounded RUN trials and a local evidence interface. It is an original implementation and application within this inventory, not a claim of worldwide novelty, patentability or freedom to operate. No exhaustive product or patent search has been performed.

A conventional clock timegrapher was rejected as the design scope: acoustic timing at normal escapement speeds would require a different sensor and timing architecture. A powered adjustable escapement test stand was also rejected because it would add actuation and stored-energy hazards. This revision is for deliberately slowed, manually driven mechanisms, not working watches or operating clock movements.

## System architecture and hardware specification

The external target is STM32F401RET6 on the ST NUCLEO-F401RE MB1136 assembly: Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware restores the internal 16 MHz HSI clock tree and acquires four digital inputs at nominal one-millisecond intervals. USART2 TX on PA2, alternate function 7, sends ASCII snapshots through the board's ST-LINK virtual COM bridge at 115200 baud, eight data bits, no parity and one stop bit. UART reception is disabled. A Linux host runs a Python standard-library Tk companion.

The custom carrier is 80 by 70 mm, two copper layers and nominal 1.6 mm FR-4. It has six through-hole headers, four 10 kilohm pullups and four 1 kilohm series resistors. Tracks are 0.3 mm wide with a 0.2 mm default clearance. Resistors use axial DIN0207, 7.62 mm lead-spacing footprints; connectors use 2.54 mm vertical pin headers. The generated schematic has real passive symbols and labeled nets, and the PCB has placed footprints, tracks, vias and a closed outline. It is not a pin-compatible Nucleo shield: connect it with short insulated wires.

```text
Pallet A position -- dry contact -- J3 -- 1k -- SENSE0 -- PA0
Pallet B position -- dry contact -- J4 -- 1k -- SENSE1 -- PA1
Wheel index zone -- dry contact -- J5 -- 1k -- SENSE2 -- PA4
Operator RUN     -- dry contact -- J6 -- 1k -- SENSE3 -- PB0
                                             |
                                     each 10k to 3V3
                                             |
USB host <--- ST-LINK VCP <--- USART2 TX <--- NUCLEO-F401RE
    |                                        |
 PC1 decoder                         independent qualification
    |                                        |
 local evidence screens <--- CRC frames <--- arrival / adjacency engine
    |
 private CSV export with provenance and errors
```

### Connector contract

| Connector | Wiring | Meaning |
|---|---|---|
| J1.1 | Nucleo CN6.4 | Regulated 3.3 V supply, net VCC |
| J1.2 | Nucleo CN6.6 | Common ground |
| J2.1 | CN8.1 / A0 / PA0 | SENSE0, PALLET_A, bit 0 |
| J2.2 | CN8.2 / A1 / PA1 | SENSE1, PALLET_B, bit 1 |
| J2.3 | CN8.3 / A2 / PA4 | SENSE2, INDEX, bit 2 |
| J2.4 | CN8.4 / A3 / PB0 | SENSE3, RUN, bit 3 |
| J3 through J6, pin 1 | Corresponding external contact | PALLET_A, PALLET_B, INDEX, RUN |
| J3 through J6, pin 2 | Other end of corresponding contact | GND |

A closed contact is electrically low and becomes logical one in firmware. All sensor connections must be disconnected, voltage-free contacts. Never connect energized machine signals, external pullups, 5 V outputs, mains wiring or a vehicle circuit. This board provides neither galvanic isolation nor qualified surge, ESD or hot-plug protection. USB connects its ground to the computer. A series resistor is not permission to apply an unsafe voltage.

At nominal 3.3 V, a closed contact draws approximately 0.30 mA through the combined 11 kilohm path. Four simultaneous closures draw approximately 1.20 mA; the ideal MCU-side closed voltage is approximately 0.30 V. Those are nominal circuit calculations, not measured results or guaranteed logic margins. Reserve 100 mA at USB 5 V as a system planning allowance and measure the actual purchased Nucleo configuration. The design has no battery, charger or power switching. Supply J1 only from the Nucleo's regulated output; do not back-feed it from another supply.

### Bill of materials and mechanical arrangement

The electrical BOM is one NUCLEO-F401RE, one carrier PCB, four 10k and four 1k axial resistors of 1% tolerance and at least 0.125 W rating, five two-pin headers, one four-pin header and matching insulated leads. External sensing uses three Littelfuse MDSR-7-10-15 normally open reed switches with captured small magnets, plus one normally open hand-held RUN switch suitable for low-current logic. Manufacturer ratings and actual magnetic operating distances must be checked for the purchased parts. No current stock or supplier quotation is asserted.

Plan for approximately USD 80 excluding labor, shipping and the mechanism, rather than treating that allowance as a market price. The Nucleo is a replaceable external assembly; the carrier does not duplicate its power or processor circuits. There are no mounting holes in the carrier. Use insulating edge supports inside an approximately 160 by 110 by 45 mm covered electronics box, with strain relief and visible connector labels. These enclosure dimensions are packaging allowances, not a supplied manufacturing drawing.

For a first experiment, use a guarded wheel at least 120 mm in diameter on a horizontal bench, turned directly by hand without a hanging weight, mainspring or motor. Attach the index magnet or magnets to an auxiliary disk rigidly coupled to the wheel. A single magnet yields one event per revolution; multiple equally spaced, retained magnets can provide a window at each selected tooth station. Firmware reports arrival events, not inferred tooth counts, so record the actual magnet count and mechanical indexing in the experiment notes.

Each pallet magnet should follow the pallet itself, not its driving lever. Mount its reed on an adjustable stationary nonconductive bracket and set the closure zone around the intended catch position. Place the wheel-index sensor so its closure occurs after the chosen pallet contact has become stable. If an arrival always precedes contact stability, reposition the index window or accept that the experiment measures uncertain/neither outcomes. Do not change software to hide that mismatch. Keep magnets far enough apart to avoid cross-triggering and verify all channel combinations manually. Glass reeds require encapsulation and lead strain relief; retained magnets must not become loose projectiles or accessible small parts.

Mechanical mounting is fixture-specific and remains unvalidated. Sensor drag, magnetic attraction and coupling compliance may change the demonstration. Compare instrumented and uninstrumented operation under video. Stop if the instrumentation changes the motion being studied. No generic mechanical CAD can establish correct positioning on every escapement, so this package specifies adjustment and acceptance checks rather than an allegedly universal attachment.

## Firmware measurement contract

Every input qualifies independently after five consecutive identical raw samples. The nominal sample interval is one millisecond. Shorter pulses may be rejected, and several physical movements can alias into one held contact. Begin with at least twenty milliseconds at each stable index-high and index-low level and similarly slow pallet changes. That is an operating recommendation, not an overspeed detector or a physical bandwidth certification.

At boot the engine is WAIT. It must observe all four channels qualified, followed by fifty intervals with INDEX and RUN low at both raw and qualified endpoints, before entering READY. Pallet positions need not be both low during this baseline. A held RUN at startup never starts acquisition. Index activity while READY invalidates the baseline. A qualified RUN rising edge from READY starts ACTIVE, clears the latest-run measurements and increments starts. If INDEX is already raw or qualified high at that boundary, the run immediately aborts rather than inventing a first arrival.

While ACTIVE, each qualified INDEX rising edge is an arrival. Compare the current qualified pallet pair with the previous qualified pair and the current raw pair. If either comparison disagrees, increment uncertain and clear the adjacency anchor. Otherwise classify pair 01 as A, 10 as B, 00 as neither and 11 as both. Neither and both also clear the anchor. This is an endpoint rule: it does not assert that the contacts were electrically stable throughout the index qualification window. Subsample changes remain invisible.

An A or B arrival increments its side counter and, if an earlier attributable arrival remains anchored, the corresponding directed pair counter. An A followed by B increments AB; B followed by A increments BA; repeated A increments AA; repeated B increments BB. The new side becomes the anchor. Pallet movement between arrivals does not itself break this sequence, but any nonattributed arrival does. For example, A, B, neither, A produces AB=1 and BA=0: the uncertain physical interval is not bridged by a fabricated pair.

RUN must remain raw high on an accepted index-rise endpoint. A raw RUN release on that endpoint aborts with a boundary fault, even if qualified RUN remains high. Normal completion occurs on a qualified RUN falling edge with INDEX both raw and qualified low. An index high at completion also aborts. Stop the wheel outside the index window, leave INDEX low, then release RUN. Completed results remain frozen through WAIT and READY and clear only at a new start. The companion is not a nonvolatile logger: export before resetting or replacing a run.

The trial deadline is 60000 acquired milliseconds. Equality rejects before processing a simultaneous completion or arrival. Any skipped or repeated timestamp after the first sample halts correlation immediately; it does not pretend the missing interval was continuously observed. HALT is terminal until reset. starts always equals completed plus aborted plus one if ACTIVE. Every accepted arrival belongs to exactly one of A, B, neither, both or uncertain. An abort preserves partial run counts, rather than pretending that an incomplete run completed successfully.

Sequence exhaustion halts before wrap. Lifetime run counters are bounded at one million; the bounded run duration also limits arrival counts well below that range. Transport timeouts saturate a separate diagnostic and do not fabricate acquisition gaps. The serial driver transmits nonblockingly, applies a 100 ms timeout and copies bounded frames. The independent watchdog is fed after sampling. Clock restoration uses bounded polling before SysTick exists; a clock failure stops before serial startup. Configuration and watchdog initialization failures produce HALT evidence if serial initialization is available. None of this is a safety-control guarantee.

## Protocol and application interface

PC1 fixes both the wire layout and qualification policy. Incompatible changes require a new marker. Every approximately 100 ms, the device sends the marker followed by these comma-separated unsigned decimal fields:

`sequence,now,state,fault,raw,qualified,valid,starts,completed,aborted,age,baseline,pulses,a,b,neither,both,uncertain,aa,ab,ba,bb,anchor,tx_errors,gaps`

The suffix is an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xffff, with no reflection or final XOR, covering the marker and fields before the asterisk. now is modular milliseconds; sequence counts acquired samples and never wraps silently. States are WAIT=0, READY=1, ACTIVE=2, HALT=3. Fault values are configuration=1, watchdog=2, gap=4, deadline=8, boundary=16 and counter=32. Only the first terminal fault is retained. Anchor is none=0, A=1 or B=2. All run counters are counts, not durations; age and baseline use acquired millisecond intervals.

The companion has three real screens. Pallet evidence shows the current state, counters, contact bits and errors. Trace displays all retained snapshots, up to a 512-row bound. Wiring and limits provides the exact connector mapping and policy. The connection bar accepts a Linux serial path, disconnects without deleting evidence, replays a PC1 recording and exports CSV. Connection failures preserve the previous session. A second connection or replay replacement while serial is owned is rejected. The host never sends a command to the MCU.

The parser bounds frame and chunk lengths, checks CRC and decimal syntax, validates conservation and timing relationships, and resynchronizes on the full PC1 marker after truncated input. Replay requires a regular file no larger than two MiB, at least one accepted snapshot and a clean frame boundary at EOF. CSV export creates a new private file with mode 0600 and refuses overwrite. Even an empty session exports author, provenance, connection state, freshness, parser-error count and warning text. Disconnection preserves whether evidence originated live or from replay.

Freshness requires a connected live session and advancing acquisition, not repeated reception of identical frames. A contradictory duplicate, time regression, counter rollback or changed halted evidence blocks the session. Cumulative host/device progress must remain within two seconds; this fixed budget exposes backlog but can conservatively reject long recordings because HSI is not calibrated. Export and reconnect rather than silently extending the allowance. Without a persistent boot identifier, some resets remain indistinguishable from forward traffic. CRC is not authentication, and necessary sparse-snapshot bounds are not proof of the entire hidden input history.

## Build, operate and verify

Use GNU Arm Embedded GCC, host GCC, make, Python 3 with Tk, KiCad 9 CLI and its Python pcbnew bindings. No Python package installation is needed for the application. On Linux, install the distribution's Python Tk and KiCad packages if absent. Use a permitted serial-device group rather than running the companion as root. From this folder, run:

```sh
make -C firmware clean all test fixture
xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py'
/usr/bin/python3 tools/check_hardware.py
/usr/bin/python3 tools/verify.py
/usr/bin/python3 app/workbench.py
```

The fixture recording at firmware/build/demo.pc1 is synthetic data from the actual C encoder. It is not a measurement. Replay it to exercise a completed run with A/B arrivals, one neither event and one uncertain event. Firmware linking produces an ELF and binary; the Makefile's flash target uses OpenOCD with ST-LINK and STM32F4 configuration, verifies the write and resets. That target has not been exercised against physical hardware during this design run.

Before assembly, inspect Nucleo manual UM1724 for the actual board revision, connector pinout, power selection and ST-LINK virtual COM solder bridges. Review STM32F401 reference manual RM0368 for register definitions and clock behavior, the STM32F401RE datasheet for GPIO limits, and the MDSR-7 manufacturer datasheet for contact and magnetic constraints. These are the governing component references, not substitutes for assembled measurements. Preserve manufacturer and library notices.

Bring up the carrier without sensors first. With power removed, inspect resistor values and continuity and verify no VCC-to-ground short. After controlled power-up, measure supply and each input's open/closed voltage using a temporary isolated contact. Compare every raw bit with the correct connector before adding magnets. Check oscilloscope timing and bounce, contact wetting reliability, USB ground effects and sensor cross-triggering. Then validate intentionally repeated A arrivals, alternating arrivals, neither/both positions, tied changes, held-RUN boot, active disconnect and deadline rejection against recorded electrical signals.

Software tests enumerate every six-event word over the four stable pallet codes against an independent count/last-letter oracle. Additional cases cover raw endpoint loss, tied qualification, cross-channel chatter, time wrap, gaps, exact deadline precedence, sequence exhaustion, bounded encoding and impossible telemetry. Python tests exercise CRC failures, replay restrictions, frozen evidence, private exports, terminal restoration and the actual Tk loop over pseudo-terminals using C-generated frames. CAD checks run ERC and DRC and compare every schematic and PCB pin, component value and footprint identity against the connectivity manifest.

Remaining acceptance work is physical: fabrication review, soldering, target programming, power and logic-margin measurement, acquisition jitter, magnetic fixture calibration, video correlation, enclosure construction and user-value testing. EMC, ESD, environmental qualification and regulatory compliance are unverified. Never use this prototype with powered machinery, lifting systems, protective devices, medical equipment or a clock whose stored energy could injure someone. A successful software test establishes only the behavior tested; it does not turn contact indications into verified mechanical engagement.
