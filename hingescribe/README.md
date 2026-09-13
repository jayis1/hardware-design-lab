# HingeScribe

Author and creator: jayis1

Copyright (c) 2026 jayis1

License: MIT

Maturity: build-verified design proposal; not yet fabricated or physically calibrated

## Purpose and overview

HingeScribe is a clamp-on, privacy-preserving door-hinge health recorder that measures how a door moves, how much force is needed to open it, whether it is beginning to sag, and whether a mechanical closer is drifting out of adjustment. A small generator coupled to the hinge pin recovers energy from normal door movement. That recovered energy extends the life of an internal rechargeable cell and makes the device practical in corridors, workshops, rental properties, schools, accessibility audits, and facilities where running cable to every door is unreasonable.

The practical problem is mundane but expensive. Doors gradually become difficult to open because hinges wear, fasteners loosen, frames shift, seals swell, closers lose adjustment, or debris reaches the threshold. The change is usually slow enough that occupants adapt. Maintenance starts only after a door scrapes, slams, fails to latch, or becomes inaccessible. Periodic inspections are subjective and provide no history. Building access-control sensors report open or closed state, but do not measure the mechanical effort or motion shape that reveals deterioration.

HingeScribe turns each ordinary opening into a short mechanical test. An AS5600 magnetic angle sensor observes hinge rotation without contact. A miniature load cell in the removable mounting bridge measures reaction force. A TMP117 temperature sensor separates seasonal seal stiffness from mechanical wear. The nRF52840 computes cycle-level features locally, stores only those features, and exposes them over Bluetooth Low Energy. It does not record audio, images, identities, room occupancy, or exact wall-clock movement logs by default. The companion progressive web application guides calibration, displays actionable maintenance findings, exports owner-controlled records, and remains useful in simulator mode without hardware.

The design is an original repository concept, not a claim of worldwide novelty or patent clearance. A preliminary comparison found three nearby categories: reed-switch door sensors, instrumented hinges used in laboratory robotics, and commercial door-force gauges used during manual inspections. Reed switches lack force and motion-shape data. Laboratory hinges generally require replacement of structural hardware and wired acquisition. Handheld gauges provide a single inspector-controlled measurement and no passive trend. HingeScribe combines removable installation, simultaneous force and angle traces, on-device trend scoring, and motion-energy recovery. A professional prior-art and freedom-to-operate search would still be required before commercialization.

## User value and complete interaction

A facilities technician clips the two-part housing around the upper hinge barrel, attaches the small frame-side reaction tab, closes the door, and presses the calibration button. The device averages magnetic angle and unloaded force samples for ten seconds. Normal traffic then supplies measurements and some energy. After several dozen cycles, the baseline becomes stable. During a monthly round, the technician opens the local app, chooses the named device, and sees a health score plus plain findings such as “opening resistance rose 18 N above baseline” or “closed-angle drift suggests approximately 2.8 mm of sag.” The history screen shows whether the issue is persistent, temperature-related, or sudden. Data can be exported as JSON for a work order, then deleted locally.

The door continues to work without the app, an account, a network, or a cloud service. A green diagnostic pulse after a cycle means the sensors were valid; an amber maintenance pulse means the local score is below the configured threshold; three red pulses indicate a sensor or calibration fault. The device never actuates or locks the door. Its recommendations are advisory and cannot replace fire-door, egress, accessibility, or structural inspection.

Target users are building maintenance teams, accessibility auditors, property managers, workshop owners, door installers, and researchers studying door mechanics. The design is deliberately not marketed for covert occupancy monitoring, access control, security alarms, medical diagnosis, or safety certification. Installation requires the property owner’s permission.

## Hardware specification

The controller is a Nordic nRF52840-QIAA: a 64 MHz Arm Cortex-M4F with 1 MiB flash, 256 KiB RAM, hardware cryptography, 12-bit SAADC, USB, and Bluetooth 5 radio. The selected package provides enough GPIO for sensors, SWD, status indication, and future expansion. Production firmware is intended for Nordic nRF5 SDK 17.1.0 or an equivalent maintained nRF Connect SDK target. The repository’s default Makefile builds a deterministic host simulation with strict warnings so protocol and analytics code can be verified without an ARM toolchain.

