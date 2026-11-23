# Parity Sled — parity-excursion and recovery notebook for magnetic teaching strips

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original source, documentation and locally authored hardware design files are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. The startup, clock, GPIO, UART, histogram acquisition, passive carrier generation, POSIX transport and desktop shell reuse jayis1's Ghost Grid platform as present at repository commit 9416166046264ce230d3620dc43793ce0f21dcbd. The parity-excursion engine extensions, PY1 protocol, recovery validation, teaching workflow and parity screens are specific to this device. The overwrite-refusing import script records the initial reuse; it is not an ordinary build step and does not reproduce subsequent device-specific edits by itself.

Revision A is an unbuilt engineering prototype. Firmware target linking, synthetic software experiments, application operation and native electrical CAD validation are reproducible. Fabrication, programming a physical MCU, magnetic fixture behavior, electrical timing, lifetime reliability and user usefulness have not been demonstrated. This is neither a safety-rated reader nor an archival-media reader, machine controller, authenticated evidence recorder or production inspection gate.

## Purpose and practical utility

Parity Sled is a tabletop reader for broad, hand-fed magnetic teaching strips with three data lanes and one even-parity lane. Four independent reed switches report the local magnetic pattern. Instead of displaying only a green or red parity lamp, the instrument records how much stable sampled time each four-bit word occupied and what happened after an odd-parity excursion. Did the reader return to the same even word, recover into a different even word, or begin without a trustworthy earlier anchor? Those distinctions help a teacher or fixture builder investigate a deliberately missing magnet, lateral misalignment, contact chatter and a transition between positions without pretending that every red indication identifies a damaged data bit.

The intended audience is a computing museum workshop, classroom building tangible error-detection exercises, or maker developing a reusable magnetic program-strip fixture. Students can encode values zero through seven, add the fourth parity magnet when necessary, and physically cover, remove or shift one marker. A normal decoder often hides the unsettled region between positions. This notebook keeps that region visible as unknown histogram time while still retaining an episode's acquisition-time envelope through it. That makes the distinction between stable code occupancy and the elapsed interval between two qualified observations concrete.

A representative experiment holds a valid tile under the reader, deliberately offsets one magnet until an odd word settles, then returns the same tile to registration. A second experiment advances to a different even word instead of returning. Both involve detection and recovery, but they should increment different counters. A third experiment disconnects and reconnects a sensor lead while a tile is stationary. The device cannot decide whether the magnetic pattern or the wire changed; the point is to preserve inspectable observations and demonstrate that parity is a limited consistency rule, not an oracle for physical truth.

The electronics never move the strip or enable another machine. There is no motor, brake, relay, buzzer, HID keyboard output or cloud service. A human moves the fixture, examines results and decides what to investigate. The software must not call an even word correct merely because its parity matches. Two changed bits preserve even parity, all-zero is even, and an absent strip can look exactly like a legitimate zero word. The absence of a presence channel is a deliberate limitation of this four-input revision.

## Originality and alternatives

The repository was pulled before development, its directories and device titles were inventoried, and the closest purpose documents were read. Pattern Sled learns a sixteen-position reference from three clocked data tracks and compares later repeats. Ghost Grid computes predicted phantom keys from four independent switch states. Ratio Window divides dual-encoder motion into input-turn apertures. Parity Sled has neither a reading-clock lane, learned reference, matrix-scanner model nor shaft encoder. Its contribution is the four-track parity-excursion/recovery workflow, with retained same-word versus changed-word endpoints and separate clean-word exposure accounting.

Parity checking and physical code readers are established technology. The ICL CES article “Reading Punched Paper Tape” describes parity holes, mechanical readers and sprocket-based timing. The East Tennessee State numerical-control notes describe even and odd parity in historical punched-tape systems. Those sources are close conceptual prior art, not evidence of worldwide novelty. This proposal does not claim to have invented parity, magnetic sensing or tape readers, and no patentability or freedom-to-operate search has been completed.

The useful design hypothesis is that a low-cost local notebook exposes transitions and recovery more clearly than a single parity LED while being easier to construct than a synchronized optical reader. It remains a hypothesis until workshop trials establish usability. A logic analyzer is superior for detailed electrical bounce. An optical or camera reader is preferable for valuable historical media. A fifth timing track and a position encoder would be required before claiming reliable per-cell character extraction. This revision intentionally records words and excursions rather than decoded text or positions.

## Hardware specifications and architecture

