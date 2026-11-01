# Balance Order — deadband-aware three-coupon balance notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software, documentation and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The delivered PCB is a routed passive carrier for a separately purchased NUCLEO-F401RE, not a standalone microcontroller board. Software target linking, simulated acquisition, desktop workflows and electrical CAD can be checked from this package. Fabrication, target programming, balance construction, calibration, electrical bring-up and actual measurement performance remain unverified. This is not a certified weighing instrument or a claim of worldwide patent novelty.

## Purpose, audience and practical value

Balance Order helps a teaching workshop investigate why three apparently simple pairwise weight comparisons do not always produce a defensible ranking. A small equal-arm balance has a finite neutral region. When neither of two beam-position switches closes, the instrument knows only that the beam remained inside that region. It does not know that the objects have exactly equal mass. Treating every neutral observation as equality can falsely declare an inconsistency when A is neutral against B, B is neutral against C, but A is visibly heavier than C.

The device retains nine counters: neutral, left-low and right-low observations for each of the three pairs AB, AC and BC. It checks whether all observed outcomes could coexist under one common symmetric deadband. Contrary outcomes are retained rather than replaced by the latest reading or hidden in an average. A real contradiction prompts the user to inspect the fixture, sample labels, balance bias, drift and repeatability. A compatible result says only that the retained observations have not disproved that particular model.

The primary user is a physics instructor building a slow, tabletop comparison experiment with three reusable coupons. A secondary user is a maker investigating the repeatability of a homemade spring-centered balance. Neither user needs a claimed milligram readout. They need to distinguish logical inconsistency from finite resolution, and to retain repeated outcomes while moving physical objects. This is deliberately not intended for medicine, commerce, dosing, hazardous materials or production acceptance.

The minimum interaction is short: remove the pair key, place the labelled coupons in their specified pans, insert the matching key, wait for DONE, and remove the key. A trial normally completes after half a second of stable observed position. The user can see immediately whether the new result changes compatibility. Setup, calibration and sensor checks take longer and must not be skipped merely because an individual trial is quick.

## Why hardware, originality and alternatives

A spreadsheet can solve the inequalities, and a phone can record manual answers. Hardware is useful here because the two physical beam thresholds and the stable-observation interval are acquired directly rather than remembered after moving the samples. The pair key supplies the user's identity declaration without putting a keyboard next to a swinging balance. It does not independently recognize coupons, detect loads or prevent deliberate mislabelling. A paper log remains the recommended baseline for the first usability comparison.

The repository was synchronized before development and its device directories and README headings were inventoried. The closest conceptual comparison is Texture Cycle, which retains subjective preference arrows and detects ordinary directed cycles. Balance Order instead observes a mechanical beam, records three outcomes for three fixed pairs, and solves strict versus non-strict difference constraints. Neutral relations are explicitly nontransitive. Shim Bracket collects fit/refusal against an ordered thickness scale; this instrument has neither a known ordered scale nor a pass/fail aperture. ContactAtlas measures switch transition topology, not object comparisons. These distinctions are part of the evidence contract, not merely different names.

Five concepts were considered informally: another environmental logger, a force-sensing balance, a camera-read balance, a manual preference panel and this threshold-based comparator. The environmental logger and preference panel offered little separation from existing projects. The force instrument required a calibrated analog front end beyond this bounded design. The camera added illumination and occlusion dependencies. The selected concept concentrates complexity in a small, exhaustively testable constraint system and an inspectable low-energy fixture.

Pan balances and pairwise comparisons are established prior art. A bounded web search found educational work on pan balances and mathematical equivalence, the Physics Classroom Balance Beam interactive, and classical balance-and-coins reasoning. None of those retrieved descriptions established this exact combination of keyed physical comparisons, retained per-pair outcomes and deadband-aware contradiction checking. That is not proof that no equivalent device exists elsewhere. The useful contribution proposed here is the explicit distinction between neutral bands and equality, backed by a reproducible implementation.

References and component provenance:

