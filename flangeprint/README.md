# FlangePrint — External Flange Alignment, Tightening, and Leak-Risk Mapper

Author and creator: **jayis1**

Copyright (c) 2026 jayis1

Hardware license: CERN-OHL-S-2.0 · Firmware and companion software: MIT

Design status: build-verified engineering reference; physical calibration and pressure-code qualification remain required

## Purpose and overview

FlangePrint is a reusable, battery-powered service instrument that clips around the outside of a bolted pipe flange and shows a mechanic how evenly the joint is closing while bolts are tightened. After commissioning, the same instrument can perform a short non-invasive survey for flange rotation, local gap change, acoustic leak signatures, and abnormal temperature. It does not enter the process, replace the gasket, or require a pressure tap. The novel element is the fusion of eight synchronized edge-gap measurements with eight contact-acoustic channels arranged around a split collar. Firmware fits a circular plane to the gap data, separates whole-flange tilt from local distortion, and recommends the next bolt sector instead of merely displaying eight unrelated numbers.

A flange can look aligned while one side of its gasket is nearly crushed and the opposite side is barely seated. Torque values do not prove gasket compression: thread friction, lubricant, corrosion, washer condition, pipe strain, and tightening order all change the relationship between wrench torque and clamp load. Laser shaft-alignment tools can measure flange-face alignment before assembly, ultrasonic bolt gauges estimate individual bolt elongation, and pressure testing reveals a finished joint that leaks. None provides a compact, reusable, outside-only map that guides the closing sequence and then listens for a developing leak at the same angular positions. FlangePrint is intended to fill that workflow gap.

The instrument has two hinged glass-filled nylon semicircles. Eight spring-loaded optical heads view the outer flange-face gap without touching the pressure boundary. Each head uses a VL6180X short-range time-of-flight sensor behind a replaceable sapphire window. Eight bonded piezoelectric contact pickups share the same positions and feed a protected, programmable-gain analog multiplexer. A ninth piezo element injects a coded chirp during offline inspection. An nRF52840 controls acquisition, performs the circular fit, logs jobs, and serves results over authenticated Bluetooth Low Energy. The collar is adjustable from DN25 through DN150 using indexed links; larger pipe sizes use a four-segment variant with the same electronics.

## The problem and the current workaround

Pipefitters presently use feeler gauges, straightedges, dial indicators, torque patterns, bolt-marking paint, and experience. In critical plants they may add hydraulic tensioners, ultrasonic bolt elongation measurements, or flange-management software. Those methods are valuable but fragmented. Feeler gauges give sparse manual points and cannot safely remain during tightening. A torque wrench reports applied torque, not retained preload. Bolt ultrasonics require prepared bolt ends and material calibration. Conventional acoustic leak detectors help after pressurization but do not explain whether the root cause is uneven closure, piping strain, or local gasket damage.

A typical use starts with an operator selecting flange size and bolt count in the app. With the joint hand-tight, the split collar is latched around the flange OD and its eight heads are zeroed. The app displays a polar gap map and calculates parallelism. During each tightening pass it highlights the sector whose residual gap most opposes the fitted tilt vector, while respecting a user-selected cross-pattern and maximum step. The operator records wrench or tensioner values manually, and FlangePrint timestamps the resulting movement. At final closure the device stores the eight-channel fingerprint. A later maintenance survey can compare both geometry and acoustic transfer functions to that baseline.

The primary users are industrial pipefitters, mechanical contractors, commissioning teams, reliability engineers, shipyards, district-heating operators, food and beverage plants, and training centers. It is especially useful where access is limited, torque/preload scatter is high, gasket replacement is costly, or traceable assembly records are required. It is not a substitute for ASME PCC-1 procedures, competent engineering judgment, pressure testing, hazardous-area certification, or legally required inspection.

## Why this is distinct from existing tools

Three adjacent product categories exist. Laser flange-alignment kits measure face offset and angularity before bolting but usually leave the joint before closure. Ultrasonic bolt meters estimate bolt stretch one fastener at a time and depend on bolt-end preparation, acoustic path, and temperature compensation. Handheld acoustic imagers locate pressurized gas noise from a distance but do not capture the mechanical closing history. FlangePrint deliberately overlaps their edges without claiming to replace their highest-accuracy modes: it continuously maps the accessible rim during tightening, uses a baseline-referenced acoustic response, and combines both into one job record.