| Function | Revision A choice |
|---|---|
| MCU | STM32F401RET6 Cortex-M4 on external ST NUCLEO-F401RE MB1136 |
| Memory | 512 KiB flash and 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz, explicitly restored at startup |
| Sensors | Four external Littelfuse MDSR-7-10-15 normally-open reed switches |
| Sensing | Four independent low-active digital inputs; closed contact becomes logical one |
| Acquisition | Nominal one sample per millisecond, whole-word qualification |
| Qualification | Five unchanged intervals; six observations establish the first valid endpoint |
| Communications | USART2 TX, PA2 AF7, 115200 baud, 8N1 through ST-LINK virtual COM |
| Custom PCB | Passive 80 × 70 mm two-layer carrier, nominal 1.6 mm FR-4 |
| Supply | Nucleo USB supply; carrier receives its regulated 3.3 V |
| Companion | Python 3 with Tk on Linux, operating-system standard library only |
| Wireless / actuation | None |

    Hand-fed four-lane magnetic strip or captive code tiles
           DATA0     DATA1     DATA2     PARITY
             |         |         |         |
          four separately mounted, normally-open reed switches
             |         |         |         |
            J3        J4        J5        J6       return pins -> GND
             |         |         |         |
          1k series per channel / 10k pullup per sense node
                      passive carrier board
                              |
                        J2 SENSE0..3
                              |
                    PA0 / PA1 / PA4 / PB0
                  external NUCLEO-F401RE module
                              |
             whole-word qualification at nominal 1 ms
                    /                         \
        clean-interval histogram       odd-parity excursion state
                    \                         /
                    cumulative PY1 + CRC-16
                              |
                 USART2 TX -> ST-LINK USB VCP
                              |
               parity / recovery / histogram desktop

The custom board contains fourteen actual components and routed copper. The processor, regulator, decoupling, reset circuitry, debugger and USB bridge belong to the separately purchased Nucleo; they are not hidden omissions from an alleged standalone MCU motherboard. No FPGA is needed for this slow experiment. An FPGA would not solve the main uncertainties: field geometry, contact hysteresis, fixture registration and the missing position reference.

## Wiring contract and electrical limits

| Carrier connection | Destination / meaning |
|---|---|
| J1 pin 1 | Nucleo CN6 pin 4, regulated 3V3 |
| J1 pin 2 | Nucleo CN6 pin 6, GND |
| J2 pin 1, SENSE0 | Nucleo CN8 pin 1, A0, PA0 |
| J2 pin 2, SENSE1 | Nucleo CN8 pin 2, A1, PA1 |
| J2 pin 3, SENSE2 | Nucleo CN8 pin 3, A2, PA4 |
| J2 pin 4, SENSE3 | Nucleo CN8 pin 4, A3, PB0 |
| J3 pin 1 | DATA0, bit weight 1 |
| J4 pin 1 | DATA1, bit weight 2 |
| J5 pin 1 | DATA2, bit weight 4 |
| J6 pin 1 | PARITY, bit weight 8 |
| J3–J6 pin 2 | Ground return for that isolated reed switch |

ST UM1724 table 16 confirms these NUCLEO-F401RE power and analog-header mappings. The pins are configured as digital inputs here, not ADC channels. The same manual describes the default virtual-COM configuration with SB13 and SB14 on and SB62 and SB63 off. Check the purchased board revision and its actual solder bridges before attaching a harness. The register definitions follow STM32F401 RM0368 and cannot be transplanted to an arbitrary STM32 without review.

Each path is 3.3 V through 10 kohm to the MCU sense node, then through 1 kohm to the contact and ground. Nominal closed current is 0.300 mA, with a nominal MCU-side low voltage of 0.300 V. Four closed channels draw 1.200 mA and dissipate 3.96 mW in the resistor networks. These are ideal calculations, not measured current, guaranteed input margin or total system power. Reserve a protected 5 V USB source with capacity appropriate to the Nucleo, such as 500 mA, and measure the completed assembly. There is no battery, charger or battery-life claim.

The carrier has no isolation, TVS, fuse or reverse-power protection. Series resistors do not make powered external wiring safe. Connect only isolated voltage-free contacts dedicated to this fixture, with initially short signal/ground pairs below half a meter. Never connect PLC outputs, installed machine wiring, mains, an unknown powered sensor or a contact still attached to another controller. USB ground becomes common with the fixture. An open cable looks like an absent magnet; a short looks like a present magnet. These failures can be parity-consistent and are not universally diagnosable.

