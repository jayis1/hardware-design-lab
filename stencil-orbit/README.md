# Stencil Orbit — manual carousel station and tool-use notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored hardware sources: MIT.

Revision A is an engineering prototype. Software is target-buildable and the custom passive PCB is routed, but this revision has not been fabricated, flashed, or physically tested. The processor, regulator, decoupling, debugger and USB bridge are supplied by a separate NUCLEO-F401RE. The delivered PCB is not a standalone microcontroller board. No print-quality, safety, or production-readiness certification is claimed.

## Purpose, specific user and practical utility

Stencil Orbit is a removable observation kit for a small printmaking workshop using a slow, hand-rotated four-station carousel. It records which indexed station was present when a squeegee was taken from its rest, whether registration remained engaged throughout that tool-use episode, and whether the next accepted episode occurred at the expected station. The immediate user is a workshop instructor or short-run printer investigating repeated stations, skipped stations, interrupted work, and apparently unexplained registration mistakes. Their current workaround is to watch the operator, mark a paper checklist, or record video and review it later. That is intrusive, easy to forget, and difficult to correlate with a particular pause in a long session.

The job is deliberately narrow: associate observed tool pickup and return with a trustworthy relative station index, while retaining evidence when that association becomes unreliable. The device does not claim that a tool pickup represents one print stroke. Someone can lift the squeegee to clean it, make several strokes, or return it without printing. Accordingly, the interface calls the result an accepted use episode, never a verified finished print. A worker can combine that evidence with their batch notebook and inspect the particular pieces associated with an irregular interval.

Dedicated hardware is justified because the four physical signals have to be observed together while the operator's hands are occupied. A phone stopwatch cannot reconstruct direction, registration release, and tool-rest state. Video can potentially do so, but requires placement, lighting, review time, and a view of people and work. This kit captures no image, sound, identity, or customer artwork. It does not need Wi-Fi, a subscription, or a radio module. A local laptop receives bounded telemetry over the development board's existing USB serial bridge.

## Originality and prior art

This concept was selected after listing repository directories and reviewing device summaries. The nearest existing entries are Pick Witness, which observes shuttle/shed/beat behavior on a handloom; WebSlack Ledger, which compares two material-feed encoders; and Swing Envelope, which observes pendulum crossings and bilateral thresholds. Stencil Orbit shares reusable contact-interface electronics with those low-voltage prototypes but not their measurement model. Its new behavior is a modulo station index, continuous registration qualification over a tool-use interval, and per-station repeat/out-of-order accounting for a manual carousel.

