# Catch Witness — settled-latch reopening association notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, software and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint material retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier for an external NUCLEO-F401RE, not a standalone processor board. Target linking, simulated software behavior and electrical CAD are verifiable. Fabrication, programming, electrical bring-up, switch adjustment, enclosure construction and physical latch performance remain unverified. Nothing here is an access-control system, safety interlock, force gauge or certification instrument.

## Purpose, audience and practical value

Catch Witness is a removable bench instrument for a cabinet hardware repairer evaluating a small mechanical catch on a detached demonstration panel. A catch can appear closed without the bolt actually seating. It can seat and later release after a handle operation, release after a deliberately declared tug, or reopen with neither event observed. A single door contact merges those cases into one opening count. Catch Witness keeps them separate, including closures that never reached the settled condition and episodes that ended without an observable result.

The intended user currently makes pencil notes while repeatedly closing a panel, touching the release lever and pulling the panel edge. That is adequate for a handful of trials, but becomes difficult when comparing several strike positions or spring adjustments. The proposed benefit is an explicit account of what was observed before reopening, without a camera pointed at the operator and without modifying the latch's load-bearing path. It does not automate the test, measure load or eliminate the need for a force gauge.

A representative session begins with the detached panel open, bolt indication clear and both declaration switches released. The repairer closes the panel and waits for LATCHED on the companion. They press the separate tug paddle before pulling the panel edge. If the door opens shortly afterward without a release-handle indication, the challenged count increases. After adjusting the strike, they repeat the procedure and export the two sessions separately, documenting their applied-force method outside this instrument. Counts alone do not prove improvement: different tug forces, switch positions or samples can explain a difference.

Hardware is useful because the observer must independently see panel position, bolt engagement and release-handle movement while the user's hands are occupied. A phone timer cannot infer those events. A phone camera might capture them with suitable lighting and annotations, but then requires an unobstructed view and manual interpretation. This carrier deliberately trades visual detail and force information for cheap, directly inspectable switch evidence. The practical hypothesis is fewer mislabeled manual trials, not improved intrinsic latch strength.

## What is proposed as new

The specific concept is a settled-latch reopening notebook with expiring, edge-triggered association tags. It does not equate every closed panel with a retained latch, and it does not let a held handle contact excuse every subsequent opening. A fresh handle edge earns a bounded association only after the latch was already settled. A separate paddle records a declared challenge without pretending to measure force. The retained outcome ledger distinguishes unseated openings, handle-associated openings, tug-associated openings, unexplained openings, timeouts and observation aborts.

This is repository-level differentiation, not a claim of worldwide originality or patentability. The inventory includes HingeScribe, which concerns hinge effort, angle and condition, and SealBeat, which concerns refrigeration seals and thermal performance. FoldTrace checks ordered folding leaves; Nest Settle concerns seating corners. Detent Loop measures analog force–travel loops on sliding catches; it does not distinguish reopening associations from separate panel, bolt and handle indications. Catch Witness instead studies a single latch episode and the temporal evidence surrounding reopening. It reuses the repository's passive four-contact electrical platform because the sensor interface is not the proposed invention. The state machine, expiration policy, app interpretation and bench procedure define this design's distinct purpose.

