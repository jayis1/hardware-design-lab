# Scoop Window — paired-bin utensil absence and foreign-opening notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. The custom PCB is a routed passive input carrier for a separately purchased NUCLEO-F401RE. The MCU, regulator, USB interface, debugger and processor decoupling are on that purchased board. Software builds and CAD checks are reproducible, but no physical assembly has been fabricated, flashed, measured or used in a training session. This instrument does not detect contamination, identify a scoop, certify hygiene, or prevent a handling mistake.

## Purpose, audience and practical utility

Scoop Window helps a training instructor compare two layouts for a manual, two-bin dispensing station. Each bin has a dedicated scoop dock and a closed-lid contact. The instrument records an observable opportunity for a mix-up: one dedicated scoop is absent from its dock while the other bin is open. It preserves separate counts and sampled overlap durations for the two directions. The instructor can move the docks, change the order of a lesson, or introduce a return-before-opening rule and compare what actually happened at the physical station.

A useful first application is an adult training bench containing differently coloured, nonhazardous craft beads. Learners transfer material from two lidded bins into separate receiving trays, returning each scoop before moving to the next bin. An instructor normally watches the activity and takes handwritten notes. A phone stopwatch records button presses rather than the two concurrent physical states. Video can answer richer questions but requires review, records people, and may obscure a dock behind a hand. This device deliberately observes only four fixture states and produces a local record without a camera, account, network connection or identity database.

The hardware has a narrow reason to exist: the event of interest is a relationship between physically separate handles and lids, sometimes involving two people. It cannot be measured from a phone interface alone. Passive contacts make an inexpensive, removable sensor arrangement that is visible to the learner. The firmware does not need to guess what a hand looks like or infer utensil motion from video. On the other hand, the contacts cannot prove which utensil is present, where an absent scoop went, whether any material was touched, or whether a contaminant moved. A flagged excursion is an observation for discussion, not an accusation or a safety finding.

Other possible audiences include museum activity designers, makerspace instructors, and ergonomics students studying dedicated-tool parking. Use only dry, inert demonstration material in this revision. Food-service operators may recognise the workflow, but use with food, allergens, medicines, laboratory reagents, ceramic dusts or hazardous chemicals is outside this design's validated scope. Neither an unflagged excursion nor a low overlap total establishes safe practice.

## Originality and design rationale

The repository's device directories and README titles were inventoried before selecting this concept. The closest physical workflow neighbours are Pick Witness, which correlates shuttle arrival with loom shed and beater states, and Clamp Relay, which records overlap and uncovered intervals during manual workholding transfer. Pantry Warden observes shelf conditions and spoilage-related environmental signals. None implements paired dedicated-scoop absence with the opposite bin's lid opening, independent excursion conservation, and direction-specific foreign-window accounting.

The proposed design distinction is this specific teaching fixture and its conservative evidence contract. Dedicated scoop holders already exist: Cambro's Ingredient Bin Scoop Holder and Manitowoc's Multi-Mount External Scoop Holder are relevant examples, and scoop-holder patents also exist. A limited search is not a market survey or patent opinion. No worldwide first, commercial absence, patentability, or superiority over those products is claimed. The reusable STM32 startup, register drivers, bounded serial transmitter, carrier topology, CAD generation approach, and Tk transport/export shell derive from jayis1's existing Clamp Relay platform. Reusing those foundations is intentional; the two independent excursion engines, SW1 protocol validation, tests and application presentation are specific to Scoop Window.

An RFID design could identify individual utensils, but it adds antenna placement, tag cleaning and read-zone ambiguity that are not needed to answer this first question. A camera could identify movements but introduces occlusion and privacy concerns. Four contacts offer a simpler experiment whose limits can be explained in a lesson. A later identity-aware revision would require a new protocol and validation programme rather than quietly relabelling the current dock sensor as identification.

## Mechanical fixture and operating envelope

Use two stable lidded bins on a dry tabletop. Give them large A and B labels. Place each scoop dock beside its own bin, outside the material-contact area. Make the dock geometrically distinct from a general resting surface so a scoop cannot casually balance on the switch. A retained magnet on the handle closes a protected reed switch only when the handle is seated. A separate retained magnet on each lid closes its reed only when the lid reaches its closed stop. All magnets and glass reed bodies must be enclosed against detachment and breakage.

