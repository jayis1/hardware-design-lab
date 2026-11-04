# Domino Ledger — first-fall and re-erection notebook for hinged cascades

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software, documentation and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier used with a separately purchased NUCLEO-F401RE, not a standalone microcontroller board. Target compilation, synthetic contact tests, Linux desktop tests and electrical CAD checks are reproducible. No board has been fabricated, target flashed, cascade assembled, magnetic threshold calibrated or physical propagation measured. This is a build-tested device design, not a certified measurement product, and worldwide novelty is not established.

## Purpose, audience and useful distinction

Domino Ledger helps an educator or museum workshop builder study a small, slow, hinged cascade without reducing every attempt to a single start-to-finish time. Four selected leaves each carry a small captive magnet. Four stationary reed switches report whether their leaves are upright. The instrument remembers the first qualified departure of each leaf, preserves simultaneous departures as equal timestamps, and separately remembers any leaf that returns upright before the attempt is finalized. Incomplete cascades remain visible instead of disappearing from a successful-run average.

The intended user is building a tabletop chain-reaction exhibit from lightweight hinged pieces. A change in spacing, hinge friction or bumper shape may make the last leaf fall sooner but cause more incomplete runs. A leaf may fall, bounce back into its upright detection zone, and then fall again. A conventional two-gate timer cannot distinguish these cases. An instructor can compare complete and partial attempts, inspect the order of the four first departures, and identify which stations returned without claiming to know why they returned.

A representative session starts with all leaves upright. The user waits for READY, nudges one end of the cascade, and does not touch it again until DONE. The display shows first-fall offsets in milliseconds, a seen mask and a re-erected mask. The user exports a local CSV before changing the fixture or resetting the processor. A second series changes only one mechanical parameter. Photographs and written specimen notes accompany the export because the instrument does not measure dimensions, hinge torque, magnet position or launch force.

The product contribution is a reusable four-station fixture and a precise evidence contract: first departures are immutable within an attempt, returns do not erase them, ties do not acquire an invented order, and a finite deadline preserves partial propagation. These observations can guide the next mechanical experiment. They do not prove energy transfer, chain-reaction causality, physical speed, successful visitor interaction or statistical significance. A full result means all four stations were observed to depart, not that each leaf hit its neighbor or remained down forever.

## Repository distinction and prior art

The repository was pulled before writing, and existing device directories and README purpose summaries were inventoried. The closest measurement contracts were then read in full. Peel Front captures irreversible backing-lift events, enforces upstream-before-downstream constraints, and rejects reclosure. Nest Settle measures locating-fixture occupancy and seating duration. Domino Ledger instead permits every first-fall ordering, records qualified returns as evidence rather than rejecting the attempt, waits through an all-down quiet window, and preserves partial cascades at a deadline. It is not a renamed peel-order checker or seating timer.

The electrical platform and MCU drivers intentionally reuse jayis1's existing Beat Aperture carrier. Resistors, reed switches and a serial port are conventional. The cascade engine, protocol schema, companion model, interface and experiment tests are specific to this proposal. Reuse avoids inventing a new power or USB subsystem merely to make the board look different. The custom assembly remains a passive input carrier and the purchased development board remains clearly identified.

A bounded prior-art search found Oxford University Press's domino-effect teaching experiment, which discusses end-to-end stopwatch timing, two photogates and frame-by-frame video. The Exploratorium's chain-reaction material discusses taped soft hinges that keep pieces in place and simplify resetting. Those are established ideas, not discoveries claimed here. The proposed distinction is the combination of retained first-fall groups, return masks, explicit partial-run evidence and a local repeatable notebook. No patent search, commercial market clearance or proof of global absence has been performed.

References consulted through retrieved search results:

- https://blog.oup.com/2015/01/physics-project-domino-effect/
- https://www.exploratorium.edu/tinkering/projects/chain-reaction-explorations
- https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7
- ST UM1724, Table 16 and USART section: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf

Direct download of the ST PDF timed out in the design environment; indexed manufacturer-manual excerpts confirmed the connector map and virtual-COM bridge configuration. Check the actual board revision against the complete manual before connecting a new assembly.

## Hardware specification and architecture

The processor is an STM32F401RET6 on the NUCLEO-F401RE MB1136 assembly: Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware restores the internal nominal 16 MHz HSI clock, samples four digital inputs with a nominal one-millisecond SysTick, and transmits through USART2 at 115200 baud, eight data bits, no parity and one stop bit. The board's ST-LINK supplies USB virtual COM and SWD programming. There is no FPGA, radio, battery, motor driver or cloud connection.

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4. It contains eight axial through-hole resistors and six 2.54 mm pin headers. It connects to the Nucleo with short insulated wires; it is not a mechanically compatible plug-on shield. The Nucleo supplies its own regulator, decoupling, reset network, boot configuration, debugger and USB connector. These parts are external purchased hardware, not omitted carrier components.

