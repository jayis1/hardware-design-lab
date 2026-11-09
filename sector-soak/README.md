# Sector Soak — absolute-sector pointing-dwell collar for slow rotary sprinklers

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and design sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. The common acquisition platform is reused from jayis1's Pole Bridge design at commit 80020a7; the Gray-sector engine, interface model, screens, tests and cam templates are specific to this device.

Revision A is an unbuilt engineering prototype. Target firmware compilation, simulated firmware/application behavior and native CAD consistency can be checked from this directory. They do not establish that a real sprinkler tolerates the collar, that contacts remain dry, or that recorded angle predicts irrigation coverage. The separate purchased NUCLEO-F401RE is a required part of the instrument, not a component omitted accidentally from its custom PCB.

## Purpose and practical utility

A slow rotating sprinkler can hesitate in one direction without stopping completely. A gardener watching briefly may see movement and assume the distribution is reasonable. An ordinary revolution counter also misses the distinction between uniform rotation and a head that spends most of its time pointing into one narrow region. Sector Soak records nominal milliseconds spent in sixteen absolute pointing sectors, alongside the longest uninterrupted observed stay in each sector. The comparison is intended to identify a sticking sector worth inspecting, not to calculate delivered water volume.

The proposed instrument has a removable mechanical collar on a slow rotary axis. Four reflected-Gray cam tracks operate electrically isolated low-level contacts on a stationary reader bracket. The controller samples the complete contact word, decodes an absolute sector, and credits time only when successive endpoints are consistently qualified. Broad pauses therefore become visible without requiring a perfect uninterrupted revolution or a known starting index. Forward and backward adjacent-sector transitions are retained separately, and a skipped sector is explicitly recorded rather than being filled with invented intermediate motion.

The intended audience is a community-garden maintenance group, irrigation repair workshop or school investigating simple water distribution mechanisms. A practical workflow is to record a head before cleaning, repeat with identical collar alignment afterwards, and compare the sector shares and longest stays. Retain the duration and unknown-time denominator as well as the plot. A shorter recording cannot be compared honestly by raw milliseconds alone, and an increasing unknown fraction may mean the reader has deteriorated rather than the sprinkler improved.

This instrument never opens a water valve, energizes a pump, controls a motor or provides a protective interlock. It has no radio, cloud account, camera or remote command receiver. Its contribution is a local, inspectable evidence record of coarse pointing behavior. Actual watering also depends on flow, pressure, jet breakup, throw distance, wind and obstruction. Use collection cups or another appropriate irrigation test for water-distribution claims. A stationary head with no water flowing produces the same angle record as a stationary head with water flowing.

## Originality and alternatives

The repository's device directories and README purpose summaries were inspected after pulling the configured default branch. Cam Dwell Map is the closest angular instrument: it records a follower threshold against incremental encoder position during a complete indexed turn. Sector Soak instead assigns time to absolute Gray-code sectors, permits reversals, retains dwell across partial arcs and exposes nonadjacent changes without inventing a revolution count. Pattern Sled learns a three-track program strip against a fourth reading clock; it neither decodes an absolute angle nor measures a time-weighted pointing distribution. Existing contact-transition notebooks do not provide this specific absolute-sector denominator.

Absolute encoders, Gray codes, sprinkler diagnostics and dwell histograms are established ideas. This proposal does not claim to have invented any of those principles or to establish worldwide novelty, patentability or freedom to operate. No exhaustive external patent or commercial-product survey was performed. The proposed differentiator is the removable, inspectable four-track collar plus a conservative dwell/unknown ledger and local before/after workflow. It is new within the inspected repository, not proven unprecedented globally.

A phone video is a strong alternative for one repair and provides richer visual evidence. An optical or magnetic commercial absolute encoder is preferable where its shaft mounting and electrical output can be accommodated. A catch-cup survey measures the outcome this instrument deliberately does not measure. Build this device only if repeatedly transcribing direction and pauses is expensive enough to justify the fixture. A decisive rejection criterion is excessive added drag: if fitting the collar changes the motion under study, the proposed measurement is not useful even though the software behaves correctly.

