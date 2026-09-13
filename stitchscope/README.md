<!-- Author: jayis1 | Copyright (C) 2026 jayis1 -->
# StitchScope

**A non-invasive, clip-on stitch-formation analyzer for domestic and industrial sewing machines**

Author and creator: **jayis1**

Hardware revision: A1

Documentation revision: 1.0

## Purpose

A sewing machine can produce hundreds of apparently normal needle cycles before an operator notices a broken bobbin thread, intermittent skipped stitch, fabric-feed stall, unstable upper-thread tension, or the beginning of a thread nest under the work. The machine usually has no direct way to observe any of those conditions. Existing accessories count stitches or monitor whether one thread is present, while factory systems aimed at industrial automation are integrated into a particular machine and cannot be moved between machines.

StitchScope is a battery-powered sensor bridge that clips to a machine without entering its electrical system or changing its timing. It observes four physical consequences of every stitch: upper-thread force, needle-bar phase, feed displacement, and the acoustic/structural impulse transmitted through the needle plate. A synchronized sensor-fusion algorithm constructs a compact “stitch signature” for each machine cycle. It warns the operator within one or two cycles when that signature indicates a skipped stitch, top- or bobbin-thread break, feed stall, needle strike, emerging birdnest, or tension drift.

The useful novelty is not merely putting sensors near a sewing machine. StitchScope phase-locks unlike signals to the needle motion, learns a baseline separately for each fabric/thread/needle recipe, and compares events in stitch coordinates rather than wall-clock time. It therefore remains useful as speed changes under foot-pedal control. The same puck can move between a vintage mechanical machine, a modern domestic machine, a long-arm quilting head, or a light industrial lockstitch machine.

StitchScope is an advisory instrument, not a machine guard and not a safety interlock. It never drives the motor. A high-visibility RGB indicator and vibration motor provide immediate local feedback; a companion progressive web application supplies setup, traces, recipe management, session review, and CSV export.

## What problem it solves

Sewing faults are expensive because the visible symptom often trails the actual fault. A bobbin can empty while the needle continues to perforate a seam. Slippery layers can stop feeding under the foot while the machine continues cycling. An incorrect needle can deflect and touch the plate intermittently. A small loop below the plate can grow into a birdnest that damages fabric or jams the hook. In upholstery, sailmaking, quilting, prototyping, and small-batch production, finding the defect late means unpicking a long seam and potentially discarding material.

StitchScope gives these machines a reversible instrumentation layer. It helps a learner understand tension and feeding, gives an operator a timely alert while attention is on seam guidance, and gives a maintenance technician objective cycle traces instead of relying only on sound. It also creates a repeatable record when tuning a machine for a difficult material stack.

## Physical concept and form factor

The main enclosure is a 68 mm by 38 mm by 15 mm glass-filled nylon puck weighing approximately 54 g including a 500 mAh lithium-polymer cell. A silicone-lined magnetic/clamp shoe attaches to a flat, stationary area of the machine bed. For aluminum or plastic beds, the kit includes a removable 3M Dual Lock landing pad. The enclosure must remain clear of the needle, presser foot, handwheel, belts, vents, and fabric path.

Three thin peripheral probes connect to keyed locking micro-connectors:

1. **Thread-force gate:** a 14 mm ceramic-eyelet clip routes the upper thread through a low-deflection flexure instrumented with a full strain bridge. The thread is displaced less than 1.5 mm, minimizing added drag. The gate opens so the thread does not need to be cut.
2. **Needle phase tag:** a 6 mm diametric neodymium tag clips around a safe upper portion of the needle bar. A three-axis MMC5603NJ magnetometer in a stationary clip observes its field. This arrangement avoids paint marks, reflective tape, and optical sensitivity to lint.
3. **Feed optical head:** a short-range TMF8821 multizone time-of-flight sensor looks obliquely at the fabric just behind the presser foot. Successive range-zone patterns estimate fabric displacement and distinguish real feed from machine vibration.

The main puck contains an IIS3DWB wide-band accelerometer mechanically coupled to the bed. Its 6.3 kHz bandwidth captures the plate impulse around needle penetration and abnormal hard strikes. Installation fixtures enforce safe sensor locations; no probe belongs below the needle plate or inside moving machinery.

## Hardware specification

### Processing and storage

- **MCU:** Nordic nRF5340, dual Arm Cortex-M33, 128 MHz application core, 64 MHz network core, 1 MB application flash, 512 kB RAM.
- **Radio:** Bluetooth Low Energy 5.3 with coded PHY; no Wi-Fi and no cloud dependency.
- **Storage:** 64 Mbit Winbond W25Q64JV QSPI NOR for approximately 18 hours of feature records or 22 minutes of full diagnostic waveforms.
- **Clocking:** 32 MHz crystal for radio and acquisition timing; 32.768 kHz crystal for low-power session timestamps.
- **Debug:** protected Tag-Connect SWD footprint and USB-C service connector with ESD protection.

