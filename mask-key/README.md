# Mask Key — counterexample-guided mechanical sensor-subset workbench

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original project sources are MIT licensed; see LICENSE. Purchased components and installed KiCad footprint geometry retain their respective manufacturers' specifications and upstream licensing. Revision A is an unbuilt engineering prototype, not a production instrument. Target builds, synthetic acquisition experiments, desktop application tests and native CAD checks are reproducible. Fabrication, target programming, electrical bring-up, mechanical integration and practical benefit have not been physically verified.

## Purpose and overview

Mask Key helps a designer ask which observed mechanical positions are actually needed to distinguish a binary response in a small hand-operated demonstrator. Three voltage-free contacts witness control positions A, B and C. A fourth witnesses response Y. The instrument retains qualified four-bit observations, tests all eight possible subsets of the three controls, and displays an explicit counterexample for every subset that cannot explain the retained responses. It also identifies all inclusion-minimal subsets that remain compatible with the evidence.

Consider an educational puzzle box with three levers and a visible latch flag. The builder suspects that the third lever has no role in the latch outcome, but a few successful openings are not enough to tell. Mask Key observes the lever contacts and the flag contact while the builder explores combinations. If two observations have the same A and B but different Y, the proposal that A and B alone determine the flag is refuted. The exact retained words are shown so the builder can inspect the lever fixture, contact mounting, friction or a genuinely omitted mechanical state. A later convenient observation cannot erase that inconvenient pair.

This is a tool for experimental explanation, not an automatic sensor-removal adviser. Compatible means only that no retained pair contradicts a memoryless explanation using those selected controls. A compatible subset can fail on an unseen combination, a later visit, a different approach direction or a condition that none of the contacts measures. The app therefore calls minimal results provisional and explicitly warns against removing a sensor on this evidence. The instrument never energizes a mechanism or authorizes a safety action.

Target users include mechanical-computing educators, museum exhibit builders, puzzle designers and makers developing low-energy demonstration fixtures. Useful questions include whether an apparent dummy lever can still matter, whether two redundant-looking witnesses can be separated experimentally, and whether three binary witnesses are insufficient even together. The small fixed hypothesis space makes explanations inspectable: there are no learned weights, opaque confidence scores or cloud services.

## Originality and comparison with the repository

The repository was pulled before work, its device directories were listed, and the existing README titles were inventoried. The closest purposes were read rather than comparing names alone. Truth Sieve retains a two-input/two-output relation and asks whether a deterministic map exists for those fixed inputs. Pair Cover observes four controls without a response, tracks pairwise setting coverage and suggests missing combinations. Label Veil compares a person's declared class against a concealed sample label. None of those devices evaluates the entire lattice of candidate three-control subsets against a separately witnessed binary response or explains each eliminated subset with a retained observation pair.

Mask Key intentionally reuses jayis1's tested passive carrier and low-level Nucleo infrastructure. The differentiating contribution is the sensor-subset investigation workflow: explicit omitted-variable hypotheses, inclusion-minimal compatible explanations, persistent counterexamples, and next-full-setting guidance. Functional dependencies, Boolean minimization and experimental design are established subjects; neither the mathematics nor the resistor network is claimed as a new scientific discovery. Repository differentiation is documented, but worldwide novelty, patentability and freedom to operate are unverified. A general-purpose logic analyzer and a custom script could implement similar analysis.

A paper notebook remains a sensible alternative for a single short experiment. A camera records richer physical context and approach direction. A faster logic analyzer is preferable for transient electrical behavior. Mask Key is potentially useful where automatically keeping stable physical observations and their contradictions reduces transcription and interpretation errors. That benefit is a prototype hypothesis requiring user trials, not a measured result.

## Hardware specifications and assembly boundary

The processor assembly is an external ST NUCLEO-F401RE MB1136 containing an STM32F401RET6 Cortex-M4, 512 KiB flash and 96 KiB SRAM. It supplies processor decoupling, voltage regulation, reset, ST-LINK programming and the USB virtual serial bridge. The firmware establishes the nominal internal 16 MHz HSI clock and samples digital inputs at observed one-millisecond ticks. It uses USART2 transmit-only at 115200 baud, eight data bits, no parity and one stop bit. There is no radio, battery, charger, motor output, relay or application command receiver.