The initial mock-up can use adjustable brackets and screws outside the bins rather than adhesive inside the material area. No enclosure dimensions for a particular commercial bin are assumed. Reserve an approximately 100 by 90 mm insulated electronics enclosure for the 80 by 70 mm carrier; put the Nucleo in a separate insulated enclosure, or provide a larger enclosure with verified clearance for both assemblies. The carrier has no mounting holes: use an insulating edge-retention rail, not improvised screws through copper. Provide cable strain relief and leave USB accessible. Do not claim an ingress-protection rating.

Keep the four contact leads short, initially at most 0.5 m each, and paired with ground. Route them away from hinges and hands. Label both ends of each lead. Check that the lid magnet does not accidentally actuate a dock reed and that neither dock can be actuated by the other scoop's normal parking position. The electronics cannot enforce these mechanical requirements. A loose magnet can simulate a docked scoop and a shorted wire can simulate a closed lid.

The instrument is for slow manual actions. Hold meaningful fixture states for at least 20 ms during commissioning, and perform ordinary lessons much more slowly. The digital qualifier requires five consecutive one-millisecond observations, not five milliseconds of guaranteed continuous electrical stability. Sub-sample transitions may be missed. Each channel qualifies independently, so chatter on A's lid cannot suppress observation of B's lid or A's dock. Previously qualified states persist while a replacement state is being qualified; this is debounced-state evidence, not a waveform recorder.

## Hardware specifications and connections

| Function | Implementation | Connection |
|---|---|---|
| Processor | ST NUCLEO-F401RE, MB1136, STM32F401RET6 | External purchased module, HSI at 16 MHz |
| Four sensors | Littelfuse MDSR-7 normally open reed contacts, sensitivity selected for fixture | Passive, magnet actuated, external to carrier |
| A dock occupied | J3.1 DOCK_A, J3.2 GND | SENSE0, J2.1 to CN8.1 / A0 / PA0 |
| A lid closed | J4.1 LID_A, J4.2 GND | SENSE1, J2.2 to CN8.2 / A1 / PA1 |
| B dock occupied | J5.1 DOCK_B, J5.2 GND | SENSE2, J2.3 to CN8.3 / A2 / PA4 |
| B lid closed | J6.1 LID_B, J6.2 GND | SENSE3, J2.4 to CN8.4 / A3 / PB0 |
| Carrier supply | J1.1 VCC, J1.2 GND | CN6.4 3V3, CN6.6 GND |
| Host telemetry | USART2 TX, PA2 alternate function 7 | ST-LINK virtual COM, 115200 baud, 8N1 |
| Carrier construction | Two copper layers, 1.6 mm nominal board | 80 by 70 mm, 0.3 mm routed tracks |
| Programming | Nucleo's onboard ST-LINK / SWD | OpenOCD target stm32f4x |

Each channel has a 10 kΩ resistor from 3V3 to SENSE and a 1 kΩ resistor from SENSE to its contact connector. Contact pin 2 returns to ground. A closure therefore produces a low electrical input, which the GPIO driver converts to a logical one. The lid signals mean CLOSED, not OPEN; the application inverts the foreign lid's meaning only when evaluating the overlap. Swapping this convention would invert the measurement and must be caught during commissioning.

The calculated current in one closed 11 kΩ path is 0.3 mA, and its SENSE voltage is 0.3 V at a nominal 3.3 V supply. Four closed paths draw 1.2 mA. Open contacts draw approximately zero resistive-path current, excluding leakage. Use contacts suitable for low-level switching rather than a switch requiring substantial wetting current. There is no active component on the carrier requiring a separate decoupling network; the external Nucleo provides the processor's regulation and decoupling. The carrier is not a standalone processor board.

The Nucleo USB connection supplies the assembly. Budget 150 mA at 5 V for the external development board as an unmeasured planning allowance; confirm actual current on the assembled unit. There is no battery, charging circuit or sleep-mode battery-life claim. Power loss resets volatile counters. The firmware remains awake to observe slow events and service the serial transmitter; optimising battery sleep is not a goal of this revision.

