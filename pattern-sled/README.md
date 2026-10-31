# Pattern Sled — position-resolved repeat comparison for mechanical program strips

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software, documentation and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library geometry retains its upstream licensing.

Revision A is a build-tested engineering prototype, not a production instrument. The custom board is a routed passive input carrier for a purchased NUCLEO-F401RE. This repository contains target firmware, a desktop companion, host tests, native KiCad files and reproducible hardware-generation/check scripts. It does not contain measured results from a fabricated unit. Electrical bring-up, switch behavior, mechanical repeatability and useful operating speed remain unverified.

## Purpose and practical value

Pattern Sled helps a maker of slow mechanical teaching automata compare repeated three-track program strips. A strip has sixteen positions per repeat. Three independent followers close dry contacts to represent the program word at each position. A fourth contact provides a reading clock after the followers have settled. The first sixteen valid words become a volatile reference. Later words are compared against the corresponding position, preserving a mismatch count per position and the latest compared word there. The result answers a practical question: does the reader repeatedly see the intended physical pattern, or do particular positions disagree?

The immediate audience is a museum workshop, automata class or paper-mechanism builder using broad, hand-made program strips rather than standardized computer tape. A student might create a repeated three-bit pattern to represent three model figures' poses. After copying the pattern several times onto a long strip, the instructor wants to find a misplaced tab, a narrow hole or a follower that sometimes fails to seat. Today this can be checked by eye, by recording a phone video, or by manually writing the three contact states into a table. Pattern Sled keeps a positional notebook without photographing people and without requiring a laptop operator to press a button at each position.

This is a reader-comparison fixture, not an automaton controller. No output enables movement or drives a solenoid. It never connects to an energized machine's program wiring. The teaching mechanism should be disconnected from its original electrical system, and the contacts should belong exclusively to the instrument. A mismatch indicates that the observed three-bit word differs from the first repeat. It does not identify whether the strip, fixture, contact, wiring or reference was wrong. Inspection remains necessary.

A useful first experiment is to copy a known sixteen-word pattern onto three successive sections. Feed the first section as the reference, the second unchanged, and the third with one deliberately moved tab. Observe whether the expected position alone acquires mismatches. Then repeat without changing the strip while gently adjusting a follower bracket. This separates a repeatable geometric difference from intermittent reading behavior. Retain a paper copy of the intended pattern, because the automatically learned reference is not a truth oracle.

## Originality, inventory and alternatives

The refreshed repository contained 118 device README files before this addition. Its purposes and titles were inventoried. Cam Dwell Map is the closest indexed mechanical observer, but measures angular occupancy of one follower using a quadrature encoder and index; it does not learn a multiword sequence or compare subsequent copies position by position. ContactAtlas maps detached selector contact signatures. LoomMapper maps electrical harness connectivity. Shutter Quartet measures fragmented exposures at four positions. None of these is a three-track, learned-reference strip-repeat comparison notebook.

