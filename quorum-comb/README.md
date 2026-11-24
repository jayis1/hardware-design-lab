# Quorum Comb — majority-versus-reference notebook for magnetic position fixtures

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original code, documentation and locally authored electrical designs are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing. Revision A is an unbuilt engineering prototype. Target compilation, synthetic software tests and electrical CAD validation do not establish physical performance, safe operation, measurement accuracy or manufacturing readiness.

## Purpose and useful distinction

Quorum Comb is a removable bench fixture for comparing three magnetic indications of the same mechanical position with a separate, mechanically operated reference contact. Three adjustable reed holders form the teeth of a comb beside a slow, hand-operated slider. A captive magnet on the slider operates those reeds near a selected endpoint. A separate cam on the slider operates a reference microswitch. The instrument records which reed is in the minority, how much qualified sampled time the three-input majority disagrees with the reference, and the longest uninterrupted observed segment of each disagreement type.

The practical problem is that adding redundant position switches can create unjustified confidence. Two poorly positioned magnetic sensors can agree with each other while missing the intended mechanical endpoint. A single minority lamp does not show whether that minority was a brief boundary effect or a persistent condition. Conversely, declaring the minority sensor broken confuses disagreement with truth. This notebook retains both internal agreement and agreement with a separately adjusted reference. It helps a fixture builder compare bracket spacing, magnet orientation and intentional common displacement without replacing those observations with a single reassuring green light.

The intended users are a teaching workshop explaining redundant sensing, a museum demonstrator builder, and a maker prototyping a noncritical removable position witness. A representative experiment first aligns all three reeds so a fully seated slider closes them, then shifts one reed outward. The minority screen identifies its observed disagreement. A second experiment shifts the entire reed bracket while leaving the reference cam unchanged. The majority can now be wrong relative to that reference even though the three reeds agree. A third experiment deliberately misadjusts the reference switch, showing that the supposed reference is itself an instrument requiring inspection, not an infallible truth source.

There is no actuator, relay, machine-enable signal, alarm output, wireless interface or remote command path. This is deliberately not a safety voter, interlock, aircraft controller, door-security sensor or certification instrument. It does not vote an output into physical service. All electrical inputs are detached, voltage-free contacts energized only by this carrier. Do not connect it to a powered industrial sensor, live machine wiring or mains-referenced circuit.

## Originality, repository comparison and prior art

The repository was pulled before implementation, all device directory names and README titles were inventoried, and related magnetic reader descriptions were inspected. The interrupted Quorum Comb directory was traced to an earlier attempt at this same task and recovered instead of creating another unfinished project. Parity Sled records parity excursions and their recovery on four-track teaching strips. Pattern Sled compares clocked words against a learned sequence. Neither compares three redundant witnesses with a heterogeneous endpoint reference. Quorum Comb does not decode a strip, track parity, learn a reference word or infer a physical movement direction.

Majority voting itself is established technology and is not claimed as an invention. The retrieved paper “Modeling Common Cause Failures in Systems with Triple Modular Redundancy and Repair,” DOI https://doi.org/10.1109/rams48030.2020.9153662, explicitly addresses the limitations common-cause failures impose on redundancy. The proposed distinction here is the inexpensive adjustable mechanical comb plus separate exposure denominators and a retained minority/reference comparison workflow. No worldwide novelty, patentability or freedom-to-operate claim has been established. A logic analyzer is preferable for electrical bounce, and a calibrated displacement instrument is preferable for actual position metrology.

The platform drivers, startup, linker, passive carrier routing and POSIX transport derive from jayis1's Touch Tour at b47eaff4728f3325fa57c5e382106d2017561c01. The import script documents initial provenance and refuses overwrites; it is not a build step. The majority engine, QC1 schema, companion interpretation and experiments are specific to this design. Reusing a carrier does not imply reusing its earlier measurement contract.

## Hardware specifications