Each input has a 10 kilohm pullup from SENSE to 3.3 V and a 1 kilohm series resistor from SENSE to the external contact. The contact closes to common ground when its leaf is upright. Opening the contact corresponds to departure from the upright magnetic zone. Firmware inverts the electrical input so a closed upright contact appears as a logical one.

| Carrier connection | External connection | Meaning |
| --- | --- | --- |
| J1 pin 1, VCC | Nucleo CN6 pin 4 | Regulated 3.3 V only |
| J1 pin 2, GND | Nucleo CN6 pin 6 | Common ground |
| J2 pin 1, SENSE0 | CN8 pin 1, A0, PA0 | Upright A, mask bit 0 |
| J2 pin 2, SENSE1 | CN8 pin 2, A1, PA1 | Upright B, mask bit 1 |
| J2 pin 3, SENSE2 | CN8 pin 3, A2, PA4 | Upright C, mask bit 2 |
| J2 pin 4, SENSE3 | CN8 pin 4, A3, PB0 | Upright D, mask bit 3 |
| J3 through J6 pin 1 | Respective reed switch | UP_A through UP_D |
| J3 through J6 pin 2 | Respective switch return | GND |
| Nucleo PA2, AF7 | Onboard ST-LINK VCP | USART2 transmit only |

Architecture:

    Hinged leaf A + magnet -> upright reed -> J3 -> 1k -> PA0
    Hinged leaf B + magnet -> upright reed -> J4 -> 1k -> PA1
    Hinged leaf C + magnet -> upright reed -> J5 -> 1k -> PA4
    Hinged leaf D + magnet -> upright reed -> J6 -> 1k -> PB0
                                                      |
                                              each 10k to 3V3

    USB -> NUCLEO-F401RE -> independent five-sample qualification
                         -> all-upright baseline / cascade state machine
                         -> first-fall offsets + retained return mask
                         -> bounded DL1 frame + CRC -> ST-LINK virtual COM
                         -> Linux/Tk: Connection | Cascade | History/export

Leave the supported factory virtual-COM bridge configuration in place: SB13 and SB14 on, SB62 and SB63 off. Firmware enables transmit, not receive. There is no host command that changes thresholds, releases a leaf or writes flash. Recover or reprogram through ST-LINK SWD, with only the intended target attached.

## Sensors, mechanical arrangement and power

The external sensor candidate is Littelfuse MDSR-7-10-15, a normally open, SPST reed switch from the MDSR-7 family. Manufacturer information lists a 12.7 mm glass body, 1.8 mm diameter and a 10–15 ampere-turn sensitivity option. The sensor is not mounted directly on the carrier: it belongs in an insulated protective sleeve beside its leaf. Confirm the actual part suffix, magnetic orientation, operating/release distances and logic-level contact performance before ordering a production quantity. No stock availability is claimed.

Use four lightweight captive leaves on an insulating base, initially about 400 by 100 mm. A starting allowance is a 60 mm high, 25 mm wide leaf with a protected lower hinge and an adjustable bumper. These are experimental fixture dimensions, not released machining tolerances. Arrange the leaves so one can contact the next without hitting a sensor housing. Retain magnets mechanically so adhesive failure cannot create loose projectiles or ingestion hazards. Shield glass reeds from direct leaf impacts and keep all wiring below the motion path.

Set each reed to remain closed in the upright rest zone and open after a small, repeatable angular departure. The four thresholds should be comparable, but identical reed part numbers alone do not establish identical angles. Use a protractor and video to determine both departure and return angles. Magnetic attraction and hinge friction can alter the cascade, especially with light leaves. Compare with sensors removed, then with magnets installed, then with all sensing stations active. If that changes the behavior materially, reject the fixture rather than compensating with invented software accuracy.

Begin with slow mechanisms whose upright and down plateaus are comfortably longer than five milliseconds. A heavily damped teaching cascade is more suitable than a fast competitive domino run. The firmware reports the sample at which five matching observations have accumulated, not the first physical motion. Short openings are intentionally filtered. Equal offsets mean unresolved coincidence at this acquisition and qualification resolution; they do not prove simultaneous physical motion. No velocity conversion is implemented because geometric distance between sensor stations is not necessarily the relevant path length.

Power comes only through the Nucleo's supported USB arrangement. Reserve a conventional 5 V, 500 mA source capability for the development board and debugger; actual draw has not been measured. The passive carrier's ideal closed-contact current is 0.30 mA per channel, its nominal sense-low voltage is 0.30 V, and all four networks dissipate 3.96 mW together. These values follow the 3.3 V supply and 11 kilohm series path, not a bench measurement. No battery-runtime estimate is relevant because there is no battery.

