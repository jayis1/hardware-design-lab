# Closure Basis — empirical implication notebook for detached mechanisms

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype. Target compilation, synthetic acquisition, desktop interaction and native CAD checks are reproducible; fabrication, programming a physical MCU, mechanical construction, electrical measurements and user benefit have not been physically verified. This is not a safety interlock or a certification instrument.

## Purpose, overview and practical value

Closure Basis helps a maker investigate which combinations of positions in a small detached mechanical assembly appear to require other positions. Four voltage-free contacts witness positions A, B, C and D. None is designated as the commanded input or the response. The instrument retains stable observed configurations and computes positive implications: whenever all positions in an antecedent are closed, which other contacts were closed in every matching observation? It shows distinct supporting configurations, provisional consequences, and a concrete retained counterexample for every refuted consequent.

Imagine a model enclosure with two sliding tabs, a hinged lid and a captive retaining bar. The builder suspects that closing both tabs requires the bar to be seated, but closing either tab separately might not. A simple pairwise correlation display misses this conjunction. A fixed truth-table tool forces the builder to decide beforehand which parts are inputs and which are outputs. Closure Basis treats all four positions symmetrically and inspects every positive antecedent, including combinations of two or three contacts. It can show that A+B has always accompanied D while A alone and B alone each have witnessed exceptions.

This is useful during exploratory design, not as an authorization signal. The screen cannot prove that an unseen configuration is impossible. A linkage can stick, a user can avoid a reachable position, a switch can be misadjusted, or a hidden mechanical condition can change. A rule can survive many observations and still fail tomorrow. The device deliberately preserves the observations that remove a rule; returning to an agreeable configuration cannot silently restore it within the same volatile session. Repeating a familiar configuration increases visits but does not inflate distinct support.

Intended users are educational mechanism builders, puzzle-box prototypers, museum workshop technicians and makers comparing low-energy mock-ups. A repairer could also document how four disconnected positional witnesses co-occur before and after an adjustment, provided each adjustment starts a separately identified experiment. The proposed benefit is reduced transcription and clearer counterexamples. No measured improvement in design speed, fault detection or usability is claimed.

## Differentiation and prior art

The repository was pulled on its configured main branch, device directories and README titles were inventoried, and the purposes of nearby devices were inspected. Mask Key asks which subsets of three selected controls can explain a separately designated binary output; its counterexample is a pair with the same selected inputs and opposite output. Truth Sieve retains a fixed two-input/two-output relation. Pair Cover measures pairwise configuration coverage rather than dependencies. Snap History records reversible chronological fastener changes. Closure Basis instead asks symmetric positive implication questions over four observed positions, including conjunctive antecedents, with one observed configuration as a counterexample.

Formal concept analysis and attribute implications are established mathematics. A bounded external search located TU Dresden's attribute-exploration teaching slides at https://iccl.inf.tu-dresden.de/w/images/3/3d/FCA2014-05.pdf, including intersection closures, Horn implications and counterexample-guided exploration. The search also returned a thesis applying formal concept analysis to special locking systems at https://digilib.k.utb.cz/handle/10563/43315. These are important related ideas, not evidence of a worldwide first. The proposed contribution is the integrated, physically witnessed bench notebook, its acquisition contract and explicit unsupported-antecedent handling. Worldwide novelty, patentability and freedom to operate remain unverified.

The product name does not mean that it computes a canonical mathematical stem basis. It lists inclusion-minimal antecedents separately for each singleton consequent. Such rules can still be transitively redundant across different consequents. No theorem of causal or mechanical dependence is inferred. A notebook and camera are reasonable alternatives; a general logic analyzer plus a script can also implement the analysis. A camera is better at preserving approach direction and uninstrumented motion, while this fixture offers direct, bounded configuration retention without manual transcription.

## Hardware specifications and system boundary

The processor assembly is a separately purchased ST NUCLEO-F401RE MB1136 containing an STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. The firmware restores the nominal internal 16 MHz HSI clock and samples each newly observed one-millisecond tick. USART2 transmits at 115200 baud, eight data bits, no parity and one stop bit through the board's ST-LINK USB virtual COM connection. The host receives telemetry only. There is no firmware command receiver, radio, cloud account, motor output, relay, battery or charger.

