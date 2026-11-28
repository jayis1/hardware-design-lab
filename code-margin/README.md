# Code Margin — empirical contact-code separation workbench

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype. Target compilation, synthetic acquisition, desktop operation and electrical CAD checks are reproducible. No board has been fabricated, physical target programmed, contact fixture calibrated or real workshop trial performed. This is not a safety interlock, certified identifier or manufacturing release.

## Purpose and practical value

Code Margin helps a maker examine the error margin of a small mechanical coding scheme before embedding that scheme in a product. Four independent voltage-free contacts read four raised tabs, magnetically operated reeds or position witnesses. The instrument keeps every sufficiently stable four-bit word it encounters. It computes the minimum Hamming distance between distinct retained words, shows an actual closest pair, and lists all hypothetical readings that could have come from more than one retained word after at most one changed contact. The companion's reading inspector makes the ambiguity tangible without silently correcting a measurement.

Imagine interchangeable educational tiles whose underside tabs operate four switches. Two tiles might look sufficiently different on paper while producing electrical words that differ at only one switch. A dirty contact on one tile can then produce the exact retained code of another. A simple binary display shows both words but does not explain the consequence. Code Margin preserves that pair and shows which channels distinguish it. If the designer changes the tab arrangement, a separate session can reveal whether the new observed words are better separated. The tool does not establish which physical tile owns a word; an experimenter must keep that correspondence separately.

A second use is a low-energy mechanical selector demonstrator. A builder may intend only two resting states, all-open and all-closed, yet a repeatable intermediate pause adds another stable word. Rather than excluding that inconvenient observation, this instrument retains it and recomputes the reduced separation. The result invites inspection of the linkage, witness placement or experimental protocol. A stable transient is not automatically an intended code, and the notebook does not pretend to know the designer's intent. Its conservative retained set is evidence about observed electrical words, not an approved list of identities.

Target users include makers of removable teaching inserts, museum demonstrator builders, mechanical-computing educators and workshop engineers prototyping disconnected position coding. They can compare contact placement, tab patterns or assembly conditions without a cloud account. A useful session begins with documented contact meanings, a manually checked fixture and a list of intended physical objects. Practical benefit remains a hypothesis to evaluate with users. The proposed contribution is making small code-space vulnerabilities visible while collecting the physical observations that motivated them.

## Originality, alternatives and boundaries

Before selecting this concept, the configured main branch was pulled, device directories were listed and README titles were inventoried. The closest designs were inspected. Insert Code uses a fixed constant-weight code and retains the first identity through a docking episode. Mask Key evaluates which of three controls can explain a separately observed binary response. Step Islands constructs connectivity through single-bit graph edges. Code Margin instead treats all four channels equally and asks how closely distinct observed words lie and where their radius-one error neighborhoods overlap. It has no response channel, docking identity latch or transition-path claim.

Coding theory and coded position sensors are established prior art. A bounded search found “Error-detecting unit-distance code,” DOI https://doi.org/10.1109/19.58616, and “On Hamming distance of partial M-sequence with application to absolute encoder,” DOI https://doi.org/10.1109/iecon.1991.239090. These results directly limit novelty claims: neither Hamming distance nor error-aware mechanical encoding is new. This repository contribution is the particular physical exploration workflow, retained-evidence contract and local ambiguity interface. Worldwide uniqueness, patentability and freedom to operate have not been established.

A paper worksheet is sufficient for a few predetermined codes. A general-purpose logic analyzer and script can perform similar analysis at much higher speeds. RFID and barcodes offer many more identities and metadata. Video records physical context that four contacts cannot. Code Margin is appropriate only when transparent, slow, four-channel observations make an early mechanical experiment easier. Its reused carrier is not claimed as a new circuit topology. No application should replace independent safety sensing or authenticated identification with this instrument.

## Hardware specifications

The controller is a separately purchased ST NUCLEO-F401RE, MB1136 family, with an STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware establishes the nominal 16 MHz internal HSI clock and observes one-millisecond SysTick increments. USART2 sends telemetry at 115200 baud, eight data bits, no parity and one stop bit. The ST-LINK USB virtual COM bridge supplies the computer connection. There is no radio, battery, charging circuit, cloud service, actuator, command receiver or nonvolatile experiment storage.

