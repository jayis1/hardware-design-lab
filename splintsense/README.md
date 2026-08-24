# SplintSense

**Author: jayis1**

SplintSense is a new class of orthopedic aftercare device: a flexible, low-profile sensing liner that lives between a removable splint or cast shell and the patient’s skin, continuously measuring the hidden conditions that clinicians rarely get to see until a problem has already become painful, expensive, or dangerous. The design combines distributed capacitive moisture sensing, skin-contact pressure mapping, volatile organic compound tracking, temperature and humidity monitoring, and inertial event detection into a reusable electronics spine and disposable sterile liner system. It is intended for fractures, ligament injuries, post-operative braces, pediatric immobilizers, and sports medicine recovery kits where pressure sores, trapped moisture, odor buildup, liner migration, and premature activity can quietly undermine healing.

Traditional casts and splints do one thing very well: immobilize anatomy. They do not, however, tell the wearer or the clinician whether a hot spot is developing under the heel, whether sweat has pooled around an incision site, whether the brace shifted overnight, or whether the patient took an impact strong enough to justify a precautionary check. SplintSense addresses that blind spot by creating a soft “digital underlayer” that can be wrapped around the limb before the rigid immobilization hardware is applied. The sensing liner routes to a coin-sized reusable electronic module mounted on the exterior edge of the brace, where it handles processing, BLE connectivity, haptic notifications, battery management, and secure event logging.

The novelty of SplintSense is not any one sensor by itself; it is the combination of hidden-surface environmental sensing, pressure analytics, recovery-aware firmware, and clinician-friendly interaction in a form factor that respects the constraints of orthopedic care. Existing wearables observe whole-body activity, but they do not meaningfully characterize the sealed microclimate and load distribution inside immobilization hardware. Smart insoles can sense foot pressure, but they cannot monitor a forearm splint, a post-surgical boot liner, or a pediatric wrist brace. Hospital dressing monitors often focus only on temperature or moisture. SplintSense is intentionally broader: it watches the environment, the contact mechanics, and the patient behavior at the same time, then translates that into recovery risk signals that are useful in home care rather than only in a lab.

## Device name, purpose, and overview

SplintSense is designed to reduce preventable complications during orthopedic recovery. In practice, the device aims to detect five classes of issues:

1. **Pressure concentration** that can lead to skin breakdown, numbness, circulation reduction, or discomfort.
2. **Moisture retention** from sweat, bathing accidents, or environmental exposure that softens skin and increases odor and infection risk.
3. **Thermal and VOC anomalies** that may correlate with poor ventilation, excessive activity, or wound-related changes in a post-operative splint.
4. **Brace migration or poor fit** inferred from shifting pressure maps and repeated micro-motions.
5. **High-impact or overuse events** that suggest the immobilized limb experienced more stress than prescribed.

A reusable electronics pod clips into a sealed rail on the splint shell. That pod interfaces with a thin flex liner containing a matrix of capacitive moisture electrodes, force-sensing cells, and routed sensor breaks. During daily use, the firmware builds a rolling picture of the recovery state. Instead of delivering noisy raw data to the user, the device computes actionable outputs such as “heel pressure asymmetry increased 18% overnight,” “liner moisture remained elevated for 42 minutes,” and “two impact events exceeded safe gait threshold.” Clinicians can review snapshots, trend lines, and fit quality metrics in the companion app.

The system is intentionally modular. A wrist configuration uses a short eight-zone liner and a compact pod. An ankle/boot configuration uses a longer twelve-zone liner and a stronger battery. The same firmware architecture supports both, because the sensor abstraction layer discovers enabled channels at boot and maps them to calibration profiles. The design is also well suited to rental and reuse workflows: only the skin-contact liner is disposable, while the electronics pod is sanitized and reissued.

## Hardware specifications

### Core electronics

