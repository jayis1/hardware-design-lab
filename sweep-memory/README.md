# Sweep Memory — directional contact maps for a hand-retraced mechanism

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and locally authored design sources are MIT licensed; see LICENSE. Platform startup, GPIO, clock, UART, passive carrier construction, serial transport and their regression tests are explicitly reused from jayis1's Insert Code at commit 396308d62ab2f6f041beff9168b56cf5a6eb8a7c. The import script records the exact source and refuses overwrites. It is provenance tooling, not a normal build step. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive interface used with a purchased NUCLEO-F401RE. It is not a standalone microcontroller motherboard. Target linking, software tests and CAD checks can validate the supplied files; they cannot establish physical accuracy, reliable contact wetting, mechanical usefulness or production readiness. No fabrication, hardware programming, electrical bring-up or calibration has been performed.

## Purpose and practical utility

Sweep Memory helps a maker investigate why two contact-operated features on a small mechanism behave differently when approached from opposite directions. A hand-turned model cam, adjustable over-center linkage or two-follower demonstration can reach the same apparent position with a different switch state. The difference may come from contact hysteresis, a loose coupling, elastic preload, a sticking lever or the chosen observation boundary. A live continuity lamp hides that history. A simple count of closures cannot associate it with position. This instrument retains a coarse outward contact map and compares the return against it at corresponding relative encoder cells.

The intended audience is a museum workshop, repair cafe, teaching laboratory or maker comparing noncritical mechanisms on an unpowered bench. It is especially useful when two followers must be compared without moving the encoder or changing the fixture between measurements. The operator observes a short outward sweep of sixteen encoder transitions, reverses, and returns through sixteen transitions. Firmware records both binary probe states along the outward path. On return, each probe gets an independent mismatch mask. The companion displays the outward and return rows side by side and exposes incomplete or rejected observations.

For example, a model builder can attach the encoder to the shaft of a hand-operated display, place two low-force switches at different adjustable follower brackets, and mark a start position. After an outward-and-return capture, the X row might disagree at four relative cells while Y agrees everywhere. That is a reason to examine the X bracket, not a diagnosis that its spring is defective. Moving one bracket and repeating under identical conditions can show whether the disagreement region changes. Saving the evidence before resetting is essential because each capture is deliberately one-shot.

The instrument does not measure dimensional runout, calibrated angular hysteresis, torque, force, wear, friction or backlash. Equal encoder phase cells are not identical physical angles. A mechanical encoder has finite switching regions and direction-dependent boundaries of its own. The fixture and encoder may contribute much of the observed disagreement. The proposed utility is a transparent comparative notebook, not a substitute for a dial indicator, precision angular encoder, force gauge or logic analyzer. Its benefit must be assessed in a real workshop pilot.

## Originality assessment and alternatives

Repository directories and README purpose statements were inspected after the required fast-forward-only pull. The closest designs were examined in more detail. Cam Dwell Map creates an indexed, one-direction, full-revolution follower map; it rejects reversals instead of making retrace comparison the experiment. Takeup Atlas uses two encoders to record the onset of following after reversal. ContactAtlas maps detached switch signatures without a positional encoder. Detent Loop estimates force versus crosshead travel using analog displacement sensors. Sweep Memory instead observes two digital thresholds against one relative phase coordinate, preserves the outward map, and pairs a bounded reverse retrace with that map.

Rotary encoding, contact hysteresis and bidirectional measurements are established techniques. A bounded external search found Keyence's discussion of quadrature transition tables and direction conventions, and an UNSW engineering note describing mechanical-encoder reversal errors and contact bounce. The latter is particularly relevant: the observer's own encoder can create apparent history differences. These sources argue against presenting the instrument as a new physical sensing principle or an accurate backlash meter. The differentiated contribution is the particular open, small, one-shot paired-map workflow and its explicit rejection/evidence contract. Worldwide novelty, patentability and absence of similar commercial equipment are not established.

A general-purpose logic analyzer can record the same electrical channels at much higher speed and is preferable for bounce analysis. A protractor and continuity meter are adequate for a single static experiment. Commercial profile-measurement systems are preferable for calibrated geometry. This design is appropriate only when a directly readable, inspectable short-sweep record saves effort over those alternatives. There is no cloud dependence, account requirement, machine learning model or hidden classification threshold.

References:

