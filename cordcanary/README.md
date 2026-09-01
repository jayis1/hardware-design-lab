# CordCanary

Author: jayis1  
Copyright (C) 2026 jayis1. All rights reserved.

## Overview

CordCanary is a clip-on extension-cord and outlet-head safety instrument that watches the failure modes conventional smart plugs miss. Most home electrical incidents do not begin with a breaker trip. They begin with a warm blade inside a loose outlet, an overloaded space heater pulling repeated surge current through a bargain power strip, a cord folded behind a couch until the copper work-hardens, or a damp garage extension lead developing leakage that is still below breaker thresholds but already high enough to carbon-track. CordCanary is designed for that gray zone between “everything is fine” and “the house smells like hot plastic.” It wraps around the plug head or power-strip input lead, senses the electrical and mechanical stress on the connection, and warns before the damage becomes visible.

The concept is deliberately different from existing smart plugs and energy meters. A smart plug can measure wattage flowing through itself, but it does not know whether the wall receptacle gripping its blades is failing. A thermal camera can show a hot spot, but it is not something most people leave attached to a cord all day. A breaker can interrupt catastrophic faults, but it is blind to gradual mechanical degradation and many localized heating cases. CordCanary fills that gap with a compact, rechargeable, edge-intelligent module that clips around the male plug body or strain-relief section of an extension lead. It combines dual thermal sensing, high-rate current waveform observation, bend and pull strain sensing, humidity and dew-risk monitoring, and inertial event tracking. The result is a practical consumer device for preventing outlet, strip, and extension-cord fires before they happen.

CordCanary is especially useful in apartments, garages, workshops, RVs, classrooms, temporary event spaces, dorms, and older homes where outlets may be worn and extension-cord use is common. It is also valuable for caregivers and facility managers who need to monitor unsafe setups without performing daily manual inspections. The device logs trends over time, scores outlet health, detects intermittent risk conditions, and provides simple actions: reseat, reduce load, dry the area, replace the strip, or retire the cord.

## What makes it novel

CordCanary is not just another inline watt meter. Its novelty comes from the fusion of physical connection health with electrical load intelligence. The device does not sit in the power path. Instead, it instruments the power path from the outside. A flexible clamshell body snaps around a plug head or strain-relief neck and uses a split-core current transformer to measure current non-invasively. Two contactless temperature sensors view the blade-side shell and cord jacket separately, allowing the firmware to distinguish localized interface heating from whole-cord warming. A thin flex strain bridge on the clamp hinge estimates bend radius and persistent side-load on the plug body. A small inertial sensor records drops, repeated wiggling, and vibration signatures associated with loose receptacles or moving appliances. A humidity sensor inside the shell estimates condensation risk when the device is used in garages, sheds, or outdoor-rated temporary installations.

The key invention is the outlet-risk model. Rather than merely reporting current and temperature, CordCanary infers why heating is happening. For example:

- High current plus even thermal rise along the cord suggests legitimate heavy load.
- High blade-side temperature rise with modest current suggests poor outlet contact.
- Strong harmonic noise and elevated crest factor during moderate load can indicate arcing or poor brush commutation from attached appliances.
- Persistent low bend radius plus high current predicts conductor fatigue and insulation stress.
- High humidity plus leakage signature plus overnight cooling predicts condensation-driven surface tracking.

That interpretation layer is what makes the device something that does not widely exist today but clearly should.

## Purpose

CordCanary helps users answer five practical questions in real time:

1. Is this outlet or extension-cord connection running hotter than it should?
2. Is the load safe for the mechanical state of the cable?
3. Is moisture, vibration, or plug looseness creating a hidden fire risk?
4. Is the setup getting worse over days or weeks?
5. What exact corrective action should the user take right now?

## Hardware specifications

### Core processing

- **Primary MCU:** Espressif ESP32-S3-WROOM-1
- **Clock:** 240 MHz dual-core Xtensa LX7
- **Memory:** 8 MB QSPI flash, 8 MB pseudo-static RAM on module variant
- **Security:** secure boot capable, NVS key storage for device pairing
- **Wireless:** Wi-Fi 2.4 GHz and Bluetooth LE for setup and telemetry

