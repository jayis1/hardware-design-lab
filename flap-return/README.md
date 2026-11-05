# Flap Return — shared-release comparison of miniature self-closing flaps

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, software, documentation and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The supplied board is a routed passive carrier for a separately purchased NUCLEO-F401RE, not a standalone processor board. Software compilation, synthetic contact tests and CAD rule checks are reproducible. Fabrication, programming, mechanical commissioning, sensor accuracy and physical usability have not been tested. No safety or regulatory certification is implied.

## Purpose and practical value

Flap Return is a small three-lane bench instrument for comparing the return behavior of miniature spring-hinged covers, model shutters and flexure flaps. A common sliding retaining bar holds the three flaps open. Removing the bar releases them, while a fourth contact marks the release. Three independent closed-position witnesses then record when each flap first arrives, how often it reopens, when it most recently closed, and whether it remains closed for a defined clearing interval. The result distinguishes a quick first touch followed by repeated reopening from a slower but stable return.

A model maker choosing a light return spring and bumper for a miniature cabinet often adjusts one mechanism at a time and judges it by eye. A stopwatch can measure a rough delay but hides contact chatter and requires the operator to watch several leaves. A single magnetic switch can report the eventual closed state but cannot explain whether the leaf arrived once or repeatedly left its rest position. This fixture is intended to make those differences inspectable while keeping the experiment local, inexpensive and mechanically understandable.

The initial audience is makerspaces, educational mechanism laboratories and prototype enclosure designers. Useful experiments include comparing three bumper materials on otherwise identical flaps, comparing spring preload settings, and determining whether a seemingly faster return has more qualified reopening. These are fixture comparisons, not measurements of hinge torque, damping ratio, impact energy or service life. A faster return is not automatically better. The operator must decide which observed behavior is appropriate for the actual product and must not use this notebook to approve a fire door, machine guard, child restraint or other safety-critical closure.

The firmware does not drive the mechanism. There is no motor, solenoid or release command. The common release is manual, the stored energy must remain low, and the entire mechanism should be accessible only through a transparent guard during a trial. The controller earns its place by maintaining acquisition timing independently of desktop scheduling and by retaining a failed experiment instead of quietly restarting it. A video camera remains a useful independent reference and may provide more information when angular motion matters.

## Distinction from existing designs and prior art

Repository directories and README concepts were inspected before implementation. Snap History records a complete sequence of reversible four-fastener occupancy vectors during a user manipulation attempt. Flap Return instead reserves one channel for a shared release origin and retains independent first/last closure, reopening count and raw-aware settling credit for the remaining three channels. It does not reconstruct the complete vector word. Rebound Rungs observes a single descending and returning slider through ordered spatial bands. This device observes three separate mechanisms with arbitrary arrival order and allows repeated returns. Creep Pair measures analog displacement of two compressible coupons under independent loading; this device has no displacement or load sensor. HingeScribe observes installed door angle and reaction force for maintenance, whereas this is a bounded, manual miniature-fixture comparison.

The common F401 startup, clock, GPIO, UART, transport and passive PCB infrastructure are reused from jayis1's Snap History at repository commit 4fb9705341d3933ecdf7c066e9cac0b0ddeeb99c. The trial engine, FR1 schema, lane comparisons and experiment contract are specific to this device. Sharing an electrically identical input carrier is intentional and does not imply the experiments are identical.

Self-closing hinges, magnetic stabilization and return testing already exist. A bounded external search found US7016492B2, describing magnetic hinge behavior, and the Royal Society paper titled “Characterization of magnetically stabilized hinges for origami-inspired mechanisms.” Those examples rule out claiming invention of a self-returning flap or magnetic hinge. The proposed contribution is the shared-release, three-lane, offline notebook with explicit reopening and raw-aware settlement evidence. No exhaustive patent or commercial-product search has been performed, and worldwide uniqueness is not established.

References and assembly documents:

- https://www.freepatentsonline.com/7016492.html
- https://royalsocietypublishing.org/rsta/article/382/2283/20240008/108334/Characterization-of-magnetically-stabilized-hinges
- ST UM1724, Nucleo-64 MB1136 connector mapping and virtual-COM solder bridges: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 register reference, for the peripheral definitions retained in firmware/registers.h.