- https://www.keyence.com/ss/general/embedded-software/peripheral-drivers/rotary-encoder-implementation.jsp
- https://mcba1.phys.unsw.edu.au/~mcba/4991.html
- Bourns PEC11R manufacturer datasheet, mirrored revision 06/13: https://datasheet.octopart.com/PEC11R-4215F-S0024-Bourns-datasheet-16095859.pdf
- Manufacturer source: https://www.bourns.com/docs/product-datasheets/pec11r.pdf
- ST Nucleo manual UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 register reference: ST RM0368. Register definitions and startup are inherited from the pinned, tested repository platform.

Direct Bourns PDF retrieval returned HTTP 403 and the direct ST manual retrieval timed out in this run. Search retrieval supplied the mirrored manufacturer encoder specification, including A/common/B labels and quadrature output. Check current manufacturer revisions and the actual purchased board before assembly; this document does not imply that every current supplier document was downloaded successfully.

## Hardware specifications and architecture

| Function | Revision A implementation |
| --- | --- |
| Controller | NUCLEO-F401RE MB1136, STM32F401RET6 Cortex-M4 |
| Clock / memory | Nominal 16 MHz HSI, 512 KiB flash, 96 KiB SRAM |
| Encoder | Bourns PEC11R-4215F-S0024, 24 pulses/revolution, A/common/B contacts |
| Threshold witnesses | Two externally mounted Omron D2F-01L switches, COM and NO |
| Electrical sensing | Four pulled-up, low-active, voltage-free contact channels |
| Sampling | Nominal 1 kHz foreground observations, explicit missed-tick rejection |
| Qualification | Entire four-bit word, five unchanged intervals after its first observation |
| Link | USART2 TX PA2 AF7, 115200 baud, eight data bits, no parity, one stop bit |
| Host interface | Nucleo ST-LINK USB virtual COM; target has no command receiver |
| Carrier | 80 by 70 mm, two layers, 1.6 mm FR-4, through-hole components |
| Software | Freestanding C11 firmware; Linux Python 3 / Tk companion, standard library only |
| Excluded subsystems | No battery, charging circuit, radio, motor, relay or FPGA |

    Hand-operated shaft or knob
              |
        mechanical coupling
              |
         encoder A/B -----------+
                                |
    follower X --> dry switch --+--> four pullup/series networks
    follower Y --> dry switch --+             |
                                     PA0 PA1 PA4 PB0
                                              |
                                     NUCLEO-F401RE
                                  qualified phase decoding
                                   outward map / retrace
                                              |
                                  USART2 --> ST-LINK USB
                                              |
                              Tk map / history / CSV export

The Nucleo supplies the MCU, regulator, decoupling, reset, debugger and USB bridge. The carrier contains fourteen actual components: six connectors and eight resistors. Its schematic, routed PCB and connectivity manifest agree on every pin, value, footprint and net. Passive ERC cannot confirm that the external board provides the right voltage. The generated layout has no mounting holes; use insulating edge supports and provide strain relief. A roughly 160 by 110 by 45 mm enclosure is a packaging allowance for carrier, Nucleo and wiring, not supplied mechanical CAD.

## Wiring, BOM and power boundaries

J1 pin 1 is VCC and connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. Never apply USB 5 V to VCC. J2 pins 1 through 4 are SENSE0 through SENSE3 and connect respectively to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. J3 pin 1 is PHASE_A, J4 pin 1 is PHASE_B, J5 pin 1 is PROBE_X and J6 pin 1 is PROBE_Y. Every one of these two-pin connectors has ground on pin 2.

Connect encoder terminal A to J3 pin 1, terminal B to J4 pin 1, and common C to ground. Its optional push switch is not used; insulate both switch terminals separately. Do not infer A/B orientation from a photograph: check the manufacturer's drawing, which labels channel A, common and channel B. Connect each probe's COM to ground and NO to its respective signal. The carrier powers no external active sensor. Using active encoder electronics or a different MCU board requires a new electrical review.

| References | Quantity | Procurement specification |
| --- | --- | --- |
| R1 R3 R5 R7 | 4 | Yageo MFR-25FBF52-10K, 10 kohm, 1%, axial |
| R2 R4 R6 R8 | 4 | Yageo MFR-25FBF52-1K, 1 kohm, 1%, axial |
| J1 J3 J4 J5 J6 | 5 | Samtec TSW-102-07-G-S, 2-pin 2.54 mm header |
| J2 | 1 | Samtec TSW-104-07-G-S, 4-pin 2.54 mm header |
| Controller | 1 | ST NUCLEO-F401RE |
| Encoder | 1 | Bourns PEC11R-4215F-S0024, externally mounted |
| Probes | 2 | Omron D2F-01L, externally mounted |
| Mechanics | 1 set | Coupling, two adjustable brackets, base, knob, guards and harness |