## Hardware specifications and architecture

| Subsystem | Revision A contract |
|---|---|
| Controller | ST NUCLEO-F401RE, MB1136 platform, STM32F401RET6 |
| CPU / storage | Cortex-M4, 512 KiB flash, 96 KiB SRAM; no persistent evidence store |
| Clock | Internal HSI restored to nominal 16 MHz before peripheral initialization |
| Acquisition | Four low-active digital inputs sampled once per nominal millisecond |
| Position sensing | Four mechanically actuated voltage-free contacts, reflected Gray code |
| Suggested contacts | Four Omron D2F-01L low-level lever switches, isolated COM/NO pairs |
| Carrier PCB | 80 × 70 mm, two copper layers, 1.6 mm FR-4, through-hole components |
| Input network | Per channel: 10k pullup to 3.3 V, 1k series resistor to contact |
| Host transport | PA2 AF7 USART2 TX, ST-LINK virtual COM, 115200 baud, 8N1 |
| Power | USB to Nucleo; its regulated 3.3 V supplies the passive carrier |
| Companion | Linux desktop, Python 3 standard library and Tk |
| Energy / outputs | No battery, charger, actuator, relay, radio or hazardous voltage |

    sprinkler's slow rotary axis
                |
       removable four-plate Gray collar
                |
      stationary isolated contact followers
        GRAY0   GRAY1   GRAY2   GRAY3
           \      |      |      /
           passive bias / series carrier
                        |
              PA0 PA1 PA4 PB0
                  NUCLEO-F401RE
             whole-word qualification
             sector dwell / unknown time
                        |
              USART2 TX -> ST-LINK USB
                        |
             Linux sector table / trace
                  private CSV export

The custom PCB is intentionally a passive interface, not a new microcontroller development board. The purchased Nucleo contains the processor, regulator, debugger, USB bridge and their supporting parts. Short jumpers connect the carrier to that board. The carrier is not an Arduino shield with mechanically matching header spacing. Its KiCad schematic contains actual authored symbols, footprint identifiers and labeled electrical connections. The PCB contains matching routed nets and an outline. The mechanical reader and development board are external assemblies identified in this document.

## Wiring, power and component contract

J1 pin 1 connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. The analog header naming does not imply ADC operation; all four are digital inputs without internal pulls.

| Input | Bit weight | MCU | Contact connector | Resistors |
|---|---|---|---|---|
| GRAY0 | 1 | PA0 | J3: 1 signal, 2 ground | R1 10k pullup, R2 1k series |
| GRAY1 | 2 | PA1 | J4: 1 signal, 2 ground | R3 10k pullup, R4 1k series |
| GRAY2 | 4 | PA4 | J5: 1 signal, 2 ground | R5 10k pullup, R6 1k series |
| GRAY3 | 8 | PB0 | J6: 1 signal, 2 ground | R7 10k pullup, R8 1k series |

A mechanically raised cam track means the corresponding NO contact closes, so the logical bit is one. The GPIO hardware voltage is then low; board_closed performs the inversion exactly once. Each contact must belong solely to the instrument. Remove any original electrical wiring, leave NC insulated and do not connect installed controller outputs, powered sensors or valve coils. This board has no galvanic isolation, surge arrestor, reverse-polarity protection or outdoor ESD qualification. A 1k resistor is not protection against mains or a miswired 24 V irrigation system.

The carrier BOM is four 10k and four 1k axial resistors, five two-pin 2.54 mm headers and one four-pin header. The electrical manifest in kicad/connectivity.json records each reference, value, footprint and pin net. Use resistors rated at least 0.125 W and nominal one-percent tolerance. Add one NUCLEO-F401RE, a suitable USB cable, four low-level switches, short signal/return pairs, a nonconductive enclosure, cam plates, spacers and a stationary bracket. The native PCB does not distribute sprinkler power.