Commercial latch testing already exists. The [Southco E4 test data sheet](https://media.southco.com/media/static/TestDataSheet/TD-E4-01-J.pdf) distinguishes engagement, disengagement, tensile and push-out loads, illustrating that latch performance is multidimensional. The [ANSI discussion of ANSI/BHMA A156.2](https://blog.ansi.org/ansi/ansi-bhma-a156-2-2022-bored-locks/) describes operational force and other lock/latch tests. Catch Witness does not perform those standardized tests. Its low-cost alternative is a local event notebook alongside an appropriate manual test method; a laboratory force-displacement stand remains the better choice when quantitative retention is required. No customer study or professional prior-art clearance has been performed.

## Hardware specifications

| Item | Revision A contract |
|---|---|
| Processor assembly | NUCLEO-F401RE, MB1136, STM32F401RET6 in LQFP64 |
| Processor configuration | Cortex-M4 at reset HSI 16 MHz, 512 KiB flash, 96 KiB SRAM |
| Sensor interface | Four normally-open voltage-free low-level contacts |
| Input semantics | DOOR, BOLT, HANDLE, TUG, active when contact closes |
| Example contact | Omron D2F-01L low-level lever switch; validate mechanical suitability |
| Carrier | 80 × 70 mm, two copper layers, 1.6 mm FR-4, through-hole parts |
| Communications | USART2 TX PA2 to ST-LINK virtual serial port, 115200 8N1 |
| Power | Nucleo USB input, regulated 3.3 V and common ground to carrier |
| Sampling | Nominal 1 ms; five consecutive samples qualify each input |
| Settlement | 200 observed milliseconds of closed panel and engaged bolt, handle clear |
| Association lifetime | Inclusive 500 ms after an eligible qualified rising edge |
| Episode deadline | 30,000 ms from qualified door closure; deadline wins over opening |
| Rearm | 50 observed milliseconds of all inputs raw and qualified clear |
| Outputs | Telemetry only; no lock, motor, solenoid or alarm-control output |

The Nucleo supplies processor decoupling, voltage regulation, reset circuitry, debugger, programming connection and USB bridge. Those parts are not duplicated on the carrier. The KiCad schematic accurately depicts the passive board rather than drawing an imaginary MCU into an otherwise disconnected project. J1 and J2 use short jumper cables; the board is not a mechanically mating Arduino shield. Preserve the Nucleo's default ST-LINK USART2 routing and check the actual board revision before wiring.

### Wiring and power contract

| Carrier connection | External connection |
|---|---|
| J1.1 VCC | Nucleo CN6.4 regulated 3V3 |
| J1.2 GND | Nucleo CN6.6 GND |
| J2.1 SENSE0 | CN8.1 A0 / PA0 / DOOR |
| J2.2 SENSE1 | CN8.2 A1 / PA1 / BOLT |
| J2.3 SENSE2 | CN8.3 A2 / PA4 / HANDLE |
| J2.4 SENSE3 | CN8.4 A3 / PB0 / TUG |
| J3.1, J4.1, J5.1, J6.1 | Respective switch signal |
| J3.2, J4.2, J5.2, J6.2 | Respective switch return to common GND |

Each SENSE net has a 10 kΩ pull-up to VCC and a 1 kΩ series resistor to its switch connector. At nominal 3.3 V, a closed switch draws 0.30 mA and the sensed low voltage is 0.30 V; four closed inputs draw 1.20 mA. These are ideal resistor calculations, not measured limits. Large power switches may be unreliable at this wetting current. Use low-level contacts and verify switching thresholds with the assembled circuit.

The carrier adds no radio or battery. Budget approximately 100 mA at the USB supply for the development-board assembly as a conservative planning allowance, not a measured consumption result. Carrier resistive consumption is under 4 mW nominal with all contacts closed, excluding the external module. Provide one USB supply only. Do not backfeed 3.3 V, connect mains, connect powered latch wiring, or attach to a vehicle. The series resistors do not provide galvanic isolation or industrial surge protection. The USB host and all switch returns share ground.

### Estimated bill of materials

A planning budget is USD 20–35 for the NUCLEO-F401RE, USD 3–8 for a small carrier PCB, USD 2–5 for eight resistors and six headers, USD 8–20 for suitable contacts and the declaration paddle, and USD 10–25 for cables, insulated supports and a simple enclosure. These are rough allowances rather than supplier quotes and exclude shipping, the latch, tools and labor. Reuse of an existing Nucleo makes the experiment cheaper. A four-channel logic analyzer plus a hand-built harness is the nearest inexpensive functional alternative; it offers finer timing but requires the operator to interpret traces rather than receiving this fixed episode classification.

## Architecture and block diagram

```text
Detached manual panel and catch
  door stop switch ----- J3 DOOR ----\
  bolt witness switch -- J4 BOLT -----+-- 1k series / 10k pulls
  release handle ------- J5 HANDLE ---+         |
  tug declaration ------ J6 TUG ------/         v
                                        J2 PA0/PA1/PA4/PB0
USB ---> NUCLEO-F401RE ---- 3V3/GND ---> J1 carrier power
             |
             +-- 1 ms acquisition -> independent qualification
             |                      -> settlement / bounded association
             |                      -> conserved outcome ledger
             +-- USART2 PA2 -> ST-LINK VCP -> Linux desktop
                                             Connection / Retention / History
                                             bounded replay + private CSV
```

The hardware has no authority over the mechanism. Removing USB power must leave the test panel freely operable. Electrical data acquisition and serial transmission are separate: a stalled serial output is counted without blocking the input loop. The desktop can close or disconnect without changing firmware policy. There is no remote configuration, incoming command parser, cloud account or network server.

## Mechanical arrangement and scope

Mount the latch on a small detached panel supported on a stable bench. Use a fixed panel-position stop and an independently adjustable bolt witness. DOOR should close only at the selected seated panel position; BOLT should close only after the intended engagement travel. A flag or insulated follower may operate the bolt switch, but must not obstruct bolt motion or become part of the retention load path. HANDLE should observe the release lever, not the same panel movement observed by DOOR. TUG is a separate momentary declaration paddle: it says a challenge was declared, not that one actually occurred.

Use slotted brackets with locking fasteners so the switches can be adjusted and then marked. Keep test forces small and controlled. Mechanical stops must absorb travel rather than the switch bodies. Provide strain relief at both ends of every cable and keep initial sensor leads shorter than 0.5 m. The PCB has no mounting holes; use insulated edge supports in an enclosure with clearance below solder joints. Allow at least 90 × 80 mm for the carrier plus separate space for the Nucleo and USB bend radius. No exact enclosure CAD or universal latch adapter is claimed, because the bracket geometry depends on the specimen.

The initial experiment should compare every indicated state with direct observation before counting useful trials. A closed switch does not prove engagement force. A misadjusted follower can create a convincing but false LATCHED indication. A broken lead can look like an open switch and a short can look closed. The instrument has no wire-supervision resistors at the far end and cannot distinguish those faults from legitimate states. Never apply the resulting classifications to fire doors, occupied enclosures, security doors, fall protection, machine guards or other safety functions.

## Firmware behavior and design decisions

The firmware is freestanding C11 without a vendor SDK or allocator. `registers.h` identifies the STM32F401 register addresses; `drivers/clock.c` restores and verifies the HSI clock with bounded waits before enabling the interface. `startup.c` initializes data and BSS, installs core vectors and enters `main`. `drivers/board.c` samples active-low GPIO, supplies SysTick and starts the independent watchdog. `drivers/serial.c` provides a fixed buffer, busy rejection, nonblocking byte transmission and a 100 ms transmit timeout. All external sensing pins remain inputs.

Reference documents are the [STM32F401 RM0368 manual](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) and [MB1136 UM1724 manual](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf), especially the NUCLEO-F401RE connector table. Verify any hardware substitution against those documents; a similarly named Nucleo board is not automatically compatible.

Each input qualifies independently after five matching acquired samples. Qualification does not move the timestamp back to the first raw edge. Brief bounce can be filtered, but sampling cannot detect transitions entirely between acquisitions. WAIT and DONE require all four raw and qualified contacts clear before READY. A qualified DOOR rising edge in READY starts SETTLING. Initial attachment to an already closed panel cannot invent a trial because the open baseline must occur first.

SETTLING accumulates only intervals whose raw and qualified endpoints show both DOOR and BOLT, with HANDLE clear at both endpoints. A one-sample raw interruption resets settlement even if it never changes the debounced state. After 200 qualifying intervals the state becomes LATCHED and the settled counter increases once. A door opening before that point increments unseated. Bolt loss after LATCHED is recorded separately and does not erase the historical fact that settlement was previously observed. Repeated bolt losses can exceed the number of episodes.

Only a qualified rising HANDLE or TUG edge during an already established LATCHED state earns a credit. An edge during SETTLING, including the exact sample that completes settlement, is ignored. This deliberately requires the challenge declaration to follow settlement rather than overlap it. A held input does not renew its credit. Credits expire after 500 ms; opening exactly at age 500 retains association, while age 501 does not. A qualifying handle or tug edge tied to the opening sample is too late. Handle association takes precedence when both credits exist. These names are temporal classifications, not causal proof of why the latch opened.

The first qualified DOOR opening ends the episode. Outcomes are intentional (handle-associated), challenged (tug-associated without handle credit), unexplained, or unseated. At 30 seconds a timeout wins even if an opening qualifies on that sample. There is no claim that a timeout means successful retention. Sampling discontinuity greater than one millisecond halts correlation permanently for that boot, abandoning an active episode exactly once. Counter exhaustion also halts with transactional rollback. Further timestamp heartbeats do not make halted acquisition valid. Reboot begins a new volatile session and requires a fresh open baseline.

## Wire protocol and software interface

The firmware attempts a snapshot every 100 ms. A line is `CW1`, comma-separated unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE uses initial value 0xffff and polynomial 0x1021 over the payload before the asterisk. The standard `123456789` check is 0x29b1. Fields in order are:

```text
now state levels valid starts settled intentional challenged unexplained
unseated timed_out aborted gaps faults tx_errors age dwell handle_age
tug_age handle_credit tug_credit bolt_losses
```

State numbers are WAIT=0, READY=1, SETTLING=2, LATCHED=3, DONE=4 and HALT=5. Input bits are DOOR=1, BOLT=2, HANDLE=4 and TUG=8. Fault bits are acquisition gap=1, counter exhaustion=2, configuration=4 and watchdog startup=8. The timestamp wraps modulo 32 bits. Age and dwell are milliseconds; completed episode age remains until the next start. Credit ages are zero when no credit exists. Policy constants are fixed by CW1; changing them requires a protocol revision rather than silently reinterpreting old recordings.

The companion uses Python's standard library and Tk on Linux. Launch `python3 app/workbench.py` from this directory. The Connection screen opens an explicitly selected serial path, disconnects while retaining evidence, loads a recording or exports CSV. Retention shows the latest state and counters including faults and gaps. History shows every retained snapshot up to the 4,096-row bound. Replay is explicitly not live. CSV creation is exclusive, uses private permissions, refuses overwrite, and includes author, provenance, connection state, warnings, parser errors and reception age even when there are no results.

CRC provides corruption detection, not authentication. The parser rejects malformed decimal values, oversized frames, impossible episode totals, invalid credit states and partial replay tails. Replay accepts regular files only and has a 2 MiB limit. Session checks reject backward timestamps, decreasing counters and changed terminal evidence. These are necessary consistency checks, not reconstruction of all hidden edges between snapshots. Some resets remain indistinguishable without a persistent boot identifier; reconnect intentionally starts a new host session.

Serial timestamps cannot prove absolute acquisition freshness. The app therefore says acquisition age is unverified, even while receiving advancing ticks. It refuses continued interpretation after a one-second reception pause and uses a fixed 1.5-second host/device progress discrepancy limit. Those guards can conservatively reject long sessions with oscillator drift. They cannot authenticate the source or remove an initial constant transport delay. HALT is prominently labeled acquisition invalid regardless of continued serial heartbeats. Export before reconnecting when retained evidence matters.

## Building, testing and validation plan

Run `make -C firmware all test` with GNU Arm Embedded tools and native GCC installed. The target link uses the supplied startup and linker script and creates `firmware/build/catch-witness.elf` and `.bin`; host tests use address and undefined-behavior sanitizers. `python3 tools/verify.py` additionally generates synthetic recordings with the real C encoder, runs the desktop tests under Xvfb, checks source attribution and minimum documentation/code counts, and runs the KiCad connectivity checks. Dependencies are system tools, not downloaded application packages: Python 3, Tk, Xvfb, GCC, arm-none-eabi tools, KiCad 9 and its Python bindings.

The generated hardware contains real resistor and connector symbols, assigned footprints, a closed outline and routed nets. `tools/check_hardware.py` runs ERC and DRC and compares each schematic pin with the board pads and connectivity manifest, including values and footprint identifiers. Passive ERC cannot validate the external module, cable polarity or switch installation. The hardware generator is included for reproducibility, not required to build firmware. Build products and local KiCad preferences are ignored by version control.

Before physical use, inspect the assembled board, verify continuity without power, check that no supply pin is shorted, and confirm each switch maps to exactly one intended bit. Program only a known NUCLEO-F401RE with the supplied optional `make -C firmware flash` target after reviewing its OpenOCD command. No programming is performed by the validation script. Compare timing against a logic analyzer and a controlled switching fixture, then evaluate bracket repeatability and operator interpretation. A useful pilot is a documented set of deliberate unseated, handle-associated, challenge-associated and unexplained trials with independent observation. No such physical trial has been performed for this revision.

Detailed executed results and remaining limits are recorded in VALIDATION.md. File-level verification is a reproducibility milestone, not evidence that the instrument measures latch strength or that a retained specimen is safe.