The instrument's most useful output is not a false absolute gasket-pressure number. It reports directly measured edge gaps, fitted angular misalignment, closure repeatability, local outliers, chirp transfer change, and a bounded leak-risk score. Absolute clamp-load claims would require flange-specific finite-element models and calibrated gasket properties, so the software avoids them. This decision keeps the product honest and makes the same collar applicable across many flange standards.

## Hardware specification

| Subsystem | Selection | Rationale |
|---|---|---|
| Main MCU/radio | Nordic nRF52840-QIAA, Cortex-M4F at 64 MHz | BLE 5, USB, hardware crypto, enough RAM for synchronized traces, mature low-power tooling |
| Gap channels | 8 × VL6180X short-range ToF modules behind sapphire windows | 0–20 mm practical seam range, repeatable digital readings, per-head calibration |
| Sensor addressing | TCA9548APWR eight-channel I2C switch | Identical fixed-address heads, electrical isolation of long flex branches |
| Acoustic pickups | 8 × shielded 27 mm piezo discs with 1 MΩ bias | Wideband contact energy from 2–45 kHz, low cost, replaceable couplers |
| Acoustic excitation | 1 × 20 mm piezo driven by DRV2700 high-voltage amplifier | Repeatable coded chirp for offline transfer measurement |
| Analog front end | ADG1608 multiplexer, OPA1652 charge amplifier, MCP3202 12-bit ADC | Low input bias, programmable channel selection, deterministic SPI capture |
| Temperature | TMP117 at hub plus optional head thermistors | Compensation and abnormal hot/cold sector detection |
| Storage | W25Q64JV 8 MB QSPI NOR | More than 2,000 summarized jobs or temporary waveforms |
| User interface | RGB status LED, sealed action button, magnetic buzzer | Glove-friendly local feedback without opening a phone |
| Connectivity | BLE GATT and USB CDC | Wireless field workflow and wired export/firmware recovery |
| Security | LE Secure Connections, per-device QR public key, signed DFU | No open control characteristic or factory default password |
| Power | 3.7 V 2,000 mAh protected Li-ion, BQ24074 charger, TPS63031 buck-boost | One shift of active surveying; USB-C charging; stable 3.3 V rail |
| Enclosure | IP54 split collar, glass-filled PA12, TPU head boots | Oil-resistant, replaceable wear components, no process penetration |
| Operating range | -10 °C to +60 °C electronics; non-condensing | General industrial service outside classified zones |
| Mass | approximately 620 g for DN25–DN150 kit | Portable one-hand installation |

The electronics PCB is a 92 mm × 48 mm, four-layer board mounted in one collar half. Layer 1 carries components and short analog paths, layer 2 is an uninterrupted ground plane, layer 3 carries power and low-speed branches, and layer 4 carries signals. The piezo charge amplifier sits beside its guard ring and away from the DRV2700 switching node. The high-voltage chirp output has a 1 MΩ bleed resistor, 100 mA resettable fuse, and 3 mm clearance even though the programmed peak is limited to 70 V. Sensor flex cables use keyed ten-pin locking connectors and chassis-drain termination at the hub only.

### Power and runtime

During active geometry acquisition, the MCU, mux, and eight sequential ToF heads average about 145 mA. Acoustic capture adds 35 mA and a chirp pulse briefly draws 300 mA. A normal eight-hour shift containing 150 three-minute jobs consumes approximately 1,050 mAh, leaving cold-weather margin. Between jobs the collar advertises at a slow interval and enters a 420 µA standby state. Long-term monitoring is intentionally not the primary mode: the collar is a service tool, and leaving it on a hot or vibrating process line would require a rated mounting kit and plant approval.