- **Primary MCU:** Nordic nRF5340, selected for dual-core BLE performance, low power consumption, field-proven medical-adjacent wearable ecosystem support, and enough RAM/flash headroom for trend modeling and encrypted session logging.
- **Sensor front-end:** Texas Instruments FDC2214 for multi-channel capacitive moisture strip measurements across the liner grid.
- **Pressure acquisition:** Analog Devices AD7147 capacitance-to-digital converter paired with printed compressive sensor cells, giving a thin and conformal pressure-sensing approach without rigid load cells.
- **Environmental sensing:** Sensirion SHT45 for temperature/humidity and Sensirion SGP41 for VOC trend estimation in low-airflow enclosed spaces.
- **Motion sensing:** Bosch BMI270 IMU to capture step-like vibration, impact magnitude, orientation drift, and limb movement compliance.
- **Battery management:** TI BQ25185 single-cell charger with programmable current limits, complemented by a MAX17048 fuel gauge.
- **User feedback:** DRV2605L haptic driver and a tiny side-firing white LED for silent, discrete alerts.
- **Storage:** 64 Mbit QSPI NOR flash for seven to fourteen days of summarized recovery telemetry and incident markers.
- **Connectivity:** Bluetooth Low Energy 5.3 with secure pairing, fast snapshot transfer, and optional clinician export over a companion phone.

### Sensor topology

The liner includes three sensing domains:

- **Moisture strips:** serpentine interdigitated electrodes distributed along bony prominence regions and high-sweat zones.
- **Pressure cells:** compressible dielectric islands laminated into the liner to measure relative contact intensity and sustained loading.
- **Context sensors:** a rigid-flex island near the pod houses the IMU, humidity, temperature, and VOC sensors in a vented chamber that samples the splint microclimate while remaining protected from direct sweat droplets.

In an ankle boot variant, the pressure map covers heel, Achilles, medial ankle, lateral ankle, forefoot, and shin contact areas. In a wrist variant, it covers ulnar side, radial side, palm bridge, dorsal forearm, and strap regions. Because the liner is flex-based, clinicians can trim guide tabs at non-active margins without damaging the sensing network.

### Connectivity and power

SplintSense communicates primarily over BLE to minimize complexity for home users. The pod advertises a compact recovery-status service that can be read by the app in under three seconds for a quick status check, while richer history sync runs in the background. The system is optimized for a 55–90 mAh LiPo cell in wrist form and a 180–250 mAh cell in ankle form. Typical power strategy includes deep sleep between scheduled scans, adaptive sensor cadence that slows during calm periods, IMU wake-on-motion for event capture, haptic-only user alerts to avoid power-hungry displays, and batch flash commits instead of per-sample writes.

Expected battery life is roughly 7 days on wrist mode and 10 days on ankle mode under four-minute baseline scan intervals with event-triggered bursts. Charging occurs via magnetic pogo adapter or wireless dock depending on final industrial design choice.

### Form factor

The reusable pod targets a footprint of approximately 32 mm x 24 mm x 7 mm with an overmolded medical-grade TPU enclosure. The liner is a breathable multilayer laminate: skin-safe wicking textile, printed sensor flex, microperforated foam, and adhesive capture tabs. The pod clips to an external rail integrated into a splint or accessory shell, keeping hard electronics away from skin and allowing the liner cable tail to remain short.

## Architecture and block diagram

The electronic architecture is intentionally split between a soft sensing plane and a protected compute pod.

```text
[Printed moisture strips] --\\
[Printed pressure cells ] ----> [Analog front-end: FDC2214 + AD7147] --\\
[SHT45 / SGP41 / BMI270] -----------------------------------------------> [nRF5340 MCU] --> [BLE 5.3]
[MAX17048 fuel gauge ] ----------------------------------------------->  /      |          [Mobile app]
[BQ25185 charger      ] ---------------------------------------------->/       |          [Clinician export]
[QSPI flash           ] -------------------------------------------------------|
[DRV2605L haptics     ] <------------------------------------------------------|
[Status LED           ] <------------------------------------------------------|
```

The firmware stack is organized around five layers:

1. **Board support layer** for rail selection, timing, battery thresholds, and SKU-specific constants.
2. **Driver layer** for BLE transport, power, pressure, moisture, sensor hub analytics, haptics, and logging.
3. **Fusion layer** that correlates raw channels into fit quality, skin risk, odor risk, and compliance signals.
4. **Session layer** that stores summaries, alarm counts, and calibration state for later review.
5. **Application protocol layer** that exposes snapshots, trends, alerts, and clinician annotations.

