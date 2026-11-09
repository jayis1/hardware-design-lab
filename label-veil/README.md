# Label Veil — concealed-code tactile label classification notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored CAD sources are MIT licensed; installed KiCad footprint geometry retains its upstream licensing. See LICENSE.

Revision A is an unbuilt engineering prototype. The deliverable includes a routed passive carrier, freestanding STM32 firmware and a working Linux desktop companion. Software builds, synthetic tests and CAD checks have been executed. Fabrication, flashing, electrical bring-up, physical timing, enclosure ergonomics and participant studies have not. It is not a medical assessment, accessibility certification or authenticated examination system.

## Purpose and practical value

Label Veil helps a packaging designer ask a narrow question: can a person identify either of two taught tactile label classes without seeing the hidden class code? A removable sample cassette carries a tactile patch on its accessible face and a concealed mechanical class key underneath. One dock contact reports that the cassette is seated. A second contact reads the hidden binary class. Two separate answer paddles let the participant declare A or B without a keyboard. The firmware withholds the current encoded class from telemetry until a clean presentation ends, then increments one cell of a two-by-two classification matrix.

The intended users are small design studios, textile educators and makers comparing proposed tactile markings on noncritical storage containers. They can teach both names, conceal the cassette underside, present an independently randomized sequence, and see which confusions accumulated. The device does not decide what a texture means. The operator assigns class membership and is responsible for attaching the correct tactile face to the encoded carrier. A swapped patch is a mislabeled experiment that electronics cannot detect.

The useful distinction from a paper tally is that dock identity, a single unambiguous declaration, release, and removal belong to the same observed presentation. Opposite paddle activity, repeated qualified answers, class changes while seated, premature removal and acquisition loss do not quietly become successful classifications. Rejected presentations remain in a separate counter instead of vanishing from the denominator. An accepted classification means that the observation contract was satisfied; an accepted wrong answer is valid evidence and remains in an off-diagonal matrix cell.

The instrument deliberately measures neither fingertip force nor intrinsic surface roughness. It cannot establish that someone touched the patch, avoided visual cues, understood the instructions or answered independently. Latest duration includes the presentation and removal process, not just reaction time. Treat this as a local design-discussion notebook, not a psychophysical threshold measurement.

## Originality and alternatives

The repository directories were inventoried before adopting this interrupted design, and overlapping README purposes were checked. Texture Cycle is the closest conceptual neighbor: it retains directed preferences between four swatches and exposes cycles in those preferences. Label Veil instead compares a declared binary identity against a concealed encoded class, retaining a confusion matrix. It has no preference graph, consumed comparison reference or assumption that one texture is better than another. TactiScript renders tactile information with actuators; Label Veil records manual responses to passive specimens. ContactAtlas records electrical switch connectivity rather than human classification.

Tactile identification and two-alternative response experiments are established methods. A bounded public search found “Verbal Labels Facilitate Tactile Texture Discrimination in a Perceptual Learning Task,” DOI 10.1101/2021.02.09.430389, and “Neuronal correlates of label facilitated tactile perception,” https://www.nature.com/articles/s41598-018-37877-w. These are relevant prior art for the experimental idea, not evidence that this proposed fixture improves results. The repository-specific contribution is the concealed cassette code, release-before-removal evidence contract and conservative rejection of ambiguous contact histories. No worldwide uniqueness or patent novelty is asserted.

A phone form is simpler when an observer can tally accurately. A camera can recognize labels but introduces occlusion, illumination and privacy issues. Automatic texture classification would answer a different question from what a participant recognized. Hardware is justified only if a pilot study shows that the dock and paddles reduce labeling mistakes or interrupt touch less than a keyboard. That benefit remains a hypothesis to test.

## Hardware specification

| Function | Revision A implementation |
| --- | --- |
| Controller | External ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| CPU | Cortex-M4, nominal 16 MHz internal HSI clock |
| Memory | 512 KiB flash, 96 KiB SRAM; no external storage |
| Acquisition | Four low-active dry-contact inputs, nominal one sample per millisecond |
| Qualification | Five identical samples; dock and class treated as one bundle |
| Connectivity | USART2 TX on PA2 AF7, 115200 baud, 8 data bits, no parity, one stop bit |
| Host connection | Nucleo ST-LINK virtual COM over USB; receive-only companion |
| Power | USB into the purchased Nucleo; its 3.3 V rail powers the carrier |
| Carrier | 80 by 70 mm, two copper layers, 1.6 mm FR-4, through-hole components |
| Proposed fixture | Covered tabletop dock approximately 220 by 160 by 70 mm |
| Application | Python 3 / Tk desktop notebook on Linux; no third-party Python packages |
| Persistence | Volatile MCU counters; explicit local CSV export |

