# Snap History — reversible four-fastener closure notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original source, documentation and locally authored hardware designs are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier for a separately purchased NUCLEO-F401RE. Target linking, synthetic contact histories, Linux application operation and electrical CAD checks are reproducible. No board has been fabricated, no MCU programmed, no fabric fixture assembled, and no physical engagement, timing accuracy or usability measured. This design is not a medical instrument, wearable product, fastener strength tester or safety interlock.

## Purpose and overview

Snap History helps a textile product designer examine how four closures on a tabletop fabric mock-up are manipulated. It records the entire bounded sequence of qualified contact changes, including opening a fastener that had already been closed. Changes detected at the same sampled instant occupy one group rather than acquiring an invented alphabetical order. After all four contacts remain raw and qualified closed for a quarter second, the notebook freezes. Its useful result is a reversible history, not merely a final completion time or a claim that the garment was fastened correctly.

Consider a designer comparing two placket layouts for a small equipment pouch. Both layouts can eventually be closed. One may encourage a user to close the middle fasteners, discover an alignment problem, open the first one again, and then finish. A stopwatch collapses those paths into one duration. A final photograph loses the intermediate reopening entirely. Video can preserve the behavior but requires annotation, and the hands can obscure the closure points. Snap History supplies a limited complementary observation that can be compared with written notes and video rather than replacing them.

The intended prototype is deliberately not worn. Four removable closure stations are sewn or clamped to a fabric strip on a tabletop board. Each station has a captive magnet on one flap and a normally-open reed switch in an insulated pocket on the other. The switch reports proximity in the chosen closed position. It does not report snap engagement force, mechanical retention, correct mating, or fabric tension. A user may bring the flaps close enough to close a reed without engaging the actual fastener; that limitation must be measured and recorded during fixture development.

The main audience is product prototypers, makerspaces and textile interaction educators. Possible uses include comparing closure spacing, exploring whether a guide tab reduces reopening, and teaching why equal final states do not imply equal interaction histories. Results must not be used to diagnose dexterity, score a person's disability, certify protective clothing, or authorize an equipment enclosure to operate. Record a fixture identifier instead of participant names wherever possible. Consent and appropriate study governance are separate responsibilities if a later project involves human subjects.

## Originality and prior art

The hardware-design-lab directories and README concepts were inventoried before this proposal. Nest Settle measures locating-nest seating, full-contact dwell and corner dropout aggregates. Domino Ledger retains the first departure of each hinged leaf and a return mask. Grip Lattice measures contact-combination occupancy durations during a separate gate interval. Snap History instead preserves every qualified changed vector in a bounded closure attempt, including repeated reopening and reclosing of the same identity. It imposes no preferred closure order and preserves sampled ties. This is a different evidence contract, not a renamed first-event or duration histogram.

The passive carrier and low-level platform are reused from jayis1's Grip Lattice at repository commit f8c5c05ea6775205257e8bc539437a019a141439. GPIO acquisition, clock restoration, the serial transmitter, reset startup and carrier routing are conventional shared infrastructure. The new contribution is the textile-mock-up experiment, bounded reversible-history state machine, SH1 snapshot protocol, history model, desktop interface and associated tests. Reuse avoids an unnecessary new USB or power subsystem and leaves the electronics independently inspectable.

A bounded external search found SensorSnaps, which integrates wireless sensing into snap fasteners for gestures and motion. It also found research comparing the manipulation of blouse closures using observed movement and lap times. Fastener sensing and timed clothing-interaction studies therefore are established prior art. The proposed distinction here is a low-cost, wired, offline history of repeated closure-vector changes with explicit ties, finite capacity and preserved incomplete evidence. No patent search or worldwide novelty determination has been performed; absence of an equivalent commercial or hobby product has not been proven.

References consulted or required for assembly:

- SensorSnaps: https://doi.org/10.1145/3332165.3347913
- Blouse-closure wearing/removal study: https://cir.nii.ac.jp/crid/1390282679184407936
- Clothing-fastener manipulation thesis: http://hdl.handle.net/1993/16359
- ST Nucleo-64 UM1724, MB1136 connector map and virtual-COM connection: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STM32F401 RM0368 for GPIO, RCC, USART, watchdog and SysTick definitions used in firmware/registers.h and drivers/clock.c.
- Littelfuse MDSR-7 series for the proposed external reed-switch variant: https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7

## Hardware specifications

