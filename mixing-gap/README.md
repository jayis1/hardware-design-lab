# Mixing Gap — mechanical recombination exploration notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original project software, documentation and authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype, not a certified instrument. Target linking, synthetic acquisition tests, application operation and CAD checks are recorded in VALIDATION.md. Fabrication, physical target programming, electrical measurements, mechanical construction and practical usefulness remain unverified.

## Purpose and overview

Mixing Gap helps a maker explore whether two groups of positions in a detached hand-operated mechanism have been seen in every recombination of their individually observed patterns. Four voltage-free contacts witness positions A, B, C and D. The device retains stable whole configurations and examines all seven unordered ways to divide the four positions into two nonempty groups. For each division it lists missing recombinations and provides two actual retained configurations whose left and right patterns produce each missing word.

Consider a puzzle mechanism with two tabs on one side and two on the other. You have observed all tabs open and all tabs closed. That does not establish that the left pair can close while the right pair remains open. For the split A+B | C+D, the instrument exposes the missing full words 3 and 12. If both are subsequently observed, the retained relation is a complete Cartesian product for that split. If a new left-side pattern later appears, new holes may appear. The notebook does not confuse a previously completed product with a permanent finding of independence.

The practical purpose is to direct attention toward untested combinations when exploring modularity, interference and accidental constraints in a slow mechanical mock-up. A missing recombination is an investigation question, not a fault diagnosis. It might reflect a real coupling, incomplete exploration, a misplaced contact, an operator habit, friction, a hidden state or an unreachable configuration. The device cannot distinguish these causes. Never force a mechanism because a displayed word is missing. It does not drive any actuator or grant access to anything.

Target users include puzzle-box designers, museum workshop staff, mechanical-computing educators and makers assembling low-energy modular demonstrations. A useful experiment compares two separately recorded configurations of the same fixture, such as before and after removing a suspected mechanical link. A teacher can also demonstrate that observing every individual position, or even every pairwise value, is not equivalent to observing every combination of larger groups. The proposed improvement is reduced transcription and clearer investigation prompts; no measured user benefit is claimed.

## Originality and related work

Before choosing this device, the repository was pulled on its configured main branch and existing device directories and README purposes were inventoried. The nearby designs were read in more detail. Pair Cover fills pairwise binary coverage tables. Truth Sieve tests a fixed two-input/two-output relation for contradictory responses. Mask Key searches for subsets of designated controls that explain a separate output. Closure Basis finds positive implications between simultaneously observed positions. Mixing Gap instead asks whether an observed relation equals the product of its two group projections, across every nonempty bipartition, and explains each missing recombination with two retained parent words.

This is an original integration proposal, not a claim to have invented relational mathematics. Cartesian products, relation decomposition and multivalued dependencies are established concepts. A bounded external search found the University of Houston notes on multivalued dependencies at https://www2.cs.uh.edu/~ceick/6340/mvd.pdf and the paper “Multivalued dependencies and a new normal form for relational databases” at https://doi.org/10.1145/320557.320571. The latter describes the special empty-left-hand-side dependency corresponding to a Cartesian product of projections. These are mathematical prior art; the proposed device applies a bounded recombination notebook to physically witnessed detached mechanisms. Worldwide novelty, patentability and freedom to operate have not been established.

A paper worksheet is cheaper for a brief experiment. A camera captures richer context, including approach direction and uninstrumented motion. A logic analyzer plus a script could implement similar processing and would be better for fast electrical transitions. Mixing Gap is justified only when direct position observation and an inspectable local evidence workflow are useful. The passive resistor carrier and low-level processor platform are intentionally reused, not represented as new semiconductor or sensor technology.

## Hardware specifications and boundary

The processor assembly is a separately purchased ST NUCLEO-F401RE MB1136 containing an STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. It supplies processor decoupling, voltage regulation, reset, ST-LINK debugging and the USB virtual COM interface. Firmware establishes the nominal internal 16 MHz HSI clock and acquires four digital inputs on newly observed millisecond ticks. USART2 sends telemetry at 115200 baud, eight data bits, no parity and one stop bit. The application receives only; no command parser, radio, cloud service, battery, charger, relay or motor output is implemented.

