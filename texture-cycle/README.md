# Texture Cycle — captive-swatch preference graph notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored hardware sources are MIT licensed. Installed KiCad footprint geometry retains its upstream licensing; see LICENSE.

Revision A is an unbuilt engineering prototype, not a validated sensory instrument. The supplied PCB is a passive interface carrier for a separately purchased NUCLEO-F401RE. Target linking, host simulations, application operation and electrical CAD checks are reproducible; fabrication, flashing, assembled electrical behavior and participant usability remain untested. No claim of patent novelty, medical benefit, objective fabric quality or certified measurement is made.

## Purpose and useful outcome

Texture Cycle helps a textile teaching studio retain the contradictions that arise when somebody compares fabric swatches by touch. Four captive samples occupy a manually indexed disk. The participant nominates one sample as a reference, turns to another sample, and declares that the new sample is preferred for one explicit criterion. The instrument stores that directed comparison without deleting any previous answer. If A is preferred to B, B to C, and later C to A, the display shows the cycle instead of silently turning the answers into a misleading strict ranking.

The specific user is a textile instructor preparing a small discussion about handle, subjective comfort, or how a comparison question changes a decision. The fixture also suits a craft designer selecting lining samples for a noncritical pouch. These users currently spread samples over a table, ask comparative questions, and write results in a notebook. Loose labels move, typing interrupts touch, and a simple score total can conceal a circular set of answers. The useful moment is seeing the third arrow close a triangle while all earlier declarations remain visible.

A cycle does not mean the participant failed. Preferences can be context-dependent, change with touch duration, or combine several properties. Someone may prefer warmth in one comparison and smoothness in another even when the instructor asked a single question. The device deliberately does not diagnose inconsistency in people, exclude participants, or decide which answer is correct. It retains a small record that makes a follow-up conversation possible. Reverse answers on the same pair form a two-edge cycle and are equally legitimate evidence to retain.

## Originality, repository comparison and prior art

The repository was synchronized and its device directories, README titles and purpose paragraphs inventoried before selection. Shim Bracket uses an indexed selector to collect fit/refusal claims against an ordered thickness scale. Texture Cycle has no ordered physical scale: it retains arbitrary directed relationships and detects cycles of any length among four samples. Its reference is consumed after one comparison, and replacing a reference does not replace previous graph evidence. This is a different evidence contract rather than a renamed thickness notebook.

ContactAtlas measures the electrical transition graph of a detached rotary switch; its vertices are observed switch signatures, not tactile samples, and its edges are physical transitions rather than human judgments. Maze Return chronologically erases loops in a checkpoint walk. Texture Cycle does the opposite: cycles are never erased, comparisons need not follow a continuous route, and the order of recording independent edges does not determine the result. Other textile devices in this inventory, such as Pick Witness and StitchScope, address loom operation and stitch formation rather than pairwise subjective choice.

Paired comparison is established prior art. The article “Effect of Scaling Method on Perception of Textiles” compares tactile scales and paired-comparison rankings. “Assessment of Knitted Fabric Smoothness and Softness Based on Paired Comparison” describes related textile evaluation. The IFATCC paper “Evaluation of Handle of Textile Fabrics” discusses the subjective contribution to handle and alternative ranking methods. The proposed contribution here is a low-cost captive-swatch interface with raw-envelope rejection, consumed references and a contradiction-preserving graph display. This bounded search does not establish that no similar apparatus exists anywhere.

References:

- https://doi.org/10.2466/pms.1985.60.1.335
- https://doi.org/10.1007/s12221-019-1112-1
- https://www.ifatcc.org/wp-content/uploads/2018/01/PL10.pdf
- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 reference manual RM0368, for the GPIO, RCC, USART, SysTick and watchdog register definitions.

An automatic softness meter was rejected because surface friction, compliance and warmth are not interchangeable quantities and require independent calibrated sensing. A camera recognizing sample labels was rejected because it adds occlusion and privacy concerns without improving the declared preference itself. A phone-only form remains a sensible alternative; hardware is justified only if captive identity and tactile paddles reduce disruption. A later usability comparison must test that hypothesis rather than assume it.

## Hardware specifications and architecture

| Function | Revision A implementation |
| --- | --- |
| Controller | STM32F401RET6 on external NUCLEO-F401RE, MB1136 |
| Core clock | Reset HSI, nominal 16 MHz; no external crystal required |
| Memory | 512 KiB flash, 96 KiB SRAM on the target MCU |
| Inputs | Two Gray selector contacts; REFERENCE and PREFER paddles |
| Contact domain | Voltage-free, normally open contacts, external 3.3 V pullups |
| Acquisition | Nominal 1 ms loop, five equal observations to qualify |
| Connectivity | USART2 TX, PA2 AF7, 115200 baud 8N1, ST-LINK virtual COM |
| Power | USB to purchased Nucleo; its 3V3 and ground feed the carrier |
| PCB | 80 × 70 mm, two copper layers, 1.6 mm FR-4, through-hole parts |
| Fixture envelope | Proposed 220 × 220 × 65 mm covered tabletop disk assembly |
| Companion | Linux Python 3 with Tk; standard library, no cloud service |
| Persistence | Volatile MCU evidence; explicit private desktop CSV exports |