For a closed contact the nominal current is 3.3 V divided by 11k, approximately 0.30 mA, and the MCU-side low is approximately 0.30 V. Four simultaneous closures therefore draw approximately 1.20 mA from the 3.3 V rail. Verify these levels and noise margins on the assembled instrument. The development board and debugger dominate USB consumption; reserve a conservative 500 mA USB supply allowance and measure actual current before selecting portable power. Do not claim an operating time from an unspecified battery pack. Low wetting current may fail on oxidized contacts, so use the specified low-level class and inspect repeatability.

The virtual COM link requires the applicable Nucleo solder bridges. The existing MB1136 platform documentation uses SB13/SB14 connected and SB62/SB63 open. Check the purchased revision against ST's UM1724 before wiring. Register definitions follow RM0368. Reference documents are https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf and https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf . This run reuses the already documented platform rather than asserting a new physical pinout audit.

## Mechanical sensing and assembly limits

mechanical/cam-plates.svg supplies four one-to-one cutting templates generated by tools/generate_mechanics.py. Each plate has a 50 mm base radius and 54 mm raised radius, a 12 mm central bore and three 3.2 mm mounting holes on an 18 mm radius at unequal angles. Use nominal 2 mm nonconductive sheet and 12 mm axial plate pitch. The unequal mounting pattern preserves alignment through stacking. The blue radius marks the beginning of sector zero when viewed from the printed side; increasing sector number proceeds clockwise. GRAY0 is the least significant plate. All text and blue guides are annotation, not cutting operations.

Use a separate split collar or supported shaft adapter sized to the actual sprinkler axis. A universal clamp cannot be dimensioned honestly without that axis specification. The template bore is a reference shaft size, not a promise of compatibility. Keep the assembly balanced, securely retained and clear of the jet and existing adjustment stops. The 108 mm maximum cam diameter and stacked follower bracket are bulky; this revision is for an accessible bench-scale head, not an unobtrusive permanent garden accessory.

Do not drive a microswitch directly through the full four-millimeter cam rise. Provide a pivoted follower with approximately 40 mm input arm and 5 mm switch arm, adjustable switch position and a mechanical overtravel stop. That nominal ratio reduces the displacement at the switch, but real lever geometry and switch travel must be checked on the actual parts. Round the cam transitions and add compliant following force. The sharp template steps encode boundaries; they are not finished load-bearing follower surfaces. Reject an assembly that jams or overtravels at any boundary.

First turn the isolated collar by hand with water off. Verify all sixteen codes at sector centers and the wrap from fifteen to zero. A displayed code does not establish correct wiring: swapping tracks still produces legal values. Mark physical orientation and check the full ordered sequence. Compare free-motion behavior with and without the collar. The sensing torque, mechanical hysteresis and rounding width have not been measured. These are mandatory prototype experiments, not small production details to ignore.

Keep the USB board, computer and carrier dry and away from spray. No enclosure ingress rating is claimed. Dry bench trials are the initial acceptance stage; wet trials require a suitable independently reviewed protective arrangement, drainage, strain relief and electrical safety precautions. Do not bring a mains-powered bench supply into the spray zone. Do not leave the prototype unattended on operating irrigation equipment. If the collar cannot be protected without altering the sprinkler's motion, abandon this approach and use remote video or a noncontact sensor instead.

## Firmware measurement contract

The engine decodes reflected Gray code g = n XOR (n shifted right by one), for sector n from zero to fifteen. Five consecutive equal sampled words qualify a position. Qualification applies to the whole word rather than filtering bits independently; independent filters can construct a word that was never actually sampled. There is no extra index pulse and no automatic homing. The physical collar alignment defines zero. All sixteen bit patterns are legal, including all-open zero, so unplugging a cable or sticking one contact cannot reliably be distinguished from a real position. Always perform the mechanical code walk before relying on a session.

The first acquired sample establishes no elapsed interval. Each later acquired interval belongs to exactly one sector dwell bin or to unknown time. A sector earns one interval only if both endpoints are clean: the same raw and qualified word has completed the qualification window, and its absolute sector has not changed. The first qualified endpoint itself does not earn preceding time. At a transition, raw glitch or return from chatter, uncertain intervals remain unknown. The sum of all sixteen dwell bins plus unknown equals sequence minus one after acquisition begins.