The custom board is an 80 by 70 mm, two-copper-layer, nominal 1.6 mm FR-4 passive interface. It is cable-connected, not a mechanically mating Arduino shield and not a standalone MCU motherboard. There are six through-hole connectors, eight axial resistors, routed tracks, one ground via, a closed board outline and silkscreen wiring labels. The MCU support components belong to the purchased Nucleo assembly and are not duplicated on the carrier schematic. A complete working system therefore requires both assemblies and four external position contacts.

Each channel uses a 10 kilohm pull-up from its sense node to regulated 3.3 V and a 1 kilohm resistor between that node and the external contact signal. The contact closes signal to shared ground. Select a low-level lever switch or reed contact appropriate to the actual mechanism. Omron D2F-01L is a candidate low-level lever switch, not a physically qualified procurement choice. No live stock, pricing, lifecycle or mechanical-fit guarantee is claimed. High-current switches with oxidized contacts may be unsuitable for this small excitation current.

## Architecture and pin map

    Four detached position contacts A / B / C / D
                         |
                   J3 / J4 / J5 / J6
                         |
             1k series resistor + 10k pull-up each
                         |
                     J2 sense nodes
                         |
             External NUCLEO-F401RE GPIO inputs
                         |
            Whole-word 100 ms stable qualification
                         |
                Retained 16-configuration set
                         |
            Seven group-projection Cartesian products
                         |
         Missing words + explicit observed parent witnesses
                         |
               MG1 frames / CRC / USART2 TX
                         |
                   ST-LINK USB VCP
                         |
         Local Linux Tk screens / replay / private CSV export

Carrier J1 pin 1 connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled connector positions are used as digital inputs. J3, J4, J5 and J6 observe POSITION_A, POSITION_B, POSITION_C and POSITION_D. Each contact connector has signal on pin 1 and ground on pin 2. Electrical low means closed, so the GPIO driver inverts the levels.

The logical word is A + 2*B + 4*C + 8*D. Word three means A and B closed while C and D are open. Word twelve means C and D closed while A and B are open. The suggestion displays named A B C D values in that order, rather than most-significant-bit-first binary text. A split mask selects the positions taken from the first parent; its complement selects positions from the second parent. These meanings agree across firmware, application, connector labels and the hardware connectivity manifest.

PA2 uses alternate function AF7 for USART2 transmit through the onboard virtual COM bridge. Inspect the purchased MB1136 revision and solder bridges SB13/SB14 and SB62/SB63 before connection. Manufacturer references are ST UM1724 for the board and connectors, RM0368 for STM32F401 registers, and PM0214 for Cortex-M4 exceptions. The board manual is available at https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf. This revision inherits the existing platform mapping; it does not claim a fresh physical board audit.

## BOM, power and mechanical fixture

R1, R3, R5 and R7 are 10 kilohm one-percent axial pull-ups. R2, R4, R6 and R8 are 1 kilohm one-percent axial series resistors. All use DIN0207 footprints with 7.62 mm lead spacing. J1 and J3 through J6 are two-position 2.54 mm vertical headers; J2 is a four-position header. kicad/connectivity.json records every component value, footprint and pin net. Add the development board, a compatible USB cable, four contacts, insulated wiring, strain relief and a nonconductive enclosure. Component voltage and power ratings must be checked when sourcing substitutes.

Nominal closed-channel current is 3.3 V divided by 11 kilohms, approximately 0.30 mA. Four closed channels consume approximately 1.20 mA and 3.96 mW from the regulated rail. The sense node is approximately 0.30 V when closed. These are nominal circuit calculations, not measured electrical results. Total USB consumption is dominated by the purchased development board and must be measured on the assembled system. There is no low-power sleep mode: the processor polls the serial queue and acquisition clock continuously. Do not describe this prototype as a battery-operated logger.

