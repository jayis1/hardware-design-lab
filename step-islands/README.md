# Step Islands — single-change configuration exploration fixture

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype. Target compilation, synthetic acquisition, application operation and electrical CAD checks are reproducible; fabrication, flashing, electrical bring-up, fixture construction and actual user benefit remain unverified. Nothing here is a safety interlock or manufacturing release.

## Purpose, overview and practical value

Step Islands helps a maker explore the resting configurations of a small, detached, hand-operated mechanism. Four voltage-free contacts witness positions A, B, C and D. The instrument retains every whole configuration that remains unchanged through its qualification window. It then constructs a deliberately limited graph: two retained configurations are adjacent if exactly one contact differs. The local companion shows disconnected groups, shortest candidate paths through retained configurations, and unobserved single configurations that would join currently separate groups if later observed.

The practical question is different from counting switches or checking a final state. A puzzle-box designer may have observed several apparently reachable resting arrangements but not know where the gaps lie in a one-change-at-a-time exploration. A museum workshop may want to teach why individually plausible endpoints do not imply a demonstrated route between them. A physical notebook can expose which intermediate configurations deserve careful investigation without demanding a video review or manual transcription of every setting. It does not prove that a candidate step can actually be performed.

Consider two observed words, zero and three. Zero means all contacts open; three means A and B closed. These form two islands because they differ at two contacts. Words one and two are both bridge candidates: either would join the two islands in the retained single-bit graph. If word one is observed next, the notebook presents the candidate path zero, one, three. It has not observed the actual motion between those positions and cannot determine whether a hidden latch, approach direction, force, collision or uninstrumented coordinate prevents that route. The distinction is central, not a footnote.

The intended audience is makers of low-energy educational mechanisms, mechanical logic demonstrators and puzzle prototypes. A useful experiment compares two separately recorded fixture arrangements, such as before and after changing a removable constraint. The experimenter records the physical meaning of each contact and pauses at freely reachable positions. The instrument helps organize questions, not answer them by force. The highest-risk assumption is that the four binary witnesses capture enough of the physical configuration to make the graph useful. Bench observation must test that assumption before relying on it.

## Originality, alternatives and scope

The repository was pulled on its configured main branch and existing device directories and README titles were inventoried before implementation. Nearby measurement contracts were inspected. Pair Cover measures pairwise value coverage. Mixing Gap identifies missing Cartesian recombinations of position groups. Closure Basis learns positive implications. Detent Route records actual qualified station departures and arrivals. Step Islands instead constructs the induced one-bit adjacency graph of retained whole configurations, reports its connected components, exposes single-node bridge candidates and offers shortest retained-set path witnesses. It does not claim that graph adjacency is a measured transition.

Reconfiguration graphs and shortest paths are established mathematics, not inventions of this project. The survey “Introduction to Reconfiguration,” https://www.mdpi.com/1999-4893/11/4/52, discusses feasible configurations, single-step adjacency, connectivity and shortest transformations. This device is a proposed physical exploration workflow applying those ideas to sensed resting positions. Worldwide originality, patent novelty and freedom to operate have not been established. The claimed contribution is the specific bounded instrument and its evidence-preserving workflow, not the underlying graph algorithms or resistor interface.

A paper worksheet is cheaper for a short exercise. A camera captures hidden motion and human context that these contacts omit. A conventional logic analyzer can observe much faster transitions and can implement similar analysis through software. Step Islands is useful only where automatic slow-position collection and a small local graph notebook are easier to operate than those alternatives. Reusing jayis1's existing acquisition, serial and passive-carrier platform is intentional; this is not a new processor board disguised as a novel sensor.

## Hardware specifications

The controller is a separately purchased ST NUCLEO-F401RE, MB1136 family, containing an STM32F401RET6 Cortex-M4 with 512 KiB flash and 96 KiB SRAM. The firmware establishes the nominal 16 MHz internal HSI clock. Sampling uses newly observed one-millisecond SysTick values in the foreground. USART2 transmits at 115200 baud, eight data bits, no parity, one stop bit. The onboard ST-LINK USB virtual COM bridge connects to the computer. There is no radio, cloud service, battery, charging circuit, actuator, relay, command receiver or persistent storage on the custom carrier.