- https://www.tandfonline.com/doi/abs/10.1080/10986065.2020.1700587 — educational pan-balance features and equivalence.
- https://www.physicsclassroom.com/interactive/rotation-and-balance/balance-beam/notes — established balance teaching context.
- https://www.cs.utexas.edu/~EWD/transcriptions/EWD10xx/EWD1083.html — classical three-outcome balance reasoning.
- https://www.littelfuse.com/products/sensors/fluid-level-position-sensors/reed-switches/mdsr-7 — MDSR-7 dry reed contact family.
- ST UM1724, STM32 Nucleo-64 boards MB1136, and RM0368, STM32F401 reference manual. The platform pin mapping is inherited from the repository's previously checked F401 carrier; a fresh manufacturer PDF download timed out during this run.

## Hardware specifications and architecture

| Function | Revision A |
| --- | --- |
| MCU | STM32F401RET6 on external NUCLEO-F401RE MB1136 |
| Clock | Reset HSI, nominal 16 MHz |
| Memory | 512 KiB flash, 96 KiB RAM |
| Acquisition | Nominal 1 ms; five equal samples qualify each bundle |
| Inputs | Two pair-key contacts and two beam reed contacts |
| Contact supply | External 3.3 V pullups, voltage-free closures only |
| Transport | USART2 TX PA2 AF7, 115200 baud 8N1, ST-LINK VCP |
| Power | USB to Nucleo; Nucleo 3V3 and ground feed carrier |
| Carrier | 80 × 70 mm, two-layer 1.6 mm FR-4, through-hole components |
| Proposed fixture | 320 × 200 × 180 mm enclosed tabletop balance |
| Application | Linux Python 3/Tk, standard library, no cloud |
| Storage | Volatile MCU counters; explicit private desktop CSV export |

```text
Pair key: PARK / AB / AC / BC --- two dry contacts ----+
                                                     |
Left-low reed --+                                    |
Right-low reed +-- beam-mounted magnet ---------------+-- passive carrier
                                                            |
                                                    PA0 PA1 PA4 PB0
                                                            |
USB --> external NUCLEO-F401RE --> qualification --> settlement window
                       |                                    |
                       |                           nine retained counters
                       |                                    |
                       +---- USART2 TX <----- deadband feasibility solver
                                  |
                           ST-LINK virtual COM
                                  |
                       Tk ledger / replay / private CSV
```

There is no FPGA, wireless module, battery charger, actuator or high-voltage circuit. The development board supplies the processor, regulator, reset circuit, decoupling, debugger and USB bridge. Those parts are not silently omitted from a purported bare-MCU schematic: they belong to the explicitly purchased controller assembly. The custom circuit is only the four-channel contact carrier and its harness.

## Electrical design, BOM and pin cross-reference

| Reference and net | Meaning | External destination |
| --- | --- | --- |
| J1.1 VCC / J1.2 GND | 3V3 supply and common ground | CN6.4 / CN6.6 |
| J2.1 SENSE0 | Pair bit zero | CN8.1 A0 / PA0 |
| J2.2 SENSE1 | Pair bit one | CN8.2 A1 / PA1 |
| J2.3 SENSE2 | LEFT_LOW | CN8.3 A2 / PA4 |
| J2.4 SENSE3 | RIGHT_LOW | CN8.4 A3 / PB0 |
| J3.1 PAIR0 / J3.2 GND | First key circuit | Captive key socket |
| J4.1 PAIR1 / J4.2 GND | Second key circuit | Captive key socket |
| J5.1 LEFT_LOW / J5.2 GND | Left beam threshold | First reed |
| J6.1 RIGHT_LOW / J6.2 GND | Right beam threshold | Second reed |

Each channel consists of a 10 kohm pullup to VCC and a 1 kohm series resistor between SENSE and its external contact. J2 carries only signals; it is not a power header. Logical one means a closed contact even though the electrical voltage is low. Preserve the normal Nucleo ST-LINK serial bridge configuration, checking the actual board revision and SB13/SB14 versus SB62/SB63 before assembly. Firmware does not enable UART reception or accept host commands.