A suggested mechanical arrangement uses four independently adjustable insulated witness brackets around a captive hand-operated demonstrator. Assign each contact a precise physical meaning, photograph the mounting, and label the cable at both ends. Position witnesses must not materially alter the force or travel under study. No universal bracket drawing is supplied because the mechanism geometry is application-specific. The carrier has no mounting holes; use insulating edge supports and do not drill into copper without a revised board review. Keep USB and reset accessible and protect wiring from sharp edges and moving parts.

Only disconnected, voltage-free switches may be connected. The carrier shares USB ground and has no galvanic isolation, reverse-polarity protection or adequate external-voltage protection. The series resistors are not a protective barrier. Never attach mains, powered logic outputs, vehicle wiring, battery-backed sensors, operating machinery interlocks or live access-control contacts. Wire with power removed, verify resistance and orientation, and use only the Nucleo regulated 3.3 V supply. This is a dry indoor, low-energy exploratory fixture, not a safety system.

## Firmware behavior and design decisions

Firmware is freestanding C11. main.c performs clock recovery, pin initialization, configuration checks, watchdog startup, sampled acquisition and periodic telemetry. board.h defines the wiring contract; registers.h and drivers contain the register-level implementation. startup.c supplies the reset vector, data/BSS initialization and required memory routines. linker.ld maps the named MCU memories and reserves stack headroom. Shared platform code is imported from jayis1's Closure Basis at commit 3762a0deb5f424a6ba0acd7ab664991b42dcffb3. New analysis, interpretation and regression tests implement Mixing Gap's separate contract.

The first sample establishes an endpoint and earns no observation credit. A complete four-bit word qualifies only after one hundred unchanged consecutive one-millisecond intervals, requiring an initial endpoint plus one hundred later samples. The qualification endpoint adds a visit and sets the corresponding history bit. Its preceding interval remains unknown rather than being retroactively labelled settled. Continuing to hold earns qualified intervals but not additional visits. Every raw change restarts qualification, including a short glitch that never becomes a qualified setting.

Skipped acquisition ticks increment gap and missed-tick diagnostics and discard current qualification. Historical settings remain retained. New observations must earn a fresh full window. Each acquired endpoint after the seed is accounted as one unknown interval, one qualified interval or a gap endpoint. Counter updates are transactional: an overflow cannot commit half a visit. Terminal faults preserve the acquired evidence, while status time and the transmission-error diagnostic may continue. Reset starts an empty volatile history; there is no flash journal or persistent boot identity.

For split s, the analysis forms every word (a & s) | (b & (15 ^ s)) where a and b are retained observations. It removes words already retained. The result is the missing-recombination bitset. The seven canonical split masks are 1, 3, 5, 7, 9, 11 and 13; A is always on the left, eliminating complement duplicates. They correspond to A|BCD, AB|CD, AC|BD, ABC|D, AD|BC, ABD|C and ACD|B. Every reported hole has an explicit left parent and right parent, chosen lexicographically by full numeric word for deterministic presentation.

A split is complete only when history is nonempty and its missing set is empty. Empty history is UNOBSERVED, not evidence of independence. A singleton history completes every split in the narrow retained-set sense; it is weak exploration, not a useful modularity conclusion. New observations can expand projections and create holes, so completeness and the missing count are deliberately not monotonic counters. The total counts split-specific holes; one missing full word may contribute to several splits. It is neither a count of unique unvisited settings nor a probability of coupling.

The generic next-setting suggestion chooses an unobserved full word requiring the fewest bit changes from the current raw word, breaking ties numerically. It is not an optimized gap-closing experiment and may introduce a previously unseen projection. Sentinel sixteen means all configurations are observed. The suggestion is never a motion command, and the application retains it as historical information after disconnection. A complete Cartesian product of retained positions is not statistical independence, causality, proof of reachability, or evidence that the transition path between configurations is safe.

## Wire protocol and companion application

MG1 frames carry eighteen unsigned decimal fields in order: now, samples, gaps, missed, tx_errors, raw, hold, credited, seeded, fault, visits, settings, coverage, qualified_ms, unknown_ms, suggestion, complete and missing_total. The body starts with MG1 followed by comma-separated fields. An asterisk, four uppercase hexadecimal CRC digits and newline terminate the record. CRC-16/CCITT-FALSE uses initial value 65535 and polynomial 0x1021 over the complete body preceding the asterisk. The check string 123456789 produces 0x29B1. This detects corruption but supplies no authentication.

