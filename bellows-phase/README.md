# Bellows Phase — stroke-correlated valve-position notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored hardware sources are MIT licensed. Installed KiCad footprint material retains its upstream licensing; see LICENSE.

Revision A is an engineering prototype. It contains target-linked firmware, a working local companion, and a routed passive carrier. It has not been fabricated, flashed, assembled on a bellows, calibrated, or physically tested. The MCU, regulator, USB bridge, debugger and decoupling are on a separately purchased NUCLEO-F401RE, not on the custom PCB. Valve indications are mechanical proxies, not measurements of airflow, pressure, leakage or pumping efficiency.

## Purpose, audience and useful outcome

A maker experimenting with a small hand bellows can feel weak output without knowing whether the inlet flap is late to close, the outlet flap sticks, or the operator reverses before completing a stroke. Watching one flap misses the other. Counting squeezes gives no indication of how the valves were positioned during compression versus expansion. A video recording can answer more questions, but only if both flaps and both stroke endpoints remain visible and someone annotates the footage afterward.

Bellows Phase is a removable four-contact witness for this specific experiment. Two contacts mark the compressed and expanded endpoints of a slow hand bellows. Two further contacts indicate that its inlet and outlet flap levers have reached an operator-defined open region. The instrument identifies a departure from a witnessed endpoint, distinguishes compression from expansion, and accumulates observed intervals in four valve categories for each direction: neither indicated open, inlet only, outlet only, and both indicated open. It separately counts completed strokes and abandoned attempts. The result is a directional valve-position notebook that can be compared before and after changing flap stiffness or hinge geometry.

The primary user is an educational fixture builder studying a transparent, room-temperature air bellows. A secondary user is a craft-tool experimenter comparing replaceable flap materials on an unpowered bench assembly. The measurable experimental outcome is the distribution of observed valve categories during strokes, together with completed/abandoned stroke counts and latest completed duration. A future physical trial should compare these indications with synchronized video; no improvement in output or diagnostic accuracy has yet been demonstrated.

Today a builder often checks valve timing by replaying a side-view video, which fails when one flap is hidden or the camera cannot see the stroke limits. This device instead captures the four witness contacts together and keeps explicit timing coverage. It should not replace video for first commissioning: video is needed to establish that the selected magnetic indication actually follows the flap rather than merely the linkage.

Non-goals are controlling a pump, measuring volumetric efficiency, certifying check-valve sealing, protecting a furnace, supporting breathing apparatus, or diagnosing any medical condition. Never use this instrument on fireplace bellows near heat, compressed-gas equipment, combustion systems, hazardous vapours, powered machinery, or any system where a misleading observation could cause injury. There are no actuator outputs, pressure-rated fittings or safety interlocks.

## Originality and repository comparison

Repository directories and README concepts were inventoried before selecting this design. Lay Count associates spindle index pulses with winding traverses and tension-arm dropout; it does not observe paired valves. Siphon Witness observes constrained liquid-level transitions in two vessels, not internal flap position during alternating air strokes. Sieve Shuttle describes directional motion categories, while Ratchet Audit observes mechanism rollback. Bellows Phase changes the physical experiment, the measured relationship, the trial semantics and the application interpretation rather than merely renaming an existing device.

Bellows and check valves are established mechanisms. Four-channel data acquisition is also established. The contribution proposed here is the compact, open, direction-conditioned valve-position workflow, including conservative exclusion of unsettled boundary intervals and retention of evidence from abandoned strokes. Worldwide novelty, patentability and absence of comparable commercial tools have not been established. The design is original relative to the inspected repository's device purposes, not a claim to have invented pneumatic pumping.

The electrical carrier and MCU foundation deliberately reuse jayis1 sources from lay-count at commit 14a3ee6. That provenance is explicit in tools/import_platform.py. The new engine, BP1 protocol field map, tests, notebook model and bellows fixture specification are local to this directory. Shared circuitry is not represented as a new electrical invention. The import helper refuses to overwrite existing destination files; it is a provenance/build aid, not a step required for normal users.

## Hardware specifications and architecture

The processor is ST STM32F401RET6, a Cortex-M4 with 512 KiB flash and 96 KiB SRAM, supplied on NUCLEO-F401RE. Firmware runs on the internal 16 MHz HSI clock and samples GPIO inputs on one-millisecond foreground ticks derived from SysTick. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit, through PA2 alternate function 7 to the development board's ST-LINK virtual COM port. There is no radio, FPGA, battery, charger, external flash or target-side USB implementation.

