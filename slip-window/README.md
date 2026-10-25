# Slip Window — interrupted-drain notebook for slip-casting cradles

Author and creator: jayis1

## Purpose and overview: why this should exist

A small ceramics studio often learns a casting recipe by making several similar pieces and comparing the fired results. The potter fills a plaster mold, waits while the mold draws water from the slip, tips out the remaining liquid, leaves the mold draining, returns it upright, and eventually opens it. A phone timer remembers when somebody touched its screen. It does not remember that the cradle was lifted twice during draining, that a student returned it upright briefly, or that the mold latch was opened before the recorded drain operation finished. Those distinctions matter when comparing apparently identical trials.

Slip Window is a low-voltage, camera-free recorder for a manually operated casting cradle. It observes upright and inverted drain endpoints, the mold-latch position, and an explicit fill-complete marker. It separates casting residence from drain occupancy, records the longest uninterrupted drain segment, counts interruptions, and measures subsequent upright rest. The result is a contact-state notebook, not a ceramic quality verdict. The intended user is a studio potter, teaching technician, or small-batch mold developer who wants to compare handling history with independently measured wall thickness and defects.

A successful user session produces a local CSV showing that one labeled casting trial spent a particular time between fill marking and first tipping, followed by distinct drain and rest intervals. The potter still judges the clay, moisture, mold condition, and appropriate release time. A dedicated device is justified because its sensors stay attached to the fixture and continue measuring while the user has wet hands, while a phone is elsewhere, or while the desktop application is disconnected. A spreadsheet remains useful for analysis, but cannot observe those physical changes unaided.

The build targets an electronics cost ceiling of approximately USD 65 excluding the existing cradle, computer, and custom mounting labor. This is a planning target rather than a supplier quotation. There is no subscription, radio pairing, cloud account, camera, microphone, or automatic actuator. The instrument neither controls the cradle nor requires changes to the structural mold support. Its value hypothesis is fewer unexplained trial-to-trial timing differences, not guaranteed reductions in scrap.

## Originality and neighboring designs

All 84 existing top-level device READMEs were inventoried by title, and ceramic, mold, and slip-casting terminology was searched before selecting this concept. FoldTrace is the nearest process recorder: it checks an ordered sequence of folding leaves and their minimum dwell. Slip Window instead decomposes one reversible cradle trajectory into residence, accumulated draining, longest continuous draining, interrupted returns, and upright rest. It has no folding recipe or pass/fail dwell threshold. Pick Witness observes shuttle arrival, shed selection, and beating on a handloom; its textile-event sequence and user workflow do not describe casting. Siphon Witness records paired vessel thresholds rather than mold orientation, operator fill marking, and latch release.

The passive four-contact carrier and MCU support code deliberately reuse jayis1's existing Pick Witness engineering. Reusing a documented resistor network does not make it a new sensor technology. The proposed distinction is the casting-specific measurement contract and interruption-aware notebook. Stopwatch applications, general industrial data loggers, and casting timers are obvious conceptual prior art. Worldwide novelty, patentability, and absence of competing products have not been established. The repository-level comparison is not a substitute for a patent search.

Two alternatives were rejected. A liquid-level electrode in the slip could automate the fill mark but would add cleaning, corrosion, and material-dependent thresholds. A camera that watches the mold could avoid contact wiring but would require clear sightlines around hands and buckets and introduce unnecessary image capture. This revision uses an honest manual marker and mechanically observable endpoints instead.

## Hardware specifications and fixed interface contract

The processor is an STM32F401RET6 in LQFP64 on an external NUCLEO-F401RE, MB1136 board. It runs from its reset-default 16 MHz HSI oscillator. The board supplies its own regulator, decoupling, reset circuitry, SWD access, and ST-LINK USB virtual COM bridge. These circuits are part of the purchased development board, not fictitious components on the passive carrier schematic. The delivered carrier is a two-layer, 1.6 mm FR-4 board with an 80 by 70 mm outline, through-hole headers, and axial resistors. It is connected by short wires rather than being a mechanically compatible Nucleo shield.

| Carrier connection | Net and MCU mapping | Physical meaning when closed |
|---|---|---|
| J1 pin 1 | VCC from Nucleo 3V3 | Regulated 3.3 V only |
| J1 pin 2 | GND | Common USB/Nucleo ground |
| J2 pin 1 | SENSE0/PA0, Nucleo A0 | Upright cradle endpoint |
| J2 pin 2 | SENSE1/PA1, Nucleo A1 | Inverted drain endpoint |
| J2 pin 3 | SENSE2/PA4, Nucleo A2 | Mold latch in closed position |
| J2 pin 4 | SENSE3/PB0, Nucleo A3 | Fill-complete marker held |
| J3 through J6 pin 1 | CONTACT0 through CONTACT3 | Individual contact signal |
| J3 through J6 pin 2 | GND | Contact return |