The ST manual could not be downloaded during this run: the HTTPS attempt failed and an HTTP/1.1 retry timed out. Connector and register assignments therefore reuse the inspected repository platform, not a falsely claimed fresh manufacturer-document review. Check the actual purchased board revision against the manufacturer's manual before assembly.

## Hardware specification and pin contract

| Function | Revision A |
| --- | --- |
| Processor | STM32F401RET6, Cortex-M4, on external NUCLEO-F401RE |
| Memory | 512 KiB flash and 96 KiB SRAM on purchased MCU |
| Clock | Nominal internal 16 MHz HSI, restored explicitly at boot |
| Acquisition | Nominal 1 ms SysTick observations in foreground |
| Contacts | Three flap-closed reeds plus one retaining-bar-engaged reed |
| Qualification | Five consecutive equal samples, independently per channel |
| Communication | USART2 transmit-only, PA2 AF7, 115200 baud, 8N1 |
| USB | Purchased ST-LINK virtual COM bridge, not a carrier USB circuit |
| Power | USB-powered Nucleo, regulated 3.3 V to carrier |
| Carrier | 80 by 70 mm, two layers, nominal 1.6 mm FR-4 |
| Application | Linux Python 3 with Tk; standard library only |

J1 pin 1 is VCC from Nucleo CN6 pin 4, 3V3; J1 pin 2 is GND from CN6 pin 6. J2 pins one through four are SENSE0, SENSE1, SENSE2 and SENSE3, connected respectively to CN8 pins one through four: PA0/A0, PA1/A1, PA4/A2 and PB0/A3. These analog-labelled header positions are configured as digital inputs. J3 is FLAP_A, J4 FLAP_B, J5 FLAP_C and J6 RELEASE_HOLD. Every field connector has signal on pin 1 and ground on pin 2. A closed reed is a logical one after inversion of the low-active electrical level.

The board is cable connected alongside the Nucleo. It is not an Arduino shield and must not be pushed onto an unmatched header. The purchased board supplies the processor, decoupling, regulator, reset, boot configuration, debugger and USB circuitry. The passive schematic explicitly represents only the carrier. It is not a substitute for the development-board schematic.

Each SENSE input has a 10 kilohm pullup to VCC and a 1 kilohm series resistor to its field contact. A contact closes the far end of that series resistor to ground. At nominal 3.3 V this produces approximately 0.30 V at the input and 0.30 mA through a closed contact; four closed channels draw approximately 1.20 mA. These are ideal circuit calculations, not measurements. Reserve a 5 V, 500 mA USB source allowance for initial development-board operation and measure actual consumption. There is no battery, charger, radio or isolated power supply.

## Architecture and block diagram

    Flap A reed ------- J3 -- 1k -- SENSE0 -- PA0/A0
    Flap B reed ------- J4 -- 1k -- SENSE1 -- PA1/A1
    Flap C reed ------- J5 -- 1k -- SENSE2 -- PA4/A2
    Hold-bar reed ----- J6 -- 1k -- SENSE3 -- PB0/A3
                                      |
                                 10k per input
                                      |
                                     3V3

    Contact returns -> carrier ground -> Nucleo -> USB host ground
    USB -> NUCLEO-F401RE -> independent qualification -> release engine
                         -> three lane histories -> FR1 CRC snapshots
                         -> USART2 -> ST-LINK VCP -> desktop comparison/export

There are fourteen carrier components: eight axial resistors and six headers. The KiCad schematic contains locally authored resistor and connector symbols with numbered passive pins, real wires and named nets. The PCB contains through-hole library footprints, routed front/back copper, a ground via and a closed outline. The native project, symbol library and connectivity manifest are included. The generator is a reproducibility aid; do not rerun it over manually edited CAD without reviewing the overwrite consequences.

## Mechanical fixture specification

Use a rigid insulating base approximately 240 mm wide and 160 mm deep. Arrange three parallel flap stations on 70 mm centers. Each initial flap can be a 40 by 50 mm lightweight polymer leaf with a captive hinge pin along its rear short edge. A practical initial moving-mass allowance is under 10 grams per flap, including the retained sensor magnet. Use soft return springs or compliant hinges appropriate to that low mass; do not scale the fixture to powered doors. Put mechanical stops at the closed rest position and an initial open angle near 45 degrees. Those dimensions are starting construction parameters, not validated tolerances or safety ratings.

