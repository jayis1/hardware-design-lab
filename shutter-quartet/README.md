# Shutter Quartet — fragmented-exposure notebook for slow teaching shutters

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint material retains its upstream licensing.

Revision A is an engineering prototype. It contains target-linked firmware, a working Linux companion and a fully routed passive carrier for a separately purchased NUCLEO-F401RE. It has not been fabricated, flashed, optically calibrated or physically tested. The MCU, regulator, debugger, USB bridge and processor decoupling are on the Nucleo, not the custom carrier. It is not a photographic shutter calibrator, radiometer or safety instrument.

## Problem, purpose and intended user

A physics teacher or museum workshop builder can demonstrate exposure with two opaque cards, a sliding slit and a diffuse lamp. What students cannot easily see is that equal total travel time does not imply equal illumination at every point, or that a brief return movement can illuminate an already exposed area twice. A stopwatch records the operator's gesture rather than the local light history. A phone camera can record the motion, but extracting four separate threshold histories requires frame review, a trustworthy timebase and careful separation of the recording camera's own exposure behavior.

Shutter Quartet places four small light detectors behind a hand-operated demonstration aperture. It records the first qualified illumination, last qualified darkening, total qualified lit duration and number of lit segments at each position. The notebook preserves the distinction between a long uninterrupted exposure and two shorter exposures separated by darkness. The primary decision is whether a changed slit or return guide reduced spatial timing differences without introducing a second exposure. Missing illuminated points are shown as missing, not as zero-width measurements.

The suggested first fixture is deliberately large and slow: a forty-millimeter square opening, a manually translated opaque card and detector centers near its four corners. Each intended lit interval should last at least twenty milliseconds, and a complete pass must fit within ten seconds including the final dark qualification. This is a practical design range for the exercise, not a verified accuracy specification. The same platform can compare a single sliding slit with two independently moved overlapping cards, without requiring either assembly to resemble a real camera.

Three concrete uses justify the hardware. A teacher compares equal-width and tapered card slits, then asks students to predict which detector receives the longest illumination. An exhibit technician checks whether a return guide reopens one corner during reset. A maker compares a diagonal wipe with a straight wipe and distinguishes crossing opening order from unequal total lit duration. None of these observations establishes photographic exposure, energy delivered to a specimen, or the correctness of a camera repair. The device never drives a shutter or authorizes motion.

## Originality and honest comparison

The repository directories and README titles/purpose excerpts were inventoried before creation. Related measurement contracts were inspected in Chord Window, Rest Quartet and Cam Dwell Map. Chord Window evaluates physical switch chords and shared holds for accessible input. Rest Quartet records independent sample-pocket residence. Cam Dwell Map records angle-weighted cam occupancy with an encoder index. Shutter Quartet instead treats four spatial optical points as one dark-delimited capture, retains fragmented exposure totals and allows every opening order crossed with every closing order. Its application compares exposure completeness, fragmentation and width spread rather than chording, resting or shaft angle.

Multi-point shutter measurement already exists. Reveni Labs' Camera Tester describes three-sensor curtain analysis and a much faster calibrated measurement range. The open-source bjpirt/shutter-tester uses three amplified optical sensors and reports curtain travel and exposure times. The billbill100 project likewise documents multi-sensor curtain adjustment and bounce detection. These are close prior art, not evidence that a new name makes the underlying measurement original. Shutter Quartet does not claim worldwide novelty, patent clearance, superior timing accuracy or invention of shutter testing.

The proposed contribution is the specific educational instrument and its bounded evidence contract: arbitrary four-point choreography, accumulated lit time distinct from first-to-last envelope, explicit missing points, dark-delimited grouping, abandonment rather than guessed continuity, and a local comparison notebook. This contribution is distinct from the inspected repository projects, but its practical value remains a hypothesis until teachers use it. If the job is repairing photographic equipment, use an appropriate shutter tester rather than this deliberately slow fixture. If only one demonstration is needed, a video and manual analysis may be cheaper than building hardware.