| Quantity | Part or specification | Purpose |
| --- | --- | --- |
| 1 | NUCLEO-F401RE | Controller, power and programming |
| 4 | 10 kohm, 1%, 0.25 W axial, 7.62 mm pitch | R1/R3/R5/R7 pullups |
| 4 | 1 kohm, 1%, 0.25 W axial, 7.62 mm pitch | R2/R4/R6/R8 series resistors |
| 5 | 1×2, 2.54 mm vertical through-hole header | Power and contact headers |
| 1 | 1×4, 2.54 mm vertical through-hole header | Signal harness |
| 2 | Littelfuse MDSR-7-10-15, encapsulated after wiring | Normally-open beam reeds |
| 1 set | Four-position keyed housing using two isolated contact pairs | Removable AB/AC/BC coded keys |
| 1 set | Beam, equal pans, central flexure, adjustable brackets, magnet | Mechanical experiment |
| 1 set | Insulated harness, USB cable, covered enclosure | Wiring and guarding |

The reed manufacturer's description explicitly includes microcontroller logic-level loads. Its sensitivity range is not a calibrated angle or mass threshold. Confirm actual switches, magnet orientation and release behavior on the assembled instrument. Do not bend reed leads adjacent to the glass seal, and protect the glass in a nonmagnetic encapsulation. No supplier availability or price quotation is implied by this BOM.

A planning budget is USD 50–100 for controller, carrier, contacts, harness and basic mechanical materials, excluding a computer, labor and calibration equipment. Allocate roughly USD 20–35 to the controller, 10–20 to the small-batch carrier and passives, 5–15 to switches and harness, and 15–30 to mechanics. These are estimates for planning, not measured procurement costs.

## Power, grounding and safety

At nominal voltage a closed channel draws 3.3 V / 11 kohm = 0.30 mA. Its SENSE voltage is approximately 0.30 V. Four closures draw 1.20 mA, corresponding to 3.96 mW at the 3V3 rail. One pair bit plus one beam reed gives about 0.60 mA of carrier load. With all contacts open the ideal resistor-network DC load is zero, excluding leakage. These calculated carrier values do not represent the USB consumption of the controller or debugger.

| Mode | Carrier estimate | Complete system allowance |
| --- | --- | --- |
| Park, no beam closure | Near-zero network DC current | USB/controller remains running |
| One key bit and one reed | 0.60 mA at 3V3 | Not measured |
| All four inputs closed | 1.20 mA at 3V3 | Fault-test condition, not normal balance position |
| Serial active / idle | No change to passive network | Reserve a 5 V, 500 mA USB supply budget |

No battery-life claim is appropriate because this revision is USB powered and the complete system has not been measured. The 500 mA figure is a supply planning allowance, not expected consumption. Do not power the carrier from 5 V, drive its contact inputs from powered electronics, or connect it to machinery. Series resistance is not galvanic isolation, reverse-polarity protection, a surge suppressor or certified ESD protection. The USB host ground is common with every contact.

Keep the harness short, preferably below 0.5 m, inside a covered dry enclosure. Disconnect USB before changing wiring. Use only lightweight blunt coupons, nominally no more than 20 g per pan for the first mechanically reviewed prototype. The structure must be independently checked for that load. Captive magnets, guarded stops and a transparent cover prevent loose parts, pinching and glass contact damage. The device must never be used as a balance controller, interlock or indication that a load is safe.

## Mechanical fixture and calibration contract

Use an equal-arm beam with pan suspension points 100 mm either side of the central rotation axis. A nominal 240 mm beam leaves space outside those points for guarded stops. Suspend identical 40 mm pans on matched links. The central assembly should be spring-centered by a torsion flexure, with both pan suspension axes level with the pivot at the nominal horizontal position. A freely swinging ordinary beam with its center of gravity substantially below the pivot can have a total-load-dependent restoring torque and may not support the common-deadband model.

