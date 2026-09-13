# CondenScope CS-1 — Spatial Condensation-Risk Cartographer

**Author and creator: jayis1**  
**Hardware license:** CERN Open Hardware Licence Version 2 — Strongly Reciprocal  
**Firmware license:** GPL-2.0-or-later · **Companion app:** MIT

## Purpose and overview

CondenScope is a handheld instrument that finds surfaces likely to condense water **before water is visible** and records where those surfaces are in a room. It combines a 32 × 24 long-wave infrared array, a precision air-temperature and relative-humidity sensor, and a time-of-flight rangefinder. Every thermal pixel is converted into a surface temperature, compared with the locally measured dew point, and projected through the measured range into a spatial risk sample. The companion application renders the result as a live false-color map, stores surveys, annotates problem areas, and exports evidence for building owners or clients.

The practical problem is common but poorly instrumented. Surveyors often carry a spot infrared thermometer and a separate hygrometer. A spot reading is easy to aim at the wrong place, misses narrow thermal bridges, and has no spatial context. A thermal camera shows cold areas but normally does not distinguish “cold” from “cold enough to condense under the current indoor moisture load.” Hygrometers report room air conditions but say nothing about surface geometry. CondenScope joins those three measurements and reports the physically useful quantity: **dew-point margin**, defined as surface temperature minus air dew-point temperature. Positive margin indicates reserve; a value near zero indicates that small weather or occupancy changes can produce liquid water.

This is not a medical moisture meter, a leak detector, or a replacement for destructive inspection. It is a preventive mapping tool for building envelopes, refrigerated spaces, archives, greenhouses, boats, vehicle interiors, and industrial enclosures. Its novel element is the synchronous combination of radiometric temperature, ambient psychrometrics, optical range, and pixel direction into a portable, auditable condensation-risk survey. No image camera is required, so surveys can be performed in privacy-sensitive rooms without capturing identifiable visual imagery.

## What makes the device useful

A technician points CondenScope at a wall, window reveal, duct, pipe, cold-store panel, or cabinet. The display-less instrument streams to a phone over Bluetooth Low Energy or USB-C. Safe pixels appear in a thermal palette, pixels within 2.5 °C of dew point turn amber, and pixels within 0.75 °C turn red. Distance gating rejects readings closer than 80 mm or farther than 2.5 m, where spot growth and atmospheric effects would make the map misleading. Pressing the physical trigger starts or ends a survey, so gloves and ladders do not require phone interaction. Audible pitch and an RGB indicator provide eyes-free feedback.

CondenScope records temperature and risk, not ordinary photographs. This enables repeatable measurements in homes, hospitals, changing facilities, rented properties, and server rooms. A contractor can compare the same window reveal before and after insulation work; a conservator can document condensation reserve behind display cases; a refrigeration engineer can identify insulation seams before frost develops. CSV export keeps the measurements usable without a proprietary cloud service.

## Hardware specifications

| Subsystem | Selected part / target | Function |
|---|---|---|
| Main MCU | STMicroelectronics STM32U585AII6, Cortex-M33 at up to 160 MHz, 2 MB flash, 786 KB SRAM | Sensor acquisition, fixed-point radiometry, USB, security, storage |
| Thermal array | Melexis MLX90640ESF-BAB, 32 × 24 pixels, 55° × 35° FoV | Non-contact long-wave infrared surface temperature |
| Ambient sensor | Sensirion SHT45 | ±0.1 °C typical temperature, ±1.0 %RH typical humidity |
| Range sensor | ST VL53L4CD | 1 mm reporting resolution, practical gated range 80–2500 mm |
| Fuel gauge | Analog Devices MAX17048G+T10 | Battery voltage and state-of-charge without a current shunt |
| Nonvolatile survey buffer | Fujitsu MB85RC256V, 32 KiB I²C FRAM | Crash-safe configuration and short offline survey queue |
| Wireless | u-blox ANNA-B112 module, Nordic nRF52832 inside | BLE 5 GATT data link with certified RF implementation |
| Wired interface | USB-C USB 2.0 full speed, USB CDC/vendor framing | Charging, firmware update, and high-rate 8 Hz thermal streaming |
| Charger / power path | TI BQ24074RGTR | 1-cell Li-ion charging, power-path operation, thermal regulation |
| 3.3 V rail | TI TPS62840DLCR | Low-quiescent-current 750 mA buck converter |
| Battery | Protected 3.7 V, 2000 mAh Li-polymer pack | Approximately 7 hours continuous survey operation |
| User controls | Sealed trigger, RGB LED, magnetic buzzer | Start/stop and local risk feedback |
| Programming | Tag-Connect SWD pads | Production test, recovery, and firmware development |
| PCB | 4 layers, 1.0 mm FR-4, ENIG, 82 mm × 46 mm | Solid reference planes and isolated thermal sensor nose |
| Enclosure | 96 mm × 54 mm × 24 mm, PC/ABS, 1/4-20 insert | Handheld, tripod-compatible, target mass under 145 g |
| Environmental target | 0–50 °C, 5–95 %RH non-condensing at electronics | Indoor and sheltered industrial surveys |