## BOM and mechanical construction

| Item | Quantity | Procurement specification |
|---|---|---|
| R1, R3, R5, R7 | 4 | Yageo MFR-25FBF52-10K, 10k, 1%, axial |
| R2, R4, R6, R8 | 4 | Yageo MFR-25FBF52-1K, 1k, 1%, axial |
| J1, J3–J6 | 5 | Samtec TSW-102-07-G-S, two-pin 2.54 mm header |
| J2 | 1 | Samtec TSW-104-07-G-S, four-pin 2.54 mm header |
| External controller | 1 | ST NUCLEO-F401RE plus USB data cable |
| External sensors | 4 | Littelfuse MDSR-7-10-15, glass normally-open reed switch |
| Magnetic markers | As required | Captive 3 mm diameter × 2 mm axially magnetized N35 discs, prototype starting choice |
| Fixture | 1 | Nonmagnetic base, adjustable sensor bridge, captive tile carriage, strain relief and covers |

The resistor footprints use DIN0207 bodies and 7.62 mm lead spacing. PCB routing uses 0.30 mm tracks, 0.20 mm nominal clearance, and a 0.80 mm through via with 0.40 mm drill. The board has no mounting holes; use insulating edge retention rather than drilling through routed copper. An enclosure allowance of roughly 160 × 110 × 45 mm can accommodate the carrier and external controller, but confirm actual module and cable clearances. This is not an ingress-rated or drop-tested enclosure drawing.

Littelfuse describes MDSR-7 as a normally-open glass switch with a 12.7 mm body, 10–25 AT family sensitivity range and suitability for microcontroller logic-level loads. The selected narrower 10–15 AT grade reduces, but does not remove, unit variation. Reed sensitivity in ampere-turns cannot be converted directly into a guaranteed disc-magnet operating distance. Magnet orientation, strength, neighboring fields and steel fasteners all matter. The exact magnetic stand-off must be established physically; no simulated field or measured detection distance is claimed here.

Use a nonmagnetic base approximately 300 × 150 mm. A removable strip or row of captive tiles travels along its length against two guide fences. Begin with four lane centers 25 mm apart and cell centers 30 mm apart. Mount the reeds above the lanes with their long axes parallel to strip travel. Provide adjustable sensor height from approximately 2 to 15 mm above the marker surface and independent longitudinal adjustment of at least 10 mm. These are tuning ranges, not promised switching distances. Insulate each glass body and support its leads without bending immediately at the glass seal.

Each cell encodes a three-bit value. The parity marker is present when an odd number of data markers is present. The resulting four-bit even words, displayed as PARITY/DATA2/DATA1/DATA0, are 0000, 1001, 1010, 0011, 1100, 0101, 0110 and 1111. Encapsulate every magnet beneath a bonded cover so no loose small magnet is accessible. Loose magnets are an ingestion hazard, especially to children. Keep magnets away from implanted medical devices and magnetically sensitive objects. The workshop fixture requires adult-controlled handling; it is not a certified children's toy.

First test one marker and one reed away from all other markers. Adjust the orientation and height until a wide stable plateau exists. Then test every combination of lanes, both travel directions, and nearby cells. Reject the proposed geometry if a marker closes a neighboring lane or if a neighboring cell extends the sensing plateau into the intended reading position. Increase spacing or revise orientation rather than compensating for cross-coupling in software. Reed hysteresis means entrance and exit boundaries need not coincide. Establish a deliberately slow working cadence with visible pauses; a nominal half-second dwell is an experimental starting point, not a rated throughput.

There is no position, direction, presence or clock sensor. Blank spaces between tiles can be read as even zero, and both reversal and repeated reading of one tile are permitted. Therefore recovery to the same codeword does not prove return to the same physical cell. The application displays data values as labels for electrical words, not extracted characters. Keep a printed strip map and video reference during commissioning if physical position matters.

## Firmware contract and design decisions

The firmware is freestanding C11 with its own Cortex-M4 vectors, RAM initialization, linker script, clock restoration, GPIO checks, SysTick, independent watchdog and bounded UART driver. No vendor SDK download, heap, RTOS or hidden code-generation step is needed. Startup restores HSI before serial setup. A failed clock restoration stops without emitting apparently valid measurements. GPIO or watchdog startup failure latches a terminal fault before normal acquisition.

