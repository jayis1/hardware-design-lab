# Braid Trace — indexed three-strand crossing-guide notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an unbuilt engineering prototype. Target linking, simulated-input software tests, desktop workflows and electrical CAD checks are reproducible. No circuit has been fabricated, MCU flashed, guide assembled, contact calibration performed or actual braid recorded. The custom PCB is a passive carrier for a separately purchased NUCLEO-F401RE. Its processor, regulator, decoupling, debugger and USB bridge belong to that development board, not this carrier. This device reports operator declarations, not independently verified textile topology.

## Problem, purpose and intended user

A craft instructor teaching three-strand decorative braiding can return the free ends to their original left-to-right order while leaving substantial crossings in the material. A photograph of the endpoints cannot explain what happened, and a tally of moves cannot distinguish passing one strand over its neighbor from passing it underneath. When a learner tries to reverse a few moves, both people can lose track of which adjacent pair was manipulated. Braid Trace makes that limited teaching problem inspectable without filming the participant.

The proposed instrument combines a three-lane cord fixture, a captive two-position crossing guide, two declaration paddles and a small local recorder. The guide selects the left or right adjacent pair of current positions. After making one crossing, the operator presses and releases OVER or UNDER while the guide stays in that detent. The recorder appends a signed generator to a bounded word, computes the resulting endpoint permutation, and separately reduces immediately adjacent inverse pairs. The app draws the full declared word, not merely the reduced result. A diagram can therefore explain why two positive left crossings restore the endpoint order but do not cancel.

The primary user is a textile or mathematics workshop instructor working with short, removable practice cords. On day thirty, that instructor should be able to keep a repeatable record of a confusing short exercise, compare it with a printed sequence, and distinguish a reversed crossing from a repeated crossing without reconstructing a video. This is a proposed practical benefit requiring a classroom trial, not a measured learning outcome. A wrong record wastes lesson time and can misdescribe a practice sample; it must never approve a load-bearing rope, climbing knot, medical suture, electrical harness or safety restraint.

Hardware is useful here because the pair indication belongs to a physical guide rather than an unrelated screen selection. The guide remains next to the moving cord positions; the paddles can be reached without touching a laptop. The electronics also reject declarations made while the guide moves. Nevertheless, this is not automated braid recognition. A person can press a paddle without crossing a cord, cross the wrong cord, turn the fixture around or forget a move. A software-only braid visualizer is a good alternative when that physical association is unnecessary. This design earns its extra hardware only if a supervised trial finds the guide and explicit rejection useful.

## Originality and comparison scope

The repository directory and README concept inventory was checked before choosing this device. Pick Witness records a loom shuttle, shed and beater sequence, rather than signed crossings of three free strands. LoomMapper maps electrical wiring harness connectivity; its name does not mean textile braiding. Texture Cycle retains subjective preference edges between swatches. Chord Window measures switch chord timing. None of those devices retains a signed adjacent-generator word, a strand permutation and an immediate-inverse reduction for an indexed braiding guide. The F401 acquisition, UART and passive carrier infrastructure is explicitly reused from jayis1's texture-cycle at commit ffed391; reusing resistors and board support is not the claimed invention.

Braid diagrams and braid-group software already exist. The University of Colorado Braid Groups interactive page lets users drag strands and explains braid equivalence and inverses. The Braid Visualizer at venkatb.me provides levels, compression, image saving and reset. These are useful prior art, not competitors whose capabilities have been exhaustively evaluated. Braid Trace proposes a physical pair-selector/declaration fixture linked to conservative, local evidence recording. Worldwide novelty, patentability, trademark clearance and absence of similar workshop projects have not been established.

References consulted for the limited comparison:

- https://math.colorado.edu/~cama5144/braids.html
- https://venkatb.me/braid_visualizer.html

Non-goals include recognizing a braid from a camera, measuring tension or cord diameter, determining knot equivalence, solving the braid-group word problem, issuing a craft-quality score, identifying participants, and driving motors. Immediate inverse cancellation is deliberately not a complete normal form. In particular, the braid relation connecting left-right-left and right-left-right positive sequences is not implemented as a rewrite. Retain the original word when comparing exercises.

## Hardware specifications and architecture

