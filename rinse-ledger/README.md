# Rinse Ledger — basket immersion and water-batch exposure notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources: MIT.

Revision A is an engineering prototype. The delivered custom PCB is a routed passive contact interface used with a separate NUCLEO-F401RE. It is not a standalone microcontroller board. Target linking, host tests and electrical design-rule checks are reproducible; fabrication, flashing, chemical performance and physical timing have not been verified. This is an observation tool, never a certification that a photograph has been adequately washed.

## Purpose, users and the actual problem

Rinse Ledger serves small analogue-photography teaching studios that wash several groups of test prints in a manually refreshed bath. The instructor needs to distinguish a print basket that spent most of a lesson submerged from one repeatedly lifted for inspection. More subtly, equal total immersion does not mean equal exposure to successive water batches. A basket inserted late may have seen only the last batch even though another basket has visited three. A stopwatch attached to the tray remembers elapsed wall time, not which basket was actually lowered during each batch.

The device observes three independently movable baskets and a deliberate water-replacement marker. Each basket has a dry contact that closes at its lowered stop. A fourth contact is a momentary pushbutton operated after the user has removed all baskets and replaced the water. Firmware accumulates total qualified immersion, immersion in the current declared batch, continuous immersion streak, longest observed streak, entry count and the number of declared batches visited. The companion notebook shows those distinctions without a camera, microphone, cloud account or connection to the chemistry.

The practical purpose is process comparison and teaching. An instructor can discover that two nominally identical groups received different handling, explain why a timer continued while a basket was on the bench, or distinguish repeated dipping in one batch from visits to several fresh batches. A craft photographer can keep a local record alongside paper type, chemistry, temperature and independent residual-chemical tests. Those additional observations are not measured by this device and must never be silently inferred from its counters.

Why hardware rather than a phone application? A person could log every movement manually, but doing so while holding a dripping basket is awkward, and forgetting a tap produces apparently precise but false durations. Removable contacts observe the fixture directly and leave the operator's hands free. Why now? Low-cost development boards and a local serial interface make a small experimental instrument feasible without a custom radio, battery charger or expensive data-acquisition system. No new sensor technology is claimed. The original contribution is the conservative basket/batch accounting workflow and its explicit handling of ambiguous water changes and missing observations.

## Originality and boundaries

The repository was inventoried before creation, including device titles and purpose excerpts. The nearest existing liquid-workflow device is Siphon Witness, which records threshold transfers between two vessels; it does not allocate three basket exposures among operator-declared water batches. Slip Window concerns casting residence and interrupted draining, rather than independent baskets sharing successive bath identities. Rinse Ledger reuses a proven passive four-contact electrical topology and board-support code authored by jayis1, but implements a different accounting model and application.

External comparison found commercial automatic print washers, including Paterson's basket-agitation washer, and published DIY continuous-flow washers. These establish that washing baskets and water replacement are existing ideas. This design does not claim to invent print washing, replace commercial archival washers, or establish global patent novelty. Its narrower proposed distinction is a removable three-basket observation ledger for a manual batch workflow. A preliminary web comparison is not an exhaustive product search, legal opinion or freedom-to-operate study.

This revision does not measure water flow, water level, temperature, conductivity, fixer concentration, print saturation or chemical diffusion. A lowered basket can still be above an insufficient water level. A pressed button can be dishonest or mistaken. A basket may contain several prints with different histories. The instrument cannot detect any of these cases. Use the same labeled material in each basket throughout a session and keep an independent handling sheet; exchanging contents mid-session invalidates the association between a counter and a photograph.

## Hardware specifications and architecture

| Item | Revision A implementation |
| --- | --- |
| Processor assembly | NUCLEO-F401RE, STM32F401RET6, Cortex-M4 |
| Clock | Reset HSI, nominal 16 MHz; no external oscillator configuration |
| Memory | 512 KiB flash and 96 KiB SRAM |
| Inputs | Three lowered-basket dry contacts plus one water-change button |
| Acquisition | Nominal 1 ms sampling, independent 8 ms qualification |
| Input network | 10 kohm pull-up and 1 kohm series resistor per channel |
| Interface | USART2 TX, 115200 baud, 8 data bits, no parity, one stop bit |
| Host bridge | Existing ST-LINK virtual COM port over USB |
| Power | USB powers Nucleo; its 3.3 V rail supplies the passive carrier |
| PCB | 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Assembly | Through-hole axial resistors and 2.54 mm vertical headers |
| Application | Python 3 / Tk desktop, Linux serial transport, no pip packages |