There is no isolation, reverse-polarity protection or dedicated cable ESD protection on the carrier. The 1 kΩ resistors are not safety barriers. Connect only disconnected, voltage-free contacts, never powered industrial outputs or machine controller terminals. USB creates a common ground with the host computer. A broken lid wire appears open; a broken dock wire appears absent. Those conditions may produce a useful warning but are not uniquely identifiable faults. A shorted lid wire can hide a true opening. Physical inspection remains mandatory.

ST's UM1724 table 16 documents the CN8 and CN6 mappings. Section 7.10 describes the default USART2 bridge: SB13 and SB14 ON, SB62 and SB63 OFF. Verify the actual board revision and solder bridges before wiring. The firmware uses HSI rather than depending on an external HSE or ST-LINK clock. It expects a normal reset entry, not arbitrary handoff from another bootloader with interrupts and memory mapping changed.

## Architecture and block diagram

```text
A dock reed -- J3 -- 1k -- SENSE0 -- PA0  \
A lid reed --- J4 -- 1k -- SENSE1 -- PA1   \
B dock reed -- J5 -- 1k -- SENSE2 -- PA4    > NUCLEO-F401RE
B lid reed --- J6 -- 1k -- SENSE3 -- PB0   /  1 ms acquisition
                 each SENSE -- 10k -- 3V3   independent qualifiers
                 each contact return -- GND       |
                                            A/B excursion engines
                                                   |
                                         SW1 snapshots + CRC16
                                                   |
                                         USART2 -> ST-LINK USB
                                                   |
                                         Linux Tk companion
                                  connection / live / history-export
```

The engine owns all accounting. The application does not reconstruct missing excursions from a low-rate status stream. Every snapshot contains cumulative counters, current qualification masks and fault status, so attachment after boot does not require an initial greeting to interpret the format. The transport is receive-only from the host's perspective. No command can change policy, clear counters, drive an actuator or open a lid. Resetting the physical Nucleo starts a new volatile measurement session.

## Firmware measurement contract

Each scoop is independent. A qualified occupied dock establishes readiness for that scoop. A subsequent qualified empty dock starts an excursion. Returning to the same dock completes it; completion is a disposition, not approval of the handling. If the device starts with a scoop already absent, it does not invent a departure. The scoop must first be observed docked. Both scoops may have active excursions at once, and neither is required to return before the other is counted.

For scoop A, the foreign condition is B's lid qualified open while A's excursion remains active. B uses A's lid symmetrically. The first such qualified condition marks the excursion and increments its flagged counter. Repeated openings during the same excursion do not increment that counter again. A simultaneous newly qualified departure and foreign opening can mark the excursion immediately, even before a full interval of overlap exists. A simultaneous return and foreign opening is not treated as ordered access: the return ends the excursion and no new overlap bin is claimed. These endpoint conventions are deliberate and tested, not sub-millisecond ordering guarantees.

Away duration accumulates one bin when the excursion was active at both endpoints of a consecutive one-millisecond sample. Foreign duration additionally requires the other lid to be qualified open at both endpoints. Startup, completion and unknown boundaries do not contribute a bin. Totals are therefore sampled durations under the qualification policy, not exact physical residence time. Foreign duration is a subset of away duration for each scoop. A and B totals may overlap in wall time; never add them to infer session duration.

For each channel, starts equals completed plus aborted plus active. Flagged excursions cannot exceed the sum of completed, aborted and currently marked activity. An acquisition step other than exactly one millisecond aborts active excursions, increments the gap diagnostic, clears qualification and readiness, and retains historical counts and durations. Redocking is required before a new departure is counted. An absent scoop after a gap is unknown ongoing activity, not an automatically restarted excursion. Timestamp arithmetic handles the ordinary unsigned timer wrap.

Counters never silently roll over. Each sample is evaluated on a staged engine copy. If any counter would overflow, the whole speculative tick is discarded; the previous active excursions are aborted once and terminal overflow fault 4 is latched. Configuration and watchdog-start failures use fault bits 1 and 2. Once faulted, measurement evidence is frozen while uptime and transmission diagnostics may still advance. The terminal fault must not be interpreted as a live observation simply because records continue arriving.