The MCU, regulator, decoupling, USB connector, oscillator support, reset and SWD programming circuits are on the purchased Nucleo, not hidden omissions from a supposed custom MCU board. The supplied PCB is intentionally only the contact carrier. There is no battery, radio, motor, mains input, galvanic isolation or claimed ESD protection. Power only through the Nucleo USB configuration documented for the actual board revision; do not connect another supply to carrier J1.

## Architecture

```text
Safe tactile patch on keyed removable cassette
                |                       |
          dock witness           concealed class key
                |                       |
            J3 DOCK                J4 CLASS_B
                |                       |
Answer paddle A --> J5       Answer paddle B --> J6
                \                       /
                 passive pullup / series carrier
                       J2 SENSE0..SENSE3
                              |
                 external NUCLEO-F401RE
                 PA0 / PA1 / PA4 / PB0
                              |
          1 ms acquisition -> qualification -> trial engine
                              |
                  retained matrix + diagnostics
                              |
                CRC-framed LV1 / USART2 TX
                              |
               ST-LINK USB virtual COM -> Tk notebook
                                               |
                                      private local CSV
```

The firmware never enables the USART receiver. No host command can change the class, erase results or remotely arm a trial. Mechanical cassette selection and answer paddles constitute the experimental input. A reset is the only way to clear MCU history. This simplifies the trust boundary but does not authenticate telemetry: anybody controlling the host serial source can fabricate records with a valid CRC.

## Wiring contract and bill of materials

| Carrier connection | Signal and Nucleo connection |
| --- | --- |
| J1 pin 1 | VCC, 3.3 V from CN6 pin 4 |
| J1 pin 2 | Ground to CN6 pin 6 |
| J2 pin 1 | SENSE0 to CN8 pin 1, A0 / PA0, dock |
| J2 pin 2 | SENSE1 to CN8 pin 2, A1 / PA1, encoded class B |
| J2 pin 3 | SENSE2 to CN8 pin 3, A2 / PA4, answer A |
| J2 pin 4 | SENSE3 to CN8 pin 4, A3 / PB0, answer B |
| J3 pin 1 / pin 2 | DOCK / ground |
| J4 pin 1 / pin 2 | CLASS_B / ground |
| J5 pin 1 / pin 2 | ANSWER_A / ground |
| J6 pin 1 / pin 2 | ANSWER_B / ground |

Each input uses a 10 kilohm pullup to VCC and a 1 kilohm series resistor between SENSE and the external contact. Firmware interprets a closure to ground as logical one. DOCK closed means occupied; CLASS_B open means A and closed means B. Class is irrelevant while the dock is empty. All contact connectors have signal on pin 1 and ground on pin 2; confirm the square pad and silkscreen before plugging anything in.

| Reference | Quantity | Selection |
| --- | --- | --- |
| R1, R3, R5, R7 | 4 | 10k axial metal-film, 1%, 0.25 W, 7.62 mm formed pitch; Yageo MFR-25FBF52-10K or dimensional equivalent |
| R2, R4, R6, R8 | 4 | 1k axial metal-film, 1%, 0.25 W, 7.62 mm formed pitch; Yageo MFR-25FBF52-1K or dimensional equivalent |
| J1, J3–J6 | 5 | 1 by 2, 2.54 mm straight pin header; cut from compatible breakaway strip |
| J2 | 1 | 1 by 4, 2.54 mm straight pin header |
| External controller | 1 | NUCLEO-F401RE with onboard ST-LINK retained |
| External switches | 4 | Gold-contact low-level dry switches or equivalent, selected for reliable sub-milliamp operation |
| Mechanical assembly | 1 | Nonconductive covered dock, two answer paddles, matched cassette bodies, captive low-force switch actuators |