A transverse sliding bar holds tabs on the three open flaps. Provide three independently adjustable tab screws so all flaps disengage over the same short bar travel. Mount the release witness on the fixed frame and a captive magnet on the bar. Its contact must open as the tabs disengage, not after a long delay. The shared electrical release is still only a proxy for physical release: tab skew, magnetic hysteresis and bar speed can introduce lane-dependent errors. Measure release skew with video or independent optical gates before interpreting millisecond differences.

Mount each closed-position reed on an adjustable insulated bracket beside the front of its flap, with the small retained magnet on the leaf. Select normally-open signal reeds such as the Littelfuse MDSR-7-10-15 variant, subject to checking its current data sheet and availability. Set the detection zone narrowly around the intended closed rest, then verify that small physical reopening actually opens the contact. A large magnetic field can make a bouncing flap look continuously closed. Reed attraction can itself alter a very light mechanism; compare behavior with and without the sensing magnet before treating the fixture as nonintrusive.

All magnets must be captive rather than held by loose adhesive alone. Support the carrier by insulated edge rails because this board has no mounting holes. Provide strain relief, short twisted signal/ground pairs, and initial lead lengths below half a meter. Place the Nucleo and carrier in a separate insulating enclosure allowance of approximately 110 by 100 by 45 mm. These are layout allowances, not a validated enclosure CAD model. The mechanical design must be commissioned for clearances, friction, release repeatability and pinch protection before a physical trial.

## Firmware and observation contract

The target is freestanding C11 with a real Cortex-M4 image, linker script, startup vectors and data/BSS initialization. The implementation uses no heap, vendor SDK, filesystem or floating point. The foreground loop owns the engine and transmitter; SysTick only advances the millisecond count. Clock-restoration waits use bounded iteration budgets before SysTick exists. GPIO configuration is read back, watchdog readiness waits are bounded, and the UART drains a fixed buffer without blocking acquisition. A transmit timeout increments a saturating diagnostic counter instead of silently pretending delivery succeeded.

WAIT requires the three flap contacts open and the hold-bar contact closed. Raw and qualified vectors must both equal eight across 250 consecutive acquired intervals. Reboot with arbitrary contacts does not start a result. READY means the bar remains engaged and setup has been observed. Any raw flap closure before accepted release halts with PRECLOSED, including a very fast flap arriving during release qualification. This conservative rule avoids inventing a nonnegative latency for a return that may precede the chosen origin.

When the hold contact qualifies open with all flap contacts still open, ACTIVE starts at age zero. The origin is the qualified electrical release, not the physical tab separation. Each subsequent qualified flap closure records its first timestamp if absent, updates its latest closure timestamp, and increments its closure count. A qualified opening increments that lane's reopening count. Any ordering or tie among the three lanes is permitted. Closure count minus reopening count equals current qualified occupancy while active and completed.

Each lane also accumulates settlement credit. An interval earns credit only when raw and qualified closure agree at both endpoints and the previous qualified state was closed. The first closing endpoint earns no earlier time. A single raw opening glitch resets credit even if it is too short to qualify as a reopening. This deliberately separates observed reopening count from the stronger continuous-settlement requirement. A lane's credit saturates at 250 ms. The whole trial completes only when all three credits simultaneously reach 250 ms; a previously settled lane can lose credit while waiting for another.

At an age of 10000 ms, timeout takes precedence over any final closing or settling event on that sample. Any raw hold-bar reassertion during ACTIVE halts immediately, because a return after being mechanically restrained again is no longer the same experiment. A sampling gap of anything other than one millisecond halts correlation; duplicate calls at the same tick are ignored. Counter capacity is bounded at one thousand closures per lane, with simultaneous channel updates committed atomically. DONE and HALT freeze accepted measurements, phase and fault until reset. Raw levels, device uptime and transmit-error diagnostics remain visible after termination.

One trial per reset is intentional. Export evidence, reengage the retaining bar with all flaps open, reset the Nucleo and reconnect the app for another comparison. Power loss erases the volatile notebook. The device does not retain a persistent run ID, and no physical setup is authorized merely because a reboot occurred. An open cable can imitate a perpetually open flap, a short can imitate closure, and this simple interface does not uniquely diagnose either fault.

## Protocol and application interface