The nRF5340 was selected because acquisition and deterministic feature extraction can remain on the application core while the network core handles BLE timing. Its hardware floating-point unit is useful during calibration, although production inference uses bounded fixed-point arithmetic. The design does not require an FPGA because sensor rates and filters fit comfortably within DMA-driven peripherals.

### Sensors and analog front end

- **Upper-thread force:** 0–2 N flexure, nominal 1.0 mV/V full bridge, excited at 3.0 V; INA333 instrumentation amplifier; OPA320 anti-alias stage; ADS131M02 simultaneous 24-bit delta-sigma ADC sampled at 8 ksps. The second ADC channel reads a shielded optional bobbin-case tension fixture for bench service.
- **Needle phase:** MMC5603NJ three-axis magnetometer at 1 ksample/s plus a digital Hall interrupt path for robust cycle boundaries up to 3,000 stitches/minute.
- **Feed displacement:** TMF8821 3×3 multizone direct time-of-flight module at 120 frames/s. The optical window is replaceable and includes an air gap to keep adhesive outside the optical aperture.
- **Bed vibration/acoustics:** IIS3DWB three-axis MEMS accelerometer at 26.667 ksps, SPI DMA, ±16 g. A stainless contact disk and thin viscoelastic perimeter gasket improve repeatable coupling.
- **Environment:** SHTC3 temperature/humidity sensor for strain-zero compensation and recipe context.
- **Power:** MAX17048 fuel gauge, USB-C 5 V input, BQ24074 power-path charger limited to 350 mA, TPS62840 3.0 V buck, and load switches for optical/analog sensor domains.
- **Feedback:** side-firing RGB LED, 10 mm LRA haptic motor with DRV2605L driver, and one recessed multifunction button.

### Connectivity and protocol

BLE exposes a custom StitchScope service. Its control characteristic accepts framed commands for session control, calibration, recipe selection, alert acknowledgement, and chunked log transfer. A notify characteristic emits one 20-byte live feature packet per stitch. A diagnostic characteristic streams decimated traces only when explicitly enabled; routine monitoring therefore has modest radio and battery cost. Device firmware updates use Nordic Secure DFU with signed images and anti-rollback counters.

No account is required. The app stores recipes and downloaded sessions locally. Export uses CSV or a compact JSON bundle, so a shop can retain records under its own policy. BLE bonding is optional in local-only mode and mandatory before enabling firmware update or changing calibration coefficients.

### Power budget

During active sewing, the expected average is 42–58 mA depending on optical reflectivity and BLE connection interval. The 500 mAh cell provides roughly eight hours with conservative conversion loss. Armed-but-idle mode consumes about 3.8 mA because the accelerometer and Hall path remain wake sources. Transport sleep is below 35 µA. Charging may occur while monitoring, but the enclosure thermally separates the charger from the strain front end and firmware suppresses automatic force re-zero while charging.

## Architecture

```text
 Upper-thread flexure -> INA333/OPA320 -> ADS131M02 --SPI/DMA--+
 Needle-bar magnet -> MMC5603 + Hall --------I2C/GPIO--------|
 Fabric surface -> TMF8821 multizone ToF --------I2C---------|--> nRF5340
 Needle plate impulse -> IIS3DWB ------------SPI/DMA---------|    app core
 Temperature/RH -> SHTC3 -----------------------I2C----------|       |
                                                                    | feature ring
 W25Q64 QSPI flash <---------------------------QSPI-----------------+-- classifier
                                                                    |
 Button / RGB / DRV2605L <----------------GPIO/I2C------------------+
                                                                    |
 Phone/PWA <============== BLE 5.3 ============== nRF5340 network core

 USB-C -> ESD -> BQ24074 power path -> LiPo -> TPS62840 3V0 rails
                          |-> MAX17048 fuel gauge -> I2C
```

The acquisition layer timestamps every DMA half-buffer against a 1 MHz monotonic timer. Hall transitions open a cycle window; magnetometer interpolation refines top-dead-center phase and rejects extra transitions caused by magnetic clutter. The resampler maps force and acceleration into 128 phase bins per revolution. Feed frames retain their native rate and are apportioned between adjacent cycles by timestamp.

A feature layer computes peak and integral thread force, pull-up force slope, force-loop hysteresis, penetration impulse energy in three frequency bands, event phase, feed displacement, feed confidence, cycle duration, and cross-cycle variance. It does not keep identifiable audio. The accelerometer is sampled as structure-borne vibration, and routine records store band energies rather than a reconstructable waveform.