Angle is measured by an AMS AS5600 Hall-effect rotary position sensor at I2C address `0x36`. A diametrically magnetized 6 mm magnet sits in a non-load-bearing cap over the hinge pin. The sensor reports 12-bit absolute angle, while its magnet-status register detects missing, weak, or excessive field. The geometry supports approximately 0–135 degrees of useful door travel and leaves the original hinge pin mechanically untouched.

Reaction force uses a 20 kg single-point load cell with an HX711-compatible 24-bit bridge converter. The reference schematic names the interface as `LOAD_DOUT` and `LOAD_SCK`; production selection should use an automotive-temperature HX711 alternative if the installation environment exceeds the commercial range. The load cell is not placed in the structural load path. It measures deflection of the mounting bridge against a frame-side pad. Calibration converts ADC counts to newtons using a known pull gauge. A replaceable elastomer pad prevents finish damage and defines repeatable compliance.

A Texas Instruments TMP117 at `0x48` measures local temperature with 0.0078125 degrees Celsius resolution. The firmware logs temperature only with cycle features, allowing force drift to be correlated with seal and lubricant temperature. Two resistor dividers feed the nRF SAADC: one monitors the lithium cell and one monitors the rectified harvester rail.

Energy comes from a low-profile, 12-pole micro generator driven by a compliant friction wheel on the hinge barrel. A Schottky bridge rectifies either direction of rotation. The BQ25570 energy-harvesting PMIC cold-starts from the generator reservoir and charges a protected 300 mAh LiPo cell. A 5.1 V TVS, 100 mA resettable fuse, cell protection, and PMIC over-voltage programming constrain faults. The generator is intentionally slip-coupled so it cannot add more than approximately 0.15 N·m resisting torque. If it jams, the friction wheel slips rather than obstructing door motion.

Estimated active-cycle consumption is 18 mA for 1.5 seconds, or 27 mAs. Sleep current target is below 18 microamps, including regulators and sensors in standby. At 200 cycles per day, active energy is roughly 1.5 mAh and sleep energy about 0.43 mAh per day. The generator target is 10–80 mJ per opening depending on angle and speed; at 30 mJ and 200 openings it returns about 1.7 mWh daily. These are calculations, not bench results. With no harvested energy and a conservative 70 percent usable cell capacity, the 300 mAh cell target is about 100 days. Physical measurements must validate generator drag, PMIC efficiency, and battery life.

Connectivity is BLE only. Advertisements reveal a rotating device identifier, firmware major version, battery band, and whether maintenance is due. Detailed readings require an explicit GATT connection. Pairing should use LE Secure Connections with a printed per-device setup code for production. No Wi-Fi, microphone, camera, GPS, cloud endpoint, or user account is present.

The reference PCB is a four-layer, 52 mm by 24 mm board with ground and power inner planes. The enclosure target is 64 × 34 × 18 mm excluding the reaction bridge, printed in flame-retardant polycarbonate/ABS. The radio end projects away from the hinge metal, with a 10 mm antenna keep-out. Two M2.5 screws join the clamp halves. The housing must not cross the hinge knuckle gap or reduce required door clearances.

## Architecture and block diagram

```text
 Hinge magnet ──> AS5600 angle ──I2C──┐
 Frame bridge ──> load cell ──HX711───┤
 Local air ─────> TMP117 ──────I2C────┤
 LiPo/harvester ─> SAADC ─────────────┤
                                         v
                                  nRF52840 MCU
                               ┌─────────┼──────────┐
                               v         v          v
                         flash journal  BLE GATT  status LED
                               |         |
                               |         v
                               └──> local-first companion PWA

 Hinge rotation -> slip wheel -> generator -> Schottky bridge
      -> BQ25570 harvester -> protected 300 mAh LiPo -> 3.3 V rail
```

The architecture separates acquisition, interpretation, persistence, and presentation. Sensor drivers return values plus validity flags rather than silently substituting believable numbers. The analyzer is a deterministic state machine with `CLOSED`, `OPENING`, `OPEN`, `CLOSING`, `OBSTRUCTED`, and `UNKNOWN` states. The history ring stores 128 completed cycle summaries. The protocol module owns framing and checksums. The app treats malformed, stale, or disconnected data as explicit states.

## Pin, bus, and power contract

