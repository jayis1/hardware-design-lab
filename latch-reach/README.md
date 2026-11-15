# Latch Reach — opening-checkpoint history versus return-latch notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original design, documentation, firmware and application are MIT licensed; see LICENSE. The acquisition drivers, startup, linker, serial transport and passive carrier generation infrastructure reuse jayis1's Float Band platform at repository commit a52abd2. The capture engine, LR1 protocol, application interpretation, experimental procedure and tests are specific to Latch Reach. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype, not a tested product. The custom PCB is a passive interface for a separately purchased NUCLEO-F401RE. It does not contain the MCU, regulator, debugger or USB bridge. Target compilation, host tests, desktop workflows and native CAD checks are reproducible. No board has been fabricated, target flashed, fixture calibrated, latch characterized or mechanical usefulness demonstrated. Do not use this design to certify a latch, secure a door, protect people or control machinery.

## Purpose and practical utility

A small push-to-open or spring-reset cabinet catch can behave differently after a shallow opening and after a large opening. Its return spring, release mechanism or floating striker may not reset until the moving part passes a particular region. A repairer closing the same demonstration panel repeatedly can accidentally alternate between shallow and deep excursions, making an intermittent engagement problem look random. A simple closure counter does not retain the opening history associated with the subsequent latch indication. A force gauge may answer another important question, but it does not automatically organize that history either.

Latch Reach is a removable bench notebook for a detached, unloaded latch mounted on a hand-operated slide. Three position contacts indicate HOME and two disjoint opening checkpoints, NEAR and FAR. A fourth contact indicates the latch bolt or catch position independently of slide position. Each qualified departure from HOME begins a trip. The device retains whether NEAR, FAR, both, or neither was actually witnessed before returning HOME. It then observes the latch indication during a short fixed return window and places one count into a checkpoint-history by latch-outcome matrix.

The resulting matrix has four rows and three columns. Rows are no checkpoint witnessed, NEAR only, FAR only, and NEAR plus FAR. Columns are latch open throughout the return window, latch closed throughout the return window, and latch disturbed during that window. Rejected trips are reported separately. A repairer can compare shallow excursions with excursions that visited both checkpoints, adjust a strike or spring, reset the experiment, and collect another session without manually pairing every opening with its closure result.

These are contact observations, not measurements of opening distance, reset force, retention force or secure engagement. In particular, FAR only remains a legitimate evidence category. The mechanism may have crossed NEAR too quickly, missed its follower, or encountered a wiring problem. Firmware does not silently infer NEAR from FAR. Likewise, latch closed means the indicator remained closed, not that the catch can withstand any specified load. The practical benefit is better organization of a controlled manual experiment, not a universal pass/fail judgment.

The intended audience is a cabinet hardware repairer, a maker developing a small printed catch, or an educator exploring mechanical memory and return behavior. The workpiece must be removed from service and supported on a guarded tabletop fixture. This is not a diagnostic accessory for an occupied enclosure, a vehicle door, a fire door, a child barrier or any other safety-related latch. It has no actuator and cannot prevent a bad closure.

## Originality and comparison with existing designs

The repository's device directories and README concept titles were inventoried before selecting this project. The nearest relevant contracts, Catch Witness and Toggle Hold, were read. Catch Witness classifies reopening of an already settled latch using fresh handle and challenge associations. Toggle Hold checks whether a bistable device remains at its destination after a pressure witness is withdrawn. Neither correlates a set of opening checkpoints with the subsequent fixed-window return-latch indication. Float Band brackets float-switch transitions by a reference water-level word; its hardware interface is reused, but its reference-band engine is not.

The proposed contribution is this opening-history-conditioned experiment and its conservative evidence contract. Position contacts, electronic counters, latch testing and mechanical reset mechanisms are established techniques. A video camera can provide more detailed motion evidence if its recording is reviewed carefully. An instrumented force/displacement stand is preferable for calibrated mechanical characterization. Latch Reach trades those richer measurements for a small local matrix that is easy to inspect during slow bench adjustments.