The bill of materials is one NUCLEO-F401RE; one carrier PCB; four 10 kilohm and four 1 kilohm, one-percent DIN0207 axial resistors with 7.62 mm lead spacing; five two-pin headers; one four-pin header; four protected MDSR-7-10-15 reeds; four captive magnets; an insulating fixture, leads and strain relief. Price depends primarily on fixture construction and development-board sourcing, so no current quotation is asserted. There are no mounting holes on this carrier: use insulating edge supports or a tray, not improvised drilling through routed copper.

## Electrical boundaries and commissioning

Connect only detached voltage-free contacts. USB ground, sensor ground and the computer's ground are common. The carrier has no galvanic isolation, dedicated transient suppressors, reverse-polarity protection or industrial field-input conditioning. The series resistors do not authorize powered signals, PLC outputs, mains, motors or connection to an operating machine. Keep initial leads below half a meter in a dry indoor bench setup. Disconnect USB before changing wires and never parallel an external supply onto VCC.

Before attaching the MCU, inspect solder joints and verify resistance between VCC and ground. With the supported supply connected, measure each open SENSE node near 3.3 V and each deliberately closed channel near its expected low level. Confirm that closing one sensor changes only its intended display bit. Verify the USB bridge and header orientation against the actual Nucleo revision. A disconnected sensor resembles a fallen leaf, while a stuck closed sensor resembles an upright leaf. The software cannot uniquely diagnose those faults.

Wait for the initial upright baseline before starting an experiment. Confirm that moving each leaf creates the intended first-fall offset and that bringing a fallen leaf back upright sets its return bit while ACTIVE. Do not handle the fixture during an actual comparative run. A return bit can reflect rebound, deliberate resetting, external touch, magnetic interference or contact behavior; the device deliberately does not assign a cause. Guard pinch points and keep the experiment low-energy. It has no safety function.

## Firmware contract and failure behavior

The firmware is freestanding C11 with no heap, operating system, vendor HAL or floating-point runtime. Files include reset vectors, linker layout, register definitions, bounded clock restoration, GPIO readback, SysTick, watchdog and a nonblocking UART queue. Board support is shared repository code credited to jayis1; the cascade logic and DL1 encoder are local. Build warnings are errors. The target image links for Cortex-M4 rather than merely passing a desktop syntax check.

Each channel qualifies independently after five consecutive matching raw samples. A different raw candidate restarts that channel's count. Chatter on B cannot indefinitely postpone A. WAIT requires all four channels known upright and raw upright, followed by 500 full acquired milliseconds with that condition present at both endpoints of every credited interval. The initial qualified endpoint receives no prior time. READY remains armed until a qualified opening begins an attempt. No order is imposed on A, B, C and D.

The first qualified departure begins ACTIVE at age zero. Every first departure records its acquired-age offset; later departures of the same station cannot rewrite that value. Multiple first departures on one sample retain identical offsets. A qualified upright return after a station's first departure sets its retained rebound bit. Returns do not erase seen membership and do not abort the attempt. This is why reconstructing order from current occupancy alone would be wrong.

When every station has been seen and all raw and qualified inputs remain down for 250 full acquired milliseconds, the attempt finishes. Both interval endpoints must be down. A raw glitch breaks this quiet window even if it never qualifies as a return. Alternatively the attempt ends at age 10,000 ms. The deadline has precedence over newly qualifying transitions on that exact sample, so an opening qualifying at the deadline is not credited. Partial means at least one station never produced an accepted first departure before finalization.

Outcomes distinguish full and partial, each with or without an observed qualified return. Completed results stay immutable through DONE and later rearming until the next attempt begins. Rebuilding after DONE is not retroactively counted as rebound. All four upright inputs must again satisfy the 500 ms baseline before READY. An early rebuild while ACTIVE is still evidence and can set return bits; wait for DONE if that distinction matters. There is no manual stop command or per-run configuration.

Every sample update is staged before commitment. Counter exhaustion rolls back the candidate sample and enters HALT rather than exposing half a completion. A missed acquisition tick also enters HALT and abandons an active attempt exactly once. It does not credit unknown time or automatically resume from a convenient contact pattern. Configuration and watchdog-start failures are terminal. Clock restoration failure stops before establishing an untrustworthy serial interface. A watchdog reset clears volatile history and requires a fresh baseline; it cannot preserve a trial across power failure.

Cumulative identities are starts = completed + aborted + active, and completed = full + partial. Disturbed counts completed attempts with a return, not individual return edges. A faulted pending attempt increments aborted, not completed. UART errors are separately retained. A busy transmitter never blocks acquisition; a stuck transmission is abandoned after 100 ms. Unsigned uptime subtraction permits ordinary timer wrap, while diagnostic totals do not silently wrap. The host must not confuse advancing HALT timestamps with continued acquisition.

## Protocol and companion application