| Function | nRF52840 pin | Net | Notes |
|---|---:|---|---|
| I2C SDA | P0.26 | I2C_SDA | 4.7 kΩ pull-up to 3V3 |
| I2C SCL | P0.27 | I2C_SCL | 4.7 kΩ pull-up to 3V3 |
| AS5600 alert | P0.06 | ANGLE_ALERT | optional wake input |
| Load data | P0.08 | LOAD_DOUT | converter output |
| Load clock | P0.09 | LOAD_SCK | defaults low |
| Harvester ADC | P0.02/AIN0 | HARVEST_SENSE | divided to under VDD |
| Battery ADC | P0.03/AIN1 | BATTERY_SENSE | switched divider in production |
| Status LED | P0.13 | STATUS_LED | active low, current limited |
| Button | P0.11 | USER_BUTTON | internal pull-up |
| SWD | P0.20/P0.18 | SWDIO/SWCLK | Tag-Connect footprint |

All digital devices share 3V3 logic. I2C pull-ups must be fitted once, not duplicated by a sensor cable. Each IC has a local 100 nF ceramic capacitor and the radio supply has additional 4.7 µF bulk capacitance. The load-cell converter uses a filtered analog rail. Input dividers are selected so maximum charger and cell voltage cannot exceed SAADC limits. Exposed contacts receive low-capacitance ESD protection.

## Firmware design

The firmware directory contains `main.c`, `sensors.c`, `analytics.c`, `protocol.c`, `platform.c`, `board.h`, `registers.h`, and a Makefile. Dynamic allocation is prohibited. Buffers have compile-time bounds. Sensor reads return typed errors and use bounded timeouts. Median filtering rejects isolated load-cell spikes; a first-order low-pass filter reduces bridge noise. Angular velocity uses wrap-safe angle differences and elapsed monotonic time.

A door event begins when a previously closed door starts opening. The analyzer captures peak opening and closing forces, maximum speed, closing duration, final angle, estimated sag, harvested energy, sensor validity, and battery state. It compares force against a slowly learned closed-state baseline and variance rather than one fixed number. The score deducts points for excess force, closed-angle drift, slow closing, slamming, obstruction, or invalid sensing. Thresholds are intentionally conservative defaults and must be calibrated for hinge geometry.

Protocol frames use little-endian fields: magic `0x4853`, version, message type, payload length, payload, CRC-16/CCITT-FALSE, and newline terminator. Status type `0x01` carries timestamp, angle in centidegrees, velocity in deci-degrees per second, force in centinewtons, temperature in centidegrees, battery millivolts, harvested millijoules, event sequence, health, flags, and validity bytes. History type `0x02` contains one cycle summary. Commands `0x80` through `0x83` request tare, erase history, set load scale, or request history. Unsupported versions and bad CRCs fail closed.

Calibration is versionable persistent state protected by CRC-32. The current host platform emulates storage in memory. A hardware port must place two alternating calibration pages in internal flash, verify before activation, and preserve the previous valid page across power loss. Factory reset deletes measurements and calibration but does not alter firmware.

Run firmware verification with:

```sh
cd firmware
make check
```

This builds all five C translation units with C11, optimization, `-Wall -Wextra -Wpedantic -Werror`, links the simulator, and runs six self-test suites covering standard CRC vectors, history wraparound, acquisition, event state transitions, framing, and calibration persistence. `make run` executes a repeatable synthetic door trace. `make target` performs translation-unit compilation with `arm-none-eabi-gcc` when `NRF5_SDK` points to Nordic SDK 17.1.0. A complete production target must additionally supply Nordic startup code, linker script, SoftDevice or Zephyr configuration, BLE security settings, and flash driver; those vendor artifacts are not copied into this repository and remain under their original licenses.

The watchdog target is eight seconds. Hardware loops must feed it only after sensor and scheduler progress. Brownout uses safe GPIO defaults: generator load disconnected, LED off, and load-cell clock low. After repeated sensor faults the device reports invalid state and avoids maintenance conclusions from the bad data. There is no remote firmware-update implementation in this proposal; SWD is the recovery path.

## Companion application

The `app` folder is an install-free progressive-style web application written in standards-based HTML, CSS, and JavaScript. It provides four functional screens: live health, local event history, guided calibration, and settings. The Connect button requests Web Bluetooth only after a user gesture, discovers devices whose name begins with HingeScribe, subscribes to the versioned status characteristic, validates every packet, and handles disconnect or malformed data visibly. The simulator creates deterministic complete cycles so evaluation does not require hardware.

