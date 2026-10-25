# Chord Window — pre-keyboard switch-chord timing workbench

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources: MIT.

Revision A is an engineering prototype. The custom PCB is a fully routed passive contact interface for a separate NUCLEO-F401RE development board. The processor, power regulator, programming hardware, USB bridge and MCU decoupling are on that purchased board. This revision has not been fabricated, flashed, electrically measured, ergonomically evaluated or tested with a person using assistive technology. Software and CAD checks are reproducible; they are not physical validation or medical evidence.

## Purpose, user and practical benefit

Chord Window helps an accessible-input maker compare arrangements of four physical switches before committing to a keyboard controller's chord-recognition policy. The primary user is a workshop engineer building a low-force, repositionable switch panel for a person who wants a small number of physical controls to express more commands. The recurring question is not simply whether a switch closes. It is whether the intended switches close sufficiently close together, remain jointly held for a useful interval, and release in a repeatable manner without unintended re-presses. This device records that evidence without turning it into an assessment of the person's ability.

Today the maker might wire the switches to an existing keyboard, run a browser key tester and adjust firmware until the output feels acceptable. That can hide the very information being investigated. Keyboard debounce, matrix scanning, USB polling, chord aggregation and operating-system shortcuts can change or discard events before the browser sees them. A long recognition window can make the demonstration look successful while slowing every intended single-switch command. Conversely, a short window can make a mechanically comfortable arrangement appear unusable. The proposed workbench observes the disconnected switches directly on a common MCU clock and separates physical timing evidence from the eventual command policy.

One concrete experiment compares a closely spaced triangular group with a wider arc of the same switches. The maker asks for the same two-switch chord several times in each arrangement, exports the raw trials, then compares how many satisfy a chosen maximum press spread and minimum shared hold. Different thresholds can be tried afterward without repeating the exercise or changing the target firmware. There is no universal passing threshold, reaction-time score or recommendation to train through discomfort. A useful outcome could be discovering that a different mounting arrangement, or abandoning chording entirely, is preferable.

The intended practical improvement is fewer blind firmware-adjustment cycles and a reproducible record of why a particular input policy was chosen. That improvement is a hypothesis for a future workshop study, not a measured claim. Secondary uses include evaluating handmade macro pads, debugging slow button contacts and teaching the distinction between overlapping gestures and sequences. The instrument is not a general keyboard tester, a medical device, a motor assessment tool, or an emergency communication system.

## Originality and prior art

Repository discovery included every device directory and README title/purpose inventory before implementation. Contact Atlas studies detached rotary-selector signatures and transitions. Pedal Window studies two musical keys and two pedal endpoints. Takeup Atlas studies reversal travel between shafts. Fold Trace studies the order and dwell of carton-jig leaves. Those instruments share some electrical techniques, but none implements four-switch chord episodes, repeated-key rejection, the intersection of all member hold intervals, or retrospective evaluation of a switch-chord recognition policy. Reusing the existing passive carrier and low-level board support is intentional; a resistor interface is not presented as the invention.

Related tools already exist outside this repository. KeyboardTester.uk's Advanced Keyboard Tester has an event inspector and a combo/chord tester; its documentation explains that firmware and operating-system processing influence observed events. InputConfig's Snap Tap Test measures overlap or gaps between opposite keyboard directions. Xbitlabs' Combo Trainer records input sequences and interprets nearby presses as chords. These establish that chord timing and overlap analysis are prior art. Chord Window's proposed distinction is the disconnected physical-switch fixture, combined press/shared-hold/release evidence, explicit rejected episodes, and local policy comparison before installing a keyboard controller. This is an original repository implementation and a bounded product proposal, not a patent search or proof that nothing similar exists anywhere.

References consulted for that bounded comparison:

- https://keyboardtester.uk/advanced-keyboardtester.html
- https://inputconfig.com/tools/snap-tap-test
- https://www.xbitlabs.com/combo-trainer/

A commercial product would need user research, a proper enclosure, validated switch interfaces and substantially stronger serviceability. The small workshop audience is why this revision favors repairable through-hole circuitry and an ordinary desktop interface instead of a custom wireless wearable.

## Hardware specifications

| Function | Revision A implementation |
| --- | --- |
| MCU | STM32F401RET6 on NUCLEO-F401RE, Cortex-M4 |
| Clock | Internal HSI, nominal 16 MHz; explicit bounded clock restoration |
| Memory | 512 KiB flash and 96 KiB SRAM |
| Sensors | Four external normally-open, voltage-free signal switches |
| Input bias | Four 10 kohm pullups to 3.3 V |
| Input series resistance | Four 1 kohm resistors |
| Transport | USART2 TX, 115200 baud, 8N1, ST-LINK virtual COM |
| Power | Nucleo USB supply; carrier uses only Nucleo 3.3 V and ground |
| Custom PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm thickness |
| Assembly | 2.54 mm pin headers and axial through-hole resistors |
| Host | Linux, Python 3 with Tk; no pip packages or network service |