The custom PCB is an 80 by 70 mm, two-layer, nominal 1.6 mm FR-4 passive contact interface. It contains six through-hole headers and eight axial resistors. The supplied KiCad schematic contains actual symbols and connected nets; the board contains assigned footprints, pad nets, routed copper, a closed outline and connector labels. The Nucleo supplies regulation, reset, MCU decoupling, oscillator support, USB bridging and debugging. Those functions are not missing from the custom board: they belong to a separate purchased assembly required by the design.

Each sensing channel has a 10 kilohm pull-up from its sense node to regulated 3.3 V and a 1 kilohm resistor from that node to a voltage-free contact. The other contact terminal connects to shared ground. A normally open low-level lever switch such as Omron D2F-01L is a candidate, subject to mechanical fit and low-current performance verification. A reed contact with an appropriate magnet can reduce mechanical loading, but magnetic thresholds and hysteresis must then be characterized. No particular sensor mounting or procurement lot has been qualified.

## Architecture and connector contract

    Four position contacts A / B / C / D
                       |
                J3 / J4 / J5 / J6
                       |
          1k series + 10k pull-up per channel
                       |
                J2 sense-node harness
                       |
          External NUCLEO-F401RE GPIO inputs
                       |
          100 ms whole-word qualification
                       |
             Retained set of sixteen words
                       |
          One-bit graph / islands / bridge candidates
                       |
           SI1 checksummed frames, USART2 TX
                       |
                ST-LINK USB virtual COM
                       |
        Local Linux Tk companion: graph table, paths,
             acquisition diagnostics, replay, CSV

Carrier J1 pin one connects to Nucleo CN6 pin four, regulated 3V3. J1 pin two connects to CN6 pin six, ground. J2 pins one through four connect respectively to CN8 pins one through four: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. The analog-labelled connector positions are used as digital inputs, not ADC channels. J3, J4, J5 and J6 correspond to POSITION_A, POSITION_B, POSITION_C and POSITION_D. Each has signal on pin one and ground on pin two. The GPIO driver converts electrical low to logical closed.

The numeric word is A + 2*B + 4*C + 8*D. The app prints the named A B C D values in that order, rather than relying on conventional most-significant-bit-first binary notation. A row labelled `3: 1 1 0 0` means A and B closed. The graph has sixteen possible vertices, numbered zero through fifteen, and a retained bitset uses bit w to indicate that vertex w was observed. Physical contact labels, not the order in which switches were installed, define the interpretation.

PA2 uses alternate function AF7 for USART2 transmit to the onboard bridge. Inspect the actual board revision and the USART solder bridges before connecting. ST UM1724 provides the MB1136 connector tables, including Table 16 for NUCLEO-F401RE; RM0368 documents STM32F401 registers; PM0214 describes the Cortex-M4 processor support. The primary manual URL is https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf. The manufacturer's endpoint could not be downloaded during this run; the indexed university-hosted copy at https://embedded.fel.cvut.cz/sites/default/files/procesory/STM32/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf corroborated the connector mapping. This does not substitute for inspecting the purchased board.

## BOM, power and physical assembly

R1, R3, R5 and R7 are 10 kilohm one-percent axial pull-ups. R2, R4, R6 and R8 are 1 kilohm one-percent axial series resistors. The selected footprints are DIN0207 with 7.62 mm lead spacing. J1 and J3 through J6 are two-position 2.54 mm vertical headers; J2 is a four-position header. The machine-readable connectivity manifest records each reference, value, footprint and pin net. Add the Nucleo board, a compatible USB cable, four suitable contacts, insulated wiring, strain relief and a nonconductive enclosure.

Nominal current through each closed channel is 3.3 V divided by 11 kilohms, or 0.30 mA. With four closed channels the carrier draws approximately 1.20 mA from that rail. The closed sense voltage is approximately 0.30 V. These are circuit calculations, not physical measurements. The computer-facing USB power budget is dominated by the Nucleo and must be measured with the actual assembly. There is no sleep implementation: the foreground loop polls acquisition and transmission continuously. Do not describe this prototype as a battery logger.

Mount the contacts on independently adjustable insulated brackets around a captive tabletop mechanism. Keep switch forces small relative to the mechanism's operating forces and avoid sharp edges, pinch points and spring-loaded projectiles. Label both ends of every cable. Use short wiring and retain photographs or sketches of each contact's physical meaning with the experiment notes. The PCB is cable-connected, not a mechanically mating Arduino shield. It has no mounting holes; use insulating edge supports rather than drilling into reviewed copper. Reset and USB should remain accessible without touching conductive joints.