History is retained in browser local storage, capped at 128 records, exportable as JSON, and deletable by the owner. Threshold preferences remain local. The interface does not use color alone: scores, text findings, labels, and focus outlines communicate state. Responsive layout supports phones and desktop browsers. Web Bluetooth support is strongest on Chromium-based Android and desktop browsers; iOS requires a future native wrapper or WebBLE-capable host. This limitation is explicit rather than disguised as tested iOS support.

Run the application checks with `cd app && npm test && npm run check`. There are no runtime package dependencies or lockfile supply-chain surface. `npm start` serves only `127.0.0.1:4173`. The unit tests verify CRC behavior, deterministic simulation, actionable flag mapping, and rejection of undersized frames.

The intended GATT service UUID is `7c2e0001-8f4a-4a4b-a632-f3d8663c7f91`; status/command characteristic UUID is `7c2e0002-8f4a-4a4b-a632-f3d8663c7f91`. Production pairing must add authenticated writes, per-device identity, replay-resistant command sequencing, and ownership reset. The current app receives notifications but does not send a physical tare packet; it records the guided local action and clearly says hardware command support follows connection work.

## KiCad design and manufacturing notes

The KiCad 8 project contains a schematic, PCB, and project settings file under `kicad/`. The schematic uses real named components, references, footprints, and nets for the nRF52840, AS5600, TMP117, HX711, BQ25570, cell connector, harvester connector, SWD connector, antenna, protection, filtering, decoupling, and user interface. The PCB contains matching references, an explicit closed board outline, net assignments, mounting holes, antenna keep-out text, and routed representative signals.

The component-level architecture is suitable for review, but the board is not manufacturing-ready until opened in KiCad 8 and subjected to library resolution, annotation consistency, ERC, netlist update, DRC, impedance review, antenna review, and mechanical fit checks. `kicad-cli` is not bundled. This repository therefore validates balanced S-expressions and cross-file references with `validate.py`, but does not claim ERC or DRC success in the absence of KiCad. Fabrication also requires Gerber, drill, pick-and-place, stencil, and assembly outputs generated from a reviewed revision.

Estimated single-unit BOM at prototype quantities is approximately USD 46: nRF52840 USD 8, sensors and converters USD 10, harvester PMIC and power parts USD 8, load cell USD 7, generator USD 5, battery USD 4, PCB USD 2, and mechanical parts USD 2. Procurement prices and availability change. Exact manufacturer part numbers are listed in schematic values; alternate parts require pin, range, startup, and footprint review.

## Safety, privacy, and failure modes

HingeScribe must never impede a door. The friction drive is torque-limited and outside the structural hinge stack. The reaction tab uses a breakaway polymer feature. Installation on fire-rated, emergency-egress, smoke-control, security, or powered doors requires approval from the responsible authority and may be prohibited. The output is maintenance evidence, not a compliance verdict. A low score must lead to inspection, not automatic adjustment.

A missing magnet, detached load bridge, saturated converter, impossible temperature, corrupted calibration, low cell, CRC failure, and radio disconnect each produce an explicit invalid or degraded state. The firmware never transforms an invalid sensor sample into a confident finding. Data minimization reduces privacy risk: stored records describe mechanical cycles without person identity or audio. Facility owners should still disclose monitoring where policy or law requires it, choose aggregate retention, and avoid correlating event timing with individuals.

The rechargeable cell requires protected cells, correct polarity, thermal validation, and an enclosure that avoids crush or puncture. The harvesting PMIC must be tested under maximum door speed and generator open-circuit voltage. RF performance near steel is uncertain until measured. ESD, EMC, environmental ingress, flammability, accessibility, and regulatory compliance remain unverified.

## Verification and traceability

| Requirement | Implementation | Verification |
|---|---|---|
| Observe hinge motion | AS5600, `sensors.c` | host sensor suite; physical test pending |
| Measure opening effort | load bridge, converter driver | simulated median/filter test; calibration pending |
| Detect trends locally | `analytics.c` state machine | strict build and event suite |
| Reject malformed data | CRC framing in both firmware and app | standard vectors and app tests |
| Work without cloud | local BLE and local storage | source inspection and simulator |
| Recover some motion energy | generator plus BQ25570 design | calculation only; bench test pending |
| Preserve ownership/privacy | no account, explicit export/delete | app flow and source inspection |
| Avoid blocking the door | slip coupling and breakaway tab | mechanical prototype test pending |

Verified in this repository means the portable firmware compiles and its self-tests pass, the application parses and its tests pass, required file and attribution checks pass, and KiCad text is structurally parseable. It does not mean the electronics have been fabricated, the RF link certified, sensor accuracy calibrated, battery life measured, enclosure fitted, or the assembly approved for a regulated door.

