# Pipette Path — two-stop plunger and source-declaration training recorder

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software and locally authored hardware design sources: MIT; see LICENSE. Installed KiCad footprint material retains its upstream licensing.

Revision A is an engineering prototype. The supplied PCB is a routed passive contact carrier for a separately purchased NUCLEO-F401RE. It is not a standalone processor board. Target compilation, software tests and CAD connectivity checks are reproducible. Fabrication, target flashing, mechanical fitting, electrical qualification and training effectiveness have not been demonstrated. This device is neither a pipette calibrator nor a medical instrument. It does not measure volume, verify sterility, detect liquid, or certify operator competence.

## Purpose and audience

Pipette Path makes the order of a manual pipette's two plunger stops visible during a deliberately slow training exercise. Three removable position witnesses report home, the first-stop threshold and the second-stop threshold. A separate normally open foot pedal reports the learner's declaration that the tip is immersed in the source vessel. The recorder checks a bounded forward-pipetting sequence and preserves completion and abandonment counts. A local desktop application shows the expected next action, the qualified contact states, elapsed cycle time, and the reason for the most recent abandonment.

The intended first users are instructors running introductory biotechnology practicals, school laboratory technicians preparing water-only demonstrations, and community biology educators teaching adults unfamiliar with the difference between aspiration and blowout. The current workaround is close observation of a learner's thumb, repeated verbal coaching, and sometimes reviewing a video. A short video can show more information than this recorder but may obscure the plunger, capture faces, and require time to review. The recorder offers a small, inspectable teaching aid rather than a camera-based score for individual learners.

The hardware is necessary because a phone button cannot independently observe the moving plunger. However, the immersion input is intentionally not independent: it is a human declaration. This distinction is central to the design. A learner can press the pedal at the wrong time, and a perfect contact sequence can occur with no liquid or no tip. The instrument should therefore be used with an instructor and a conventional demonstration of correct technique, never as a substitute for one. Do not attach it to a clinical or production pipette whose calibration, cleaning procedure or warranty would be affected.

The measurable pilot objective is to identify deliberately scripted second-stop-before-aspiration and missing-blowout sequences in a water-only demonstration, while retaining every valid scripted cycle. A proposed acceptance experiment is twenty correct cycles and twenty deliberately incorrect cycles independently annotated by an instructor. The target is no misclassification within that controlled script, not an advertised accuracy figure. Compare instructor review time with and without the display; any time saving must be measured rather than assumed. Hardware budget target is USD 60 excluding the pipette and host computer. A USD 180 kit would need to justify its price through reliable fixture fitting and saved teaching time; willingness to pay has not been tested.

## Originality, alternatives and repository comparison

The repository's directories and README summaries were inventoried before this device was selected. FoldTrace is the closest sequence-checking neighbour: it observes four separate carton-folding leaves in a prescribed order. Scoop Window observes paired utensil absence and opposite-bin opening, and Chord Window compares keyboard-like contact timing. Pipette Path instead follows one returning actuator through different meanings of the same first-stop threshold, conditioned on a separate source declaration. An explicit post-aspiration source-removed phase prevents re-immersion at home from being accepted as a valid continuation. This context-dependent interpretation is the device-specific contribution.

Electronic pipettes, pipetting lessons and automated liquid handlers already exist. The CDC's forward-pipetting demonstration and Thermo Fisher's technique guide describe the familiar first-stop aspiration and second-stop blowout distinction. Pipette Path does not claim to invent that technique. A limited web search found those sources and electronic-pipette alternatives; it does not establish worldwide originality, absence of a similar product, or patentability. The bounded proposal is an open, removable, receive-only teaching fixture for an existing manual mechanism, with explicit uncertainty and no liquid-volume claims.

The STM32 register drivers, startup, linker layout, bounded UART transmitter, passive carrier topology and Linux serial/Tk shell reuse jayis1's existing Scoop Window platform. The plunger state machine, PP1 record, phase/contact validation and training presentation are specific to this project. Reusing an inspected electrical foundation is preferable to pretending that a new arrangement of pull-up resistors is an invention. A commercial electronic pipette may be the better choice when the requirement is repeatable dosing rather than learning the feel of a manual plunger.

## Mechanical fixture design and constraints

