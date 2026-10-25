# Crossover Track — lane-swap cart comparison instrument

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored hardware design sources: MIT.

Revision A is an engineering prototype, not a calibrated race timer. The delivered custom PCB is a fully routed passive contact interface for a separate NUCLEO-F401RE. The microcontroller, regulator, USB bridge, programming interface, and decoupling are on that purchased development board. No physical cart, fabricated carrier, target programming, electrical measurement, or classroom trial has been performed. Build and CAD validation establish reproducibility, not measurement accuracy or suitability for safety decisions.

## Purpose, user, and the moment of use

A teacher comparing two homemade gravity carts faces a deceptively simple question: is cart A slower, or is its lane slower? A crooked track, different wheel alignment, unequal start releases, and an enthusiastic student declaring the first visible finisher can all contaminate the answer. Crossover Track makes that distinction the organizing principle of the instrument. It records separate departures and arrivals, then guides a four-run crossover experiment in which the identity occupying the left lane follows A, B, B, A. The right lane always carries the other cart. Its output is a pair of descriptive contrasts: an A-minus-B travel-time effect and a left-minus-right lane effect.

Before using the device, students typically run one race, note the winner, exchange lanes inconsistently, and try to remember whether the release was fair. A stopwatch cannot preserve two independent release times and two arrival times while its operator also watches the carts. Afterwards, the teacher labels the carts, installs four contact flags, checks the live gate page, and follows the lane-placement prompts. Each successful trial records the actual observed release skew, both travel intervals, and the finish ordering. A rejected trial remains visible instead of being quietly replaced by a plausible-looking winner.

The primary user is a supervised school physics or maker-space group experimenting with slow, lightweight carts. Secondary users are workshop educators comparing bearings, wheel materials, or added payloads on an unchanged test track. This is deliberately not a timer for racing competitors, motor vehicles, powered machinery, or high-speed projectiles. Its value is the experimental workflow and the explicit handling of ambiguous observations, not a claim to unprecedented timer resolution.

## Originality and design selection

The repository inventory was checked before design, including the purpose descriptions rather than folder names alone. Nearby concepts include Swing Envelope, which observes pendulum threshold crossings; WebSlack Ledger, which compares encoder travel in strip-feed fixtures; and Pedal Window, which measures piano-key and pedal transitions. None provides the paired-cart ABBA lane-swap experiment or decomposes paired trial differences into cart and lane terms. The electrical carrier topology is intentionally reused from jayis1's existing contact instruments; reuse of a proven interface is not presented as a new electrical invention.