Each channel has a 10 kilohm pull-up from SENSE to VCC and a 1 kilohm series resistor between SENSE and CONTACT. A dry contact joins CONTACT to ground. Nominal closed voltage is 0.30 V and contact current is 0.30 mA at 3.3 V. All four closed channels draw 1.20 mA, dissipating approximately 3.96 mW in the complete resistor network. Those are circuit calculations, not bench measurements. Qualify the selected contacts at this low wetting current; a high-current oxidized switch is not automatically an acceptable substitute.

A practical sensor option is a normally-open Littelfuse MDSR-7 reed switch in an insulated protective pocket, with a captive magnet on the corresponding moving feature. The manufacturer describes this family as a 12.7 mm normally-open glass-envelope switch; procure a sensitivity grade that works in the actual mount and establish the operating distance experimentally. The glass must never carry structural load. Mount upright and drain reeds on the fixed cradle frame with separate magnet targets, mount the latch reed on the stationary latch bracket, and operate the fourth reed with a spring-return manual magnet lever. Reed closure indicates position, not latch force or actual liquid presence.

| Item | Quantity | Selection / budget allowance |
|---|---:|---|
| NUCLEO-F401RE | 1 | ST board, USD 20 allowance |
| R1, R3, R5, R7 | 4 | 10 kΩ axial 1%, 0.25 W, DIN0207 |
| R2, R4, R6, R8 | 4 | 1 kΩ axial 1%, 0.25 W, DIN0207 |
| J1, J3–J6 | 5 | 2-pin 2.54 mm vertical headers |
| J2 | 1 | 4-pin 2.54 mm vertical header |
| Contact sensors | 4 | MDSR-7 family plus insulated mounts; USD 12 allowance |
| Carrier, resistors, headers | 1 set | USD 15 small-quantity allowance |
| Cable, magnets, enclosure | 1 set | USD 18 allowance |

The BOM uses footprint dimensions rather than an invented assembled board part number. Check the actual axial body and lead pitch against the selected resistor. The carrier footprints use a 7.62 mm resistor lead pitch and standard 2.54 mm header spacing. Current stock, regional prices, magnet grade, and supplier tolerances are not verified procurement guarantees.

## Architecture and block diagram

```text
upright reed ----- J3 -- 1k -- SENSE0 -- PA0
                          +-- 10k -- 3V3
inverted reed ---- J4 -- identical network -- PA1
latch reed ------ J5 -- identical network -- PA4
fill marker ----- J6 -- identical network -- PB0
all contact returns ------------------------ GND

NUCLEO-F401RE / STM32F401RE
    1 ms SysTick -> contact qualification -> casting state machine
                          |                         |
                          +---- continuity checks --+
                                                    |
                       bounded SW1 encoder -> USART2 PA2 TX
                                                    |
                                      onboard ST-LINK USB VCP
                                                    |
                          Linux Tk app: connection / notebook / history
                                                    |
                                        explicit local CSV export
```

The passive board has no charger, battery, regulator, power switch, radio, USB receptacle, or analog measurement channel. The computer powers the Nucleo over its ST-LINK USB connector. PA2 USART2 TX must remain routed to the onboard virtual COM bridge. Consult UM1724 before changing solder bridges; its default VCP route uses SB13 and SB14 on, with SB62 and SB63 off. Do not connect an additional transmitter to this signal. The app only receives telemetry; there is no remote start, erase, or release command.

| Power state | Design allowance | Meaning |
|---|---:|---|
| Carrier contacts all open | approximately zero DC resistor current | Excludes leakage |
| Carrier contacts all closed | 1.20 mA at 3.3 V | Calculated worst combination |
| Whole Nucleo plus carrier | reserve 150 mA at USB 5 V | 0.75 W planning budget, not measured |
| Computer disconnected or USB off | no operating session | Volatile records lost |

No battery-life claim is made. The MCU stays active because this is a bench instrument, not a wearable. The internal watchdog uses the independent low-speed oscillator with a nominal multi-second recovery period; its oscillator tolerance means it is not a precision deadline source. The HSI clock also requires measurement if absolute timing accuracy matters. For example, an assumed 2% clock error would represent 72 seconds over one hour; this illustration is not a guaranteed specification at every voltage or temperature. Calibrate elapsed time against a traceable reference before comparing close recipes.

## Mechanical installation and safety envelope

Use the recorder only with a stable, manually operated cradle in a dry, supervised workshop. Mount the electronics above and away from slip buckets. Route short signal/ground pairs, initially no longer than 0.5 m, with drip loops and strain relief. Provide an insulating enclosure and a removable cover over the carrier. There are no mounting holes in this PCB revision: use insulating board-edge guides or clips, not a conductive screw through a trace. Keep magnets captive so they cannot fall into clay or create a loose small-part hazard.