The classifier combines transparent rules with a bounded anomaly score. Rules catch physically specific failures: no force recovery after penetration suggests a broken or escaped upper thread; normal needle impulses with collapsing force modulation can indicate bobbin depletion; near-zero optical displacement over multiple cycles indicates feed stall; a high-frequency impulse far above baseline indicates a hard strike. A diagonal-covariance distance over normalized features catches recipe-specific changes not covered by one rule. Alerts use hysteresis and persistence so one unusual seam intersection does not trigger a nuisance warning.

## Firmware design

The `firmware/` tree is portable C11. The included Makefile builds a deterministic host simulator with GCC, while `STITCHSCOPE_TARGET_NRF5340` selects memory-mapped peripheral definitions and board hooks for the target port. This split makes signal processing and protocol parsing testable without hardware. Drivers expose explicit status results and never allocate memory after initialization.

At boot, firmware validates two redundant configuration pages using version, length, monotonic sequence, and CRC-32C. It initializes safe rail states, samples battery voltage, verifies sensor identities, then advertises. Monitoring begins only by a button hold or authenticated BLE command. Failure of one noncritical sensor degrades specific detections and is reported as a capability mask; ADC or timing failure prevents arming.

The scheduler is event-driven. DMA interrupts enqueue fixed descriptors. Main context drains descriptors in priority order, updates cycle state, classifies completed cycles, appends records to a RAM page cache, and services BLE. There is no blocking flash erase while armed: sectors are prepared during idle time. A watchdog requires progress from acquisition, processing, and radio-health tokens.

Calibration has three stages. First, with thread removed and the machine stationary, 512 force samples establish offset/noise and six orientations establish accelerometer bias. Second, the user turns the handwheel through three slow cycles; the phase tracker learns field extrema and rotation polarity. Third, the user sews 30–100 known-good stitches on the intended material. Robust medians and median absolute deviations form the recipe baseline. Baseline adaptation is deliberately slow and frozen whenever any warning is active, preventing a developing fault from becoming “normal.”

Each completed cycle is classified into `OK`, `SKIP_SUSPECT`, `TOP_THREAD_BREAK`, `BOBBIN_DEPLETION`, `FEED_STALL`, `HARD_STRIKE`, `BIRDNEST_GROWTH`, or `UNTRAINED`. Severe strike and thread-break classes alert immediately; subtler conditions require two or three consecutive votes. LED colors communicate status without a phone: green heartbeat is armed/normal, amber double pulse is quality warning, red rapid pulse plus haptic is stop-now, blue is calibration, and purple reports degraded sensing.

The protocol uses a two-byte preamble, version, message type, sequence, payload length, payload, and CRC-16/CCITT. Parsing is incremental and length-bounded. Multi-byte values are little-endian. Commands with side effects include the expected sequence number, making retries idempotent. The app displays firmware/hardware compatibility before allowing a calibration write.

## Companion application

`app/` is an installable offline progressive web application written without a framework dependency so it can be audited and served from any static host. Chromium-based Android, Windows, macOS, and Linux browsers can use Web Bluetooth. iOS users can run the same assets inside an approved BLE-capable WebView wrapper; browser support limitations are shown clearly rather than hidden.

The **Live** view connects to a nearby StitchScope, starts or stops a session, displays current class, stitch rate, tension peak, feed distance, vibration energy, battery, and a scrolling quality history. Alerts remain latched until acknowledged so a transient fault is not missed while watching the seam.

The **Calibration** view guides safe placement and captures zero, handwheel, and good-seam stages. It refuses to advance when noise, magnetic span, or sample count is inadequate. The **Recipes** view stores named combinations such as “8 oz denim / Tex 40 / 90-14 needle,” including expected stitch length and sensitivity. Recipes can be exported independently of device logs. The **Sessions** view receives chunked records, verifies checksums, graphs fault markers, and exports CSV. A simulator button generates local packets for demonstration and UI testing without claiming that simulated values came from hardware.

Application state is persisted in IndexedDB where available and local storage for small settings. There is no analytics, remote API, or background upload. The service worker precaches local assets and uses a cache-first policy for them. BLE access requires a user gesture and the UI explains that the device name and records remain on the local machine.

## Design decisions and tradeoffs

A camera was rejected. Although machine vision can inspect a finished top seam, it struggles with occlusion by the presser foot, cannot directly observe the underside, raises privacy concerns, consumes more power, and requires careful lighting. StitchScope instead measures physical signals already generated by stitch formation. It cannot guarantee the cosmetic correctness of every stitch, but its modalities are complementary and work in ordinary shop lighting.

A microphone was also rejected in favor of a contact accelerometer. Airborne audio varies strongly with room noise and could capture speech. Mechanical coupling is more repeatable and naturally emphasizes machine events. The wide-band sensor costs more than a generic IMU, but hard-strike and penetration features benefit from its bandwidth.