The four input bits are qualified together. A new raw word seeds a candidate and immediately removes validity. Five subsequent unchanged one-millisecond intervals establish the qualified word on the sixth observation. Whole-word qualification avoids synthesizing a word from bits qualified at different times. Conversely, chatter on one lane postpones the entire word. That conservative tradeoff is intentional. Samples do not prove electrical continuity between observations, and pulses faster than sampling or qualification can disappear completely.

A clean histogram interval requires the same raw and qualified word at both adjacent acquired endpoints. The qualification endpoint itself receives unknown credit, not retroactive dwell. Each of sixteen bins retains clean interval total, segment count and longest segment. A one-sample disturbance splits a segment even if it never becomes a qualified word. All acquired samples are partitioned into clean bins or unknown. The initial endpoint and qualification period belong to unknown; this accounting convention is not an assertion that elapsed physical time existed before the first sample.

Separately, every qualified even word updates the last known even anchor. A qualified odd word starts an excursion only when no excursion is active. Qualification of another odd word does not end or split it. Its age advances on acquired one-millisecond intervals, including raw disturbances and requalification time. A later qualified even word completes the excursion: same if it equals the captured earlier even word, changed if it differs, or orphan if no earlier even anchor existed. The minimum completed age is six intervals. Last-result fields retain the latest completed recovery, not the latest sample or a distribution of every duration.

These two clocks intentionally answer different questions. Histogram time describes clean sampled occupancy of one exact word. Episode age describes the acquisition-time envelope from the first qualified odd endpoint through the recovering qualified even endpoint. It can include unknown and intermediate odd words. It is not continuous proven erroneous-media exposure, nor a delay measured from the physical beginning of a defect. A transient odd word that never qualifies does not start an episode, although its disturbance remains visible in unknown histogram time.

On an acquisition gap, missed grows by the elapsed tick count minus one, the current acquired sample is unknown, qualification restarts, and any active excursion is abandoned exactly once. The anchor is cleared. Later odd attachment can start a new unanchored excursion; silence does not restore the old association. Repeated or backward timestamps, invalid raw input, counter exhaustion and platform faults halt acquisition. Normal uint32 timestamp rollover is supported. All measurement updates are transactional, so a failed increment cannot publish half a histogram or episode transition. Entering terminal fault abandons a pending episode once; later halted snapshots freeze evidence except wall time, sticky fault bits and saturating transmission-error count.

The episode invariant is starts = same + changed + orphan + aborted + active. Histogram conservation is samples = unknown + sum(bins). Segment totals and maxima have independent consistency bounds. Measurements are volatile: reset or power loss erases them. There is no boot nonce or flash journal. The host detects many regressions but cannot distinguish every reset from forward-looking traffic. Deliberately reset and reconnect for a new physical experiment; do not mistake reconnecting alone for erasing device history.

## Protocol and companion application

PY1 snapshots are attempted about every 100 ms. The UART queues a bounded frame and drains without blocking the acquisition loop. A transmission stuck for 100 nominal milliseconds is dropped and counted; the next full marker permits resynchronization. The record begins with ASCII PY1 and comma-separated unsigned decimal fields. The first fields are:

    now samples missed unknown tx_errors fault raw qualified valid run

They are followed by bins0 through bins15, segments0 through segments15, peaks0 through peaks15, then:

    starts same changed orphan aborted active known anchor age
    last_age last_from last_to last_kind

The trailer is an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses polynomial 0x1021 and initial value 0xffff over the entire body before the asterisk; the check string 123456789 yields 0x29b1. CRC detects accidental corruption, not malicious senders. The 1024-byte transport buffer covers the full unsigned-decimal schema. Firmware enables no command receiver or remote update endpoint; flashing is a physical ST-LINK operation.

Fault bits are configuration 1, watchdog 2, timestamp 4, exhaustion 8 and input range 16. last_kind is none 0, same 1, changed 2 or unanchored 3. Anchor zero with known zero means absent, not an observed zero word. last_from zero for kind 3 is similarly a sentinel. An idle episode has age zero. The raw/qualified fields are retained acquisition evidence after HALT, not current magnet positions.

Run `python3 app/workbench.py` on a Linux desktop. The Parity and recovery screen shows four named contact states, current qualification, parity interpretation and episode classifications. Exposure histogram shows all sixteen words, decoded three-bit labels, parity, clean totals, segment counts and peaks. Protocol and limits explains wiring and interpretation. These are functioning local screens, not static screenshots. No pip, Node, radio, cloud credential or remote backend is needed.

