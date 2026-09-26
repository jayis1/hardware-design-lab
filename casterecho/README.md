# CasterEcho

Author and creator: **jayis1**
Copyright (c) 2026 jayis1
License: MIT

## Purpose and overview

CasterEcho is a self-powered condition recorder that clips to the fork of a caster wheel and turns ordinary rolling trips into evidence about wheel, bearing, swivel, and floor-contact health. It is intended for hospital beds, warehouse carts, mobile workstations, library trolleys, stage equipment, food-service racks, and factory material-handling equipment. These assets often have four or more small wheels, yet a single damaged caster can make the whole vehicle noisy, hard to steer, unsafe on ramps, and physically demanding to push. Today the usual diagnostic process is waiting for a complaint, lifting the vehicle, spinning each wheel by hand, and guessing which component is at fault. CasterEcho observes the caster under its real load and speed instead.

The device combines wheel-synchronous vibration, structure-borne sound, rotational timing, direction, bearing temperature, and coast-down behavior. A tiny generator coupled magnetically to the rotating hub supplies operating energy. The same rotating magnetic field provides a trusted angular reference, allowing firmware to distinguish a true once-per-revolution flat spot from vibration caused by rough flooring. The sensor wakes when the wheel moves, records a short rolling pass, computes compact features locally, saves them to nonvolatile FRAM, and advertises a health summary over Bluetooth Low Energy. No cloud connection is required. A technician can walk a ward or warehouse with the companion web application, collect records, compare trends, and export a service list.

The novel element is not merely attaching an accelerometer to a cart. CasterEcho uses a speed-normalized, wheel-order model plus generator loading as a passive drag probe. A wheel defect repeats at the same angular position even as travel speed changes; floor joints repeat in distance or time but not at a stable wheel phase. Bearing roughness produces broad high-frequency energy and heat. Tangled thread or brake rub causes rapid coast-down and an abnormal relationship between generated voltage, speed, and vibration. Swivel flutter appears as alternating direction and lateral shocks. By correlating those independent clues, the device gives a repair-oriented result rather than a generic vibration alarm.

CasterEcho is an engineering design proposal, not a certified medical or machinery safety device. Deployment on clinical or regulated assets requires enclosure, electrical, electromagnetic compatibility, cleaning, biocompatibility, and risk-management validation appropriate to that environment.

## Practical problem

A failing caster is deceptively expensive. Operators compensate with higher push force. Loads drift toward one side. A flat polyurethane tread produces noise on every revolution and can damage sensitive payloads. A seized bearing skids, leaving marks and concentrating wear. A fluttering swivel can destabilize tall carts. In hospitals, technicians may receive only a vague report that a bed is “hard to move,” after which the symptom disappears on a smooth workshop floor. In logistics, carts are repaired reactively, while the maintenance team lacks evidence about travel exposure or which wheel is responsible.

CasterEcho gives every wheel its own flight recorder. The unit does not need continuous infrastructure and does not stream raw audio. It records numerical features, fault flags, energy state, and a bounded event history. The technician sees the most probable failure mode and the measurements behind it. This makes replacement decisions auditable, supports condition-based maintenance, and avoids discarding healthy casters in a set.

## Hardware specification

### Compute and radio

The controller is a Nordic Semiconductor nRF52840-QIAA, a 64 MHz Arm Cortex-M4F with 1 MiB flash, 256 KiB RAM, hardware floating point, timers, PPI, SAADC, and Bluetooth 5 support. The floating-point unit is useful for spectral features, while PPI can capture Hall edges without waking the CPU for every transition. The radio operates only after a completed pass or an authenticated local request. The design exposes SWDIO and SWCLK on a Tag-Connect footprint for production programming and recovery.

The firmware host build included here uses a deterministic simulator so signal processing, framing, history behavior, and CRC handling can be compiled and tested without an SDK. Production integration targets Nordic nRF Connect SDK drivers for TWIM, SAADC, GPIOTE/PPI, nonvolatile settings, and Bluetooth. The separation is deliberate: analytics and protocol code remain portable, while `platform.c` is the hardware boundary.

