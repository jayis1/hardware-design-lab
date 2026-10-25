# Swing Envelope — bilateral pendulum threshold and crossing notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources: MIT.

Revision A is a build-tested engineering prototype, not a physically calibrated instrument. The delivered custom PCB is a passive four-contact carrier used with a separate NUCLEO-F401RE. It is not a standalone MCU board. No fabricated hardware, flashed target, or real pendulum has been tested for this revision.

## Purpose, overview, and primary user

Swing Envelope helps an experimental clock builder or physics instructor answer a question that a single period display conceals: when the period changes, is the pendulum still reaching the same left and right excursion bands? It records directional passage through two inner gates, the elapsed time between successive rightward passages, and whether each intervening left or right excursion visited an outer threshold. It does not estimate a continuous angle from four switches. It makes the distinction between observed timing and binary amplitude evidence explicit, including when that evidence becomes invalid.

The primary user is a teacher comparing a lightweight demonstration pendulum before and after adding a removable paper air vane. Today the teacher might time ten swings with a stopwatch, then ask a student whether the amplitude seemed smaller. With this device, a local notebook records period alongside completed excursions that did or did not reach each outer threshold. In the first week, the teacher can repeat the same release procedure with two vane sizes and look for the point where one side stops reaching its threshold. The instrument replaces handwritten tally marks and a second observer, not the teacher's explanation of damping or a precision laboratory timer.

A clockmaking hobbyist is a secondary user. An asymmetric escapement intervention can be associated with changed directional gate transit times or unequal threshold retention. That is a diagnostic observation, not proof of a bad escapement, a torque measurement, or a recommended adjustment. A museum demonstrator could also use the receiver's live screen to explain why constant period does not imply constant amplitude. The design has no motor, brake, automatic correction, or safety function.

## Originality, alternatives, and neighboring designs

Repository directories and existing README concepts were inventoried before selecting the design. A search of all README files found no pendulum or escapement concept. The closest measurement projects are ContactAtlas, which maps disconnected rotary-switch signatures; WebSlack Ledger, which compares encoder travel along a hand-fed strip; and Slip Window, which decomposes a casting cradle's residence, draining, and rest. None provides directional central crossing pairs plus side-specific amplitude-threshold evidence for pendulum experiments. This is a new experiment contract, not a claim that passive resistor interfaces are novel.

Adjacent public examples include the PASCO Smart Timer pendulum demonstration and Vernier's “Pendulum Periods” experiment. Vernier explicitly investigates period versus amplitude, so the general scientific question is not new. Swing Envelope's proposed distinction is the small receive-only notebook that automatically pairs a period observation with completed left/right threshold visits and preserves uncertainty after ambiguous motion. A configurable laboratory acquisition system could implement the same experiment. Worldwide novelty, patentability, and absence of equivalent products have not been established.

References for the comparison are https://www.vernier.com/experiment/pwv-14_pendulum-periods/ and https://www.youtube.com/watch?v=bgG_PTVdEjw . A camera-based alternative could reconstruct continuous amplitude, but requires image processing, lighting and geometric calibration. A rotary encoder at the pivot could deliver better angular information, but can change the friction and inertia of an existing educational pendulum. This revision instead uses four adjustable magnetic contact zones, accepting their own measurement disturbance and limited resolution rather than hiding those limitations.

## Hardware specifications and architecture

The processor is the STM32F401RET6, Cortex-M4, on a purchased ST NUCLEO-F401RE MB1136 development board. Firmware uses its reset-default 16 MHz HSI oscillator, SysTick, GPIOA/GPIOB, USART2 and independent watchdog. Flash capacity is 512 KiB and RAM is 96 KiB. The development board provides regulation, decoupling, SWD, reset, the ST-LINK programmer and USB virtual COM connection. Those circuits are not represented as components on the passive carrier because they are not built on that carrier.

The custom board is an 80 by 70 mm, two-layer, 1.6 mm FR-4 interface with fourteen through-hole components. Four 10 kilohm pull-up resistors and four 1 kilohm series resistors serve four external dry contacts. There are five two-pin headers and one four-pin header. Standard 2.54 mm connectors and DIN0207 axial resistor footprints make manual assembly possible. The board is connected by short wires; it is not an Arduino-footprint shield and must not be plugged directly onto the Nucleo headers as if its geometry matched them.