Architecture and signal direction:

    Basket A lowered contact -- J3 -- 1k -- SENSE0 -- PA0 / A0
    Basket B lowered contact -- J4 -- 1k -- SENSE1 -- PA1 / A1
    Basket C lowered contact -- J5 -- 1k -- SENSE2 -- PA4 / A2
    Water-change pushbutton --- J6 -- 1k -- SENSE3 -- PB0 / A3
                                          |
                                      10k pull-up
                                          |
                                         3V3

    USB -> NUCLEO-F401RE -> qualified contact vector -> batch accounting
                               -> CRC-framed UART -> ST-LINK VCP
                               -> local exposure screen / notebook / CSV

J1 pin 1 is 3.3 V from Nucleo CN6 pin 4, and J1 pin 2 is ground from CN6 pin 6. J2 pins 1, 2, 3 and 4 connect to CN8 pins 1, 2, 3 and 4, respectively: A0, A1, A2 and A3. J3 through J6 each have signal on pin 1 and ground on pin 2. The contact closes those two pins. There is no voltage output intended for powering a sensor at the contact connector. A closed contact produces a low electrical input; firmware inverts it to a one bit.

The delivered schematic has fourteen real component instances, numbered symbol pins, footprint assignments, wires and shared net labels. The PCB has matching pads and named nets, routed tracks, a ground via and a closed Edge.Cuts boundary. The local symbol library is supplied. The hardware generator writes a component and connectivity manifest that is independently compared against fresh schematic netlist export and actual PCB pad connectivity. Shared net labels are intentional electrical connections, not decorative text.

The processor, regulator, decoupling, reset circuitry, USB bridge and SWD connection are on the external Nucleo assembly. They are not falsely represented as components on the passive board. The standard MB1136 virtual-COM solder-bridge arrangement is required: SB13 and SB14 fitted, SB62 and SB63 open. PA2 is configured for AF7 USART2 transmit. Receive is disabled. Confirm the exact Nucleo revision against UM1724 before wiring or modifying solder bridges.

## Mechanical fixture, power and safety

Use an adjustable nonconductive bracket above the wet tray, with a switch or reed contact actuated only when its assigned basket reaches the repeatable lowered stop. Sensors and connections remain dry; the basket itself may be wet. Secure magnets mechanically if using reeds, protect leads from strain, and make sure the bracket cannot trap a hand or knock over the tray. No dimensioned basket, enclosure or immersion-safe sensor assembly is supplied; the tray-specific fixture must be engineered and tested locally. The carrier is a bench interface, not a waterproof appliance.

Choose gold-contact low-current signal switches or suitable reed contacts rather than oxidized power switches. The nominal closed-contact current is 3.3 V divided by 11 kohm, calculated as 0.300 mA. The sense voltage in that state is approximately 0.300 V. Four closed contacts consume 1.200 mA from the 3.3 V rail. Open contacts have essentially no resistor-network DC consumption, excluding leakage. These calculations do not include the processor board, its LEDs or ST-LINK.

Reserve 100 mA at 5 V as a planning allowance for the Nucleo assembly, not a measured consumption claim. Use its documented USB powering arrangement with a regulated source capable of at least 500 mA. The firmware does not implement sleep, and no battery or runtime estimate is supplied. For a small one-off prototype, the board, contacts, resistors, headers, fabrication and mechanical fixture are likely to dominate cost; obtain supplier quotations rather than treating a guessed total as a procurement budget.

All electrical connections share the USB host ground. There is no galvanic isolation, external surge suppressor, enclosure ingress rating or verified ESD immunity. Start with short twisted signal/ground pairs no longer than half a meter and test interference susceptibility. Never connect a powered washer's control wiring, mains circuits or another instrument's energized input. The series resistors are not a safety barrier. Keep the computer, Nucleo, carrier and USB connections away from splashes and provide drip loops and strain relief. Follow the chemistry manufacturer's handling and disposal instructions independently of this design.

The desktop display is not a photographic safelight. Use the system only where the paper's documented light-handling requirements permit it, for example after the relevant fixing stage in an appropriate wet work area. Do not assume a red software theme would protect undeveloped photographic material. No medical, potable-water, food-processing, sterilization or safety-interlock use is supported.