### Sensors

A LIS3DH three-axis accelerometer is mounted close to the caster fork. It samples radial, lateral, and vertical vibration. The hardware supports an interrupt for wake-on-motion and shock detection. A bonded piezoelectric contact pickup extends sensitivity above the useful accelerometer band and captures bearing clicks through the metal fork without recording intelligible airborne conversation. Its signal is biased at midrail, clamped, low-pass filtered before the SAADC Nyquist limit, and sampled on `PIEZO_ADC`.

Two digital Hall channels observe alternating hub magnets. Edge period gives wheel speed and phase; channel order gives direction. Dual channels avoid treating a backward maneuver as impossible motion and help detect swivel chatter. A 10 kΩ NTC bonded near the wheel bearing feeds a divider on the temperature ADC input. Temperature is intentionally a corroborating feature rather than a sole alarm because sunlight, wash water, and ambient HVAC can shift absolute readings.

The generator voltage is measured through a high-value divider. At a known rotational speed, lower-than-expected open-circuit voltage can indicate a detached generator magnet or electrical fault, while voltage under a controlled energy-harvesting load contributes to the drag estimate. Sensor fusion rejects passes with missing Hall timing, saturated acceleration, impossible temperature, or insufficient revolutions.

### Power and energy harvesting

A low-cogging axial generator uses a stationary coil on the fork and a small magnetic ring on the hub. Rotation enters through J1, a Schottky bridge converts either direction, and a BQ25570 energy-harvesting power-management IC performs cold start and stores energy in a 10 F, 3 V supercapacitor. A TPS62740 buck regulator generates the 3.3 V rail. The supercapacitor avoids lithium battery maintenance, thermal exposure, and disposal on fleets with thousands of wheels.

Firmware admits work by energy budget. Wake detection and Hall capture run at very low power. A full 256-sample analysis window is accepted only above the configured capacitor threshold. BLE advertising is deferred when energy is scarce, but completed records remain in FRAM. Brownout cannot partially create a valid record because data is written with a sequence number and CRC. The harvester input has surge clamping and a resettable fuse in the production bill of materials. The generator is sized so its parasitic braking torque is below the variation between clean caster bearings; that requirement must be confirmed on a dynamometer.

### Storage and connectivity

An MB85RC256V I2C FRAM stores calibration and 192 pass records. FRAM tolerates repeated writes better than emulated flash and completes a record quickly when available energy is falling. Bluetooth Low Energy provides a custom service with separate command and notification characteristics. It supports device information, latest record, indexed history, baseline acceptance, history clear, wheel circumference configuration, and live commissioning data. Records are local by default. The protocol uses a magic value, version, payload length, sequence, and CRC-32; malformed or oversized frames fail closed.

### Mechanical form factor

The electronics fit a 58 mm by 28 mm four-layer PCB in a curved, gasketed, glass-filled nylon enclosure. A replaceable stainless strap and elastomer shoe clamp it to caster forks from 3 mm to 10 mm thick. The piezo disk is preloaded against the shoe. The Hall flex tail follows the fork to a molded sensor nose near the hub, maintaining an air gap of 1.5 mm to 3.5 mm. A keyed generator/Hall connector prevents reversed field installation. The enclosure target is IP67, with a hydrophobic vent to limit condensation and no user-opened battery door.

The assembly must clear wheel rotation, swivel motion, brakes, tow bars, bedding, cleaning tools, and pinch zones. A breakaway flex tail is preferable to a cable strong enough to trap fingers. Medical variants require disinfectant-compatible materials and rounded surfaces. Installation includes a spin-clearance check through 360 degrees of caster swivel.

## Architecture

