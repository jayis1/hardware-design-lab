# Tap Echo — reference-preserving two-pad rhythm fixture

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its original upstream licensing. Revision A is an unbuilt engineering prototype, not a medical instrument, a reaction-time assessment, a certified musical timing standard or a safety controller.

## Purpose and practical benefit

Tap Echo is a small, repositionable two-pad fixture that captures a demonstrated sequence of left and right switch presses and compares later repetitions with that actual demonstration. It retains both pad identity and the interval between qualified press onsets. The reference is not an ideal metronome grid: an intentionally uneven rhythm is a valid reference. Two additional voltage-free controls select reference recording and delimit the capture. The companion desktop shows the reference and latest attempt side by side, including per-interval signed differences, rather than reducing every experiment to an unexplained score.

The intended user is a maker comparing physical switch layouts for a deliberately slow, two-button interface. A concrete experiment starts with two large paddles on a flat base. The maker records A, A, B, A at comfortable, unequal spacings. After moving the right paddle farther away, the participant repeats the same phrase. If the long A-to-B interval changes while repeated A intervals remain similar, the maker has a useful question to investigate about the layout. It is not evidence of disability, learning ability, fatigue, or a causal ergonomic effect. The participant's comfort and observations remain more important than a match label.

Another audience is a classroom studying event identity versus event timing. Two phrases can have identical counts and total duration yet differ in their internal order or intervals. Students can deliberately construct that contrast, inspect the retained arrays and explain why a total counter or average tempo does not detect it. A third use is commissioning handmade low-force switches: swapping the same switch between positions helps distinguish a persistent mechanical issue from a placement-related effect. The instrument does not measure actuation force, contact resistance, key travel or acoustic latency.

The proposed distinctive combination is a disconnected physical-switch fixture, an operator-captured unequal reference, conservative rejection of mixed press envelopes, and atomic reference replacement. A malformed replacement attempt never destroys the previous good reference. No cloud account, phone pairing, audio generation or remote control is needed. “Replay” in this project means either a human repeating a captured phrase or the app reading a saved serial recording; it never means automatic physical pad actuation or audio playback.

## Originality and provenance

Before selecting this concept, the configured hardware repository was pulled and all existing device directories and README titles were inventoried. The closest purposes were inspected. Touch Tour closes four-station coverage rounds while tolerating repeated stations; it does not compare timing against a learned phrase. Chord Window measures overlapping chord press and release relationships; Tap Echo deliberately forbids overlapping pads and compares sequential onsets. Tip Cadence investigates alternating tipping-bucket intervals and handling interruptions; it neither learns ordered human demonstrations nor compares identity-aligned attempts. The pre-existing, incomplete quorum-comb directory was left unchanged and is not part of this design.

Rhythm capture and rhythm tutoring already exist. A bounded public search found Rhythm Lab's tap controls, which include precision and duration settings, and musical rhythm machines such as Korg's KR-11. These are prior art, not evidence of universal novelty. Tap Echo's proposal is the specific transparent workshop fixture and conservative evidence contract described here. Worldwide uniqueness, patentability and freedom to operate have not been established. Product usefulness also remains a hypothesis until a supervised comparison against a simple video-and-stopwatch workflow demonstrates an advantage.

The passive carrier, low-level MCU support, UART implementation, host serial transport and their driver tests were imported from jayis1's Touch Tour at revision `b47eaff4728f3325fa57c5e382106d2017561c01`. The overwrite-refusing `tools/import_platform.py` records the exact import. Do not run it again over an existing checkout. Engine logic, TE1 fields, comparison model, desktop workflow and sequence tests are specific to Tap Echo. Reusing a proven resistor carrier is intentional; the resistor circuit itself is not claimed as an invention.

References for subsequent physical review:

- ST UM1724, Nucleo-64 MB1136 manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 peripheral reference manual, and ST PM0214, Cortex-M4 programming manual.
- Rhythm Lab tap controls: https://www.rhythmlab-app.com/knowledge-base/screens/tap-screen/tap-screen-controls/

A fresh ST manual download was attempted using two HTTP modes; one failed and one timed out. The connector mapping therefore remains the inherited platform contract, not a newly verified manufacturer revision. Check the actual purchased board and its current manual before applying power. Search snippets establish only the bounded prior-art comparison, not a comprehensive product survey.

## Hardware specification and architecture