| Function | Revision A implementation |
|---|---|
| MCU module | External ST NUCLEO-F401RE, MB1136, STM32F401RET6 Cortex-M4 |
| MCU memory | 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz, explicitly restored before peripheral setup |
| Witness inputs | Three external normally open Littelfuse MDSR-7-10-15 reed switches |
| Reference input | External Omron D2F-01L low-current lever microswitch, COM/NO contacts |
| Carrier | 80 × 70 mm, two copper layers, 1.6 mm FR-4; passive components only |
| Acquisition | Nominal 1 ms GPIO observations, independent qualification per input |
| Transport | USART2 TX PA2 AF7, 115200 baud, 8N1 via ST-LINK virtual COM port |
| Power | Nucleo USB supply; carrier takes only regulated 3.3 V and ground |
| Companion | Python 3.13 with Tk on Linux; standard library only |
| Persistent storage | None on device; manual CSV export on host |
| Battery / radio | None |

The processor, reset network, supply regulator, decoupling, programmer and USB bridge are on the purchased Nucleo board, not absent components on an alleged standalone processor PCB. The custom board has fourteen actual components: eight resistors and six headers. It includes authored schematic symbols, numbered pins, net labels, actual footprint assignments and routed copper. The checked connectivity manifest is a design aid, not a substitute for the native schematic and PCB files.

## Architecture and block diagram

    Hand slider with captive magnet        Separate endpoint cam
             |                                       |
       reed A / reed B / reed C                  COM/NO reference
          J3       J4       J5                         J6
             \      |      /                           |
              four 1k series / 10k pull-up networks
                              |
                         J2 SENSE0..3
                              |
                  PA0 / PA1 / PA4 / PB0 inputs
                     external NUCLEO-F401RE
                              |
               independent five-interval qualification
                         /                  \
            three-voter clean time     four-input clean time
            A/B/C minority ledgers     two reference mismatch ledgers
                         \                  /
                        cumulative QC1 + CRC16
                              |
                    PA2 UART -> ST-LINK USB
                              |
               comparison / history / CSV desktop app

Each sense node has a 10 kilohm pull-up to 3.3 V and a 1 kilohm resistor between that node and its external contact. The other contact terminal connects to ground. An open contact reads high; firmware converts a closed contact to logical one. At nominal values a closed path draws 0.300 mA and places the GPIO node at 0.300 V. Four closed paths draw 1.200 mA, or 3.960 mW, excluding all Nucleo consumption. These are circuit calculations, not measurements. Verify the selected contacts' low-current suitability and the MCU input thresholds before assembly; old oxidized power switches are unsuitable substitutions.

J1 pin 1 is 3.3 V from Nucleo CN6 pin 4; J1 pin 2 is ground from CN6 pin 6. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. J3, J4, J5 and J6 each have signal on pin 1 and ground on pin 2. J3 is reed A, J4 reed B, J5 reed C and J6 the reference. Bit zero through bit three follow that order. Use the connector numbers printed on the PCB, not an assumed cable color order.

Reference documents are ST UM1724 for the MB1136 board and ST RM0368 for the STM32F401 register map. Links: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf and https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf. A fresh manufacturer PDF retrieval timed out during this recovery; the board mapping and register definitions are inherited from the existing tested platform. Reconfirm the exact purchased board revision and VCP solder-bridge configuration against its manufacturer documents before wiring. No external HSE source is required by this firmware.

## Mechanical fixture and assembly

Use a nonconductive base approximately 180 by 100 mm and a guarded hand slider with approximately 40 mm travel. Provide mechanical stops independent of the microswitch. A suggested slider body is 25 mm wide, with a fully captured small magnet near its endpoint-facing side. These are starting fabrication dimensions, not validated magnetic geometry. Fit three independently adjustable insulated reed sleeves to a removable bracket beside the endpoint. Each sleeve needs transverse and longitudinal adjustment so the operator can introduce an individual offset without moving its neighbours. Label A, B and C permanently.

Place the reference microswitch on a separate bracket actuated by a nonmagnetic cam. Its mounting should not move when the magnetic comb is shifted. Adjust actuation inside the manufacturer's permitted travel and let the mechanical stop absorb hand force. Protect the lever and all pinch points. Reed glass must be supported without transferring bending force from its leads into the envelope. Retain the magnet mechanically and enclose it so it cannot detach or be swallowed. This is a supervised bench fixture, not a children's toy.

