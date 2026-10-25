# Pedal Window — two-key sustain-change timing notebook

Author and creator: jayis1
Copyright (c) 2026 jayis1. Original software and locally authored design sources: MIT.

Revision A is an engineering prototype, not an acoustically calibrated product. The custom PCB is a fully routed passive contact interface used with a separate NUCLEO-F401RE. The processor, regulator, decoupling, debugger and USB bridge are on that development board, not hidden inside the carrier schematic. No physical assembly, target flashing, piano installation, electrical measurements or teaching trial has been performed for this revision.

## Purpose and primary user

Pedal Window is a removable teaching fixture for a piano teacher working on the timing of a single sustain-pedal change between two selected keys. It observes four physical contacts: key A travel, key B travel, pedal fully released, and pedal depressed to a chosen endpoint. It records the gap or overlap between the keys, whether pedal release happened before or after the new key, and the delay until the pedal was depressed again. The intended exercise is deliberately narrower than playing a complete score. A teacher can repeat the same two-note transfer, inspect the intervals, adjust an instruction, and compare the next attempts without filming the student's feet and hands.

The current workaround is listening while watching the foot, slowing a video recording, or practicing on an electronic piano that exposes MIDI. Listening remains necessary: contact timing does not determine the quality of a musical phrase. The fixture adds a small piece of otherwise difficult evidence, particularly on an acoustic instrument without MIDI. It does not label a performance correct or incorrect. It shows which mechanical events were observed and which orderings are unresolved.

A phone application alone cannot reliably observe two independently selected key positions and both pedal endpoints without cameras, suitable sight lines and image interpretation. Dedicated contacts provide a common clock, can operate without recording a person, and make sensor placement explicit. They also expose their own limitations: a badly placed switch gives a repeatable but musically irrelevant threshold. This is a measurement notebook for a supervised exercise, not an automatic piano teacher.

## Originality, related work and scope

Repository discovery covered the existing device directory inventory and README titles/purpose summaries before implementation. The nearest internal designs are Synthand, which converts muscle and finger movement into musical controls; Swing Envelope, which measures pendulum crossings; and Stencil Orbit, which associates manual printing-tool episodes with carousel stations. None implements the two-key sustain-change exercise, signed key/pedal offsets, paired pedal endpoint plausibility, or the lesson notebook described here. The contact carrier and bare-metal board support deliberately reuse a proven electrical pattern rather than claiming that a resistor network is an invention.

Piano pedaling measurement already exists. The NIME 2017 paper “Piano Pedaller: A Measurement System for Classification and Visualisation of Piano Pedalling Techniques” describes optical pedal sensing, audio capture and technique classification. MIDI visualizers can display sustain messages and note activity on compatible instruments. Pedal Window's proposed distinction is a removable, no-audio, four-contact exercise fixture that explicitly records both key release/press overlap and a complete pedal endpoint cycle, with rejected incomplete trials retained alongside completed ones. This is an original implementation and combination new to this repository, not a patent search or a claim that no similar device has ever existed.

The niche is small enough that a configurable research instrument is more plausible than a mass-market product. Inexpensive development boards and a local desktop interface make the fixture accessible without a custom radio or cloud service. A full optical keyboard scanner, an audio-based score follower, and a generic MIDI practice application were rejected because they solve much larger problems and obscure the narrowly testable timing question. A pressure-only pedal pad was rejected because foot load is not the same observation as reaching a pedal endpoint.

## Hardware specifications and architecture

| Item | Revision A choice |
| --- | --- |
| Processor | STM32F401RET6 on NUCLEO-F401RE, Cortex-M4 |
| Clock | Reset HSI, nominal 16 MHz; no PLL or external oscillator dependency |
| Memory map | 512 KiB flash, 96 KiB SRAM |
| Sensors | Four externally mounted, isolated normally-open dry contacts |
| Input network | Four 10 kohm pull-ups and four 1 kohm series resistors |
| Connectivity | USART2 TX, 115200 baud, 8N1, through ST-LINK virtual COM |
| Power | USB through Nucleo; carrier supplied only from its 3.3 V rail |
| Carrier | 80 x 70 mm, two copper layers, 1.6 mm nominal thickness |
| Layout | Through-hole headers and axial resistors, 0.3 mm tracks |
| Desktop | Python 3 with Tk on Linux; no pip dependencies |