| Connection | Net / target pin | Function |
|---|---|---|
| J1 pin 1 | VCC / Nucleo 3V3 | Regulated 3.3 V supply only |
| J1 pin 2 | GND | Common ground |
| J2 pin 1 | SENSE0 / PA0 / A0 | Inner left gate, bit 0 |
| J2 pin 2 | SENSE1 / PA1 / A1 | Inner right gate, bit 1 |
| J2 pin 3 | SENSE2 / PA4 / A2 | Outer left gate, bit 2 |
| J2 pin 4 | SENSE3 / PB0 / A3 | Outer right gate, bit 3 |
| J3–J6 pin 1 | CONTACT0–CONTACT3 | Gate contact signal |
| J3–J6 pin 2 | GND | Individual contact return |
| Nucleo PA2 | USART2 TX | Onboard ST-LINK virtual COM input |

```text
Pendulum magnet, moving on a captive lightweight bob
             outer L    inner L    inner R    outer R
                 |          |          |          |
                J5         J3         J4         J6
                 |          |          |          |
             dry reed contacts, common return GND
                 |          |          |          |
             each CONTACT -- 1k -- SENSE -- MCU input
                                     |
                                    10k
                                     |
                                    3V3

NUCLEO-F401RE: 1 ms tick -> 3 ms stable-vector qualification
                         -> paired crossings and outer excursion ledger
                         -> SE1 CRC-protected status every 250 ms
                         -> USART2 / ST-LINK USB VCP
Linux Python/Tk: Connection | Live envelope | History / CSV
```

A practical sensor family is the normally-open Littelfuse MDSR-7 reed switch, in a protective insulating mount, operated by a firmly retained small magnet. Select sensitivity and magnet geometry experimentally. The sensor's contact rating is not a guarantee of reliable operation at the low current used here. The glass envelope must not carry fixture loads. The device observes magnetic contact closure, not the bob's geometric position directly. Each closure zone must be characterized in both directions because reed operate and release positions have hysteresis.

All contacts must be isolated, voltage-free switches. Never connect powered sensor outputs, mains circuits, or an existing electrical clock drive. The carrier has no galvanic isolation, surge suppressors, protected power input, or industrial cable protection. Keep leads short, indoors, and away from motors and static-prone materials. An open cable looks like an unvisited gate; a short looks like a continuously occupied gate. Neither fault is uniquely diagnosed by the electronics.

At nominal 3.3 V, a closed contact sees approximately 0.30 mA through the combined 11 kilohms, and the MCU sense node sits near 0.30 V. The pathological all-closed electrical case consumes approximately 1.20 mA and 3.96 mW in the complete resistor network, even though that combination is rejected as a measurement. These are calculations, not measured performance. Reserve 150 mA at USB 5 V for the entire Nucleo system as a conservative planning allowance, not a measured average. No battery or runtime claim is made.

## Mechanical installation and experimental limits

Use a guarded, lightweight pendulum with a restrained suspension and a captive magnet. Place the two inner gates on opposite sides of the lowest point, then place one outer gate farther out on each side. Keep all four active zones disjoint: no two switches may be qualified closed together. The bob must clear the rigid switch mounts without touching them throughout its allowed motion. Attach switches to the stationary frame; avoid changing the moving mass except for the retained magnet. Fix wires away from the swept volume and provide strain relief at every connector.

The supplied electronics do not specify a universal angular scale or a printable mount that suits every pendulum. Record the actual inner spacing, outer offsets, magnet orientation, bob dimensions, suspension length and mounting photograph with each experiment. Begin with a slow hand pass and confirm each gate closes separately. The qualified closure must last at least four sampling observations, and the all-open interval between gates must also qualify. A sensible first experiment uses a pendulum with a period of at least one second and measured closure dwell comfortably above ten milliseconds. This is a commissioning guideline, not a verified operating range.

Magnetic attraction to reeds and nearby steel can perturb the motion. Repeat an independent optical or video observation with and without the sensor fixtures before treating this as a minimally invasive instrument. Reed hysteresis, debounce delay, HSI error and sample scheduling all affect timestamps. Directional gate transit times include different magnetic thresholds and must not be interpreted directly as directional velocity without independent geometry and timing calibration. The outer results are only reached/not-reached observations; neither a missed visit nor a reached visit gives the maximum angle.