The longest array stores the maximum continuous credited run for each sector. A rejected short raw pulse still splits that run because the position was not cleanly known throughout the interval. This matters when diagnosing sticking: software debounce must not turn a noisy signal into a falsely uninterrupted hold. Dwell is a conservative digital observation, not exact mechanical residence. Sector width is nominally 22.5 degrees, while boundary precision depends on the physical reader. Never report subdegree accuracy from this design.

A new qualified sector adjacent in increasing code order increments forward; adjacent in decreasing code order increments backward, including the circular zero/fifteen boundary. These are code directions, not an unconditional claim about physical shaft rotation: at a stationary reader, rotating the printed plate clockwise presents sectors in decreasing order. Every other changed sector increments skipped. A skipped observation clears the direction anchor rather than inventing which way the collar traveled. A later adjacent step sets a new anchor. Reversals count changes between retained adjacent-step directions; they do not count full mechanical reversals hidden between snapshots or complete revolutions. A held sector does not clear the direction anchor. Dwell continues after a skipped position because the new absolute code is usable even when the traversed path is not.

Any acquisition interval other than exactly one nominal millisecond causes a terminal gap fault. Measurement counters and position evidence then freeze; telemetry wall time can continue advancing. Configuration or watchdog setup failure also enters HALT before measurement. Fault bits are configuration=1, watchdog=2, gap=4 and counter exhaustion=8. A reset is needed to start a fresh measurement. The one-billion-sample limit bounds all evidence counters and stops before mutation, so overflow cannot partially credit a histogram. Transmission errors are separately counted and saturate into a counter fault if exhausted.

The board driver restores the HSI clock with bounded waits, configures inputs and transmit-only USART2, and uses SysTick for nominal milliseconds. The independent watchdog is fed by the acquisition loop. Its nominal reload is not a calibrated timing guarantee because the low-speed oscillator varies. A watchdog reset or brownout destroys volatile history; it never restores a previous measurement. There is no nonvolatile journal, boot identifier or authorized remote-reset command. The host blocks detectable counter/time regressions, but some resets can resemble forward traffic. Preserve that limitation when interpreting continuity.

## Protocol and companion application

SS1 is a newline-delimited, receive-only status protocol, emitted approximately every 100 ms when the transmitter is free. The frame is SS1 followed by comma-separated unsigned decimal integers, an asterisk, four uppercase hexadecimal CRC digits and a newline. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial 0xffff over the body before the asterisk. It detects ordinary transmission corruption; it does not authenticate a device or protect against malicious forgery. There are no credentials or incoming commands.

The fixed field order is sequence, now, state, fault, raw, qualified, valid, sector, unknown, forward, backward, skipped, reversals, direction, run, tx_errors, gaps, then dwell0 through dwell15, then longest0 through longest15. State values are WAIT=0, TRACK=1 and HALT=2; direction is none=0, forward=1, backward=2. Sequence counts acquired samples, not transmitted packets. The difference between successive sequence values is therefore not a packet-loss count. now is a wrapping 32-bit millisecond clock. Quantities other than now have the documented bounded ranges.

The desktop has Sector dwell, Trace, and Wiring and limits screens. Enter the Linux serial path and connect. A second connect is refused until disconnect, preserving ownership of terminal settings. Bad paths, denied permission and nonterminal files surface errors without discarding a previously loaded replay. Disconnect retains the evidence and its live/replay provenance. The app restores serial attributes on normal close and failed setup; actual cable removal is detected with poll hangup/error as well as read failures.

Sector shares divide each dwell bin by total credited dwell, excluding unknown time; unknown remains prominent in the summary. Thus the percentages are not fractions of all wall time and cannot be labeled watering uniformity. Longest runs and raw counts remain available for inspection. Only the latest 512 snapshots are retained; the cumulative device bins cover earlier acquired time, but the GUI is not an unlimited raw event archive. Export creates a new owner-only CSV and refuses overwrite. Even an empty session exports metadata including author, provenance, connection state, freshness, parser errors and warning.