Existing manual carousels already have useful mechanical registration. [Brildor's four-station machine](https://www.brildor.com/en/screen-printing-machine-manual-4-stations) describes independent rotation, locking arms and micro-registration. [Sureprint manual carousels](https://www.screenprint-machines.com/manual-carousels) describe bearing registration, adjustable clamps and repeatable positioning. [US6101938A](https://patents.google.com/patent/US6101938A/en) describes a rotary screen-printing registration system with gauges used to recover frame alignment. These are adjacent mechanisms, not evidence that registration itself is a new invention.

The proposed differentiation is a removable, receive-only notebook that records registration continuity during tool use rather than adjusting frame alignment or controlling the press. The limited searches used here do not prove global novelty, patentability, or freedom to operate. Before commercial development, interview working printers and conduct a broader product and patent search. A useful validation question is whether the notebook explains enough rework to justify mounting four sensors; this has not yet been answered in a workshop trial.

## Hardware specifications and architecture

The MCU is the STM32F401RET6 on a NUCLEO-F401RE MB1136 board: Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware runs at the reset HSI clock of 16 MHz, not the part's maximum frequency. A 1 ms SysTick drives foreground acquisition. USART2 transmits at 115200 baud, eight data bits, one stop bit and no parity. There is no receive command parser. USB transport is provided by ST-LINK's virtual COM interface, not by a fabricated USB stack in this firmware.

The custom board has four identical resistor-isolated contact inputs, six through-hole headers, and eight resistors. Its outline is 80 by 70 mm, two copper layers, 1.6 mm nominal thickness. Tracks are 0.3 mm wide; the project clearance is 0.2 mm. It is an electrically passive cable adapter. There is no charger, cell, RF antenna, motor driver, relay, display, or onboard regulator. Use an insulated carrier enclosure and insulated supports; the current board has no dedicated mounting holes. Do not drill through a routed board to improvise mounting.

```text
Four-cycle quadrature cam -- A/B dry switches -- J3/J4 -- 1k -- PA0/PA1
Registration witness ------ dry switch -------- J5 ---- 1k -- PA4
Squeegee rest ------------- dry switch -------- J6 ---- 1k -- PB0
                                                         |
                      10k pulls to Nucleo 3V3 ------------+
                                                         |
                        STM32F401RE: qualify, index, episodes
                                                         |
                             USART2 TX / ST-LINK USB VCP
                                                         |
                       Linux desktop: live / history / export
```

The mechanical sensing arrangement uses a four-cycle-per-revolution quadrature cam on the observed rotating member, two low-current-rated switches, a separate switch witnessing registration engagement, and a switch in the squeegee rest. Do not monitor one independently rotating axis and assume it identifies the other. Choose the screen or pallet carrier appropriate to the particular workflow and label that choice on the enclosure. A carousel whose screens and pallets rotate independently needs additional instrumentation to identify both; revision A intentionally observes only one member.

A practical prototype cam has two concentric tracks with four repeated sectors each. Each electrical cycle spans 90 mechanical degrees. Offset track B by 22.5 degrees relative to track A to obtain a quarter electrical-cycle phase shift. Arrange both tracks open at every indexed station, with the 00 plateau centered at the index. Four decoded edges then equal one station, and sixteen edges equal one revolution. Mechanical dimensions depend on the carousel hub; the proposed starting ring is 100 mm outside diameter with track radii near 35 and 42 mm. These are fabrication suggestions, not a delivered fitted mechanical drawing. The builder must measure shaft clearance, runout, switch travel, contact hysteresis and available guards before making a removable clamped ring. Never modify a load-bearing registration component to mount the witness switch.

Use switches explicitly rated for low-level dry circuits, preferably gold-contact devices. Commodity power microswitches may require more wetting current than this circuit supplies. Keep each stable quadrature state present for at least 10 ms in commissioning tests. The five-millisecond whole-vector qualifier intentionally rejects very short pulses. This is a slow manual workflow recorder, not a high-speed encoder counter. Simultaneous opposing quadrature transitions invalidate position rather than guessing direction.

### Connector and net contract

| Reference | Pin | Signal and destination |
|---|---|---|
| J1 | 1 | VCC, 3.3 V from Nucleo CN6 pin 4 |
| J1 | 2 | GND, Nucleo CN6 pin 6 or 7 |
| J2 | 1 | SENSE0 to A0, PA0, CN8 pin 1 |
| J2 | 2 | SENSE1 to A1, PA1, CN8 pin 2 |
| J2 | 3 | SENSE2 to A2, PA4, CN8 pin 3 |
| J2 | 4 | SENSE3 to A3, PB0, CN8 pin 4 |
| J3 | 1 / 2 | CONTACT0 / GND: encoder A |
| J4 | 1 / 2 | CONTACT1 / GND: encoder B |
| J5 | 1 / 2 | CONTACT2 / GND: registration engaged |
| J6 | 1 / 2 | CONTACT3 / GND: tool parked |

R1, R3, R5 and R7 are 10 kilohm pulls from VCC to SENSE0 through SENSE3. R2, R4, R6 and R8 are 1 kilohm series resistors between the corresponding sense and external contact nets. A contact closure pulls its sense low; firmware inverts the GPIO values so a set protocol bit always means closure. This convention is identical in board.h, the hardware generator, and the application setup screen.

The board mapping and serial solder-bridge configuration follow [ST UM1724, revision 17, table 16 and section 7.10](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf). For ST-LINK VCP, retain SB13/SB14 enabled and the conflicting external USART bridge connections disabled as specified for the exact board revision. Inspect the actual board rather than assuming every secondhand Nucleo has factory settings. The register definitions refer to the STM32F401 reference manual RM0368; they are real RCC, GPIO, USART2, watchdog and SysTick addresses, not a made-up peripheral abstraction.

### BOM and power budget

| Quantity | Reference / part | Selection | Prototype allowance |
|---|---|---|---|
| 1 | External controller | ST NUCLEO-F401RE | USD 25 |
| 4 | R1/R3/R5/R7 | 10k, 1%, 0.25 W axial, 7.62 mm pitch | USD 1 total |
| 4 | R2/R4/R6/R8 | 1k, 1%, 0.25 W axial, 7.62 mm pitch | USD 1 total |
| 5 | J1/J3/J4/J5/J6 | 1x2, 2.54 mm male header and mating harness | USD 3 total |
| 1 | J2 | 1x4, 2.54 mm header and harness | USD 1 |
| 1 | Carrier PCB | Two-layer prototype board | USD 5 allocation |
| 4 | External contacts | Low-current-rated switches, builder-selected mechanics | USD 12 allocation |
| 1 set | Mechanics | Cam, rest, clamps and insulated enclosure | USD 15 allocation |

These are engineering allowances, not retrieved distributor quotations. The estimated parts total is USD 63 excluding shipping, labor, computer and taxes. A possible assembled kit target is USD 100–140 only if interviews show value; no sale price or availability is promised.

A closed contact draws approximately 3.3 V divided by 11 kilohms, or 0.30 mA. The MCU-side low voltage is approximately 0.30 V. All four closed draw about 1.20 mA from the 3.3 V rail. The custom carrier dissipates approximately 4 mW in this condition. Each 10k resistor dissipates approximately 0.9 mW, comfortably below its nominal rating. USB consumption is dominated by the Nucleo and debugger, not the carrier. Reserve 100 mA at 5 V as a provisional system budget and measure the actual board before making any battery-runtime claim. Revision A has no battery and no deep-sleep promise. All regulator, reset and MCU decoupling components are on the purchased Nucleo; passive-carrier ERC does not verify them.

## Firmware behavior and design decisions

The firmware is freestanding C11 with a real vector table, reset handler, linker script and direct register drivers. The portable engine owns all mutable measurement state in the foreground. The SysTick interrupt only increments a tick counter. USART transmission uses a bounded 256-byte queue and sends opportunistically rather than blocking acquisition. A 100 ms transmit deadline discards stalled pending output and increments a loss counter. A status is attempted every 250 ms. There is no dynamic allocation, filesystem, background networking, or external SDK download.

Each complete four-bit input vector must remain unchanged for five consecutive one-millisecond intervals after its first observation. Whole-vector qualification avoids manufacturing an artificial ordering by independently debouncing different inputs. Its tradeoff is that chatter in one switch delays acceptance of the others. An acquisition interval other than exactly one millisecond increments the gap counter, cancels qualification age, and permanently loses position for the current boot. It does not pretend an unsampled interval was stable. Ordinary 32-bit tick wrap is handled by unsigned differences; a missed full timer revolution cannot be inferred and is outside the operating assumption.

Positive quadrature progression is 00, 01, 11, 10, 00, where A is bit zero. The engine maintains position modulo sixteen. Station numbers are position divided by four only when position is a multiple of four and the known flag is set. Reverse rotation is supported, but the workflow expectation for consecutive accepted tool episodes is station zero, one, two, three, zero. A repeated station increments repeats. A different station other than the expected next increments skips, which means out-of-order transition count, not an exact count of missing printed pieces.

Homing is deliberately operator-controlled. After power-on, align the physical station-zero mark, arrange A and B open, engage registration, park the tool, then press the Nucleo reset button. The board captures and clears RCC reset flags. Only a pin reset without power, brownout, software, watchdog or low-power reset flags can authorize initial homing. Automatic watchdog recovery therefore cannot silently relabel station one as station zero. The first qualified vector must also equal 12. If it does not, that boot remains position-unknown. The firmware cannot prove that the operator chose the correct physical mark; this remains a documented setup obligation.

A tool pickup begins an episode on a qualified parked-to-unparked transition. Registration must have been engaged before pickup and remain engaged in the arriving vector. Pickup simultaneous with registration engagement is rejected because ordering is unknowable. Any accepted quadrature movement during the episode, loss of registration, invalid quadrature jump, or sampling gap marks the episode bad. Returning the tool completes the episode. Good episodes increment the relevant station count; bad episodes increment rejected. Time is accumulated with saturating arithmetic, and episodes reaching 60 seconds are overdue and rejected on completion. Counts also saturate rather than rolling over into deceptively small values. Reset clears RAM counters; there is no flash-backed history.

These decisions prioritize explicit uncertainty over uninterrupted counting. A high-resolution optical encoder with interrupt capture could support faster movement, but would require different input electronics and different filtering. BLE would add pairing, radio firmware and mobile-platform dependencies without improving this fixed workbench use case. The simple wired, receive-only arrangement is easier to audit and keeps the custom board electrically conservative.

## Protocol and companion application

Every record has the following ASCII form, with unsigned decimal fields and a trailing line feed:

```text
SO1,sequence,now,flags,levels,position,episode_ms,last_duration,accepted,rejected,repeats,skips,quad_faults,motion_faults,registration_faults,gaps,transport_losses,station0,station1,station2,station3*CCCC\n
```

CCCC is four uppercase hexadecimal digits containing CRC-16/CCITT-FALSE over the bytes from S through the last decimal digit, excluding the asterisk. The polynomial is 0x1021, initial value 0xffff, without reflection or final xor. Flags are ready=1, known=2, active=4, bad episode=8 and overdue=16. The SO1 marker also identifies the fixed four-station/sixteen-edge policy; incompatible future policies need a new marker. Sequence and uptime wrap modulo 32 bits. Cumulative counters saturate. There are no commands, settings packets, or transport acknowledgements.

Run `python3 app/app.py` on Linux with Tk installed. The Connect / Live screen accepts a serial path, opens it at 115200, shows current station and episode totals, and displays provenance and freshness. History / Export retains up to 2000 accepted telemetry rows. Setup / Diagnostics explains installation, calibration and limitations. Replay opens a captured telemetry text file up to 1 MiB; the supplied C fixture is synthetic test evidence, not a measurement from a press. Replay is always labeled not live.

The parser checks complete field count, numeric bounds, policy ranges and CRC. It resynchronizes on a full SO1 marker so a truncated line cannot consume the following valid record. Input buffering is bounded. Sequence gaps are counted, while backwards sequence, backwards uptime or decreasing cumulative counters block the session as a suspected reset or reordering event. The user should export and explicitly open a new session rather than merge incompatible histories. Without a persistent boot identifier, some resets with forward-looking values remain indistinguishable. This is a heuristic warning, not guaranteed restart detection.

Disconnect retains history and original live/replay provenance. Opening a new successful connection or replay clears old numeric summaries and history. No received status for two seconds is stale; disconnected and replay data are never represented as fresh live evidence. CSV export uses exclusive creation, refuses overwriting, requests owner-only file permissions, and includes author, origin, connection state, blocked state, parser errors, sequence losses, age and warning text alongside every measurement. Filenames and storage location are chosen by the operator. Data remains local unless the operator shares the files.

## Build, CAD regeneration and verification

From this device directory, run `make -C firmware check` to compile and link the actual Cortex-M4 target and execute host tests with address and undefined-behavior sanitizers. The build also generates synthetic records with the production C encoder. Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` to test the parser and real Tk event loop through Linux pseudo-terminals. No package installation is needed beyond Python/Tk and the named system tools. The flash target uses OpenOCD and ST-LINK but was not exercised without physical hardware.

The committed KiCad files target KiCad 9. Run `/usr/bin/python3 tools/generate_hardware.py` with the distribution pcbnew bindings to regenerate the authored resistor/header symbols, native schematic, routed PCB, project settings and connectivity manifest. Run `/usr/bin/python3 tools/check_hardware.py` to execute real ERC and DRC, export an actual schematic XML netlist, and compare every pin, net, value and footprint against actual PCB pads and the manifest. The simple symbols are real locally authored passive devices; they are not empty placeholders for an absent processor. Inspect the full assembly, including the external Nucleo, separately.

See VERIFICATION.md for the executed results and exact limits. Passing software tests and CAD checks does not validate physical contact timing, cam fit, switch force, ESD survival, workmanship, host-driver behavior or print results. The principal next engineering step is a bench assembly with controlled synthetic switch patterns followed by a supervised manual-carousel trial, not an unattended production installation.

## Commissioning, failure modes and non-goals

Start with the USB cable disconnected. Inspect continuity and resistor values, then wire only isolated voltage-free switches. Verify J1 polarity before powering. Measure 3.3 V, open-input voltage and closed-input voltage before attaching MCU signal leads. With power removed again, complete the harness. After boot, align the home mark and press reset as described above. Confirm position zero and known status, rotate one station in the intended direction, and confirm position four. Reverse one station and confirm return to zero. Test pickup, registration release, tool return, and deliberate excessive-speed movement before recording any real work.

A broken input lead generally looks like an open switch, not a separately diagnosed wiring fault. A short can look like a permanent closure. At startup, manually exercise all four channels and observe the raw level mask in exported data. A stationary stuck encoder may not be diagnosable. A full legal sequence missed between samples can also escape invalid-transition detection. Therefore no absence of faults proves correct movement. Keep leads short, approximately half a meter or less as a conservative bench target, route them away from power wiring, and evaluate noise on the actual assembly.

This design has no galvanic isolation, surge arresters, certified ESD protection, reverse-polarity protection or emergency-stop function. A one-kilohm resistor is not a mains barrier. Do not connect it to powered sensor outputs, press controllers, heaters, mains switches, pneumatic controls, or safety interlocks. Use only a purely manual mechanism with independent mechanical safeguards. Secure every cable outside pinch and sweep zones. Never use it to authorize a print stroke, release a guard, evaluate operator safety, or certify process compliance.

No clinical, child-safety, RF or hazardous-energy use is intended. Any commercial assembly still needs the applicable electrical, EMC and product-safety assessment for its market. Privacy is improved by absence of cameras and microphones, but timestamps and workflow records can still reveal worker behavior; obtain appropriate consent and minimize retention. The device measures neither ink coverage nor registration error in millimeters, does not identify garments, cannot distinguish cleaning from printing, and does not replace operator inspection. These boundaries are part of the design rather than unfinished claims hidden behind the word prototype.