Originality here means a distinct proposed device within the inspected repository. No comprehensive commercial search, patent search, worldwide uniqueness claim, patentability assessment or freedom-to-operate investigation has been performed. Practical value remains a hypothesis until real users compare its records with independent observations on a physical fixture. Reusing a proven electrical topology is intentional: the experiment, not a gratuitously different resistor network, defines the proposed device.

## Hardware specifications

| Item | Revision A design |
|---|---|
| Processor assembly | ST NUCLEO-F401RE, MB1136 family |
| MCU | STM32F401RET6, Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz; no external crystal dependency |
| Acquisition | Nominal 1 ms samples; five identical observations per channel to qualify |
| Position inputs | HOME, NEAR, FAR; disjoint low-energy dry-contact windows |
| Independent input | LATCH; closed contact indicates the selected bolt position |
| Carrier PCB | 80 × 70 mm, two copper layers, 1.6 mm FR-4 |
| Carrier supply | Nucleo 3.3 V and common ground; no local regulator |
| Host interface | USART2 TX on PA2 AF7 through onboard ST-LINK virtual COM |
| Serial framing | 115200 baud, eight data bits, no parity, one stop bit |
| Programming | Onboard ST-LINK SWD; flash origin 0x08000000 |
| Application | Linux desktop Python 3 and Tk; standard library only |
| Environment | Dry, supervised indoor bench; slow hand-operated detached mechanism |

The processor and its support circuitry remain on the purchased board. Consult ST UM1724 and the actual MB1136 board revision before attaching harnesses; verify the Arduino connector labels and ST-LINK virtual COM solder bridges. Register definitions identify the STM32F401 RM0368 peripheral addresses. A successful build is not a substitute for checking the correct board, wiring and supply voltage during bring-up.

### Connector and firmware mapping

| Bit | Signal | Field connector | J2 pin | Nucleo header | MCU pin |
|---|---|---|---|---|---|
| 0 | HOME | J3.1 | 1 | CN8.1 / A0 | PA0 |
| 1 | NEAR | J4.1 | 2 | CN8.2 / A1 | PA1 |
| 2 | FAR | J5.1 | 3 | CN8.3 / A2 | PA4 |
| 3 | LATCH | J6.1 | 4 | CN8.4 / A3 | PB0 |

Every field connector pin 2 is ground. J1 pin 1 connects to Nucleo 3V3 at CN6.4 and pin 2 to ground at CN6.6. Never connect the 5 V header to J1. The sensing signals use external pull-ups; firmware disables internal pulls and reads contact closure as logical one. J2 is not a powered sensor bus, and the field headers have no dedicated supply pin.

Each channel has a 10 kohm resistor from 3.3 V to SENSE and a 1 kohm resistor from SENSE to its field connector. At nominal values, a closed contact draws 0.300 mA and produces approximately 0.300 V at the MCU input. Four closed contacts draw 1.20 mA, with about 3.96 mW total resistor-network dissipation. These are calculations for the passive network, not measured system power. The development board and ST-LINK consumption must be budgeted separately and measured in the intended USB configuration.

There is no isolation barrier, surge protector, reverse-polarity circuit, battery charger or separate field power source. USB ground is shared with every switch. The series resistor is not permission to attach a powered output. Disconnect all contacts from their previous circuits before wiring. Do not attach mains, industrial control signals, a powered latch solenoid, a PLC output or a sensor of unknown output type. A broken wire looks open; a short to ground looks closed. The device cannot diagnose those faults from one channel alone.

## Architecture and block diagram

```text
Detached hand-operated latch fixture
  HOME window ---- J3 -- 1k -- SENSE0 ---- PA0
  NEAR window ---- J4 -- 1k -- SENSE1 ---- PA1
  FAR window ----- J5 -- 1k -- SENSE2 ---- PA4
  LATCH follower - J6 -- 1k -- SENSE3 ---- PB0
                            |
                   each SENSE: 10k to 3V3
                            |
NUCLEO-F401RE: 1 ms acquisition -> independent qualifiers
                                     |
                   baseline -> travel -> return window
                                     |
                 checkpoint set x latch outcome histogram
                                     |
                    bounded LR1 frame + CRC-16
                                     |
Linux Tk companion <- USB ST-LINK VCP <- USART2 TX
  matrix / setup / replay / private CSV export
```