The BAA/BAB field-of-view option is intentionally moderate rather than wide. At one metre, one pixel spans roughly 30 mm horizontally, which is suitable for window frames and framing members. A very wide lens would see more wall but dilute narrow thermal bridges. The SHT45 is mounted on a thermally isolated PCB tongue behind a vented PTFE membrane. Copper is removed under that tongue and only thin traces connect it, reducing heat conducted from the MCU and charger. The MLX90640 is at the opposite front edge, behind a germanium-compatible open aperture; ordinary plastic or glass must not cover the infrared sensor.

The rangefinder sits close to the thermal array. Their optical centres are separated by 12 mm, and firmware/app calibration accounts for that parallax. A matte internal baffle prevents charger LED and enclosure reflections from entering the VL53L4CD receiver. The BLE module occupies the rear edge with its antenna keep-out extending through every copper layer. USB ESD protection uses a USBLC6-2SC6 and the CC1/CC2 pins each have a 5.1 kΩ sink resistor, advertising a USB current sink correctly.

## Architecture

```text
              thermal radiation                   room air
 Wall / pipe ───────────────────> MLX90640       ───────────> SHT45
      │                              │ I²C1                     │ I²C2
      └─ reflected 940 nm ─> VL53L4CD│                           │
                                     v                           v
                              +---------------------------------------+
 Trigger / RGB / buzzer <---->| STM32U585 Cortex-M33                  |
                              | calibration → radiometry → dew point  |
                              | pixel projection → risk classification |
                              +----------+---------------+------------+
                                         | I²C2          | UART
                                    MB85RC256 FRAM    ANNA-B112 BLE
                                         |               |
 Battery → BQ24074 → TPS62840 → 3V3      |               +── phone app
    │              │                     |
 MAX17048 ─────────┘              USB FS + DFU
                                         |
                                      USB-C host
```

Two I²C buses prevent the MLX90640 burst traffic from delaying environmental measurements. I²C1 runs at 1 MHz Fast-mode Plus where allowed by the array and reads 834 words per subpage. I²C2 runs at 400 kHz for SHT45, range, fuel gauge, and FRAM. The buses have independent recovery: firmware can clock a stuck sensor line without interrupting the thermal stream. The USB pair is routed as a 90 Ω differential pair. The BLE module uses a framed UART transport internally and exposes the same logical protocol as USB.

The power architecture supports measurement while charging but controls thermal error. BQ24074’s load-sharing output feeds the high-efficiency buck. Firmware marks samples acquired during high charge current and can suppress them until internal temperature settles. The charger, buck inductor, and USB connector are placed at the rear; the environmental sensor nose is separated by a routed slot. Estimated active consumption is 235–280 mW with BLE and sensors active, approximately 420 mW while continuously transmitting full frames over USB, and below 2 mW in shelf mode. The battery has independent protection; firmware state of charge is advisory and never substitutes for that protection.

## Measurement model and design decisions

The SHT45 returns relative humidity and air temperature. Firmware applies the Magnus approximation with constants *a* = 17.67 and *b* = 243.5 °C to compute dew point. The implementation uses Q16 fixed-point logarithm range reduction, avoiding a heavyweight floating-point math dependency and producing deterministic Cortex-M33 timing. At normal building conditions its numerical error is much smaller than sensor uncertainty. Survey reports retain raw air temperature and humidity so a future application can recompute dew point with another standard if required.

The MLX90640 stores per-pixel offset, alpha, thermal-gradient, and supply-sensitivity calibration values in factory EEPROM. On boot, the driver retrieves and validates all 832 words. Chess-mode subpages are acquired alternately at 8 Hz and assembled only after both are valid. Compensation subtracts offset and compensation-pixel response, applies gain and emissivity, and estimates object temperature from radiance. Known bad or outlier pixels are repaired from orthogonal neighbours and flagged by calibration state. The reference C driver demonstrates the complete acquisition and processing flow; production characterization should compare its fixed-point approximation to Melexis’s reference library over a blackbody sweep.