A plain two-lane race timer was the initial candidate and was rejected after a comparable-product search. Arduino Project Hub's [Race Photogate](https://projecthub.arduino.cc/nfarrier/race-photogate-9837a1), Balsatron's [Dual Lane Race Timer](https://www.balsatron.com/products/dual-lane-race-timer), and Trackmate's [dual-lane package](https://trackmateracing.com/shop/en/sports-timers/198-speed-trap.html) already cover independent timing and related race functions. Crossover Track is a new repository design combining qualified contact evidence with the guided crossover analysis; worldwide novelty, patentability, and freedom to operate have not been established. No vendor source code is incorporated.

Other candidates considered were a generic temperature logger, rejected because the repository already has many environmental monitors; a strip-feed differential recorder, rejected because WebSlack Ledger covers that workflow; a motorized cart launcher, rejected because its actuator and mechanical hazards are unnecessary; and a phone-only experiment worksheet, rejected because it cannot acquire the four physical events. The selected instrument keeps all actuation in the students' hands and keeps the electronics passive at its external connectors.

## What the hardware measures, and what it does not

Each lane has a start-presence contact and an arrival contact. At rest, the cart closes its start switch while its finish switch remains open. Departure opens the start contact. Arrival closes the finish contact. The firmware independently qualifies each contact for ten milliseconds, preventing chatter on the other lane from shifting a timestamp. A hundred milliseconds of the complete ready vector is needed to arm the next trial. Contact qualification timestamps are not raw mechanical timestamps: installation geometry, switch hysteresis, magnet position, bounce, and the internal clock introduce errors that must be measured before interpreting small differences.

The hardware is necessary because four contact states can change while nobody is touching a computer. The computer is appropriate for the experimental instructions, stored evidence, and analysis. No radio, account, cloud service, or phone permission is required. The application is a Linux Python/Tk desktop program using standard-library serial access rather than a mobile app with a large Bluetooth stack. That choice keeps the bench workflow inspectable and avoids pretending that a USB development board has wireless hardware.

The device cannot identify a cart. Labels A and B and all lane swaps are operator declarations. It cannot measure velocity without a separately measured distance and a model of cart motion, and it deliberately does not display a derived speed. It cannot prove that a start sensor disconnected, because an open cable looks like an open contact. It does not measure slope, wheel friction, mass, acceleration, or force. A complete four-run block is evidence about those particular trials, not a statistical confidence interval or a causal proof that a wheel material is better.

## Hardware specifications and bill of materials

The controller is an ST NUCLEO-F401RE with an STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware runs from the reset 16 MHz HSI clock without an external oscillator assumption. SysTick supplies a nominal millisecond timebase. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit, through the ST-LINK virtual COM connection. RX is disabled, so application actions cannot command target outputs or alter timing policy.

The carrier is an 80 by 70 mm, two-layer, 1.6 mm board with 0.30 mm tracks, 0.20 mm clearance, 0.80 mm vias and 0.40 mm via drills. It has six through-hole headers and eight axial resistors. There are no batteries, chargers, motors, relays, mains connections, or independently powered sensor rails. Use only the Nucleo's 3.3 V and ground at J1. Do not connect a second power supply to that connector.

| Part | Quantity | Selection and role |
|---|---:|---|
| NUCLEO-F401RE | 1 | Purchased processor, regulated supply, debugger and USB bridge |
| 10 kΩ resistor | 4 | Yageo CFR-25JB-52-10K or equivalent axial pull-up, R1/R3/R5/R7 |
| 1 kΩ resistor | 4 | Yageo CFR-25JB-52-1K or equivalent axial series element, R2/R4/R6/R8 |
| 1x2, 2.54 mm header | 5 | Samtec TSW-102-07-G-S or footprint-compatible J1/J3–J6 |
| 1x4, 2.54 mm header | 1 | Samtec TSW-104-07-G-S or footprint-compatible J2 |
| Dry-contact sensor | 4 | Littelfuse MDSR-7 reed switch with separate magnet, mechanically protected |
| USB data cable | 1 | Correct connector for the purchased Nucleo board |
| Carrier PCB and enclosure | 1 each | Fabricate only after review and inspect before connection |

These part selections are design choices, not a live stock quotation. A planning allowance of roughly US$50–80 for controller, small-quantity PCB, connectors, switches, and cable is not a verified purchase price and excludes track construction and tools. Reed switches require a suitable magnet and a protective mount; a bare glass switch must not be exposed to cart impacts. Avoid substituting ordinary oxidized power switches whose minimum wetting current exceeds this interface's current.

With a closed ideal contact the current is 3.3 V divided by 11 kΩ, or 0.30 mA. The MCU input sees approximately 0.30 V because the series 1 kΩ resistor is below the sense node. Four closed contacts add about 1.20 mA, or 3.96 mW on the 3.3 V rail. The USB/ST-LINK board dominates the total load; reserve a conservative 150 mA at 5 V for planning and measure the actual assembly. This is not a battery-life estimate. The carrier needs no local MCU decoupling because it contains no MCU, but cable behavior and ESD susceptibility still need physical testing.

## Pin map and physical installation

| Header | Pin/net | Nucleo connection | Logical role |
|---|---|---|---|
| J1 | 1 VCC, 2 GND | CN6 pin 4, CN6 pin 6 | Sole 3.3 V supply and shared ground |
| J2 | 1 SENSE0 | CN8 pin 1, A0, PA0 | Left start; bit 0 |
| J2 | 2 SENSE1 | CN8 pin 2, A1, PA1 | Left finish; bit 1 |
| J2 | 3 SENSE2 | CN8 pin 3, A2, PA4 | Right start; bit 2 |
| J2 | 4 SENSE3 | CN8 pin 4, A3, PB0 | Right finish; bit 3 |
| J3–J6 | 1 CONTACT0–3, 2 GND | External voltage-free switch | Close to ground only |

ST's UM1724 revision 13, Table 16, supplies the F401RE Arduino connector map. Section 6.8 describes PA2/PA3 USART2 and the default ST-LINK virtual COM bridges: SB13/SB14 on, SB62/SB63 off. Verify the actual purchased board revision and solder bridges before connection. The register definitions reference STM32 RM0368; they are not an invented generic MCU register map. Only PA2 TX is enabled. The board's SWD programming pins remain untouched.

Use short paired contact-and-ground wiring, preferably below half a metre per sensor during initial testing. Keep the carrier away from the moving carts and secure all cables. Start sensors must remain closed while a cart is waiting and open as it leaves. A finish magnet must remain in sensing range long enough to survive qualification; use slow carts with a wide magnetic flag or a retained arrival station. Do not assume that a very brief reed pulse at high speed will be captured. Install matching geometry on both lanes and swap carts, not sensor positions, during a block.

This circuit is not isolated and has no TVS protection or industrial input conditioning. A 1 kΩ resistor is not permission to connect 5 V, 24 V, an inductive load, or a mains-referenced circuit. USB ground is shared with the contacts. Assembly, continuity inspection, and initial voltage measurements must be performed before connecting the Nucleo. Mechanically guard magnets against ingestion and keep them away from magnet-sensitive equipment.

## Architecture and block diagram

```text
Left start reed ---- J3 -- 1k -- SENSE0 -- PA0/A0
Left finish reed --- J4 -- 1k -- SENSE1 -- PA1/A1
Right start reed --- J5 -- 1k -- SENSE2 -- PA4/A2
Right finish reed -- J6 -- 1k -- SENSE3 -- PB0/A3
                     |           |
                  common GND   each has 10k to 3V3

NUCLEO-F401RE / STM32F401RE
  1 ms sampling -> independent qualification -> trial state machine
  bounded counters + last terminal result -> CRC ASCII frame
  USART2 PA2 -> ST-LINK USB VCP -> Linux Tk companion
                                    |
                           gate screen / trial evidence
                           ABBA prompts / two contrasts
                           private, exclusive-create CSV
```

The KiCad schematic embeds locally authored resistor and connector symbols with real numbered pins. Every reference has an actual library footprint with pads. The generator writes the schematic, routed PCB, project settings, symbol library, and a connectivity manifest. The validator separately exports the schematic netlist, loads the PCB, and compares reference, pin, net, value, and footprint against the manifest. ERC on a passive-only board cannot establish external supply correctness; the development-board wiring remains an installation responsibility.

## Firmware state and failure handling

WAITING is phase zero. It qualifies all four inputs and waits for mask 5: both starts closed and both finishes open. After one hundred qualifying baseline samples it enters ARMED, phase one. A changed qualified vector begins a numbered attempt and enters RUNNING, phase two. The first departure defines offset zero. The second departure must qualify within two thousand milliseconds inclusive. Each finish must qualify strictly after its own start departure. A departure and finish in the same qualified vector is rejected rather than given a zero travel duration.

Reclosing a start during a trial rejects that attempt, even after that lane has finished, because the object may have been returned before the paired observation completed. Finish contact release after a recorded arrival is permitted: the initial arrival remains frozen. A trial ends successfully only after both finishes are recorded. The maximum total span from first departure is thirty thousand milliseconds inclusive. Invalid ordering, returned starts, excessive release skew, timeout, acquisition gap, or UART timeout produces a retained rejected result. Every attempt has one terminal outcome; a complete new baseline is required before rearming.

The main loop never fills in missed samples. If successive sample timestamps differ by anything other than one millisecond, active timing is rejected and contact qualification restarts. This conservatively loses a trial instead of fabricating continuity. Millisecond arithmetic uses unsigned subtraction across the ordinary timer wrap, while trial intervals are far shorter than half the timer range. Counters saturate rather than wrap, and reaching the maximum trial identifier prevents future arming until reset.

There is no heap allocation, floating-point calculation, file system, or flash writing on the target. A bounded transmit buffer is serviced one byte at a time without waiting for an entire frame. A hundred-millisecond transmit timeout invalidates continuity. The independent watchdog is configured from LSI with bounded initialization waits and fed only while the sampling loop progresses. Its timing depends on the uncalibrated watchdog oscillator. No outputs drive carts, gates, or indicators that could be mistaken for a safety interlock.

## Wire protocol and desktop application

Each line is `CT1` followed by sixteen decimal unsigned integers separated by commas, an asterisk, four hexadecimal CRC digits, and LF. CRC is CRC-16/CCITT-FALSE over all bytes before the asterisk: polynomial 0x1021, initial 0xffff, no reflection, no final XOR. The decimal fields are, in order: sequence, device milliseconds, phase, qualified mask, attempt count, successful count, rejected count, acquisition-gap count, UART-loss count, last terminal ID, last-valid flag, reason, left start offset, right start offset, left finish offset, right finish offset. Unobserved offsets use 4294967295. Offsets are relative to the first qualified departure of that attempt, not wall-clock timestamps.

Reason values are zero for success/no result, one for ordering, two for return to start, three for release skew, four for timeout, five for sampling gap, and six for UART loss. Seven is reserved for saturation. Frames repeat at least nominally every hundred milliseconds and on phase/result changes. They carry current state and the last result, not a lossless event queue. The app rejects malformed syntax, CRC failures, impossible successful offsets, inconsistent counts, and contradictory retained results. A complete marker resynchronizes after a truncated frame. Duplicate timestamps cannot refresh freshness.

The first screen connects or disconnects a serial device, loads a bounded replay file, and exports visible evidence. The second shows qualified contacts, phase, counters, and successful or rejected trial rows. The third guides the crossover block and presents contrasts. Before a live block, Begin flushes pending input and waits at least a second for a new ARMED frame; keep both carts at their starts until the placement prompt appears. This avoids using a terminal result that was already queued when the operator clicked. It is a receive-only synchronization procedure, not an authenticated command acknowledgement.

A live stream becomes stale after one second without an advancing accepted frame, and an incomplete study stops. Disconnect retains evidence with its original live or replay provenance. Replay never receives a LIVE badge. Replay input must be a regular file no larger than two megabytes; FIFOs and devices are rejected without blocking. The history keeps at most one thousand results. CSV files are created with mode 0600 and exclusive creation, so an existing file is never silently overwritten. Export includes provenance, connection state, stale state, parser errors, warning text, and all raw trial fields; it does not claim that old results are current measurements.

Reset detection is heuristic: regressions in sequence, uptime, cumulative counters, or changed retained results block the session. There is no persistent boot identifier. A reset with missed initial frames and coincidentally compatible later counters can evade detection, especially during replay. Disconnect and start a new study after any known reset, power event, cable change, or cart-identity mistake. This limitation prevents treating the output as an audit-grade uninterrupted record. The lack of incoming commands and persistent storage is intentional, but does not eliminate this measurement provenance limitation.

## Crossover calculation and interpretation

For each valid run, calculate d as left travel time minus right travel time. With left identities A, B, B, A, the cart contrast is `(d1 - d2 - d3 + d4) / 4`, and the lane contrast is `(d1 + d2 + d3 + d4) / 4`. Positive cart contrast means A took longer than B; positive lane contrast means the left lane took longer than the right. Release skew is separately right departure minus left departure. Thus independent releases do not automatically become travel-time differences.

The calculation assumes approximately additive cart and lane effects over the block. ABBA balances a linear drift in the paired differences for the cart contrast, but does not remove all temperature changes, wheel wear, sensor differences, interactions, or nonlinear drift. Repeat complete blocks and inspect the raw rows rather than relying on one contrast. Changing the track, moving a gate, touching a wheel, or introducing another cart requires restarting. A rejected or missing trial, detected acquisition loss, UART timeout, reset suspicion, disconnect, or stale live stream stops collection rather than skipping a problematic run and silently changing the assigned identities.

## Building, validation, and bring-up

From the device folder, run `make -C firmware check` to compile/link the actual Cortex-M4 image with warnings as errors, run AddressSanitizer/UndefinedBehaviorSanitizer host tests, and generate synthetic protocol fixtures. `make -C firmware flash` uses OpenOCD with the ST-LINK interface and STM32F4 target configuration; it is an explicit operator action and was not executed during repository validation. It must only be used on the intended attached development board. Build outputs stay ignored.

Run `python3 -m unittest discover -s tests -p test_app.py`, then `python3 app/app.py --check-replay firmware/build/fixture.txt`. The latter checks stream validity and reports available contrast data; it is not a requirement that every legitimate recording contain a complete four-run block. Run `python3 app/app.py` for the desktop interface. Linux requires Python with Tk and permission to read the chosen serial device. Use a proper device group rather than running the GUI as root. The app has no third-party Python package dependency.

With KiCad 9 Python bindings installed, `/usr/bin/python3 tools/generate_hardware.py` regenerates the authored carrier files. `/usr/bin/python3 tools/check_hardware.py` runs ERC, DRC, netlist export, and exact connectivity checks. Do not hand-edit generated CAD without updating the generator. See VERIFICATION.md for actual results and the distinction between simulated input and measured hardware behavior.

For physical bring-up, first inspect and continuity-test the unpowered carrier. Verify resistor values and all header pin-one orientations. Power only the Nucleo by USB, verify its 3.3 V rail, connect the carrier supply, and measure each open and grounded sense input. Test one switch at a time on the gate screen. Then exercise all failure paths with manually operated contacts before mounting sensors to a track. Compare logged intervals to a calibrated reference source or oscilloscope. Repeat with both lane wirings exchanged to quantify sensor asymmetry. Only after those checks should slow, guarded cart trials begin. Fabrication, flashing, timing calibration, ESD, environmental qualification, and teaching effectiveness remain open work, not implied by passing software tests.
