# QuadraAlign

**Four-foot machine load and level balancing system**  
**Author and creator: jayis1**  
**Copyright © 2026 jayis1. Licensed under MIT for software and CERN-OHL-S-2.0 for hardware.**

## Purpose

QuadraAlign is a portable commissioning instrument that answers a deceptively difficult question: *is a four-foot machine actually supported evenly, or does it merely look level?* It places four thin instrumented pucks beneath a machine, appliance, workbench, laboratory instrument, or equipment skid. The controller measures load at every foot, machine tilt, transient vibration, and movement stability at the same time. It then tells the installer which leveling foot to turn, in which direction, and approximately how far. A companion application records before-and-after evidence for maintenance and quality records.

A spirit level alone cannot identify a “soft foot”: a foot that touches the floor but carries almost no load because the frame is twisted. A single platform scale cannot reveal load distribution. Feeler gauges can find gaps only while the machine is unloaded or mechanically forced. Laser systems are accurate but expensive and often assume rigid shafts or precision reference surfaces. QuadraAlign combines four synchronized force measurements with two-axis tilt in a field-sized kit. It sees both geometric level and structural support, allowing a technician to correct one without accidentally degrading the other.

This is not another generic development board. It is a complete tool architecture consisting of four 500 kg compression pucks, a precision analog controller, inertial sensing, Bluetooth Low Energy, deterministic embedded firmware, and a guided mobile workflow. The distinguishing function is its coupled estimator. It converts measured corner loading, footprint dimensions, level-screw pitch, and tilt direction into a specific physical adjustment while refusing to certify data taken while the machine is moving.

## Why it should exist

Poorly supported equipment causes bearing wear, objectionable vibration, walking washing machines, inaccurate scales, warped machine frames, cracked laboratory optics, noisy HVAC units, and doors that refuse to stay aligned. Installation crews usually adjust for visible level, then stop. That misses unequal support and can leave one foot effectively floating. Conversely, equal force is not always desirable if the machine has an intentionally offset center of mass, so QuadraAlign exposes both raw values and a configurable acceptance policy rather than hiding the measurements behind a single score.

The instrument is useful during first installation, after relocation, after floor repairs, and in predictive-maintenance rounds. Captures become a baseline. If corner loads drift while the floor angle does not, the frame or internal load may have changed. If angle and center of load move together, the floor, shim, or leveling screw may be settling. Rising vibration with stable static geometry points toward a dynamic machine fault rather than installation geometry.

## System overview

The standard kit has four 92 mm diameter, 14 mm thick stainless pucks. Each puck contains a sealed 500 kg full-bridge load cell and an elastomeric top disk that permits small angular compliance without side-loading the sensing element. Four keyed Micro-Fit cables connect to a handheld controller. A local ADS1234 provides four ratiometric 24-bit conversion channels from one shared, low-noise bridge excitation supply. An STM32G071 performs acquisition and estimation. A LIS2DW12 accelerometer mounted parallel to the controller datum plate measures tilt and vibration. During use, the controller magnetically docks to a designated flat datum on the machine; an optional rigid angle bracket provides a repeatable reference when the machine lacks a suitable horizontal surface.

The controller reports over BLE through an nRF52840-based Raytac MDBT50Q module. Separating radio duties from the measurement MCU isolates protocol timing and radio updates from the critical sampling loop. USB-C supplies charging and wired service power. A protected 2,000 mAh lithium-polymer cell supports approximately fifteen hours of typical commissioning use. The enclosure is a 132 × 72 × 24 mm glass-filled nylon handheld with a rubber perimeter, one capture/tare button, and a high-visibility RGB status light.

## Hardware specifications

### Measurement controller

