# Peel Front — four-point lift-front propagation notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is an engineering prototype. Firmware has a real STM32F401 target build, and the supplied KiCad project describes a routed passive input board for a separately purchased NUCLEO-F401RE. This is not a standalone MCU board. No specimen fixture has been assembled, no PCB fabricated, no target flashed and no physical peeling measurements made. The design's mechanical sensing assumption remains a hypothesis requiring experimental validation. Do not use this instrument for adhesive certification, structural inspection, medical work or safety decisions.

## Purpose and useful distinction

Peel Front helps a packaging or repair workshop investigate whether a slowly removed adhesive strip lifts evenly across its width. Four resettable lift flags sit above the strip backing at two longitudinal stations and two transverse positions. Each flag opens a dry contact when the backing lifts it. The firmware records the first qualified opening at each position and the desktop application calculates two transverse timing skews and two longitudinal transit intervals. This provides a compact description of a visibly crooked or uneven peel without pretending to measure adhesive strength.

The primary user is a small workshop developing removable labels, protective films, temporary masking strips or reusable assembly instructions. The present workaround is often to peel several samples by hand, watch whether one edge lifts first, and write a subjective note. Recording video is a stronger alternative but usually requires frame review and a repeatable camera view. The proposed device offers immediate four-point timing evidence in a local trial table, including rejected attempts. A workshop could compare centered and off-center pull tabs, or compare a narrow relief cut on one edge with an unmodified coupon, while keeping the adhesive, substrate and preparation procedure constant.

The hook is the reusable spatial fixture: two points along each edge distinguish a persistent edge lead from a lead that reverses farther along the coupon. Merely timing the end of a peel would hide that distinction. A force gauge alone answers a different question, and a phone application alone cannot determine these four local lift events without a sensing arrangement. The instrument does not claim to replace a testing machine or a high-speed camera. It deliberately targets slow, supervised, comparative experiments where four coarse observations can help decide which sample geometry deserves closer study.

## Originality, nearby designs and prior art

The repository was updated and its device directories and README summaries inventoried before implementation. Chord Window measures the overlapping press and release intervals of an intentional switch chord. Its complete README and board-support implementation were inspected. Fold Trace observes folding-jig operations; Tilt Break observes incline departure and downstream arrival; Creep Pair observes compression and recovery. Peel Front instead records a spatially indexed, normally closed set of irreversible lift events, checks upstream-before-downstream order independently on each side, and compares transverse skews between two stations. The existing passive resistor carrier and board-support code are reused deliberately. Neither a GPIO input nor a resistor network is presented as novel.

A bounded web search for peel-front asymmetry and propagation found prior work titled “Toughening and Asymmetry in Peeling of Heterogeneous Adhesives,” and a public manuscript explaining interfacial cavitation, stepwise front motion and observations using high-speed imaging. These results establish that asymmetric peeling, front tracking and stick-slip are existing research topics. The proposal here is a low-cost, mechanically resettable four-flag workshop fixture and its particular evidence contract. Global uniqueness and patentability have not been established. It would be misleading to say that no related device exists anywhere.

Prior-art references returned by that search:

- https://link.aps.org/accepted/10.1103/PhysRevLett.108.196101
- https://par.nsf.gov/servlets/purl/10629441

The manuscript's warning that debonding can occur ahead of visible front motion is particularly relevant. A flag sees backing lift, not the microscopic interface crack. Results must therefore be described as lift-front timings rather than crack velocities or adhesion energies. Video comparison on the assembled fixture is the most important feasibility experiment, and it can legitimately disqualify the concept if the flags perturb the specimen too much.

## Hardware specifications and architecture

| Function | Revision A specification |
| --- | --- |
| Processor | STM32F401RET6 on NUCLEO-F401RE, Cortex-M4 |
| Clock | Internal nominal 16 MHz HSI, explicitly restored at startup |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Sensing | Four external mechanically latched lift flags operating voltage-free contacts |
| Acquisition | Nominal one-millisecond polling; independent five-millisecond qualification |
| Carrier | 80 by 70 mm, two copper layers, 1.6 mm nominal thickness |
| Components | Axial through-hole resistors and 2.54 mm pin headers |
| Transport | USART2 TX at 115200 baud, 8N1, through ST-LINK virtual COM |
| Supply | USB-powered development board; carrier takes its 3.3 V and ground |
| Software | Freestanding C11 firmware and Linux Python/Tk desktop application |
| Radio/storage | No radio, cloud service, battery, removable storage or device-side history |