References used for comparison and component selection:

- https://www.reveni-labs.com/reveni-labs-camera-tester
- https://github.com/bjpirt/shutter-tester/
- https://github.com/billbill100/Camera-Shutter-tester-Cheap-Easy-it-Works
- https://www.vishay.com/docs/81341/tept4400.pdf
- https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

The Vishay source was retrieved through search during this run. Nucleo pin mapping and register definitions reuse the repository's documented platform, not a newly claimed independent manufacturer-document audit. Verify the exact purchased Nucleo revision and sensor lead drawing before assembly.

## Hardware specification and architecture

| Function | Revision A |
|---|---|
| Processor | STM32F401RET6 on NUCLEO-F401RE MB1136 |
| CPU | Cortex-M4, internal HSI restored to nominal 16 MHz |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Detectors | Four external Vishay TEPT4400 NPN visible-light phototransistors |
| Sampling | Nominal 1 ms polling, independent 5 ms qualification |
| Input bias | Four 10 kohm pullups to Nucleo 3.3 V |
| Series network | Four 1 kohm resistors between detector collectors and MCU sense nodes |
| Power | USB to Nucleo; carrier takes only its 3.3 V and ground |
| Communication | USART2 transmit, 115200 baud, 8N1, ST-LINK virtual COM |
| Carrier form | 80 by 70 mm, two copper layers, 1.6 mm nominal FR4 |
| Host | Linux Python 3 with Tk, POSIX serial, standard library only |
| Actuators, battery, radio | None |

Architecture:

    steady diffuse lamp -> hand-operated opaque cards -> four optical points
                                                          A B C D
                                                          | | | |
    TEPT4400 collectors -> J3..J6 pin 1 -> 1k -> SENSE0..3 -> MCU inputs
    TEPT4400 emitters   -> J3..J6 pin 2 -> GND      |
                                              10k each to 3V3

    USB -> NUCLEO-F401RE -> independent input qualification
                         -> dark-delimited capture / fragmented exposure ledger
                         -> CRC-framed UART -> ST-LINK VCP
                         -> Linux comparison / history / setup screens -> CSV

The lamp is an external low-power, steady visible source with its own suitable supply. It is not powered or switched by the carrier. Do not electrically join an unknown lamp circuit to this instrument. The detector heads contain unpowered phototransistors, not powered output modules. Only collector and emitter leads connect to the board. There is no emitter LED, comparator, adjustable analog threshold or transimpedance amplifier in this revision.

## Pin contract, parts and electrical limits

J1 pin 1 connects to Nucleo CN6 pin 4, 3.3 V; J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect to CN8 pins 1 through 4, labeled A0, A1, A2 and A3. These correspond to PA0, PA1, PA4 and PB0 and are used as digital inputs, not ADC channels. The carrier is attached with short jumpers; its headers are not laid out as a mechanically compatible Arduino shield.

| Carrier | Optical location | MCU | Series resistor | Pullup |
|---|---|---|---|---|
| J3 pin 1 / LIGHT_A | upper left | PA0 | R2, 1k | R1, 10k |
| J4 pin 1 / LIGHT_B | upper right | PA1 | R4, 1k | R3, 10k |
| J5 pin 1 / LIGHT_C | lower left | PA4 | R6, 1k | R5, 10k |
| J6 pin 1 / LIGHT_D | lower right | PB0 | R8, 1k | R7, 10k |

Every detector connector uses pin 2 for ground. Connect each TEPT4400 collector to signal and emitter to ground, following its actual datasheet orientation rather than assuming lead length. Illumination increases collector current and lowers the sense node; firmware inverts the digital level so one means qualified light. USART2 TX is PA2 AF7 to the ST-LINK virtual COM bridge. The applicable MB1136 bridge configuration is SB13/SB14 fitted and SB62/SB63 open. No receiver or control commands are enabled.