## Firmware state and accounting decisions

At boot the engine waits until every contact has qualified open. This establishes that all baskets are raised and the marker is released. A button held during power-up cannot manufacture a water batch. The ready state means that contact accounting may proceed, not that water is clean. Batch identity begins at zero. No immersion time is credited until the operator makes the first accepted water-change declaration. A basket lowered before that declaration receives a sticky uncertainty flag.

A rising qualified marker edge advances the batch identity only when every basket was out both before and after that accepted vector. A marker arriving in the same qualified vector as extraction is rejected because the observation cannot establish that replacement happened after extraction. Likewise, insertion coincident with a marker is rejected. A held button produces one edge rather than repeatedly adding batches. Every accepted marker clears each basket's current-batch duration but preserves lifetime session totals, longest streaks, entry counts and distinct batches visited.

Each basket's entry counter advances on an observed qualified insertion. Its baths-visited counter advances only on its first insertion into a particular declared batch. Removing and reinserting the same basket in the same water increases entries but not baths visited. Duration accrues to the previously qualified state for the interval ending at the current sample. Removal ends the current streak, while the longest streak remains retained. These are durations of accepted switch states, not measurements of wet paper surface area or continuous chemical contact.

Each contact has its own candidate level and qualification age. Eight milliseconds of repeated candidate observations are required after the first sample of a changed level. At nominal one-millisecond polling a clean edge is therefore accepted on its ninth sample. An unrelated contact cannot postpone another contact's acceptance. Short glitches are rejected; switch hysteresis, mounting compliance and asymmetric bounce remain measurement limitations. Qualified time can include the debounce tail following physical extraction, so it must not be called a guaranteed lower bound on actual immersion.

A sampling interval greater than four milliseconds is an acquisition gap. The unknown interval receives no time credit. All basket uncertainty flags become sticky, streaks end, and the engine requires a fresh all-open baseline before resuming. Previously credited totals are retained. Recovery can split a physically continuous immersion into separate observed entries; entry counts are therefore not guaranteed physical incident counts after a gap. Flag 1 marks incomplete observation, and flag 2 marks saturated arithmetic. Both remain visible until hardware reset.

Counters and accumulated durations saturate at the unsigned 32-bit maximum rather than rolling back. Ordinary uptime rollover is handled by unsigned subtraction. A completely unobserved full timer cycle cannot be detected by that arithmetic alone, but the independent watchdog is intended to reset a stalled main loop long before it occurs. Exhausted batch identities inhibit further accounting instead of aliasing an old identity. Reset erases all totals; there is no flash journal, removable storage or persistent boot identifier.

Board initialization uses the actual STM32F401 register map in RM0368. Startup initializes data and BSS, and the linker describes the target memory and reserves stack margin. The independent watchdog starts with bounded oscillator and register-update waits; it is fed only after acquisition processing. If watchdog initialization fails, firmware reports no valid observations. UART transmission uses a bounded nonblocking queue with a 100 ms deadline. A transmit timeout increments losses and abandons the incomplete frame, allowing the receiver to resynchronize on the next complete protocol marker.

## Wire contract and desktop screens

Every snapshot is ASCII with this envelope:

    RL1,<31 comma-separated unsigned decimal fields>*<CRC16 in four uppercase hex digits>LF

The first ten fields are seq, ms, ready, levels, epoch, rejected, gaps, losses, debounce and max_gap. They are followed by seven fields for basket A, then the same seven for B and C: total, epoch_ms, streak, longest, entries, baths and flags. Durations are milliseconds. Levels bits zero through three are A, B, C and the marker. The policy values repeat 8 and 4 on every frame, making late attachment compatible without depending on a one-shot boot message. CRC-16/CCITT-FALSE covers RL1 through the final decimal digit, with polynomial 0x1021, initial 0xffff, no reflection and no final XOR. The maximum frame buffer is 512 bytes.

The firmware sends snapshots on observed level, readiness, batch or rejected-marker changes, and every 100 ms when the transport is available. A snapshot is state, not an event archive; intermediate states can be missed. Cumulative totals survive missed host frames while the MCU remains running, but an earlier per-batch total can be lost when a subsequent accepted marker clears it. Export before advancing if that exact boundary matters. The protocol has integrity checking but no authentication and must be treated as local untrusted input.