Architecture:

    Key A contact -------- J3 -- 1k -- SENSE0 -- PA0 / A0
    Key B contact -------- J4 -- 1k -- SENSE1 -- PA1 / A1
    Pedal REST contact --- J5 -- 1k -- SENSE2 -- PA4 / A2
    Pedal DOWN contact --- J6 -- 1k -- SENSE3 -- PB0 / A3
                                      |
                                each pulled up
                                through 10k to 3V3
                                      |
    USB -> NUCLEO-F401RE -> 1 ms sampling -> contact qualification
                            -> trial state machine -> CRC framed UART
                            -> ST-LINK VCP -> desktop lesson notebook

Each external contact closes its CONTACT net to ground. J1 pin 1 connects to Nucleo CN6 pin 4, +3.3 V; J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4, A0 through A3. J3 through J6 pin 1 is the associated contact signal; pin 2 is ground. Never use the 5 V pin for J1. The MCU sees a low level when a contact closes, and firmware inverts that reading so a one bit means closed.

The custom board contains fourteen components and ten named nets. Its schematic contains real, locally authored header and resistor symbols, numbered pins, wires and shared net labels. Its PCB contains matching library footprints, pad nets, routed copper, vias and an Edge.Cuts outline. The generation script and pin-level connectivity manifest are included. The MCU is an external assembly connected by J1 and J2; the carrier is not advertised as a complete standalone controller.

The Nucleo's factory virtual-COM configuration is required: SB13 and SB14 fitted, SB62 and SB63 open. PA2 uses alternate function AF7 for USART2 transmit. Receive is disabled, so a desktop process cannot change timing policy or trigger a trial by sending bytes. SWD and the onboard ST-LINK provide the programming path. The carrier requires neither a second USB connector nor duplicate MCU reset and decoupling circuits.

## Contact mounting and calibration

Use four low-current signal contacts suitable for approximately 0.3 mA wetting current, not oxidized high-current power switches. Miniature reed contacts with securely retained magnets are one option; gold-plated signal switches are another. They are external sensors, not included in the PCB BOM. The two pedal contacts must have distinct mounting locations: REST closes only near the released endpoint, DOWN only near the chosen depressed endpoint, and both remain open during intermediate travel. Simultaneous closure is a contradiction, not half-pedal information.

For initial experiments, use a separate two-key and pedal mock-up before attaching anything to an instrument. On an actual piano, a technician should select removable fixtures that do not interfere with the action, change key resistance, abrade finishes or enter moving mechanisms. Do not drill the instrument, glue to finished surfaces, or attach loose magnets that could fall into the action. Key contacts represent selected travel thresholds, not hammer strikes. Pedal contacts represent the chosen geometry, not necessarily damper engagement or disengagement.

Before every lesson, observe the live contact display while independently moving each sensor. Confirm that key A does not change key B, that pedal REST and DOWN never overlap, and that releasing all controls produces only REST closed. Move slowly, then repeat at the intended practice speed. If the displayed sequence differs from the observed motion, correct the fixture before collecting trials. Keep an installation photograph and threshold notes outside this repository with the lesson recording if comparisons across days matter.

Use short twisted signal/ground pairs, initially no longer than half a meter, and keep them away from USB power converters and foot traffic. This cable length is an initial test constraint, not a verified immunity rating. The design has no galvanic isolation, external surge suppression or certified ESD protection. The resistors limit ordinary contact current but do not make the inputs safe for powered equipment. Never connect an electronic piano's existing switch matrix, pedal jack or MIDI output directly to these headers.

## Power, feasibility and approximate cost

At nominal 3.3 V, a closed contact draws 3.3 V divided by 11 kohm, approximately 0.300 mA. Its MCU sense voltage is approximately 0.300 V because the series resistor forms a divider with the pull-up. Four closed contacts would draw 1.200 mA, although both pedal endpoints closed is an invalid configuration. Carrier resistor dissipation is small; a normal open contact consumes essentially no DC current apart from leakage. Confirm logic levels and leakage on the actual board before relying on them.

| Budget item | Planning value | Qualification |
| --- | --- | --- |
| Carrier, all contacts open | Near zero DC load | Excludes leakage and cables |
| Carrier, four contacts closed | 1.2 mA at 3.3 V | Calculated resistor-network value |
| Nucleo plus ST-LINK and LEDs | Reserve 100 mA at 5 V | Conservative design allowance, not a measurement |
| USB supply | Regulated 5 V source with at least 500 mA capacity | Use the board's documented USB supply arrangement |

The firmware continuously services acquisition and UART transmission; it is not a low-power sleeping design. There is no rechargeable battery, charger or runtime claim. USB power loss erases volatile trials. The desktop retains only the evidence already received and exported. Leaving out a battery avoids charging hazards and makes a supervised bench lesson the intended operating environment.