Direct electrical connection to the foot controller or motor was rejected for portability and safety. Speed derives from needle phase. The device therefore observes rather than controls; users must still operate the machine according to its manual and use guards supplied by its manufacturer.

The thread gate introduces a tiny path deflection, so its ceramic surfaces, flexure stiffness, and alignment matter. The gate is placed before the machine’s own tension discs and calibrated only as a relative dynamic-force sensor. StitchScope does not claim to measure standardized static thread tension. A recipe belongs to a gate position and thread path, and the app detects gross repositioning through baseline changes.

ToF feed measurement is less reliable on black, glossy, transparent, or highly textured fabrics. The nine-zone confidence metric identifies poor returns. A reusable low-tack speckle tab can be placed on scrap margin when needed; otherwise the classifier degrades gracefully and labels feed-related detection unavailable. It never converts missing optical data into a feed-stall alarm.

## Use cases

- **Alteration and repair shops:** detect an empty bobbin or skipped stitch before a long hem must be reopened.
- **Quilters and long-arm operators:** flag feed anomalies at thick seam intersections while preserving a map of where warnings occurred.
- **Upholstery, sail, and gear workshops:** monitor difficult multilayer stacks and document setup repeatability.
- **Fashion and product prototyping:** compare needle, thread, tension, and stitch-length recipes objectively.
- **Makers and learners:** see the relationship between handwheel phase, upper-thread force, material feed, and machine sound.
- **Repair technicians:** record a diagnostic trace before and after hook timing, tension, or feed-dog service.
- **Small-batch manufacturing:** obtain per-session quality evidence without replacing otherwise serviceable mechanical machines.

The target audience is a technically curious home sewist through a small industrial workcell. It is especially valuable where material is costly, seam rework is slow, or legacy machines are mechanically excellent but digitally blind.

## Safety, limitations, and validation

StitchScope is not protective equipment, a certified quality system, or a substitute for inspection. Installation must occur with the sewing machine unplugged. Wires and clips must be secured outside all moving paths. The magnet tag must use the specified captive clip; a loose magnet near a mechanism is unacceptable. Users with implanted medical devices should follow manufacturer guidance around magnets. The LiPo enclosure includes strain relief, thermal monitoring, overcurrent protection, and no user-serviceable cell.

False positives can arise at zipper teeth, seam intersections, reverse sewing, thread cutting, and intentional stationary tacking. The app supports event markers and a temporary “expected transition” button. Different stitch mechanisms produce different signatures, so a lockstitch recipe should not be applied to chainstitch or zigzag operation. Revision A firmware targets straight lockstitch first; zigzag support requires a lateral-phase feature and separate training.

Validation should use an instrumented test machine and deliberately seeded faults. At least ten machine models, six fabric families, four thread sizes, and speeds from handwheel motion to 3,000 stitches/minute should be represented. Ground truth should combine underside video after the fact, conductive thread continuity where applicable, high-speed needle footage, and expert seam annotation. Report per-class precision/recall, detection delay in cycles, and nuisance alerts per thousand stitches. Thermal, ESD, radio coexistence, battery abuse, flexure fatigue, lint ingress, and clamp retention tests are required before production.

## Repository layout

```text
stitchscope/
├── README.md
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── stitchscope.h
│   ├── main.c
│   ├── sensors.c
│   ├── signal.c
│   ├── classifier.c
│   └── protocol.c
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    ├── index.html
    ├── app.css
    ├── app.js
    ├── protocol.js
    ├── manifest.json
    ├── sw.js
    └── package.json
```

## Prototype and production path

Bring-up starts with the power tree and SWD, then validates BLE and QSPI before sensor population. Analog testing uses bridge simulators and known weights over the gate’s operating range. A shaker or calibrated impact source characterizes the accelerometer path. Firmware host tests exercise phase interpolation, feature extraction, classifier persistence, framing, malformed packet rejection, and log recovery. On-target hardware-in-loop tests replay synchronized ADC/SPI captures through the same processing functions.

A pilot build should use a four-layer 1.0 mm PCB: signal/top, solid ground, power, and low-speed/bottom. The strain ADC and reference occupy a guarded analog island but retain an unbroken ground plane. Radio keep-out extends through every layer. The contact accelerometer sits near the enclosure’s coupling post, away from charger heat and the haptic motor. Firmware inhibits analysis during haptic actuation to prevent self-generated alerts.

Future revisions may add a second wireless underside puck for research, standardized service fixtures, or machine-specific mounting kits. The core principle remains unchanged: reversible observation, local processing, understandable features, and no authority over machine motion.

## License and authorship

Design, firmware, application, and documentation are authored by **jayis1**. Copyright (C) 2026 jayis1. All rights reserved. This repository presently documents an engineering prototype; no patent, regulatory approval, production warranty, or safety certification is implied.