The ESP32-S3 is chosen because CordCanary benefits from local signal processing. Current-waveform features, thermal trend analysis, and risk scoring are all computed on device. The MCU also supports BLE-first commissioning and optional Wi-Fi sync without requiring a cloud-only architecture.

### Sensing stack

- **Current waveform sensing:** split-core current transformer feeding ADS131M04 low-noise delta-sigma ADC front end
- **Blade-side thermal sensing:** TMP117 digital temperature sensor aimed at plug blade housing region
- **Cord-jacket thermal sensing:** second TMP117 aimed at insulated cable exit region
- **Bend and pull sensing:** half-bridge flex/foil strain element read by HX711 instrumentation ADC
- **Motion and tamper sensing:** ICM-42670-P 6-axis IMU
- **Ambient humidity/temperature:** Sensirion SHT41
- **Battery gauge:** MAX17048 fuel gauge
- **User feedback:** 2.13-inch monochrome e-paper status strip, RGB status LED, piezo buzzer

### Connectivity

- BLE for local pairing and low-power alert sync
- Wi-Fi for firmware updates, event uploads, and multi-device fleet view
- USB-C for charging and service console
- Optional magnetic pogo accessory port for wall-mount dock

### Power

- 900 mAh LiPo cell
- BQ24074 linear charger and power-path manager
- TPS63031 buck-boost regulator for stable 3.3 V rail
- Typical battery life: 12 days at 2-minute sampling with event-triggered bursts
- Charging time: under 2 hours from depleted state

### Form factor

- Clamshell body: 62 mm x 48 mm x 21 mm
- Weight: approximately 58 g including battery
- Flame-retardant PC/ABS enclosure with silicone inner grip pads
- Split-core CT channel integrated into body hinge
- Replaceable flex strain insert bonded to inner clamp arm

## System architecture

CordCanary uses a layered architecture so the hardware remains practical while the firmware remains explainable.

1. **Acquisition layer** samples thermal, current, strain, humidity, and motion sources.
2. **Feature layer** derives RMS current, crest factor, hot-spot delta, load transients, bend severity, pull persistence, and condensation probability.
3. **Inference layer** classifies the scenario into one of several safety states.
4. **Response layer** updates the e-paper banner, local buzzer cadence, BLE advertisement flags, and app telemetry payload.
5. **Logging layer** stores rolling history for trend analysis and maintenance advice.

### Block diagram

```text
          +---------------- USB-C Charge / Service ----------------+
          |                                                         |
      +---v----+      +-------------+      +------------------+     |
      | BQ24074|----->| TPS63031 3V3|----->| ESP32-S3-WROOM-1 |<----+
      +---+----+      +------+------+      +---+----+----+----+
          |                  |                 |    |    |    |
      +---v----+         +---v---+             |    |    |    |
      | LiPo   |         |MAX17048|            |    |    |    |
      +--------+         +-------+             |    |    |    |
                                                |    |    |    |
      +------------------- I2C -----------------+    |    |    |
      |                                              |    |    |
 +----v-----+  +---------+  +--------+               |    |    |
 | TMP117 A |  |TMP117 B |  | SHT41  |               |    |    |
 +----------+  +---------+  +--------+               |    |    |
                                                      |    |    |
 +----------------- SPI / DRDY -----------------------+    |    |
 |                                                          |    |
 |         +-------------------+                            |    |
 +-------->| ADS131M04 + CT AFE|                            |    |
           +-------------------+                            |    |
                                                             |    |
 +---------------- HX711 ------------------------------------+    |
 |                                                                 |
 +-------> Flex strain bridge                                      |
                                                                   |
 +--------------------- SPI/I2C -----------------------------------+
 |                                                                 |
 +-------> ICM-42670-P IMU                                          
                                                                   
 +---------------- UI / Alert -------------------------------------+
 |                                                                 |
 +-------> E-paper strip, RGB LED, piezo buzzer                    
```