| Function | Revision A contract |
| --- | --- |
| MCU | STM32F401RET6 on external NUCLEO-F401RE MB1136 |
| Core and memory | Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal nominal 16 MHz HSI, explicitly restored before I/O |
| Acquisition | Foreground samples on nominal 1 ms SysTick changes |
| Qualification | Five consecutive equal samples per channel |
| Sensors | Four normally-open voltage-free reed contacts |
| Interface | USART2 TX on PA2 AF7 through ST-LINK USB virtual COM |
| Link | 115200 baud, eight data bits, no parity, one stop bit |
| Supply | Nucleo USB supply; regulated 3.3 V and ground to carrier |
| Custom PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Application | Linux Python 3 with Tk, standard library only |
| Outputs | Telemetry only, no actuator, command receiver or radio |

The carrier is cable connected. It is not a plug-on Arduino shield and does not contain a hidden microcontroller. The development board supplies its regulator, decoupling, reset circuitry, programming interface, USB bridge and boot configuration. These are external purchased functions. Do not populate an unlisted MCU or connect a bare chip in place of the Nucleo. Confirm the board revision and solder-bridge settings against its manufacturer manual before construction.

### Pin and net contract

J1 pin 1 is VCC, connected to Nucleo CN6 pin 4, regulated 3V3. J1 pin 2 is GND, connected to CN6 pin 6. J2 pins 1, 2, 3 and 4 are SENSE0 through SENSE3, connected respectively to CN8 pins 1 through 4: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled header pins are used as digital inputs. J3 is SNAP_A, J4 SNAP_B, J5 SNAP_C and J6 SNAP_D. Each field connector has signal on pin 1 and common ground on pin 2. The names match board.h, schematic, connectivity manifest and application setup screen.

Each SENSE node has a 10 kilohm pullup to VCC and a 1 kilohm series resistor to its field contact. Closing the contact to ground pulls the GPIO low; firmware inverts this into a logical closed bit. The nominal ideal closed voltage is 0.30 V and the nominal excitation current is 0.30 mA per channel at 3.3 V. Four closed channels draw 1.20 mA. These are circuit calculations, not measurements. Reserve 150 mA at USB 5 V for initial system planning; actual Nucleo current depends on assembly and must be measured. There is no battery or charger.

| Item | Quantity | Specification |
| --- | --- | --- |
| R1, R3, R5, R7 | 4 | 10k ohm, 1%, axial DIN0207, 7.62 mm pitch |
| R2, R4, R6, R8 | 4 | 1k ohm, 1%, same mechanical package |
| J1, J3, J4, J5, J6 | 5 | 1x2 straight 2.54 mm through-hole header |
| J2 | 1 | 1x4 straight 2.54 mm through-hole header |
| Processor assembly | 1 | ST NUCLEO-F401RE, external |
| Reed contacts | 4 | Proposed Littelfuse MDSR-7-10-15, external |
| Magnet assemblies | 4 | Captive, insulated, adjusted against the selected reeds |
| Fabric fixture | 1 | Tabletop four-station mock-up, no worn installation |

The sensor sensitivity designation does not establish the installed detection distance. Verify current manufacturer specifications and availability before procurement. Large power switches with oxidized contacts are not appropriate substitutes merely because a continuity tester sometimes reads them; this circuit supplies little wetting current. The hardware generator uses installed KiCad standard through-hole footprint geometry and locally authored passive symbols with explicit numbered pins.

### Electrical and mechanical limits

Use only disconnected, voltage-free contacts with short insulated leads, initially below 0.5 m. The inputs are not isolated from USB ground. There is no field-rated ESD protection, reverse-polarity protection or overvoltage clamp on the carrier. The series resistors are not safety barriers. Never attach the inputs to powered clothing electronics, mains equipment, a machine controller or another supply. Power down before rewiring and use a current-limited USB source during bring-up. A miswired supply can damage the development board and host computer despite a passing CAD report.

Start with a rigid backing approximately 300 by 120 mm and four interchangeable fabric stations spaced about 60 mm apart. These dimensions are proposed fixture targets, not calibrated requirements. Mount the PCB in an insulated enclosure with stand-offs or nonconductive edge retention; the present carrier has no mounting-hole pattern. Add strain relief where sensor leads exit the enclosure. Insulate glass reeds against bending and impact, and fully capture all small magnets so they cannot become loose ingestion hazards. Do not expose the prototype to children or use it around implanted medical equipment.

The first fixture experiment must compare magnetic closed-zone indication against actual physical fastening. Check fully open, nearby but unlatched, properly latched, sideways misaligned, and deliberately lifted positions. If unlatched proximity is indistinguishable, label the result flap-proximity history rather than engagement history. If the sensor pockets alter handling or fabric stiffness materially, redesign or abandon the measurement. Software cannot repair that mechanical measurement bias.

## Architecture and block diagram