At the system boundary, the liner is treated like a semi-disposable sensing consumable, while the pod remains a reusable authenticated compute endpoint. That division matters both economically and clinically. It allows hygiene-sensitive pieces to be replaced cheaply while keeping radios, batteries, memory, and secure identity in the expensive portion. It also makes field service easier: the app can detect the liner profile at attachment time and automatically load the correct calibration set.

## Firmware details and design decisions

The included firmware in this repository is a compile-ready simulation-oriented reference implementation written in C. It mirrors how the production embedded code would be structured on target silicon while remaining easy to build on a desktop toolchain for algorithm development. The decision to provide a simulation build is important because SplintSense’s value comes from analytics as much as electronics. Teams iterating on pressure thresholds, moisture decay models, or activity compliance rules need rapid testing without waiting on a hardware bring-up loop.

The firmware samples multiple sensing domains and computes a **Recovery Stability Index (RSI)** plus several sub-scores:

- **Pressure balance score** compares local sustained loads and cross-region asymmetry.
- **Moisture burden score** integrates absolute wetness and duration above a risk threshold.
- **Odor trend score** tracks VOC rise rate instead of naive raw level to better separate transient spikes from worsening enclosure conditions.
- **Thermal comfort score** distinguishes expected warmth during motion from persistent stagnant heat.
- **Compliance score** uses inertial activity bursts and orientation drift to infer whether the splint is being used roughly within plan.

Risk rules are intentionally layered. A single wet reading does not trigger a serious alert. Instead, the firmware looks for duration, rate of change, and co-occurrence. For example, elevated moisture plus rising VOC and increased local heat is more concerning than any of those signals alone. Likewise, high pressure that moves around during active hours may represent normal gait, while the same pressure concentrated over six nighttime windows suggests a fit problem or liner fold.

Another major design choice is progressive notification. SplintSense avoids bombarding users with raw medical-style alarms. The pod first records an event, then provides a gentle haptic nudge if the condition persists. The app translates the event into plain-language guidance like “air out the brace if allowed” or “check for wrinkled liner at heel.” Only repeated or high-severity patterns are flagged for clinician review. This matters because adherence falls quickly when devices cry wolf.

Data handling is also conservative by design. The app receives concise summaries by default: latest risk state, battery, recent events, and six-hour trends. Detailed sensor history stays on device until the user or clinician requests export. This preserves privacy, reduces radio time, and keeps the user interface legible.

The driver organization in the firmware directory reflects production realities:

- `power.c` models charger state, brownout policy, and battery prediction.
- `pressure.c` handles zone filtering, dwell estimation, and asymmetry extraction.
- `moisture.c` manages strip calibration, hydration persistence, and drying curves.
- `sensor_hub.c` fuses humidity, VOC, temperature, and IMU context into recovery metrics.
- `ble.c` serializes a compact status packet and a clinician packet.
- `logger.c` stores event timelines, persistent summaries, and snapshots for export.
- `haptics.c` turns analytics into tactile patterns without over-notifying the user.

Because the device may be used in medically relevant contexts, traceability matters. Every event is timestamped relative to therapy session start, severity is recorded separately from user notification level, and the export protocol includes calibration metadata so remote review is not detached from device conditions. The firmware therefore maintains a history buffer, an alert event store, a symbolic register map for desktop testing, and a portable packet format that can be reused later in a true MCU build.

The included simulation intentionally introduces several realistic issues over time: mild humidity drift, occasional impact excursions, increasing VOC burden, fit degradation after prolonged use, and a one-time moisture spike that resembles accidental splash or heavy sweat loading. That gives engineers a fast regression corpus for validating thresholds and user-experience choices.

## Application and software interface

The `app/` directory contains a React Native style CommonJS application scaffold that emphasizes practical recovery workflows. It is organized into the following screens:

- **Home Screen:** current RSI, active alerts, brace fit score, battery, and daily moisture burden.
- **Trend Screen:** rolling charts for pressure asymmetry, moisture persistence, VOC trend, and impact events.
- **Alert Screen:** chronological event feed with severity tags and suggested next actions.
- **Device Screen:** device identity, liner profile, calibration age, battery health, and sync controls.
- **Clinician Screen:** export bundle preview, annotated incident cards, and follow-up prompts.
- **Settings Screen:** vibration strength, quiet hours, scan cadence, and profile mode.