The custom PCB is a passive cable-connected interface, not a standalone MCU motherboard and not a mechanically mating Arduino shield. Its outline is 80 by 70 mm, with two copper layers and nominal 1.6 mm FR-4. It contains six through-hole headers, eight axial resistors, routed tracks, one ground via and silkscreen legends. The external Nucleo supplies regulation, decoupling, reset, debugging, processor support and USB conversion. Its purchased circuitry is not duplicated in the carrier schematic, and the carrier must never be described as operating independently.

Each contact channel has a 10 kilohm pull-up from its sense node to regulated 3.3 V, followed by a 1 kilohm series resistor between the sense node and the external contact signal. The external switch connects signal to shared ground when closed. Suggested witnesses are low-level lever switches or reed contacts selected for the actual fixture and excitation current. An Omron D2F-01L is a candidate low-level lever switch, not a validated sourcing or mechanical-fit recommendation. No live stock, pricing or lifetime guarantee was checked.

## Architecture and pin assignments

    Detached position witnesses A / B / C / D
                       |
                 J3 / J4 / J5 / J6
                       |
          Four 10k pull-ups and 1k series resistors
                       |
                  J2 sense nodes
                       |
       External NUCLEO-F401RE digital GPIO acquisition
                       |
        Whole-word 100 ms qualification and gap accounting
                       |
              Retained 16-configuration set
                       |
       Supported antecedents / intersection consequences
                       |
          Minimal rules and retained counterexamples
                       |
             CB1 telemetry with CRC over USART2
                       |
                ST-LINK USB virtual COM
                       |
           Linux Tk screens / strict replay / private CSV

J1 pin 1 connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled header positions are used as digital inputs. J3 carries POSITION_A, J4 POSITION_B, J5 POSITION_C and J6 POSITION_D. Every external contact connector has signal on pin 1 and ground on pin 2. None is a driven actuator connection.

Electrical low means closed, so the GPIO driver inverts the sampled levels. The logical word is A + 2*B + 4*C + 8*D. Numeric word three therefore means A and B closed, C and D open. Printed A B C D suggestions list named channels in that order, not most-significant-bit-first binary notation. Antecedent five means A+C must both be closed, while the other channels are unrestricted. A raw configuration with additional closed positions still matches that antecedent.

PA2 uses USART2 transmit alternate function AF7. Before wiring, verify the purchased MB1136 revision against ST UM1724, especially the virtual-COM solder bridges SB13/SB14 and SB62/SB63. Peripheral definitions follow the inherited STM32F401 RM0368 register implementation; exception and SysTick behavior follow the Cortex-M4 platform. Useful manufacturer reference: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf. This revision reuses previously documented platform choices and does not claim a fresh physical pin audit.

## BOM, power budget and mechanical assembly

R1, R3, R5 and R7 are 10 kilohm one-percent axial pull-ups. R2, R4, R6 and R8 are 1 kilohm one-percent axial series resistors. Their DIN0207 footprints have 7.62 mm lead spacing. J1 and J3 through J6 are two-position 2.54 mm vertical headers, and J2 is a four-position header at the same pitch. Add the development board, a compatible USB cable, four low-level contacts, short insulated leads, strain relief and an insulating enclosure. The connectivity manifest lists every on-carrier component, footprint and pin net.

Nominal closed-channel current is 3.3 V divided by 11 kilohms, or 0.30 mA. Four closed channels require approximately 1.20 mA and 3.96 mW from the regulated rail. The corresponding sense voltage is approximately 0.30 V. These are circuit calculations, not measured values. Development-board consumption dominates total USB power and must be measured on the assembled system. Contact wetting at this low current may be unreliable with oxidized switches intended for higher loads. A falsely open contact can be an electrical contact problem rather than a mechanical counterexample.

A practical fixture uses four individually adjustable insulated witness brackets on a captive, hand-operated model. Position each switch so it observes a named endpoint without materially changing the mechanism's spring force or friction. Use a removable label for the physical meaning assigned to A through D and photograph the setup before collecting evidence. No universal bracket is supplied: geometry depends on the demonstrator. The carrier has no mounting holes; use insulating edge supports rather than drilling through unreviewed copper. Leave access to USB and reset, and keep the PCB away from conductive debris.