The TEPT4400 datasheet identifies a visible-light NPN phototransistor in a three-millimeter package, with a six-volt collector-emitter maximum and broad sensitivity variation. Its typical light current is not a guaranteed digital switching threshold. Strong enough steady illumination must be established experimentally at every head. At ideal full conduction, the 11 kohm path draws 0.300 mA and leaves approximately 0.300 V at the MCU input. Assuming a 0.2 V collector residual gives approximately 0.482 V at the sense node. These are circuit calculations, not guaranteed saturation behavior or measurements. Four ideal closed paths draw 1.200 mA and approximately 3.96 mW together.

Verify high and low levels against the actual MCU input specification under the intended lamp, ambient light and geometry. A slow optical transition may dwell near the switching threshold, and hysteresis and filtering bias both endpoints. Sensor mismatch, finite detector area, lamp ripple and saturation recovery can dominate a small timing difference. Do not convert the reported milliseconds to calibrated camera accuracy, illuminance or exposure value. Reject the setup if any detector cannot reliably distinguish fully blocked and fully lit conditions.

The carrier manifest contains fourteen populated components: four 10k axial resistors, four 1k axial resistors, five two-pin headers and one four-pin header. Use one-percent quarter-watt DIN0207 axial parts with 7.62 mm lead pitch and insulated mating housings. Add four TEPT4400 detectors, one Nucleo, USB cable, opaque fixture materials, a diffuse light source, wiring and strain relief. Budget roughly USD 60–120 for a hand-built teaching assembly, excluding labor and shipping; this is a planning allowance, not a live supplier quotation or availability promise.

Reserve a conventional regulated 5 V, 500 mA USB supply budget for the Nucleo and debugger rather than claiming measured consumption. There is no battery lifetime calculation, charger or second carrier supply. The board shares ground with the computer and has no galvanic isolation or dedicated surge protection. Never connect mains, externally driven sensor outputs, industrial controls or unknown wiring. The series resistor is not a safety barrier. Keep cables short, initially below half a meter, and keep all electronics dry.

## Mechanical arrangement and experimental setup

Use a nonconductive detector plate behind a forty-by-forty-millimeter aperture. A useful starting coordinate system places detector centers at x/y values of minus and plus twelve millimeters relative to the aperture center. Label A upper left, B upper right, C lower left and D lower right when viewed from the light-source side. Each detector should look through a short matte-black tube or baffle that limits stray light without touching the moving cards. Keep all detector faces approximately in one plane and retain their position between comparative trials.

Provide two parallel card guides ahead of the detector plate so two opaque masks can move without rubbing the detector heads. Use hand motion only. A removable slit card permits equal-width and tapered openings; a separate return guide permits deliberately testing an unwanted second opening. Mechanical end stops should absorb operator force rather than the sensors or PCB. An initial twenty-millimeter clearance between moving cards and detector packages allows inspection, but final dimensions depend on the actual baffles and fixture. These are construction specifications for a prototype, not a released enclosure CAD model.

Place the diffuse lamp far enough away to illuminate the aperture consistently and shield the assembly from room-light changes. Use a low-power visible source without lasers, concentrated sunlight or high-power lamps. Avoid PWM-dimmed lighting; qualified flicker can look exactly like shutter reopening. Keep the lamp setting fixed, record it in the experiment notes, and swap detector locations in a control run to distinguish fixture effects from sensor mismatch. The notebook cannot identify which cause produced a second optical segment.

The PCB has no mounting holes; retain it in insulated edge rails with the Nucleo nearby and reset, USB and programming access available. Protect the exposed headers from accidental contact. Before an optical experiment, check the power rail and continuity with power removed, then power through the documented Nucleo arrangement. Cover all heads and confirm dark readings; uncover each separately and confirm the corresponding raw bit. Do not start interpreting widths until all four heads pass this simple mapping check.

## Firmware behavior and capture contract