Architecture and block diagram:

    Switch A -- J3 -- KEY_A -- 1k -- SENSE0 -- PA0 / A0
    Switch B -- J4 -- KEY_B -- 1k -- SENSE1 -- PA1 / A1
    Switch C -- J5 -- KEY_C -- 1k -- SENSE2 -- PA4 / A2
    Switch D -- J6 -- KEY_D -- 1k -- SENSE3 -- PB0 / A3
                                            |
                                      10k each to 3V3

    USB -> NUCLEO-F401RE -> independent contact qualification
                         -> chord episode engine
                         -> bounded CRC-framed USART2 transmission
                         -> onboard ST-LINK VCP
                         -> desktop live display / trial table / policy comparison

J1 pin 1 connects to Nucleo CN6 pin 4, 3.3 V; J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4, the analog-header signals A0, A1, A2 and A3. These are used as digital GPIO inputs, not ADC channels. J3 through J6 each have signal on pin 1 and ground on pin 2. The corresponding order is A, B, C, D. Closing a switch pulls its input low; firmware inverts the electrical level so a one means closed. A=1, B=2, C=4 and D=8 in the exported bitmask.

The Nucleo's factory ST-LINK virtual-COM arrangement is required: SB13 and SB14 fitted and SB62 and SB63 open. PA2 uses AF7 for USART2 transmit. Receive is deliberately disabled. Do not attach another output to PA2 or connect the switch fixture to a powered keyboard matrix. SWD through ST-LINK is the programming path. Consult the board's UM1724 manual and STM32F401 RM0368 reference manual when bringing up a physical assembly. The direct ST manual download timed out in this run; connector assignments follow the existing repository carrier design and must be checked against the actual board revision before power is applied.

## Electrical limits, power and bill of materials

This is a low-energy indoor bench instrument, not an isolated field-input module. Every switch must be electrically disconnected from any other circuit. The circuit shares ground with the USB-connected computer. The series resistors limit ordinary input current but do not establish protection against industrial voltage, static discharge, mains exposure or miswiring. There are no dedicated TVS devices, galvanic isolators, battery cells or charging circuits on the carrier. Long outdoor cables, unknown switch wiring and powered assistive interfaces are outside scope. Use short leads, initially below half a meter, and keep the assembly away from liquids and conductive debris.

With a closed contact, the nominal pullup-plus-series path is 11 kohm. At 3.3 V this is approximately 0.30 mA per closed channel, approximately 1.20 mA with all four closed, and approximately 3.96 mW drawn by those paths. The MCU-side low level is approximately 0.30 V. These are resistor-network calculations, not bench measurements. Verify thresholds, leakage and actual rails before connecting a person-operated fixture. Select signal-rated gold contacts or another low-current switching technology; a power switch requiring a large wetting current may be unreliable at this excitation.

The Nucleo dominates total power consumption. Reserve a conventional 5 V, 500 mA USB supply budget for the development board and debug interface rather than claiming measured current or battery life. The carrier requires no separate regulator and must never be powered by both the Nucleo rail and another supply. Use a known-good protected USB source. There is no sleep-current optimization because this instrument is intended to be attached to a local workstation during a supervised setup session.

Approximate planning BOM, not supplier quotes:

| Item | Quantity | Budget assumption |
| --- | --- | --- |
| NUCLEO-F401RE | 1 | USD 20–35 |
| Signal-rated switches with suitable mounts | 4 | USD 3–15 each, strongly application-dependent |
| 10 kohm axial resistors | 4 | USD 0.05–0.20 each |
| 1 kohm axial resistors | 4 | USD 0.05–0.20 each |
| Two-pin headers | 5 | USD 0.10–0.50 each |
| Four-pin header | 1 | USD 0.10–0.50 |
| Two-layer carrier PCB | 1 | USD 2–8 in a small batch |
| Insulated wiring, strain relief, enclosure | 1 set | USD 10–25 |

At quantity 100, obtain fresh supplier quotations and include assembly, test fixtures and compliant enclosures; the development board and mechanical switch choice will dominate cost. This table is intentionally a planning range, not a fabricated availability check. KiCad footprints are taken from the installed KiCad libraries; their upstream provenance and license obligations are not replaced by the project author credit. The schematic symbols, net manifest, wiring, copper routing and generator logic are locally authored by jayis1.