```text
Four fabric flap magnets and insulated voltage-free reed switches
                         |
                   J3 .. J6
                         |
           1k series + 10k pullup per channel
                         |
                J2 SENSE0 .. SENSE3
                         |
        NUCLEO-F401RE PA0 / PA1 / PA4 / PB0
                         |
         independent five-sample qualification
                         |
       WAIT -> READY -> ACTIVE -> DONE or HALT
                         |
        retained 32-entry timestamp/vector history
                         |
       SH1 CRC snapshot -> USART2 PA2 -> ST-LINK USB
                         |
         Linux framing + history validation + Tk UI
                         |
              local exclusive-create CSV export

USB -> purchased Nucleo regulator -> J1 3V3/GND -> carrier
```

No network service or smartphone permission is needed. Host commands cannot change thresholds or clear a trial because the firmware enables only transmission. A physical MCU reset is the deliberate new-notebook action. Export the result first, open all four stations, reset, reconnect the application, and wait for READY. The inability to remotely reset is intentional: a careless application action cannot silently erase evidence on the device.

## Firmware behavior and design decisions

The reset routine initializes static memory and vectors, then main restores HSI and checks the intended GPIO/USART configuration. The watchdog uses the independent low-speed oscillator and bounded initialization waits. A clock-restoration failure stops before the interface initializes; a configuration or watchdog-start failure creates a HALT snapshot. SysTick increments a millisecond counter, while the foreground loop polls the transmitter and samples inputs when that counter advances. There is no heap allocation, dynamic task creation, interrupt-driven contact timestamping or vendor runtime dependency.

WAIT requires all four raw and qualified inputs open at both endpoints of 250 consecutive acquired intervals. A unit powered while contacts are closed does not treat those contacts as a started attempt. Five initial samples establish per-channel qualification, so READY first appears after 255 consecutive open samples in the host fixture. Any raw disturbance during the baseline restarts the interval credit. READY waits indefinitely for the first qualified nonzero vector. That vector starts ACTIVE at offset zero and becomes event group one.

During ACTIVE, every changed qualified vector produces one immutable event entry. The entry stores the elapsed trial milliseconds and the complete new four-bit closed mask. A group can include several closures, several openings, or both; the application derives closed and opened identity sets by comparing consecutive vectors. The first preceding vector is defined as all open. Repeated manipulation of the same identity is retained rather than collapsed into a first-event time or return flag. Independent debounce lets one stable fastener transition qualify while another contact chatters.

Settlement requires all four raw and qualified inputs closed at both ends of each interval. The first fully closed endpoint earns no preceding time. A raw glitch cancels settlement even when it is too short to change the qualified mask or produce an event. This is why the last event timestamp alone cannot always predict the eventual completion timestamp. At 250 credited intervals, DONE freezes trial age, event count, qualified mask and the complete event array. Later raw levels and telemetry uptime can change, but opening the mock-up afterward does not rewrite the completed notebook.

A finite trial has a rejecting deadline at age 60000 ms. Deadline checking precedes both event append and settlement, so an otherwise completing sample exactly at that boundary becomes a timeout instead. The log holds 32 changed-vector groups. A 33rd group causes HALT with capacity fault; the first 32 groups remain untouched and the triggering vector is not falsely claimed as recorded. Acquisition discontinuity also halts instead of stretching a debounce window across missing samples. Unsigned timestamp subtraction permits normal millisecond-counter wrap, but a skipped or repeated acquisition tick is not treated as continuous evidence.

States are WAIT=0, READY=1, ACTIVE=2, DONE=3 and HALT=4. Faults are none=0, sample gap=1, counter exhaustion=2, configuration=3, watchdog startup=4, timeout=5 and capacity=6. First fault wins. Transport errors are counted with overflow protection. A trial cannot automatically restart, even after all stations reopen. The finite history remains available through repeated snapshots until physical reset or power removal. There is no persistent storage; power loss destroys device evidence, making regular local exports important.

## SH1 application interface

Each line starts with `SH1,`, contains canonical unsigned decimal fields separated by commas, then an asterisk, four uppercase hexadecimal CRC digits, and LF. The CRC is CRC-16/CCITT with polynomial 0x1021 and initial value 0xffff over the bytes from S through the last decimal digit, excluding the asterisk and suffix. Fields are now, phase, fault, raw, mask, valid, age, quiet, count, tx_errors, followed by 32 pairs t0,m0 through t31,m31. Masks use bit zero for A through bit three for D. Timestamps and quiet intervals are nominal milliseconds. Unused event pairs must both be zero.