The supplied custom board is a cable-connected passive interface, not a standalone MCU PCB and not a mechanically mating Arduino shield. Its outline is 80 by 70 mm with two copper layers and nominal 1.6 mm FR-4 thickness. It carries six through-hole headers and eight axial resistors. Each of four independent channels has a 10 kΩ pull-up from the sense node to regulated 3.3 V and a 1 kΩ resistor between that node and the contact signal. Contact closure joins the external end to common ground. The PCB includes real footprints, routed tracks, a ground via, silkscreen legends and a closed outline.

Use four maintained low-level switches or reed contacts that are electrically disconnected from all other circuitry. A low-level lever switch such as Omron D2F-01L is a candidate part for a hand-operated fixture, not a physically qualified purchasing recommendation. Three switches must describe the selected lever positions and the fourth must describe the response flag. Contact force, hysteresis, mounting compliance and cable motion can all change the mechanism being observed. Position the witnesses so the fixture remains safely captive, and evaluate whether their actuation loads alter the experiment.

The mechanical assembly is application-specific. This revision supplies no certified enclosure or fabrication-ready lever brackets. Use an insulating enclosure, support the PCB without stressing solder joints, provide cable strain relief and retain access to USB and reset. There are no mounting holes in the carrier; choose an insulating edge support or revise the layout and re-run mechanical and electrical checks before drilling. Do not place exposed circuitry where conductive swarf or loose linkage parts can bridge it.

## Architecture and electrical contract

    Hand-operated control positions A, B, C -> J3, J4, J5
    Mechanically witnessed response Y -----> J6
                         |
                 four passive channels
                    10k pull-up / 1k series
                         |
                      J2 GPIO
                         |
                 NUCLEO-F401RE
                whole-word qualification
                         |
                retained observation set
                         |
              eight subset counterexample tests
                         |
           compatible and inclusion-minimal masks
                         |
                 MK1 + CRC over USART2
                         |
                   ST-LINK USB VCP
                         |
        Linux desktop: subsets / diagnostics / CSV

J1 pin 1 is VCC, connected only to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 is ground, connected to CN6 pin 6. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled connector positions are configured as digital inputs. J3 is CONTROL_A, J4 CONTROL_B, J5 CONTROL_C and J6 RESPONSE_Y. All contact connectors have signal on pin 1 and ground on pin 2. RESPONSE_Y is an observed input, never an output driven by this PCB.

The raw logical word is A + 2*B + 4*C + 8*Y, with one meaning a closed contact. GPIO voltage is active low, so the driver inverts the four electrical levels. A printed A B C sequence lists the named controls in that order, not conventional most-significant-bit-first binary notation. Numeric input one therefore means A=1, B=0, C=0. A counterexample shown as raw words 0 and 8 has identical open controls but opposite response, and refutes every subset including the full A+B+C set.

PA2 is USART2 transmit with alternate function AF7. The firmware disables the receiver. Verify the purchased MB1136 revision's ST-LINK virtual-COM routing, especially SB13/SB14 and SB62/SB63, before connecting. Relevant manufacturer documents are ST UM1724 for board connectors and bridges, RM0368 for STM32F401 peripheral registers, and PM0214 for Cortex-M4 exceptions and SysTick. These platform choices are inherited from the pinned repository implementation; this revision does not claim a new manufacturer-document audit or measured pin timing.

## Bill of materials, power and safe limits

R1, R3, R5 and R7 are 10 kΩ one-percent axial pull-ups. R2, R4, R6 and R8 are 1 kΩ one-percent axial series resistors. The footprints use DIN0207 bodies and 7.62 mm lead spacing. J1 and J3 through J6 use two-position 2.54 mm vertical headers, and J2 uses a four-position header of the same pitch. Add the NUCLEO-F401RE assembly, its compatible USB cable, four contacts, short insulated leads and an insulating enclosure. The connectivity manifest is the on-carrier assembly list; it deliberately does not pretend to be a complete mechanical procurement package.