Only detached voltage-free switches may be connected. Shared USB ground is not galvanic isolation. The series resistors do not provide adequate external-voltage, mains, surge or ESD protection. Never connect powered logic outputs, vehicle wiring, industrial signals, live access-control contacts or operating machinery interlocks. Remove power before wiring and verify pin orientation and resistance before reconnecting. Open cables look like open contacts; shorted cables look like closed contacts. The instrument cannot distinguish these wiring failures from real positions, so a manual channel check is required before each session.

## Firmware and acquisition decisions

Firmware is freestanding C11 with a target linker script and reset vector. main.c establishes the clock, initializes GPIO and UART, checks the pin configuration, starts the watchdog, samples new ticks and emits periodic status. board.h states the external wiring contract. registers.h contains explicit MMIO definitions. The drivers implement clock recovery, GPIO reads, watchdog support and bounded nonblocking serial transmission. startup.c initializes data and BSS and supplies the required memory routines. No vendor SDK, heap allocation or operating system is needed.

The platform is explicitly imported from jayis1's Mixing Gap at commit 64d950ff8168329354bdedbb41f35b2ef12007fd. The new analysis, interpretation, path interface and associated tests implement Step Islands. The one-time import script refuses to overwrite existing outputs and is provenance documentation, not a regeneration command for the completed device. Normal builds do not run it. The CAD generator is separately reproducible and does rebuild the authored electrical artifacts.

The first acquisition establishes an endpoint without observation credit. A word qualifies after one hundred unchanged one-millisecond intervals, requiring that endpoint plus one hundred later samples. Qualification adds a visit and retains the word. Continuing to hold adds qualified dwell but not more visits. Any raw change resets qualification, including a short glitch that never becomes a retained word. The interval completing qualification remains unknown; it is not retroactively credited as settled dwell. These conservative rules avoid inventing an observed configuration from separately debounced bits.

Skipped ticks increment gap and missed-sample diagnostics and restart qualification. History remains retained, but continuity across the hole is not asserted. Every acquired endpoint after the initial seed belongs to an unknown interval, a qualified interval or a gap endpoint. Counter updates are transactional so exhaustion cannot publish half a visit. Terminal faults freeze acquisition evidence while status time and transmission-error diagnostics can continue. The fault codes are zero normal, one configuration failure, two watchdog startup failure, three invalid raw input and four counter exhaustion. Reset begins an empty volatile history.

## Graph semantics and algorithms

Let H be the retained set. Vertices are precisely the words in H. An undirected edge joins a and b only when the population count of a XOR b is one. Connected components are called islands. The lowest numeric member labels each island in the app. Empty history has zero islands; one observed word has one island but no demonstrated movement. New observations can create a new island or merge several islands, so the island count is not a monotonic progress score. Adding words cannot destroy a previously existing graph path.

A bridge candidate is an unobserved word adjacent to at least two distinct current islands. Two neighbors in the same island are insufficient. Observing that candidate would merge those islands in the abstract graph, but its physical feasibility remains unknown. Absence of any bridge candidate does not imply that the islands cannot be joined by several additional observations. For example, zero and fifteen require a chain of intermediate words; no single missing word touches both. This distinction prevents an empty candidate list from being mistaken for exhaustive investigation.

Firmware component labelling uses bounded flood expansion over sixteen vertices. Bridge analysis collects neighboring component labels and tests whether at least two differ. The path routine uses breadth-first search, visits neighbors in ascending numeric order and returns one deterministic shortest witness. Returned length includes both endpoints; a same-vertex observed query returns one vertex. Zero length means one or both endpoints are absent or no retained-set path exists. The app independently recomputes graph summaries rather than trusting the transmitted count and mask.

A separate generic next-setting suggestion minimizes the number of changed bits from the current raw word among all unobserved configurations, breaking ties numerically. Sentinel sixteen means every configuration has been observed. This suggestion is not specifically a bridge search, does not optimize a whole exploration session and may propose an unreachable word. The table explicitly marks bridge candidates independently of the generic suggestion. No screen commands motion or interprets a missing word as a mechanical fault.

## Protocol and companion application