- **Primary MCU:** STM32G071KBT6, Arm Cortex-M0+ at 64 MHz, 128 KiB flash, 36 KiB RAM. It owns real-time acquisition, calibration, filtering, fault handling, flash persistence, and adjustment estimation.
- **Load converter:** Texas Instruments ADS1234IPW, four differential 24-bit bridge channels, programmable gain 128, ratiometric operation, 10 or 80 samples per second. Production firmware interleaves channels and rejects the settling samples after every multiplexer transition.
- **Inertial sensor:** ST LIS2DW12TR three-axis accelerometer. The 200 Hz high-performance mode supports static gravity estimation and a vibration RMS gate. The design does not integrate acceleration into displacement, avoiding drift-heavy claims.
- **Radio:** Raytac MDBT50Q-RX module using Nordic nRF52840 silicon. It exposes a custom BLE GATT service, receives framed UART telemetry from the measurement MCU, and supports device firmware update independently.
- **Load cells:** four 500 kg, 2.0 mV/V, 350 Ω full Wheatstone bridges in low-profile stainless compression elements. Individual serialized calibration is stored in the controller.
- **Bridge excitation:** 3.0 V low-noise rail, shared between ADC reference and bridges for ratiometric cancellation. Four high-side switches permit open-wire diagnosis and lower standby draw.
- **Power:** USB-C 5 V input, BQ24074 power-path charger, protected single-cell LiPo, TPS63031 buck-boost system rail, and TPS7A2033 low-noise analog rail.
- **User controls:** sealed momentary button; short press marks a capture, long press initiates tare after warning. RGB LED communicates settling, balanced, adjustment-needed, overload, and low-battery states.
- **Connectors:** four keyed 4-pin Micro-Fit 3.0 load-cell inputs, USB-C, and recessed SWD programming pads.
- **Environmental target:** controller IP54, pucks IP67, 0–45 °C compensated operating range, -20–60 °C storage.
- **Resolution target:** 0.05 kg displayed below 200 kg per puck; absolute field accuracy ±0.25% of reading after calibration; level repeatability ±0.03° on a clean datum.
- **Safety limit:** 500 kg per puck and 1,600 kg recommended combined working load. Firmware asserts overload at any individual channel limit because total load alone cannot protect one corner.

### Mechanical form factor

The controller PCB is 80 × 55 mm on 1.6 mm FR-4. The four-layer production stack is signal/ground/power/signal; the repository’s routable two-layer representation documents placement, nets, and critical connectivity and can be promoted to four layers during manufacturing preparation. Analog circuitry occupies the connector end, separated from the BLE antenna keep-out. Load-cell differential pairs are length-balanced, guard-referenced, RC filtered at the converter, and kept away from switch-mode current loops.

Each puck has a captured cable strain relief and a replaceable nitrile contact disk. The load path does not pass through the PCB or cable gland. The slightly crowned force button prevents edge contact. A bright numbered collar establishes an unambiguous front-left, front-right, rear-left, and rear-right mapping. A transport spacer protects the cells from shock while packed.

## Architecture and block diagram

```text
  [Puck FL]--bridge--\
  [Puck FR]--bridge---+--> [input protection + RC] --> [ADS1234 24-bit ADC]
  [Puck RL]--bridge---+              |                       |
  [Puck RR]--bridge--/        3.0 V ratiometric              | clock/data
                                                             v
  [LIS2DW12 accelerometer] -- I²C --> [STM32G071 measurement MCU]
                                             |       |       |
                                     RGB/button   flash    framed UART
                                                             |
                                                     [MDBT50Q nRF52840]
                                                             |
                                                           BLE 5
                                                             |
                                                [iOS / Android application]

  USB-C --> BQ24074 charger/power path --> LiPo --> TPS63031 system supply
                                                   +--> TPS7A2033 analog rail
```

The trust boundary is deliberate. Raw sensing, calibration, overload detection, and stable-state decisions stay on the STM32. A phone disconnection cannot interrupt measurement or permit an overload to go unnoticed. The radio module transports versioned binary frames but cannot silently alter calibration. Configuration writes include range validation and CRC protection. The application presents data and commissioning history but labels demo data and never fabricates a connected status.

## Measurement method

Before loading, the operator lays the four pucks on a clean floor and performs tare. The tool averages 32 readings per channel and stores offsets. The machine is lowered gently onto all four pucks. The operator enters the center-to-center foot width and depth, plus leveling screw pitch. Those dimensions are project data, not hidden constants.

For each report, the estimator applies a trimmed moving average to every load channel. It sums the four forces and computes deviation from ideal quarter loading. The result called **imbalance percent** is the largest absolute corner departure divided by ideal corner load. This conservative metric identifies the worst foot rather than allowing opposite errors to cancel. Center of load follows the normalized difference between left/right and front/rear pairs, scaled by half the measured footprint.

Pitch and roll come from the gravity vector. The estimator calculates vibration as acceleration magnitude departure from one gravity across a fixed observation window. A report becomes stable only after the filter is populated and mean vibration remains below the acceptance gate. A stable flag does not mean the machine is safe to operate; it means the static reading is not contaminated by detectable motion.

Adjustment selection projects pitch and roll onto the four corners. The lowest corner receives priority. When the instrument is already geometrically level, the least-loaded corner is selected to address soft foot. Approximate turn count derives from the tangent of combined tilt angle multiplied by the larger footprint dimension, divided by screw pitch. The result is clamped to eight turns. It is guidance, not an unattended actuator command: the operator adjusts in small passes and watches live feedback.

## Firmware