The custom PCB is a cable-connected passive interface, not a standalone microcontroller board or mechanically mating Arduino shield. Its closed outline is 80 by 70 mm, with two copper layers and nominal 1.6 mm FR-4 thickness. Six through-hole headers and eight axial resistors provide four independently pulled-up contact channels. The Nucleo supplies voltage regulation, processor decoupling, reset, debugging and USB bridging. Those required functions belong to the purchased board rather than being omitted from a purported complete MCU circuit.

Each sense node has a 10 kilohm pull-up to regulated 3.3 V and a 1 kilohm series path to its external contact. The other contact terminal connects to shared ground. Closed contacts therefore read electrical low and logical one. A low-level lever switch such as Omron D2F-01L is a candidate, subject to mounting and low-current verification. Reed contacts are another possibility but require magnetic threshold and hysteresis characterization. This package does not claim a qualified switch lot, a completed mechanical fixture or a production-ready enclosure.

## Architecture and wiring

    Four tab / position witnesses A, B, C, D
                         |
                    J3 through J6
                         |
              1k series / 10k pull-up
                         |
                     J2 GPIO harness
                         |
             External NUCLEO-F401RE
                         |
          100 ms whole-word qualification
                         |
            Retained sixteen-word bitset
                         |
          Minimum distance + closest pair
            Radius-one ambiguity analysis
                         |
              CM1 + CRC / USART2 TX
                         |
                 ST-LINK USB VCP
                         |
       Linux Tk app: code table / reading inspector
              acquisition / replay / CSV

J1 pin one connects to Nucleo CN6 pin four, regulated 3V3; J1 pin two connects to CN6 pin six, ground. J2 pins one through four connect respectively to CN8 pins one through four: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. The analog-labelled connector positions are configured as digital inputs. J3, J4, J5 and J6 are POSITION_A, POSITION_B, POSITION_C and POSITION_D. Every contact connector uses pin one for signal and pin two for ground. Check this harness independently; connector pitch does not prevent reversal.

The numeric word is A + 2*B + 4*C + 8*D. The app prints A B C D in that named order rather than conventional most-significant-bit-first notation. Thus word three is printed as 1 1 0 0. A retained-set bit at position w means word w was observed. There are sixteen possible words, not sixteen independently instrumented objects. Two physical objects that produce the same word collapse into one set member and are not distinguishable by this system.

PA2 uses alternate function AF7 for USART2 transmit. Inspect the purchased board revision and ST-LINK virtual-COM solder bridges, including SB13/SB14 and SB62/SB63, before use. ST UM1724 provides the connector tables; RM0368 describes the STM32F401 register map, and PM0214 describes Cortex-M4 support. A copy of UM1724 was downloaded during verification from https://embedded.fel.cvut.cz/sites/default/files/procesory/STM32/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf. The manufacturer's document is https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf. Neither document substitutes for inspecting the actual assembly.

## BOM, power and physical limits

R1, R3, R5 and R7 are 10 kilohm one-percent axial pull-ups. R2, R4, R6 and R8 are 1 kilohm one-percent axial series resistors. The DIN0207 resistor footprints have 7.62 mm lead spacing. J1 and J3 through J6 use two-position 2.54 mm vertical headers; J2 uses a four-position header. The connectivity manifest records all on-carrier references, values, footprints and pin nets. Add the development board, its USB cable, four suitable contacts, insulated wiring, cable strain relief and a nonconductive enclosure.

At nominal values, a closed channel draws 3.3 V divided by 11 kilohms, or 0.30 mA, and its sense voltage is approximately 0.30 V. Four closed channels draw 1.20 mA from the regulated rail. These are calculations, not measurements. Development-board and debugger consumption dominate the total USB budget and must be measured with the actual unit. The low excitation current may be insufficient to penetrate oxide on contacts designed for larger wetting currents. An apparent open bit can be a contact-quality problem rather than a different mechanical code.

Mount witnesses on adjustable insulating supports and keep tabs or magnets captive. Contact force must not substantially change the mechanism under study. Keep wiring short, label both ends and photograph the physical code arrangement for each session. The PCB has no mounting holes; use insulating edge supports rather than drilling through reviewed copper. Provide safe access to reset and USB. Mechanical tolerances, switch overtravel, magnet spacing and enclosure fit remain application-specific commissioning work, not verified dimensions in this release.

