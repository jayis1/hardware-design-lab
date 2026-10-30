# Sand Stroke — face-separated manual sanding traversal notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application and locally authored hardware design sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype, not a calibrated abrasion tester. The supplied PCB is a routed passive contact carrier for a separately purchased NUCLEO-F401RE. Firmware target linking, host tests and CAD checks are reproducible. No board has been fabricated or flashed during this design run; no sanding experiment, accuracy calibration, EMC assessment or production qualification is claimed.

## Purpose and overview

Sand Stroke is a removable witness for a small, hand-operated sanding sled. It records whether the sled made a complete traversal between two adjustable endpoints while its engagement witness remained asserted. An independent selector declares which face of a test coupon is being worked. The device separates accepted traversal counts and transit times for face A and face B, while retaining short-return, lift, face-change, collision, timeout and sampling-gap diagnostics. Its purpose is to make repetitive manual coupon preparation easier to compare without asking the operator to remember every stroke.

The intended users are woodworking instructors, model makers and small material-development workshops preparing paired surfaces by hand. A lesson can compare nominally equal stroke recipes on two faces, or show how an interrupted motion differs from a complete pass. A workshop can attach a log to photographs of two coupons before deciding whether a more expensive abrasion tester is justified. This is a workflow notebook, not a substitute for measuring mass loss, thickness, roughness or coating adhesion.

A tally counter records button presses. A stopwatch records elapsed session time. Neither distinguishes a short return from an endpoint-to-endpoint pass, separates declared coupon faces automatically, or marks a lifted sled during transit. Sand Stroke derives those distinctions from a constrained mechanical fixture. The crucial constraint is that the sled can only translate along its guide; freehand sanding over an unrestricted surface is outside the design. Using the tool on a different geometry requires a new measurement-contract review.

The measurement is deliberately modest. ENGAGED means a low-force witness switch is closed, not that the abrasive applies a calibrated force. FACE_B is an operator declaration, not optical recognition of the coupon. Endpoints establish a travel envelope, not a continuous position trace. A full stroke can include pauses or reversals within the envelope that never revisit the starting endpoint. Consequently, neither stroke count nor accepted transit duration is abrasive work, surface coverage, uniformity or removed material. The interface uses those literal names rather than displaying a misleading percentage of sanding completion.

## Originality and closest alternatives

Repository directories and README descriptions were inspected before selecting the concept. Ratchet Audit observes encoder rollback in a manual mechanism; Takeup Atlas studies reversal-to-following motion; Creep Pair compares compression and recovery. Teardown Stack tracks removable parts cups, while Rest Quartet tracks coupon residence. None of these provides face-separated, engagement-qualified endpoint traversal records for a sanding sled. This package reuses the established passive four-contact carrier and low-level platform, but the measurement engine, protocol and application are specific to this workflow.