Firmware under `firmware/` is portable C11 and exceeds 500 lines of implemented code. `main.c` contains the application state machine, command handling, button semantics, status indication, acquisition schedule, and self-test. `sensors.c` contains ADS1234 acquisition, LIS2DW12 setup and decoding, tare, calibration, filtering, center-of-load mathematics, adjustment estimation, CRC32 configuration protection, and defaults. `protocol.c` implements a bounded binary parser with CRC-16/CCITT, command queue, typed little-endian telemetry, and outbound events. `board.h` isolates target hardware. `registers.h` records peripheral addresses and command values. `board_sim.c` provides a deterministic host implementation used for continuous validation.

The primary loop samples at a nominal 200 Hz scheduling cadence and reports at 10 Hz. On production hardware, ADC availability naturally bounds actual bridge samples because channel conversion and settling are slower than the loop. There are no dynamic allocations, recursive calls, unbounded strings, or blocking radio dependencies. Every parser length is checked before payload use. Calibration permits only a positive loaded reading and a meaningful reference mass. Limit updates are accepted only within defined ranges.

Compile and execute the host validation with:

```bash
cd firmware
make clean
make
make test
```

The host target is not a mock UI. It exercises configuration write/read with CRC, the standard CRC-16 test vector, simulated sensor acquisition, estimator state, telemetry encoding, and finite-result checks under `-Wall -Wextra -Wpedantic -Werror`. For STM32 deployment, replace `board_sim.c` with the HAL or register-level target implementation and define `QUADRALIGN_EMBEDDED`. Pin assignments are centralized in `board.h`; peripheral constants are in `registers.h`.

### Firmware design decisions

A trimmed average was selected instead of a long low-pass filter because puck placement often creates isolated mechanical impulses. Removing the high and low member of the short window rejects a single spike while preserving useful response. Floating point is acceptable on this low-rate instrument and keeps calibration readable, but raw ADC acquisition remains integer. Flash records carry a CRC rather than trusting erased-memory patterns. The separate CRC-16 transport catches UART framing corruption between MCUs.

The overloaded flag is evaluated per channel before totals. Configuration defaults are safe enough for simulation, but production units must be calibrated with traceable masses. Temperature coefficients are reserved per channel so a production temperature sensor can apply characterized correction without changing record shape. Unknown commands are ignored. A sustained sensor failure enters a visible fault state rather than sending stale data as current.

## BLE protocol

Frames begin with `0xA5`, followed by type, one-byte payload length, payload, and little-endian CRC-16/CCITT over type, length, and payload. Telemetry includes sequence number, four corner loads, total load, imbalance, X/Y center, pitch, roll, vibration RMS, recommended foot, recommended turns, battery voltage, and packed state flags. Commands cover tare, capture, threshold update, channel calibration, and sleep. Payloads remain below the negotiated 185-byte MTU.

A production radio image exposes service UUID `7a100001-6fd5-4b6c-a4f8-34aa51414c31`, telemetry characteristic `...0002`, and command characteristic `...0003`. Calibration should require a bonded administrative connection. Ordinary viewing may be available without bonding for quick service work, depending on site policy.

## Companion application

The React Native application under `app/` is authored in strict TypeScript. `App.tsx` implements three functional screens:

- **Live** shows connection state, total supported mass, each corner load, highlighted adjustment foot, a two-axis bubble indicator, tilt, imbalance, vibration, tare, and capture controls.
- **Guide** turns estimator output into a safe, numbered field procedure. It shows the selected foot, estimated turn count, center of load, and lockout reminder rather than encouraging blind adjustment around moving equipment.
- **Captures** stores timestamped commissioning readings, stability state, and imbalance for comparison. The provided implementation keeps captures in the current session; production integration should persist them through encrypted local storage and export signed CSV/PDF records.

`src/model.ts` defines strongly typed measurements and projects, validates frame shape and CRC, decodes little-endian telemetry, creates every command frame, and provides clearly marked demo measurements for development. Replace the demo update timer with a BLE adapter such as `react-native-ble-plx`, feeding notifications to `decodeTelemetry` and writing output from `commands`. Keeping BLE transport behind that boundary makes the visual and protocol logic testable without radio hardware.

Run `npm install` followed by `npm run typecheck`. A normal React Native platform scaffold can host these sources for Android or iOS. The app deliberately avoids cloud dependence. Fleet integrations can export captures to a CMMS, but local measurement and guidance remain available offline in basements, plants, and secure labs.

## KiCad design files