## Mechanical arrangement and setup

Mount switches to a removable insulating panel, with strain relief that prevents a pulled lead from moving the switch into an uncomfortable position. The carrier has no mounting holes in this revision; use an insulated board tray or edge supports and ensure the underside cannot touch metal. A typical enclosure allowance is roughly 110 by 100 by 45 mm for the carrier, cables and access, with the separate Nucleo mounted beside it. This is an envelope suggestion, not a finished mechanical drawing or an enclosure compatibility claim.

The hardest practical problem is interpreting a repeatable electrical threshold as a useful physical gesture. Two switches with different travel, force or hysteresis can produce different timing even if the user's motion is unchanged. Start with identical signal switches and change one mounting variable at a time. Label positions and record the fixture geometry outside the repository. Before a session, press each switch alone and confirm the live bit display, then hold all four, release all four, and check that the device reaches ready. An open wire looks like an unpressed switch; the firmware cannot distinguish the two.

Participation must be voluntary, brief and comfortable. Stop when there is discomfort or fatigue. Do not use this prototype to decide medical eligibility, employment suitability, access to communication, or whether a person is capable of a task. The right design response to difficult timing can be fewer commands or a different interaction style, rather than a demand for faster movement.

## Firmware behavior and timing semantics

The firmware is freestanding C11 with explicit startup, vector table, linker map, register definitions, GPIO/clock support, nonblocking UART transmission and an independent watchdog. On entry it enables HSI and uses bounded register waits to restore the intended clock tree. It lowers flash latency only after switching away from a potentially faster clock. A clock-initialization failure stops before measurements begin. After peripheral initialization, a configuration readback failure or watchdog-start failure enters a terminal fault state. These checks are engineering defenses, not a certified safety function.

SysTick increments a 32-bit millisecond clock. The foreground loop samples contacts whenever that clock advances, while UART output advances independently whenever the transmit register is ready. Each input must hold its new raw value for five milliseconds before its qualified state changes. Chatter on an unrelated channel cannot postpone another channel's qualification. A gap greater than two milliseconds between engine calls invalidates an active episode and restarts baseline acquisition. Qualification timestamps are not raw first-contact timestamps; rejected short glitches are deliberately absent from the results.

Four operational states are defined. WAIT requires every contact to be qualified open with a continuously open candidate vector for 100 milliseconds. READY accepts the first qualified closure as the start of an episode. ACTIVE records first press and first release offsets for every participating key until all qualified inputs are open. FAULT is terminal until reset. The 100-millisecond baseline prevents held switches or immediate repetitions from being silently reused, but presses during that baseline are not counted as trials. The app displays the state so the operator can wait for ready.

For a complete episode, the member mask is the union of keys pressed. Press spread is the latest first press minus the earliest first press. Release spread is the latest first release minus the earliest first release. Shared hold is the intersection of all members' first hold intervals: maximum of zero and earliest release minus latest press. A rolling gesture can therefore have a nonzero press spread and zero shared hold, even if adjacent fingers overlap. Duration is last release minus first press. A one-key episode has zero press/release spread and shared hold equal to duration.

A re-press of a previously seen key while the episode remains active makes the final result RETAP, even if a visually plausible chord later forms. An episode lasting ten seconds is TIMEOUT; timeout wins over a release qualifying on the same tick. An acquisition gap yields GAP. Invalid results retain identity, member mask and bounded duration but set all three timing estimates to zero. They are evidence of an incomplete observation, not zero-time performances. Result identifiers increase without wrapping; exhaustion enters FAULT rather than aliasing an old result. Acquisition-gap and transmission-error counters are also bounded.

## Serial contract and reliability

There are no application commands, pairing keys, radio services or mutable settings. The device sends one ASCII snapshot approximately every 50 milliseconds. Each snapshot repeats the latest completed result, allowing late attachment without depending on a one-shot boot announcement. A host connection cannot actuate anything or change the firmware's timing policy. Physical programming access remains trusted; CRC detects accidental corruption and is not authentication.

The frame is `CW1,ms,state,mask,gaps,tx_errors,id,reason,keys,spread,overlap,release,duration*HHHH` followed by LF. Decimal fields are unsigned 32-bit values. All timing fields are milliseconds. CRC is CRC-16/CCITT-FALSE, initial value 0xFFFF and polynomial 0x1021, calculated over the ASCII bytes from C through the last duration digit, excluding the asterisk and newline. HHHH is four uppercase hexadecimal digits. State codes are WAIT=0, READY=1, ACTIVE=2, FAULT=3. Reason codes are NONE=0, complete=1, RETAP=2, TIMEOUT=3 and GAP=4. CW1 fixes the timing policy described above; incompatible future policy changes require another protocol version.