The processor is an STM32F401RET6 on a separately purchased ST NUCLEO-F401RE MB1136 board. Its nominal resources are 512 KiB flash and 96 KiB SRAM. Firmware explicitly selects the internal 16 MHz HSI clock, uses nominal one-millisecond SysTick acquisition and sends USART2 status at 115200 baud, eight data bits, no parity and one stop bit. USART2 TX is PA2 in alternate function 7. The ST-LINK virtual COM connection carries data to a Linux desktop. UART receive is disabled; the application cannot change the capture policy or flash the processor through this protocol.

The custom board is an 80 by 70 mm, two-copper-layer, nominal 1.6 mm FR-4 passive carrier. It has fourteen actual components, including six headers and eight resistors, with routed copper and a complete board outline. It is cable-connected, not a mechanically mating Nucleo shield. Processor decoupling, regulator, reset circuit, programmer and USB bridge are on the external Nucleo; those functions are not hidden unconnected symbols in the carrier schematic. No FPGA, radio, battery, charger, heater or motor is present.

    PAD A / PAD B       RECORD / GATE
           voltage-free contacts
                    |
          1k series + 10k pullups
                    |
           Nucleo PA0/PA1/PA4/PB0
                    |
        nominal 1ms whole-word sampling
                    |
       raw-envelope guard + qualification
                    |
          bounded 16-event trial array
                    |
       reference commit / identity-and-gap compare
                    |
       CRC-protected TE1 through ST-LINK USB
                    |
     Linux pattern screen / diagnostics / local export

### Pin and net contract

| Carrier | Function | External connection |
|---|---|---|
| J1 pin 1 | VCC, regulated 3V3 | Nucleo CN6 pin 4 |
| J1 pin 2 | GND | Nucleo CN6 pin 6 |
| J2 pin 1 | SENSE0, pad A | CN8 pin 1, PA0/A0 |
| J2 pin 2 | SENSE1, pad B | CN8 pin 2, PA1/A1 |
| J2 pin 3 | SENSE2, RECORD | CN8 pin 3, PA4/A2 |
| J2 pin 4 | SENSE3, GATE | CN8 pin 4, PB0/A3 |
| J3 pin 1 | PAD_A | A switch, other terminal to J3 pin 2 |
| J4 pin 1 | PAD_B | B switch, other terminal to J4 pin 2 |
| J5 pin 1 | RECORD | Maintained mode switch, other terminal to J5 pin 2 |
| J6 pin 1 | GATE | Maintained capture switch, other terminal to J6 pin 2 |

Every J3–J6 pin 2 is ground. Header pin numbers, not an assumed cable colour or viewing direction, define orientation. Electrical low means closed; software converts it to logical one. The four logical masks are A=1, B=2, RECORD=4 and GATE=8. The virtual COM arrangement assumes Nucleo SB13/SB14 connected and SB62/SB63 open; inspect the purchased board. Development programming uses its ST-LINK SWD connection.

R1, R3, R5 and R7 are 10k pullups, for example Yageo MFR-25FBF52-10K. R2, R4, R6 and R8 are 1k series resistors, for example MFR-25FBF52-1K. Both use DIN0207 axial 7.62 mm footprints. J1 and J3–J6 may use Samtec TSW-102-07-G-S; J2 may use TSW-104-07-G-S. Confirm purchased dimensions against footprints. Two Omron D2F-01L switches under separate paddles are candidate input contacts; RECORD and GATE need maintained voltage-free switches suitable for low current. Contact procurement and wetting-current performance require bench verification.

A closed branch ideally draws 0.300 mA at 3.3 V and places the sense node near 0.300 V. Four closed branches draw 1.200 mA and dissipate 3.96 mW in the resistor networks. These calculated values exclude the Nucleo, LEDs and USB bridge, and are not measured noise margins. Reserve a protected 500 mA USB supply for development and measure actual consumption. The carrier has no isolation, fuse, reverse-polarity protection or TVS network. Never connect powered machine wiring, mains, PLC outputs or another powered sensor to these headers. USB ground is shared with the fixture.

## Mechanical proposal and assembly

Use a dry tabletop base approximately 280 by 180 mm with two repositionable paddle modules. Start with 45 mm square rounded pads, their centres separated by approximately 100 mm. Slotted mounts permit spacing changes without changing the contacts themselves. Provide a mechanical travel stop independent of the switch lever; hand force must not be carried by the switch body alone. Protect pinch points and use captive fasteners. These are proposed prototype dimensions, not supplied mechanical CAD or a validated ergonomic specification.