| Subsystem | Revision A design |
| --- | --- |
| Processor | STM32F401RET6, Cortex-M4, on external NUCLEO-F401RE MB1136 |
| MCU resources | 512 KiB flash; 96 KiB RAM; internal HSI at nominal 16 MHz |
| Sensors | Two voltage-free guide-detent contacts, two voltage-free paddle contacts |
| Contact examples | Signal-rated gold-contact microswitches; exact actuation geometry chosen during fixture commissioning |
| Acquisition | One observation per 1 ms SysTick advancement |
| Qualification | Five identical consecutive observations per input group |
| Host link | USART2 TX at 115200 baud, 8N1, ST-LINK virtual COM over USB |
| Power | Nucleo USB source; carrier receives only regulated 3.3 V and GND |
| PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Components | Eight axial resistors and six through-hole headers |
| Host software | Python 3, Tk, Linux POSIX serial; standard library only |
| Storage | Volatile full word of 32 crossings; host CSV exports |
| Actuation/radio | None; no battery, charger or radio |

Architecture and block diagram:

    three cords anchored A B C -> movable pair guide -> left/right detent contacts
                  operator -> OVER and UNDER declaration paddles
                                         |
                  J3 LEFT -- 1k -- SENSE0 / PA0 / A0
                 J4 RIGHT -- 1k -- SENSE1 / PA1 / A1
                  J5 OVER -- 1k -- SENSE2 / PA4 / A2
                 J6 UNDER -- 1k -- SENSE3 / PB0 / A3
                                         |
                                   10k each to 3V3
                                         |
    USB -> NUCLEO-F401RE -> grouped qualification -> stroke envelope -> word ledger
                         -> CRC snapshots -> PA2 / USART2 -> ST-LINK VCP
                         -> Linux connection / diagram / history / setup-export

J1 pin 1 is VCC from Nucleo CN6 pin 4, 3V3. J1 pin 2 is ground from CN6 pin 6. J2 pins 1 through 4 connect to CN8 pins 1 through 4, respectively PA0, PA1, PA4 and PB0. J3 through J6 each have signal on pin 1 and ground on pin 2. They are wired alongside the Nucleo; the carrier is not a mechanically mating Arduino shield. PA2 alternate function AF7 supplies USART2 transmit. The factory ST-LINK virtual-COM solder-bridge configuration is required: SB13/SB14 fitted and SB62/SB63 open. Do not attach another output to PA2.

Relevant platform documents are ST UM1724 for the MB1136 board and RM0368 for STM32F401 registers. This revision reuses the repository's existing register definitions and connector map; no new manufacturer-document audit or physical board-revision check is claimed. Confirm the purchased board's markings, connector orientation, power jumpers and virtual-COM routing against its manufacturer documentation before applying power. The supplied startup vector and linker script are for direct reset into flash at 0x08000000, not arbitrary bootloader handoff.

## Electrical limits, power and assembly

All four external contacts must be disconnected from other electrical equipment. The circuit shares the USB computer's ground and is not galvanically isolated. It has no dedicated surge suppressors or industrial input protection. A 1 kohm series resistor does not make a powered switch matrix, long outdoor cable or unknown voltage safe to connect. Use short indoor leads, initially below half a meter, with strain relief. Keep the carrier in an insulating tray and away from wet materials, conductive dust and exposed metal.

Each closed contact draws nominally 3.3 V divided by 11 kohm: 0.30 mA. The MCU-side low voltage is nominally 0.30 V. Four closed channels draw 1.20 mA through the resistor network, or 3.96 mW at the supply. These are calculations, not measured currents or input margins. Use contacts rated for low-current signaling; a power switch that relies on substantial wetting current may give intermittent results. A broken lead looks like an open switch and is not diagnosed automatically. At normal detents only one of the two pair contacts should be closed.

Reserve a conventional protected 5 V, 500 mA USB supply for the Nucleo and debugger assembly. Actual whole-device current has not been measured. The firmware continuously samples during an attached session and makes no low-power or battery-runtime claim. There is no sleep implementation, cell management or charging path to qualify. The custom PCB must not receive a second independent supply while connected to the Nucleo rail.

A planning BOM consists of one NUCLEO-F401RE, four low-current contacts, four 10 kohm resistors, four 1 kohm resistors, five two-pin headers, one four-pin header, the carrier PCB and an insulating mechanical fixture. Budget approximately USD 20–35 for the development board, USD 12–40 for contacts and mounts, USD 3–10 for a small-batch carrier and passive parts, and USD 10–30 for cord guides, wiring and enclosure materials. These are budgeting assumptions, not current supplier quotations. Through-hole assembly avoids fine-pitch rework and RF layout. Production cost reduction is outside this revision.