The `kicad/` directory contains `device.kicad_sch`, `device.kicad_pcb`, and `device.kicad_pro`. The schematic defines real functional symbols for STM32G071, ADS1234, LIS2DW12, MDBT50Q, TPS7A2033, and four bridge connectors. Named nets document the UART, I²C, ADC clock/data, analog rails, and individual differential bridge channels. The PCB establishes the board outline, real footprints, pads assigned to those nets, routed critical signals, ground via, silkscreen ownership, connector placement, and an antenna-side layout.

Before fabrication, an engineer must complete production-specific decoupling, ESD parts, USB-C configuration resistors, SWD header, load-cell input protection, charger passives, antenna clearance, all four physical connectors, mounting holes, four-layer stack, filled ground planes, and formal ERC/DRC. Those are explicitly manufacturing gates, not implied by the conceptual routing. The files are an editable electrical design rather than images or empty placeholders.

## Calibration and commissioning workflow

1. Inspect puck faces and verify that all transport spacers are removed.
2. Put the pucks on an unloaded, rigid, level reference surface and long-press tare.
3. Calibrate each channel with a certified mass near the expected operating region. Calibration must transfer force concentrically through the puck button.
4. Enter the machine foot spacing and leveling-screw pitch in the project.
5. Lock out the machine, lift it according to the manufacturer’s instructions, and place the numbered pucks under corresponding feet.
6. Lower slowly. Stop immediately if one puck approaches 500 kg, a cable is trapped, or the frame is unstable.
7. Attach the controller datum to a known machine surface. Wait for the stable indication.
8. Save a “before” capture. Follow the guide in fractions of the estimated turn count, tightening jam nuts and allowing the structure to settle after each pass.
9. Accept only when site-specific load and level tolerances both pass. Record shims or intentional load bias in the maintenance note.
10. Save an “after” capture, unload the pucks evenly, clean them, and reinstall transport spacers.

## Use cases

**Appliance installation:** A washer may be visually level but rock during spin because one rear foot is nearly unloaded. QuadraAlign finds that foot before repeated vibration damages flooring or hoses.

**Machine tools:** Mills, lathes, and precision saws require geometry that can shift as a cast frame is pulled against uneven feet. Technicians can balance support before fine optical or test-bar alignment.

**Laboratory equipment:** Optical tables, centrifuges, environmental chambers, balances, and analytical instruments benefit from documented support and low vibration. The raw center-of-load view also catches shifted internal modules.

**HVAC and pumps:** Rooftop and plant equipment often settles on pads. Periodic captures distinguish support changes from rotating faults and provide objective evidence before re-shimming.

**Accessible furniture and workstations:** Heavy adjustable benches, cabinets, printing equipment, and medical carts can be stabilized without trial-and-error rocking tests.

**Commissioning and insurance evidence:** Installers can attach a measurement record showing that equipment was within agreed limits when handed over, reducing disputes after relocation or misuse.

## Target audience

QuadraAlign is intended for industrial maintenance technicians, millwrights, appliance installers, field-service engineers, laboratory managers, machine-tool owners, HVAC contractors, facilities teams, and careful advanced makers. It is not a certified lifting accessory, a legal-for-trade scale, an intrinsically safe instrument, or a substitute for the equipment manufacturer’s alignment process. Users must understand safe lifting and lockout procedures.

## Limitations and safety

Never place hands beneath suspended equipment. Pucks measure compression only and must not be exposed to shear, impact, unstable cribbing, or loads beyond their rating. A tilted or compliant floor can affect both force and angle interpretation. Flexible frames may redistribute weight after the pucks are removed; where final conditions matter, repeat checks using permanent shims or jack bolts. Magnetic mounting is inappropriate near sensitive magnetic equipment. Vibration RMS is a settling gate and trend signal, not a standards-compliant vibration analyzer.

Absolute accuracy depends on cell selection, mechanical load introduction, temperature characterization, and calibration. The present PCB is a design baseline, not a released safety-certified product. A production release needs EMC testing, battery safety review, ingress testing, connector pull testing, overload proof tests, calibration procedures, and risk analysis under relevant regional standards.

## Repository layout

```text
quadralign/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── board_sim.c
│   ├── main.c
│   ├── protocol.c
│   ├── protocol.h
│   ├── registers.h
│   ├── sensors.c
│   └── sensors.h
├── kicad/
│   ├── device.kicad_pcb
│   ├── device.kicad_pro
│   └── device.kicad_sch
└── app/
    ├── App.tsx
    ├── package.json
    ├── tsconfig.json
    └── src/model.ts
```

## Authorship

QuadraAlign, its hardware architecture, schematic, PCB baseline, embedded firmware, protocol, application, documentation, and device concept were created by **jayis1**. Contributions should preserve attribution to jayis1 in source headers and design metadata.