Set the upright and inverted sensors so their operating zones cannot overlap at any cradle angle. Confirm this with an unpowered continuity meter before attaching the MCU. The latch sensor must open when the latch is released, but must not interfere with the mold restraint. The marker should be out of the splash path and require a deliberate action after filling. Label cables physically because the firmware cannot identify which dry contact has been plugged into which socket. A reversed two-wire dry-contact pair is electrically equivalent, but a swapped sensor function is not.

The carrier has no galvanic isolation, surge suppression, or industrial field-input protection. Its series resistors are not a license to connect powered signals. Never connect mains, a powered kiln controller, a PLC output, thermocouple wiring, a motor, or an unknown existing machine circuit. USB ground is shared with the computer. Avoid live electrical equipment, wet worktops, and long external cable runs. ESD, EMC, enclosure ingress, vibration, and occupational safety certification have not been performed. The system must never serve as an interlock or substitute for safe handling of heavy molds.

## Firmware details and design decisions

The firmware is freestanding C11 with its own reset vector, data initialization, linker script, GPIO register setup, SysTick handler, watchdog, and bounded UART transmitter. It has no vendor SDK, RTOS, heap allocation, filesystem, or hidden package download. `registers.h` identifies the STM32F401 RM0368 register families and the Cortex-M SysTick addresses. Flash is linked at 0x08000000 with 512 KiB available, RAM at 0x20000000 with 96 KiB available, and a linker assertion preserves at least 4 KiB for the stack. Programming uses ST-LINK through OpenOCD, not a fictitious bootloader.

The cooperative main loop samples once for each newly observed millisecond. It does not fabricate missing samples when delayed. A sampling interval other than exactly one millisecond increments the gap counter, restarts contact qualification, and rejects an active cycle. Inputs must agree for twenty consecutive samples before changing the qualified mask. This whole-mask debounce deliberately ignores brief excursions; it is not a high-speed impact detector. Timing follows qualified states, not the exact mechanical switching instant. A bounce on the manual marker can delay qualification of another input because all four bits share the qualification window.

At boot CLEAR waits for upright, latch closed, marker released, and drain stop open for one second of qualified evidence. READY accepts a new rising marker edge only while upright and latched. CAST begins at that edge; residence ends when the upright endpoint is released. TURN waits for the inverted endpoint, bounded by a thirty-second transition limit. DRAIN accumulates total occupancy and the current uninterrupted segment. Leaving the drain endpoint completes the segment and enters RETURN or REST if already upright. Returning to the drain endpoint increments interruptions and starts a new segment. Longest-drain time is the maximum observed segment, not total drain time minus a guessed interruption duration.

REST accumulates only qualified upright occupancy after a drain. Another tip can resume draining and is explicitly counted as an interruption. A tip that returns upright without ever reaching the drain endpoint is rejected as NO_DRAIN rather than silently merging residence periods. Opening the latch is accepted as the end of an observed cycle only in REST while upright; opening earlier is OPEN_EARLY. Simultaneous upright and inverted signals are CONTRADICTION. A second fill marker during an active cycle is MARK_AGAIN. A cycle lasting twenty-four hours is rejected, and duration arithmetic checks overflow rather than wrapping a large interval into a small one.

DONE and REJECT preserve measurements until a newly marked cycle starts. They rearm after the same upright/latched/marker-released interval, so DONE is not a permanent screen and an error reason may remain visible alongside READY. The app shows both state and reason. Completed, rejected, sampling-gap, and transmit-loss counters are cumulative within a boot and saturate. All measurements are volatile. Disconnecting the desktop does not reset firmware, but removing USB power does. There is no automatic reconstruction of an interrupted casting session after reset.

## Application and software interface

The appropriate companion is a dependency-free Python Tk application for Linux rather than a radio-enabled mobile dashboard. Run `python3 app/app.py` from this directory with Python 3 and Tk installed. Connection lets the operator choose the serial device, reconnect, disconnect while retaining evidence, load an offline replay, and export CSV. Casting notebook presents the current state, reason, contact masks, residence, total draining, longest drain segment, upright rest, interruptions, and continuity counters. History displays every retained received status row, including several records delivered in one USB read. It retains at most one thousand rows; this is a rolling notebook, not unlimited archival storage.

The serial link is 115200 baud, eight data bits, no parity, one stop bit, without hardware flow control. Firmware emits a complete status record approximately every 250 ms. Format is `$SW1,<decimal fields>*<four uppercase CRC hex digits>\n`. CRC is CRC-16/CCITT-FALSE, initial value 0xFFFF, polynomial 0x1021, over bytes starting at `$` through the last digit before `*`. It is integrity checking, not authentication. The firmware rejects an insufficient output buffer; the transmitter abandons a stuck transfer after 100 ms and increments its loss counter. The app recognizes a new `$` start after an incomplete record and never grows an input frame past 256 bytes.