The hardware generator creates actual resistor and connector symbols, wires, named nets, footprint assignments and a routed PCB. Symbols are embedded and supplied in a local KiCad library. PCB footprints retain complete library identities. The verifier runs native ERC and DRC, exports a fresh schematic netlist, and compares each reference, value, footprint and pin-to-net connection against both PCB pads and the connectivity manifest. Passive-only ERC cannot establish that the external Nucleo is powered correctly or that the mechanical fixture is safe.

## Bill of materials and fixture construction

Populate R1, R3, R5 and R7 with 10 kohm one-percent axial resistors. Populate R2, R4, R6 and R8 with 1 kohm one-percent axial resistors. All use DIN0207 bodies with 7.62 mm lead spacing; at least 0.125 W rating is sufficient for the intended resistor network. J1 and J3 through J6 are two-pin 2.54 mm headers; J2 is a four-pin header of the same pitch. Use mating insulated connectors, strain relief, clear signal labels and short signal/ground pairs. The board has no mounting holes and is intended for insulating edge rails inside a dry enclosure.

External items are one NUCLEO-F401RE, a data-capable USB cable, four low-level dry-contact switches, a hand-operated linear slide, a rigid base, adjustable switch brackets, an unloaded latch specimen, and guards for pinch points. Omron D2F-01L is an example low-level lever-switch family to investigate, not a claim that every contact in that family is validated at this network's current. Verify the purchased suffix, contact material, operating force and suitability for roughly 0.3 mA switching. Contact wetting current and mechanical drag are procurement and calibration gates.

A starting fixture geometry uses a 200 mm base rail with a captive carriage and a mechanically limited 100 mm stroke. Place HOME near the closed stop, NEAR around 40 mm opening and FAR around 80 mm opening. Use adjustable cam tabs to produce disjoint windows: for example HOME over the first 5 mm, NEAR from 35 to 45 mm, and FAR from 75 to 85 mm. These are construction starting points, not calibrated dimensions or universal latch requirements. Relocate the windows to suit the specimen's reset motion while preserving disjoint operation and documenting the actual positions.

At a nominal hand speed no greater than 200 mm/s, a 10 mm window lasts approximately 50 ms, comfortably longer than the five-sample qualifier. The operational recommendation is at least ten milliseconds of observed contact closure and clear separation. Do not use high-speed impacts. If the physical travel cannot provide adequate windows, change the mechanical arrangement rather than increasing confidence in missing observations. Stop guards must keep fingers away from the catch and carriage; fixture parts must not carry stored spring energy beyond a small supervised demonstrator.

Place LATCH on an independent light follower or auxiliary indication that follows bolt state without supporting the bolt. The follower must not help engagement or prevent release. Compare the mechanism with and without that follower to detect disturbance. Define in the notebook exactly which position closes it: a bolt reaching a switch is not automatically proof of striker capture. Photograph or sketch the installation, record checkpoint positions and keep those records with the exported CSV. An enclosure drawing and calibrated mechanical assembly are not supplied; the prototype requires this physical engineering work before use.

## Firmware behavior and design decisions

The reset handler initializes data and BSS and installs a Cortex-M vector table. The clock driver restores the HSI configuration with bounded waits before the serial interface becomes usable. GPIO initialization leaves PA0, PA1, PA4 and PB0 as inputs. PA2 becomes the USART2 transmitter; no command receiver is enabled. SysTick provides the nominal millisecond timebase. The independent watchdog is started after board setup, and watchdog initialization failure latches a terminal acquisition fault. A failed clock restoration stops before telemetry can be trusted.

The main loop polls the UART without blocking, samples when the tick changes, feeds the watchdog and attempts a cumulative snapshot every 100 ms when the transmitter is free. A frame is bounded by the 768-byte buffer. An undrained transmission times out after 100 ms; the companion can recover at the next complete LR1 marker even when the preceding frame lacked a newline. UART timeout counters are separate from acquisition evidence. Actual oscillator accuracy and electrical serial performance remain physical validation work.