## Use cases and target audience

Facilities teams can prioritize lubrication, fastener tightening, closer adjustment, seal replacement, and frame inspection based on trend rather than complaints. Accessibility auditors can compare opening effort before and after maintenance while retaining the need for certified measurement procedures. Rental managers can identify a dragging exterior door before it damages flooring. Workshop owners can monitor large cabinet and machinery-guard doors, provided the installation is not safety interlocked. Door manufacturers can use the portable mount during development to compare hinge, gasket, and closer configurations.

A secondary research use is studying how temperature and traffic affect mechanical opening effort without deploying cameras. Researchers must still obtain consent and choose appropriate aggregation. The design is not suitable for inferring room use, counting people, policing access, or monitoring domestic behavior without informed permission.

## Assembly, calibration, and test plan

First assemble and inspect the board without a cell. Check shorts on VBAT, 3V3, and harvester rails. Power from a current-limited 3.7 V source, verify 3V3, program over SWD, and exercise the host-equivalent self-test on target. Confirm angle status with the magnet at minimum and maximum gap. Calibrate the load channel with at least five known forces in both directions and fit scale plus hysteresis. Rotate the generator across the expected speed envelope while measuring peak open-circuit voltage, harvested energy, and drag torque.

Next mount the enclosure on a sacrificial door. Verify all clearances through the full swing and demonstrate that removing or jamming the device cannot stop opening. Record at least one hundred cycles using a reference angle gauge and force gauge. Compare calculated angle, peak force, closing time, and sag estimate. Test weak and excessive magnetic field, detached bridge, obstruction, slow closer, slam, brownout, full history, corrupted packet, interrupted calibration, phone replacement, permission denial, and factory reset.

Acceptance targets for the first prototype are angle error under 1.5 degrees after calibration, force error under 3 N from 10–80 N, complete-cycle detection above 98 percent in the tested door, no false maintenance alarm in 500 stable cycles, BLE update latency below two seconds while connected, and added opening torque below 0.15 N·m. These targets are engineering hypotheses until measured.

## Detailed engineering decisions

The main design decision is to measure reaction force at a removable bridge instead of replacing the hinge pin with a load-bearing instrument. A structural instrumented pin could produce cleaner torque data, but installing it would require lifting the door, selecting an exact pin diameter, accepting structural responsibility, and potentially invalidating a rated assembly. The reaction bridge is less direct, so its geometry must be calibrated, but it can be removed without changing the door. The compliant pad and mechanical stop limit overload. The firmware reports force at the bridge in newtons rather than claiming hinge torque unless the installer enters a measured lever arm.

Magnetic angle sensing was chosen over an optical encoder because the sensor can remain fully enclosed and tolerates dust. It also avoids the accumulating error of integrating a gyroscope. The AS5600 is inexpensive and widely documented, but ferrous hinge material distorts the field. The cap therefore holds the magnet above the pin and the PCB positions the sensor on a fixed axis. The status register is checked on every acquisition. If the enclosure shifts or the magnet gap changes, the firmware marks the trace invalid instead of using the last value. A future board may use a three-axis magnetic sensor to estimate alignment and reject lateral displacement more precisely.

The load converter is clocked only during an active trace. This reduces average power and prevents the converter from dominating sleep consumption. Five consecutive samples feed a median filter, then a bounded low-pass filter. Median filtering is preferable to a long moving average for this application because cable motion and contact bounce can create isolated impulses; preserving the peak shape matters when finding sticking. A production calibration fixture should apply at least five rising and five falling loads. The difference quantifies hysteresis in the reaction pad and load cell. If hysteresis exceeds the error budget, the mount must change rather than hiding it in software.

Cycle summaries were selected instead of raw long-term traces for privacy and storage efficiency. A 256-sample raw trace can be retained temporarily for diagnostics, but normal history stores about forty bytes per cycle. At 128 records, the journal occupies only a few kilobytes. The oldest record is replaced deterministically. This gives a technician recent trend evidence without maintaining a detailed, indefinite occupancy timeline. Exports include an author and device marker but no person or site identity unless the owner adds it after export.