```text
Four captive fabric patches on detented hand disk
                |
       two Gray-code dry contacts -----------+
                                             |
REFERENCE paddle ---- dry contact -----------+-- passive pullup/series board
PREFER paddle ------- dry contact -----------+       |
                                                    | PA0 PA1 PA4 PB0
USB power/debug --> external NUCLEO-F401RE --> qualify press envelopes
                          |                         |
                          |                  consume reference / add edge
                          |                         |
                          +--- USART2 TX <---- retained graph + cycle check
                                      |
                              ST-LINK virtual COM
                                      |
                          Linux Tk graph/history/export
```

The MCU, regulator, decoupling, reset circuit, USB interface and SWD debugger are on the purchased controller, not on the passive carrier. Ordering only the supplied PCB will not produce a working device. There is no radio, battery charger, actuator, heater, haptic motor or connection to powered workshop machinery. The absence of hazardous outputs is intentional: this is a notebook for declarations, not an interlock.

## Electrical design, pin map and bill of materials

| Carrier connection | Meaning | Nucleo destination |
| --- | --- | --- |
| J1.1 / J1.2 | 3V3 / ground | CN6.4 / CN6.6 |
| J2.1 SENSE0 | selector low bit | CN8.1 A0, PA0 |
| J2.2 SENSE1 | selector high bit | CN8.2 A1, PA1 |
| J2.3 SENSE2 | REFERENCE paddle | CN8.3 A2, PA4 |
| J2.4 SENSE3 | PREFER paddle | CN8.4 A3, PB0 |
| J3.1 / J3.2 | SELECT0 / ground | selector contact 0 |
| J4.1 / J4.2 | SELECT1 / ground | selector contact 1 |
| J5.1 / J5.2 | REFERENCE / ground | first paddle |
| J6.1 / J6.2 | PREFER / ground | second paddle |

These connector assignments match UM1724 Table 16. Preserve the default ST-LINK USART routing: SB13/SB14 on and SB62/SB63 off, subject to the actual board revision. The firmware enables transmit only; PA3 is not a command input. Verify bridge population rather than assuming every used Nucleo has its factory setup. J2 carries signals, not power. Never apply 5 V to J1 or connect an externally powered sensor output to J3–J6.

Each channel uses a 10 kohm pullup from 3V3 to SENSE and a 1 kohm series resistor from SENSE to the external contact. Nominal closed current is 3.3 V divided by 11 kohm, or 0.30 mA. The low input is nominally 0.30 V. All four closures draw approximately 1.20 mA, or 3.96 mW from the 3V3 rail. These are calculated values, not measurements. Select gold signal contacts rated for low wetting current; a switch designed only for mains loads may be unreliable here.

| Quantity | Part or specification | Role |
| --- | --- | --- |
| 1 | ST NUCLEO-F401RE | External controller and programmer |
| 4 | 10 kohm, 1%, 0.25 W axial, 7.62 mm pitch | R1/R3/R5/R7 pullups |
| 4 | 1 kohm, 1%, 0.25 W axial, 7.62 mm pitch | R2/R4/R6/R8 series resistance |
| 5 | 1×2, 2.54 mm vertical through-hole header | J1, J3, J4, J5, J6 |
| 1 | 1×4, 2.54 mm vertical through-hole header | J2 signal harness |
| 2 | Gold dry-circuit normally-open lever switches | Gray cam followers |
| 2 | Gold dry-circuit normally-open pushbuttons | Large labelled paddles |
| 1 set | Insulated jumper harness, strain relief, covered enclosure | Assembly |
| 1 set | Disk, axle, detent, sample clips, two cam tracks | Mechanical fixture |

The switch selection is a performance specification, not a verified supplier stock claim. Confirm the chosen switch's minimum current rating and mounting drawing before cutting the fixture. Budget approximately USD 45–90 for controller, small-batch carrier, switches and hardware, excluding labor, shipping and a computer; this is a planning estimate rather than a quotation. USB/controller consumption dominates the passive network and has not been measured. Reserve a conventional 5 V, 500 mA USB supply budget pending bench measurements; no battery-runtime figure is offered.

Series resistance does not provide isolation or a certified ESD barrier. All grounds connect to the USB host. Keep contact wiring below approximately 0.5 m inside the covered assembly. No TVS, reverse-polarity circuit or outdoor surge protection is provided. Use clean, dry fabric samples and unplug before rewiring. Protect exposed header pins from fingers and conductive scraps. The passive schematic's zero ERC result cannot validate the external power source or connector installation.