Snapshots repeat about every 100 ms, including idle and terminal states, so a late-attaching application can recover the retained trial without receiving its first frame. Firmware encodes only internally valid structures into a bounded 768-byte buffer. The host bounds individual frames to 512 bytes and input chunks to 65536 bytes. It checks CRC, exact field count, numeric syntax, state relationships, event ordering, terminal settlement and unused entries. SH1 is the compatibility marker; unknown protocol data is not silently interpreted as an older device.

The host additionally checks transitions between accepted snapshots: time cannot regress within the modular half range, the phase cannot roll back, event prefixes cannot change, new groups cannot be backdated into an already observed ACTIVE interval, and terminal trial evidence cannot move. These are necessary consistency checks, not a cryptographic proof of what occurred between sparse snapshots. There is no authentication and no persistent boot identifier. A reboot whose resulting data looks forward-consistent can be indistinguishable; explicitly reconnect after every intentional reset and never describe this as tamper-evident evidence.

Run the application from this directory with `python3 app/workbench.py`. Linux needs Python's Tk package and permission to open the USB serial device, usually through the distribution's serial-device group. Do not run the application as root solely to bypass permissions. The Closure History screen shows grouped offsets, newly closed identities, reopened identities and resulting occupancy. The Setup and Limits screen provides wiring and the trial procedure. Connect opens the entered serial path at 115200 baud, flushes pending host input and starts a new local session only after opening succeeds. A failed replacement retains existing evidence.

Freshness requires advancing device time, not merely repeated reception. Duplicate identical frames do not renew freshness. Live acquisition is marked stale after a receive pause, and a subsequent long host pause or accumulating host/device time divergence blocks the session until reconnect. The fixed 500 ms drift allowance is deliberately conservative and can reject a long otherwise valid run with an uncalibrated oscillator. LIVE means telemetry progression, never physical validity; inspect DONE versus HALT and the fault description separately.

Replay accepts only regular files up to 2 MiB, rejects empty input and partial final frames, and is always labelled replay. CSV export uses exclusive file creation with account-private permissions and refuses overwrite. The first record preserves author, protocol, provenance, connection state, freshness, parser-error count, warning and complete latest snapshot even when no event exists. Following rows contain the derived event history. Disconnect retains provenance and results but clears freshness. No data is uploaded, no credential is needed, and no hidden demonstration values are shown as live readings.

## Build, verification and physical commissioning

From this device directory run `/usr/bin/python3 tools/verify.py`. The script builds and links the STM32 target, runs C tests with address and undefined-behavior sanitizers, generates a synthetic recording using the production encoder, runs model/transport/Tk tests under Xvfb, and checks electrical CAD. Required installed tools are GNU Arm Embedded GCC/binutils, host GCC, Python 3 with Tk, Xvfb, KiCad 9 CLI and KiCad's Python bindings. The project has no downloaded application dependency tree or package lock because the application uses only the Python standard library and system Tk.

For an isolated firmware build use `make -C firmware all`; `make -C firmware test` exercises host behavior. Programming, when a real assembled target is available and wiring is inspected, uses `make -C firmware flash` with OpenOCD's ST-LINK and STM32F4 configurations. This design run does not invoke that target. Build artifacts and local KiCad preference files are ignored rather than committed. The reproducible hardware generator is available for design changes, but regeneration is not necessary for normal checks of the delivered CAD files.

The electrical check exports a fresh XML schematic netlist and compares every component reference, value, footprint and numbered pin/net against the PCB and connectivity manifest. ERC and DRC must report zero violations, and PCB checking must report zero unconnected items. Passive symbols cannot prove that the external supply is correct, that the installed reed has adequate wetting behavior, or that a human connected the harness correctly. Passing these checks is not a fabrication or regulatory certificate.

Software acceptance includes all 65536 four-step contact-mask words against an independent previous-vector history, held-closed startup, arbitrary reopening, simultaneous changes, unrelated-channel chatter, raw-glitch settlement cancellation, exact timeout, event-capacity exhaustion, timestamp wrap, encoder capacity limits and real serial-driver timeout behavior. Python tests cover corrupt and impossible records, immutable history, stale sessions, replay limits, PTY transport restoration, and real C-produced records through the Tk read loop. The synthetic fixture is explicitly software-generated; it is not an experiment performed on fabricated hardware.

Before a physical pilot, inspect resistance and shorts with power disconnected, verify J1 polarity, measure the regulated supply, and confirm all four input levels one contact at a time. Flash only after those checks. Correlate events with slow manual switching and an independent timing reference, then test misalignment and nearby-but-unlatched states. Repeat the same closure word several times to identify fixture variation. Record supply, board revision, sensor position, fixture identifier and any anomalies alongside each export. Only after these steps should the designer decide whether the notebook answers a useful question or whether a different sensing method is required.