The driver layer uses only the STM32 registers it needs. It establishes the reset clock, configures inputs without internal pull resistors, selects USART2 TX, and starts SysTick. Clock waits before SysTick use an iteration budget; watchdog setup uses bounded elapsed time. The independent watchdog is fed after acquisition progresses, not merely because a serial loop runs. Serial transmission copies a bounded frame, sends bytes only when TXE permits, and abandons a stalled record after 100 ms. A later full marker lets the companion recover from the partial record. There is no heap allocation, file system, flash wear mechanism, network stack or hidden host-build substitution in the target firmware.

## SW1 interface specification

Every record is ASCII `SW1,` followed by 22 unsigned decimal fields, then `*`, four uppercase hexadecimal CRC digits, and LF. CRC-16/CCITT-FALSE covers the body from S through the final decimal digit: polynomial 0x1021, initial value 0xffff, no reflection, no final XOR. CRC detects accidental corruption; it does not authenticate the sender.

The field order is:

1. qualify, now, levels, qualified, ready, active, marked;
2. a_starts, a_completed, a_aborted, a_flagged, a_away_ms, a_foreign_ms;
3. b_starts, b_completed, b_aborted, b_flagged, b_away_ms, b_foreign_ms;
4. gaps, tx_errors, faults.

`qualify` must be 5. `now` is unsigned milliseconds. `levels` and `qualified` use bits 0 through 3 in the hardware table's order. `ready`, `active` and `marked` use bit 0 for A and bit 1 for B. All counters are unsigned 32-bit integers. Snapshots are attempted every 100 ms, with no claim that the host receives each one. There is no response format because no inbound commands are implemented. An unsupported marker or qualification constant is rejected rather than guessed.

The companion bounds frames to 511 bytes, checks CRC and field ranges, enforces conservation and duration subsets, and rejects impossible snapshots even if their CRC is valid. Across snapshots it checks monotonic counters, sticky faults, terminal immutability and duration growth bounded by device time. Timestamp rollback and contradictory duplicate timestamps freeze the session. Identical duplicates do not refresh freshness. Two advancing status timestamps are required before live progress is shown.

Without a persistent boot identifier, some resets remain indistinguishable from forward traffic. Do not merge sessions across power interruptions. Live freshness also checks host/device elapsed-time agreement, with a bounded oscillator tolerance, and blocks after a prolonged host reception pause. This can conservatively require reconnect during a long uncalibrated run. Reconnect flushes serial input and replaces the host session; it does not reset device counters. Record start and end snapshots if a lesson's delta rather than the device's lifetime total is needed.

## Companion application and local data

Run `python3 app/scoop.py` on Linux with Tk installed. The application uses Python's standard library, Tk, POSIX termios and poll; it has no pip dependency or internet service. Python 3.13 and Tk were used for automated GUI tests. The Connection screen accepts a serial device path, connects and flushes, disconnects without discarding retained evidence, and opens a bounded SW1 recording for replay. Close any other terminal application holding that port before connecting.

The Live witnesses screen shows dock/lid states, active masks, faults, gaps, completed/aborted counts and both directions' cumulative durations. Closed means the electrical contact is closed, so for a dock it means occupied and for a lid it means shut. Unknown qualification is shown separately. The History/export screen retains up to 2,000 snapshots and displays each accepted row rather than only a summary. The table is intentionally a status history, not a claim to have saved every physical event.

Exports use CSV with a session metadata row containing author, provenance, connection state, freshness, parser errors and any blocking warning. Each subsequent row preserves the full protocol and host age. Files are created privately with mode 0600 and exclusive creation, so an existing export is not overwritten. No information is uploaded. Keep exported recordings with the lesson's fixture notes, and delete them locally according to the participants' agreed retention policy.

Replay accepts only regular files of at most two megabytes, refuses empty or malformed recordings, and rejects an incomplete final frame. A failed replay leaves the old evidence intact. Replaying cannot take over an open serial connection. Disconnect retains the original live or replay provenance; it never relabels retained readings as a new capture. Serial unplug is detected through hangup/error polling, and original terminal attributes are restored where the operating system still permits it.

## Parts, fabrication and assembly