Place RECORD and GATE at the rear for an instructor or maker to operate without obstructing the tapping pads. Mark their open and closed positions visibly. A second person may delimit the phrase, but this introduces coordination uncertainty; do not interpret first-onset latency relative to GATE as reaction time. The engine deliberately ignores that first latency. Keep leads below 0.5 m initially, route signal and ground together, and add strain relief. An insulating carrier enclosure must support the PCB edges because the routed design has no mounting holes; do not drill holes through existing copper.

Assemble with USB disconnected. Check resistor values, continuity, shorts, header numbering and the 3V3 supply polarity. Confirm each contact is isolated from any external circuit. Connect ground and then the documented harness; inspect with a meter before powering. An open wire is indistinguishable from a released contact, and a short resembles a held control. The firmware cannot prove wiring integrity from these four bits. No environmental, ESD, drop, liquid ingress or cleaning-agent resistance is claimed.

## Firmware behavior and design decisions

The firmware is freestanding C11 with explicit register definitions, startup vectors, RAM initialization and a linker memory map. `main.c` establishes the clock, configures inputs and transmit-only UART, checks configuration and starts the independent watchdog. The foreground loop polls the UART without waiting for a whole message, samples each new SysTick value and requests a status frame every 100 ms when the previous frame is no longer pending. No heap allocation, operating system, interrupts from external contacts or vendor SDK is required.

All four input bits qualify as one word after five unchanged intervals, requiring six observations. This intentionally treats GATE and pad changes close together as an ambiguous capture boundary rather than independent precisely timed events. A rejected glitch can delay an onset. This is appropriate only for slow tapping with deliberately separated control changes, not high-speed musical measurement. Begin testing with presses and releases at least 20 ms long and inter-onset intervals at least 100 ms. HSI accuracy, switch mechanics, sampling phase and qualification delay remain uncalibrated.

At startup, GATE must be observed open with both pads released before arming. RECORD may already be closed. To teach, close RECORD, wait for qualification, then close GATE. Press and release between two and sixteen pads in any order. Open GATE only after releasing the final pad. The completed nonempty phrase replaces the reference atomically; a one-tap, over-capacity, ambiguous, interrupted or timed-out capture does not. Open RECORD while idle to choose comparison mode. A comparison without a reference is rejected rather than treated as an empty match.

Each event stores pad identity and the interval from the preceding qualified onset. The first event has interval zero. Release duration and held duration are not comparison metrics, but a qualified all-clear is required between distinct taps. A held pad produces one event. The raw identity union is retained through an entire press envelope: brief A followed directly by B without a qualified all-clear aborts even if A never qualified. Simultaneous A+B also aborts. Any raw RECORD change during a capture aborts; changing mode and ending a capture together is not accepted as a clean completion.

Comparisons align events by ordinal position, not by dynamic time warping. Length difference counts as missing or extra slots. Each shared slot counts once as wrong if its pad identity differs or its interval differs by more than the greater of 30 ms and one fifth of the reference interval. The first slot compares identity only. Maximum absolute interval error is retained separately. Equal-to-threshold differences are accepted. No tempo normalization is applied: replaying the correct proportions twice as slowly can be a mismatch. This explicit policy is a workshop convention, not a validated performance threshold.

A capture expires at 30,000 acquired-clock milliseconds, with timeout rejection before completion on the same observation. Missing even one sample abandons an active capture and restarts qualification. Repeated or backward timestamps latch a clock fault. Unsigned subtraction handles an ordinary 32-bit wrap for contiguous acquisition. Counters never silently wrap; exhaustion latches a fault and disables capture. A reset clears all volatile history and the reference. There is no persistent configuration, flash write or update protocol to corrupt mid-transaction. Physical programming and power-cycle recovery remain development-board procedures.

## Application, protocol and privacy

The companion uses Python's standard library, Tk and POSIX termios on Linux. There are three actual screens: Pattern comparison shows the reference and latest attempt arrays; Session diagnostics shows counters, source, inferred epochs and bounded retained snapshots; Wiring and procedure gives the operating contract. Connect opens a real serial device at the configured baud and flushes queued input. Disconnect restores its terminal settings. Replay accepts a regular raw TE1 file up to 2 MiB, rejects corrupt or incomplete content and explicitly labels the view historical. The generated demonstration file is synthetic output from the real C engine, not a physical measurement.