At nominal values, one closed channel draws 3.3 V divided by 11 kΩ, approximately 0.30 mA, and its sense node is approximately 0.30 V. Four closed channels draw approximately 1.20 mA and 3.96 mW from the regulated rail. These calculated loads exclude the development board and debugger, which dominate total USB demand. Measure assembled consumption and input voltages before a first experiment. The small contact excitation may not penetrate oxide on switches intended for higher wetting current, so an apparent open state can be a contact-quality problem rather than mechanical evidence.

The series resistors do not provide isolation or adequate external-voltage protection. All returns share USB ground. There is no reverse-polarity protection, transient suppressor or galvanic barrier on this carrier. Never connect mains, a battery-powered contact circuit, a powered logic output, a machine interlock or a sensor with its own supply. Wire and inspect with power removed, verify resistance between the supply rails and check connector orientation before applying only the Nucleo's regulated 3.3 V. Use this in a dry indoor low-energy bench environment, not medical, industrial protection or access-control service.

## Firmware and evidence model

The firmware is freestanding C11, built with GNU Arm Embedded tools and warnings treated as errors. startup.c defines reset vectors, copies initialized data and clears BSS. linker.ld maps flash and SRAM for the selected MCU and reserves stack headroom. registers.h contains the peripheral addresses used by the board, clock and serial drivers. main.c initializes the interface, checks input configuration, starts the independent watchdog, samples each newly observed tick and attempts periodic telemetry without waiting for an entire frame to leave the UART.

The first sample establishes a raw endpoint and earns no visit. A complete word qualifies after one hundred consecutive unchanged one-millisecond intervals. This requires an initial endpoint and one hundred later samples. A raw change, including a short disturbance that never qualifies, restarts the window. Qualification records one visit and retains the word. Continued holding earns qualified interval time without new visits. Returning to a prior word after a fresh qualification can increase visits, but cannot duplicate a set member or erase contradictory evidence.

If the main loop skips ticks, the engine records a gap and the number of missed ticks, clears current qualification and requires a complete new window. It does not interpolate across absent observations. Historical words survive. Every acquired endpoint after seeding is accounted as an unknown interval, a qualified interval or a gap endpoint. Updates are staged transactionally: counter exhaustion enters a terminal fault without committing half an observation. The first terminal cause is retained. Timestamps and transport diagnostics can continue after terminal failure, but new experimental evidence cannot.

A subset mask uses bits zero, one and two to select A, B and C. Mask zero means a constant-response explanation with no controls. For each mask, firmware searches retained words for a pair with unequal Y but equal selected control bits. Finding such a pair refutes the subset. With no pair, that subset remains compatible. The full subset can be refuted if repeated identical controls produced both responses. Inclusion-minimal compatible subsets have no compatible proper subset. This is not the same as selecting only the fewest-bit subsets: incomparable explanations of different sizes can both be inclusion-minimal.

An empty observation history leaves all subsets formally compatible, with the empty subset formally minimal. The app labels every row NO EVIDENCE rather than interpreting this as a discovered constant mechanism. Constant observed responses can leave the empty subset provisional even after several different controls were tried. Guidance chooses an unobserved full three-control setting requiring the fewest changes from the current controls, breaking ties by numeric input. It never prescribes the response. After all eight control words have been observed, guidance asks for repeated trials and inspection instead of announcing proof.

The hundred-millisecond qualification convention does not establish physical settlement. A mechanism can pause long enough to qualify and later move. Whole-word qualification can also be postponed indefinitely by chatter on any channel. This is appropriate for a stable-configuration notebook, but unsuitable for measuring independent response latency. All evidence is volatile and lost on reset. The instrument cannot identify which hidden physical variable matters, distinguish causation from correlation, or authenticate that two observations came from the same unchanged assembly.

## Protocol and companion application

MK1 telemetry contains eighteen unsigned decimal fields in this exact order: now, samples, gaps, missed, tx_errors, raw, hold, credited, seeded, fault, visits, settings, coverage, qualified_ms, unknown_ms, suggestion, compatible and minimal. The prefix and comma-separated body are followed by an asterisk, four uppercase hexadecimal CRC digits and newline. CRC-16/CCITT-FALSE uses initial value 65535 and polynomial 0x1021 over the body including MK1. The conventional check string 123456789 yields 0x29B1. CRC detects corruption; it does not authenticate an instrument or operator.