## Mechanical fixture and commissioning

Use an insulating base approximately 240 by 180 mm with three origin clamps spaced 40 mm apart. Fit replaceable smooth cord guides and use short soft practice cords, for example 2–4 mm diameter. These dimensions are prototype starting points, not validated tolerances. Mark the clamped end as ORIGIN and label the starting cords A, B and C. The operator always looks from that origin toward the free ends; rotating the fixture changes the meaning of the over/under convention and invalidates comparison.

A captive sliding pair window has two stops centered over positions 1–2 and 2–3. Two separate detent contacts report left and right. There must be a neutral region in which neither is closed and no intended position in which both close. The window selects the working pair but does not lock the cords into a verified crossing. Route leads away from the slide and add mechanical stops that do not depend on switch bodies. Mount OVER and UNDER paddles beside the guide with visibly different labels. Do not mechanically couple both paddles to one ambiguous rocker in this revision.

Before recording, disconnect the fixture from any other circuit, inspect continuity with a meter, connect the carrier, and verify each contact bit in the app. Left is bit 0, right bit 1, OVER bit 2 and UNDER bit 3. Confirm that the neutral selector reads zero, left reads one, right reads two, and both-detent closure reads three and cannot be used for an accepted declaration. Test each paddle independently. Then restore three separate cords to A B C, deliberately reset the MCU and reconnect the application. MCU reset does not physically restore the cords and must never be treated as automatic homing.

There is no analog calibration constant. Commissioning checks the correspondence between mechanical pose and digital contacts, the sign convention, and the minimum stable hold. Keep every intended state stable for at least 20 ms; the nominal five-sample qualification is not a promise to capture arbitrarily fast movements. Recheck the contacts after adjusting the guide. Record any changed clamp spacing, cord material or viewing orientation in the export label or an accompanying workshop note. A camera can be used during validation to compare declarations with actual actions; that comparison has not been performed here.

## Firmware contract and design decisions

The firmware is freestanding C11 with no vendor HAL, heap, operating system or command receiver. board.h defines the external board contract; registers.h defines the MMIO addresses used by the drivers. Startup initializes data and BSS, supplies core exception vectors and reserves the configured SRAM stack. The clock driver restores the reset HSI tree using bounded polling before GPIO or serial initialization. The GPIO driver configures four inputs and PA2 transmit. The independent watchdog is initialized with bounded status waits and refreshed by acquisition progress. Hardware timing, watchdog reset behavior and electrical baud accuracy remain untested on a physical target.

Two selector bits qualify as one bundle, so intermediate bit changes cannot synthesize a detent out of independently delayed bits. OVER and UNDER qualify independently. A group qualifies after five identical samples, spanning four nominal millisecond intervals. Duplicate calls at the same timestamp do nothing. Ordinary 32-bit millisecond wrap is supported; any other inter-sample interval than one millisecond permanently halts the word ledger because a missing crossing cannot be reconstructed from later quiet contacts.

WAIT requires qualified inputs and raw/qualified released paddles. READY allows selection changes with both paddles released. The first qualified paddle closure admits a stroke and captures the prior selector identity. Raw selector changes and raw appearance of both paddles anywhere in the declaration envelope taint it, even if an opposite pulse was too short to qualify. ACTIVE waits for both raw and qualified paddles to return low. A clean stroke at selector one or two appends one generator; an uncertain stroke increments rejected without changing the word. A ten-second active deadline aborts and permanently halts; release qualifying exactly at the deadline loses. This rejects coincident endpoint evidence rather than assuming a favorable order.

Codes are 1=L+, 2=R+, 3=L-, 4=R-. Positive means the left member of the selected adjacent pair goes over the right member, viewed from the origin. Negative means under. The original word is retained in four 32-bit packed fields, eight four-bit codes per field, earliest code in the low nibble. Unused nibbles must remain zero. Endpoint identities start A B C and exchange positions on either sign. A separate stack cancels an immediately adjacent generator and its inverse. The conservation rule is accepted = reduced depth + twice the cancellation count. Positive and negative counts sum to accepted.

At 32 accepted crossings the complete word remains available. A further clean stroke aborts and halts with capacity fault rather than dropping an old crossing. Rejected strokes do not spend word capacity. Counter exhaustion halts before admitting an unrepresentable start. Every admitted stroke has exactly one disposition: accepted, rejected, aborted or currently active. Faults preserve the word and accumulated evidence. A deliberately new physical setup and MCU reset are required after halt; reconnecting the host cannot repair a missing operation.