## Firmware design

The firmware is written in portable C and structured as a simulation-ready reference implementation. The repository includes a complete `Makefile` and host-buildable drivers so the logic can be exercised without target hardware. On the real device, the same modules map cleanly onto MCU HAL calls.

### Safety states

CordCanary firmware models six operating states:

- **Nominal:** load and thermals are consistent and safe.
- **Load Watch:** heavy but expected current draw; monitor for sustained heating.
- **Outlet Wear:** localized blade-side thermal anomaly suggests poor receptacle contact.
- **Cord Fatigue:** mechanical bend or pull stress threatens conductor life.
- **Damp Leakage:** humidity and leakage indicators suggest moisture risk.
- **Arc Suspect:** burst noise, crest factor, and heat rise imply intermittent arcing or unstable contact.

Each state has a bounded risk score, human-readable advisory string, local alert policy, and escalation threshold. The firmware does not simply latch the highest state forever. It applies hysteresis and persistence windows so nuisance alerts stay low while genuine trends still surface.

### Why the signal fusion matters

A single sensor cannot explain a dangerous cord setup. Thermal rise alone could mean nothing more than a normal space heater. Current spikes alone could be a vacuum motor starting. Strain alone might just be a neatly coiled cable in a drawer. The firmware instead fuses multiple drivers using weighted evidence:

- Temperature delta between blade side and cord jacket
- Current RMS versus transient burst density
- Crest factor and high-frequency noise proxy
- Pull force persistence and minimum bend radius
- Ambient humidity and internal dew estimate
- Motion signatures tied to plug wobble and accidental tugs

This lets the device recommend a concrete action, not just show a scary red number.

### Data path and timing

- Slow sensor sweep every 2 minutes for routine monitoring
- Fast burst capture at 2 kHz equivalent feature windows when a transient is detected
- UI refresh every 5 minutes or immediately on state change
- Event-log write on state transition or risk-score delta above threshold
- BLE advertisement updates every 10 seconds in active watch mode

### Local user experience

The e-paper strip displays a plain-language condition such as `Outlet getting hot`, `Cord bent too tight`, or `Garage moisture risk`. A single-button side switch acknowledges alerts or toggles installation mode. The piezo buzzer uses distinct patterns: one slow beep for maintenance watch, triple chirp for urgent unplug recommendation. Because the device may live behind furniture, the app mirrors every alert and includes placement guidance.

## Companion application

The companion app in this repository is an installable web interface suitable for mobile or desktop browser use. It uses a single-page architecture with real screens and no external build requirement for the mock implementation:

- **Dashboard:** live safety score, current, hot-spot delta, and battery status
- **Incidents:** timeline of thermal and electrical events
- **Inspector:** simulated outlet-health explanation and root-cause text
- **Placement Coach:** setup guidance for different plug geometries
- **Simulator:** lets users emulate heaters, dehumidifiers, power tools, and damp garages
- **Settings:** alert sensitivity, quiet hours, sync mode, and maintenance reminders

In a production release, the same interface could be wrapped in Capacitor or React Native WebView form. The current repository version focuses on logic, content, and interaction so the concept is easy to review.

## Mechanical design notes

CordCanary’s enclosure is a critical part of the product. The body must be easy to install while preserving accurate sensing.

- The split-core CT is positioned in the hinge so the conductor loop passes through the magnetic window when clipped onto a cord neck.
- The blade-side temperature sensor is mounted on a small rigid tongue looking toward the plug face cavity, isolated from board self-heating.
- The cord-jacket temperature sensor sits near the cable exit to detect distributed heating.
- The strain bridge is laminated into a replaceable flex element so enclosure wear does not permanently degrade calibration.
- A silicone pad improves grip on molded plug heads without crushing insulation.

## Use cases

### Apartments and older homes

Residents often use portable heaters, air conditioners, or kitchen appliances in outlets that have unknown wear. CordCanary catches the pattern where current is acceptable but blade-side heating is not.