The firmware is freestanding C11 with an explicit vector table, data/BSS initialization, linker memory regions, bounded reset-clock restoration, GPIO configuration checks, a one-millisecond SysTick, independent watchdog and nonblocking UART transmitter. The build uses the real ARM embedded compiler without a vendor SDK, heap or operating system. Board support, serial transport and carrier topology reuse jayis1's rail-skew platform at commit 386daf9. The exposure engine, wire fields and companion interpretation are specific to this device.

WAIT means that a complete dark baseline has not yet been observed. Every channel qualifies independently: the first changed raw sample establishes a candidate, then five further consecutive samples accept it. A dark window advances only while raw and qualified levels are all dark at both ends of each interval and all channels are known. After 250 such milliseconds the state becomes READY. A raw glitch restarts this clear window even if it never becomes a qualified optical edge. Illumination already present at boot cannot create a fictitious opening time.

The first qualified light in READY starts ACTIVE with capture age zero. Every newly lit channel records its first opening offset only once, increments its segment count on each later qualified rising edge and records its most recent closing offset on every falling edge. Each consecutive interval credits the previous qualified lit channels. Thus total lit time equals the envelope for one segment, but is smaller when a channel reopened after darkness. There is no required left-to-right ordering, and simultaneous openings receive equal offsets rather than an invented ranking.

A full raw-and-qualified dark window finishes the capture. The published span is the latest closing offset, not the additional 250 ms clearing period. Published results contain the mask of observed detectors, their first/last offsets, total lit intervals and segment counts. A missing detector has all-zero fields but is interpreted through its absent mask bit, not as an instantaneous exposure. A new active attempt does not overwrite the last completed result. Application labels explicitly distinguish current capture state from retained completed evidence.

Dark intervals shorter than the clearing window remain inside one capture, allowing return leakage or repeated openings to be studied. Darkness lasting the entire window ends that capture; later illumination belongs to another. This grouping rule is a deliberate experimental policy, not proof that two events physically belong to the same shutter movement. Explain it to students before comparisons and do not silently change it between runs.

At age 10,000 ms the attempt times out before a simultaneous closing edge can complete it. Working evidence is discarded, aborted and timeouts increase, and a fresh dark baseline is required. Any acquisition interval other than one millisecond similarly abandons active evidence and restarts qualification, while preserving completed results. Unknown intervals are never credited as exposure. Ordinary unsigned uptime rollover remains continuous when the difference is one millisecond.

The invariant is starts = completed + aborted + active, with active one only in ACTIVE. Sample updates are staged transactionally. Counter exhaustion rolls the attempted update back, abandons the old active capture once and enters HALTED. Configuration or watchdog-start failures also halt measurement before trusting telemetry. Fault bits are gap=1, overflow=4, configuration=8 and watchdog=16. Gaps are recoverable but sticky; the other bits are terminal. Restart clears volatile evidence, so export useful records first. This is not nonvolatile logging.

## Protocol and companion workflow

Every available hundred-millisecond transmission begins SQ1 and contains thirty-four unsigned decimal fields, then an asterisk, four uppercase hexadecimal CRC characters and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021, initial 0xffff, no reflection and no final XOR over everything before the asterisk. The encoder bounds its output and the UART abandons an unfinished transmission after one hundred milliseconds. CRC detects accidental corruption; it is not authentication or a signature.

The field order is time, raw, levels, known, state, starts, completed, aborted, timeouts, gaps, tx, flags, age, span, seen, qualify, dark, limit; then first, last, lit, pulses for A, B, C and D in that order. States are WAIT=0, READY=1, ACTIVE=2 and HALTED=3. All durations are milliseconds. Policy values 5, 250 and 10000 repeat in every frame so late attachment does not depend on a boot message. The mask bits A/B/C/D are 1/2/4/8. The definitions in app/model.py and firmware/wire.c are the executable schema.