This is not a certified safety, timekeeping, or metrology product. Do not use a heavy swinging mass, public unguarded installation, vehicle, industrial machine, or valuable clock as the first test subject. The PCB has no mounting holes; use insulating edge supports or an enclosure tray rather than drilling through routed copper. Complete electrical inspection and continuity testing before connecting the purchased MCU board.

## Firmware details and design decisions

The firmware is freestanding C11 with explicit reset initialization, a linker script, the Cortex-M vector table, direct register definitions and two peripheral drivers. `registers.h` names the STM32F401 peripheral registers used from ST RM0368; `board.h` declares the connector contract and public driver interface. `drivers/board.c` initializes the reset clock assumptions, inputs, USART alternate function, SysTick and watchdog. `drivers/serial.c` owns a bounded transmit buffer and reports transmission timeouts. No vendor SDK, heap allocator, floating-point library or networking stack is required.

The foreground loop samples the complete four-bit input vector when the millisecond tick advances. Three milliseconds of unchanged observations qualify a vector. Whole-vector qualification deliberately avoids interpreting separately debounced inputs as a physical ordering that was never observed. A scheduling interval greater than two milliseconds increments a gap counter, invalidates interval evidence, and requires a fresh all-open qualification. It does not invent stable contact time across the gap. The interrupt only increments a tick; the measurement engine and transmitter have one foreground owner.

A qualified inner-left closure followed by a qualified inner-right closure completes a rightward crossing. The reverse pair completes a leftward crossing. Each pair is consumed, so the normal inner event sequence is left, right, right, left, left, right. A repeated first gate within an unfinished pair is ambiguous and becomes a fault rather than an assumed bounce or reversal. Consecutive completed crossings in the same direction are also faults. Multiple qualified closed zones are invalid. Recovery requires a qualified all-open vector, followed by new crossing evidence.

After a rightward crossing, visits to the outer-right gate are remembered until the next leftward crossing completes that excursion. Multiple visits during the excursion count only once. Its completion increments either right-reached or right-missed. The equivalent process applies on the left. The initial partial excursion has no outcome; starting a recording near an outer gate does not claim a complete swing. An outer event on the wrong side or during an unfinished central pair is inconsistent with the required geometry and invalidates continuity.

A period is the interval between consecutive rightward crossing completions with a valid intervening leftward crossing. Pending pairs expire after ten seconds, and cycle or idle intervals expire after sixty seconds. Saturating counters and ages never wrap into plausible low values. Normal millisecond timestamp wrap is handled by unsigned differences. Previous numerical measurements remain available after invalidation, but the period-valid flag clears; consumers must not present a historical value as current evidence. Power loss erases all RAM history. There is no flash wear, persistent trial counter, or autonomous storage beyond current cumulative counters.

## Application, protocol, and data handling

The appropriate companion for a bench experiment is a Linux Python 3 Tk desktop application, not a mobile radio application. Run `python3 app/app.py`. Its Connection tab selects a serial device, connects, disconnects and loads a bounded local replay. The Live envelope tab shows crossings, period validity, last directional transit times, bilateral reached/missed counts and continuity failures. History displays the most recent 200 received rows; the model retains up to 5,000 rows for explicit CSV export. The app never sends a command to the MCU.

Transport is 115200 baud, eight data bits, no parity, one stop bit, through the Nucleo's onboard ST-LINK VCP. Check the MB1136 routing in ST UM1724: the default USART2 VCP solder-bridge configuration must be retained. The receive-only MCU firmware does not initialize a command channel. There is no universal password or fictitious pairing exchange. Physical possession of the USB serial path determines who can observe or forge experiment traffic; the CRC detects corruption, not malicious alteration.

Each line has the ASCII form `SE1,<17 unsigned decimal fields>*<four uppercase hexadecimal CRC digits>\n`. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xFFFF, covering every byte before the asterisk. There is no terminating NUL on the wire and no carriage return. Fields, in order, are sequence, uptime_ms, flags, levels, direction, crossings, period_ms, east_ms, west_ms, left_seen, right_seen, left_missed, right_missed, faults, gaps, timeouts, transport_losses. Direction is zero unknown, one rightward, two leftward. Flags are bit zero ready, bit one period valid, bit two pending central pair, and bit three blocked after an ambiguity. Levels use the connector bit order above.