SI1 frames contain eighteen unsigned decimal fields in this order: now, samples, gaps, missed, tx_errors, raw, hold, credited, seeded, fault, visits, settings, coverage, qualified_ms, unknown_ms, suggestion, components and bridges. The body begins with SI1 followed by comma-separated fields. An asterisk, four uppercase hexadecimal checksum digits and newline terminate the record. CRC-16/CCITT-FALSE uses initial value 65535 and polynomial 0x1021 over the body before the asterisk. It detects corruption but does not authenticate the sender. The interface is transmit-only from the device.

settings is the retained sixteen-bit set; coverage repeats it for compatibility with the shared acquisition validator. components is the number of islands, not a bitmask. bridges is a sixteen-bit candidate set, not a count. The companion verifies those summaries against settings and checks acquisition conservation and bounded transitions. A recording from another sibling device is rejected by its different protocol marker. Sparse validation is a set of necessary consistency bounds, not reconstruction of every unreported acquisition or proof of a physical experiment.

Run `python3 step-islands/app/main.py` on Linux with Tk installed. The Configuration islands screen lists all sixteen words, their named contact values, observation status, island roots and observed one-bit neighbors. The Candidate paths screen accepts start and end words and shows a shortest candidate witness or an explicit no-retained-path result. The Acquisition / limits screen displays telemetry and limitations. Connect opens the chosen serial port; Disconnect preserves retained evidence. A failed replacement connection does not erase an earlier session, and a second connection is refused while one port is owned.

Replay is available while disconnected. It accepts regular files up to two MiB and rejects FIFOs, oversized files, malformed decimals, invalid checksums, contradictory summaries, impossible bounded transitions, empty recordings and partial tails. Live parsing resynchronizes on the complete SI1 marker. Acquisition freshness is tracked separately from receiving bytes. A two-second progress pause or excessive cumulative host/device clock disagreement closes the link and requires explicit reconnect. There is no persistent boot identity, so some resets remain indistinguishable from forward traffic. The fixed drift allowance can conservatively terminate long sessions with an uncalibrated oscillator.

CSV export exclusively creates a new file with private permissions and refuses overwrite. It includes author jayis1, provenance, connection state, warnings, parser diagnostics, progress age, total accepted rows and the latest two thousand retained snapshots. Exported settings suffice to reconstruct the graph and candidate paths; the current path query is a view, not a measured event. Synthetic fixtures remain synthetic even when all checks pass. Save before resetting or altering the mechanism, and never pool old and new fixture conditions without an explicit separate analysis.

## Build, validation and commissioning

From the repository root run `make -C step-islands/firmware clean all test fixture`. This builds the ARM ELF and binary, runs sanitized C acquisition, graph, clock and serial tests, and emits a synthetic recording. Run `xvfb-run -a python3 -m unittest discover -s step-islands/tests -p 'test_app.py' -v` for the companion tests. These exercise every retained-set graph through an independent oracle, real C-encoder records, replay, screens, paths, export, serial restoration, stale acquisition and PTY disconnection. A desktop with an available display can omit xvfb-run.

Generate hardware using `/usr/bin/python3 step-islands/tools/generate_hardware.py` with KiCad 9 Python bindings and installed footprint libraries. Validate with `/usr/bin/python3 step-islands/tools/check_hardware.py`. The checker runs native ERC and DRC, exports a fresh schematic netlist and compares every reference, value, footprint and pin net with actual PCB pads and the manifest. Passive-only ERC does not validate the external processor board or cable assembly. Build outputs and KiCad local preferences are ignored. Exact results and remaining limitations are recorded in VALIDATION.md.

For eventual commissioning, inspect the unpowered assembly, measure for supply shorts, verify resistor values and check the connector harness independently. Program through ST-LINK only after confirming the target and power arrangement; the optional Makefile flash target uses OpenOCD. Test all sixteen switch words with a known fixture, verify node voltages and compare timing with an independent reference. Disconnect a contact and deliberately introduce chatter to understand the diagnostic boundary. Measure worst-case foreground acquisition latency during graph telemetry before relying on the nominal sample interval.

Finally, conduct a small supervised exploration with synchronized manual notes or video. Verify that the contacts do not disturb the mechanism and that identical words do not hide materially different internal states. Treat graph paths as hypotheses to examine manually, never instructions to push through resistance. The complete source package supports that evaluation; it does not claim that a physical unit has already passed it.