## Protocol and companion application

The device emits self-contained BT1 snapshots every nominal 100 ms when the nonblocking transmitter is free. ASCII decimal fields have no leading zeros except zero itself. The payload begins BT1 followed by comma-separated fields; an asterisk introduces four uppercase hexadecimal CRC digits, followed by LF. CRC-16/CCITT uses polynomial 0x1021 and initial value 0xFFFF over the payload before the asterisk. CRC detects accidental corruption, not impersonation. Only trusted local serial devices and files should be used.

Field order is now, state, levels, valid, starts, accepted, rejected, aborted, gaps, faults, tx_errors, age, tainted, selected, outcome, depth, cancelled, permutation, positive, negative, word0, word1, word2, word3. All fields are unsigned 32-bit decimal values. State values are WAIT=0, READY=1, ACTIVE=2 and HALT=3. Fault bits are configuration=1, watchdog initialization=2, counter overflow=4, acquisition gap=8, stroke timeout=16 and capacity=32. Permutation is the base-three encoding of the left-to-right strand identities A=0, B=1 and C=2. The initial value is 21, not zero.

The Connection screen opens an explicitly selected Linux serial path; there is no radio pairing or network discovery. Successful replacement clears previous numeric and diagram displays. Failed opens preserve existing evidence. The Crossing diagram screen shows all retained crossings with strand colors, over/under separation, current endpoint order, reduced word and exponent sum. Identity endpoint order is explicitly not proof of an unbraided object. History lists changed declaration outcomes; Setup / export explains the fixture contract and writes a private CSV without overwriting an existing path.

The parser bounds chunks and frames, checks CRC, recomputes the summary from the full word, and verifies immutable word prefixes between snapshots. It rejects counter rollback, contradictory duplicate timestamps, lost sticky faults and changes to halted evidence. Sparse transition checks are necessary consistency bounds, not proof of every hidden raw transition. Two advancing snapshots are needed before the app labels transport live. Duplicate frames do not renew freshness. A reception pause or cumulative host/device divergence over two seconds requires reconnect; oscillator drift may therefore conservatively interrupt a long session.

Replay is always labeled replay, including the synthetic fixture generated by tests. Replay inputs must be regular files no larger than two MiB, with at least one valid snapshot and no partial tail. The host retains 2000 snapshots; each contains the entire bounded crossing word, so dropping older status rows does not erase the current word. CSV metadata records author, provenance, label, connection state, freshness, parser errors and warning. Labels are protected against spreadsheet formula interpretation. Export is an explicit local action; there is no cloud upload, telemetry service or participant identifier.

## Build, verify and physical release gates

From this device directory, run `python3 tools/verify.py`. Prerequisites are GNU make, GCC with AddressSanitizer/UndefinedBehaviorSanitizer, arm-none-eabi-gcc and binutils, Python 3 with Tk, Xvfb, KiCad 9 CLI and the system Python pcbnew bindings. The verifier builds and links the Cortex-M4 image, runs engine/clock/UART host tests, generates a synthetic recording with the real C encoder, exercises the app and GUI, and checks schematic/PCB connectivity plus ERC and DRC. It also enforces the README word count, substantive firmware line count and author attribution. No package installation or network access is needed by the tests.

Launch the companion with `python3 app/workbench.py`. The generated `firmware/build/demo.bt1` is useful for replay and is explicitly synthetic. To rebuild hardware files after a reviewed electrical change, use `/usr/bin/python3 tools/generate_hardware.py`; then rerun all checks. The KiCad project contains authored embedded symbols, actual standard-library footprints, named nets and routed copper. Passive-only ERC cannot validate the external Nucleo supply or a mechanical fixture. The connectivity checker compares every pin, net, component value and footprint identifier across exported schematic XML, actual PCB pads and the manifest.

`make -C firmware flash` requires OpenOCD and an attached ST-LINK target and programs the built ELF. That command was not executed during this design run. Before flashing or fabrication, review the purchased board revision, cable polarity, clearances and mechanical retention. Then bring up one input at a time, verify 3.3 V and sense levels, measure serial timing, intentionally unplug a sensor, test watchdog recovery, and compare recorded actions with video under controlled conditions. Repeat over many short exercises before claiming practical reliability. These physical release gates remain open; the repository deliverable is a reproducible design and tested software prototype, not a certified finished instrument.