Only detached voltage-free switches may connect to the carrier. It has no galvanic isolation, reverse-polarity protection, surge suppressors or adequate external-voltage protection. USB ground is common to all contacts. Never attach powered logic, industrial wiring, vehicle circuits, mains or live machine interlocks. Wire with power removed, check rail resistance and connector orientation, and apply only the Nucleo's regulated 3.3 V. Open wiring resembles an open switch; shorted wiring resembles closure. Perform a manual channel check before every experiment.

## Firmware implementation and acquisition contract

Firmware is freestanding C11, built with GNU Arm Embedded tools and warnings treated as errors. startup.c provides vectors, reset initialization and memory support; linker.ld maps flash and SRAM. registers.h defines the used MMIO locations. board.h states the external connection contract. The drivers configure GPIO, clock, watchdog and a bounded nonblocking transmitter. main.c acquires newly observed ticks and schedules status transmission. There is no heap, RTOS or vendor SDK dependency.

The platform is explicitly reused from jayis1's Step Islands at commit 181a193a02a4996b943970ee9c643f40ece0d9ca. Acquisition, startup, serial framing, passive hardware generation and the desktop transport shell are shared infrastructure. Code Margin adds its own distance, witness and ambiguity analysis, protocol interpretation, reading inspector and exhaustive tests. tools/import_platform.py records provenance and refuses overwrites; it is not a normal build command and does not recreate subsequent edits. The electrical generator remains separately reproducible.

The first acquired sample establishes an endpoint without credit. A word qualifies after one hundred consecutive unchanged one-millisecond intervals: an initial endpoint plus one hundred subsequent samples. Qualification adds one visit and retains that word. Further holding adds qualified interval time, not repeated visits. A raw change restarts qualification, including a short glitch that never becomes a retained word. The qualification-completing interval remains unknown rather than being retroactively counted as settled dwell. Whole-word qualification prevents independently delayed bits from inventing a configuration never observed together.

Skipped ticks add gap and missed-sample diagnostics and restart qualification. Retained history survives, but absent intervals do not receive inferred exposure. Every acquired endpoint after initialization is classified as unknown, qualified or a gap endpoint. Updates are transactional so a counter overflow cannot publish half a visit. Terminal faults retain their first cause and freeze experimental evidence; timestamps and transmission diagnostics may continue. Fault codes are zero normal, one pin configuration, two watchdog initialization, three invalid raw input and four exhaustion. Reset erases volatile history.

## Distance, ambiguity and interpretation

For retained distinct words a and b, Hamming distance is the number of unequal contacts, implemented as population count of a XOR b. The minimum is taken over every distinct pair. The closest-pair witness uses numeric tie-breaking. Empty and singleton histories have no pair; wire value five means undefined, not five-bit protection. The UI prints undefined. A newly retained word can reduce the minimum distance, but repeated visits cannot improve it. Removing an inconvenient observation requires a new explicitly separate session, not editing the evidence in place.

For each hypothetical received word r, the candidate set contains every retained word whose distance from r is at most one. Zero candidates means outside the restricted radius-one model. One means a unique candidate under that model, not verified object identity. Two or more means ambiguity, including when r exactly equals a retained word. Exact membership must not override the fact that a neighboring code could have produced the same reading through a single error. The ambiguous mask has bit r set precisely for those multiple-candidate readings.

For example, the two retained words zero and fifteen have minimum distance four and disjoint radius-one neighborhoods. Reading one has only zero as a radius-one candidate; reading three has no candidate. Adding word one reduces the minimum to one, and readings zero and one become ambiguous. The firmware constructs each radius-one neighborhood from the reading and its four single-bit flips. The companion independently recomputes candidates by scanning retained words. Both implementations are checked across every possible retained set, not just a few selected codebooks.

In an ideal complete codebook, minimum distance at least two detects any single-bit substitution through membership checking, and minimum distance at least three separates single-error neighborhoods. These mathematical statements do not establish that the observed set is complete, that only one contact can fail, or that a physical object has the intended code. Correlated wiring faults, hidden objects and unobserved states break those assumptions. The instrument therefore never performs automatic correction, commands machinery or issues a safe-to-use verdict.

A generic unobserved-word hint remains available to help inspect coverage. It chooses the unobserved word nearest the current raw word, with numeric ties; sixteen means none remain. It is not a request to manufacture every code or optimize a codebook. Record a hinted word only if it represents a relevant freely achievable condition. Idle or intermediate words held long enough also enter the history. Keep that conservative evidence, annotate its physical meaning externally, and use separate sessions for changed fixtures. The desktop inspector is the nonmutating way to explore hypothetical errors.