The JSON connectivity manifest includes every board component, reference, value, footprint and pin net. It is not a distributor stock claim. Before ordering, confirm body dimensions, pin diameter, resistor lead forming, switch low-level rating and availability. Do not replace signal switches with power contacts that require a higher wetting current.

For a closed contact, the nominal current is 3.3 V divided by 11 kilohms, or 0.30 mA. SENSE sits at approximately 0.30 V through the divider. Four closed contacts draw 1.20 mA and dissipate approximately 3.96 mW across the eight resistors. These are circuit calculations, not measurements of an assembled unit. The Nucleo and ST-LINK dominate system power; no total USB consumption or battery runtime is claimed. The 1 kilohm resistors limit ordinary accidental currents but are not surge suppressors, isolation barriers or permission to connect energized equipment.

Keep prototype contact leads short, initially below 300 mm, and route each signal beside its ground return. There are no filter capacitors or line receivers. Five-sample qualification is not an EMC test. Verify open and closed voltages at every MCU input using the actual board before allowing a participant near the fixture.

## Mechanical cassette design and experimental preparation

Use two otherwise identical nonconductive cassette bodies, nominally 60 by 60 by 8 mm, each with a central 40 by 40 mm tactile patch. Round exposed edges and recess adhesive so the participant cannot distinguish specimens by glue ridges. A covered rail guides each cassette to a positive end stop. The dock actuator must close only after the concealed class actuator has reached its stable position. On extraction, dock should open before the class feature leaves its sensing position. The engine treats dock and class as a bundled code, but good mechanical sequencing still avoids avoidable rejections.

Provide a common underside channel for the class switch. The A cassette clears its lever; the B cassette depresses it. Both bodies need the same external grip shape and comparable mass. Use an opaque shroud so the participant can reach the patch but cannot see the underside, wires or class switch. These dimensions are prototype construction guidance, not a tolerance-qualified CAD enclosure. Adjust travel and lever geometry to the purchased switch's rated overtravel and force, then validate seated-code stability manually. The class mechanism must not create an obvious sound or insertion resistance cue; otherwise the test measures the fixture rather than the label.

Teach the two class names with known examples before collecting evidence. Use an independently prepared randomized presentation schedule; the application does not randomize or conceal the operator's behavior. Include both classes repeatedly and counterbalance order across sessions. Do not use an alternating schedule that lets the participant predict the answer. Export one block before changing patch definitions, operator, instructions or participant; the firmware stores no identifiers that can separate mixed experimental conditions afterward.

Start with an empty dock and both paddles released. Insert one cassette, let it settle, allow touch, then press and release exactly one answer paddle before removing the cassette. Leave the dock empty until READY returns. Do not demand speed: the 60-second limit bounds abandoned observations, not participant performance. Stop immediately if a specimen is irritating, dirty, sharp or uncomfortable. This design is for safe nonclinical materials and consenting adult demonstrations.

## Firmware behavior and decisions

The freestanding C target contains startup vectors, a linker script, clock restoration, GPIO setup, SysTick, independent watchdog support, a nonblocking UART driver, trial logic and a bounded CRC encoder. There is no RTOS, dynamic allocation, vendor HAL or network stack. Reset restores the documented HSI clock tree with bounded readiness loops before initializing the external interface. Input-mode and UART configuration are read back; unexpected settings put the engine in terminal HALT. Clock initialization failure stops before serial setup, so absence of telemetry is itself a fault to investigate.

Three qualifiers run independently: bundled dock/class, answer A, and answer B. Five equal observations qualify a group. This means chatter on a paddle cannot postpone the dock qualifier, while the two identity bits cannot assemble an artificial cassette code from separately delayed bits. WAIT requires an empty dock and released paddles in both raw and qualified observations. READY waits for a qualified dock insertion. Occupied startup does not count as a new presentation; first remove the cassette to establish a baseline.

Insertion captures the class privately and reserves one start. ACTIVE age advances only on consecutive acquired ticks. Any raw opposite paddle activity during the presentation taints the trial, even if too short to qualify. A second qualified answer, answering while removing, class changes under an occupied raw dock, or raw removal followed by reentry also taints the trial. Removal does not complete successfully unless the dock qualifies absent and both raw and qualified paddles are released. A clean presentation without an answer is rejected as unanswered.