```text
 Hub magnetic ring ---> generator coil ---> bridge ---> BQ25570 ---> 10 F store
          |                                                   |
          +--> Hall A/B --> phase and direction               +--> TPS62740 --> 3V3
                                                                      |
 Fork vibration --> LIS3DH --I2C--+                                  |
 Contact sound ----> piezo --ADC---+--> nRF52840 --> FRAM             |
 Bearing NTC ------> ADC-----------+       |                          |
 Generator sense --> ADC-----------+       +--> BLE summaries --------+
                                           |
                                           +--> order tracking, drag,
                                                scoring, fault flags

 Companion PWA <---- local Web Bluetooth ----> command/notification service
       |-- fleet dashboard    |-- trend chart    |-- service queue    |-- CSV
```

The PCB uses four layers: signal and components on the top, a continuous ground plane, a quiet 3.3 V plane, and secondary signal routing on the bottom. The radio antenna end has a copper and component keep-out. The piezo trace is guarded and kept away from the buck switch node. Generator and supercapacitor currents return directly to the power stage rather than crossing the sensor ground region. I2C has local pull-ups and test points. Every external line receives ESD protection in a fabrication revision even where the concise source schematic represents protection as part of the connector block.

## Firmware operation

### State machine

At rest, the controller retains wake-on-motion, Hall edge capture, and capacitor supervision. The first valid edge changes the state from stopped to starting. Firmware waits for stable speed above 0.08 m/s, then captures synchronized acceleration, piezo, temperature, generator voltage, and Hall-derived speed. A 256-sample window at 800 samples per second gives a 320 ms observation. Successive windows can be joined into a pass until motion stops for 1.5 seconds. Sudden acceleration over the shock threshold is tagged but not automatically interpreted as a wheel fault.

The host implementation demonstrates three windows and a ring history. Production firmware uses timer/PPI-driven sampling and DMA to avoid timing jitter. Watchdog supervision covers the acquisition and radio loops. Each driver reports explicit validity so stale values never silently enter a score. Calibration, history, and settings include CRCs. A failed sensor permits degraded operation only when the remaining measurements can support a bounded claim; otherwise the record carries `SENSOR_FAULT` and no confident maintenance recommendation.

### Signal processing and decisions

Firmware removes the static gravity component from the radial axis and computes RMS vibration, peak shock, and crest factor. A bounded discrete Fourier transform measures energy around the instantaneous wheel rotation frequency and in an 80 Hz to 320 Hz bearing band. Production optimization can replace this reference transform with a fixed-size real FFT without changing the feature contract.

Wheel-order energy is the key discriminator. The Hall period converts each sample to rotational phase. A tread flat generates a sharp pulse at the same phase each turn and elevates the first wheel order and crest factor. Rough concrete may have greater absolute energy, but its peaks do not remain phase locked across speed changes. The baseline is learned from at least twelve passes that an installer declares healthy. After commissioning, it adapts slowly, preventing one bad trip from normalizing a defect.

Drag is estimated from speed decay during unpowered coast intervals and compared with generator output and the learned baseline. Bearing roughness combines high-band energy, RMS growth, and temperature. Swivel chatter uses rapid direction alternation and lateral impulses. Flags are evidence labels: flat spot, bearing roughness, drag, swivel chatter, overheat, sensor fault, low energy, and uncalibrated. A health score starts at 100 and applies capped penalties so one noisy feature cannot make the result meaningless. The application always displays the evidence and action alongside the score.

### Design decisions

No microphone is used. A contact piezo reduces privacy risk and focuses on structure-borne vibration. No permanent cloud identity is required. Device history can be downloaded during a service round and cleared locally. Raw high-rate samples are not retained after feature extraction in the default build, limiting both storage and accidental surveillance value.

FRAM is chosen over internal flash for endurance and short writes. A supercapacitor is chosen over a battery for fleet maintenance. BLE is chosen over Wi-Fi to avoid credentials, idle power, and dependence on building coverage. The custom binary frame is intentionally small and deterministic. CRC-32 detects transfer and torn-write corruption; BLE link security and application-level authorization should be enabled for production but CRC is not presented as authentication.