Connect takes an explicit serial path and refuses a second connection until disconnected. Permission errors and non-TTY paths preserve earlier evidence. Replay accepts only regular files up to two MiB, rejects empty files and incomplete final records, and is always offline. Export creates a new CSV with mode 0600 and refuses overwrite. It retains author, provenance, connection/freshness state, parsing errors, initial fragments, warning and session error along with the latest thousand snapshots. Final cumulative totals can cover more history than those retained rows. CSV is not a PY1 replay recording.

Session checks reject counter rollback, contradictory duplicate timestamps, impossible histogram attribution, changed retained results, premature qualification and impossible episode progression. Adjacent acquired endpoints have exact attribution checks; sparse snapshots receive necessary bounds, not reconstruction of every hidden word. Live freshness requires acquisition advances rather than received bytes alone. A half-second without sample progress is stale, and a host pause or cumulative host/device disagreement beyond one second blocks live evidence until explicit reconnect. Fixed timing allowances can conservatively reject healthy long sessions with oscillator drift. No inference here authenticates uninterrupted physical acquisition.

## Build, validation and commissioning

From this directory:

    make -C firmware clean all test fixture
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/check_release.py
    python3 app/workbench.py firmware/build/demo.py1

Install GNU Arm Embedded GCC, GNU Make, host GCC, Python with Tk, KiCad 9 with its Python pcbnew module and footprint libraries, and Xvfb for headless desktop tests. No package download occurs during a normal build. Target ELF and binary files are generated under ignored firmware/build. `make -C firmware flash` uses OpenOCD with ST-LINK and STM32F4 configuration; review target identity and wiring before executing it. Physical flashing has not been performed in this run.

The C tests enumerate all ordered triples of four-bit settled words against a separate population-count/anchor oracle. They also exercise rejected raw glitches, odd-to-odd continuation, recovery duration, acquisition loss, terminal abandonment, timer rollover, counter exhaustion and CRC framing. The synthetic fixture uses the production C engine and encoder. Python tests feed dense and sparse real-encoder records into the companion, reject CRC-valid impossible transitions, exercise actual Tk widgets over Linux pseudo-terminals, export private CSV, and verify connection failure and unplug behavior. Synthetic electrical input is not evidence from magnets or a fabricated PCB.

The CAD checker runs native ERC and DRC and compares every schematic pin/net, value and footprint identifier with actual PCB pads and the connectivity manifest. A rendered schematic is inspected for readable labels and connector mapping. Passing passive-board ERC cannot validate the external power source, wiring harness or field geometry. See VALIDATION.md for executed checks and tool versions rather than interpreting this commissioning plan as completed physical testing.

For bring-up, inspect resistor values, connector polarity, solder joints and isolation with power removed. Power the Nucleo alone, measure its rail, then attach the unpowered carrier and repower through a protected supply. Measure every open and closed sense node and exercise one reed at a time. Confirm bit mapping and check actual sample timing using a reference logic trace. Test all sixteen electrical words before mounting magnets. Then characterize lateral cross-coupling, height sensitivity, both travel directions, contact bounce, neighboring cells and repeated fixture assembly. Compare a known single-marker disturbance and a known two-marker disturbance to the expected parity behavior. The latter must demonstrate a blind spot rather than being mislabeled as a passed inspection.

A useful physical acceptance test must establish repeatable magnetic plateaus, manageable unknown time, correct recovery classification and a documented operating cadence. Mechanical durability, enclosure design qualification, manufacturing tests, EMC/ESD assessment, absolute timing accuracy and teaching benefit remain open engineering work. The released design and verified software are a starting point for those experiments, not a substitute for them.

## References

- ST UM1724, Nucleo-64 MB1136 manual, connector table and virtual COM: https://www.st.com/resource/en/user_manual/um1724-.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Littelfuse MDSR-7 series, switch geometry and logic-level application: https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7
- ICL CES, established punched-tape parity and reading methods: https://iclces.uk/articles/reading_punched_paper_tape.html
- ETSU numerical-control programming notes, historical tape parity: http://faculty.etsu.edu/hemphill/entc3710/nc-prog/nc-prog-04.htm

Manufacturer and historical source extracts were retrieved during design review. Verify current complete part drawings and application guidance before purchasing or assembling components; indexed availability and maximum switching ratings are not guarantees for this fixture.