### Garages and sheds

Moisture, dust, and temperature swings make temporary cords risky. CordCanary can notice dew-risk and leakage signatures before visible corrosion appears.

### RVs and shore power accessories

Adapters and extension leads are commonly stacked, loaded, and moved. The motion and bend model helps identify dangerous setups in transit or campground use.

### Classrooms and event spaces

Temporary power runs accumulate under tables and stages. Staff can deploy multiple CordCanary units and check fleet summaries without physically inspecting every cord.

### Caregiving and aging-in-place

Family members can watch vulnerable high-load devices such as heaters, kettle cords, or medical-equipment backup strips while respecting privacy because the device tracks electrical safety, not audio or video content.

## Target audience

- Homeowners and renters in older buildings
- Makers and workshop operators
- RV owners and mobile technicians
- Facility managers for schools, churches, and small offices
- Caregivers monitoring heating devices and power strips
- Insurance and safety auditors for recurring problem circuits

## Design decisions and tradeoffs

### Non-invasive sensing over inline switching

CordCanary intentionally does not break the power path. That keeps regulatory scope narrower, avoids adding contact resistance, and allows the device to work with existing strips and extension leads. The tradeoff is that direct watt-meter precision is lower than a fully inline meter, so the design emphasizes risk classification rather than billing-grade energy metering.

### E-paper over OLED

A continuously visible status indicator matters when the device is tucked behind furniture and may not be opened in the app daily. E-paper provides persistent visibility with low quiescent power at the expense of slower refresh. That is acceptable because safety states change on the scale of seconds to minutes, not animation frames.

### Local inference over cloud-only analysis

Hazard alerts must work during Wi-Fi outages and in detached spaces without reliable internet. Local inference ensures the buzzer, LED, and history log remain useful offline. Optional sync adds convenience rather than core safety dependence.

### Dual thermal channels instead of one

Differential temperature is more meaningful than absolute temperature for this use case. A warm cord under heavy load can be normal; a much warmer plug face under moderate load is suspicious. Two channels materially improve interpretability.

## Manufacturing path

A realistic manufacturing path uses a four-layer PCB with antenna keep-out for the ESP32-S3 module, a separate flex strain subassembly, and a two-part flame-rated enclosure with a stainless hinge pin. The split-core CT can reuse geometry from small energy-monitor clamps but at reduced aperture size. Calibration occurs in three steps: thermal offset trim, strain zeroing, and current scale alignment with a known appliance load.

## Regulatory considerations

While CordCanary is non-invasive and not a mains switch, it still lives adjacent to hazardous voltages and must be treated accordingly. Production development should consider:

- IEC/UL enclosure flammability requirements
- Safe creepage and clearance around service/charging ports
- Electromagnetic compatibility from the CT front end and wireless radios
- Consumer guidance for indoor, damp, and temporary outdoor use
- Battery safety and charging thermal management

## Repository contents

```text
cordcanary/
├── README.md
├── firmware/
│   ├── main.c
│   ├── board.h
│   ├── registers.h
│   ├── Makefile
│   └── drivers/
│       ├── thermal.c/.h
│       ├── current.c/.h
│       ├── strain.c/.h
│       ├── motion.c/.h
│       ├── power.c/.h
│       ├── inference.c/.h
│       ├── comms.c/.h
│       └── logger.c/.h
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── index.html
    ├── styles.css
    ├── protocol.js
    ├── app.js
    └── package.json
```

## Conclusion

CordCanary is the kind of device that becomes obvious once described. People already understand that outlets, strips, and extension cords fail in hidden ways, but consumer tools for continuously watching those failure modes are almost nonexistent. By combining non-invasive current sensing, dual thermal monitoring, strain analysis, humidity awareness, and interpretable edge inference, CordCanary turns a neglected home-safety problem into something measurable and actionable. It is original without being speculative, useful without demanding infrastructure changes, and manufacturable using current parts and processes. That is why it is a strong candidate for a real hardware product rather than just a concept sketch.