A small-quantity planning BOM is roughly USD 25 for the Nucleo, USD 8 for a simple carrier PCB, USD 4 for resistors and headers, USD 12 for four suitable contacts, and USD 15 for cables and adjustable brackets: about USD 64 before shipping, taxes and assembly. These are estimates rather than checked supplier quotations. A future assembled teaching kit might require a price near USD 120 to cover mounting hardware, assembly and support, but willingness to pay has not been established. The brackets, rather than processor performance, are likely to dominate practical usability and development cost.

## Firmware behavior and design decisions

The portable engine has three phases. WAIT_BASELINE requires both keys released and pedal REST closed. READY allows the teacher or student to press key A and depress the pedal in either order; key B must remain released. Once the qualified vector is exactly A plus DOWN, a five-second capture begins. The engine records the first A release, the first B press, the first subsequent pedal REST arrival, and the subsequent pedal DOWN arrival. A complete trial requires A released, B still held, and the full pedal endpoint cycle observed. Pressing B before releasing A is allowed and produces a negative key gap.

Returning DOWN before the new key is established rejects the attempt. Re-pressing A after its release, releasing B before completion, contradictory pedal endpoints, an acquisition gap, or a five-second timeout also reject it. Rejected trials have zero interval fields; those zeros mean unavailable, not simultaneous. Every completion and rejection requires a baseline before another trial. If a failure already leaves the fixture at baseline, the engine recovers without requiring an artificial extra key movement.

Each input has an independent eight-millisecond debounce timer. An unrelated pedal transition cannot postpone qualification of a key edge. Qualified transitions are timestamped at acceptance, not retroactively at the first raw contact movement. For clean edges with equal debounce latency, interval differences cancel the common delay; asymmetric bounce, switch hysteresis and mechanical thresholds still affect accuracy. Events accepted in the same sample receive a simultaneous flag and cannot be ordered more finely than the acquisition interval. Sub-eight-millisecond pulses are deliberately rejected.

Acquisition uses a nominal one-millisecond SysTick. A gap greater than four milliseconds invalidates the active capture and restarts qualification without instantly accepting the first post-gap sample. Unsigned time subtraction handles ordinary 32-bit millisecond rollover. Trial-relative offsets remain bounded by five seconds, so signed interval arithmetic does not depend on converting wrapped absolute timestamps. Counters saturate instead of wrapping; restart a session long before exhausting their 32-bit range.

The board driver configures only the required GPIO and USART2 registers. Startup initializes data and BSS without a vendor runtime. The linker reserves a stack margin and maps the exact target memory. The independent watchdog is fed only after acquisition processing. Its oscillator and update waits have bounded timeouts; failure to initialize it leaves the unit silent rather than collecting apparently healthy trials. No configuration is written to flash, so there is no interrupted-settings-write recovery problem. Firmware replacement is a local SWD operation, not an unsigned network update.

UART output is nonblocking with a bounded buffer and a 100 ms transmit deadline. Transmission failures increment a diagnostic counter. Snapshots are sent on phase, level and result changes and at least every 100 ms when transmission works. Only the last trial result is carried in a snapshot, so the device is not an archival recorder. If the host misses multiple trials, result identifiers reveal missing evidence but cannot reconstruct it.

## Protocol and application interface

Every frame is ASCII, newline terminated, at most 256 bytes:

    PW1,<18 comma-separated decimal fields>*<four uppercase CRC hex digits>LF

Field order is `seq,ms,flags,levels,phase,attempts,completed,rejected,gaps,transport_losses,id,outcome,result_flags,key_gap,release_delta,down_delta,debounce,trial_limit`. CRC is CRC-16/CCITT-FALSE: polynomial 0x1021, initial 0xffff, no reflection, no final XOR, covering `PW1` through the final decimal digit before the asterisk. There is no NUL terminator on the wire. The shared fixture generator exercises this exact C encoder, not a second handwritten protocol implementation.

Levels bits zero through three are A, B, REST and DOWN. Phase values are zero for WAIT_BASELINE, one for READY and two for CAPTURE. Outcome values are zero for no result, one for complete and two for rejected. Flags are 1 simultaneous observations, 2 contradictory endpoints, 4 timeout, 8 acquisition gap, 16 B released too soon, 32 reattack or premature pedal return, and 64 inputs not yet qualified. Current flags and retained result flags are separate: returning to baseline must not erase the reason an earlier trial failed.

The signed intervals are milliseconds. `key_gap = B_press - A_release`; a positive result denotes a key gap and a negative result denotes overlap. `release_delta = REST_arrival - B_press`; positive means pedal release after B and negative means before B. `down_delta = DOWN_return - B_press`. The policy fields repeat 8 and 5000 on every frame, so a late-attaching application can reject incompatible firmware without requiring a one-shot boot announcement.