The electronic BOM is four 10k and four 1k axial DIN0207 resistors, 1% tolerance and at least 0.125 W; five two-pin 2.54 mm headers; one four-pin header; matching insulated leads; the carrier PCB; one NUCLEO-F401RE; USB cable; three specified reeds and one reference switch. Use resistor lead spacing of 7.62 mm to match the footprints. Place the carrier in an insulating enclosure with strain relief and standoffs; no mounting holes are provided, so use edge-retaining supports rather than drilling through unknown copper.

There is no isolation, reverse-polarity protection or dedicated ESD clamp on this passive carrier. USB ground is common to every contact. Keep cables short, preferably below 300 mm, and enclosed. Wire with power removed, check J1 polarity and continuity, inspect for solder bridges, then power from a current-limited development setup. Never drive the carrier from a second supply while connected to the Nucleo. The series resistor is not a general overvoltage protector. Any field-deployed version requires a separate protection and EMC redesign.

## Firmware measurement contract

The MCU uses direct register definitions rather than a vendor SDK. Startup initializes data and BSS, establishes the stack and SysTick vector, and enters main. Clock restoration has bounded polling before UART is enabled. GPIO pins remain inputs and configuration is checked. The independent watchdog is enabled with a bounded initialization wait. Main services nonblocking UART output, acquires whenever the millisecond counter changes and emits a complete cumulative snapshot about every 100 ms when the transmitter is idle. There are no interrupts except SysTick and no shared multiword measurement updates in an interrupt handler.

Each channel requires five unchanged sampled intervals, meaning six observations establish its first qualified endpoint. The first qualified endpoint receives no retrospective evidence credit. A voter interval is counted only when all three voter channels were qualified, raw and qualified values matched at both endpoints, and the raw three-bit word did not change across that interval. Reference comparison additionally requires the reference channel to meet those conditions. Reference chatter therefore cannot erase otherwise usable minority evidence. Any raw voter glitch interrupts voter evidence even when qualification filters it out.

For each credited voter interval, majority means at least two of A, B and C are closed. At most one voter can be in the minority. The first three ledgers accumulate each identity's minority time. The final two ledgers accumulate majority-closed/reference-open and majority-open/reference-closed time. Every ledger retains total credited intervals, observed segment count, maximum segment and current run. Unknown intervals clear current runs but do not erase accumulated totals or maxima. An observed segment is not necessarily a unique physical fault: a rejected glitch or missed acquisition can split one continuous physical condition into multiple segments.

When time advances by more than one millisecond, the engine increments gap and missed counts, clears current runs and restarts qualification. It does not invent observations for missing ticks. Cumulative evidence survives. Repeated or backwards time, invalid input masks, counter exhaustion and startup configuration failures produce a terminal fault. A staged copy of the engine makes every sampled counter update transactional; failure cannot publish half an interval's evidence. Terminal acquisition fields and timestamps freeze. Ordinary 32-bit timestamp wrap is supported provided successive accepted observations are less than half a timer cycle apart.

All counters are volatile and disappear on reset. No flash endurance or interrupted-write protocol is needed because firmware does not write flash. There is no persistent boot identifier. A host can reject obvious rollback but cannot prove uninterrupted identity when a reset happens to produce later-looking values. Start a new named experiment after every reconnect or target reset. Host status must not be used as an authentication guarantee or as evidence that a captured result came from a particular physical sensor.

## Protocol and companion software

QC1 is receive-only ASCII, one newline-terminated record per snapshot. Fields are unsigned decimal with no leading zeros except zero itself. The exact sequence is now, samples, intervals, gaps, missed, tx_errors, raw, qualified, valid, fault, voter_known, compare_known, followed by total, segments, peak and run for ledgers zero through four. The payload begins with QC1 and comma separators. An asterisk introduces four uppercase hexadecimal CRC digits. CRC is CCITT-FALSE, polynomial 0x1021, initial value 0xffff, covering the payload before the asterisk. The encoder reserves a terminator and refuses insufficient output capacity; host frames are bounded to 1024 bytes.