The BQ24074 enforces a 900 mA charge limit and provides power-path operation. Firmware rejects acoustic excitation while charging or below 3.45 V, because amplifier droop would corrupt comparisons. Battery voltage is measured through a switched divider. A 72 °C board threshold disables charging and excitation; a hardware 85 °C thermal cutoff opens the battery path independently.

## Architecture and block diagram

```text
 Eight optical heads ── flex ── TCA9548A ── I2C ─┐
                                                  │
 Eight piezo pickups ─ protection ─ ADG1608 ─┐    │
                                             ├─ OPA1652 ─ MCP3202 ─ SPI ─┐
 Chirp piezo ◀─ DRV2700 ◀─ timer/PWM ────────┘                           │
 TMP117 ─ I2C ────────────────────────────────────────────────────────────┤
 W25Q64 QSPI log ◀────────────────────── nRF52840 ─── BLE/USB ─── App    │
 RGB LED/button/buzzer ◀─────────────────────────────────────────────────┘
 USB-C ─ BQ24074 ─ Li-ion ─ TPS63031 3V3 ─ rails and switched sensor power
```

Acquisition is intentionally serialized. The TCA9548A enables one optical branch at a time, avoiding address collisions and reducing crosstalk. A complete geometry frame takes 160 ms: each head receives two 10 ms ranging cycles, the median is used, and factory offset plus field zero are subtracted. Acoustic channels are sampled at 96 ksample/s for 21.3 ms each. The injected chirp uses a 2–32 kHz logarithmic sweep encoded with a device/job nonce; cross-correlation rejects unrelated machinery better than a simple RMS threshold.

The circular solver models the rim gap as `g(theta) = c + ax*cos(theta) + ay*sin(theta)`. The constant is average closure, while the first harmonic describes flange tilt. Residuals after subtracting this plane reveal local flange distortion, debris, sensor seating faults, or gasket irregularity. Robust Huber weighting prevents a dirty optical window from rotating the entire fit. The recommendation engine finds the largest positive residual projected opposite the tilt direction, then maps it to the nearest legal bolt sector and refuses to recommend if fewer than six channels are valid.

Acoustic processing removes DC, applies a Hann window, computes energy in 2–8 kHz, 8–20 kHz, and 20–45 kHz bands, and compares the normalized vector to the stored dry baseline. Continuous passive listening is used only to flag unusual turbulence. The active chirp is never emitted on a pressurized hazardous-gas line unless the work permit explicitly allows it; the app makes the user select offline, water-service, or approved-service mode before enabling the driver.

## Firmware implementation and design decisions

Firmware is written in C11 around an explicit board contract. `main.c` contains the scheduler, job state machine, command handling, deterministic host simulation, and self-tests. `drivers.c` implements branch selection, filtered ranging, acoustic capture, CRC-protected records, the robust circular fit, scoring, and protocol framing. `board.h` owns data structures and public interfaces; `registers.h` documents the nRF52 peripheral and external-device register constants. The host build uses a deterministic plant model, so the core algorithms can be compiled and exercised with ordinary GCC before target integration.

The job states are idle, zeroing, geometry capture, tightening guidance, acoustic baseline, verification, complete, and fault. Every transition has a timeout. A disconnected head does not silently become zero; validity bits travel with every sample and into each flash record. The instrument requires at least six valid optical channels for a plane fit and seven for a final pass. A sensor that disagrees with both neighbors by more than 1.5 mm is marked suspect, but its raw reading remains available to the app.

Records use a fixed little-endian wire format with magic, protocol version, length, monotonically increasing sequence, payload CRC-32, and commit marker written last. This protects against power loss during flash programming. Configuration writes are copy-on-write between two sectors. Firmware updates use Nordic's signed secure DFU path; the application never accepts arbitrary memory-write commands over BLE.

BLE exposes a read/notify status characteristic, a write-with-response command characteristic, and a credit-controlled bulk log characteristic. Pairing requires numeric comparison against the six digits printed under the collar latch. Commands contain sequence numbers and CRC-16; replayed or malformed frames are rejected. The app can start a job, set bolt count, record a completed tightening step, request logs, and erase a selected job after confirmation. It cannot change calibration without a physical long press.