The processor, regulator, decoupling, debugger, USB connector and USB bridge are on the purchased Nucleo. They are not missing parts secretly assumed to be on the custom PCB. The PCB is an interface between short sensor leads and the Nucleo headers.

    Upstream left flag  -> J3 -> UP_LEFT    -> 1k -> SENSE0 -> PA0
    Upstream right flag -> J4 -> UP_RIGHT   -> 1k -> SENSE1 -> PA1
    Downstream left     -> J5 -> DOWN_LEFT  -> 1k -> SENSE2 -> PA4
    Downstream right    -> J6 -> DOWN_RIGHT -> 1k -> SENSE3 -> PB0
                                                      |
                                               10k each to 3V3

    USB -> NUCLEO-F401RE -> independently qualified contacts
                        -> spatial ordering and trial engine
                        -> bounded ASCII/CRC snapshots over USART2
                        -> ST-LINK virtual COM -> Linux desktop
                        -> live fixture / trial table / private CSV export

The board connector map is explicit. J1 pin 1 goes to Nucleo CN6 pin 4, 3.3 V; J1 pin 2 goes to CN6 pin 6, ground. J2 pins 1, 2, 3 and 4 go to CN8 pins 1, 2, 3 and 4, respectively: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. These analog-header pins are used as digital inputs. Every external flag connector has signal on pin 1 and ground on pin 2. A closed contact becomes logical one after GPIO inversion. Bit values 1, 2, 4 and 8 correspond to upstream left, upstream right, downstream left and downstream right.

PA2 uses alternate function AF7 for USART2 transmit. Receive is disabled. Use the factory ST-LINK virtual-COM solder-bridge arrangement, SB13/SB14 fitted and SB62/SB63 open, after checking the actual board revision. SWD through ST-LINK is the recovery and programming path. Manufacturer references are STM32F401 RM0368 and Nucleo UM1724. Direct retrieval of the ST UM1724 PDF timed out during this run; pin assignments follow the inspected repository platform and require physical board/manual confirmation before energizing a new assembly.

## Electrical circuit, budget and limits

Each channel has one 10 kohm pullup from SENSE to 3.3 V and one 1 kohm series resistor from SENSE to the external signal. Closing the signal to ground yields approximately 0.30 V at the MCU and approximately 0.30 mA through the contact. All four closed channels draw approximately 1.20 mA and 3.96 mW from 3.3 V. These are nominal resistor-network calculations, not measured results. Use one-percent resistors; their tolerances are less important than uncertain switch resistance and wiring errors, which must be checked during bring-up.

This excitation is suitable only for contacts rated for low-level signals. Large power microswitches may have unreliable resistance at this wetting current. A gold-contact subminiature switch family such as Omron D2F with the appropriate gold-contact option is a candidate, but operating force and exact suffix must be confirmed from a current supplier datasheet. No stock availability or production suitability has been verified. The selected switch is external to the carrier and its mechanical lever arrangement is part of the experiment.

The interface is not isolated. USB ground, Nucleo ground and every sensor return are common. There are no dedicated ESD suppressors or industrial-input protection devices. The 1 kohm resistors are not permission to connect powered signals. Only detached voltage-free contacts are allowed. Keep leads below approximately half a meter for initial trials, prevent contact with conductive debris, and keep the board away from wet adhesives and cleaning fluids. Never connect this fixture to mains, an operating machine controller or an unknown powered circuit.

Reserve a normal 5 V, 500 mA USB supply budget for the development board and debugger. Actual system current has not been measured, and no battery-life estimate is offered. The carrier has no local regulator, no charger and no competing supply input. Its 3.3 V must come from the Nucleo only. Disconnect USB before moving header wires. Use an insulating tray because the carrier currently has no mounting holes; conductive screws or a metal bench can short exposed through-hole joints.

Planning bill of materials, with estimates rather than supplier quotations:

| Reference or assembly | Quantity | Planning cost |
| --- | --- | --- |
| NUCLEO-F401RE | 1 | USD 20–35 |
| R1, R3, R5, R7: 10 kohm axial | 4 | USD 0.05–0.20 each |
| R2, R4, R6, R8: 1 kohm axial | 4 | USD 0.05–0.20 each |
| J1, J3–J6: two-pin headers | 5 | USD 0.10–0.50 each |
| J2: four-pin header | 1 | USD 0.10–0.50 |
| Two-layer carrier PCB | 1 | USD 2–8 in a small batch |
| Signal switches, lift flags and adjustable mounts | 4 sets | USD 5–15 per set allowance |
| Coupon base, guides, strain relief and insulating tray | 1 set | USD 15–40 allowance |

## Mechanical fixture and measurement geometry