Use a dedicated teaching pipette with accessible plunger movement and a noncritical, externally clamped reference surface. Do not drill the pipette, glue parts to its internal shaft, obstruct its ejector, or tighten a clamp around a calibration adjustment. A split polymer collar with a soft replaceable liner supports an external parallel guide. A lightweight follower loosely references the underside of the thumb button. The guide carries three adjustable switch brackets. The follower must disengage freely if snagged; it must not prevent the plunger spring from returning home.

For a first fixture, reserve a 60 mm long external guide with 20 mm axial adjustment slots for the switch brackets. Use M3 fasteners and rounded polymer covers. These are fixture design allowances, not asserted dimensions of a particular pipette. Measure the selected pipette at its chosen volume before manufacturing its collar. An instructor must reset the bracket positions whenever volume adjustment changes the first-stop travel. Record pipette model, set volume, measured home-to-first and first-to-second travel, bracket positions and switch hysteresis in the experimental notebook. There is no universal mechanical fit claim.

HOME is a normally open low-level switch operated only at the fully released position. FIRST is operated by a ramp-and-flat cam on the follower: it closes at the first stop and remains closed through travel to the second stop. SECOND closes at the second stop. Use a flat cam region rather than forcing switch plungers through their maximum overtravel. Suitable starting parts are three Omron D2F-01L low-level lever switches, subject to checking actual operating force, overtravel and mounting against the manufacturer's drawing. Their force and friction can change pipette feel. That disturbance must be measured with and without the fixture before training use; a low-force noncontact revision may be necessary.

The supported qualified plunger codes are zero in transit, HOME alone at rest, FIRST alone at the first threshold, and FIRST plus SECOND at blowout. HOME must never overlap either threshold. SECOND without FIRST is contradictory. Adjust the brackets to provide positive separation between home and first, and enough first-only travel that the qualifier can observe it during slow operation. Hold meaningful teaching positions for at least 20 ms during commissioning. Fast motions can skip the first-only region and are intentionally not reconstructed from later endpoints.

The fourth input is a protected, normally open foot pedal with voltage-free contacts suitable for low current. Label it SOURCE IMMERSION DECLARATION. Hold it only while the tip is in the source vessel; release it after withdrawing from that source. It does not describe contact with the receiving vessel. This revision does not monitor destination withdrawal while holding blowout, immersion depth, plunger speed, tip angle, prewetting repetitions, bubbles, tip ejection or liquid retention. Its successful result covers only the stated sequence subset.

Keep all electronics and switches above and away from the water tray. Use dry demonstrations first, then clean water only under supervision. There is no ingress protection or chemical compatibility claim. Secure paired contact/ground leads, initially no longer than 0.5 m, away from fingers and the pipette tip. Place the carrier in an approximately 100 by 90 mm insulated edge-retention enclosure; the 80 by 70 mm PCB has no mounting holes. House the Nucleo separately or provide a larger enclosure with checked clearance and strain relief. Never suspend either board by its wires.

## Hardware specifications and exact interface

| Function | Implementation | Wiring |
|---|---|---|
| Processor module | NUCLEO-F401RE, MB1136, STM32F401RET6 | Separate purchased assembly |
| Clock and memory | HSI 16 MHz, Cortex-M4, 512 KiB flash, 96 KiB SRAM | No external clock dependency |
| HOME witness | J3.1 HOME, J3.2 ground | SENSE0 through J2.1 to CN8.1 A0 PA0 |
| FIRST threshold | J4.1 FIRST, J4.2 ground | SENSE1 through J2.2 to CN8.2 A1 PA1 |
| SECOND threshold | J5.1 SECOND, J5.2 ground | SENSE2 through J2.3 to CN8.3 A2 PA4 |
| SOURCE declaration | J6.1 SOURCE, J6.2 ground | SENSE3 through J2.4 to CN8.4 A3 PB0 |
| Carrier supply | J1.1 VCC, J1.2 ground | CN6.4 3V3 and CN6.6 GND |
| Communication | USART2 TX PA2 alternate function 7 | ST-LINK virtual COM, 115200 baud 8N1 |
| Sampling | Nominal one millisecond | Five consecutive observations qualify each contact |
| PCB | Two layers, 1.6 mm FR-4, 80 by 70 mm | 0.3 mm tracks, 0.2 mm clearance rule |
| Programming | Onboard ST-LINK SWD | OpenOCD stm32f4x target |