Fit a small captive magnet to an extension near the pivot, with two reed brackets adjustable independently in angle and radial distance. Set the initial sensing positions around opposite small beam angles, nominally two degrees from center, and place mechanical stops beyond them. These angles are assembly starting points, not guaranteed switch thresholds. Magnet attraction, reed hysteresis, flexure friction and pan offset can all invalidate the model. The balance deliberately has no firmware conversion from angle to grams.

Mount the carrier in insulated edge rails in a 90 × 80 × 25 mm electronics compartment; it has no mounting holes. Provide a separate covered Nucleo compartment and USB strain relief. Put the key socket on the front of the base. Wire a four-contact keyed socket as two isolated shorting pairs: AB closes only PAIR0, AC only PAIR1, BC both, and an empty socket closes neither. Give the three removable keys permanent large labels. The physical left/right pan mapping is always the alphabetical pair order.

The empty socket is PARK. Load and unload only in PARK. A key transition that briefly presents another pair may cause rejection, especially for BC insertion where contacts do not mate simultaneously. Rejection is conservative and requires removing the key before retrying; it must not be bypassed by increasing debounce blindly. Operators should verify the selected pair displayed by the companion. A coding key declares identity but does not prove which coupons were placed on the pans.

Before meaningful experiments, zero with empty matched pans, exercise both reed contacts, and check that both are open in the intended neutral region. Use known small added masses to measure each threshold in both directions, at several total loads. Repeat after swapping pans and reversing the coupons. The solver assumes one common positive threshold D for all comparisons, symmetric between directions and stable within the session. Reed hysteresis is a particular reason this assumption may fail. If threshold intervals do not overlap sufficiently for the intended demonstration, retain raw counts only and do not interpret the compatibility verdict as physical evidence.

## Evidence model and firmware decisions

For each pair with left coupon L and right coupon R, define unknown normalized mass values xL and xR. Neutral contributes -1 <= xL - xR <= 1. Left-low contributes xL - xR > 1. Right-low contributes xR - xL > 1. The normalization uses an unknown but common positive D; no numerical mass calibration is needed to ask whether an assignment exists. Neutral is not exact equality and is not transitively closed as equality.

The retained nine-bit evidence mask marks which outcomes have ever been accepted. Nine counters retain repetitions. A strict difference-constraint solver uses a three-node distance matrix and a strictness bit. A negative cycle, or a zero-bound cycle containing a strict inequality, makes the accumulated observations incompatible. The original outcomes are never replaced with inferred edges. Both neutral-chain compatibility and strict-cycle failure are important tests. Exhaustively checking all 512 possible outcome masks is practical at this scale.

WAIT requires a qualified PARK baseline before any trial. READY begins a trial when a nonzero pair code qualifies and agrees with raw input. ACTIVE freezes the pair identity. The beam pair qualifies as one bundle independently of the key bundle. A stable classification needs 500 consecutive observed millisecond intervals whose raw and qualified beam patterns agree at both endpoints. The first newly qualified endpoint earns no previous time. Even a short raw glitch resets settlement; a qualified neutral state cannot hide it.

DONE retains the result until PARK qualifies again. Stable both-closed beam inputs reject rather than indicate equality. Raw key changes immediately reject an active trial. A 10,000 ms trial deadline takes precedence over settlement at that exact endpoint. A lost acquisition tick aborts the active trial and permanently halts accounting, because omitted samples cannot establish continuity. Transport errors are counted separately. All admissions reserve room for one disposition so starts equals accepted plus rejected plus aborted plus the active indicator, including exhaustion paths.

The implementation is freestanding C11 using actual STM32F401 register addresses, Cortex-M4 startup vectors and a linker script. It has no RTOS, dynamic allocation or vendor SDK dependency. HSI setup uses bounded polling before UART initialization. SysTick provides nominal millisecond ticks; the main loop samples once per changed tick and never invents missed samples. The watchdog is fed after acquisition. UART transmission uses a bounded nonblocking buffer and a 100 ms timeout. HALT continues to report retained evidence if transport remains usable.

## Protocol, application and privacy