settings is a sixteen-bit set whose bit w means raw word w was retained. coverage is the same evidence rearranged into eight two-bit response rows: bit 2*input+output. compatible and minimal are each eight-bit sets of subset masks; bit m refers to mask m, not to control m. suggestion is a full input from zero through seven, or sentinel eight when every full input has a response. Fault codes are zero for normal, one for configuration failure, two for watchdog setup failure, three for invalid input and four for counter exhaustion. The app recomputes the relation projection, subset results and guidance instead of trusting those transmitted summaries.

Run `python3 mask-key/app/main.py` on Linux with Tk available. Enter the serial path, normally /dev/ttyACM0, then Connect. The Sensor subsets / counterexamples screen shows each selected-control subset, a retained raw-word pair when refuted, and its provisional or refuted state. The Acquisition / limits screen displays every protocol field and the raw-word decoding rule. Disconnect preserves the current evidence. Reconnecting starts a separate host session. A failed port open preserves the old evidence, and a second simultaneous connection is refused. Serial permissions must be configured through the host's normal device-access policy; the app does not request elevated privileges.

Replay is available only while disconnected and accepts a regular MK1 file of at most two MiB. It rejects empty recordings, malformed fields, invalid checksums, impossible summaries and partial trailing frames. Live decoding supports late attachment by counting discarded fragments and resynchronizing at a complete marker. The app detects counter rollback, disappearing observations, impossible sparse acquisition budgets, stale acquisition and excessive host/device progress discrepancy. These are necessary consistency checks, not reconstruction of every hidden sample. Without persistent boot identity, some resets can resemble plausible forward traffic; reconnect explicitly after resetting the instrument.

CSV export uses exclusive creation with private permissions, refuses overwriting an existing path, and includes jayis1 attribution, provenance, connection state, warnings, parser diagnostics and the retained snapshots. The latest two thousand accepted rows are kept, while the total accepted count is separately recorded. This is a bounded notebook rather than an indefinite logging system. Files and serial recordings should be treated as untrusted input, and recordings must not be presented as physical measurements when generated by the synthetic fixture.

## Build, verification and first physical trial

From the repository root, run `make -C mask-key/firmware clean all test fixture`. This links the actual Cortex-M4 firmware, creates its binary, runs sanitizer-enabled host tests and generates a synthetic MK1 recording from the real C encoder. The exhaustive analysis test compares every possible retained observation set with an independently structured bucket oracle, checks inclusion-minimal results and counterexample validity, and exercises every raw-word pair through acquisition. Additional tests cover qualification boundaries, gaps, overflow, timer wrap, CRC, clock failure paths and UART timeouts.

Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s mask-key/tests -p 'test_*.py'` for the companion's model, transport and real Tk event-loop tests. Tests exercise the C-generated XOR study followed by a contradictory response, serial pseudo-terminals, disconnection, reconnect, denied opens, replay rejection and CSV export. Run `/usr/bin/python3 mask-key/tools/check_hardware.py` for native KiCad ERC and DRC plus fresh schematic-netlist comparison against every PCB pad, component value, footprint identity and manifest net. The generator requires distribution Python with pcbnew; ordinary application operation does not.

The Makefile flash target invokes OpenOCD through ST-LINK, but flashing has not been performed here. Before physical use, inspect assembly and polarity, measure all open and closed input levels, establish the connector map by continuity, and confirm that contact closure is correctly represented in telemetry. Then use a deliberately simple known mechanism or disconnected switch fixture. Demonstrate a constant response, a response depending on A alone, and a response depending on A and B while C varies. Finally introduce both responses for identical controls and confirm that every subset is refuted without losing earlier evidence.

Compare timing against an independent instrument, test reset and USB removal, and investigate any difference between predicted and observed behavior before interpreting a real mechanism. Repeat control settings in different orders and from different approach directions. Document the physical fixture, its contact mounting and any changed conditions alongside exports. A passing build and clean CAD rules do not establish electrical safety, enclosure fit, switching repeatability, EMC compliance or useful experimental speed. See VALIDATION.md for the actual software and CAD results and their limits.
