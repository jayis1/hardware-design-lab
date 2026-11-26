# Truth Sieve — contradiction-preserving mechanical truth-table workbench

Author and creator: jayis1

Copyright (c) 2026 jayis1. Project-owned code and documentation are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype. Target compilation, synthetic software experiments and CAD checks are reproducible; fabrication, target programming, electrical behavior, mechanical construction and practical user benefit remain unverified.

## Purpose and overview

Truth Sieve is a small bench instrument for exploring a slow, hand-operated mechanism with two binary input positions and two binary output indications. Four voltage-free contacts witness those positions. Instead of asking the operator to transcribe a truth table, it accumulates every qualified input/output association and displays a four-row relation. Crucially, a second response for the same input does not replace the first. Both remain visible, and the instrument withdraws the claim that any deterministic, memoryless two-input/two-output table fits the observations.

Consider a teaching linkage intended to behave as a mechanical half adder. Two hand levers represent A and B; a pair of output flags represents sum X and carry Y. An instructor can set the input levers, let the mechanism rest, and inspect the measured relation. If a sticky linkage sometimes leaves the sum flag up after returning to zero, Truth Sieve retains that disagreement rather than quietly learning whichever output happened last. The operator can then investigate friction, backlash, contact adjustment, a missing spring, or genuine stateful behavior. None of these causes is diagnosed automatically.

The primary audience is makers of mechanical logic demonstrations, museum exhibit technicians and educators comparing hand-built linkages. A secondary use is documenting two mechanically sensed control settings against two disconnected switch indications in a completely unpowered teaching assembly. This is not a powered-circuit logic analyzer. It does not drive stimulus, connect to a logic output, identify which wire is wrong, or certify an interlock. Its practical hypothesis is that retaining contradictions and showing missing input cases reduces transcription mistakes during slow exploratory experiments.

A useful result is not necessarily a complete deterministic table. An incomplete table honestly describes missing exploration. A contradictory table honestly says that a memoryless explanation is not supported under the chosen observation conditions. Even a single compatible table is only compatible with retained observations; it is not proof that every future visit will repeat, that transient states were harmless, or that the underlying mechanism implements the intended function.

## Originality, repository comparison and alternatives

The repository directories and device titles were inventoried after a successful fast-forward-only pull. The closest existing designs were read before choosing this concept. Pair Cover tracks pairwise combinations of four independent controls and suggests uncovered whole settings; it does not divide the channels into stimulus and response or test whether a function exists. ContactAtlas maps contact signatures and transitions of a detached selector. Pattern Sled learns an ordered multiword strip and compares subsequent indexed repeats. Truth Sieve instead retains a relation from two witnessed inputs to two witnessed outputs, detects contradictions within individual rows, and counts all compatible memoryless maps without learning a privileged reference run.

Truth tables, mechanical logic and switch teaching boards are established ideas. A bounded external search located the Logic Gates Board at https://logicgat.es/product/logic-gates, which combines switches, LEDs and printed truth tables, and ManyPI's logic sandbox at https://manypi.com/edu/logic-gates-sandbox, which derives tables from simulated circuits. These are relevant alternatives, not evidence of worldwide novelty. This design's proposed contribution is the physical, contradiction-preserving observation workflow with explicit missing-sample qualification and local evidence export. No patentability or freedom-to-operate conclusion is claimed.

A paper worksheet is cheaper when only one observation per input is required. A camera preserves richer mechanical context. A commercial logic analyzer resolves much faster transitions. Truth Sieve is worth building only if automatically keeping repeated physical observations separate from deterministic assumptions is useful. Reusing jayis1's passive carrier, startup code and Linux serial infrastructure is intentional. The resistor network is not represented as a new sensor invention, and the firmware's relation model is not presented as a new mathematical discovery.

## Hardware specifications