The application has three working screens. Basket Exposure shows each cumulative duration, current-batch duration, streak, longest streak, entries, bath visits and flags. Water-Batch Notebook retains the last 256 accepted snapshots and their batch identities. Setup and Limits explains the contacts, session workflow and uncertainty rules. The toolbar opens a specified Linux serial device, disconnects while preserving evidence, imports a bounded local replay and exports a CSV. No browser server or network listener is started.

Frames with invalid CRC, excessive length, bad policy, field overflow or inconsistent durations are rejected. A complete RL1 marker can recover after a truncated preceding frame. Sequence regression, time regression, cumulative counter rollback or disappearing sticky flags blocks further acceptance until an explicit new desktop session. Duplicate timestamps do not refresh freshness; contradictory data at one timestamp blocks the session. These checks are reset heuristics, not guaranteed boot detection. A reset whose values all appear forward remains indistinguishable without persistent identity.

Data older than one second is marked stale. Disconnect retains the original live or replay provenance, and offline replay is explicitly labeled. CSV export is exclusive-create with private permissions from creation and includes author, source, connection state, age, stale state, parser errors, missing-frame count and blocked status. Existing files are not overwritten. The retained rows are bounded rather than a complete session log; export regularly and distinguish missing frames from missing chemical information. A reconnect clears only desktop history, whereas a physical reset clears device accounting.

## Build, validation and bring-up

From the repository root run:

    make -C rinse-ledger/firmware check
    xvfb-run -a python3 -m unittest discover -s rinse-ledger/tests -p 'test_*.py'
    /usr/bin/python3 rinse-ledger/tools/check_hardware.py
    python3 rinse-ledger/tools/verify.py
    python3 rinse-ledger/app/app.py

GNU Arm Embedded GCC and Make build and link the Cortex-M4 firmware without a vendor SDK. GCC runs host engine tests with address and undefined-behavior sanitizers. Python with Tk supplies the desktop application; Xvfb provides a real headless display for GUI tests. KiCad 9 and its standard footprint libraries are required for generation and electrical checks. Use the distribution Python that provides pcbnew, here /usr/bin/python3. The committed CAD can be checked without regenerating it. The bootstrap helper is only a provenance-preserving initial source-copy helper; it does not replace the device-specific engine, protocol, application or tests.

Tests exercise independent qualification, separate basket accumulation, repeated visits, ambiguous marker order, held buttons, acquisition gaps, baseline recovery, uptime rollover, saturation and CRC bounds. A synthetic fixture generated by the actual C encoder feeds the production application parser. GUI tests use a Linux pseudoterminal and the actual Tk event loop, including connect, populated screens, unplug detection, replay and provenance retention. Passing those tests demonstrates software behavior for synthetic inputs, not fabricated hardware or validated photographic outcomes.

The hardware checker runs schematic ERC and board DRC with violation exit codes, exports a fresh netlist and compares every component reference, value, footprint and pin/net assignment against actual PCB pads and the manifest. Passive ERC cannot validate the external processor assembly or sensor mounting. Inspect the assembly unpowered, check resistor values and connector orientation, and meter VCC-to-ground resistance before applying USB power. Measure open and closed input voltages, then flash through ST-LINK using the documented Makefile target only after verifying the attached board. Physical flashing has not been performed for this revision.

Begin with dry jumper tests, then a dry basket fixture, then supervised wet trials with electronics safely separated. Compare timestamps against a logic analyzer, deliberately introduce bounce, remove USB, and verify the desktop marks stale and disconnected evidence. Establish sensor repeatability before using records for any process comparison. Record independently measured temperature, paper and chemistry alongside exports. A qualified photographic-conservation process must determine acceptable washing and verify residual chemicals; this device intentionally has no green clean-enough indicator.

## References and attribution

- STMicroelectronics UM1724, Nucleo-64 MB1136 user manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- STMicroelectronics RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Paterson Auto Print Washers, existing basket-agitation product comparison: https://patersonphotographic.com/product/auto-print-washers/
- DIY Darkroom Print Washer, existing continuous-flow approach: https://shootitwithfilm.com/diy-darkroom-print-washer/

Original firmware, application, tests, local symbol definitions and documentation are by jayis1. Reused board-support sources and carrier generation originated in jayis1's pedal-window device. Standard KiCad footprint assets retain their upstream library licensing; no claim of original authorship over third-party footprint geometry is made. See VERIFICATION.md for executed checks and remaining physical validation limits.