Start with an insulating flat base approximately 180 by 100 mm and a removable substrate coupon. An initial specimen allowance is 30 mm wide and 100 mm long, with a free pull tab extending past the upstream edge. Locate the first station 25 mm from the beginning of the bonded region and the second 30 mm farther downstream. Place left and right followers approximately 3 mm inward from their respective specimen edges. These dimensions define an initial jig experiment, not a machined production tolerance drawing. Mark the actual measured locations in the laboratory notebook and keep them unchanged during a comparison.

Each follower is a lightweight insulating finger on a horizontal pivot outside the specimen width. Its tip rests on the top surface of the backing without being inserted into the adhesive interface. A small over-center mechanism has a down detent and an up detent. In the down position a gold signal contact is closed. As the backing lifts, it raises the finger through the detent; the contact opens and the finger remains latched up, clear of the moving strip, until manually reset. The latch prevents a passing strip from letting the follower drop back and obscure the first opening. The firmware rejects qualified reclosure during a trial rather than assuming it was harmless.

Design the finger clearance so the peeled strip cannot snag beneath another follower or pull wiring into the specimen. The four pivots must move independently. Route cables behind the base, not across the peel path. Use blunt, rounded tips and a low-energy hand pull; there is no motor or actuator. The provisional tip-lift target is below one millimeter and the provisional release-force target is below 0.02 N at each tip. Neither target has been achieved or validated. If a switch's operating force is too high, change the lever ratio or reject that switch rather than pretending software can remove the disturbance.

Validate the fixture using side and overhead video with a visible reference scale. Compare identical coupons with no flags, with one flag and with all four. Measure the delay between visible local backing lift and electrical contact opening, and compare that delay across positions. If the flag changes the motion substantially, this revision cannot support useful inference. The practical result may be that optical sensing is necessary in a later design; that would require different electrical hardware and a new validation cycle, not an undocumented substitution.

Hold substrate preparation, dwell before peeling, temperature, peel angle and operator technique constant. Record them separately with specimen identity. The device does not measure these variables. A hand pull does not establish constant speed, and converting the nominal 30 mm station spacing to a speed is not implemented. Transverse timing skew is not a geometric crack-front angle. A zero interval means events were coincident at the qualified sampling resolution, not instantaneous physical travel.

## Firmware behavior and failure semantics

The firmware includes a reset vector, linker script, register definitions, bounded clock restoration, GPIO readback, SysTick, nonblocking UART output and an independent watchdog. It uses no vendor HAL, heap allocation, operating system or floating-point arithmetic. Main restores HSI before reducing flash latency; a failure stops acquisition. Invalid GPIO configuration or a watchdog-start failure enters a terminal FAULT state that is visible in telemetry. These defenses are not a certified safety function.

On initial acquisition, each channel must remain at its candidate level for five milliseconds before becoming known. Changes are qualified independently so chatter on one flag cannot postpone another flag's event. WAIT requires all four contacts to be known closed and all raw candidates closed for a further 100 milliseconds. READY then accepts the first qualified opening. ACTIVE captures every first opening as an offset from that event. A downstream opening is allowed only if its same-side upstream opening has already occurred or qualifies in the same sample. Opposite sides are deliberately not ordered: one edge can reach the second station before the other reaches the first.

A trial completes when all four flags are open. Its duration is the last opening offset. A qualified reclosure of an already-opened flag yields RECLOSE. A downstream event without its corresponding upstream event yields ORDER. A trial reaching 30,000 milliseconds yields TIMEOUT, which takes precedence over a final opening on the deadline. A sampling interval greater than two milliseconds yields GAP, abandons an active trial and requires a new baseline. Short rejected glitches are not events. Neither a missing sample nor a raw candidate proves uninterrupted physical contact.

Completed and rejected results are immutable until a newer result replaces them. Restoring all flags closed permits another trial after the baseline interval. The device retains only the latest result, not a queue of every specimen. Result identifiers increase without wrapping. Exhaustion enters FAULT and preserves the previous result; diagnostic-counter exhaustion also faults. A terminal fault freezes acquisition time and the host eventually marks the record stale. Power loss clears all volatile evidence and requires a new closed baseline, but cannot prove that the current specimen was mounted correctly. Export before resetting or disconnecting power.

## Protocol and application interface

PF1 fixes the hardware order and qualification policy. The device sends a snapshot approximately every 50 milliseconds:

    PF1,ms,state,closed,known,gaps,tx_errors,faults,id,reason,seen,UL,UR,DL,DR,duration*HHHH