## Mechanical fixture definition

Use a 180 mm diameter disk, nominally 6 mm thick, on a captive central M6 shoulder axle above a 220 mm square base. Fit four removable 35 mm square sample frames with centers at radius 55 mm and angles 0, 90, 180 and 270 degrees. Label the underside A, B, C and D. The sample surface must remain below a rounded cover so neither loose threads nor fingers can enter the disk/cover shear line. A 32 mm square rounded inspection aperture at one station exposes one patch at a time. A perimeter hand knob moves the disk; no motor is fitted.

Two concentric cam tracks on the underside actuate stationary low-force lever switches. At detents A, B, C and D, the required logical closure patterns are 00, 01, 11 and 10 respectively. Provide at least ±12 degrees of constant cam height around each detent, with transitions confined between detents. The exact cam lift depends on the selected switch's travel and overtravel specification; provide an adjustable follower bracket rather than using the switch as a mechanical stop. Four spring-ball detents locate the disk independently of the sensing switches. A continuity meter must verify all four codes with the assembly unpowered before controller connection.

A nominal 1 mm cover clearance and 2 mm edge radii are starting dimensions, not a validated pinch-safety design. Use guarded access and adult supervision for the first prototype, and adjust clearances after inspecting the actual materials. The carrier slides into insulated edge rails in a separate 90 × 80 × 25 mm compartment; the PCB intentionally has no mounting holes. The Nucleo occupies a separate covered pocket, with USB strain relief and access to its reset button. Do not stack the carrier loose on exposed controller pins.

A code is not a positive detent sensor. Every two-bit combination is legal, so an intermediate disk position held still can qualify as a sample. The detent mechanism, aperture registration and visible indexing are essential assumptions. Firmware rejects selector movement during a press but cannot prove the chosen patch is under the aperture or prevent a person pressing without touching it. An upgraded detent witness would need a fifth input and a protocol revision, not an undocumented interpretation of the existing four wires.

## Operating procedure and evidence semantics

Install four dry samples, record their mapping and one criterion in a local study note, and deliberately reset the MCU. Open the companion and connect. Release both paddles until READY appears. Turn to the intended reference, touch it, and press then release REFERENCE without moving the disk. The graph display highlights that reference. Turn to the other sample and consider the same criterion. If it is preferred, press and release PREFER. An arrow goes from the newly selected sample to the stored reference; the reference is then consumed.

To record the opposite direction, nominate the other sample as reference and then select the preferred sample. There is no implicit negative answer. A tie or inability to choose should be written in the study note, not encoded by a double press. Both paddles together are rejected. Nominating a new reference before using the old one simply replaces the pending reference and increments the reference-stroke counter. Self-comparison is rejected and consumes the pending reference so a misplaced next press cannot reuse it.

Repeated identical preferences increase the preference count but do not add duplicate graph edges. Opposing and cyclic preferences stay present until MCU reset. The graph does not retain per-edge vote counts, participant identity, magnitudes or confidence. No cycle means only that the retained declarations are acyclic; it does not mean all pairs were tested or that an objective ordering exists. The companion explicitly displays the number of unordered pairs without any declaration.

Use one participant, one sample mapping and one question per MCU session. Do not replace samples midway. Export before resetting. Reconnecting the app does not clear MCU SRAM. Conversely, unplugging or watchdog reset loses MCU evidence. Because there is no persistent boot identifier, some resets can look like forward traffic. Clock rollback and counter rollback freeze the companion when detectable, but they do not prove uninterrupted physical session identity.

## Firmware implementation and failure behavior

The firmware is freestanding C11 with an actual Cortex-M4 startup vector, linker script and register-level drivers. No vendor SDK, RTOS, dynamic allocation or floating point is needed on the target. Clock setup restores nominal HSI before UART initialization. The board driver checks GPIO mode and pull configuration, starts SysTick at nominal 1 kHz, and feeds the independent watchdog. The acquisition loop samples once when the millisecond counter changes; it never fabricates missed intermediate observations.

Selector bits qualify as a bundle after five identical samples. Each paddle qualifies independently so selector chatter cannot indefinitely hide a sustained press. WAIT requires a released baseline. READY captures an identity at the first qualified press; ACTIVE retains it through release. Raw selector changes, opposite paddle activity, or either paddle appearing within the same prequalification envelope mark the stroke tainted. A tainted stroke is rejected on release and clears the pending reference. This guards a sequential REFERENCE-to-PREFER handoff as well as simultaneous overlap.

