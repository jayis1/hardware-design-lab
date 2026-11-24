# Insert Code — identity-retaining notebook for interchangeable jig inserts

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original source, documentation and locally authored electrical design files are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Board startup, clock, GPIO, UART, passive carrier generation, serial transport and the desktop shell are explicitly reused from jayis1's Quorum Comb at abf9a413a3201e072a8660884098b16a033f0d9d. The import script records that provenance and refuses to overwrite files. It is not a normal build step and does not recreate subsequent device-specific edits.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier for a separately purchased NUCLEO-F401RE, not a standalone microcontroller motherboard. The delivered software and CAD have reproducible checks; no physical fabrication, programming, electrical measurement, fixture evaluation, EMC testing or production readiness is claimed.

## Purpose, audience and practical benefit

Insert Code helps a small workshop compare the reliability of mechanically coded removable inserts in a manual jig. Each insert carries two raised code features selected from four positions. Four low-force switches read those features when the insert enters a guided dock. There are six supported identities. Instead of continually relabeling an insert whenever the switches change, the instrument captures the first qualified identity and retains it through the entire observed docking episode. A different valid code is recorded as a changed-code episode; a nonzero invalid code is recorded separately. A qualified empty dock is required before another identity can be admitted.

The intended user is a maker designing interchangeable stencil holders, assembly nests, embossing guides or educational tooling. Such inserts often look similar from above, and a printed label does not reveal whether the underside coding features remain readable after repeated handling. A useful bench experiment compares a broad flat actuator against a narrower actuator, or a square lead-in against a tapered guide. The question is not simply which insert is present now. It is whether one insertion remained consistently attributable to its first accepted code until removal.

A conventional live binary display can hide a troublesome sequence: code AB, then code AC, then AB again. At the final instant everything looks correct. Insert Code retains a substitution finding for that episode even after AB returns. Another insertion may first produce a one-bit word and later settle into AB. That episode stays unidentified rather than retroactively becoming a clean AB admission. These choices expose marginal mechanical engagement without claiming to diagnose its cause. The same evidence could result from bent tabs, switches, wiring, deliberate handling or an incorrectly coded insert.

The electronics never enable a machine, choose a torque recipe, release a lock or energize an actuator. There is no control output. Use the instrument on an unpowered bench fixture with dedicated, isolated contacts. Its role is to support a design discussion about code readability. It is not an anti-counterfeit reader, tool-presence safety system, proof of correct seating, or proof that the object carrying a code has the expected geometry.

## Originality and comparison with existing designs

Repository directories and device titles were inventoried after a fast-forward-only pull. The closest purpose documents were read. Parity Sled records even/odd parity excursions on a magnetic strip, including recovery into the same or another word. It has no required empty-dock admission boundary and considers zero an even code. Pattern Sled learns sixteen clocked words and compares later strip repeats. Nest Settle measures four-corner seating, while Label Veil compares a human binary answer with a concealed cassette class. Insert Code instead uses constant-weight identities, first-admission capture, and per-episode substitution and invalid-code findings in a removable tooling dock.

Physical tool coding is established prior art. A bounded search found US4530148, describing coded tools and code readers, and Liebherr's tool-identification documentation describing attached code carriers. Those are alternatives and evidence against claiming that tool identification itself is new. This proposal's contribution is its specific low-cost, local docking notebook and conservative first-code retention contract, not a new coding theory or patent-clearance claim. Worldwide uniqueness is unverified.

A barcode or RFID reader is better when many identities or stored metadata are needed. A camera can inspect both code and physical shape, but adds lighting and occlusion concerns. A logic analyzer is better for contact bounce. This fixture is useful only if four transparent contact channels and explicit episode accounting make mechanical iteration easier than those alternatives. That practical value remains a design hypothesis until a workshop pilot is performed.

Prior-art references:

- https://patents.google.com/patent/US4530148A/en
- https://www-assets.liebherr.com/media/bu-media/lhbu-lvt/broschures/gear-cutting-machines/liebherr-werkzeug-identifikationssysteme-en.pdf
- ST Nucleo connector reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 register reference: RM0368. This revision inherits the repository's reviewed MB1136 wiring contract; a fresh manufacturer PDF download failed during this run. Recheck the actual purchased board revision before wiring.

## Hardware specifications and block diagram

| Function | Revision A choice |
| --- | --- |
| MCU | STM32F401RET6, Cortex-M4, on external NUCLEO-F401RE MB1136 |
| Memory | 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz, explicitly restored at startup |
| Sensing | Four independent dry-contact channels, low-active electrical input |
| Suggested switches | Four Omron D2F-01L low-level lever switches, externally mounted |
| Qualification | Whole four-bit word; five unchanged intervals, six observations |
| Sample rate | Nominal 1 kHz, foreground sampling with missed-tick detection |
| Connectivity | USART2 TX PA2 AF7, 115200 baud, 8N1, ST-LINK USB virtual COM |
| Power | USB into Nucleo; regulated 3.3 V and common ground to passive carrier |
| Custom PCB | 80 by 70 mm, two copper layers, 1.6 mm FR-4 |
| Companion | Python 3 / Tk desktop on Linux, standard library only |
| Radio / actuator / battery | None |

    Six removable inserts, each with two of four code features
                         |
               Four lever switches A B C D
                         |
                 J3 J4 J5 J6 contact headers
                         |
           1k series and 10k pullup on every channel
                    passive carrier
                         |
                   J2 SENSE0..3
                         |
          PA0 / PA1 / PA4 / PB0 on NUCLEO-F401RE
                         |
           whole-word qualification, empty baseline
                         |
             first-identity docking episode engine
                         |
              IC1 cumulative status + CRC-16
                         |
            USART2 --> ST-LINK VCP --> USB host
                         |
            Insert identities / acquisition history

The Nucleo supplies processor decoupling, regulator, reset, debugger and USB circuitry. They are not represented as imaginary populated parts on the carrier. The carrier contains fourteen real placed components, labeled connectors and routed copper. The schematic uses locally authored passive connector and resistor symbols; the PCB uses library footprints with explicit full library identities. A fresh schematic netlist is compared against actual PCB pads and the connectivity manifest, including reference, pin, net, value and footprint.

## Wiring contract, BOM and power

J1 pin 1 connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect to CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4, A3/PB0. J3 through J6 pin 1 are CODE_A through CODE_D. Every two-pin contact connector has ground on pin 2. Logical bit weights are A=1, B=2, C=4, D=8. The displayed binary word is DCBA. Closed switches are logical ones after firmware inversion.

The ST-LINK virtual-COM path uses the Nucleo's default USART2 arrangement, normally SB13/SB14 fitted and SB62/SB63 open. Check board revision and actual solder bridges. This firmware is specific to the STM32F401, its reset clock and this pin map. An Arduino-compatible connector shape on a different board does not establish electrical or firmware compatibility.

| Reference | Quantity | Procurement specification |
| --- | --- | --- |
| R1 R3 R5 R7 | 4 | Yageo MFR-25FBF52-10K, 10 kohm, 1%, axial |
| R2 R4 R6 R8 | 4 | Yageo MFR-25FBF52-1K, 1 kohm, 1%, axial |
| J1 J3 J4 J5 J6 | 5 | Samtec TSW-102-07-G-S, 2-pin 2.54 mm |
| J2 | 1 | Samtec TSW-104-07-G-S, 4-pin 2.54 mm |
| External controller | 1 | ST NUCLEO-F401RE |
| External contacts | 4 | Omron D2F-01L; verify actual actuation and low-current suitability |
| Fixture | 1 set | Nonconductive base, six coded inserts, guides, stops, harness, enclosure |

