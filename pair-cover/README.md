# Pair Cover — physical pairwise-configuration coverage workbench

Author and creator: jayis1

Copyright (c) 2026 jayis1. Project-owned sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype. The firmware links for the target, the application is exercised with synthetic serial traffic, and the carrier passes CAD checks. Physical electrical behavior, programming, enclosure fit and production suitability remain unverified.

## Purpose and overview

Pair Cover is a four-switch tabletop instrument that helps an experimenter explore combinations without confusing a large number of repeated settings with useful coverage. Four maintained, voltage-free contacts represent binary controls A, B, C and D. Each full setting that remains unchanged for a complete qualification window contributes its six pair projections to a persistent-in-RAM coverage map. The companion displays which of the four possible values have been observed for every unordered control pair, then suggests an uncovered setting requiring few changes from the current switches.

A practical use is planning exploratory tests of a mock control panel. Suppose a designer wants to exercise every pair of options together before attempting more expensive three-way or exhaustive testing. Handwritten checklists are easy to lose track of when the operator changes several switches at once. Pair Cover records stable physical settings, exposes omissions, and makes the next suggested action concrete. It does not observe the controlled product's response. A separately recorded outcome, operator judgment, or automated test result is still necessary to establish that a configuration actually works.

The proposed device is also useful in combinatorial-testing education, museum activity development and low-cost fixture planning. Students can compare a complete pairwise map with the smaller set of full settings used to produce it. Seeing all twenty-four pair-value cells filled is deliberately not labelled exhaustive testing: sixteen full settings and higher-order interactions are separate concepts. A repeated qualified visit can increase the visit count without adding any new coverage. The instrument keeps that distinction visible rather than rewarding button activity alone.

## Originality and relationship to existing devices

The existing repository directories were inspected before recovery of this design. Pair Cover was an interrupted implementation of this same authorized task, not an unrelated contribution. Its specific contribution is a physically witnessed binary-setting coverage notebook with minimum-toggle next-setting guidance and explicit qualification and acquisition-loss accounting. Pairwise software testing and covering arrays are established techniques; this project makes no worldwide novelty, patentability or freedom-to-operate claim.

Touch Tour, a nearby repository design, closes rounds after four distinct stations have been visited with clear releases between them. It tracks individual station identity and first-visit order, not pairwise binary values. ContactAtlas discovers stable signatures and their directed transitions for a detached selector. Pair Cover instead projects a retained set of whole configurations into six pair tables and proposes a setting with additional coverage. It neither counts routes nor measures rotary transition topology. Reuse of the passive resistor carrier and processor support is intentional; the resistor network is not represented as a new invention.

The practical differentiator is therefore the integrated workflow rather than new semiconductor technology. An operator sees a physical configuration, receives a locally computed next suggestion, and exports exactly the retained evidence with provenance. There is no cloud dependency or account. A logic analyzer with a custom decoder could implement a similar measurement, and a software-only checklist is cheaper if physical observation is unnecessary. This instrument is justified only when automatically recording actual detached switch positions reduces errors enough to warrant a small fixture.

## Hardware specifications

The controller is an external ST NUCLEO-F401RE MB1136 development board containing an STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware uses the internal nominal 16 MHz HSI oscillator, not a new external oscillator on the carrier. Four digital GPIO inputs represent contact closures. Acquisition is nominally one sample per millisecond. A setting qualifies after one hundred consecutive unchanged intervals, which requires one initial observation and one hundred later observations. The communication interface is USART2 transmit-only at 115200 baud, eight data bits, no parity and one stop bit, through the Nucleo ST-LINK USB virtual COM port.

The custom carrier is an 80 by 70 mm two-layer board with nominal 1.6 mm FR-4 thickness. It contains six through-hole headers and eight axial resistors. Four 10 kΩ resistors provide pull-ups and four 1 kΩ resistors sit between sense nodes and external contacts. The carrier has no processor, regulator, USB connector, oscillator, battery charger, isolation barrier or actuator. Those absent functions must not be inferred from the presence of a complete KiCad project. The separately purchased development board supplies regulation, decoupling, reset, debug access and the USB bridge.

Use four maintained low-level dry-contact switches mounted in an insulating panel, labelled A through D. Select contacts rated by their supplier for low excitation current; a suitable low-level signal toggle is preferable to an oxidized mains switch. No exact switch procurement or enclosure fit has been physically qualified. Short insulated leads and strain relief are part of the proposed assembly. Keep all conductive joints inaccessible during use, and provide an opening for the development board's reset and USB connection. The PCB is cable-connected, not a mechanically mating Arduino shield.

## Architecture and pin contract

    Maintained dry contacts A / B / C / D
                    |
            J3 / J4 / J5 / J6
                    |
          1k series + 10k pull-up per channel
                    |
            SENSE0 / 1 / 2 / 3
                    |
            J2 -> external Nucleo GPIO
                    |
         1 ms sampled whole-word qualification
                    |
         16-bit setting set -> 24-bit pair map
                    |
          minimum-toggle uncovered suggestion
                    |
          PC1 records + CRC -> USART2 TX
                    |
            ST-LINK USB virtual COM
                    |
          Linux Tk workbench / replay / CSV