Field order is sequence, uptime_ms, state, reason, raw_mask, stable_mask, qualified, cycle_ms, residence_ms, drain_ms, longest_drain_ms, rest_ms, interruptions, completed, rejected, gaps, tx_loss. State numbers follow CLEAR=0, READY=1, CAST=2, TURN=3, DRAIN=4, RETURN=5, REST=6, DONE=7, REJECT=8. Reason numbers are NONE=0, OPEN_EARLY=1, CONTRADICTION=2, SAMPLE_GAP=3, MARK_AGAIN=4, TURN_TIMEOUT=5, CYCLE_TIMEOUT=6, COUNTER_LIMIT=7, BAD_INPUT=8, NO_DRAIN=9. Contact bits are upright=1, drain=2, latch=4, marker=8. `qualified` describes whether a stable baseline has been acquired; raw and stable can still disagree during debounce.

The parser checks protocol version, CRC, field count, numeric ranges, enumerations, drain invariants, sequence progression, uptime progression, and cumulative-counter rollback. A suspected reset or duplicate blocks further ingestion until a deliberate new session. Normal modular sequence and uptime wrap are accepted. There is no unique persistent boot identifier: a reset followed by sufficiently late attachment can remain indistinguishable if all observed fields progress plausibly. Explicitly reconnect after any known reset; this prototype cannot provide audit-grade continuity across power loss.

Records become stale after 1.5 seconds without a valid update. Disconnection retains their provenance and contents; reconnecting clears them before receiving new data. Replay is always labeled offline, never represented as physical live evidence. CSV creation is explicit, uses restrictive permissions, and refuses to overwrite an existing file. Exports include jayis1 attribution, source, connection state, parser errors, missing frames, blocked status, per-row age, and stale flags. No data leaves the computer. Export before replacing a session or before the rolling history evicts a row that matters.

## Build, validation, and bring-up

From the repository root run:

```sh
make -C slip-window/firmware all test
python3 slip-window/tests/test_app.py
xvfb-run -a /usr/bin/python3 slip-window/tests/test_gui.py
/usr/bin/python3 slip-window/tools/check_hardware.py
python3 slip-window/tools/check_release.py
```

The ARM GNU toolchain builds both ELF and binary artifacts. Host tests compile the actual engine and encoder with address and undefined-behavior sanitizers. A separate C fixture emits synthetic casting telemetry through the real wire encoder, which is then consumed by the Python tests and real Tk event loop through a Linux pseudo-terminal. Those tests are simulation, not physical UART, USB, or casting measurements. The GUI checks connection, multiple screen tabs, batched history, stale indication, disconnect, reconnect clearing, unplug handling, and replay.

KiCad 9 files include authored symbols, actual library footprints, routed copper, explicit board outline, project metadata, and a connectivity manifest. `generate_hardware.py` reproducibly generates the carrier with the system KiCad Python bindings. `check_hardware.py` runs actual ERC and DRC, exports a fresh schematic XML netlist, and compares every reference, pin, net, value, and footprint with the PCB and manifest. Its passive schematic boundary must be understood: a clean ERC does not verify Nucleo power selection or installation wiring. The rendered schematic is also inspected for label and reference overlap.

Before assembling, inspect the actual board in KiCad, print it at scale, and verify connector orientation. Assemble resistors first, measure resistance with power absent, inspect solder joints, and confirm no VCC-to-GND short. Power the Nucleo alone, verify 3.3 V with a meter, then attach the carrier and measure both open and closed input levels. Program with `make -C slip-window/firmware flash` only after verifying the connected target is the intended NUCLEO-F401RE. Exercise each contact independently and compare the reported bit mask. Next test all legal transitions with an empty mold, deliberately introduce each fault, and confirm that sampling resets and disconnected cables do not look like successful cycles.

Finally run controlled casting trials with known fill markers and an independent reference timer. Measure real contact bounce, timing drift, mounting repeatability, and the effect of each interrupted drain on the actual product. Define recipe-specific acceptance criteria with the potter only after these trials. This repository delivers a build-tested design prototype, not fabricated or production-qualified hardware. It does not measure wall thickness, slip viscosity, moisture, mold absorption, clamping pressure, dryness, or readiness to demold.

## References and attribution

- ST UM1724, STM32 Nucleo-64 MB1136 user manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Littelfuse MDSR-7 family: https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7

The MCU connector and VCP mapping was checked against the locally cached UM1724 manual because a fresh manufacturer download was unavailable during this run. All original design, firmware, application, tests, and documentation are credited to jayis1. Newly written firmware uses the repository's GPL-2.0 policy; reused support files retain their existing MIT notices. Python tools and application use MIT. Hardware follows the repository's CERN-OHL-S v2 policy. No third-party library source is republished or relabeled as original work.