Emissivity defaults to 0.95, appropriate for painted plaster, wallpaper, oxidized masonry, and many non-metal building surfaces. User presets alter it for wood or brick. Shiny metal is a hard radiometric case because reflected room radiation dominates. The app’s “metal tape” workflow instructs the surveyor to place a small patch of known high-emissivity tape on the surface rather than pretending that a single correction can remove reflections. This is a deliberate honesty feature.

Each pixel has a known bearing and elevation in the 55° × 35° field. Combining those angles with range creates a local fan of surface samples. Version 1 assumes the viewed patch is approximately planar at the centre range; it does not claim photogrammetric reconstruction. Phone orientation may later rotate sample fans into a room coordinate system, but core risk classification remains valid without granting motion or location permissions.

Warning and danger defaults are 2.5 °C and 0.75 °C above dew point. They are margins, not probabilities. A margin under 0 °C means the surface is colder than calculated dew point under measured air conditions; visible water can still lag because nucleation, airflow, sensor response, and surface contamination matter. The app always displays the numeric margin and never hides it behind a categorical color.

## Firmware

The `firmware/` tree is freestanding C11 intended for STM32U585. `board.h` is the hardware abstraction contract and pin map. `registers.h` records MCU and sensor registers rather than relying on unexplained numeric constants. `drivers/sensors.c` contains CRC validation, MLX EEPROM parsing, dual-subpage acquisition, fixed-point dew-point calculation, range reading, battery fallback, bad-pixel repair, and pixel-to-angle map generation. `drivers/protocol.c` provides a CRC-16 protected little-endian frame format that works over USB and BLE. `main.c` is a cooperative event loop with watchdog servicing, button debounce, scan-session accounting, alarm output, low-battery handling, and command validation.

The firmware state machine is `BOOT → IDLE → SCANNING`; `CALIBRATING`, `LOW_BATTERY`, and `FAULT` are explicit side states. The trigger starts or closes a session. During a scan, ambient conditions update once per second and thermal subpages update at 8 Hz. Only complete chess frames generate map points. A danger point changes the LED to red and drives a 1.8 kHz alert; warning points use amber and a quieter 900 Hz response. Under 5% estimated charge, scanning stops cleanly and transmits the accumulated summary.

Frames start with magic `0x4353`, protocol version, message type, sequence, payload length, and finish with CRC-16/Modbus. Status, thermal frame, risk point, and summary messages are emitted. Commands start/stop sessions, set emissivity, set thresholds, initiate one-point ambient calibration, and erase volatile session data. Thermal samples use deci-degree deltas around frame ambient with an escape for full centi-degree values. Ordinary indoor frames therefore fit the 1024-byte USB buffer.

The Makefile targets `arm-none-eabi-gcc`; `make syntax` performs host-side strict C syntax checking. A production port adds ST’s startup file, linker script, interrupt vectors, and the concrete board HAL functions declared in `board.h`. Secure boot should use the STM32U5 immutable boot path and signed update images. Debug access should remain enabled for owner repair unless a deployment’s threat model explicitly requires readout protection.

## Companion application

`app/` is an Expo React Native application for Android, iOS, and web development. The Live screen has air temperature, humidity, dew point, range, battery data, a 32 × 24 touch-selectable thermal map, numeric minimum margin, and a start/stop control. Red and amber are applied from margin thresholds rather than arbitrary image scaling. Selecting a pixel reports its coordinates, surface temperature, and exact margin.

The Surveys screen stores completed sessions locally with timestamp, duration, minimum margin, number of danger samples, and editable site notes. It exports standards-friendly CSV via the phone’s share sheet. The Settings screen controls BLE connection state, emissivity presets, thresholds, and acoustic feedback. State persists in AsyncStorage. `protocol.js` implements CRC, command encoding, status parsing, delta thermal decompression, point decoding, and session-summary decoding. `App.js` includes a deterministic simulated stream to make UI development possible without hardware; a native BLE adapter can replace that source while preserving the state model and codec.

Privacy is local-first: no account, analytics SDK, cloud endpoint, image, address, or geolocation is required. A facility may copy CSV files into its own maintenance system. Future BLE integrations should bond with LE Secure Connections and display the short device identifier printed on the enclosure.

## Calibration, manufacturing, and test

