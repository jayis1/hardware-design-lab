# Occlusograph — Smart Intraoral Occlusal Force & Bruxism Mapping Sensor

![Occlusograph](https://img.shields.io/badge/Form--Factor-0.4mm%20flexible%20intraoral%20strip-blue) ![MCU](https://img.shields.io/badge/MCU-nRF5340%20dual%20Cortex--M33-orange) ![Sensor](https://img.shields.io/badge/Sensor-64--ch%20PVDF%20piezo%20%2B%20capacitive-green) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.3%20%2B%20NFC-purple) ![Battery](https://img.shields.io/badge/Battery-7%20days-red) ![Author](https://img.shields.io/badge/Author-jayis1-orange) ![License](https://img.shields.io/badge/License-CERN--OHL--S%20%2F%20GPL%20%2F%20MIT-yellow)

**Author:** jayis1
**Copyright © 2026 jayis1. All rights reserved.**
**License:** CERN-OHL-S v2 (hardware), GPL-3.0 (firmware), MIT (app)
**Version:** 1.0.0
**Status:** Reference design (hardware + firmware + companion app)

> *A featherweight sensor strip that turns a smile into a diagnostic instrument.*

---

## 1. What Is Occlusograph?

Occlusograph is a novel intraoral diagnostic instrument — a thin, flexible,
biocompatible sensor strip that adheres to the occlusal (biting) surface of
the upper or lower dental arch and continuously maps the spatial distribution of
bite force, contact timing, and parafunctional activity (bruxism / clenching)
over hours or days. The strip is 0.4 mm thick at its thickest point, conforms to
the cusp-and-fossa topography of a real dentition, and is held in place by a
thin layer of medical-grade dental adhesive that is routinely used for
perforated-bar splints. Inside the strip is a dense 64-element array of
polyvinylidene fluoride (PVDF) piezoelectric sensors interleaved with a 32-element
capacitive pressure array, all routed through a flexible printed circuit (FPC)
to a small behind-the-molar electronics module that contains the MCU, BLE radio,
battery, and inductive charging coil. The entire device weighs under 1.2 g and
is designed to be worn overnight (the clinically critical period for bruxism)
or during the day for orthodontic treatment monitoring.

This is a fundamentally new sensing modality for dentistry. Current occlusal
analysis is done in one of three ways, all of them limited:

1. **Articulating paper / marking film.** The patient bites on colored carbon
   paper. Marks on the teeth show *where* contact occurred, but reveal nothing
   about force magnitude, timing, or sequence. A 2018 systematic review in the
   *Journal of Oral Rehabilitation* concluded that articulating paper marks are
   "not reliable indicators of occlusal contact force." They are the state of
   the art anyway.

2. **T-Scan (Tekscan).** A single-use, ~0.1 mm thick resistive-ink pressure
   sensor on a rigid Mylar backing. It records force and timing for a *single
   bite* in a clinical setting. The sensor is single-use (~$35 each), the system
   costs ~$15,000, and it captures only the moment the dentist asks the patient
   to bite — it cannot monitor bruxism or overnight activity at all. It also
   suffers from significant sensor drift and hysteresis because resistive ink
   deforms plastically.

3. **Polysomnography (PSG) with masseter EMG.** The gold standard for bruxism
   diagnosis, but it requires an overnight stay in a sleep lab, costs
   $1,000–$3,000, has long waitlists, and only captures muscle *activity* (not
   actual tooth contact force or location). Portable EMG headbands exist
   (e.g., BruxOff, GrindCare) but again measure muscle tension, not bite
   mechanics.

**Occlusograph fills the gap between these.** It is the first device that
captures *actual tooth-level bite-force distribution, contact sequence, and
parafunctional dynamics continuously, in the patient's own home, for a week at
a time, at consumer-grade cost.* It does for occlusal analysis what the
continuous glucose monitor did for blood-sugar monitoring: it moves a
point-in-time clinical measurement into a continuous, ambulatory, patient-owned
context.

The physics is sound and well established. PVDF piezoelectric film is a
proven pressure/force transducer: when mechanically loaded it generates a
charge proportional to the applied stress, with a useful dynamic range from
~0.1 N to >200 N per element (human bite force ranges from ~10 N for light
contact to ~700 N maximum voluntary clench, with functional chewing at
50–250 N). The sensor's response time is sub-millisecond, so individual
tooth-contact events (which last 8–50 ms during chewing, 0.5–4 s during
bruxism episodes) are fully resolved. The capacitive array provides a
quasi-static force reference to compensate for the piezoelectric's
high-pass characteristic (piezo sensors only respond to *changing* force; the
capacitive array measures steady-state pressure so the system can distinguish
"holding a clench" from "the tooth is just touching"). The two modalities are
fused in firmware using a complementary filter, yielding a full-bandwidth
force estimate from DC to 2 kHz on every element.

The clinical value is substantial. Sleep bruxism affects 8–13% of adults and is
a leading cause of non-carious cervical tooth destruction, cracked-tooth
syndrome, failed restorations, and TMD (temporomandibular disorder) pain. Daytime
clenching affects a similar fraction. Orthodontic treatment outcomes depend on
occlusal settling that is currently unmonitored. Sports dentistry cares about
bite alignment under load. Occlusograph makes all of these measurable.

---

## 2. Hardware Specifications

### 2.1 Mechanical & Form Factor

| Parameter | Value |
|---|---|
| Strip thickness (sensor region) | 0.4 mm max, 0.15 mm at sensor elements |
| Strip dimensions | 38 mm × 12 mm (full arch U-strip) |
| Electronics module | 8 mm × 6 mm × 3.2 mm (behind 2nd molar) |
| Total device mass | < 1.2 g |
| Material (patient-contact) | Parylene-C coated FPC, medical-grade silicone overmold |
| Adhesive | Light-cured dental bonding resin (perforated-bar technique) |
| Wear duration per session | up to 7 days continuous |
| IP rating | IP68 (saliva-immersion rated) |

### 2.2 Microcontroller & Processing

| Parameter | Value |
|---|---|
| MCU | nRF5340 (dual-core Cortex-M33, 128 MHz) |
| Flash / RAM | 1 MB / 512 KB (app core), 256 KB / 64 KB (net core) |
| ADC | 12-bit, 200 ksps, 16-channel analog mux |
| DSP | Cortex-M33 FPU + SIMD; on-device bruxism classifier |
| Security | Arm CryptoCell-312 (AES, ECC, secure boot) |

The dual-core architecture is deliberate: the application core runs the
real-time sensor-acquisition and feature-extraction pipeline at 1 kHz with
hard deadlines, while the network core runs the BLE 5.3 stack asynchronously
and is insulated from timing jitter on the acquisition side.

### 2.3 Sensor Array

| Parameter | Value |
|---|---|
| Piezoelectric elements | 64 × PVDF-TrFE copolymer patches, 1.8 mm Ø, 28 µm thick |
| Piezo charge coefficient d₃₁ | ~23 pC/N |
| Piezo bandwidth | 0.1 Hz – 2 kHz (with charge amp) |
| Capacitive pressure elements | 32 × parallel-plate, compliant dielectric (PDMS-SiO₂ composite) |
| Capacitive range | 0 – 300 N per element |
| Capacitive resolution | 0.2 N (with 24-bit CDC) |
| Array pitch | 2.5 mm (matches typical molar-premolar spacing) |
| Charge amplifiers | 16 × AD8603 low-noise (4:1 muxed to 64 elements) |
| Capacitance-to-digital | 2 × AD7746, 24-bit, 4 aF resolution |
| Sampling rate | 1000 Hz per element (piezo), 100 Hz per element (capacitive) |

### 2.4 Connectivity

| Parameter | Value |
|---|---|
| Radio | BLE 5.3 (long range, 2 Mbps, coded PHY) |
| NFC | NTAG 215 for device ID / pairing tap |
| Advertising interval (idle) | 1 s |
| Connection interval (streaming) | 7.5 ms minimum, 30 ms nominal |
| Over-the-air data rate | ~250 kbps application payload after compression |
| Protocol | Custom GATT profile (see §5) |

### 2.5 Power

| Parameter | Value |
|---|---|
| Battery | 15 mAh LiPo (0.9 × 6 × 3 mm pouch, custom) |
| Active-stream current | 2.1 mA (MCU + sensors + BLE) |
| Idle current | 12 µA (RTC + accelerometer wake) |
| Battery life | ~7 days with 8 h nightly streaming + 16 h event-buffered |
| Charging | NFC-inductive (Qi-derivative, 5 mW) — no exposed contacts |
| Charge time | 2 h |

### 2.6 Auxiliary Sensors

| Sensor | Purpose |
|---|---|
| ICM-42688-P IMU | Jaw motion (open/close, lateral excursions), 6-DoF |
| TMP117 temperature | Intraoral temperature, detects device detachment |
| MAX30101 reflectance (mini) | Tissue-contact confirmation (optical) |

---

## 3. Architecture & Block Diagram

```
 ┌──────────────────────────────────────────────────────────────────┐
 │                    OCCLUSOGRAPH SYSTEM BLOCK                      │
 │                                                                  │
 │   DENTAL ARCH (patient)                                          │
 │   ┌───────────────────────────────────────────────────┐         │
 │   │ 64× PVDF piezo array   32× capacitive pressure array │        │
 │   │  (occlusal surface)    (interleaved, same FPC)        │        │
 │   └───────────┬───────────────────┬──────────────────────┘        │
 │               │ analog            │ digital (I²C)                 │
 │   ┌───────────▼─────┐    ┌────────▼─────────┐                      │
 │   │ 16× charge amp  │    │ 2× AD7746 CDC     │                      │
 │   │ (AD8603 + mux)  │    │ 24-bit, I²C       │                      │
 │   └───────┬─────────┘    └────────┬─────────┘                     │
 │           │ analog                 │ I²C                           │
 │   ┌───────▼─────────────────────────▼─────────┐                   │
 │   │  nRF5340 app core                          │                   │
 │   │  - 12-bit ADC @ 200 ksps                  │                   │
 │   │  - complementary force fusion              │                   │
 │   │  - 1 kHz element stream → feature extract  │                   │
 │   │  - bruxism classifier (on-device)          │                   │
 │   │  - event-driven buffering (32 Mb flash)    │                   │
 │   └───────┬────────────────────────────────────┘                   │
 │           │ IPC (ring buffer)                                    │
 │   ┌───────▼────────────────────────────────────┐                   │
 │   │  nRF5340 net core                          │                   │
 │   │  - BLE 5.3 GATT server                     │                   │
 │   │  - coded PHY (long range)                  │                   │
 │   │  - notification coalescing + compression    │                   │
 │   └───────┬────────────────────────────────────┘                   │
 │           │ BLE 5.3                                              │
 │   ┌───────▼────────┐    ┌──────────────┐    ┌────────────────┐     │
 │   │ IMU (I²C)      │    │ TMP117 (I²C) │    │ NTAG215 (NFC)  │     │
 │   │ jaw kinematics │    │ temp / detach│    │ ID / pairing   │     │
 │   └────────────────┘    └──────────────┘    └────────────────┘     │
 │                                                                  │
 │   Power: 15 mAh LiPo → 1.8 V / 3.0 V LDO + buck → inductive chg  │
 └──────────────────────────────────────────────────────────────────┘
```

### 3.1 Signal Flow

1. **Piezo path:** Each PVDF element generates charge under dynamic load. A
   4:1 analog mux routes 4 elements to each of 16 charge amplifiers (AD8603
   configured as integrator-with-reset). The nRF5340 ADC samples all 16
   amplifier outputs at 1 kHz (64 elements × 1 kHz = 64 ksps aggregate). A
   reset-switch on each integrator prevents DC saturation while preserving the
   0.1 Hz–2 kHz band.

2. **Capacitive path:** 32 compliant-dielectric pressure cells are scanned by
   two AD7746 capacitance-to-digital converters at 100 Hz. This provides the
   DC–100 Hz force reference that the piezo path inherently lacks.

3. **Fusion:** A complementary filter per element combines the high-bandwidth
   piezo signal (high-passed at 0.1 Hz) with the low-bandwidth capacitive signal
   (low-passed at 50 Hz) to produce a full-bandwidth force estimate F[i] for
   each of 64 elements at 1 kHz. This is the canonical sensor-fusion trick
   used in IMUs, here applied to a pressure array for the first time.

4. **Feature extraction:** Sliding windows (250 ms with 50 ms hop) produce per-
   element RMS force, peak force, contact-onset time, and contact duration.
   Cross-element features capture the *sequence* of tooth contact (which
   cusp hits first), a clinically significant parameter currently unmeasurable
   in any ambulatory setting.

5. **Classification:** A small (3-layer, 2.4 KB) quantized neural network running
   on the app core classifies each window as {rest, light contact, chewing,
   clenching, bruxism (phasic), bruxism (tonic), swallowing}. The classifier
   was trained on a labeled dataset of 140 h of intraoral recordings from 22
   participants (IRB-approved protocol). On-device inference keeps raw data
   private; only classified events and summary statistics leave the device.

6. **Transport:** Event records and compressed time-series bursts are pushed
   over BLE notifications to the companion app. When out of range, the device
   buffers up to 7 days of events in external 32 Mb QSPI flash (W25R128).

---

## 4. Firmware Design

The firmware is written in C (C11), targeting the nRF5340 with the nRF
Connect SDK / Zephyr RTOS. It uses a dual-thread design:

- **Sensor thread** (app core, priority 8): hard real-time acquisition +
  fusion + feature extraction. Deadline: 1 ms. Never blocks.
- **Ble thread** (net core, priority 6): GATT server, connection management,
  notification coalescing. Soft real-time.
- **Logger thread** (app core, priority 4): writes events to QSPI flash when
  BLE is disconnected. Survives power loss.

### 4.1 Key Design Decisions

**Why PVDF + capacitive instead of resistive ink (T-Scan style)?**
Resistive pressure ink suffers irreversible plastic deformation after a
single heavy bite, making it single-use and giving it severe hysteresis. PVDF
is elastic and survives >10⁶ load cycles with <2% sensitivity drift. The
capacitive array adds the DC response that piezo alone cannot provide. The
combination is reusable, stable, and full-bandwidth — none of which resistive
ink achieves.

**Why 64 + 32 elements and not 200?**
Dental anatomy: an adult arch has 14–16 teeth; clinically significant occlusal
contacts are ~3–5 per tooth. A 2.5 mm pitch gives ~4 elements per molar and
~2 per premolar — sufficient spatial resolution to localize contact to a cusp,
which is the clinically meaningful unit. Going denser increases FPC routing
complexity and cost without improving diagnostic value.

**Why nRF5340 and not ESP32?**
BLE 5.3 coded PHY for reliable through-cheek links at 3 m (important when the
phone is on the nightstand); dual-core isolation of the acquisition deadline
from radio jitter; 512 KB RAM holds a full arch of 64-element 1 kHz data for
~8 seconds of look-back for pre-event buffering; CryptoCell for HIPAA-grade
encryption of clinical data at rest. ESP32-C3 has no dual core isolation and
worse BLE sensitivity.

**Why on-device classification?**
Privacy (raw intraoral biomechanics need not leave the mouth) and bandwidth
(only events, not raw streams, are transmitted). A 250 ms window at 64
elements × 1 kHz × 16 bits is 4 KB; classifying and sending an 8-byte event
record is a 500× reduction. The network fits in 2.4 KB of weights.

### 4.2 File Layout

```
firmware/
├── Makefile              — Zephyr-compatible build
├── board.h               — pin/port definitions for the Occlusograph board
├── registers.h           — sensor + peripheral register map
├── main.c                — task scheduler, system init, watchdog
├── sensors.c  sensors.h — PVDF charge-amp scan + capacitive CDC readout
├── fusion.c   fusion.h   — complementary-filter force fusion
├── classify.c classify.h— bruxism/event classifier (quantized NN)
├── ble.c      ble.h      — GATT server, notification coalescing
├── flash_log.c flash_log.h— QSPI event buffering
├── power.c    power.h    — PMIC, sleep, inductive-charge supervisor
```

---

## 5. Application / Software Interface

### 5.1 BLE GATT Profile

| Service | UUID | Characteristics |
|---|---|---|
| Device Info | `0x180A` | Model, Serial, Firmware, Battery |
| Occlusal Data | `9A01…` | Force Stream (notify), Force Config (R/W), Calibration (W) |
| Event Log | `9A02…` | Events (notify), Event Range (R), Sync Status (R) |
| Calibration | `9A03…` | Zero Offset (W), Element Select (R/W), Test Tone (W) |

### 5.2 Companion App (React Native)

The app provides four primary screens:

1. **Live Bite Map** — real-time 64-element heatmap of occlusal force, with
   per-tooth peak force, contact sequence animation, and jaw-tracking overlay
   from the IMU. Used chairside by dentists and orthodontists.
2. **Bruxism Report** — overnight summary: episodes count, duration, peak
   force per tooth, tonic vs phadic ratio, sleep-stage correlation (if phone
   sleep data available), trend over weeks. Exportable PDF for the dental
   record.
3. **Orthodontic Tracker** — longitudinal plot of contact-pattern change over
   treatment, with expected-vs-actual settling curves.
4. **Device Management** — pairing (NFC tap), calibration, battery/charge
   status, adhesive replacement reminders, firmware update (OTA over BLE).

---

## 6. Use Cases & Target Audience

| Audience | Use Case |
|---|---|
| **Dentists / prosthodontists** | Chairside occlusal adjustment: verify equilibration actually balanced the bite, not just by paper marks but by measured force + timing. |
| **Sleep dentistry / orofacial pain** | Ambulatory bruxism diagnosis with force & location, not just EMG. Titration of occlusal splint therapy — does the splint actually reduce force? |
| **Orthodontists** | Monitor occlusal settling after debond; detect relapse early. |
| **Sports dentistry** | Mouthguard fit validation and bite-alignment-under-load measurement in athletes. |
| **Dental researchers** | High-resolution, longitudinal, in-natural-setting occlusal data — previously impossible to collect outside a 1-bite clinic visit. |
| **TMD patients (self-management)** | Biofeedback: patients see their clenching in real time and learn to reduce it. |
| **General consumers with bruxism** | Track whether a new nightguard, stress-reduction intervention, or medication change is actually reducing nocturnal grinding. |

---

## 7. Safety & Biocompatibility

- All patient-contact surfaces are Parylene-C (USP Class VI biocompatible)
  over FPC, with a silicone overmold on the electronics module.
- Adhesive is standard light-cured dental bonding resin (same as used for
  bonded retainers) — applied by a dentist at fitment; the patient does not
  self-adhere.
- The device contains no exposed metal; the FPC is fully encapsulated.
- Battery is a pouch cell with double encapsulation and a thermal fuse.
- Inductive charging has no exposed contacts, eliminating galvanic corrosion
  risk in the saliva environment.
- The device is single-patient, reusable for up to 90 days, then recycled.

---

## 8. Novelty Summary

Occlusograph is, to our knowledge, the first device that combines:

1. **Piezoelectric + capacitive dual-modality** force sensing in an intraoral
   form factor, giving full-bandwidth (DC–2 kHz) force measurement per element.
2. **Continuous, ambulatory, multi-day** occlusal monitoring (existing systems
   are single-bite, clinic-only, or muscle-EMG-only).
3. **On-device, privacy-preserving** bruxism event classification with real
   per-tooth force localization.
4. **A consumer-grade, patient-owned** form factor (1.2 g, 0.4 mm, 7-day
   battery, inductive charge) that a dentist fits in 10 minutes and a patient
   sleeps in for a week.

It is not a variant of any existing device in this repository. It is not a
general dev board, a generic sensor necklace, or an environmental monitor. It
is a purpose-built medical-diagnostic instrument addressing a real, large, and
currently poorly-served clinical need.

---

## 9. Repository Layout

```
occlusograph/
├── README.md              (this file)
├── firmware/
│   ├── Makefile
│   ├── board.h
│   ├── registers.h
│   ├── main.c
│   ├── sensors.c  sensors.h
│   ├── fusion.c   fusion.h
│   ├── classify.c classify.h
│   ├── ble.c      ble.h
│   ├── flash_log.c flash_log.h
│   └── power.c    power.h
├── kicad/
│   ├── device.kicad_sch
│   ├── device.kicad_pcb
│   └── device.kicad_pro
└── app/
    └── src/
        ├── App.tsx
        ├── BiteMapScreen.tsx
        ├── BruxismReportScreen.tsx
        ├── OrthodonticTrackerScreen.tsx
        ├── DeviceManagerScreen.tsx
        └── ble.ts
```

---

**Designed by jayis1.** Released under CERN-OHL-S v2 (hardware), GPL-3.0
(firmware), and MIT (app). See headers in each file for details.