The app protocol is intentionally compact. A quick snapshot packet contains firmware version, battery percent, last sync age, current RSI, peak pressure zone, moisture burden, VOC slope, and unresolved alert count. A richer clinician export packet adds trend buckets, per-zone summaries, calibration factors, brace configuration, and event history. This allows a sports medicine clinic to review whether an athlete repeatedly overloaded a boot or whether a post-op patient’s liner stayed damp overnight across several days.

Beyond the visible UI, the product concept assumes three software interaction layers:

1. **Patient layer** with plain-language guidance, quiet alerts, and confidence-building summaries.
2. **Caregiver layer** with setup help, liner replacement reminders, and exception notifications.
3. **Clinician layer** with exports, incident timelines, and trend interpretation aids.

That separation is important. Patients should not be forced to interpret raw capacitance data or microclimate jargon. Clinicians, on the other hand, need enough structured evidence to decide whether a fit issue is persistent, worsening, or resolved.

## Use cases and target audience

### 1. Post-operative ankle boot monitoring
A patient returns home after Achilles repair with a removable boot. SplintSense notices that heel pressure remains high for long nighttime periods and moisture burden rises sharply after evening activity. The app suggests checking sock smoothness and pad placement. Two days later, the pressure asymmetry trend improves after adjustment, likely preventing skin breakdown.

### 2. Pediatric wrist brace oversight
Parents of a child with a wrist fracture often cannot tell whether the brace got damp at school or shifted during play. SplintSense detects a sustained moisture event after recess and sends a low-friction alert. The child’s caregiver can intervene before odor and skin irritation build up.

### 3. Sports rehabilitation compliance
A physical therapist provides a SplintSense-equipped ankle support to an athlete transitioning from immobilization to controlled loading. The app shows impact counts and movement bursts exceeding the planned range, helping steer the athlete away from premature return-to-play behavior.

### 4. Rural follow-up and telehealth
Patients who live far from orthopedic clinics often delay check-ins until discomfort becomes severe. SplintSense provides trend summaries that can be reviewed remotely, allowing clinicians to prioritize in-person visits for cases showing repeated abnormal pressure, persistent moisture, or worsening odor signature.

### 5. Elder recovery support
Older adults with fragile skin are especially vulnerable to hidden pressure sores under braces and walkers. SplintSense’s low-power passive monitoring and simple haptic cues help caregivers identify issues without needing a full instrumented bed or continuous observation.

### Target audience

SplintSense is intended for orthopedic clinics and outpatient surgery centers, sports medicine and physical therapy practices, rehabilitation hospitals, pediatric fracture care programs, home health providers, durable medical equipment innovators, insurers and care organizations seeking reduced complication rates, and researchers studying recovery adherence and microclimate effects in immobilization.

The device is particularly compelling where the cost of one preventable complication vastly exceeds the cost of lightweight monitoring. Skin breakdown, unplanned follow-ups, liner replacement, emergency cast removal, or infection-related escalation all represent expensive failure modes that current dumb braces do little to prevent.

## Why this device should exist

Modern medicine has instrumented operating rooms, ICUs, and even inhalers, yet millions of people still go home in mechanically effective but informationally silent immobilization hardware. Recovery quality between appointments is largely inferred from patient memory and visible symptoms after the fact. SplintSense turns the splint itself into a quiet observer of fit, environment, and behavior. It does not attempt to replace clinician judgment. Instead, it gives clinicians earlier, better signals and gives patients practical feedback before discomfort becomes damage.

From an engineering perspective, the device is feasible now because the enabling pieces already exist: low-power BLE SoCs, printable flexible sensors, small fuel gauges, compact environmental sensors, and inexpensive mobile compute. What has been missing is a system-level product definition focused on orthopedic recovery rather than general fitness or one-dimensional wound monitoring. SplintSense fills that gap.

From a product strategy perspective, it also has a realistic deployment path. The reusable pod lowers recurring electronics cost, while disposable liners support hygiene and revenue. The same core electronics can serve multiple brace geometries. The app can begin as a patient-facing summary tool and later expand into EHR-connected clinician workflows. The simulation firmware in this repository gives engineering teams a head start on testing risk logic before the first flex PCB spin.

In short, SplintSense is a believable next-generation recovery platform: original enough to be interesting, focused enough to be buildable, and practical enough that patients, caregivers, and clinicians would genuinely benefit from it.