Firmware emits a full FR1 snapshot approximately every 100 ms. Records are ASCII CSV terminated by an asterisk, four uppercase hexadecimal CRC digits and LF. CRC-16/CCITT-FALSE covers the bytes before the asterisk, with polynomial 0x1021, initial value 0xFFFF, no reflection and no final XOR. The field order is:

    FR1,now,phase,fault,raw,mask,valid,age,baseline,tx_errors,
        first0,last0,closures0,reopens0,quiet0,
        first1,last1,closures1,reopens1,quiet1,
        first2,last2,closures2,reopens2,quiet2*HHHH

The displayed wrapping above is explanatory; each real record is one line. Times are nominal milliseconds. Phases are WAIT=0, READY=1, ACTIVE=2, DONE=3, HALT=4. Faults are none=0, acquisition gap=1, capacity=2, configuration=3, watchdog=4, timeout=5, preclosed=6 and reheld=7. The closure count distinguishes absent timestamps from recorded values. Raw is current electrical contact state, mask is accepted qualified state, and valid marks channels that have qualified. Terminal mask is frozen evidence, so it can differ from later raw input.

Launch `python3 flap-return/app/workbench.py` from the repository root on Linux with Tk installed. The connection bar opens a selected serial device and provides disconnect, strict recording replay and CSV export. The Return comparison screen displays the three lane histories, terminal or active state, fault, raw mask and transport losses. The Fixture setup and limits screen contains connector mapping and the experiment procedure. There are no prepopulated demonstration measurements in the application.

The parser rejects malformed decimals, wrong field counts, CRC failures and contradictory state relationships. Session checks reject backward time, rewritten first closures, disappearing counts, terminal mutations and impossible credit growth. These are necessary consistency checks, not complete reconstruction of hidden waveforms between snapshots. Transport freshness requires advancing timestamps; repeated identical frames cannot renew it. A long host pause or excessive accumulated host/device time divergence blocks continued live interpretation until reconnect. Freshness never overrides HALT or substitutes for acquisition validity.

Replay accepts only regular files up to two MiB, rejects truncated tails and never labels replay as live. The actual C encoder supplies synthetic fixtures for tests, clearly separated from physical measurements. Export creates a new private CSV and refuses overwrite. It includes author, protocol, provenance, freshness, connection status, parser errors, warning and the entire latest snapshot even if no valid trial completed. Disconnect preserves evidence. A failed replacement connection preserves the prior capture; successful reconnect replaces it, so export first. No data leaves the host automatically.

## Bill of materials, safety and build acceptance

Populate four 10 kilohm and four 1 kilohm, one-percent axial DIN0207 resistors with 7.62 mm lead spacing; five two-pin and one four-pin straight 2.54 mm headers; and one supplied passive board. Add one NUCLEO-F401RE, four external signal reeds, four captive magnets, short insulated leads, a USB cable, the three miniature flap mechanisms, adjustable tabs, sliding bar and transparent guard. A quantity-one planning allowance of USD 60–140 excluding labor and measuring equipment is an estimate, not a supplier quotation. Mechanical repeatability can cost more effort than the electronics.

Only disconnected voltage-free contacts may be connected. There is no galvanic isolation, transient suppression, industrial input protection or mains interface. USB ground is shared with the computer. The series resistors are not safety barriers. Do not connect powered proximity sensors or machine wiring. Keep this low-energy tabletop prototype away from unattended automation and safety acceptance. Secure the fixture, guard the flaps, and keep fingers clear of the release tabs.

Run `python3 flap-return/tools/verify.py` to rebuild target firmware, run sanitized C tests, generate a real-encoder fixture, exercise the Tk app under Xvfb, check Python syntax, run KiCad ERC/DRC and compare every schematic pin/net against PCB pads and the component manifest. The same script checks README length, substantive production C lines and author attribution. The native hardware generator requires system Python with KiCad pcbnew bindings. Normal builds do not regenerate CAD. Flashing is available through `make -C flap-return/firmware flash` with OpenOCD and an attached target; it has not been performed during this run.

Before physical use, inspect assembly and polarity with power removed, verify each contact identity using isolated jumpers, measure rail and low/high input voltages, and test the release sequence without springs. Then compare independent video and electrical traces against reported first closure and settlement. Test a deliberately rebounding flap, unplugged sensor, held bar and missed acquisition. Record invalid trials, not only successful ones. ERC on passive symbols cannot validate an external board or wiring, and geometric DRC does not prove fabrication quality. VALIDATION.md distinguishes executed checks from outstanding physical qualification.