TE1 is newline-delimited ASCII: the literal version marker, comma-separated unsigned decimal values, an asterisk and four uppercase hexadecimal CRC digits. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xFFFF over everything before the asterisk. `wire.c` and `app/model.py` define the exact field order: timing and diagnostic scalars, capture/result scalars, sixteen reference pad/interval pairs and sixteen event pairs. Unused event slots are zero. Status arrays can be incomplete while active or retained after abort; only a completed reference/match/mismatch has the corresponding finished meaning.

The parser bounds chunks and frames, verifies CRC, validates field count and ranges, and resynchronizes live streams at the full version marker. Replay is stricter and rejects any framing error or unfinished tail. The host retains at most 1,000 snapshots and reports dropped old rows. A decreasing sample counter increments a heuristic epoch count. There is no persistent boot identifier, so some resets remain indistinguishable; the app does not claim proof of uninterrupted session identity or reconstruct events hidden between status frames.

LIVE denotes recent reception of a changing acquisition counter, not authenticated real-time delivery. Duplicate counters do not renew freshness; after two seconds without progress the view is stale. Buffered advancing traffic can still be old, and the receive-only link cannot prove absolute freshness. This is acceptable for a supervised non-controlling notebook, not an alarm or safety application. After host suspension or suspected backlog, reconnect and repeat the experiment. CRC detects corruption but provides no authenticity, confidentiality or protection against a forged serial stream.

Exports use new owner-only JSON files and refuse overwrite, carrying author, source, epoch and history information. They are evidence snapshots, not inputs accepted by raw TE1 replay. No telemetry leaves the computer through this application and no credentials are embedded. Local filesystem access controls and physical USB access define the trust boundary. Treat exports as potentially personal behavioral data: obtain consent, avoid unnecessary identity fields, and delete files when the workshop no longer needs them.

## Reproducible build and verification

Required tools are GNU Arm Embedded GCC/binutils, native GCC with sanitizers, make, Python 3 with Tk, KiCad 9 with its system Python pcbnew bindings, and Xvfb for headless GUI tests. There are no pip or npm runtime dependencies. Run from the repository root:

    make -C tap-echo/firmware all test fixture
    xvfb-run -a /usr/bin/python3 -m unittest discover -s tap-echo/tests -p 'test_*.py'
    /usr/bin/python3 tap-echo/tools/check_hardware.py
    /usr/bin/python3 tap-echo/tools/check_release.py
    /usr/bin/python3 tap-echo/app/workbench.py --replay tap-echo/firmware/build/demo.te1 --summary

Start the interactive app with `/usr/bin/python3 tap-echo/app/workbench.py`. A user with serial-device permissions can select `/dev/ttyACM0` or the actual enumerated path. Do not run the desktop as root merely to avoid a permissions issue. `make -C tap-echo/firmware flash` uses OpenOCD with ST-LINK and the STM32F4 target configuration; this command is provided for a connected development board and was not executed during source validation.

The hardware checker runs KiCad ERC and DRC with violation-sensitive exit status, exports the schematic netlist and compares every component reference, value, footprint and pin net against the actual PCB and connectivity manifest. Passing these checks establishes consistency of the passive carrier, not external supply correctness. The schematic was also rendered and inspected for readability. The firmware tests exercise capture, matching, identity mismatch, timing mismatch, failed replacement, gap abandonment, clock wrap, capacity, timeout, raw hand exchange, CRC bounds and diagnostic faults. Host tests exercise actual pseudo-terminal serial I/O, split frames, CRC rejection, invalid fields, private exports, GUI screens, stale duplicates, connection failure and the C-generated replay.

## Remaining physical acceptance work

No PCB has been fabricated, no processor flashed, no switch mechanism assembled and no human trial performed for this revision. Before a pilot, compare oscilloscope-observed contact transitions with firmware intervals, measure low levels and pullup current, verify board reset causes and watchdog recovery, inspect UART timing and test unplug/reconnect behavior on the actual ST-LINK interface. A switch that behaves reliably at a larger current may fail at this carrier's low excitation; that must be tested rather than inferred from a digital logic threshold.

Run deliberately malformed teaching attempts and confirm the last valid reference survives. Exercise all sixteen events, sustained closed controls, short contact bounce and a deliberate sampling disruption. Compare several comfortable layouts using the same contacts, retain written observations separately, and stop if the participant experiences discomfort. A useful prototype result is an explainable design decision, not a universally improving score. Any future battery, wireless link, automated actuation, persistent storage or faster sampling revision requires a fresh electrical, firmware and security review rather than an assumption that these checks carry over.