A closed contact connects 3.3 V through 10k and 1k to ground. Ideal current is 0.300 mA and the MCU-side low level is 0.300 V. All four closed channels draw 1.200 mA; the two resistor networks of a valid two-bit code draw 0.600 mA. These are nominal calculations, not measurements or a total system power estimate. Reserve a protected USB supply appropriate to the Nucleo, for example a 5 V source capable of 500 mA, and measure the assembled system before final enclosure design.

There is no galvanic isolation, TVS array, fuse or reverse-polarity protection on the carrier. Series resistance does not make a powered external signal safe. Connect only dedicated voltage-free contacts disconnected from other equipment. Start with paired signal/ground cables shorter than half a meter and provide strain relief. Never connect mains, PLC outputs or an existing machine's switch circuit. A broken wire can look like an open switch; two wiring faults can map one valid two-bit code into another. No code validates its own wiring.

## Mechanical design and code assignment

Use a nonconductive base approximately 140 by 100 mm, with a guided insert pocket approximately 70 by 45 mm. Place four switch followers along a line at 12 mm centers. An initial code-feature design uses four possible 6 mm-wide actuator lands, two present and two recessed. Provide a keyed side rail so an insert cannot be accidentally reversed. Hard stops should take insertion force; switch bodies and levers should not serve as structural stops. These are starting fixture dimensions, not validated tolerances or supplied production mechanical CAD.

Number the inserts conspicuously and maintain this assignment: 1=AB=3, 2=AC=5, 3=BC=6, 4=AD=9, 5=BD=10, 6=CD=12. All-zero means observed empty. Other nonzero words are invalid, including all-four. A single bit error from a valid code cannot form another valid code. Two bit changes can, which is why retained identity comparisons matter. This is error detection with limited redundancy, not authentication.

Arrange the guide so both intended switches operate near the final insertion position. If one switch remains alone for six observations during a slow insertion, the firmware correctly starts an unidentified episode. It does not wait indefinitely for a preferred answer. That may be too strict for a particular fixture: adjust the mechanics or revise the explicitly tested protocol rather than concealing the intermediate word. A fifth independent seated sensor would allow a different admission contract, but it is not part of revision A.

Use insulating edge supports for the carrier, which has no mounting holes. A separate enclosure allowance of about 160 by 110 by 45 mm accommodates the carrier and Nucleo beside the dock. Check actual board, USB connector and harness clearances. Keep fingers out of lever pinch points. Captive rounded tabs are preferable to loose sharp pins. No environmental or impact rating is claimed.

## Firmware behavior and design decisions

At boot the engine waits for six consecutive zero observations. Attaching while an insert is present therefore does not invent a new admission. The whole word is qualified together: a change in any bit restarts the five-interval settling counter. Independent per-bit debounce could synthesize a code whose bits never coexisted, so it is deliberately not used here. The last qualified word remains visible during a new candidate's qualification, and the raw word shows the distinction.

Once READY, the first qualified nonzero word starts an episode. A valid code increments its identity's seat count and is retained. An invalid code increments unidentified and invalid, retaining identity zero for the entire episode. During ACTIVE, another valid code distinct from the original sets changed_here and increments changed once. A qualified nonzero invalid word sets invalid_here and increments invalid once. Both findings may belong to one episode, so their totals must not be added as a count of distinct bad episodes. Returning to the original word clears neither finding.

A qualified zero ends the episode and returns to READY. Completion means observed removal, not clean or correct use. Active age starts at admission and advances by acquired milliseconds, including qualification on removal. Latest age freezes at the most recent completion. A five-observation empty glitch is insufficient to end an episode. Very short disturbances below qualification are not classified; the instrument is not an electrical glitch recorder.

Any missing sample invalidates current correlation. An active episode is abandoned once, current identity and age are cleared, and a fresh empty baseline is required. Previously accumulated counters remain. Startup, gap handling and terminal faults all preserve starts = completed + abandoned + active. Identified admissions plus unidentified admissions equal starts. Counter updates are staged in a copied engine structure, checked for overflow and committed together. On exhaustion the original state enters HALT, abandoning only its prior active episode. No wraparound silently creates new evidence.