Carrier J1 pin 1 connects to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1, 2, 3 and 4 connect respectively to CN8 pins 1, 2, 3 and 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. The analog-labelled header positions are used as digital inputs. J3, J4, J5 and J6 correspond to CONTROL_A, CONTROL_B, CONTROL_C and CONTROL_D. Each contact header has signal on pin 1 and ground on pin 2. Logical one means closed, although the electrical input is active low.

PA2 uses alternate function AF7 for USART2 transmit to the onboard virtual COM interface. The receiver is not enabled. The platform assumes the MB1136 virtual-COM solder-bridge configuration documented by ST; inspect SB13/SB14 and SB62/SB63 on the purchased board before connection. Consult ST UM1724 for connector and solder-bridge assignments, RM0368 for peripheral registers, and PM0214 for Cortex-M4 exception and SysTick behavior. The project inherits this established platform contract; this recovery does not claim a fresh audit of the latest manufacturer document revision.

## Electrical limits, power and assembly

Each closed path is 3.3 V through 10 kΩ, then the sense junction, then 1 kΩ and the closed switch to ground. At nominal values its current is 0.30 mA and the sense level is 0.30 V. Four closed paths consume 1.20 mA, or 3.96 mW from the 3.3 V rail. These are calculated carrier loads, not measured total USB consumption. The Nucleo and debugger dominate the overall budget. Verify their actual USB demand, supply voltage and thermal behavior during bring-up rather than assigning a fictional whole-device measurement.

Use ordinary appropriately rated through-hole resistors, preferably one-percent values for consistent channels. The footprint is the KiCad axial DIN0207 6.3 mm body with 7.62 mm lead spacing. Headers use 2.54 mm pitch: J2 has four positions and the other five headers have two. The connectivity manifest lists every component, value, footprint and pin net. The authored schematic symbols are embedded and also supplied in Ledger.kicad_sym. The board uses routed copper, actual library footprints and a closed outline; there are no unrouted decorative components.

Only disconnected, voltage-free contacts may be attached. Never connect this fixture to powered machinery, mains, battery packs, externally driven logic, or a switch that remains wired into another apparatus. USB ground is common with the contact returns. The 1 kΩ resistors do not provide galvanic isolation, adequate surge protection, or permission to apply an external voltage. There is no reverse-polarity protection on the carrier supply header. Connect with power removed, inspect pin orientation, check resistance between supply and ground, and use the Nucleo's regulated 3.3 V output exclusively.

Low contact current can fail to break through contamination. A missed closure is evidence about this test condition, not proof that a contact cannot carry its rated load. There is no calibrated resistance measurement. Long leads can collect interference; begin with short bench wiring and compare against an independent analyzer. The proposed enclosure must prevent accidental shorts and strain on the header leads. Neither CAD checks nor firmware tests establish ESD immunity or regulatory compliance.

## Firmware structure and decisions

firmware/main.c performs clock setup, initializes GPIO, starts the watchdog, and samples on each newly observed SysTick value. startup.c and linker.ld provide the reset entry, exception vector, RAM initialization and target memory layout. registers.h contains the memory-mapped addresses used by the small board drivers. No vendor SDK or hosted C library is required. The Makefile compiles and links a freestanding Cortex-M4 ELF with the GNU Arm toolchain and produces a binary image.

The clock driver explicitly restores HSI and prescalers before using timing-dependent interfaces. Its waits have iteration budgets because SysTick is not yet running. If the reset clock cannot be established, execution stops before serial initialization. Input configuration is checked after initialization. Watchdog startup has bounded waits while SysTick is available. The main loop services the watchdog during acquisition. These are defensive software paths, not evidence that physical watchdog recovery or brownout behavior has been tested.

engine.c holds the measurement state independently of registers. A changed raw word clears its current hold and qualification credit. A one-sample glitch therefore restarts qualification even if it never becomes a credited setting. Once one hundred unchanged intervals have accumulated, one visit is recorded and the setting's bit is retained. Holding the same setting indefinitely adds qualified interval time but not repeated visits. Leaving and returning can produce another visit while leaving the distinct-setting set unchanged.

Whole-word qualification is deliberate: the unit of evidence is a complete four-control configuration. This differs from independently timing unrelated contacts. A missed sample prevents a claim of continuous stability. If the observed timestamp jumps, the engine records a gap endpoint and missed ticks, clears the active hold, and requires a new full qualification window. Historical settings and pair coverage remain available. Unknown intervals, qualified intervals and gap endpoints conserve acquired samples after the initial seed. Missed wall-clock intervals are separately represented rather than silently assigned to a stable configuration.

Updates are transactional. A counter increment that would overflow rejects the entire speculative update and latches an exhaustion fault while preserving prior evidence. The first fault cause wins. Terminal fault records can advance their timestamp and transmit-error diagnostic, but do not earn further acquisitions. Ordinary unsigned timestamp wrap is supported for elapsed timing. Reset clears volatile history, so sessions must not be treated as a permanent calibration or long-term usage ledger.