## Companion application

The `app/` directory contains a dependency-free progressive-style web interface suitable for wrapping in a React Native WebView or using directly in a Chromium-based field device with Web Bluetooth. It provides four real workflows. The dashboard shows health, vibration RMS, drag, temperature, speed, and current findings. The rolling-signature chart compares recent vibration and drag trends. Commissioning writes measured wheel circumference and accepts a known-healthy baseline only after confirmation. The history table maps flags to service actions and exports all local records as CSV.

The interface starts with representative demo records so it can be reviewed without hardware. When connected, it requests the latest record, subscribes to notifications, validates framing and CRC, parses pass features, and keeps a bounded local history in browser storage. A cancelled Bluetooth chooser is distinguished from a connection failure. Invalid wheel dimensions are rejected. The local static server restricts paths and file types, sets `nosniff`, and binds to loopback by default.

A production mobile package should add a signed-device allowlist, authenticated pairing, firmware-update flow, role controls for clearing records, asset barcode association, and encrypted exports. Fleet aggregation remains optional. Facilities may synchronize CSV or structured records into their maintenance system, but the caster remains useful without an account or subscription.

## Use cases and target audience

Hospital biomedical and facilities teams can identify which bed caster causes high push force before moving the bed to a workshop. Records taken under patient-equivalent load are more representative than a hand spin. Overheat is treated as a stop-and-inspect signal, while a stable flat-spot trend can become a scheduled replacement.

Warehouse managers can install units on high-use picking carts and rank maintenance by degradation rather than a fixed calendar. A rising drag index can reveal shrink wrap or thread wound around an axle. Comparing left and right casters can identify misalignment. Material-handling engineers can quantify whether a wheel compound survives a particular floor.

Museums, libraries, studios, and laboratories can monitor quiet carts carrying sensitive objects. The wheel-synchronous metric separates actual tread damage from occasional thresholds. Food-service operations gain a battery-free recorder that tolerates frequent washdowns once the enclosure and connector are validated for their cleaning regime.

Caster manufacturers and OEMs can use CasterEcho during product development. Instrumented durability rigs can correlate tread formulation, bearing seal, load, and floor texture. Rental fleets can document shock exposure and verify that a repaired cart returned to a healthy signature. Accessibility and ergonomics teams can pair the device with push-force measurements to investigate operator strain.

## Safety, security, and failure modes

CasterEcho never controls motion or a brake. It is advisory and must not replace required inspections. Its enclosure must not create a snag, pinch, electrical, infection-control, or balance hazard. Installation torque, strap retention, wheel clearance, and cleaning compatibility are acceptance tests. A detached magnet, broken Hall tail, saturated accelerometer, implausible temperature, FRAM CRC error, low energy, or repeated brownout produces an explicit fault rather than a fabricated health score.

False positives can occur on expansion joints, elevator gaps, aggressive turns, impacts, or unusual loads. The phase model, minimum-revolution rule, corroborating features, and trend requirement reduce that risk. False negatives can occur at very low speed, during insufficient travel, or when a defect is load dependent. The application exposes coverage and calibration status so “no alarm” is not confused with “adequately observed.”

BLE commands that alter calibration or erase history should require an encrypted bonded connection and a physical-presence window opened by the enclosure button. Device information and a coarse health advertisement may remain readable if fleet scanning requires it, but serial numbers should be pseudonymous. Firmware updates require signature verification and rollback protection. Debug access should be locked after manufacturing while retaining a documented recovery process.

The supercapacitor and generator require overvoltage, short-circuit, and reverse-energy tests. The generator must not produce perceptible drag or hazardous voltage at maximum rated speed. Temperature derating, capacitor leakage, cold start, washdown, corrosion, vibration, drop, ESD, radiated emissions, and coexistence need validation. Clinical deployments require the relevant quality and risk processes; this repository does not claim certification.