```text
hand-operated air bellows
  compressed endpoint ---- COMPRESSED ---- J3
  expanded endpoint ------ EXPANDED ------ J4
  inlet flap open proxy -- INLET --------- J5
  outlet flap open proxy - OUTLET -------- J6
                                            |
                     passive carrier: 10k pullups, 1k series paths
                                            |
                    PA0 / PA1 / PA4 / PB0 on NUCLEO-F401RE
                                            |
             independent qualification -> stroke engine -> eight bins
                                            |
                  USART2 TX -> ST-LINK USB -> local Linux/Tk app
                                            |
                      live notebook / replay / private CSV
```

The custom board is 80 by 70 mm, two-layer FR-4, nominally 1.6 mm thick. It uses 0.30 mm traces, a 0.20 mm default clearance, through-hole resistors and standard 2.54 mm headers. The delivered schematic includes every carrier component; the external Nucleo is an explicitly separate assembly rather than an unconnected decorative MCU symbol. Fourteen components, thirty connected pads and ten nets are checked against both the schematic-exported netlist and actual PCB pads.

All field sensors are normally open, voltage-free magnetic reed contacts suitable for low-level switching. A captive magnet attached to each endpoint witness or valve lever closes the corresponding contact in its defined region. Choose a low-force lever extension so the magnet and linkage do not materially alter valve dynamics. Contacts are external assemblies, not integrated onto the PCB. No exact sensor sensitivity or repeatability is promised: procurement must select and characterize a reed/magnet combination appropriate to the fixture and approximately 0.3 mA contact current.

## Pin map, supply and electrical boundaries

J1 pin 1 is VCC, connected only to the Nucleo 3V3 source at CN6 pin 4. J1 pin 2 is GND, connected to CN6 pin 6. J2 pins 1–4 connect respectively to CN8 pins 1–4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These Arduino analog labels identify digital inputs in this project. Square PCB pads indicate pin 1. Every field connector has its signal on pin 1 and ground on pin 2.

| Input bit | Contact | Field header | MCU signal | Pullup / series |
|---|---|---|---|---|
| 0 | COMPRESSED | J3 | SENSE0 / PA0 | R1 10k / R2 1k |
| 1 | EXPANDED | J4 | SENSE1 / PA1 | R3 10k / R4 1k |
| 2 | INLET open | J5 | SENSE2 / PA4 | R5 10k / R6 1k |
| 3 | OUTLET open | J6 | SENSE3 / PB0 | R7 10k / R8 1k |

Each pullup connects the input-side SENSE net to 3.3 V. Its series resistor connects SENSE to the external contact. An ideal closed contact draws 3.3/11000 amperes, approximately 0.300 mA, and leaves approximately 0.300 V at the MCU input. Four closed contacts draw approximately 1.20 mA. These are nominal circuit calculations, not measurements. Internal MCU pull resistors are disabled. No analogue filtering capacitor is fitted because unmatched filters could alter relative timing.

Power the development board through its normal USB arrangement. Reserve 150 mA at 5 V as a planning allowance for the development board, with the additional small carrier load on its 3.3 V rail. This is not a measured consumption or battery-life specification. Do not connect a second supply to J1 while the Nucleo powers it. Consult the actual MB1136 revision before selecting jumpers or changing solder bridges. The reused platform assumes the ST-LINK USART route associated with SB13/SB14 connected and SB62/SB63 open; verify against the purchased board.

Only disconnected, voltage-free contacts are allowed. Never connect existing powered switch wiring, PLC inputs, 5 V sensor outputs, mains wiring or an unknown industrial transducer. The carrier has no isolation, TVS network, industrial ESD protection or certified hot-plug behaviour. A 1k series resistor is not a protective isolation barrier. USB ground is common with every contact. A broken wire can look like an open valve indication and is not independently diagnosed. Keep initial field leads below 0.5 m and verify noise margins with the actual harness.

Design references for the reused platform are ST UM1724 (NUCLEO-64 MB1136), RM0368 (STM32F401 peripheral registers) and PM0214 (Cortex-M4 programming). Obtain these manufacturer documents and match them to the board before assembly. This run verifies the reused implementation through target compilation and simulated register tests, not through fresh electrical measurements or a new manufacturer qualification.

## Bill of materials and mechanical arrangement

| Item | Quantity | Specification |
|---|---|---|
| R1/R3/R5/R7 | 4 | 10k, 1%, axial DIN0207, 7.62 mm pitch, at least 0.125 W |
| R2/R4/R6/R8 | 4 | 1k, same tolerance, package and rating |
| J1/J3/J4/J5/J6 | 5 | 1x2 straight 2.54 mm pin headers, insulated mating harness |
| J2 | 1 | 1x4 straight 2.54 mm header, labelled harness |
| Carrier | 1 | Supplied two-layer 80x70 mm PCB |
| Processor assembly | 1 | ST NUCLEO-F401RE |
| Witness assemblies | 4 | Low-level normally open reed contacts and captive magnets |
| Fixture | 1 | Transparent low-pressure hand bellows with accessible flap extensions |
| Packaging | 1 set | Insulating edge supports, enclosure, strain relief and USB cable |