All contacts must be disconnected from every other electrical circuit. The board shares USB ground and provides no isolation, reverse-polarity protection or adequate external-voltage protection. The series resistors are not a safety barrier. Never attach mains, a powered logic output, a battery-backed sensor, an operating machine interlock or a live access-control circuit. Wire with power removed and inspect resistance, connector orientation and supply polarity before applying only regulated Nucleo 3.3 V. Use a dry indoor low-energy bench environment. There is no battery charging safety subsystem because no battery is used.

## Firmware acquisition and analysis

The firmware is freestanding C11 with warnings treated as errors. startup.c provides the reset vector, data copying and BSS initialization. linker.ld maps the selected flash and SRAM and reserves stack headroom. board.h defines the hardware contract; registers.h defines the peripheral addresses used by the clock, GPIO, serial and watchdog drivers. main.c initializes the clock and pins, verifies their configuration, starts the independent watchdog, acquires new ticks and submits periodic bounded telemetry. Shared infrastructure is explicitly imported from jayis1's Mask Key at commit b8b5b3dc1ce9b80028fa46ff7bc62e206d49090b; analysis, interpretation, tests and documentation are specific to Closure Basis.

The first sample establishes a raw endpoint and earns no observation credit. A word must remain unchanged for one hundred consecutive one-millisecond intervals to qualify. Thus qualification requires one initial endpoint and one hundred later observations. The qualification endpoint adds one visit and sets the corresponding history bit; its preceding interval remains unknown, not retroactively settled dwell. Continued holding earns qualified intervals without repeated visits. Any raw disturbance resets the window, including a brief glitch that never becomes a qualified word.

When acquisition skips ticks, the engine increments gap and missed-tick counters, discards current qualification and requires a complete new window. Historical configurations remain. Every post-seed acquired endpoint is accounted as an unknown interval, a qualified interval or a gap endpoint. Updates are transactional so counter exhaustion cannot commit half a visit. A terminal fault freezes acquisition-derived evidence, while the timestamp and transmission-error diagnostic can continue. Evidence is volatile; reset begins an empty device history, and no flash persistence or authenticated session identity is implemented.

For an antecedent mask a, analysis collects every retained word w containing all bits of a. Support is the number of distinct matching configurations, not the number of visits, elapsed dwell, probability or statistical confidence. If support is nonzero, bitwise intersection of matching words gives its observed closure. Removing the antecedent bits leaves the extra positive consequences. If support is zero, the public result is sentinel sixteen and the screen says UNOBSERVED. The mathematical convention of assigning all attributes to an empty intersection is deliberately not presented as empirical evidence.

For each prospective consequent contact, the firmware also finds the numerically first retained matching word in which that contact was open. This is an explicit counterexample, preserved by the monotonic configuration set. For each singleton consequent, minimal-rule analysis excludes tautologies and removes antecedents having a supported proper subset that already implies that consequent. Empty antecedents are permitted: if every observed configuration contains B, the empirical rule empty -> B appears provisionally. Such a rule can merely reflect incomplete exploration or a stuck switch.

The suggestion chooses an unobserved full configuration with minimum Hamming distance from the current raw word, breaking ties numerically. It is an investigation prompt, never a motion instruction or proof of reachability. Do not force a linkage to satisfy it. There is no button to declare an unobserved configuration impossible, and the suggestion may remain on a genuinely unreachable setting. If all sixteen configurations are observed, no nontrivial positive implication remains. Negative literals, ordering, elapsed-time dependencies and hidden state are outside this revision's analysis.

## Wire protocol and desktop application

CB1 frames contain eighteen unsigned decimal fields in this exact order: now, samples, gaps, missed, tx_errors, raw, hold, credited, seeded, fault, visits, settings, coverage, qualified_ms, unknown_ms, suggestion, supported and rules. The body begins with CB1 and comma-separated fields, then an asterisk, four uppercase hexadecimal CRC digits and newline. CRC-16/CCITT-FALSE uses initial value 65535 and polynomial 0x1021 over the complete body before the asterisk. The conventional check string 123456789 produces 0x29B1. CRC detects corruption but does not authenticate data or prevent malicious fabrication.