The device uses a rechargeable cell even though it harvests motion energy. A purely batteryless design would fail on rarely used doors, could lose the closing portion of a cycle after a weak opening, and would make BLE availability unpredictable. The harvester is therefore an extender, not a guarantee of perpetual operation. Firmware adapts advertising and notification activity before reducing sensing quality. Below the low-battery threshold it stores summaries and advertises infrequently. It never increases mechanical drag in an attempt to recharge faster.

BLE was selected instead of Wi-Fi because installation should not require network credentials or cloud infrastructure. The short-range connection also encourages a technician to be physically present when inspecting a fault. Rotating advertisement identifiers reduce passive tracking. Production firmware must store pairing state in protected flash, rate-limit failed authentication, and require physical button presence for ownership reset. Read-only summary access may be acceptable without bonding in some facilities, but calibration, deletion, threshold changes, and firmware updates must require authenticated control.

## Mechanical integration

The mount has three separable elements: a sensor cap over the upper end of the hinge pin, a frame-side electronics shell, and a narrow reaction bridge between shell and door-side leaf. None of these parts carries door weight. The sensor cap uses a soft collet rather than a set screw against the hinge finish. Its magnet is captive even if adhesive fails. The electronics shell references the frame leaf with two padded jaws and includes a tether point for overhead installations. The bridge terminates in a replaceable elastomer foot whose compression range is bounded by hard stops.

Installation geometry is important. The sensor axis should be within one millimeter of the hinge axis, the reaction pad should contact a flat region, and the shell must remain clear of the moving leaf through at least five degrees beyond normal maximum opening. The app calibration flow should ask the technician to move the door slowly through its range before enabling history. This commissioning trace detects reversed angle, insufficient magnet field, shell movement, and an unexpected mechanical stop.

The proposed 52 by 24 millimeter PCB leaves most of the enclosure volume for the cell, load bridge interface, generator support, and antenna separation. The generator should be mounted on elastomer bushings so cogging vibration does not couple strongly into the load measurement. Its wheel is a consumable polyurethane ring. The bill of materials should offer two durometers because painted steel and brass hinge barrels have different friction. The wheel must remain replaceable without opening the battery compartment.

Environmental sealing is intentionally modest for revision one. Gasketed shell halves and a membrane-covered button can target incidental cleaning spray, but the hinge gap is a harsh location with dust, lubricant, impact, and condensation. The electronics should use conformal coating except at the antenna and connectors. Outdoor gates, refrigerated rooms, corrosive plants, and washdown areas are outside the verified operating envelope. Temperature recording helps diagnose behavior but does not make the enclosure suitable for every temperature that the sensor IC can measure.

## Data interpretation and maintenance workflow

A rising opening-force trend does not identify one failed part. Hinge friction, gasket compression, latch alignment, air-pressure difference, closer preload, floor contact, and wind can all contribute. HingeScribe presents observations and suggested inspection order. A technician first checks for visible floor or frame contact, then fastener movement, then hinge contamination and lubrication, then latch and closer adjustment. Comparing several cycles at similar temperatures helps avoid unnecessary work caused by a cold seal.

Closed-angle drift is also treated cautiously. The magnetic zero can move if the mount slips, so a sag finding requires both repeated angular drift and stable magnet status. The estimate in millimeters uses an installation-specific coefficient based on door width and sensor geometry. Without that coefficient the app should display angular drift only. A sudden shift after impact is more urgent than the same total shift accumulated slowly. The event structure therefore preserves sequence and duration so future analytics can distinguish step changes from gradual wear.

Closer performance is measured from the beginning of closing motion until a stable closed state. Many regulated doors have specific closing-time tests with prescribed start angles and forces. HingeScribe does not reproduce those procedures. It flags deviation from the door’s accepted baseline and directs the technician to perform the applicable certified test. Similarly, the slam flag reflects excessive measured angular velocity, not impact energy or injury risk.

Alert suppression prevents maintenance fatigue. A single marginal cycle does not permanently lower status. The intended production policy requires the same noncritical condition in three of the latest ten valid cycles. Sensor faults and severe obstruction can be reported immediately. Acknowledgement records that a human saw the finding but does not erase evidence. After maintenance, the technician starts a new baseline epoch while retaining the earlier aggregate for comparison.

## Communications contract and recovery

Every multibyte field in protocol version one is little-endian. Receivers must verify magic, exact protocol version, declared length, CRC, and terminator before reading payload fields. Unknown message types are ignored. Unknown flag bits are retained in exports but not interpreted. Integer scaling avoids compiler-dependent floating-point serialization. The maximum frame is 128 bytes, comfortably below the negotiated BLE data length when fragmented by the transport layer.