Timestamp arithmetic permits ordinary 32-bit wrap but rejects zero or backward half-range steps. Terminal status freezes, including its timestamp. GPIO configuration is checked; clock and watchdog initialization failures stop acquisition. UART transmission is nonblocking with a bounded frame buffer and timeout. There is no dynamic allocation, receive command interpreter, filesystem, flash log or wireless stack. Hardware watchdog recovery loses volatile history; the operator must export and deliberately begin a new session. Brownout behavior, actual oscillator drift and watchdog timing need bench verification.

## Application and protocol

Run `python3 insert-code/app/main.py` from the repository root. The Insert identities tab shows six cumulative seat counts, current retained identity, raw and qualified words, episode age, completion totals, taint counts, missed samples and fault status. Acquisition history retains the latest 2000 snapshots. Connect opens a Linux serial path at 115200 baud; Disconnect preserves evidence but labels it historical. Replay replaces a disconnected session only after a bounded regular-file recording passes framing and semantic checks. Failed connection or replay leaves earlier evidence intact.

IC1 is a receive-only ASCII protocol. Each record begins `IC1,`, contains unsigned decimal fields in the exact order in app/model.py FIELDS, then an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE covers everything before the asterisk, initial value 0xFFFF and polynomial 0x1021. Records contain time, acquisition diagnostics, raw/qualified status, phase and identity, active taints, cumulative episode counts, ages and six seat counts. Firmware normally emits every 100 ms. CRC detects transmission corruption, not hostile forgery.

The decoder bounds chunks and frames, resynchronizes on the full IC1 marker, rejects incompatible schemas and checks semantic invariants. Session checks reject counter rollback, contradictory duplicates, changed terminal evidence, unbacked identity migration and missing taints. Sparse snapshots are necessary bounds, not reconstruction of every intervening physical event. A live label requires advancing acquisition, recent host progress, an open connection and bounded cumulative host/device clock divergence. Long captures may conservatively require reconnect because the HSI clock is not calibrated.

Export creates a private, exclusive CSV and refuses overwrite. A metadata row records jayis1, source, connection, freshness, parser errors and session error even when there are no completed episodes. No network service receives the data. There is no persistent boot identifier: some resets with plausible forward timestamps and counters can be indistinguishable. A user who knows the board reset must start a new session rather than trusting automatic inference.

## Reproducible verification and next bench work

From the repository root run `make -C insert-code/firmware clean all test fixture`, then `python3 insert-code/tests/test_app.py`, then `xvfb-run -a /usr/bin/python3 insert-code/tests/test_gui.py`. Target compilation needs GNU Arm Embedded GCC; host tests need GCC with address and undefined-behavior sanitizers. The Tk GUI tests use actual event loops and Linux pseudoterminals, not physical USB. The fixture file is synthetic output generated by the production C encoder and is not a measured experiment.

Run `/usr/bin/python3 insert-code/tools/generate_hardware.py` with KiCad 9 pcbnew and installed footprint libraries to regenerate native design files. Run `/usr/bin/python3 insert-code/tools/check_hardware.py` for ERC, DRC and exact schematic/PCB connectivity comparison. The generator overwrites its generated CAD targets, so preserve deliberate hand edits before regeneration. Build products, caches and local KiCad preferences are ignored. See VALIDATION.md for executed results and remaining limitations.

Before building, review the schematic and connector orientation with the actual purchased Nucleo manual. Assemble without power, check every resistance and continuity path, then power from a current-limited USB arrangement. Verify open and closed voltage levels at every MCU input, inspect edges with an oscilloscope, and test all six code inserts plus missing and stuck switches. Compare notebook observations with synchronized video while varying insertion speed. Determine whether the six-observation admission rule is practical before proposing any deployment. Repeat tests after changing follower geometry, and never use this observer as a substitute for a protective interlock.