Budget approximately USD 25–40 for the Nucleo, USD 10–25 for a small-quantity carrier and passive population, and USD 20–50 for contacts, magnets, harness and enclosure. These are planning allowances, not supplier quotations or stock claims. The computer, bellows, labour, custom brackets, tooling, shipping and taxes are excluded. The device uses standard components, but the mechanical witness installation is necessarily fixture-specific.

For an initial experiment, choose a small transparent bellows that moves slowly by hand and vents freely to room air. Fit rigid end stops defining separate expanded and compressed regions. Arrange their magnets so both endpoints cannot close at once. Mount the valve witness on a light external extension of each flap, with the reed fixed to the frame. Make the open threshold visible with a mark so video can establish whether the indication is meaningful. Do not attach heavy magnets directly to a thin valve without measuring the resulting disturbance.

The delivered electronics enclosure allowance is roughly 160 by 110 by 45 mm for the carrier and separate Nucleo, not released mechanical CAD. The carrier has no mounting holes; use insulating edge guides rather than drilling through copper. Keep wires away from hinges, add strain relief, retain all magnets and avoid pinch points. Use a dry indoor supervised bench, initially around 10–35 degrees Celsius. No ingress-protection, shock, thermal or regulatory qualification is claimed.

## Measurement contract and firmware decisions

A closed contact maps to an asserted input bit. Each input qualifies independently after five consecutive matching one-millisecond samples. The first sample counts as one, so qualification spans four observed intervals. Short pulses may be rejected and pulses between samples may be invisible. Begin physical testing with strokes lasting at least one second and contact states held for at least 20 ms. The sample rate does not establish a maximum safe mechanism speed.

WAIT means no origin is available. A unique qualified endpoint, matching the raw endpoint and with all four inputs initially qualified, establishes READY. The valve states need not be favourable to arm: otherwise the recorder would systematically exclude the very fault being studied. A qualified departure with both raw endpoints clear begins ACTIVE and increments starts. Leaving expanded means compression, direction zero; leaving compressed means expansion, direction one. Direct endpoint swaps without an observed middle region do not create a stroke.

During ACTIVE the wall duration advances on each consecutive sample. A valve bin receives one millisecond only when both adjacent samples have both endpoints raw and qualified clear, all inputs qualified, raw values matching qualified values, and identical valve states. Consequently a change in flap indication, an endpoint boundary or even a sampled raw glitch creates an uncredited interval rather than being retrospectively assigned to a convenient category. The first active sample earns no earlier travel time. Bins are conservative observed interval totals, not interpolation across uncertain periods.

Each direction has bins in this order: neither open, inlet only, outlet only, both open. Compression ordinarily expects outlet only; expansion ordinarily expects inlet only. These are descriptive expectations, not hardwired safety thresholds. Neither-open time can be legitimate while pressure rises, and both-open indications do not prove reverse flow. The recorder intentionally avoids issuing a pass/fail efficiency score. Totals include valid observations from strokes that are subsequently abandoned; they are not restricted to completed strokes.

Reaching the opposite endpoint completes a stroke only when its raw endpoint agrees. Returning to the originating endpoint abandons the attempt. Duration expires at 60000 ms, and timeout wins over arrival on that same sample. Completion or abandonment clears active duration and permits immediate rearming if a valid endpoint is already present. The latest completed duration and direction remain retained until another completion. Starting in mid-travel cannot invent a direction, and a timeout in mid-travel requires a new endpoint before another attempt.

Both endpoints simultaneously raw or qualified asserted are a terminal geometry/wiring fault. A skipped acquisition millisecond is a terminal continuity fault. An active stroke is abandoned exactly once before HALTED. Counter updates are staged in a copied state so exhaustion rolls back all prospective accounting before terminal abandonment. The conservation rule is starts = compression completions + expansion completions + abandoned + one if ACTIVE. Once halted, accounting remains fixed; raw inputs, time and UART diagnostics can still update. Removing power or resetting loses all volatile state.

The firmware is freestanding C11 with actual GPIO, RCC, USART, SysTick and independent-watchdog register access. Clock setup has bounded waits before communication starts. The UART driver is nonblocking, buffers a complete record, rejects concurrent submissions and expires stalled transfers after 100 ms. The foreground feeds the watchdog after sampling. Peripheral configuration and watchdog-start failures halt the measurement engine. Startup initializes data and BSS; the linker reserves the actual flash/SRAM ranges and stack headroom. No readings are simulated in the target firmware.