Each channel qualifies independently after five identical acquired observations. This prevents LATCH chatter from postponing an unrelated NEAR observation. Startup requires all four channels to have qualified before the initial HOME baseline can become ready. Baseline credit requires raw and qualified HOME-only position at both ends of an interval. One hundred such intervals produce READY. LATCH can be open or closed during this baseline; the experiment does not assume a previously successful latch.

A qualified HOME departure while raw HOME is also absent begins TRAVEL. The start sample has age zero. If a checkpoint qualifies on that same sample, it is credited immediately; it is not lost because departure processing happened first. During travel, raw NEAR with qualified NEAR adds the NEAR bit, and raw FAR with qualified FAR adds the FAR bit. Repeated visits and reverse travel do not erase bits or create additional trials. This is a set of witnessed checkpoints, not an ordered trajectory or maximum-distance estimate.

When the raw position is HOME only and the qualified position agrees, TRAVEL becomes RETURN. The current qualified LATCH level is captured. Any disagreement with the raw latch at that boundary already marks the window disturbed. Over the following 100 intervals, HOME must remain raw and qualified. Every observed latch disagreement with the captured level, including a one-sample raw excursion that does not survive qualification, makes the disturbed flag sticky. At completion, exactly one histogram cell is incremented: stable open, stable closed or disturbed for the retained checkpoint set.

The complete trip must finish strictly before age 30000 acquired milliseconds. Rejection wins at the deadline even when a result would otherwise complete on that sample. Any simultaneous raw position contacts reject an active trip; loss of HOME during RETURN also rejects it immediately. Rejection clears the working capture and requires a fresh HOME baseline. It does not replace the completed histogram or guess an outcome. Baseline overlaps merely prevent arming because no trip has started yet. Short HOME dips in READY that never qualify are not trips.

After completion a new 100 ms baseline is required before another departure can start a trial. The operator must wait for READY, not simply assume that returning to the stop armed the next trip. No event identifier follows a real physical object, so replacing the specimen or moving the fixture midway through a run invalidates experimental comparability even when the software remains internally consistent.

Sampling discontinuity is terminal. Repeated or skipped acquisition timestamps are not credited as stable time. A gap abandons an active trip once, sets F_GAP and freezes all acquisition-derived evidence; subsequent heartbeats can still advance wall time. Configuration failures and exhausted counters are also terminal. Counter updates are staged transactionally, so overflow cannot publish half a histogram result. Counters are capped at one billion, and ordinary unsigned timestamp wrap is supported. All state is volatile; resetting the MCU begins a new experiment rather than resuming an old notebook.

## LR1 protocol and companion interface

Each line contains ASCII `LR1`, followed by comma-separated unsigned decimal fields, then an asterisk, four uppercase hexadecimal CRC digits and LF. CRC is CRC-16/CCITT-FALSE with polynomial 0x1021, initial value 0xFFFF and no final XOR, computed over the bytes before the asterisk. Fields are, in order: sequence, now, fault, raw, qualified, valid, phase, seen, latch, disturbed, age, quiet, starts, completed, rejected, twelve row-major histogram cells, gaps and tx_errors. The cell order is four checkpoint rows times open, closed, disturbed. The standard check string `123456789` gives 0x29B1.

Sequence counts accepted acquisition samples, not transmitted packets. Phase values are baseline zero, ready one, travel two, return three and halted four. Fault bits are configuration one, watchdog two, gap four and overflow eight. A nonzero fault is terminal for acquisition. Raw and qualified masks use the connector bit order above. Valid identifies channels that have qualified at least once. Seen is a two-bit set with NEAR one and FAR two. Age describes the active trip; quiet describes baseline or return progress. Working fields are cleared on ordinary completion or rejection; the matrix and totals remain cumulative.