Every approximately 100 ms the firmware emits a bounded ASCII snapshot with LF termination. The fixed DL1 policy includes channel order and timing constants; changing those semantics requires a new protocol version. Fields, in order, are:

    DL1,now,samples,phase,fault,raw,mask,valid,starts,completed,full,partial,
    disturbed,aborted,outcome,age,seen,rebound,baseline,quiet,
    offset_a,offset_b,offset_c,offset_d,tx_errors*HHHH

The physical record is one line; the layout above wraps only for readability. Integers are unsigned decimal, with 4294967295 meaning an absent offset. Masks use A=1, B=2, C=4 and D=8. Phases are WAIT=0, READY=1, ACTIVE=2, DONE=3 and HALT=4. Outcomes are pending=0, clean-full=1, clean-partial=2, disturbed-full=3, disturbed-partial=4 and aborted=5. Here “clean” means no qualified return observed, not mechanically flawless. Fault codes are none=0, acquisition gap=1, overflow=2, configuration=3 and watchdog initialization=4.

CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial 0xFFFF over the ASCII bytes before the asterisk. HHHH is four uppercase hexadecimal digits. CRC detects corruption, not impersonation. This local, receive-only observation protocol has no authentication or firmware-update service. Do not interpret a recording from an untrusted source as authenticated physical evidence.

Run `python3 app/workbench.py` from this directory on Linux with Python Tk. Connection provides explicit serial open, disconnect and replay actions. Cascade shows readiness, counters, contacts, first offsets and return evidence. History/export shows the latest 100 accepted snapshots; memory and exports retain at most 2000 snapshots. This is not a lossless trial database. A disconnected host cannot recover earlier overwritten trials, and late attachment may first show a result from before connection.

The decoder bounds frames, resynchronizes on the entire DL1 marker after truncated traffic, and rejects invalid CRC, enum, range and conservation combinations. The session rejects counter rollback, contradictory same-time frames, rewritten first offsets and changed frozen evidence. These are necessary consistency checks, not a mathematical reconstruction of every hidden transition between sparse snapshots. Reset inference is heuristic because there is no persistent boot identifier: some resets with lost startup records remain indistinguishable from forward traffic.

Only advancing accepted timestamps refresh reception progress. A live host pause exceeding two seconds, or cumulative host/device disagreement exceeding a fixed two-second budget, blocks resumed acceptance and requires reconnect. Uncalibrated HSI drift can eventually cause a conservative stop during long sessions. The recent-acquisition indicator also requires a connected live session, a non-HALT state and progress within one second. It is not a calibrated proof of absolute acquisition freshness; initial transport latency remains unknown.

Successful reconnect starts a fresh session and clears old displays. Failed connection attempts preserve existing evidence. A second open is refused while the first port is owned, and disconnect restores its original terminal settings. Use ordinary serial-device permissions, not root execution to bypass denial. Replay accepts regular files up to two MiB, rejects FIFOs and incomplete tails, and is never labeled live. CSV export exclusively creates a new owner-only file, refuses overwrite, and records author, provenance, connection/freshness state, parser errors and any blocking warning before the data rows.

## Reproducible builds and acceptance limits

Required tools are ARM GNU GCC, native GCC with sanitizers, Python 3 with Tk, KiCad 9 with its Python pcbnew bindings and installed footprints, and Xvfb for headless UI checks. The tested versions are recorded in VALIDATION.md. There are no pip or npm application dependencies. Run from this directory:

    python3 tools/verify.py
    python3 app/workbench.py

The verifier performs a clean target build, sanitizer tests, an independent exhaustive station-history oracle, generation of a synthetic stream by the production C encoder, Python model/transport tests, real Tk event-loop tests through Linux pseudo-terminals, source attribution/size checks and KiCad checks. The synthetic stream exercises full, disturbed and partial attempts. It is test input, not measured hardware data. Build products are ignored by Git and regenerated locally.

CAD source regeneration is explicit: `python3 tools/generate_hardware.py`. The checker runs ERC and DRC, exports a fresh schematic netlist, and compares every component, footprint identity and pin/net assignment against the actual PCB pads and connectivity manifest. Real embedded symbols, labels, footprints, tracks, vias and a closed board outline are supplied. Passive-only ERC cannot validate the external supply, Nucleo wiring or magnetic fixture. No waiver substitutes for a connected net.

`make -C firmware flash` offers an OpenOCD/ST-LINK programming path, but it was not executed without a physical target. Remaining acceptance work includes fabrication review, continuity, rail measurement, target programming, clock and watchdog checks, oscilloscope characterization, actual USB disconnection, sensor-threshold calibration and video correlation for every accepted first fall and return. Repeat with sensors removed to quantify disturbance. EMC, ESD, enclosure durability, regulatory qualification and real-user usefulness remain unverified. Passing software and CAD checks does not authorize precision, safety or commercial-readiness claims.