The controller is a separately purchased ST NUCLEO-F401RE, MB1136, with an STM32F401RET6 Cortex-M4 processor, 512 KiB flash and 96 KiB SRAM. Firmware restores the internal nominal 16 MHz HSI clock. It reads four digital inputs once per observed millisecond, qualifies the complete four-bit word, and emits receive-only host telemetry through USART2 at 115200 baud, eight data bits, no parity and one stop bit. There is no radio, cloud account, application command receiver, motor driver, relay or battery charger.

The supplied custom PCB is an 80 by 70 mm, two-layer, nominal 1.6 mm FR-4 passive interface. It contains six through-hole headers and eight axial resistors. Four 10 kΩ resistors pull the sense nodes to 3.3 V; four 1 kΩ resistors connect those nodes to the external contacts. The purchased Nucleo supplies the processor, voltage regulation, processor decoupling, reset circuit, ST-LINK debugger and USB virtual serial bridge. Those functions are not secretly omitted from a supposed standalone MCU schematic: this project explicitly requires the development board as a complete assembly.

Use four low-level, voltage-free position switches or reed contacts. A low-level lever switch such as an Omron D2F-01L is a candidate, not a physically qualified procurement choice. Contact force and hysteresis can disturb a light teaching mechanism, so the mounting must be evaluated experimentally. The carrier is cable-connected rather than a mechanically mating Arduino shield. Enclose the boards and joints, provide cable strain relief, and leave the Nucleo USB and reset controls accessible. Enclosure machining and exact switch-bracket geometry depend on the observed demonstrator and are not supplied as fabrication-ready mechanical drawings.

## Architecture and pin contract

    Two manual input-position witnesses A / B ---- J3 / J4
    Two mechanism output witnesses X / Y --------- J5 / J6
                       |
             1k series + 10k pull-up channels
                       |
                     J2 GPIO
                       |
             NUCLEO-F401RE, 1 ms sampling
                       |
           100 unchanged intervals qualify word
                       |
            retained sixteen-cell relation
                       |
       row conflict / compatible maps / missing input
                       |
          TS1 + CRC -> USART2 -> ST-LINK USB
                       |
          Linux Tk relation / diagnostics / CSV

Carrier J1 pin 1 is VCC and connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. Carrier J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled header positions are used as digital inputs. J3 is INPUT_A, J4 INPUT_B, J5 OUTPUT_X and J6 OUTPUT_Y. Every contact connector uses pin 1 for signal and pin 2 for ground. Output here means an observed mechanical response, never an electrically driven carrier output.

Logical one means a closed contact despite the electrical input being active low. The raw numeric word has A at bit zero, B at bit one, X at bit two and Y at bit three. The GUI prints A B and X Y in those named orders; it does not display ordinary most-significant-bit-first binary strings. Numeric input one therefore appears as A=1, B=0. This distinction prevents a swapped-bit interpretation that could otherwise make a correct half-adder fixture appear wrong.

PA2 is configured as alternate function AF7 for USART2 transmit. The receiver is disabled. The assumed MB1136 ST-LINK virtual-COM bridge arrangement must be checked on the purchased revision, especially SB13/SB14 and SB62/SB63. Manufacturer references are ST UM1724 for board connectors and solder bridges, RM0368 for STM32F401 registers, and PM0214 for Cortex-M4 exceptions and SysTick. This revision reuses the repository's established platform contract rather than claiming a new manufacturer-document audit.

## Bill of materials, power and limits

R1, R3, R5 and R7 are 10 kΩ pull-ups. R2, R4, R6 and R8 are 1 kΩ series resistors. Use one-percent axial parts suitable for the KiCad DIN0207 body, 7.62 mm lead spacing. J1 and J3 through J6 are two-position, 2.54 mm headers; J2 is a four-position header with the same pitch. Add the external Nucleo board, a compatible USB cable, four low-level contacts, short insulated wiring and an insulating enclosure. The machine-readable connectivity manifest records each on-carrier reference, value, footprint and pin net. It is the PCB assembly list, not a complete procurement list for the mechanical demonstrator.