The Exposure comparison screen shows retained timings and a decision prompt. It first identifies missing points, then qualified re-openings, then compares the maximum-minus-minimum total lit duration against a local tolerance. That tolerance is an educational comparison setting, never a camera specification or firmware command. Snapshot notebook shows all retained snapshots up to its five-hundred-row limit. Setup and interpretation explains wiring, grouping and limitations. The program has real serial connection, disconnect, strict replay and private CSV export; there is no cloud account or external network service.

Replay accepts only a regular file up to two MiB, rejects FIFO/device inputs and incomplete trailing frames, and always labels evidence offline. Live framing resynchronizes on a complete SQ1 marker after truncated data. The parser checks decimal syntax, CRC, masks, conservation, policy, bounds and necessary pulse-envelope relations. Session validation rejects counter rollback, contradictory same-time snapshots, disappearing fault bits and completed results changing without another completion. These are consistency checks, not a cryptographic proof that the light fixture existed.

LIVE requires advancing device time and an owned connection. Duplicate timestamps do not refresh it. A reception pause above one second or host/device cumulative progress disagreement above half a second blocks resumed acceptance until reconnect. Oscillator drift can therefore require conservative reconnects in long sessions. Reset detection is heuristic: there is no persistent boot identity, and a lost reset followed by forward-looking data can be indistinguishable from continuity. Disconnect preserves provenance; successful reconnect clears desktop history but does not reset the MCU. Failed connection or replay replacement preserves existing evidence.

CSV export creates a new file with mode 0600 and refuses overwrite. Its metadata row records jayis1, provenance, connection/freshness state, age, parser errors, blocked reason and evicted rows even when no capture completed. The export contains snapshots, not a lossless event journal. If several captures finish between received records, their individual details may be missing even though cumulative counters advance. Save experiment notes separately, including lamp, detector layout, card geometry and whether any aborted attempts occurred.

## Building and reproducible verification

From this device directory:

    make -C firmware all test
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_quality.py
    python3 app/workbench.py

Prerequisites are GCC for host tests, arm-none-eabi-gcc/binutils for target linking, Python 3 with Tk, Xvfb for headless GUI tests, and KiCad 9 with its Python bindings and installed footprint libraries. No pip dependencies are needed by the companion. The hardware generator can reproduce the authored schematic, board and manifest using /usr/bin/python3 tools/generate_hardware.py; regeneration overwrites those generated design artifacts. The import_platform helper is provenance documentation and refuses to overwrite already delivered files.

The test suite exercises all 576 combinations of distinct four-point opening and closing orders against an independent time-window oracle, simultaneous exposure masks, fragmentation, unrelated-channel chatter, initial illumination, exact dark-window boundaries, acquisition loss, timeout precedence, uptime wrap and diagnostic exhaustion. Host builds enable address and undefined-behavior sanitizers. App tests consume real C-encoder synthetic frames, inject CRC-valid contradictions, check replay bounds and use Linux PTYs for connection behavior. GUI tests run the actual Tk event loop through connection, navigation, replay, export, hangup and reconnect.

KiCad checks run ERC and DRC without waivers, require zero unconnected items, export a fresh schematic netlist and compare every component reference, value, footprint and pin net with the PCB and manifest. The carrier uses real embedded symbols and installed through-hole footprints. A rendered schematic was visually inspected for legible net labels and meaningful connections. Passive-symbol ERC does not prove external power polarity, detector current or optical performance; those remain bench responsibilities.

Before relying on a fabricated instrument, verify shorts, rail voltage, sensor lead orientation, all GPIO mappings, dark/bright noise margins, UART output and watchdog behavior. Compare timed light transitions against an independent reference at multiple widths, intensities and detector positions. Measure channel-to-channel threshold bias, repeat the experiment after swapping sensors and test intentional lamp flicker and card re-openings. Only then decide whether the prototype is useful for teaching. No physical acceptance result, regulatory compliance or fabrication success is claimed by the software and CAD tests.