The desktop has three real screens: Live Exercise shows qualified contacts, phase and diagnostic counters; Trial Notebook retains up to 256 observed results with signed intervals and flags; Mounting and Interpretation explains the fixture and the meaning of its measurements. Connection uses a user-selected Linux serial path. Replay reads a bounded local recording and is visibly labeled offline. Export writes an exclusive-create, private CSV containing author, provenance, connection/freshness state, parser errors, missing-evidence count and the retained trial rows. Existing exports are not overwritten.

Malformed CRCs, lengths, field ranges, policy values and inconsistent result semantics are rejected. A complete protocol marker can resynchronize after a truncated frame. Sequence and uptime regression, or a cumulative counter rollback, blocks the session pending an explicit reconnect. These are reset heuristics, not a persistent cryptographic boot identifier: a reset whose new values all appear forward can be indistinguishable. Duplicate frames do not refresh freshness. Missing-evidence counts combine frame and result gaps and are not a count of distinct lost trials. Data older than one second is marked stale; disconnect preserves provenance and history rather than relabeling replay as live evidence.

## Build, tests and reproduction

From the repository root:

    make -C pedal-window/firmware check
    xvfb-run -a python3 -m unittest discover -s pedal-window/tests -p 'test_*.py'
    /usr/bin/python3 pedal-window/tools/check_hardware.py
    python3 pedal-window/app/app.py

Firmware requires GNU Arm Embedded GCC and Make. Host tests require GCC with address and undefined-behavior sanitizers. The application uses the standard Python library, Tk and Linux termios; a distribution may package Tk separately as `python3-tk`. Headless GUI tests use Xvfb. CAD generation and checking require KiCad 9, its standard footprint libraries and the distribution Python interpreter that can import `pcbnew`. Regenerate hardware with `/usr/bin/python3 pedal-window/tools/generate_hardware.py`; generation is not required merely to inspect or validate the committed project.

Host tests cover positive and negative intervals, simultaneous events, endpoint contradiction, timeout, gaps, baseline recovery, independent debounce, premature release, reattack, timer rollover, saturation and encoder bounds. Application tests consume frames produced by the real C engine. GUI tests use a Linux pseudoterminal and a real Tk event loop to exercise connect, replay, populated screens, disconnect, private export, overwrite refusal, stale display and clearing old summaries on reconnect. A serial-setup fault injection verifies restoration of terminal attributes. These are simulated electrical inputs, not physical piano measurements.

The hardware checker runs actual schematic ERC and PCB DRC with violation exit codes, exports a fresh XML netlist, then compares every reference, value, footprint identifier and pin/net assignment to the actual PCB pads and manifest. Passive-only ERC cannot check the external regulator, mechanical switches or Nucleo solder bridges. A clean CAD result is necessary but not sufficient for safe assembly. See VERIFICATION.md for the exact executed results and remaining limits.

## Bring-up and remaining work

Inspect the unpowered assembly first: resistor values, header orientation, solder bridges and continuity against the schematic. Confirm that VCC and ground are not shorted. Connect only USB power to the Nucleo, verify its 3.3 V rail, then attach the carrier with power removed. Check open and closed input voltages with a meter before connecting the mechanical fixture. Flash through the existing ST-LINK only after reviewing the target identity. The supplied `make flash` command uses OpenOCD; it has not been run on physical hardware for this revision.

Use isolated jumpers to reproduce the documented exercise before involving a piano. Compare UART timestamps to a logic analyzer, deliberately introduce contact chatter, unplug the host, and verify stale/disconnected behavior. Then test mounting repeatability and ask a teacher whether the displayed interval is useful. The absence of microphones protects privacy but also prevents acoustic conclusions. No medical, safety, accessibility certification, product-compliance or guaranteed educational outcome is claimed. Do not use the device as an assessment of a person's ability without their informed agreement.

## References and authorship

- STMicroelectronics UM1724, Nucleo-64 MB1136 user manual, connector table for NUCLEO-F401RE and USART solder-bridge section: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STMicroelectronics RM0368, STM32F401 reference manual, RCC, GPIO, USART and independent-watchdog register maps: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Piano Pedaller prior art: https://nime.org/proceedings/2017/nime2017_paper0062.pdf
- MIDI visualizer comparison: https://github.com/onlaj/Piano-LED-Visualizer/blob/master/Docs/features.md

All original design, firmware, tests, application and documentation are authored by jayis1. Standard KiCad library footprints remain third-party library assets with their original licensing; using them does not transfer their authorship. Author attribution appears in source comments, schematic and PCB title blocks, symbol properties, project text variables and manifest metadata. Generated binaries, caches and local KiCad preferences are intentionally excluded from version control.