A nominal closed channel draws 3.3 V divided by 11 kΩ, or 0.30 mA. Its sense voltage is approximately 0.30 V. With four closed contacts the passive load is 1.20 mA, or 3.96 mW from the 3.3 V rail. These are calculated nominal loads, not measured total USB consumption. The development board and debugger dominate the device budget; their real demand must be measured on the assembled unit. The low contact excitation may fail to penetrate oxide on switches intended for larger loads. Missed closure under this condition is not evidence about the switch's full rated-current performance.

Never attach a contact still connected to mains, a battery, powered machinery or an external logic driver. USB ground is shared with all contact returns. The series resistors are neither galvanic isolation nor adequate external-voltage protection. The carrier has no reverse-polarity protection, surge suppressor or isolation barrier. Wire it with power removed, check header orientation and inspect for shorts before using only the Nucleo's regulated 3.3 V supply. This is a dry indoor low-energy bench fixture; it must not make safety decisions.

## Firmware behavior and design decisions

The target is freestanding C11 compiled with GNU Arm Embedded tools. main.c establishes the clock, configures GPIO and serial transmission, starts the watchdog and acquires at each new SysTick value. startup.c and linker.ld provide reset vectors, RAM initialization and the target memory layout. registers.h contains the memory-mapped register definitions used by the drivers. The main loop never waits for an entire serial frame to finish; the UART driver drains one available byte at a time and abandons a stalled transmission after a bounded interval.

engine.c implements acquisition independently of hardware registers. The first sample seeds the raw word but earns no visit. A setting qualifies only after one hundred consecutive unchanged one-millisecond intervals. It therefore needs an initial endpoint and one hundred later observations. A raw change restarts the window, including a single rejected glitch. Qualification records one visit and its relation cell. Continued holding adds qualified interval time without adding visits. Leaving and later returning to the same word can add another visit while its retained relation cell remains unchanged.

The qualification window is an observation convention, not proof that the mechanism has physically finished moving. If a linkage pauses incorrectly for more than one hundred milliseconds and later recovers, both outputs can become retained evidence. That is appropriate for a notebook of qualified states but must not be described as a calibrated settling-time test. Change the physical experiment or undertake a separately reviewed protocol revision if this window is unsuitable; do not reinterpret the existing recording after the fact.

Every acquired post-seed endpoint belongs to exactly one of three accounting classes: unknown interval, qualified interval, or gap endpoint. When sampling skips ticks, the engine counts the gap and missed ticks, clears ongoing qualification, and requires a new full window. Historical relation cells survive. Missing intervals never become continuous stable exposure. The engine stages each update in a temporary state so counter exhaustion cannot commit half a visit. The first terminal fault cause wins; later terminal telemetry can advance timestamp and transmit-error diagnostics but cannot earn evidence.

analysis.c interprets each row as the set of outputs actually observed there. An empty row leaves four deterministic responses possible. A singleton row fixes one response. A row containing two or more outputs contradicts every deterministic memoryless mapping. The number of compatible maps is the product of those independent row possibilities, or zero after any contradiction. This is exact for the retained finite relation and the explicitly chosen model class. It does not infer causality, diagnose physical defects or assign confidence probabilities.

Next-input guidance chooses an unobserved input requiring the fewest toggles from the current A/B positions, with smaller numeric input breaking ties. The suggestion never dictates X/Y. When every row has been observed it returns the sentinel four, even if conflicts remain. The UI then asks the operator to inspect conflicts and repeat trials rather than pretending the experiment is finished successfully. All state is volatile; a reset starts a new experiment. There is no flash journal or permanent instrument identity.

## Protocol and companion application

TS1 records contain sixteen unsigned decimal values in this order: now, samples, gaps, missed, tx_errors, raw, hold, credited, seeded, fault, visits, settings, coverage, qualified_ms, unknown_ms and suggestion. A record begins with TS1 followed by comma-separated fields; a star introduces four uppercase hexadecimal CRC digits, followed by newline. CRC-16/CCITT-FALSE uses initial value 65535 and polynomial 0x1021 over the marker and comma-separated body. The standard check string 123456789 yields 0x29B1. CRC is corruption detection, not authentication.