Each closed channel ideally draws 3.3 V divided by 11 kohm, or 0.300 mA. The MCU-side low voltage is nominally 0.300 V; four closed channels draw 1.200 mA. These are circuit calculations, not measured currents or a complete USB power budget. Reserve a protected 5 V USB source capable of 500 mA for the Nucleo and measure actual assembled consumption. The encoder datasheet's contact rating is a maximum, not proof that this lower sensing current reliably wets worn contacts. No hardware RC noise filter is fitted; the manufacturer's filtered bounce specifications cannot simply be transferred to this circuit.

There is no galvanic isolation, TVS array, fuse or reverse-polarity protection on the passive carrier. Connect only detached voltage-free contacts. Never connect mains, powered switch circuits, PLC outputs or machinery safety wiring. Start with paired signal/ground wiring under half a meter and keep it away from switching power cables. A broken wire can look like an open switch; an undetected complete encoder cycle can leave no obvious invalid transition. Neither valid CRC nor a completed capture proves correct wiring or complete physical observation.

## Mechanical setup and operating procedure

Use a rigid, nonconductive or grounded-and-insulated base with the shaft independently supported. The encoder bearing must not carry the mechanism's radial load. A lightly loaded flexible coupling reduces mounting stress, but its compliance and backlash also become part of the experiment. Provide finger clearance and hard stops that do not transmit excessive force through the switch levers. Use only low-energy hand-operated models; disconnect every motor and stored-energy source before attaching the instrument.

Mark an arbitrary start position on the fixture. It need not be a particular encoder phase, but it should be repeatable enough for the comparison being attempted. Bring the mechanism there manually, hold still, reset the Nucleo, and wait for READY in the companion. Reset does not find a home or certify a mechanical reference. It creates a new volatile observation. Watchdog and power resets likewise cannot recover a previous position. Export the prior session and explicitly reconnect after any reset.

Turn slowly in either direction. The first qualified phase edge chooses the outward direction and starts the trial. Advance exactly sixteen qualified edges while watching position in the app. The state then becomes RETURNING. Reverse and traverse back to relative position zero. Do not continue past the turnaround or reverse again during return. The device freezes at DONE, REJECTED or HALT. Repeating requires another deliberate reset and a newly established physical start. There is no remote reset command.

Begin bench trials at no more than about five qualified phase edges per second, pausing between movements. This is a conservative procedure, not a verified maximum speed. Each whole word needs six observations, and contact chatter can delay phase qualification. The 24-pulse encoder nominally has four electrical transitions per pulse, but detents and phase edges are not interchangeable. The UI intentionally reports relative cells, not degrees. Sixteen transitions delimit the software capture; they do not establish a calibrated physical span.

## Firmware state model and measurement contract

WAIT_BASELINE accepts the first unchanged whole word after five complete observation intervals. READY follows and can update the resting qualified contact state without starting a trial. A legal phase transition opens OUTWARD. Position zero captures the previous qualified probe word, and position one captures the new probe word. Each further outward edge records another position. Position sixteen changes the phase to RETURNING. There is no independent return measurement of position sixteen, because a shared turnaround sample would falsely appear to demonstrate agreement with itself.

Return edges capture positions fifteen down to zero. The completed result therefore contains seventeen outward entries and sixteen paired comparisons per probe. Each mismatch bit is the XOR of outward and return states masked by actually compared positions. X and Y remain separate; a combined count would hide which follower changed. Contact-only changes while the encoder phase stays fixed update the qualified word but do not revise already captured entries. The instrument samples state at qualified cell entry, not every value throughout that cell.

The numerical phase ring is 0, 1, 3, 2. Adjacent movement determines signed direction; a two-bit phase jump is invalid. Either initial direction is permitted, so direction value one or two denotes electrical ordering rather than guaranteed clockwise motion. Early outward reversal rejects the trial. An additional outward edge after position sixteen, or a reversal during return, rejects it as overshoot/return reversal. The rejecting sixty-second deadline has precedence over a completion qualified on that same sample. A sample gap rejects any unfinished capture and invalidates qualification rather than treating missing time as stable input.

Counters and bit planes have bounded integer representations. Starts equals completed plus abandoned plus active, and a powered run starts at most one trial. Unexpected input masks, timestamp regression, diagnostic overflow, GPIO configuration failure and watchdog initialization failure cause HALT. Transport errors are counted, but a USB host that does not read cannot be reliably detected by the target UART. The nonblocking serial driver bounds transmission work so telemetry cannot intentionally stall acquisition. Firmware never drives a contact or controls the mechanism.