Each SENSE node connects to 3V3 through 10 kΩ and to its contact through 1 kΩ. The contact's other terminal is ground. A closure produces a logical one after active-low conversion. At nominal 3.3 V, a closed path draws 0.30 mA and the input node sits at 0.30 V. Four closed paths draw 1.20 mA, although that is not a valid normal plunger code. These are calculated resistor-network values, not measurements. Select contacts rated for low-level signals; a large oxidized mains switch is unsuitable.

The carrier has no regulator, MCU, USB controller, debugger or active sensor hidden in its description. Those functions belong to the purchased module. Its schematic has actual authored passive symbols, linked footprints, reference designators and connected net labels. The PCB has corresponding pad nets, routed copper and a closed board outline. All fourteen components and thirty pins are cross-checked against the exported schematic XML and the connectivity manifest. The passive ERC check cannot validate the Nucleo's external supply or incorrect jumper wiring.

ST UM1724 table 16 maps CN8 and CN6 as listed above. Its USART section specifies the default ST-LINK bridge with SB13 and SB14 ON and SB62 and SB63 OFF. Inspect the actual board revision before wiring, and do not power VCC from 5 V. The firmware configures PA0, PA1, PA4 and PB0 as inputs with no internal pulls, and configures PA2 as USART2 TX. The UART receiver is not enabled.

### Bill of materials and power planning

| Reference | Quantity | Part or procurement specification |
|---|---:|---|
| External MCU | 1 | ST NUCLEO-F401RE |
| R1 R3 R5 R7 | 4 | 10 kΩ, 1%, axial resistor, 7.62 mm pitch |
| R2 R4 R6 R8 | 4 | 1 kΩ, 1%, axial resistor, 7.62 mm pitch |
| J1 J3 J4 J5 J6 | 5 | 1x2, 2.54 mm straight through-hole header |
| J2 | 1 | 1x4, 2.54 mm straight through-hole header |
| Position witnesses | 3 | D2F-01L low-level lever switches; qualify fitted force |
| Foot pedal | 1 | Normally open, voltage-free, low-level contact |
| Fixture | 1 | Insulated collar, follower, cam, guide and guarded brackets |
| Enclosures and leads | 1 set | Insulated edge retention, strain relief, labelled pairs |

Allow USD 20 for the MCU module, USD 10 for carrier PCB/passives/headers, USD 15 for switches and pedal, and USD 15 for mechanical parts and leads. These are budget allocations, not supplier quotations, stock checks or a purchasing recommendation. The pipette and computer are excluded. Obtain current quotations before deciding whether to build a batch.

Power comes only through the Nucleo USB arrangement. Budget an unmeasured 150 mA at 5 V, or 0.75 W, for the development board and carrier together. The extra closed-contact current is small relative to that allowance. There is no battery, charger, battery-life estimate or implemented low-power sleep mode. Continuous sampling is intentional. Unplugging resets all volatile evidence. There is no isolated input, reverse-polarity circuit or dedicated cable ESD protection on the carrier. Its series resistors are not safety barriers. Connect only disconnected, voltage-free contacts; USB shares ground with the host.

## Architecture

```text
Home switch -------- J3 -- 1k -- SENSE0 -- PA0
First threshold ---- J4 -- 1k -- SENSE1 -- PA1
Second threshold --- J5 -- 1k -- SENSE2 -- PA4
Source pedal ------- J6 -- 1k -- SENSE3 -- PB0
                          each SENSE -- 10k -- 3V3
                          contact returns ---- GND
                                                |
USB --> NUCLEO-F401RE --> 1 ms samples --> independent qualifiers
                                                |
                                 contextual pipetting state machine
                                                |
                               counters / current phase / last result
                                                |
                            PP1 + CRC16 --> USART2 --> ST-LINK USB
                                                |
                             Linux Tk: Connection / Live / History
```

The engine owns all sequence decisions. The application never invents missing intermediate steps from the ten-hertz snapshots. There are no actuators, remote commands, wireless radios or cloud accounts. A disconnected application does not stop acquisition, but the module retains only cumulative totals and the latest result, not a full trial log. Earlier individual results can be overwritten before a host receives them. The history screen is a bounded snapshot history, not a guaranteed row for every trial.