The settings field is a sixteen-bit set of raw words. The coverage field is the same evidence arranged by input row: bit number four times input plus output. Input equals raw AND three, while output equals raw shifted right by two. Keeping both encodings permits a cross-check in the firmware and host decoder. They are cumulative relation membership, not per-cell frequencies. The global visits count does not say how many repetitions belonged to a particular row. Historical dwell totals likewise do not describe response latency or time spent in a particular relation cell.

Run `python3 truth-sieve/app/main.py` on Linux with Tk installed. Enter the serial device path and connect. The Observed truth relation screen shows four input rows, all witnessed outputs and explicit UNOBSERVED, single-response or CONFLICT status. The summary shows the compatible-map count. The Acquisition / limits screen displays the protocol fields and interpretation cautions. Disconnect retains evidence; reconnect deliberately begins a new host session and clears displayed results. A failed port open preserves the previous evidence instead of destroying it.

Replay loads a regular local TS1 file only while disconnected. It rejects special files, files above two MiB, empty input, corrupt records and incomplete trailing frames. Live decoding permits late attachment by counting discarded fragments and resynchronizing on the complete marker. CSV export uses exclusive creation and private permissions, refuses overwriting an existing file, and includes author, provenance, connection state, warnings and parser diagnostics. Only the latest two thousand accepted rows are retained, while the total accepted count remains visible in exports. Export is a bounded evidence snapshot, not an unlimited logging service.

The companion checks field ranges, relation projection, qualification budgets, cumulative accounting, disappearing settings, counter rollback and necessary sparse-transition bounds. It stops a live connection after a terminal firmware fault, stale acquisition or excessive host/device time-progress discrepancy. A fixed discrepancy allowance limits gradually accumulating backlog, but uncalibrated oscillator drift can also cause conservative disconnection. Without a persistent boot identifier, some resets are indistinguishable from plausible forward traffic. Always reconnect after resetting the instrument and do not silently merge separate experiments.

## Build, verification and physical acceptance

Run `make -C truth-sieve/firmware clean all test fixture` from the repository root. This produces the Cortex-M4 ELF and binary, exercises host-side C tests with address and undefined-behavior sanitizers, and generates an explicitly synthetic TS1 fixture using the real C encoder. The relation test checks every possible retained settings set against an independent enumeration of all deterministic mappings, then exercises ordered stable-word pairs, qualification boundaries, gaps, contradiction retention, timestamp wrap, exhaustion and CRC handling.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s truth-sieve/tests -p 'test_*.py'` for the standard-library model, serial and real Tk event-loop tests. Run `/usr/bin/python3 truth-sieve/tools/check_hardware.py` with KiCad installed for native ERC, DRC and fresh schematic-netlist comparison against PCB pads and the manifest. The generator requires the distribution Python environment exposing pcbnew. Generated build products and local KiCad preferences are ignored. The import script documents pinned infrastructure ancestry and refuses existing destinations; it is not a command to overwrite this finished device implementation.

See VALIDATION.md for commands, tool versions and actual outcomes. Software tests use generated traffic and pseudo-terminals; they are not measurements from hardware. Before physical use, inspect the carrier, verify supply polarity, measure each open and closed input, confirm contact numbering, program the target through ST-LINK, and compare sampling and serial behavior against an independent instrument. The Makefile flash target invokes OpenOCD, but flashing was not performed during design validation.

For a first mechanical experiment, construct or select a disconnected two-input mechanism whose intended output relation is independently known. Record all four input cases, repeat them in a different order, and deliberately introduce a persistent alternate output for one input. Confirm that the earlier cell survives and the compatible-map count becomes zero. Repeat with a brief rejected contact disturbance and with a recording interruption. Check whether switch loading changes the demonstrator. These are proposed physical acceptance tests, not claimed results. Only after such work can useful speed, repeatability, enclosure suitability and practical benefit be assessed.