A completed presentation increments exactly one matrix cell: AA, AB, BA or BB, with true class first and declared class second. It retains the latest accepted true class, answer and presentation duration. Rejection does not overwrite that last accepted result. The deadline is rejecting: completion at age 60000 is a timeout; completion at 59999 can succeed. Timeout returns to WAIT, requiring a fresh empty baseline rather than counting a held cassette again.

The accounting invariant is starts equals completed plus rejected plus one if ACTIVE. Matrix cells sum to completed. Counters are bounded at one billion rather than silently wrapping. A sampling delta other than one after acquisition begins is a gap: the active presentation is rejected exactly once and the engine halts. HALT retains experimental evidence while timestamps and permitted transport diagnostics may still change. A fresh-looking timestamp in HALT does not mean acquisition continues.

The watchdog is fed after the acquisition service. A stalled loop can reset the MCU, losing volatile history. There is no nonvolatile boot identifier, automatic rollback or signed update system. Some resets are detected by counter or timestamp rollback at the host, but a reset that produces apparently forward values can be indistinguishable. Never interpret a successful reconnect as proof that device history was continuous. Programming uses the onboard ST-LINK and requires physical USB access.

## Protocol and desktop application

An LV1 line is ASCII: the marker `LV1`, comma-separated unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits, and LF. CRC is CCITT-FALSE with polynomial 0x1021 and initial value 0xFFFF, covering the marker and decimal body but not the asterisk or terminator. Fields, in order, are:

```text
sequence now state fault valid starts completed rejected aa ab ba bb
age answer tainted last_truth last_answer last_ms reason tx_errors gaps
```

State is WAIT=0, READY=1, ACTIVE=2 or HALT=3. Answer and retained class use absent=0, A=1, B=2. Reason is none=0, ambiguous=1, unanswered=2, timeout=3, gap=4 or fault=5. Fault bits are configuration=1, watchdog=2, acquisition gap=4 and counter exhaustion=8. `valid` is the accumulated qualified input-bit mask, not the current contact vector. Neither the current dock code nor provisional truth is serialized. LV1 fixes the qualification and deadline policy; incompatible policy changes require a different protocol marker.

The main loop attempts a snapshot every 100 ms. Serial service moves available bytes without blocking acquisition, rejects submissions while busy, and abandons a stuck frame after 100 ms. The receiver resynchronizes on the complete marker. Initial mid-frame fragments on late attachment are counted but tolerated; corruption after the first marker disconnects the application and demands an explicit reconnect. CRC detects accidental corruption, not malicious modification.

Run `python3 app/workbench.py` from this directory on a Linux desktop with Python Tk installed. The Classification screen connects to a chosen serial path, shows acquisition phase and the matrix, and provides disconnect and replay controls. Notebook shows up to 2000 recent accepted snapshots. Policy / Export explains the experimental limits and writes a CSV without overwriting an existing file. File permissions are requested as 0600. Exports include author, provenance, connection state, freshness, parser errors, partial bytes and warning text even when no trial completed.

A successful new connection starts a new local notebook; export old evidence first. Connection failure preserves existing evidence. Opening the same port twice is refused. Disconnect retains rows and origin but clears live status. Replay accepts only regular files up to 2 MiB, rejects empty recordings and truncated tails, and labels results historical. Snapshot history is not a full event log: several trials can finish between periodic records, while cumulative matrix counts remain available. CSV is a report format, not an LV1 replay file.

The host validates field ranges, trial and matrix conservation, monotonic counters, retained-result consistency and necessary sparse transition bounds. These checks cannot reconstruct every hidden contact transition. Freshness requires advancing device time, a live connection, no latched error and no HALT. A reception pause or cumulative host/device discrepancy beyond two seconds requires reconnection; this conservative allowance may also reject long sessions with oscillator drift. No wireless discovery, pairing service, cloud upload or participant name field exists.

## Reproducible build and checks

Dependencies are GNU Make, native GCC with address/undefined sanitizers, Arm GNU bare-metal GCC and binutils, Python 3 with Tk, Xvfb for headless GUI tests, and KiCad 9 with its Python bindings and standard footprints. The companion uses only the Python standard library and has no package-install scripts. `/usr/bin/python3` is used for CAD because distribution KiCad bindings may not be available in another interpreter.

From this directory:

```sh
make -C firmware clean all test fixture
python3 -m py_compile app/model.py app/transport.py app/workbench.py
python3 tests/test_app.py
xvfb-run -a python3 tests/test_gui.py
/usr/bin/python3 tools/check_hardware.py
```

`make -C firmware flash` invokes OpenOCD with ST-LINK and STM32F4 target settings, then requests programming, verification and reset. This command has not been run against hardware. The linker places vectors at flash address 0x08000000 and reserves stack headroom in 96 KiB SRAM. Do not use this binary on a different Nucleo merely because its headers fit.

The validation run used Arm GCC 14.2.1 and KiCad 9.0.2. The target linked with warnings treated as errors, producing 2504 bytes of text, zero initialized data and 788 bytes of static BSS; stack-local engine and frame storage are additional. The seven C implementation files contain 533 nonblank, noncomment lines, excluding tests and headers. The C tests exercise all four-step category-pair words against an independent matrix oracle, ambiguity, raw identity changes, removal races, repeated declarations, occupied startup, counter exhaustion, deadline precedence, timer wrap, framing capacity and peripheral failure paths.

The synthetic fixture passes through the actual C encoder, strict Python parser and real Tk event loop. Its four accepted outcomes fill each matrix cell once; a contradictory fifth presentation is rejected. GUI tests exercise all screens, private export, overwrite refusal, serial PTY input, late attachment, double-connect refusal, hangup and preservation after failed connection. These are synthetic transport and UI checks, not physical USB or participant evidence.

KiCad ERC and DRC report zero violations and zero unconnected items. A fresh exported schematic netlist is compared with actual PCB pads and the manifest: 14 components, 30 connected pins and 10 nets agree, including values and footprint identifiers. The schematic was rendered and visually inspected. Passive-symbol ERC cannot verify the external regulator or determine whether a cable is plugged into the wrong Nucleo header. The CAD generator is reproducible source; the committed files are ready to inspect without regenerating them. The import utility records one-time reuse of repository platform infrastructure and intentionally refuses overwriting existing files.

## Assembly, acceptance and limitations

Inspect the PCB in KiCad before ordering: confirm the closed outline, fabrication rules, drill sizes and library identities against the intended manufacturer. Solder resistors first, then headers, keeping pin-one markings visible. The board has no mounting holes; use nonconductive edge clips or an insulated tray, not screws through arbitrary copper. Mount the Nucleo separately with access to USB and reset. Provide strain relief and prevent participants from touching circuitry.

Before power, continuity-check every signal against the manifest and check for a VCC-to-ground short. Power the Nucleo alone first, confirm its rail, then attach the carrier with USB disconnected. Verify all open and closed input voltages after reconnecting power. Use a logic analyzer to compare raw contact edges, nominal sampling cadence and transmitted timestamps. Confirm the four classification cells with known scripted cassettes before collecting any human observations. Test every cable unplug and intermittent contact deliberately; open class wiring can masquerade as class A, because this binary encoding has no separate wire-integrity channel.

A physical acceptance campaign should compare scripted presentations with independent video or observer annotations, test switch chatter and insertion sequencing, and verify that the hidden class cannot be inferred from noise, friction or appearance. Repeat with swapped patch positions and varied presentation order. Report rejected trials as well as accepted accuracy, and keep denominators by encoded class. Do not infer general population performance from a few workshop demonstrations. No physical results, regulatory compliance, calibrated duration accuracy or participant benefit are claimed by this revision.

## Hardware references

- ST UM1724, STM32 Nucleo-64 boards MB1136: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- Accessible earlier UM1724 mirror, including F401RE connector table and USART bridge configuration: https://electronilab.co/wp-content/uploads/2019/04/STM32-Nucleo-boards-User-Manual_ST.pdf
- STM32F401 reference manual RM0368 defines the peripheral register map used in registers.h and the clock driver.

The F401RE table maps CN8 pins 1–4 to PA0, PA1, PA4 and PB0, and CN6 pin 4 to 3.3 V. Standard VCP routing uses SB13/SB14 connected and SB62/SB63 disconnected; inspect the actual board rather than changing bridges blindly. The direct ST PDF endpoint did not respond during this run; connector and VCP details were cross-checked against indexed manual content from the mirror. Use the current manufacturer documents and actual revision markings during physical bring-up.