settings is a sixteen-bit set with bit w indicating an observed word. coverage redundantly carries the same set in this protocol revision, making inherited acquisition validation explicit. supported is a sixteen-bit set of antecedents having nonzero support. rules counts inclusion-minimal nontrivial singleton-consequent rules. suggestion ranges from zero through fifteen, or sentinel sixteen when all settings were seen. Fault values are zero normal, one invalid board configuration, two watchdog setup failure, three invalid input and four counter exhaustion. The app recomputes summaries rather than trusting transmitted conclusions.

Run `python3 closure-basis/app/main.py` on Linux with Tk installed. No package download, account or network connection is required. Enter the serial path, normally /dev/ttyACM0, and select Connect. The Implications / counterexamples screen contains all sixteen antecedents, their extra closure bits, explicit refuted-contact/raw-word witnesses and distinct support. The Acquisition / limits screen shows all protocol fields and the individual minimal rules. Disconnect preserves evidence; reconnect starts a separate host session. Failed port setup does not erase the previous session, and opening a second connection is refused.

Replay accepts only regular files no larger than two MiB and is available while disconnected. It rejects bad checksums, malformed values, unsupported summaries, inconsistent counters, empty recordings and partial trailing frames. Live parsing resynchronizes at a complete marker after fragments. The app watches sample progress separately from reception, stops on stale acquisition, and rejects excessive cumulative host/device time discrepancy. Sparse snapshot checks are necessary consistency bounds, not reconstruction of every unreported observation. Without persistent boot identity, some resets can resemble valid forward traffic; reconnect explicitly after any reset.

CSV export uses exclusive file creation, private permissions and no overwrite. It includes author jayis1, live or replay provenance, connection state, warnings, parser diagnostics, progress age and the latest two thousand accepted snapshots. Total accepted count is retained separately. Exported raw settings allow every rule and counterexample to be recomputed later. A replay of a synthetic fixture must remain labelled synthetic; neither fresh serial traffic nor a valid checksum proves that a real mechanism was measured.

## Build, verification and first experiment

From the repository root run `make -C closure-basis/firmware clean all test fixture`. This links Cortex-M4 target firmware with GNU Arm Embedded tools, emits a binary, executes sanitizer-enabled host tests, and generates a synthetic CB1 recording using the real encoder. The exhaustive C test covers all possible retained configuration sets and every antecedent/consequent against an independently structured oracle. Acquisition tests cover every ordered word pair, the qualification boundary, raw disturbance, skipped acquisition, timestamp wrap, terminal fault behavior and bounded serialization. Separate tests exercise the actual clock and UART driver implementations with simulated registers.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s closure-basis/tests -p 'test_*.py'` for independent Python analysis, C-fixture decoding, strict replay, serial pseudo-terminal behavior and real Tk event-loop checks. The fixture first records configurations 0, 3, 7, 11 and 15, supporting A -> B, then adds word 1 and retains the counterexample that removes it. These are synthetic configurations, not measured device output. GUI tests inspect displayed evidence, permissions, overwrite refusal, disconnection, stale acquisition and failed-open preservation.

Run `/usr/bin/python3 closure-basis/tools/check_hardware.py` for native KiCad ERC and DRC and a newly exported schematic netlist comparison against every PCB pad, value, footprint identifier and manifest net. The generator uses KiCad 9 distribution Python bindings and can regenerate the authored carrier. Do not rerun the import script over maintained files; it refuses existing destinations by design. See VALIDATION.md for actual results and limitations, rather than interpreting the existence of these commands as proof they passed.

Before a first physical trial, inspect soldering and connector orientation, measure open and closed input voltages, continuity-check every channel and verify that only the intended bit changes. Use a disconnected four-switch panel to reproduce the synthetic example, then collect all sixteen configurations to confirm removal of nontrivial positive rules. Test reset, USB removal and deliberately noisy contacts. Only then attach a safely captive mechanism. Record the fixture mapping, witness adjustment and physical limits; explore from multiple directions and repeat questionable cases. Programming, electrical bring-up, calibration, EMC testing, enclosure fit and useful real-world performance remain future physical validation, not claims made by the software tests.