settings is a sixteen-bit set whose bit w indicates that raw word w was qualified. coverage repeats that set in this protocol revision so the inherited acquisition validator remains explicit. complete is a seven-bit mask in canonical split order. missing_total sums each split's missing-word population. The app recomputes these summaries from settings rather than trusting transmitted conclusions. Fault values are zero normal, one invalid board configuration, two watchdog startup failure, three invalid input and four counter exhaustion. No binary response or acknowledgment command exists.

Run `python3 mixing-gap/app/main.py` on Linux with Tk installed. The first screen lists all seven divisions, missing full words, retained-product status and left/right projection populations. The Acquisition / limits screen displays every telemetry field and the parent words for each gap. Connect opens the requested serial device, usually /dev/ttyACM0. Disconnect preserves evidence. A second connection is refused while a port is owned. A failed port open does not erase the prior session, and a successful reconnect clears old summaries before new evidence arrives.

Replay is available only while disconnected and accepts regular files no larger than two MiB. It rejects malformed values, checksum failures, inconsistent summaries, impossible bounded transitions, empty files and partial trailing records. Live parsing resynchronizes on the complete MG1 marker after fragments. Acquisition freshness is tracked separately from traffic reception. Stale acquisition or excessive cumulative device/host time disagreement closes the connection and requires an explicit reconnect. These are conservative diagnostics; without a persistent boot identity some resets remain indistinguishable from valid forward traffic.

CSV export creates a new private file without overwriting an existing path. It records author jayis1, provenance, connection state, warnings, parser diagnostics, acquisition progress age and the latest two thousand accepted snapshots. Total accepted count remains separate from retained row count. The exported settings allow all recombination gaps and parents to be recomputed. Sparse transition validation enforces necessary consistency bounds, not a reconstruction of every unreported sample. Synthetic replay remains synthetic evidence even when it passes all validators.

## Build, test and commissioning

From the repository root, run `make -C mixing-gap/firmware clean all test fixture`. This produces the ARM ELF and binary, runs sanitized C tests and generates a synthetic MG1 fixture. Run `xvfb-run -a python3 -m unittest discover -s mixing-gap/tests -p 'test_app.py' -v` for model, transport, replay, GUI and PTY tests. A normal desktop can omit xvfb-run when a display is available. No Python packages beyond the standard library and Tk are required for the companion. Target compilation requires the GNU Arm Embedded toolchain; host tests require GCC with address/undefined-behavior sanitizers.

Generate the CAD with `/usr/bin/python3 mixing-gap/tools/generate_hardware.py` using KiCad 9 pcbnew bindings and installed connector/resistor footprints. Then run `/usr/bin/python3 mixing-gap/tools/check_hardware.py`. The checker runs native ERC and DRC, exports a fresh schematic XML netlist, and compares reference, value, footprint identity and every pin net against the board and manifest. Passive-only ERC cannot verify the external development board, contact suitability or supply polarity. Generated local preference files and build products are ignored rather than committed.

When real hardware is available, inspect the assembly unpowered, check shorts and resistor values, and verify each connector mapping independently. Program with `make -C mixing-gap/firmware flash` only after confirming the ST-LINK target and supply. That target invokes OpenOCD; physical flashing was not performed in this run. Exercise each contact separately, compare measured node voltages against the MCU electrical limits, and compare reported words against known switch combinations. Measure timing and USB behavior before trusting an experimental session. A passing software build is not a completed bring-up.

Start a real session by defining what each contact witnesses and recording the fixture identity. Explore freely reachable configurations with pauses longer than the qualification window. Save evidence before reset, adjustment or rewiring, and use a new output file for each fixture condition. Do not pool histories across a mechanical change: an old observation may no longer be reachable. Before any stronger modularity claim, investigate missing words, repeat trials with changed approach paths, inspect sensor disturbance and record the limitations of the physical setup. The delivered design supports this investigation; it does not replace it.