Each assembled unit should pass rail-current and SWD tests before sensors are fitted. Functional test reads every device ID, checks I²C rise times, verifies USB enumeration, and measures BLE packet loss in a shielded fixture. Thermal calibration uses two matte blackbody plates near 10 °C and 35 °C after a 20-minute soak. Measurements are compared at the centre, corners, factory-marked bad pixels, and charger-on condition. A correction record with hardware serial, offsets, calibration date, and fixture traceability is written to FRAM.

Ambient calibration places the complete instrument and a traceable reference probe in a slowly stirred chamber, away from radiant walls. One-point trim is limited to ±5 °C in firmware so a bad command cannot conceal gross assembly error. Humidity should be checked near 35%, 60%, and 85% RH with saturated-salt or calibrated chamber references. The range sensor is verified against diffuse targets at 100, 500, 1000, and 2000 mm. Enclosure aperture contamination and membrane placement are inspected optically.

PCB assembly requires no-clean flux control around the SHT45; residues can cause humidity drift. The thermal sensor must remain within Melexis reflow constraints and receive no conformal coating over its aperture. Board fiducials, test pads for both I²C buses, rails, UART, reset, and USB are included in the layout concept. The four-layer stack is signal/ground/power/signal. A continuous ground plane remains under USB and digital buses, while the SHT tongue intentionally narrows conductive paths.

## Use cases and target audience

- **Building surveyors and energy auditors:** discover cold bridges around lintels, slab edges, corners, windows, and insulation discontinuities while simultaneously documenting indoor moisture load.
- **HVAC technicians:** assess diffuser condensation, chilled-water insulation, duct seams, and cabinets under actual operating conditions.
- **Housing maintenance teams:** triage mold complaints without taking privacy-invasive photographs and compare repairs over time.
- **Conservators and archive managers:** monitor cases, exterior walls, and storage surfaces where transient condensation threatens paper, textiles, or metal objects.
- **Cold-chain and refrigeration engineers:** find panel joins or door seals approaching dew point before frost or dripping disrupts operation.
- **Boat, camper, and vehicle owners:** map condensation-prone ribs, glazing edges, and enclosed lockers to guide ventilation and insulation work.
- **Greenhouse operators:** detect framing and glazing zones where nightly condensation can drip onto crops and promote disease.
- **Electronics and control-panel maintainers:** establish margin inside outdoor cabinets before adding heaters, vents, or desiccant.

A normal workflow is: allow the instrument to acclimate for ten minutes; inspect the lens and air vent; choose a surface preset; stand 0.3–2 m away; start a survey; sweep slowly enough to obtain multiple complete frames; add room orientation and weather notes; and export CSV. Repeat surveys should use similar distance, indoor conditioning, and time of day. CondenScope reports the condition at measurement time, not a guarantee against future condensation.

## Repository layout

```text
condenscope/
├── README.md                  This design specification and operating rationale
├── firmware/
│   ├── main.c                 State machine and application scheduler
│   ├── board.h                Pins, constants, and board HAL contract
│   ├── registers.h            STM32U5 and sensor register definitions
│   ├── Makefile               Cortex-M33 and host syntax targets
│   └── drivers/
│       ├── sensors.c/.h       Radiometry, psychrometrics, ranging, fusion
│       └── protocol.c/.h      CRC-protected USB/BLE framing
├── kicad/
│   ├── device.kicad_pro       KiCad project configuration
│   ├── device.kicad_sch       Real symbols, labels, rails, and buses
│   └── device.kicad_pcb       Board outline, footprints, pads, and netlist
└── app/
    ├── App.js                 Live map, surveys, export, settings
    ├── protocol.js            Binary protocol codec
    └── package.json           Expo dependencies and author metadata
```

## Limitations and safety

CondenScope is not safety-certified for explosive atmospheres, medical diagnosis, mains electrical work, or measurement through unknown infrared windows. It must not be aimed at the sun or used where its 940 nm Class 1 ranging emitter violates site rules. The enclosure is not waterproof. Surface readings can be biased by reflections, direct sunlight, heaters, moving air, thermal transients, lens contamination, or insufficient acclimation. Dew-point conclusions are only as representative as the air sampled at the device; stratified or locally humid air may require measurements closer to the suspect surface.

The hardware design is open for review and prototyping, not a declaration of regulatory approval. A product derived from it requires USB, Bluetooth, EMC, battery transport, product safety, and regional radio compliance testing. Li-ion charging component values and thermistor network must be reviewed against the selected cell manufacturer’s limits. All design files, code, and documentation in this directory were created and are credited to **jayis1**.