A single LF terminates the line. Fields are unsigned decimal integers up to 32 bits; an absent event offset is 4294967295. All event offsets and duration are milliseconds. State codes are WAIT=0, READY=1, ACTIVE=2 and FAULT=3. Reason codes are NONE=0, COMPLETE=1, ORDER=2, RECLOSE=3, TIMEOUT=4 and GAP=5. Fault bits are configuration=1, watchdog=2 and counter exhaustion=4. CRC-16/CCITT-FALSE uses initial value 0xFFFF and polynomial 0x1021 over the ASCII body beginning with P and ending before the asterisk. HHHH is uppercase hexadecimal. CRC is corruption detection, not authentication.

The formatter is bounded and validates result invariants before serializing. The UART holds one frame and abandons a blocked transmission after 100 milliseconds without waiting in the sampling loop. The host resynchronizes on the entire PF1 marker, bounds line sizes and rejects CRC-valid impossible records. Periodic repetition allows late attachment but cannot recover earlier overwritten trials. The first observed result may predate attachment. Missing identifiers after attachment are counted explicitly.

Run `python3 app/peel.py` on Linux with Python Tk. The Live fixture page shows readiness, closed and known masks, latest classification, diagnostic counters and freshness. Trials lists every retained result, including rejected ones. For complete trials, upstream skew is UR minus UL, downstream skew is DR minus DL, left transit is DL minus UL and right transit is DR minus UR. Positive skew means the right position opened later. Rejected trials have blank derived metrics rather than misleading zeroes. Setup and limits supplies the connector order and interpretation warnings beside the operational screens.

Enter a serial path such as `/dev/ttyACM0` and choose Connect. Successful connection configures 115200 8N1, flushes preexisting input and begins a new local session. A failed open preserves existing evidence. Disconnect preserves provenance and rows but marks the transport disconnected. There is no discovery service, pairing, authentication, command channel or remote configuration. Use ordinary Linux serial-device permissions; do not run the app as root merely to bypass a permission error.

Only advancing device time refreshes evidence. Contradictory repeated timestamps, counter rollback, disappearing sticky faults or changed results under the same identifier freeze acceptance until reconnection. A live reception pause exceeding one second also freezes resumed data rather than presenting queued traffic as current. Cumulative host/device progress is compared with a 500 ms allowance plus three percent oscillator/drift allowance. This is a diagnostic heuristic, not clock calibration. Without a persistent boot identifier, some restarts are indistinguishable from ordinary forward traffic.

Replay accepts regular files no larger than four MiB, refuses FIFOs and devices, and rejects incomplete final frames, empty input, corruption or inconsistent histories. It is labeled historical and is not authenticated. The last one thousand distinct trials are retained; evictions and observed missing identifiers remain visible. Export creates a new CSV with owner-only permissions, refuses overwrite, and includes a session metadata row even when no trial completed. Metadata records source, connection state, age, errors, missing rows, evictions and acceptance blocking. No cloud upload occurs.

## Build, tests and physical acceptance

Run from this device directory:

    make -C firmware clean all test
    xvfb-run -a python3 tests/test_app.py
    python3 app/peel.py --check firmware/build/fixture.txt
    /usr/bin/python3 tools/generate_hardware.py
    /usr/bin/python3 tools/check_hardware.py

The target compiler is arm-none-eabi-gcc with warnings treated as errors. Host engine tests use GCC AddressSanitizer and UndefinedBehaviorSanitizer. The fixture text is generated by the production C encoder from a synthetic contact sequence, not from physical measurements. Application tests exercise actual Tk screens and serial transport through a Linux pseudo-terminal, including hangup. KiCad generation uses system Python, pcbnew and installed KiCad footprint libraries; the application itself has no third-party Python dependencies. See VERIFICATION.md for the executed checks and their exact scope.

The schematic contains real symbols, wires, labels and component values. Its fourteen footprints, thirty pads and ten nets are compared against a freshly exported netlist and the connectivity manifest. ERC and DRC must report zero violations and zero unrouted items. Those checks validate the passive board's internal consistency, not external board wiring or the mechanical fixture. Library footprints retain their upstream licensing obligations; author credit applies to the locally authored design and its arrangement.

`make -C firmware flash` provides an OpenOCD/ST-LINK programming path but has not been executed. Physical acceptance must include connector continuity, polarity, rail measurement, four input levels, simulated contact transitions, oscilloscope timing, firmware programming, watchdog recovery and actual unplug behavior. Then validate flag force, latch clearance, specimen disturbance, repeatability and video correlation. Until those steps pass, this is a build-tested design proposal with an unverified sensing fixture, not a validated adhesive-testing product.