Every frame uses the marker BO1 and these unsigned decimal fields in order:

```text
now,state,levels,valid,starts,accepted,rejected,aborted,gaps,faults,tx_errors,
age,dwell,selected,outcome,evidence,conflict,c0,c1,c2,c3,c4,c5,c6,c7,c8
```

The full line is `BO1,<fields>*<four uppercase CRC digits>\n`. CRC-16/CCITT-FALSE starts at 65535 with polynomial 0x1021 and covers the marker and comma-separated body. State values are WAIT=0, READY=1, ACTIVE=2, DONE=3 and HALT=4. Selected pair values are PARK=0, AB=1, AC=2 and BC=3. Outcome values are neutral=0, left-low=1, right-low=2 and invalid-both=3. Counters are grouped by pair and then outcome: c0..c2 for AB, c3..c5 for AC, c6..c8 for BC. Fault bits are gap=1, overflow=2, configuration=4 and watchdog=8.

Run `python3 app/workbench.py` from this folder on Linux with Tk installed. Connection/replay provides an explicit serial path, connect/disconnect actions and strict replay loading. Comparison ledger shows all three rows, counters, current settlement and a decision-oriented compatibility warning. History retains changes in completed/rejected/aborted trials. Setup/export records the label and writes a private CSV with author, provenance, connection state, freshness, parser errors and retained snapshots. Export refuses overwrite and protects free-text labels against spreadsheet formula interpretation.

The app has no cloud, telemetry, Bluetooth permission, network listener or target-control protocol. Anyone with access to the local serial device can fabricate data; CRC is error detection, not authentication. Keep the computer and USB path trusted. Replay files must be regular files no larger than 2 MiB and must end on a complete valid frame. The interface labels replay as historical and potentially synthetic. Session history is bounded to 2,000 snapshots; counters retain cumulative totals even when old snapshots leave that window.

Live status requires advancing device timestamps and recent receipt, not just repeated identical lines. Contradictory duplicate timestamps, counter rollback, impossible conservation, changed pair identity and implausible settlement rates freeze the session. A fixed two-second host/device progress allowance detects backlog and large drift but may require conservative reconnect on a long uncalibrated session. Sparse checks are necessary consistency bounds, not reconstruction of every input. There is no persistent boot identifier, so some resets remain indistinguishable from forward traffic. Export before reset; reconnect does not clear target SRAM.

## Build, verification and remaining work

Run `python3 balance-order/tools/verify.py` from the repository root. It builds the actual Cortex-M4 ELF and binary, runs address/undefined-behavior-sanitized host tests, generates a synthetic trace using the production C encoder, exercises Python parsing and the real Tk event loop through a Linux pseudo-terminal, and checks schematic/PCB connectivity and design rules. Required tools are arm-none-eabi GCC/binutils, host GCC, Python with Tk, Xvfb, KiCad 9 CLI and pcbnew. Build products and local KiCad preferences are ignored.

For a target build alone use `make -C balance-order/firmware all`. The optional flash target invokes OpenOCD with ST-LINK after an intentional hardware connection and wiring review. It is not run during software verification. The synthetic demo under firmware/build contains neutral-chain compatibility followed by an opposing strict observation and an invalid beam trial; it is not a physical balance measurement.

The CAD checker exports a fresh schematic XML netlist and compares all references, values, footprint identifiers and pin-net assignments against the PCB and connectivity manifest. ERC and DRC require zero violations and zero unconnected items. Passive ERC cannot verify the external Nucleo supply, harness orientation or magnetic geometry. Inspect the rendered schematic, actual connector orientation and manufacturing tolerances before ordering.

Physical acceptance remains a separate stage: continuity and polarity tests, current-limited electrical bring-up, measured GPIO levels, target flashing, UART and tick timing, watchdog behavior, magnetic threshold calibration, repeated load reversals and user comparison with handwritten notes. Reject the shared-deadband interpretation if these experiments do not support it. The deliverable is a reproducible design-and-software package, not a claim that an assembled instrument has already worked.