Replay accepts regular files no larger than two MiB, rejects malformed or truncated tails and validates every snapshot. Live framing can discard and count incomplete fragments until a full SS1 marker returns. Snapshot checks enforce the interval partition, Gray mapping, counter bounds, movement rates and maxima. Between sparse snapshots, the model rejects rollback, frozen-history mutation, impossible single moves and dwell transfer without movement. These are necessary constraints, not a proof that every hidden physical trajectory occurred. A forged but self-consistent stream remains possible.

Freshness requires two advancing accepted snapshots, a connected live source, a tracking state and recent progress. Duplicate frames do not refresh progress. A receive pause over two seconds or cumulative host/device elapsed disagreement over two seconds blocks the session until reconnect. This intentionally conservative limit can reject a long run with ordinary oscillator drift. A recently received record is not cryptographic evidence of recent physical acquisition, especially on first attachment. Replay is always marked not live.

## Build, tests and reproducibility

Install an ARM bare-metal GCC toolchain, host GCC, GNU Make, Python 3 with Tk, Xvfb and KiCad 9 with its Python pcbnew bindings. No pip packages, JavaScript dependencies or lifecycle installation scripts are needed. On this environment the system Python provides pcbnew and Tk; use /usr/bin/python3 for the documented checks rather than an unrelated virtual environment.

From this device directory, run `make -C firmware clean all test fixture`. The target build links a Cortex-M4 ELF and creates a flash binary using the supplied linker script and startup code. Host tests run the measurement engine and the production clock/serial drivers under address and undefined-behavior sanitizers. The fixture emits synthetic SS1 records from the actual C encoder, not handwritten example telemetry. Run `/usr/bin/python3 app/workbench.py` to open the application, then replay firmware/build/demo.ss1 for an explicitly synthetic demonstration.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py'` for parser, model, serial pseudo-terminal and real Tk event-loop tests. The tests exercise every ordered sector pair, all Gray values, bidirectional wrap, rejected glitches, acquisition loss, counter exhaustion, frame corruption, truncated replay, stale progress, private export, failed setup and connection hangup. They do not involve physical USB hardware, a real sprinkler, actual wet contacts or a programmed MCU. Permission denial is injected in software and is labeled as such.

Run `/usr/bin/python3 tools/check_hardware.py` for native ERC, DRC and an exported schematic-netlist comparison against every PCB pad and the manifest. Regenerate CAD with `/usr/bin/python3 tools/generate_hardware.py` only when deliberately editing the hardware source; the checked-in native files are directly usable. Regenerate mechanical templates with `/usr/bin/python3 tools/generate_mechanics.py`. `tools/verify.py` combines builds, tests, CAD checks, author checks and minimum substantive source/documentation counts. Build outputs, caches and local KiCad preferences are ignored.

Flashing requires a physically connected target and OpenOCD. `make -C firmware flash` uses the ST-LINK interface and STM32F4 target configuration, programs the ELF, verifies it and resets. That command is provided for assembly work, not claimed to have run in this software-only validation. Inspect wiring with power removed, check supply polarity and shorts, then use an appropriate current-limited bring-up procedure. Validate every contact and measure clock/UART timing before a mechanical trial. Do not infer that compiler success proves the hardware works.

## Acceptance and remaining work

The deliverable includes a real routed carrier, substantial freestanding firmware, an executable local companion, mechanical templates and repeatable checks. VALIDATION.md records actual run results and limitations. Electrical bring-up, mechanical fabrication, sensor torque, hysteresis, ingress protection, long-duration clock behavior and before/after irrigation usefulness remain unverified. There is no regulatory, outdoor-safety, metrology or production-readiness claim.

For a first physical experiment, compare the printed scale against a ruler, assemble the plates without a sprinkler and exercise all sector centers and boundaries by hand. Repeat both directions to estimate hysteresis. Record a deliberately held sector and a complete slow sweep, then compare the app with independent video timestamps. Next measure added friction on the intended axis before introducing water. If these tests fail, retain the evidence and revise the fixture rather than adjusting software until the desired answer appears. A useful device is one whose uncertainty stays visible even when the experiment disappoints.