The desktop has Witness comparison and Acquisition history screens. It connects to a user-selected Linux serial path, rejects a second connection while a port is owned, shows qualified and raw masks, presents all five ledgers and separate coverage counts, and retains up to 2000 snapshots. Replay accepts only regular files no larger than 2 MiB, rejects malformed frames and partial EOF tails, and never labels replay as live. Replacing evidence requires disconnecting first. Failed replay or failed connection setup preserves the previous evidence rather than displaying a partially loaded session.

CSV export uses exclusive creation with private file permissions and refuses overwriting an existing file. A metadata row credits jayis1 and carries source, connection state, freshness, parser error count and latched session error even if no measurement rows exist. Disconnect preserves source provenance. Exports contain bounded recent history rather than an unlimited raw recorder; cumulative counters still include the device's earlier observations. Save separate files with a fixture description, magnet orientation and operator notes when comparing runs.

CRC detects accidental corruption, not malicious modification. The host also checks counter conservation, episode bounds, monotonic growth and elapsed acquisition. Sparse snapshot checks are necessary consistency bounds, not a proof of all unreported samples. Freshness requires advancing acquisition, a connected port, no fault and recent progress. A fixed one-second host/device progress budget rejects sustained backlog or clock jumps. Uncalibrated HSI drift can eventually trigger conservative reconnection even with a healthy device. There is no cloud account, pairing secret, remote update endpoint or command authorization surface.

## Build, run and verification

From the repository root, run `make -C quorum-comb/firmware all test fixture`. The target is ARM Cortex-M4 Thumb, freestanding C11, linked against compiler support routines only. The Makefile enables warnings as errors, includes the actual startup and linker script, emits ELF and binary images, and runs host engine, UART and clock tests with address and undefined-behavior sanitizers. `fixture` emits deliberately synthetic QC1 records for all sixteen stationary input words. That recording must never be represented as physical measurement data.

Run `python3 quorum-comb/tests/test_app.py` for parser, semantic, replay and POSIX serial tests. Run `xvfb-run -a /usr/bin/python3 quorum-comb/tests/test_gui.py` for the real Tk event loop with a paced pseudo-terminal carrying the C encoder's output. Start the interactive companion with `/usr/bin/python3 quorum-comb/app/main.py`; Tk may need installation as the distribution's python3-tk package. No pip package or network connection is required. Flashing, when hardware is available, uses `make -C quorum-comb/firmware flash` with OpenOCD's ST-LINK and stm32f4x configurations. Flashing was not performed in this run.

Regenerate CAD with `/usr/bin/python3 quorum-comb/tools/generate_hardware.py` using KiCad 9 Python bindings. Validate with `/usr/bin/python3 quorum-comb/tools/check_hardware.py`. This runs native schematic ERC and board DRC, exports a fresh XML netlist and compares every reference, pin, net, value and footprint against actual PCB pads and the manifest. Passive ERC cannot validate the external power source, contact behavior or correct Nucleo cable installation. See VALIDATION.md for executed versions and results.

## Bring-up and remaining physical work

Before interpreting disagreement, verify each connector independently with a detached contact and inspect the displayed raw bit. Confirm all sixteen stationary combinations with a switch jig if available. Then assemble the slider, mark reference actuation and release locations, and record magnetic actuation separately in both travel directions. Reed hysteresis, magnet spread, bracket flex and switch lever hysteresis can all produce expected boundary disagreement. Repeat slow passes with the same geometry before intentionally introducing offsets.

The next acceptance steps are physical fabrication inspection, current and GPIO-level measurement, actual target programming, contact and cable fault injection, UART timing measurement, reference repeatability measurement, and supervised workflow trials. The current design does not establish those results. If all three reeds agree while the reference disagrees, inspect both the comb and the reference rather than automatically condemning either. The useful output is an inspectable disagreement notebook with explicit unknown coverage, not an unearned assertion that redundancy made the mechanism correct.