Several choices are conservative. The design uses eight channels rather than attempting a dense camera because grimy, reflective steel rims defeat uncontrolled imaging. It reports geometric movement rather than inferred absolute gasket stress. It stores raw acoustic snippets only when explicitly requested, reducing privacy and flash wear. Floating-point math is retained on the nRF52840 because its M4F executes the small eight-point fit comfortably and clearer code lowers verification risk.

Build and test the portable firmware with:

```sh
cd firmware
make check
./flangeprint --demo
```

The target rule expects an nRF5 SDK installation and `arm-none-eabi-gcc`. Production bring-up must replace the host platform functions with nrfx TWIM, SPIM, SAADC, QSPI, timer/PPI, BLE, and watchdog integrations while preserving the tested algorithm interfaces.

## Companion application and software interface

The companion is a local-first responsive web application served by a tiny dependency-free Node server. This is appropriate for shared industrial tablets because it installs as a PWA, works offline, and does not require an app-store account. Modern Chromium browsers can use Web Bluetooth; unsupported browsers can import a USB-exported JSON job. No cloud service is required.

The Live screen renders eight gap bars and a polar-style sector map, shows fitted tilt magnitude and direction, and highlights the suggested next sector. The Jobs screen stores job summaries in browser storage, supports baseline comparison, and exports JSON for a flange-management record. The Setup screen validates pipe ID, flange standard, nominal size, bolt count, service mode, and operator initials. The Diagnostics screen shows validity masks, battery, temperatures, firmware version, protocol version, and a safe sensor test. A simulator button feeds deterministic frames so the complete interface can be evaluated without hardware.

The app never claims a joint is safe. Its completion banner reads “instrument criteria met” and lists actual thresholds. The exported report includes calibration ID, invalid channels, firmware/app versions, user-entered tool readings, and the warning that pressure-code acceptance remains the owner's responsibility. Browser storage contains process metadata, so the Clear Data action requires typing the job identifier.

The binary status packet starts with ASCII `FP`, protocol version, state, sequence, timestamp, battery voltage, validity mask, and suggested sector. Eight signed 16-bit gap values use micrometers as units. Mean gap, tilt, direction, and residual follow, then acoustic risk, hottest sector, and CRC-16/CCITT-FALSE. Commands are short CRC-protected frames. The repository includes the same CRC reference vector in C and JavaScript tests so protocol changes cannot silently diverge.

## Calibration and field procedure

Factory calibration uses a granite surface plate and eight traceable gauge-block heights from 0.5 to 8 mm. Each head receives offset and scale terms at 20 °C and 50 °C. A reference steel ring records acoustic transfer curves with three couplant conditions. Calibration data includes instrument serial, head serials, date, fixture ID, residual error, and CRC. Annual verification uses the supplied stepped ring; a head exceeding ±0.12 mm is replaced or recalibrated.

For a job, inspect the pressure boundary and follow the plant's isolation procedure. Clean only the external viewing land. Select the flange and bolt pattern, latch the collar, align sector zero with the marked top bolt, and allow temperature stabilization. Capture hand-tight zero. Follow the app's sector recommendation only when it agrees with the approved tightening procedure; the approved procedure always wins. Enter each wrench or tensioner reading, capture the final fingerprint, remove the instrument, complete prescribed visual and pressure tests, and attach the exported record to the work package.

A later survey must use the same sector-zero reference and collar link positions. The app compensates average temperature but clearly labels comparisons taken outside a ±15 °C window. Increased first-harmonic gap indicates rotation or pipe strain; a single-sector residual suggests local damage or seating change; broadband passive acoustic rise may indicate leakage or rubbing. These are prompts for inspection, not autonomous diagnoses.

## Use cases and target audience

During planned shutdowns, maintenance crews can document that a steam, chilled-water, or process-water flange closed evenly across several passes. Commissioning teams can find pipe-strain-induced rotation before pressure testing. Training programs can show apprentices how a star pattern changes real geometry rather than presenting torque order as an abstract rule. Reliability teams can compare troublesome recurring joints over multiple gasket replacements. Shipyards can use one tool across crowded mechanical rooms where cameras have poor line of sight. Food plants can retain local records without uploading facility layouts or process names to a vendor cloud.