## Protocol and application

SM1 is a versioned ASCII status record emitted approximately every 100 ms. It contains unsigned decimal fields in exactly the order declared by app/model.py FIELDS and firmware/wire.c: device milliseconds; acquired samples; gaps; missed ticks; transmit errors; raw and qualified words; validity; fault; state; rejection reason; position; direction; starts; completed; abandoned; age; outward X/Y planes; return X/Y planes; comparison mask; mismatch X/Y planes. A star introduces four uppercase hexadecimal CRC digits, followed by LF. CRC is CCITT-FALSE with polynomial 0x1021 and initial value 0xffff over all bytes before the star.

The companion validates ranges, trial conservation, coverage masks, mismatch arithmetic, monotonic counters, retained-reference immutability and terminal evidence freezing. Sparse status packets cannot prove every hidden transition, and CRC detects accidental corruption rather than malicious modification. The serial decoder bounds buffers, recovers at the complete SM1 marker, counts discarded fragments and rejects partial replay tails. Replays are limited to regular files no larger than two MiB; FIFOs and devices are refused before reading. CSV export uses a distinct format and is not a serial replay file.

The Position comparison screen shows four rows, with blue for closed, pale for open, yellow for unobserved and red ordinal labels for disagreement. Acquisition history shows the most recent two hundred accepted snapshots; the session retains up to two thousand for export. The status line distinguishes live acquisition, waiting, disconnected evidence and replay. Two advancing observations are needed before acquisition is called live. Duplicate traffic does not renew freshness. A stale acquisition or more than one second of cumulative device/host timing divergence closes the link and requires explicit reconnection. Oscillator error may cause conservative disconnects; no unlimited drift allowance is applied.

Export creates a new private CSV without overwriting an existing path. Its metadata includes jayis1 attribution, provenance, connection state, freshness, parser errors, warning and session-blocked status, even when no capture completed. Disconnect retains evidence but does not label it live. A failed connection attempt does not erase an earlier session. The host is Linux because its serial implementation uses POSIX termios; port permissions must be configured by the operator. No privileged application launch is necessary or recommended.

## Build, verification and physical acceptance

From the repository root, run `make -C sweep-memory/firmware all test fixture`. The target build uses arm-none-eabi-gcc, Cortex-M4 Thumb, a custom linker script and startup vector table, with warnings treated as errors. Host engine, clock and serial tests use AddressSanitizer and UndefinedBehaviorSanitizer. The synthetic fixture is produced by the actual C encoder and is not a physical capture. Run `python3 sweep-memory/tests/test_app.py` for protocol checks and `xvfb-run -a /usr/bin/python3 sweep-memory/tests/test_gui.py` for the real Tk event loop, PTY serial exchange, replay and export checks. Start the application with `python3 sweep-memory/app/main.py`; an optional recording path opens replay.

KiCad 9 and its Python pcbnew module are required for CAD regeneration and validation. Run `/usr/bin/python3 sweep-memory/tools/generate_hardware.py` and `/usr/bin/python3 sweep-memory/tools/check_hardware.py`. The checker runs ERC and DRC, exports a fresh schematic netlist, and compares all schematic pins, actual board pads, component values and footprint identities to the manifest. A schematic SVG export should also be visually inspected. Keep build products, local KiCad preferences and exported inspection files out of Git. Every delivered authored source carries jayis1 attribution in a format-valid location.

After independent electrical inspection, an operator can program the target using `make -C sweep-memory/firmware flash`, which invokes OpenOCD with ST-LINK and STM32F4 configuration. That command changes attached hardware and is not part of the unattended file-verification run. First inspect supply polarity and continuity without power, then verify idle and closed input voltages with a meter. Check the actual Nucleo's USART2/ST-LINK solder bridges against UM1724. Confirm timing and edge qualification against a logic analyzer, exercise both directions, deliberately introduce an illegal jump and sampling interruption, and verify that no rejected trial appears completed.

Finally, compare several captures against video or an independent angular reference while keeping the mounting and loading fixed. Repeat with a rigid dummy follower to estimate the fixture's own contribution. If the encoder or probe mechanism dominates the mismatch region, use better sensors rather than subtracting an invented correction. Validation results in VALIDATION.md describe software and CAD evidence only. Calibration, ESD robustness, EMC compliance, environmental endurance, reliable minimum sensing current and practical workshop benefit remain open physical acceptance work.