## Protocol and companion application

CM1 telemetry contains eighteen unsigned decimal fields in this order: now, samples, gaps, missed, tx_errors, raw, hold, credited, seeded, fault, visits, settings, coverage, qualified_ms, unknown_ms, suggestion, distance and ambiguous. The body begins with CM1 and comma-separated values. An asterisk, four uppercase hexadecimal CRC digits and newline terminate the record. CRC-16/CCITT-FALSE uses initial value 65535 and polynomial 0x1021 over the body before the asterisk. CRC detects corruption but does not authenticate the sender.

settings is the retained sixteen-bit code set. coverage repeats that set for the common acquisition contract. distance is a value from one through four or the undefined sentinel five. ambiguous is a sixteen-bit reading mask, not a count of physical errors. The companion recomputes summaries, checks ranges and conservation, and enforces necessary acquisition transition bounds. Dense adjacent observations receive exact qualification and credit-attribution checks. Sparse validation cannot reconstruct every hidden sample or prove that a recording came from real hardware.

Run `python3 code-margin/app/main.py` on Linux with Tk installed. The Code margins tab shows every possible reading, whether it was observed, its radius-one status and all candidate codes. The Reading inspector accepts a hypothetical integer zero through fifteen and shows the candidate list without changing evidence. Acquisition / limits presents counters and caveats. Connect opens the selected serial port; Disconnect retains the session. A second connection is refused while a port is owned. Failed opens preserve prior evidence and restore terminal attributes where applicable.

Replay accepts only regular files up to two MiB and rejects empty recordings, malformed decimals, bad checksums, impossible summaries and partial tails. Live parsing can resynchronize on the complete CM1 marker after corruption. Freshness tracks acquisition progress separately from byte reception. A two-second progress pause or excessive cumulative host/device clock disagreement closes the link and requires reconnect. There is no persistent boot identifier, so some resets remain indistinguishable from forward traffic. Uncalibrated oscillator drift can conservatively end a long live session.

Export CSV exclusively creates a new file with private permissions, refusing overwrite. It includes author jayis1, provenance, connection state, warnings, parser diagnostics, progress age, total accepted rows and the latest two thousand snapshots. The retained set permits later reconstruction of every candidate neighborhood. Inspector queries are views, not acquired events. Save evidence before reset and keep physical object labels, contact placement and experimental conditions beside it. Synthetic fixtures remain synthetic even when accepted by the actual GUI.

## Build, verification and commissioning

From the repository root run `make -C code-margin/firmware clean all test fixture`. This builds an ARM ELF and binary, runs sanitized C tests and generates a synthetic CM1 recording. Run `xvfb-run -a python3 -m unittest discover -s code-margin/tests -p 'test_app.py' -v` for model, serial and desktop tests. The test suite covers every retained codebook and reading, minimum-distance witnesses, qualification boundaries, gaps, wrap, terminal faults, real C-encoded replay, malformed frames, live PTYs, reconnect, stale acquisition and private exports. No external Python package installation is required; the GUI needs system Tk and the headless tests need Xvfb.

Generate CAD using `/usr/bin/python3 code-margin/tools/generate_hardware.py` with KiCad 9 bindings and footprint libraries. Validate using `/usr/bin/python3 code-margin/tools/check_hardware.py`. The checker runs native ERC and DRC, exports a fresh schematic XML netlist, and compares every reference, value, footprint and pin net with PCB pads and the manifest. Passive-only ERC cannot validate the external Nucleo or harness. Build outputs and local KiCad preferences are ignored. Exact execution results are recorded in VALIDATION.md.

Before fabrication, review the electrical and mechanical design independently. After assembly, inspect soldering, check rail resistance, verify the harness and measure open and closed input voltages. Program through ST-LINK only after confirming target and power selection; the optional Makefile flash target uses OpenOCD. Exercise all sixteen raw words, introduce chatter and missed acquisitions, and measure the worst-case foreground latency during telemetry. Nominal one-millisecond scheduling is not a measured timing guarantee.

Finally, compare a small known set of physical inserts with synchronized manual notes. Verify that the retained words correspond to the intended contact configurations, that switch loading does not alter seating, and that different physical objects sharing a code remain visibly documented as indistinguishable. Deliberately open one contact to explore ambiguity without introducing powered faults. The supplied source package supports that evaluation; it does not claim that a fabricated unit or practical user trial has already passed it.