The design is not certified for explosive atmospheres, oxygen service, subsea use, radioactive areas, or direct contact with product. It cannot see hidden inner gasket damage, quantify bolt preload, prove material compatibility, or replace leak testing. Hot flanges require a thermal stand-off kit; surfaces over 60 °C are outside this revision's rating. Strong sunlight, mirror-polished rims, steam, or heavy vibration can invalidate optical channels. The firmware exposes these failure conditions instead of manufacturing a clean-looking answer.

## Bill of materials and economics

At 100-unit volume, the electronics BOM is estimated at USD 118: MCU/radio $10, optical heads $48, analog/acoustic section $18, power and storage $14, PCB/connectors $18, and battery/interfaces $10. Piezo elements, flex branches, molded collar parts, sapphire windows, fasteners, and assembly add roughly $92. Calibration labor, case, reference ring, and charger add approximately $70, producing a landed build cost near $280 before certification and support. A plausible professional kit price is $850–$1,200, below many laser alignment and ultrasonic bolt systems but far above a feeler gauge. Rental at $45–$75 per day may be the better model for small contractors.

## Risks, safety, and honest failure case

The largest reason FlangePrint might not be worth building is that disciplined crews may get adequate results from established alignment checks plus calibrated tensioning, while undisciplined crews may misuse a colorful gap map as permission to deviate from approved procedure. Optical access also varies across flange standards, insulation, guards, corrosion, and cramped installations. If field trials cannot demonstrate repeatable installation within ±0.15 mm and correlate residual patterns with independently measured flange rotation, the guidance feature should be abandoned rather than disguised with machine learning.

Other risks include couplant-dependent acoustic drift, sensor-window contamination, overconfidence in leak scores, and certification cost. Mitigations are replaceable indexed heads, validity masks, baseline-only comparisons, conservative wording, signed records, and a physical excitation interlock. The device must never command a powered wrench. It recommends a sector; a qualified human controls torque, sequence, isolation, and acceptance.

Electrical risks center on the DRV2700 output and lithium battery. The high-voltage output is current limited, fused, discharged by a bleeder, and physically separated. Firmware cannot arm it while charging, overheated, undervoltage, or in passive-only mode, but hardware interlocks remain necessary because software is not a safety barrier. Battery production units require IEC 62133-2-qualified cells, transport testing, crush protection, and a flame-rated enclosure. EMC testing must include the long sensor flexes as installed.

## Verification plan

Bench verification covers every optical head across distance, reflectivity, temperature, angle, ambient light, and contamination. The plane solver is tested with level rings, known wedges, missing channels, single outliers, and local shims. Acoustic validation uses controlled air and water leaks only in a guarded fixture, with rotating machinery, valve chatter, and impact noise as negative examples. Records undergo interrupted-power tests at every flash-write boundary. BLE receives malformed lengths, replayed sequences, bad CRCs, pairing failures, and log floods.

Mechanical tests include one-meter drops in six orientations, collar-cycle endurance, latch overload, flex-cable bend life, oil exposure, window scratching, and vibration. Human-factors trials verify gloved installation and that sector numbering cannot be mirrored. A blinded field study compares FlangePrint results with dial-indicator flange measurements, calibrated bolt elongation, and accepted leak tests. Passing software tests proves only deterministic implementation; it does not establish metrological performance.

## Repository layout

- `firmware/main.c`, `drivers.c`, `board.h`, `registers.h`, and `Makefile`: portable algorithms, deterministic simulation, tests, and target build contract.
- `kicad/device.kicad_sch`, `device.kicad_pcb`, and `device.kicad_pro`: attributed KiCad 8 project with real components, footprints, named nets, board outline, and routing constraints.
- `app/`: installable local-first companion with Live, Jobs, Setup, and Diagnostics views, protocol parser, simulator, and tests.

FlangePrint is a buildable engineering proposal, not a fabricated or certified product. Before field deployment, perform EMC, electrical safety, battery, environmental, optical repeatability, mechanical drop, cybersecurity, and application-specific pressure-code validation.