## Firmware state and evidence contract

WAIT accepts only qualified dry home as a baseline. READY starts a trial when the plunger leaves home. If the source pedal is asserted while still at home before the trial, readiness is removed without inventing a start. APPROACH accepts transit followed by FIRST while the pedal remains released. The learner must reach FIRST before declaring source immersion; simultaneous first arrival and pedal assertion are rejected conservatively. IMMERSED permits release through transit to HOME while the source pedal remains held. Reaching SECOND instead is a wrong-order abandonment.

ASPIRATED requires home to remain observed until the pedal is released. Source removal and plunger departure in the same qualified vector are not ordered evidence and are rejected. The resulting REMOVED phase remembers that removal has already happened: a later pedal assertion abandons the trial even if the plunger is still home. From REMOVED, pressing to FIRST starts DISPENSE; SECOND without an observed FIRST is rejected. DISPENSE accepts the second threshold as BLOWOUT. BLOWOUT permits return through FIRST and transit to dry HOME, completing the cycle. This does not assert that liquid reached a destination or that the destination was left before release.

The state numbers are WAIT 0, READY 1, APPROACH 2, FIRST 3, IMMERSED 4, ASPIRATED 5, DISPENSE 6, BLOWOUT 7 and REMOVED 8. Numbers are protocol identifiers rather than a numeric ordering of progress. Position contradictions abandon active trials. The last reason is zero for completion, one for stop order, two for source ordering, three for contradictory contacts, four for acquisition loss, five for the thirty-second limit, and six for a hardware fault. The latest result is retained until the next disposition.

Every trial obeys starts = completed + aborted + active. Completion means the defined contact/declaration sequence was observed, not a successful pipetting operation. Aborted includes incorrect sequences and unknown trials lost to acquisition gaps. An active elapsed counter advances on each consecutive one-millisecond step; at 30000 ms the timeout takes precedence over completion. Last duration is bounded by that limit. It is a qualified-state cycle duration, not aspiration speed or liquid flow rate.

Each input has an independent five-observation qualifier. Rejected short glitches do not update the qualified state. Existing qualified states remain in effect while replacements are being qualified; sub-sample movement is unobserved. A sampling delta other than one millisecond increments the gap diagnostic, abandons active correlation, clears qualification and requires a new dry-home baseline. Historical counts remain. Normal unsigned timestamp wrap is supported. Startup in mid-stroke never fabricates an earlier preparation step.

Sample updates occur on a staged engine copy. If a counter cannot increment, the speculative update is discarded, the previous active trial is abandoned once, and overflow fault 4 is latched. Configuration fault 1 and watchdog-start fault 2 are also terminal. Faulted evidence is frozen while uptime and UART diagnostics may continue. The application labels the acquisition halted even when telemetry is fresh. Register initialization has bounded waits, the independent watchdog is fed only after sampling progresses, and the UART driver is nonblocking with a one-hundred-millisecond transmit timeout. There is no heap or flash persistence.

## Companion application and serial protocol

The appropriate companion for this USB bench fixture is a dependency-light Linux desktop application rather than a mobile Bluetooth application. Run `python3 app/pipette.py` from the device folder with Python 3 and Tk installed. On Debian, the needed GUI package is `python3-tk`. Access the serial device through the normal operating-system permissions; do not run the application as root merely to open a port. No pip packages or network service are required.

Connection selects a local serial path, opens it nonblocking, configures 115200 baud and flushes old input. A second connection is refused until the current one is closed. Disconnect retains evidence and provenance. Live shows qualified witnesses, the current instruction, totals, latest duration and reason, fault bits and gaps. History retains at most 2000 accepted snapshots. Replay requires a regular file no larger than two megabytes and rejects malformed framing, an empty recording or a truncated tail. CSV export exclusively creates a private file and refuses overwrite; it includes source, connection state, freshness, parser diagnostics and row ages even when no trial has completed.