The UART driver holds one bounded frame and abandons a blocked transmission after 100 milliseconds. It does not block sampling while waiting for output. The host parser resynchronizes on the full CW1 marker after truncated transmissions, rejects oversized records, validates field invariants and verifies CRC. There is no persistent history on the device. If more than one result is skipped between observed identifiers, the app reports missing results; it cannot recover them. Results lost before the first attachment are unknown, and the first repeated result may predate connection.

The host treats repeated identical timestamps as duplicates, not freshness. Contradictory duplicates, result changes under an unchanged identifier, counter rollback or backwards device time freeze acceptance until explicit reconnection. Timestamp comparison permits ordinary unsigned wrap. Without a persistent boot identifier, some resets remain indistinguishable from forward traffic; this limitation is not solved by a CRC or a USB device name. Keep separate recordings for separate power sessions and never interpret reconnect as proof of uninterrupted observation.

## Desktop application and reproducible operation

Run `python3 app/chord.py` from this directory on Linux with Python Tk installed. The Live contacts page shows readiness, four contact bits, device uptime and diagnostic counters. Trials displays each retained result with its classification and timing fields. Compare policy accepts a target bitmask, maximum press spread and minimum shared hold; it recalculates the retained trial set locally. Setup explains wiring and interpretation limits. Policy changes are analysis filters only: they do not send commands or retroactively modify raw results.

Enter the ST-LINK serial path, commonly `/dev/ttyACM0`, and choose Connect. The app configures 115200 8N1 and discards preexisting host input before starting a new session. Connect replaces retained evidence, so export first. Disconnect preserves the data's original live provenance but marks the transport disconnected. A one-second absence of advancing device time marks live telemetry stale. Linux hangup is detected through poll flags as well as read errors, including devices that return empty reads after unplugging.

Replay accepts a regular text recording no larger than four MiB. It refuses FIFOs and device files so selecting a path cannot block the GUI indefinitely. Replayed data is explicitly historical and its origin is not authenticated. A partial final frame is rejected rather than silently ignored. Up to one thousand distinct results are retained; evictions and detected missed identifiers are visible. Export creates a new CSV with owner-only permissions and refuses overwriting an existing path. CSV includes jayis1 attribution, source provenance, transport state, receipt age, parser errors, missing/evicted rows, reset suspicion and raw measurements. It does not export an editable policy verdict as if that were measured data.

Build and validation commands, run from this device directory:

    make -C firmware clean all test
    xvfb-run -a python3 tests/test_app.py
    python3 app/chord.py --check firmware/build/fixture.txt
    /usr/bin/python3 tools/generate_hardware.py
    /usr/bin/python3 tools/check_hardware.py

The cross compiler is `arm-none-eabi-gcc`; host C tests use GCC with AddressSanitizer and UndefinedBehaviorSanitizer. The test fixture is generated by the real C frame encoder from a synthetic switch sequence, not captured hardware. UI tests use an actual Tk event loop and Linux pseudo-terminal, not a physical USB device. KiCad generation requires the system Python that can import pcbnew, KiCad 9 and installed footprint libraries. No dependency installation scripts run and the app has no third-party Python packages.

`make -C firmware flash` is provided for a supervised assembly using OpenOCD and ST-LINK. It is not part of the automated verification and has not been executed. Inspect wiring, power rails, polarity, contact thresholds and UART timing on a real board before collecting meaningful evidence. Hardware bring-up should first use a switch jig, then a representative mechanical panel, before any participant session.

## CAD checks and remaining work

The delivered KiCad schematic contains actual symbols, pin numbers, wires and net labels. The PCB contains fourteen component footprints, thirty pads, ten nets, routed copper and a rectangular outline. The hardware checker exports a fresh schematic netlist and compares every component value, footprint identifier and reference/pin/net assignment with the PCB and connectivity manifest. ERC and DRC are run with violation exit status enabled; unconnected pads are not accepted as a finished routed design. The generator provides reproducible design intent rather than an unconnected drawing disguised as hardware.

Passing CAD checks does not validate the external Nucleo, cable assembly, wetting-current suitability, mechanical endurance, ESD tolerance or measurement uncertainty. Remaining work includes fabrication review, physical power-on, firmware programming, oscilloscope comparison of qualified edges against raw contacts, long-cable disturbance testing within the intended short-lead envelope, enclosure validation and a consent-based usability study. See VERIFICATION.md for results actually produced during this run. Do not substitute that report for a manufacturing approval or a claim of global novelty.