The suggestion function enumerates all sixteen possible settings that add at least one unseen pair value. It first minimizes Hamming distance from the raw switch word, then maximizes newly covered pair cells, then breaks ties by smaller numeric setting. A suggestion of sixteen means no pair cell remains uncovered. This is a deterministic greedy helper, not an optimal covering-array solver or proof of the shortest remaining experimental sequence.

## Protocol and application interface

PC1 is the periodic protocol marker. Every record contains sixteen unsigned decimal fields in this order: now, samples, gaps, missed, tx_errors, raw, hold, credited, seeded, fault, visits, settings, coverage, qualified_ms, unknown_ms and suggestion. A star introduces four uppercase hexadecimal CRC digits; newline terminates the record. CRC-16/CCITT-FALSE covers the marker and comma-separated body, with initial value 65535 and polynomial 0x1021. The known check string 123456789 produces 0x29B1. CRC detects accidental corruption, not malicious modification.

Pair order is AB, AC, AD, BC, BD, CD. Each pair occupies four consecutive coverage bits for values 00, 01, 10 and 11, with the earlier named control contributing the high bit of that pair value. Full setting masks instead use A as bit zero, B as bit one, C as bit two and D as bit three. These conventions are intentionally explicit because confusing printed bit order with numeric mask order produces convincing but wrong coverage tables.

The Linux companion uses Python's standard library, Tk and POSIX termios; no network service or package download is required. Run `python3 pair-cover/app/main.py`, select the device's virtual serial port, and press Connect. The Pair coverage tab shows observed and missing values for all six pairs, distinct full settings, visits and a suggested A B C D setting. The Acquisition / limits tab exposes the underlying fields and measurement qualifications. Change the physical switches, hold the new combination, and watch the relevant cells appear.

Disconnect preserves retained evidence. Replay loads a local PC1 recording only while disconnected and labels its provenance explicitly. Export CSV uses exclusive creation and owner-only permissions, refuses to overwrite an existing file, and includes author, provenance, warning, parser diagnostics, connection status and retained-row count before the samples. Only the latest two thousand accepted rows are retained, while the total accepted count is tracked separately. Choose a new filename for each export; this is an evidence snapshot rather than an unlimited logging daemon.

The decoder bounds records and input chunks, validates CRC and field ranges, and resynchronizes using the complete PC1 marker after truncation. Replay is stricter than live late attachment: it rejects fragments, partial trailing records, empty input, special files and files over two MiB. The model checks pair projection, conservation, monotonic counters, acquisition budgets, disappearing settings and contradictory duplicates. Sparse checks are necessary bounds, not a reconstruction proving every hidden physical action.

The GUI disconnects on terminal faults, stale acquisition, impossible transitions or excessive host/device progress mismatch. A two-second acquisition-age limit and fixed one-second progress discrepancy budget guard against stopped acquisition and accumulating backlog. Oscillator error can cause a conservative reconnect on sufficiently long sessions. Without persistent boot identity some resets remain indistinguishable from forward traffic; reconnect after reset and do not combine separate experiments implicitly. There is no authenticated pairing because transport is local and receive-only, but anyone who can replace serial bytes can forge CRC-valid evidence. Restrict local device and file access accordingly.

## Reproduction and verification

From the repository root, run `make -C pair-cover/firmware clean all test fixture`. The target build needs arm-none-eabi-gcc and related binutils; host tests need gcc with address and undefined-behavior sanitizers. Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s pair-cover/tests -p 'test_*.py'` for model and real Tk event-loop tests. Linux packages providing Tk and Xvfb are required for headless testing. The fixture is generated by the actual C encoder from explicitly synthetic switch inputs, not from fabricated physical measurements.

Run `/usr/bin/python3 pair-cover/tools/check_hardware.py` with KiCad 9 and its pcbnew bindings installed. It performs ERC, DRC, fresh XML netlist export, and exact comparison of schematic and PCB pin nets, component values and footprint identities against connectivity.json. The generator can recreate hardware sources, but normal validation does not rewrite them. tools/import_platform.py records the initial reused platform provenance and refuses existing-file overwrites; it is not an update command for an established checkout.

The recovered implementation was freshly rebuilt: target image text was 2400 bytes, data zero and BSS 1044 bytes. Three host C test programs passed with sanitizers, including all 65536 setting subsets and all 256 ordered raw-word transitions. Twelve Python tests passed, including real-encoder replay, malformed frames, suggestions, GUI rendering, private export, live PTY reception, duplicate connection rejection, hangup and stale detection. CAD verification reported zero ERC violations, zero DRC violations and zero unconnected items, with fourteen components, thirty connected pins and ten nets matching exactly.

Physical acceptance remains a separate gate. Inspect the assembled board unpowered, verify the supply and every input level with a meter, then program through ST-LINK using the Makefile flash target only after confirming the exact board. Exercise every switch combination against an independent checklist. Inject brief glitches and deliberately interrupted acquisition, compare timing with a logic analyzer, unplug and reconnect USB, and verify startup and watchdog behavior. Measure current and enclosure clearances. Until those experiments are completed, treat the design as a reproducible engineering prototype, not a fabrication-qualified commercial instrument or a safety-related test system.