Punched-tape reading is established technology. A bounded prior-art search found [ICL CES, Reading Punched Paper Tape](https://iclces.uk/articles/reading_punched_paper_tape.html), describing mechanical, conducting-brush and optical reading with sprocket-based self-clocking. [Eight Ways to Read Punched Tape](http://www.decodesystems.com/ce-punched-readers.html) describes mechanical fingers, optical techniques and stepped motion. Those sources are important alternatives, not evidence that this proposal invented tape sensing. The proposed distinction is the small, local, sixteen-position comparison workflow, explicit provisional clock intervals and terminal loss-of-alignment behavior for hand-made three-track teaching strips. Worldwide novelty, patentability and freedom to operate have not been established.

Why this combination now? A low-cost development board can keep timing and positional evidence outside the desktop event loop, while an ordinary local desktop can display every position without a cloud account. The electronics are inexpensive enough to reuse across classroom fixtures. There is no new scientific sensor principle. The practical hypothesis is that automatic comparison is less distracting than transcribing switches and easier to audit than an opaque recognition system. A conventional logic analyzer is superior for fast electrical timing; a purpose-built optical tape reader is superior for archival media. Neither should be replaced merely to build this prototype.

## Hardware specifications and architecture

| User requirement | Design decision and limitation |
|---|---|
| Compare physical copies without typing every word | Learn the first sixteen words; a bad reference remains a bad reference |
| Locate recurring differences | Per-position errors and denominators, not just a global pass/fail lamp |
| Avoid silently shifting alignment after a malformed clock | Terminal setup, hold, pulse and gap faults; deliberate realignment/reset needed |
| Keep teaching patterns local | Receive-only USB, no radio or cloud service, explicit owner-only CSV |
| Make electronics inspectable | Passive routed carrier and external documented development board |
| Avoid implying complete cycle history | Show independently updated latest slots and bounded snapshot history |

| Function | Revision A specification |
|---|---|
| External controller | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Processor / memory | Cortex-M4; 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI restored to nominal 16 MHz |
| Acquisition | One four-bit sample every nominal millisecond |
| Inputs | Four disconnected, voltage-free dry contacts |
| Suggested contact | Omron D2F-01L low-level lever switch; verify variant and mechanical suitability |
| Reference | First sixteen accepted three-bit words; volatile |
| Interface | USART2 TX, PA2 AF7, 115200 baud, 8N1, through ST-LINK virtual COM |
| Custom board | 80 × 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Passive network | Four 10k pullups and four 1k series resistors |
| Power | USB to Nucleo; regulated 3.3 V and common ground to carrier |
| Companion | Python 3 and Tk; Linux serial, replay, history and private CSV export |
| Radio / actuator | None |

    Three strip followers -------- DATA0 DATA1 DATA2 ---- J4 J5 J6
    Reading-clock lever ---------- CLOCK ---------------- J3
                                                         |
                                   1k series + 10k pullup per channel
                                                         |
    USB --> NUCLEO-F401RE --> 3V3/GND --> J1              J2
                |                                        |
                +--- PA0 / PA1 / PA4 / PB0 <--------------+
                |
                +--- 1 ms sampler --> setup / hold / clock validator
                |                         |
                |                    sixteen-word reference
                |                    per-position comparisons
                +--- CRC PS1 snapshots --> USART2 TX --> ST-LINK VCP
                                                           |
                                            Linux desktop companion
                                            Pattern / History / Diagnostics

J1 pin 1 is 3V3 from Nucleo CN6 pin 4; J1 pin 2 is ground from CN6 pin 6. J2 pins 1 through 4 connect to CN8 pins 1 through 4, respectively PA0, PA1, PA4 and PB0. J3 pin 1 is CLOCK, J4 pin 1 DATA0, J5 pin 1 DATA1 and J6 pin 1 DATA2. Every contact connector pin 2 is ground. Electrical low means closed; firmware inverts the inputs. Raw bit 0 is CLOCK; raw bits 1, 2 and 3 are DATA0, DATA1 and DATA2. The numeric program word is raw shifted right by one. The app displays three-bit binary with DATA2 at the left.

The Nucleo supplies the MCU, regulator, decoupling, reset circuitry, debugger and USB bridge; those functions are not fictitiously represented as components on the passive carrier. USART receive is disabled. The host does not change policy or reset the reference by sending commands. Board support follows STM32F401 RM0368 and the existing repository platform; use ST UM1724 to confirm the purchased MB1136 revision, analog header mapping and ST-LINK virtual-COM solder-bridge arrangement before connection.

## Mechanical fixture and operating envelope

Use a rigid nonconductive sled with an interchangeable program strip and three spring-loaded followers. A starting teaching-strip geometry is 60 mm wide with three data lanes spaced 15 mm apart and sixteen positions at 20 mm pitch. Each complete repeat is therefore 320 mm long. These are fixture design dimensions, not measured optimal values. A removable guide fence should stop lateral wandering. Number positions zero through fifteen and add an unmistakable reference-start mark. Keep the first sample ahead of the reading line during reset.

A follower can be operated by a raised tab or by a blunt pin entering a deliberately large hole in a supported strip. Its lever operates a low-level switch but must not load a fragile paper edge directly. A rigid support beneath the strip and adjustable travel stops should prevent the switch from taking insertion or hand force. Validate that the unloaded and engaged positions produce stable electrical levels. Never use valuable historical tape: mechanical fingers can catch or damage it, and this nonstandard pitch is unsuitable for archival media.

The reading-clock lever is separate from the data followers. Advance and settle a position, operate the clock, release it, then advance again. A cam attached to a manually indexed carriage may automate that lever later, but this revision does not supply a motor or infer carriage direction. A nominal training cadence around two words per second gives generous settling time; it is a proposed starting point, not measured throughput. Contact bounce can cause terminal rejection. The conservative raw-clock policy intentionally favors stopping over silently losing a position. Choose and characterize contacts with clean sampled pulses; if real bounce makes the workflow impractical, revise the clock conditioning and its tests rather than concealing the failures.

There is no position encoder, home sensor or independent repeat marker. The operator establishes alignment. Reversing the strip, skipping an entire valid pulse, operating the clock twice at one position, or omitting a complete sixteen-word repeat may be undetectable or may look like ordinary pattern mismatches. The system does not authenticate position. Stop, export, restore the reference-start mark and reset after a known handling error. Do not describe the notebook as proof that every physical position was inspected.

Keep initial contact harnesses below half a meter, pairing each signal with its ground return. Put the carrier and Nucleo in an insulating enclosure beside the sled, with strain relief and accessible USB. The carrier has no mounting holes; use insulated edge supports. An approximately 160 × 110 × 45 mm enclosure is a planning allowance for the boards, not enclosure CAD. Verify clearances against actual hardware. No ingress, impact or ESD rating is claimed.

## BOM, power budget and safety

The custom carrier has fourteen placed components: four 10k resistors, four 1k resistors, five two-pin headers and one four-pin header. Use one-percent quarter-watt axial resistors compatible with DIN0207 bodies and 7.62 mm lead pitch, and 2.54 mm headers. Native KiCad footprints come from the installed libraries. External purchases add one Nucleo, four low-level switches, mating cables, fixture stock and an enclosure.

Quantity-one planning allowances in USD are 25 for the Nucleo, 5 for carrier fabrication allocation, 2 for resistors, 4 for headers/harness material, 12 for switches and 10 for a simple sled, totaling 58 before labor, tax, shipping and enclosure. These are assumptions, not quotations. This is an educational build budget, not a committed retail price; a finished assembled tool would need a new cost analysis including manufacturing test, support and enclosure. Reusing an existing development board makes sense if its pin and bridge configuration is checked again.

A closed contact draws 3.3 V divided by 11k, or 0.300 mA. Four closed contacts draw 1.200 mA. The MCU-side closed node is approximately 0.300 V above ground because of the series resistor. These ideal calculations exclude contact resistance, leakage and tolerances. Measure the actual open and closed voltages and check MCU input margins before use. Switch minimum-load and wetting-current suitability matter; an arbitrary power switch may be unreliable at this excitation.

Reserve 100 mA at USB 5 V as an unmeasured system planning allowance. There is no battery or charging subsystem and no battery-life claim. Power the Nucleo through its supported USB arrangement; do not backfeed its 3V3 rail with another source. All signal returns share USB host ground. The series resistors are not galvanic isolation, surge suppression or certified hot-plug protection. Do not connect powered sensor outputs, PLC wiring, mains wiring, original machine contacts still attached to another circuit, or unknown voltages. Use a dry, unpowered tabletop mechanism. This is not a safety interlock, production quality gate or protective monitor.

## Firmware architecture and exact measurement contract

The firmware is freestanding C11 with a real Cortex-M4 startup table, RAM initialization, linker regions, clock reset, GPIO verification, SysTick, independent watchdog and bounded nonblocking UART. The board/clock/serial drivers and carrier-generation platform reuse jayis1's Coupon Transit implementation at commit 4d5cc29. The new engine, PS1 payload, application model and pattern UI implement this device's distinct behavior. The one-time import script records that reuse and refuses existing destinations; it is not a routine build command.

The engine receives timestamped raw samples. It uses consecutive one-millisecond intervals, not desktop arrival time. The first sample establishes only an endpoint and earns no elapsed credit. A gap of anything other than one millisecond after initialization terminates acquisition with F_GAP. Unsigned timestamp subtraction permits ordinary 32-bit rollover. Once halted, completed words, learned reference, latest comparisons and mismatch totals remain frozen; timestamps and raw input diagnostics may continue to update.

At startup the engine is WAIT. Initial clock-high attachment does not fabricate a rising edge. Twenty continuously sampled clock-low intervals establish READY. A rising edge can also qualify when the falling-to-rising interval reaches exactly twenty milliseconds, including the final interval ending at the rising endpoint; a nineteen-millisecond interval fails rearm. Independently, the three data bits must have remained unchanged for twenty sampled intervals through the rising-clock endpoint. A data transition at that endpoint therefore fails setup. A rising clock before the low rearm requirement terminates with F_REARM. A valid rise captures the three-bit word and enters HIGH, but does not yet count it.

While HIGH, the data must equal the captured word through every sampled endpoint, including the falling clock endpoint. Changing data and releasing the clock together is rejected as F_HOLD because their physical ordering is unknown. A pulse must last at least ten milliseconds and less than one thousand milliseconds. A fall at ten milliseconds is accepted; a fall at one thousand is rejected because the deadline wins. Short pulses terminate with F_SHORT, and an extended pulse with F_LONG. Raw sub-sample glitches can escape detection; this contract concerns observed endpoints, not oscilloscope-quality electrical continuity.

Only a valid falling edge completes a word. The first sixteen completed words fill reference slots in order. They are training observations, not comparisons. Subsequent words update latest and error count at slot words modulo sixteen. A mismatch is any unequal three-bit word, counted once even when two or three bits differ. The app can show the latest XOR to identify differing lanes, but the device does not maintain cumulative bit-error counts. Every completed word returns to WAIT and requires a new low baseline. Setup and rearm can accrue simultaneously.

Counter updates are preflighted. Word-count exhaustion halts before partially updating a slot. Transport errors saturate, and further transport-error accounting at saturation can produce F_OVERFLOW. The first fault is retained. GPIO configuration and watchdog initialization failures terminate before ordinary acquisition. A clock-reset failure stops before serial initialization, so absence of telemetry is itself a bring-up problem, not an all-clear status.

The invariant is that bad equals the sum of all sixteen slot-error counts. A slot cannot have more errors than comparisons, and its latest comparison must agree with whether at least one success or failure exists there. Unlearned reference and never-compared latest entries remain zero internally but are displayed as absent rather than as measured zero. The first learned repeat is immutable until MCU reset. A watchdog reset or power loss erases it; no flash journal, boot nonce or persistent calibration is implemented. The host detects many regressions but cannot distinguish every possible reset from legitimate forward traffic.

## Application and PS1 interface

The companion is a standard-library Python/Tk desktop application appropriate to a USB-connected workbench. It has Connect/protocol, Pattern, History and Diagnostics/setup screens. The connect screen accepts an explicit Linux serial device; it does not silently probe arbitrary ports. Permission denial and non-TTY paths are reported without discarding a previously loaded recording. A second connection is refused while the existing serial port is owned. Disconnect retains evidence and changes freshness; it is not a delete operation.

Pattern shows each slot's reference, latest comparison, latest XOR, mismatch count and comparison denominator. Latest values are independently updated per slot, so during a partial repeat the table can contain observations from two different repeats. Do not call that a coherent latest completed cycle. History shows up to 2048 accepted status snapshots, not every physical word. A batch of frames is retained and displayed as a batch rather than only showing its final member. Diagnostics carries source, connectivity, parser errors, blocked state and an operating checklist.

Every nominal 100 ms the MCU emits one ASCII line. Its body begins PS1, followed by decimal unsigned fields: now, raw, state, fault, tx, words, bad, high_ms; then ref0 through ref15, latest0 through latest15 and errors0 through errors15. An asterisk, four uppercase hexadecimal CRC digits and LF finish the line. CRC is CCITT polynomial 0x1021 with initial value 0xffff over the complete body before the asterisk. There is no NUL terminator on the wire. PS1 identifies the fixed sixteen-word and timing policy; there are no host commands or configurable on-device thresholds.

State numbers are WAIT 0, READY 1, HIGH 2 and HALTED 3. Fault numbers are none 0, GPIO 1, watchdog 2, gap 3, setup 4, hold 5, short clock 6, deadline 7, rearm 8 and exhaustion 9. high_ms is provisional while HIGH; a successful word clears it, while a terminal fault retains its last recorded value. It is not a complete history of accepted pulse widths. Acquisition validity and transport freshness are different: a progressing timestamp can legitimately accompany a halted instrument.

The parser bounds frame and input sizes, validates decimal formatting and CRC, resynchronizes on a complete PS1 marker after fragments and rejects incomplete replay tails. Replay must be a regular file no larger than two MiB, opened nonblocking before checking its type. Empty replay is rejected; a valid idle snapshot is allowed. The session checks immutable learned words, counter monotonicity, per-slot visit bounds, final outcomes and frozen terminal evidence. These are necessary consistency checks, not reconstruction of all missing samples. CRC is accidental-corruption detection, not authentication.

Live freshness needs two advancing status timestamps. Duplicates do not renew it. A reception pause over two seconds, timestamp regression, contradictory duplicate or more than two seconds of cumulative host/device divergence blocks the session and requires reconnect. The absolute divergence allowance does not grow forever; an uncalibrated oscillator can eventually cause a conservative reconnect during a long run. Reconnect flushes the serial queue but does not reset the physical target. Retained data are explicitly marked disconnected or replay rather than being portrayed as newly measured.

CSV export creates a new owner-only file, refuses overwrite, and includes author, source, connected/fresh/blocked state, parser errors, warning and retained-row count before the snapshot table. A failed or empty session still exports diagnostic metadata. No telemetry goes to a network service. Local records may reveal a proprietary physical pattern; delete exported files and recordings through the operating system when no longer needed. The device's volatile reference disappears on reset or loss of power.

## Build, verification and commissioning

From the repository root:

    make -C pattern-sled/firmware all test
    python3 -m unittest discover -s pattern-sled/tests -p test_app.py
    xvfb-run -a python3 -m unittest discover -s pattern-sled/tests -p test_gui.py
    /usr/bin/python3 pattern-sled/tools/check_hardware.py
    python3 pattern-sled/app/workbench.py

The target Makefile uses arm-none-eabi-gcc and produces ELF and binary files under ignored firmware/build. The flash target uses OpenOCD with ST-LINK and STM32F4 configuration. Do not flash an attached board without confirming it is the intended Nucleo and disconnecting unrelated circuitry. The desktop needs Python with Tk; its Linux serial path uses termios. There are no pip dependencies, JavaScript packages or cloud credentials. GUI tests need an X server or Xvfb. CAD generation/checking needs KiCad 9 CLI and its system Python pcbnew bindings. Build products, Python caches and KiCad local preferences are ignored.

Tests exercise valid learning, repeated comparisons against an independent array oracle, setup and hold failures, pulse boundaries, acquisition gaps, timestamp wrap, transport bounds, wire framing, replay rejection, private exports, actual Tk navigation and PTY disconnect. The PTY stream is explicitly synthetic input generated by the production C engine and encoder; it is not evidence from physical contacts. Hardware checks run ERC and DRC, export a fresh schematic netlist and compare every reference/pin/net/value/footprint against PCB pads and the connectivity manifest. A passing passive-board ERC does not validate the external Nucleo power source or your harness.

Before assembling, review connector numbering, footprints, resistor values and clearances against actual parts. Inspect the routed board and schematic. Power only the Nucleo first, then measure its regulated rail. Attach the unpowered carrier and verify each open/closed node using a meter. Exercise each individual input while observing raw telemetry. With a logic analyzer, characterize switch bounce and the actual sample timing. Only then attempt the known three-repeat pattern experiment. Compare every result to a manually recorded reference and deliberately introduce setup, hold, missing-clock and double-clock faults.

Acceptance for a useful physical prototype requires reproducible detection of known single-position changes, an acceptably low nuisance-halt rate, stable mechanical registration and demonstrated recovery by realignment/reset. Fabrication, programming, electrical margins, oscillator accuracy, ESD, mechanical wear, absolute performance and user benefit remain unverified. See VALIDATION.md for the actual software and CAD checks performed in this revision, rather than treating this commissioning plan as completed testing.