Every status is self-describing through the SE1 marker; late connection does not depend on a lost boot announcement. Firmware emits status every 250 milliseconds when the transmitter is available. A transmit operation has a 100 millisecond deadline and failed or rejected submissions increment transport_losses. The app rejects oversized lines, wrong versions, invalid enums, CRC mismatches and intervals outside firmware policy. It resynchronizes on the full SE1 marker after a truncated line. Invalid input cannot grow the frame buffer indefinitely.

A sequence or time regression, or a decrease in a cumulative counter, blocks new rows as a suspected reset until the user starts a new session. Sequence and timestamp wrap are accepted using modular comparisons. This is a heuristic: without persistent boot identity, a reset whose new counters and timestamps all appear to advance can remain indistinguishable. CSV exports preserve provenance, parser errors, missing-frame estimates, connection state and each row's age. Disconnect preserves evidence and its source; reconnect clears it. More than two seconds without valid live telemetry is stale. Replay is always explicitly labelled non-live, even for a file that was originally captured from real hardware.

Exports use exclusive file creation with mode 0600 and refuse overwrite. There is no automatic upload, telemetry collection, network listener or dependency download. The application uses only the standard library plus the distribution's Tk installation. An attacker controlling the serial device or replay file can supply false measurements but cannot legitimately command an actuator because none exists. Debug access and updates use physical ST-LINK/SWD; signed updates, secure boot and rollback protection are not implemented or claimed.

## Build, checks, and commissioning

Build and exercise the design from this folder:

```sh
make -C firmware all test
xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v
/usr/bin/python3 tools/check_hardware.py
python3 app/app.py
```

Firmware builds with ARM GNU GCC and links an ELF plus a raw binary for STM32F401RE. `make -C firmware flash` invokes OpenOCD with ST-LINK and STM32F4 configuration and requests programming, verification and reset; this command must only be used when the correct target is attached. It has not been exercised on physical hardware in this revision. The test target builds the portable engine under host AddressSanitizer and UndefinedBehaviorSanitizer and generates a synthetic fixture using the actual C encoder. Tests distinguish synthetic switch sequences from real electrical observations.

KiCad 9 sources include embedded authored symbols, a local symbol library, real footprint identities, a routed PCB and a JSON connectivity manifest. `tools/generate_hardware.py` regenerates the carrier using distribution KiCad footprints; it requires the system Python with pcbnew. The checker runs real ERC and DRC, exports a fresh schematic XML netlist, and compares all component values, footprint identities and reference/pin/net connections to actual PCB pads and the manifest. A passing passive-board ERC does not validate Nucleo power circuitry, external wiring or physical sensor performance.

Before a real experiment, inspect soldering, test connector polarity and verify the unpowered board's connectivity. Power from the Nucleo's 3.3 V output only. Confirm each gate individually, then feed a slow left-to-right and right-to-left sweep with all-open separation. Intentionally hold two gates closed and verify fault recovery. Compare a hand-timed sequence with the notebook, then measure absolute timing against a known reference. Remove and replace a sensor to characterize mounting repeatability. Keep an independent record of geometry and environmental conditions because those are not sensed electronically.

## BOM, cost boundary, and non-goals

Budget allowances, not vendor quotations, are USD 20 for a NUCLEO-F401RE, USD 12 for four reeds and protected mounts, USD 15 for a small-quantity carrier with resistors and headers, and USD 18 for cable, magnet and enclosure. The resulting USD 65 electronics target excludes the pendulum stand, computer, assembly labor and calibration apparatus. Purchase quantities and local prices can exceed it. No retail price, supplier availability or regulatory approval is asserted.

Deliberately absent are continuous angular reconstruction, automatic damping estimation, GPS time, Bluetooth, batteries, clock correction, torque sensing and cloud synchronization. These omissions keep the measurement contract reviewable. The most useful next engineering step is physical bring-up with an independently measured slow pendulum, not adding another dashboard. Build, software tests and CAD checks establish consistency of the supplied artifacts; they do not establish metrological accuracy or prove product-market novelty. See VERIFICATION.md for the checks actually executed and their scope.