## Verification and build

Host firmware requires a C11 compiler and `make`:

```sh
cd firmware
make check
make run
```

The self-test checks sensor configuration, Hall-period conversion, the standard CRC-32 vector, packet round trips, corruption rejection, feature extraction, score bounds, history persistence behavior, and command handling. The simulator creates deterministic wheel-order and roughness signals. It is a logic test, not a substitute for sensor characterization.

The application requires Node.js only:

```sh
cd app
npm test
npm start
```

Open `http://127.0.0.1:4173`. Tests cover the CRC check vector, packet round trips, corruption rejection, and payload limits. Web Bluetooth requires a compatible secure browser context when used beyond loopback.

KiCad sources are in `kicad/`. `device.kicad_sch` includes the controller, accelerometer, FRAM, harvester, regulator, generator/Hall connector, programming connector, bridge, and supercapacitor. `device.kicad_pcb` contains assigned footprints, named nets, routing, planes, antenna keep-out marking, and a 58 mm by 28 mm outline. Run:

```sh
python3 kicad/validate.py
python3 verify.py
```

The structural validator checks balanced KiCad expressions, expected references, required nets, footprints, routes, project metadata, authorship, README size, firmware size, firmware tests, and application tests. Before fabrication, open the project in the matching KiCad release, update footprints from verified manufacturer land patterns, complete ERC and DRC, inspect clearances in 3D, review the antenna layout, and have a hardware engineer approve the power stage.

## Prototype and validation plan

Build ten instrumented casters and ten controls across soft tread, hard tread, sealed bearing, and plain-bearing types. Characterize generator torque on a dynamometer from 0.05 m/s to maximum rated speed and at temperature extremes. Measure cold-start distance, energy per meter, sleep consumption, radio energy, and FRAM write margin. Confirm that a depleted unit recovers without external power during an ordinary trip.

Create controlled defects: a molded tread flat of known depth, added radial bearing play, contaminated bearing grease, thread wrap, partial brake rub, swivel preload error, and loosened axle. Run each across smooth epoxy, tile joints, carpet, concrete, ramps, and elevator thresholds under multiple loads and speeds. Keep technicians blind to defect labels. Evaluate detection sensitivity, false-alert rate, fault classification, score repeatability, and time-to-detection. A useful release criterion is not merely high laboratory accuracy; service actions must be more accurate and faster than manual triage.

Perform mechanical retention, obstruction, impact, vibration, ingress, disinfectant, salt-fog, thermal cycling, ESD, and electromagnetic tests. Inspect the Hall flex after full swivel cycling. Verify that enclosure breakaway behavior cannot pull the wheel or expose conductors. Use a calibrated push-force gauge to prove the generator adds negligible operator effort. Conduct privacy review despite the contact sensor, because vibration can still reveal usage patterns.

Finally, pilot the system on a small fleet with known asset IDs. Track every alert through physical inspection and repaired-state verification. Do not enable slow baseline adaptation until enough healthy and failing examples establish that it cannot hide gradual degradation. Publish thresholds with uncertainty ranges and preserve raw validation data. CasterEcho should earn trust by showing evidence, admitting insufficient data, and making a technician’s decision easier rather than pretending to replace one.

## Repository layout

- `firmware/` contains portable C11 acquisition, analytics, storage, protocol, platform abstraction, deterministic simulation, self-tests, register definitions, board contract, and build rules.
- `kicad/` contains the KiCad project, schematic, routed four-layer PCB proposal, and structural validator.
- `app/` contains the local-first companion UI, BLE codec, Web Bluetooth workflow, trend visualization, service queue, CSV export, static server, and protocol tests.
- `verify.py` runs the complete local quality gate and checks that every source file credits jayis1.

CasterEcho was conceived, designed, documented, and authored by **jayis1**.