Every ASCII line starts with `PP1,` and contains fifteen unsigned decimal values in this order: qualify, timeout, now, levels, qualified, phase, starts, completed, aborted, elapsed, last_ms, last_reason, gaps, tx_errors, faults. A star, four uppercase hexadecimal CRC digits and LF terminate the line. CRC-16/CCITT-FALSE covers the body before the star, with initial value 0xffff and polynomial 0x1021. It detects accidental damage, not forgery. The qualification and timeout constants repeat so a late connection need not catch a boot greeting. There is no host command protocol.

The parser bounds each frame, resynchronizes on the complete marker following a truncated transmission, checks CRC and rejects impossible phase/contact combinations and broken accounting. The model also detects counter rollback, contradictory duplicate timestamps, disappearing sticky faults and changes to retained results without a disposition. Freshness requires two advancing records and recent progress, not merely receipt of identical data. A host pause or cumulative timing mismatch blocks the session and requires reconnect with input flushing. The allowed host/device timing difference starts at half a second and grows by two percent of host elapsed time only up to an additional second. Uncalibrated clock drift can therefore eventually require a conservative reconnect.

There is no persistent boot identifier. Some resets can be detected from timestamp or counter regression, but a reset followed by sufficiently advanced new counters may be indistinguishable. Preserve exported sessions separately and record physical resets. The application does not automatically erase evidence to conceal ambiguity. Local USB ownership is the security boundary; anyone able to impersonate the serial stream can forge records. Do not use these logs for regulated audit trails, employee ranking, access control or safety decisions.

## Build, verification and commissioning

Build with `make -C firmware all`. The real target is Cortex-M4 Thumb, linked at the STM32 flash address with a vector table, data initialization, BSS clearing and a stack reservation. The build produces an ELF, binary and linker map under ignored `firmware/build/`. Run `make -C firmware test` for the production engine and simulated-register driver tests using address and undefined-behaviour sanitizers. Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_app.py' -v` for protocol and real Tk/PTY workflows. Synthetic frames come from the C encoder; they are not physical measurements.

For CAD, use KiCad 9 and its installed footprint libraries. `/usr/bin/python3 tools/generate_hardware.py` regenerates the authored carrier project. `/usr/bin/python3 tools/check_hardware.py` runs ERC, DRC, fresh schematic netlist export and exact pad/pin/value/footprint comparison. The checker requires zero violations and zero unconnected items, without exclusion suppressions. Render the schematic and inspect labels as well as running automated checks. Native files are `kicad/device.kicad_sch`, `device.kicad_pcb` and `device.kicad_pro`.

Before any physical trial, inspect solder joints and verify resistance and ground mapping with power disconnected. Apply USB power with the sensor harness disconnected, verify 3V3 polarity, and measure each sense input before connecting the Nucleo signals. Close one contact at a time and verify the correct live bit. Then verify home, transit, first and second combinations using a loose demonstrator rather than a valuable pipette. Do not proceed if the follower binds, switch forces alter return, or the contact thresholds cannot be separated reliably.

Program the target using the documented OpenOCD flash target and a real ST-LINK connection after the unpowered resistance checks and powered voltage checks, before attempting the live-bit checks above. No target has been flashed in this delivery. Compare qualified timing with a logic analyzer, test USB disconnect and watchdog recovery, measure total current, and repeat fitting over the intended volume range. Finally perform the instructor-annotated water-only trial and report both false rejections and missed mistakes. A zero-error software test run cannot replace these mechanical and educational checks.

## References

- ST UM1724, Nucleo-64 MB1136 user manual, table 16 and USART bridge section: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual, GPIO/RCC/USART/IWDG register definitions: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- CDC, Forward Pipetting Technique with a Manual Micropipette: https://reach.cdc.gov/sites/default/files/video-transcripts/Forward%20Pipetting%20Technique%20with%20a%20Manual%20Micropipette_script.pdf
- Thermo Fisher, 10 Steps to Improve Pipetting Accuracy: https://www.thermofisher.com/us/en/home/life-science/lab-plasticware-supplies/lab-plasticware-supplies-learning-center/lab-plasticware-supplies-resource-library/fundamentals-of-pipetting/proper-pipetting-techniques/10-steps-to-improve-pipetting-accuracy.html

The manufacturer manual's connector and bridge information was checked through indexed document text after direct PDF retrieval failed. That is a documentation check, not inspection of an actual board. Consult the manual for the purchased board revision before assembly.