The app must distinguish live status, cached status, simulator status, and malformed input. When a device disconnects, the last values may remain visible only with a stale label and timestamp. Automatic reconnect is limited to a device chosen during the current session; the application does not scan continuously in the background. A permission denial leaves all offline functions usable. If the firmware version is newer than the app protocol, the app shows an upgrade message rather than guessing at fields.

Command writes need a monotonically increasing command sequence in the production security revision. The device acknowledges accepted commands with the resulting configuration version. Retries may repeat the same sequence without applying the operation twice. Destructive history deletion requires both an authenticated connection and a button press within thirty seconds. Tare is rejected while the angle indicates an open or moving door. Load-scale changes outside documented bounds are rejected before flash writes.

Power loss during a cycle discards that incomplete cycle. Power loss during calibration leaves the previous CRC-valid calibration active. A journal record is committed by writing data first and its validity marker last. On boot, the firmware scans until the first invalid marker, then resumes at the next slot. If all calibration copies fail CRC, defaults enable diagnostics but maintenance scores remain marked uncalibrated. SWD pads provide recovery from corrupted application firmware; production readout protection policy must preserve an authorized repair path.

## Component and layout review

The nRF52840 requires close attention to its DC/DC inductors, high-frequency crystal option, decoupling network, antenna matching, exposed ground pad, and reference layout. The simplified project represents their architectural positions but should be expanded from Nordic’s reviewed reference design before fabrication. The antenna end must contain no copper, battery, load cell metal, generator, or hinge overlap in its keep-out volume. RF matching values remain designated for tuning with the final enclosure.

The BQ25570 thresholds depend on resistor tolerances and cell chemistry. Over-voltage must remain below the protected cell’s charge limit across tolerance and temperature. Under-voltage should disconnect nonessential loads before the cell protection trips, allowing controlled state storage. Generator maximum voltage is measured with the fastest credible door slam and no load. Rectifier reverse leakage matters at low harvested power, so diode choice cannot be based only on forward drop.

Analog and digital return currents are separated near the HX711 and meet at a controlled ground region. Load-cell inputs route as a differential pair away from the radio, generator, LED, and SWD clock. The bridge connector includes ground-adjacent pins and ESD protection close to the enclosure entry. I2C traces are short and have series-resistor footprints for edge control. Test points expose 3V3, ground, VBAT, HARVEST_DC, load data, load clock, I2C, and reset. Silkscreen marks connector polarity and the hinge-facing side.

The four-layer stack is selected for RF return continuity and easier separation, not because signal density demands it. A two-layer prototype could function, but antenna behavior and analog noise would become more layout-sensitive. Production review must use the actual fabricator stack, dielectric thickness, copper weight, controlled-impedance capability, minimum drill, and solder-mask registration. Mounting holes should have copper keep-outs unless explicitly chassis bonded.

## Service life, repair, and retirement

The battery, friction wheel, pad, and clamp liner are service parts. The enclosure should open with ordinary screws rather than adhesive. A QR code can link to a versioned service guide, but essential reset and polarity markings remain on the product. The app reports cycle count and battery trend to support preventive replacement. No component is intentionally paired to one account or cloud service.

At retirement, the owner can erase local records with the physical reset sequence, remove the protected cell for appropriate recycling, and separate the PCB from polymer parts. The device remains locally usable if the project website disappears. Protocol documentation and export format are in the repository so another application can be written. Replacement firmware must preserve jayis1 attribution for this original design while retaining licenses for any third-party code added later.

## Known limitations and future work

The present design cannot distinguish every source of resistance; a high force may come from hinge friction, seals, latch geometry, wind, or a user’s motion. The app should recommend inspection rather than name a failed component. Sag estimation depends on mount geometry and requires per-installation calibration. Generator output varies greatly with door speed. Metal surroundings may reduce BLE range. The hardware BLE transport, nonvolatile flash driver, secure pairing, and production bootloader remain integration work. KiCad ERC and DRC need a machine with KiCad 8. Mechanical drawings and a fabricated enclosure are not included.

Future revisions can add a removable reference force fixture, signed firmware updates, native iOS support, encrypted facility exports, and fleet comparison that runs entirely on an owner-controlled laptop. Any fleet function should preserve coarse timing and avoid constructing occupant profiles.

Concept, industrial design direction, electronics, firmware, companion application, and documentation are authored by jayis1.