The carrier BOM consists of J1 and J3–J6 two-pin 2.54 mm headers, J2 a four-pin header, R1/R3/R5/R7 10 kΩ resistors, and R2/R4/R6/R8 1 kΩ resistors. Suggested resistor orderable examples are Yageo MFR-25FBF52-10K and MFR-25FBF52-1K, fitted to the axial DIN0207 7.62 mm pitch footprints. Header examples are Samtec TSW-102-07-G-S and TSW-104-07-G-S; verify the exact lead geometry before purchasing substitutes. The external NUCLEO-F401RE and four MDSR-7 contacts complete the electrical assembly, with magnets, cable and insulated mounting selected for the mock-up.

For planning only, allow roughly US$25–40 for the Nucleo, US$8–20 for four reeds, US$5–15 for low-volume carrier fabrication, and US$10–25 for connectors, resistors, wiring and mechanical consumables. These are budget allowances, not verified current quotations or stock claims. Confirm supplier availability and actual assembled fit before ordering. The reusable computer is not included.

Open `kicad/device.kicad_pro` in KiCad 9. The project includes locally authored connector/resistor symbols, actual library footprints, assigned pad nets, routed copper, silk labels and a closed board outline. `connectivity.json` records the expected reference, value, footprint and pin/net mapping. `tools/check_hardware.py` exports a fresh schematic XML netlist and compares it to PCB pads and that manifest in addition to running ERC and DRC. Passive-only ERC does not verify the external module's power source or the physical connector wiring.

Before power, inspect solder joints and check continuity from every contact header to its series resistor and from each SENSE to the correct J2 pin. Confirm no short between 3V3 and ground. Power only the Nucleo first, verify its output voltage, then attach the carrier while unpowered. Test one input at a time with a temporary voltage-free switch before installing reeds. This sequence catches wiring errors without bringing the demonstration bins into the electrical debugging process.

## Reproducible build, tests and commissioning

From this device directory, build with `make -C firmware all test`. This compiles and links real Cortex-M4 firmware using arm-none-eabi-gcc, emits ELF and binary images, and runs separate address/undefined-behaviour-sanitized host tests. `make -C firmware flash` is the explicit physical-programming command using OpenOCD; it was not run during source validation. Do not mistake host test execution for target programming.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_app.py' -v` for protocol, serial and real Tk event-loop tests. These tests feed records from the actual C encoder through Linux pseudo-terminals, exercise screen switching, CSV export, disconnect and replay, and inject malformed input. The simulated serial boundary is explicit: no physical USB adapter or contact fixture is involved. Regenerate CAD with `/usr/bin/python3 tools/generate_hardware.py`, then run `/usr/bin/python3 tools/check_hardware.py`. The system interpreter must have KiCad's pcbnew module and library data installed.

For physical commissioning, start with both scoops parked and both lids shut. Verify all four logical contacts read closed. Remove A while B stays shut: A starts should increase without a flag. Open B while A remains absent: A flagged should increase once and foreign duration should grow. Close and reopen B during the same excursion: the flag count must remain unchanged. Return A: its completed count increments and its durations stop. Repeat symmetrically for B and then perform both excursions together. Unplug one sensor at a time to learn the indistinguishable broken-wire states. Finally compare the observations with video or a separate reference timer before making timing-accuracy claims.

See VERIFICATION.md for the actual automated results. Remaining work is physical fabrication, assembled electrical measurements, magnetic mounting validation, timing calibration, usability trials, ESD assessment and any application-specific safety review. None of those is replaced by source-level tests. This revision is a complete source design package for a bounded prototype experiment, not a qualified food-handling instrument.

## References

- ST UM1724, Nucleo-64 MB1136 user manual, table 16 and section 7.10: https://www.st.com/resource/en/user_manual/dm00105823-nucleo-f401re-user-manual-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual, GPIO/RCC/USART/SysTick-related programming reference: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Cambro Ingredient Bin Scoop Holder, adjacent passive storage product: https://www.cambro.com/Products/food-storage/mobile-ingredient-bins/ingredient-bin-scoop-holder/
- Manitowoc Multi-Mount External Scoop Holder, adjacent passive storage product: https://www.manitowocice.com/product/fam_faqvsz/Multi-Mount-External-Scoop-Holder