## Protocol and application interface

BP1 is a fixed-policy ASCII status record, emitted approximately every 100 ms. Fields are unsigned decimal, in this exact order:

`time,raw,levels,known,state,anchor,starts,compression,expansion,aborted,active_ms,bin0,bin1,bin2,bin3,bin4,bin5,bin6,bin7,last_ms,last_dir,flags,gaps,tx`

Each line starts with `BP1,` and ends with `*` plus four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xffff, without reflection or final XOR, over the marker and decimal fields. The complete line is bounded below 512 bytes. State values are zero WAIT, one READY, two ACTIVE, three HALTED. Anchor values one/two identify compressed/expanded. Flag bits are one gap, two conflicting endpoints, four exhaustion, eight configuration failure and sixteen watchdog setup failure. Time is modular uint32 milliseconds; bin counts are milliseconds of credited intervals.

The companion is a Linux Python/Tk application with three working tabs: Connect / Replay, Stroke / Valves, and History / Export. It discovers common USB serial paths, opens a selected path, shows contact bits and directional totals, lists the latest 200 retained snapshots and exports evidence. It does not issue commands, alter device thresholds, silently reset counters or require a cloud account. Fixed policy makes a separate configuration screen unnecessary; changing policy requires a reviewed firmware/protocol revision.

Launch `python3 app/workbench.py` from this directory in a graphical Linux session. Python 3 and Tk are sufficient; no pip packages or mobile build system are required. Connection failure preserves existing evidence. A second connect request does not reopen an owned serial port. Disconnect restores terminal attributes when possible and retains provenance. Opening a valid replay replaces the session only after full validation. Export before replacing a session if its evidence is needed.

Replay accepts only regular files up to two MiB, validates CRC and state relationships, and rejects empty files, device nodes, FIFOs and partial final records. Live framing resynchronizes on the full BP1 marker after corruption. Storage retains at most 6000 snapshots with a dropped-snapshot count. CSV export uses exclusive creation and mode 0600, refuses overwrite, and includes author, source, connection state, freshness, parser errors, warnings and retention metadata even for an empty session.

Live status requires a connected serial transport and two advancing timestamps. Duplicate snapshots do not refresh progress. More than one second without device progress makes evidence stale. A reception pause exceeding one second or cumulative device/host timing disagreement above 750 ms blocks resumed live interpretation until reconnect. This fixed allowance can conservatively reject long sessions because HSI drift is not calibrated. Rollbacks, disappearing faults, contradictory duplicates and impossible exposure increments also block the session. These checks are necessary consistency bounds, not a proof of every possible hidden contact history. Without a persistent boot identifier, some resets that appear forward remain indistinguishable. CRC is error detection, not authentication.

## Build, commissioning and verification

Run `make -C firmware clean all test` to build the Cortex-M4 ELF/binary and execute engine, clock and serial-driver tests under AddressSanitizer and UndefinedBehaviorSanitizer. Run `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v` to exercise the companion, including real Tk widgets and Linux pseudo-terminals. Fixtures come from the actual C encoder and are synthetic inputs, not bench measurements. `make -C firmware flash` uses OpenOCD and an attached ST-LINK; flashing has not been performed in this run.

Regenerate the carrier with `/usr/bin/python3 tools/generate_hardware.py`, requiring KiCad 9 bindings and standard footprints. Run `/usr/bin/python3 tools/check_hardware.py` for actual ERC, DRC and exported-netlist comparison. Passive-only ERC does not validate the purchased processor's power system. The schematic is rendered and visually checked as an additional legibility check, not a substitute for electrical review.

On day one, verify resistance and continuity before connecting USB, then exercise each contact individually and confirm its bit. Check both endpoint overlap detection and recovery by explicit reset. Run slow compression and expansion strokes while filming both flaps, comparing expected category changes with the notebook. Introduce a same-end return and a deliberately stuck flap. Confirm that the app does not label an abandoned stroke a completed one and that raw glitches remove coverage rather than fabricate observations.

After a month, repeat the witness threshold and magnet alignment checks. Export records with notes about flap material, geometry, operator and enclosure changes. When power is lost there is no battery backup: counters reset and the app must start a new deliberate session. Before relying on comparisons, measure actual acquisition timing, contact bounce, cable noise, watchdog behaviour, regulator margins and fixture disturbance. VALIDATION.md records software/CAD checks actually run. Passing them establishes a reproducible prototype baseline, not fabrication readiness, calibrated pneumatic performance or safety certification.