The maximum qualified press age is 10,000 nominal milliseconds. Timeout wins if release qualifies on that exact endpoint. A missed acquisition interval aborts an active stroke, clears the pending reference and restarts qualification while retaining committed graph edges. Completed strokes conserve starts as references plus preferences plus rejected plus aborted; an active stroke adds one outstanding disposition. Counter exhaustion enters HALT without silently wrapping. References and preferences cannot overflow independently because each completion consumes a previously reserved start.

The cycle detector computes a four-node transitive closure in a temporary array and checks for reachability back to the same node. Only the original direct edges are stored. A different bounded-walk oracle in the tests checks all 4,096 possible directed graphs without self edges. Main-loop telemetry uses a bounded nonblocking UART buffer, checks a 100 ms transmit deadline and reports transport failures. A faulted engine continues reporting retained evidence if serial output remains available. Hardware timing and watchdog behavior still require real-board testing.

## Wire protocol and companion application

Every periodic record starts with TC1 and contains twenty unsigned decimal fields in this exact order:

```text
now,state,levels,valid,starts,references,preferences,rejected,aborted,gaps,
faults,tx_errors,graph,cycle,anchored,anchor,age,tainted,selected,outcome
```

The complete frame is `TC1,<fields>*<four uppercase hexadecimal CRC digits>\n`. CRC-16/CCITT-FALSE uses initial value 65535 and polynomial 0x1021 over the marker and comma-separated fields before the asterisk. State numbers are WAIT=0, READY=1, ACTIVE=2 and HALT=3. Graph bit `4*winner+reference` stores one direct edge. Logical contact bits indicate closure, despite low electrical voltage. Outcome values 4 and 8 denote REFERENCE and PREFER; 12 denotes ambiguous paddle activity. Compatibility is included in every frame, so late attachment does not depend on a one-shot boot announcement.

Run `python3 app/workbench.py` from this directory. The connection page opens a Linux tty at 115200 baud and supports explicit disconnect or bounded replay loading. The graph page shows directed arrows, consumed-reference status and retained cycle warnings. Opposing arrows are offset so neither hides the other. History shows changes in counters and graph. Policy/export explains the procedure and writes a private CSV with author, provenance, label, parser errors, connection status and freshness. Exports refuse to overwrite an existing file and quote free text defensively against spreadsheet formula interpretation.

The app uses only the Python standard library and Tk; install the operating system's Tk package if absent. It has no package download scripts, cloud account, Bluetooth permissions, network listener or target-control commands. Replay files must be regular files at most 2 MiB, with complete framing and at least one accepted record. History retains at most 2,000 snapshots, not an unlimited event log. Invalid checksums, impossible accounting, disappearing edges, reused references and contradictory duplicate timestamps are rejected or counted according to live/replay policy.

Live status requires advancing device time, connection ownership and recent receipt. A fixed two-second host/device progress allowance detects extended buffering and drift; it can also require a conservative reconnect when an uncalibrated clock drifts too far. Repeated timestamps do not refresh progress. Sparse snapshot checks are necessary consistency bounds, not a reconstruction of every hidden raw sample. In particular a qualified high paddle can already be partway through raw release qualification, so the next record may legitimately complete sooner than five additional milliseconds.

## Build, verification and release limits

From the repository root, run `python3 texture-cycle/tools/verify.py`. It builds and links the Cortex-M4 firmware, runs sanitized host tests, generates a synthetic recording using the real C encoder, compiles Python sources, exercises the parser and real Tk event loop with a Linux pseudo-terminal, and checks the KiCad schematic and PCB. Required host tools are GCC, arm-none-eabi GCC/binutils, Python with Tk, Xvfb, KiCad 9 CLI and its pcbnew module. No hardware flashing is performed by this verification command.

For manual target compilation use `make -C texture-cycle/firmware all`. On an intentionally connected controller, after checking wiring, `make -C texture-cycle/firmware flash` invokes OpenOCD through ST-LINK and verifies programming. That command was not executed in this run. The linker reserves a stack margin and builds an actual ELF and binary; generated products remain ignored. The synthetic demo under firmware/build is not an observation of a real person or fabric.

KiCad sources contain real local passive symbols, installed footprint geometry, pin-to-net mappings and routed copper. The checker exports a fresh schematic XML netlist and compares every pin, value and footprint with actual PCB pads and the connectivity manifest. Zero ERC, zero DRC violations and zero unconnected items are gates, but not a substitute for fabrication review. The exported schematic was rendered and inspected for readable labels. Check manufacturing tolerances, connector orientation and the selected switch drawings before ordering.

Bring-up must first measure the unpowered board for shorts and verify every switch code, then check open and closed GPIO voltages with current-limited power. Confirm UART framing, acquisition timing and reset recovery on the actual target. Next test the fixture with identical swatches to evaluate positional bias, then exchange sample positions and repeat. Compare results with paper notes and ask whether the paddles help or distract. These physical and usability stages remain outstanding; the current deliverable is a build-verified design package, not a demonstrated product.