The companion validates field counts, canonical decimal syntax, ranges, CRC and accounting conservation. It checks monotonic counters, terminal evidence freezing, plausible sample/clock progress, retained checkpoint membership, return-window credit and the attribution of an existing trip's completion. Missing serial snapshots are allowed; missing acquisition ticks in an otherwise healthy interval are not. These are necessary consistency checks, not a reconstruction of every hidden sample. CRC detects ordinary corruption, not malicious changes or trustworthy physical sensing.

Launch the application with `python3 app/workbench.py`. The Return outcomes screen shows the matrix, current phase, totals, working age and raw mask. Setup and limits supplies the connector and bench procedure. Connect opens a Linux serial device at the fixed baud rate and clears the prior displayed session only after opening succeeds. A second connection is refused. Disconnect retains evidence. Replay loads a regular LR1 file of at most 2 MiB and never labels it physical live evidence. Malformed recordings, empty recordings, FIFOs and partial final frames are rejected.

The serial read loop is nonblocking and detects hangup through poll flags. Input fragments and CRC failures increment visible parser errors. Session inconsistency blocks the run and preserves the retained records for export. Up to 2000 snapshots are retained in memory; cumulative matrix totals still describe the whole device run. Export creates a new mode-0600 CSV exclusively and refuses to overwrite an existing file. Metadata includes author, provenance, status, parser errors and retained row count even if no trial completed.

Advancing telemetry is distinguished from absolute freshness. The app requires advancing timestamps and checks short reception pauses and bounded cumulative host/device drift, but a receive-only protocol has no persistent boot identifier or request/response clock synchronization. Some resets can resemble forward traffic, and a stream already delayed before connection can remain plausible. Explicitly reset the experimental setup and reconnect when continuity is uncertain. Long runs can be marked stale because the uncalibrated HSI drifts relative to the host; this conservative warning is not proof of a broken latch.

## Build, test and first bring-up

Required development tools are GNU Make, host GCC, arm-none-eabi-gcc and binutils, Python 3 with Tk, KiCad 9 CLI and its pcbnew bindings. On Debian-family systems pcbnew may be available only through `/usr/bin/python3`. No Python package installation or application network service is required. Run from this device directory:

```sh
make -C firmware all test fixture
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 app/workbench.py
```

The fixture target emits synthetic C-encoded observations to `firmware/build/demo.lr1`. It is a software fixture, not a measurement. Load it through Replay to exercise all four rows with stable-open and stable-closed outcomes. The host engine tests also exercise disturbance, raw overlaps, rejected returns, exact deadline precedence, timestamp wrap, terminal gaps, transactional overflow and every six-symbol clear/NEAR/FAR word against an independent set oracle. Clock and UART drivers have separate register-model tests. GUI tests use actual Tk callbacks and Linux pseudo-terminals, not a physical USB board.

`tools/generate_hardware.py` regenerates the native electrical files. Its sibling import script documents platform provenance and refuses to overwrite existing files; it is not a build step. `make -C firmware flash` invokes OpenOCD only when the operator deliberately connects a target. Do not run flashing against an unidentified debugger. The validation report records the actual software and CAD results for this revision and distinguishes them from unperformed hardware work.

For physical bring-up, first inspect the unpowered PCB, resistor values, solder joints and connector polarity. Check continuity against the schematic and check for supply shorts. Power the Nucleo without the carrier and establish programming and virtual COM behavior. Then connect the carrier with all field switches open, measure its supply and all sense voltages, and close one disconnected test contact at a time. Confirm the corresponding raw bit and no others. Only after these tests should the specimen harness be attached.

Finally, compare slow controlled trips with independent video or a written contact-state reference. Test each checkpoint alone, both checkpoints, repeated reversals, a deliberate missed checkpoint, a disturbed latch during return and a return that leaves HOME. Repeat with the LATCH follower removed to assess mechanical influence. Verify a deliberate sampling interruption in a controlled debug experiment, but do not mistake simulated host fault tests for proof of physical watchdog or brownout behavior. Fabrication, enclosure safety, switch repeatability, ESD, EMC, temperature performance and calibration remain open release gates.