A bounded external search found a useful discussion in [Penturners, Sanding Problems](https://penturners.co.uk/threads/sanding-problems.7630/), where manually counting passes is proposed to improve consistency, and [Craftsmanspace's sanding-block plan](https://www.craftsmanspace.com/sites/default/files/free-plans-pdf-files/Sanding%20block%20plan.pdf), which discusses uniform stroke length and pressure. These are adjacent practices, not evidence that counting alone establishes a finish. Commercial sanding blocks and bench hooks remain simpler alternatives when an operator can keep a reliable tally. The incremental contribution here is the combination of fixture-derived traversal qualification, declared-face separation and explicit uncertainty reporting.

No worldwide novelty, patentability or freedom-to-operate claim is made. Generic industrial counters, programmable controllers and laboratory abrasion rigs can implement related functions. This proposal is new to this repository and targets an inexpensive teaching fixture rather than replacing a standardized tribometer. Before producing multiple units, run a small blinded comparison with a paper tally sheet: if the fixture changes the operator's technique more than the log helps, do not use it for comparative preparation.

## Hardware specification and electrical boundary

| Item | Revision A contract |
|---|---|
| MCU module | ST NUCLEO-F401RE, MB1136, STM32F401RET6 |
| Core and memory | Cortex-M4, 512 KiB flash, 96 KiB SRAM |
| Clock | Internal HSI, nominal 16 MHz |
| Sensors | Four external voltage-free switch closures |
| Carrier | 80 by 70 mm, two-layer FR-4, nominal 1.6 mm |
| Components | Six headers, four 10k pull-ups, four 1k series resistors |
| Link | USART2 TX through ST-LINK virtual COM, 115200 baud, 8N1 |
| Power | USB to Nucleo; Nucleo regulated 3.3 V to carrier |
| Sample cadence | Nominal one millisecond, ten-sample independent qualification |
| Stroke limit | Opposite arrival strictly before 10000 ms after departure |
| Storage | Volatile MCU totals; local host history and CSV |
| Outputs | Telemetry only; no actuator, interlock or machine control |

The Nucleo contains the regulator, decoupling, reset circuit, processor, programming interface and USB bridge. The custom carrier does not duplicate those functions. It connects through a short insulated harness and is not a mechanically mating Arduino shield. The schematic explicitly describes this boundary; passive-component ERC cannot validate the separate development board or its USB source. The design basis is ST's [UM1724](https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf) connector and VCP documentation and [RM0368](https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) peripheral definitions.

### Exact connector and firmware contract

| Header | Pins and net | External destination |
|---|---|---|
| J1 | 1 VCC, 2 GND | CN6 pin 4 regulated 3V3, CN6 pin 6 GND |
| J2 | 1 SENSE0 | CN8 pin 1 A0 / PA0 |
| J2 | 2 SENSE1 | CN8 pin 2 A1 / PA1 |
| J2 | 3 SENSE2 | CN8 pin 3 A2 / PA4 |
| J2 | 4 SENSE3 | CN8 pin 4 A3 / PB0 |
| J3 | 1 LEFT, 2 GND | Left endpoint switch |
| J4 | 1 RIGHT, 2 GND | Right endpoint switch |
| J5 | 1 ENGAGED, 2 GND | Engagement witness switch |
| J6 | 1 FACE_B, 2 GND | Maintained face declaration selector |

Each SENSE net has a 10k pull-up to VCC and a 1k series resistor toward its named switch net. Closing a switch connects its signal to ground. Firmware inverts the GPIO input: mask bits zero through three mean LEFT, RIGHT, ENGAGED and FACE_B respectively. A cleared FACE_B bit declares A, including a broken selector wire; the circuit cannot distinguish those cases. PA2 alternate function 7 is USART2 TX to the development board's ST-LINK VCP. Verify the actual board revision and VCP solder bridges before use.

The carrier has no battery, charging circuit, radio, analog amplifier or sensor bus. Its four inputs accept only disconnected, voltage-free switches. Do not connect PLC outputs, powered tool switches, mains circuits or externally excited sensors. USB ground is shared, and there is no galvanic isolation, surge suppressor or qualified ESD protection. The 1k resistors are not a substitute for those protections. Keep paired signal/ground leads short, provisionally below half a metre, and test interference sensitivity before trusting a workshop log.

## Architecture

```text
Manual guided sled             Face declaration lever
  | left and right stops          |
  | engagement witness            |
  +---------------+---------------+
                  v
         J3..J6 voltage-free contacts
                  |
            1k series resistors
                  |
            SENSE0..3 ---- 10k pulls ---- 3V3
                  |
          J2 -> PA0 / PA1 / PA4 / PB0
                  |
USB -> NUCLEO-F401RE -> independent contact qualification
                  |                |
                  |       traversal state machine
                  |       face totals and diagnostics
                  v
           PA2 USART2 -> ST-LINK VCP -> Linux Tk companion
                                         | three screens
                                         + local CSV / strict replay
```

There is no command receiver. The app cannot change thresholds, erase MCU totals, arm a machine or rewrite firmware. Configuration changes require reviewing and rebuilding source, followed by reflashing through ST-LINK. The firmware has no network updater or bootloader supplied by this package. A reset loses volatile accounting, so export before intentional power removal. App reconnect is not an MCU reset and does not start a new physical coupon automatically.

## Mechanical fixture design and setup

Use a nonconductive base with parallel guides and a captive sanding sled. A suggested starting geometry is a 300 by 120 mm base, an 80 by 50 mm sled, and 100 mm between qualified endpoint positions. These are prototype dimensions, not tolerance-certified mechanical drawings. Adjustable stops must absorb user force separately from the switches. Install low-force lever contacts such as Omron D2F-01L variants only after checking the purchased part's electrical and mechanical ratings. Low wetting current, dust and overtravel need physical evaluation.

Mount the endpoint switches so the sled can operate exactly one at either end and neither in the intervening travel region. Both asserted simultaneously is a wiring or geometry fault. Set stop spacing with a ruler, then check it with a repeatable reference block if travel comparability matters. The recorded transit excludes residence at endpoints; it begins at qualified departure and ends at qualified arrival. Moving the stops changes the experiment even though firmware does not know their separation. Record that separation and the abrasive grade in a separate laboratory note alongside the exported CSV.

The engagement witness should detect whether the abrasive carrier is down in its working position. One practical arrangement is a vertically floating abrasive shoe with a very light spring return and a separately stopped witness lever. The guide must permit lifting without trapping fingers, and the switch must not carry sanding load. This fixture is not delivered as a fabricated or tested mechanism. Spring force and friction could alter manual preparation substantially; compare the assembled fixture with an ordinary block before interpreting experiments. Do not call the witness a force sensor.

The face declaration selector should be conspicuously labelled A/B and mounted away from the sliding hand path. Lift the sled, change the coupon orientation, set the selector, and reengage at an endpoint. The device does not verify this procedure physically. An instructor should mark the specimen and check the declaration before each batch. A keyed coupon holder is a possible future improvement, but it is not silently assumed in the current electronics or software.

Cover the carrier in an insulating enclosure with strain relief and removable access for inspection. It has no mounting holes; support its edges rather than drilling through copper. Keep the enclosure outside the dust collection path. Use appropriate workshop dust extraction and personal protection independently of this device. Restrict initial trials to benign, untreated wood or inert test coupons; do not sand unknown coatings, lead paint, composites with hazardous dust, sharp blades or energized equipment as part of commissioning.

## Firmware behavior and decisions

The portable C engine exposes WAIT, READY, ACTIVE and HALT. Each contact qualifies independently after ten consecutive samples of one raw level. An unrelated chattering contact cannot delay an endpoint's qualification. Qualified values remain in effect until replaced; therefore sub-qualification lifts and rapid intermediate reversals are not guaranteed detectable. All times describe qualified observations rather than exact mechanical contact instants. Physical switch hysteresis and the nominal HSI clock add uncertainty beyond the one millisecond representation.

WAIT means no accepted starting endpoint. An engaged, single-endpoint baseline with all channels known enters READY and captures the declared face. Leaving that endpoint into the neither-endpoint region while engagement and face remain unchanged starts ACTIVE. Opposite-end arrival before the deadline completes a traversal, adds its elapsed milliseconds to the captured face and makes the destination the next READY endpoint. Both directions count equally. A return to the originating endpoint is a short return and aborts the traversal rather than counting a complete stroke.

While ACTIVE, simultaneous both-endpoint indication has first precedence and abandons correlation. Otherwise a qualified lift takes precedence over a face change, which takes precedence over the maximum-duration deadline, which takes precedence over endpoint arrival. Arrival exactly at the deadline is rejected. An opposite arrival simultaneous with lift or face change cannot be accepted. Diagnostic categories are exclusive for a rejected active observation, so a lift and flip in the same sample are recorded as a lift, not two separate aborted strokes.

A direct READY transition from one endpoint to the other without an observed neither-endpoint interval increments collisions and reseeds at the new endpoint; it never invents an intervening stroke. Both-endpoint episodes are counted on entry, not every held sample. A valid endpoint after a short return, timeout or face change can establish a new baseline for a later stroke. This recovery does not undo the rejected count. Starting the device in mid-travel never creates a departure event.

Any sampling delta other than one millisecond abandons an active stroke, increments gaps and clears qualification. Missing time is unknown, not repeated samples. A new qualified endpoint is required before counting resumes. Ordinary unsigned timer wrap is supported for consecutive samples. Arithmetic updates are transactional: if any bounded counter or duration would overflow, the update is discarded, any previously accepted active stroke is abandoned exactly once, and HALT latches. The invariant is started = full_a + full_b + aborted + active. Duration sums belong only to completed traversals; rejected motion is intentionally excluded.

The freestanding target includes startup vectors, RAM initialization, a linker map, direct-register clock/GPIO/UART drivers and an independent watchdog. The acquisition loop has no allocation or blocking transmission. The UART uses a fixed pending buffer, sends opportunistically and abandons stalled transmission after 100 ms. Clock restoration uses bounded iteration waits before SysTick exists. Configuration and watchdog-start failures latch fault before telemetry. Actual flashing, watchdog expiry and timing accuracy remain hardware bring-up tasks.

## Protocol and application

Every 100 ms the target attempts an SS1 status record. It contains the marker SS1 followed by twenty-one comma-prefixed uppercase eight-digit hexadecimal values, then an asterisk, four CRC digits and LF. CRC-16/CCITT-FALSE has initial value 0xFFFF and polynomial 0x1021, covers all bytes before the asterisk and has no final XOR. The full record is 198 bytes. Every record is independently versioned for late attachment; there is no required one-shot boot message.

Fields in exact order are now, state, mask, known, anchor, face, elapsed, started, full_a, full_b, aborted, ms_a, ms_b, short_returns, lifts, flips, collisions, timeouts, gaps, tx and fault. State codes are WAIT=0, READY=1, ACTIVE=2 and HALT=3. Anchor is left=1 or right=2 when correlated, otherwise zero. Face is A=0 or B=1. Time is milliseconds modulo 2^32. Fault bits mean configuration=1, watchdog-start=2, exhaustion=4 and invalid raw input=8. WAIT and HALT clear anchor, face and elapsed while retaining historical totals.

The companion uses Python 3 standard-library Tk and Linux termios; no npm packages or mobile SDK are necessary. Run `python3 sand-stroke/app/workbench.py`. Connection / fixture explains wiring and opens the selected tty explicitly. Stroke balance presents live state, accepted counts and transit totals with diagnostics. History / export retains the latest 1000 accepted snapshots, reports evictions and creates a new owner-only CSV without overwriting an existing file. Metadata includes jayis1, provenance, connection state, freshness, errors and warnings even when no data rows exist.

Replay is offline and never labelled live. It accepts a regular recording no larger than two MiB, rejects FIFOs and devices before reading, and rejects empty, corrupt or truncated recordings. Failed replay preserves previous evidence. Live framing tolerates a partial late-attachment suffix, counts discarded fragments and resynchronizes on the complete SS1 marker. A partial buffered record at disconnect is counted as an error rather than silently lost. The app does not itself write raw SS1 recordings; capture those with a separate serial terminal while this app is disconnected. CSV exports are not SS1 replay files.

The parser checks field ranges, conservation, per-face duration bounds, cumulative monotonicity and consistency between adjacent records. Duration growth requires matching new completions, and combined face-duration growth cannot exceed available observed time plus an already active traversal. A duplicated timestamp does not renew freshness; contradictory duplicates or clock/counter rollback block the session. Two distinct advancing observations are required before live progress is shown. Freshness expires after 750 ms without progress; a receive pause beyond 1.5 seconds or accumulated host/device clock disagreement beyond 750 ms requires reconnect.

That accumulated allowance is deliberately fixed. It prevents an indefinitely growing backlog from being called live, but an uncalibrated HSI clock with sustained frequency error can also cause a conservative disconnect during a longer capture. This is a known tradeoff, tested rather than hidden. Use short experiments and export before reconnecting; a future calibrated clock design needs a reviewed drift policy. CRC is not authentication, and the protocol has no persistent boot identity. Some resets remain indistinguishable from plausible forward traffic. Treat logs as local experimental notes, not tamper-evident certification.

## BOM, power and validation plan

A provisional purchasing allowance is USD 25 for the Nucleo, USD 5 for the carrier allocation, USD 2 for resistors, USD 4 for headers and leads, USD 12 for switches and USD 20 for fixture materials. These are planning estimates, not verified supplier quotes, and exclude labor, shipping and dust-control equipment. Standard through-hole 10k and 1k resistors use DIN0207 horizontal footprints with 7.62 mm pitch; headers use 2.54 mm pitch. Do not substitute footprint-incompatible parts without regenerating and reviewing the CAD.

At nominal 3.3 V, each closed 10k-plus-1k path draws approximately 0.30 mA, with approximately 0.30 V at SENSE. All four paths together draw approximately 1.20 mA and dissipate approximately 3.96 mW. Reserve a provisional 100 mA at 5 V for the complete Nucleo-based system, then measure actual consumption. There is no battery-life estimate because there is no battery. Broken wires and stuck switches are not reliably diagnosed: open engagement means disengaged, shorted engagement means engaged, and a broken face selector means A. Inspect these manually before every comparative study.

Build and test with `make -C sand-stroke/firmware all test`. Run the companion suite with `xvfb-run -a python3 -m unittest discover -s sand-stroke/tests -p 'test_*.py'`. Generate CAD with `python3 sand-stroke/tools/generate_hardware.py` using KiCad 9 Python bindings and installed footprints, then run `python3 sand-stroke/tools/check_hardware.py`. The checker runs ERC and DRC and compares schematic netlist pins, PCB pads, component values and full footprint identities against the manifest. Shared platform provenance is recorded in tools/import_platform.py; normal builds do not depend on neighboring device files.

Before physical use, meter continuity with power removed, bring up one contact at a time, verify the mask mapping and deliberately exercise all invalid paths. Compare video-labelled strokes against logs across slow, rapid, interrupted and mid-travel attachment trials. Measure false acceptance and rejection rates instead of assuming software tests establish physical accuracy. Separately measure force, thickness or roughness if those outcomes matter. See VERIFICATION.md for executed checks and remaining limitations; fabrication and real workshop value are explicitly not established by passing builds.
