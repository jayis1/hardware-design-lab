# HiveVox — Acoustic Beehive Health Monitor

**Author:** jayis1
**Copyright:** © 2026 jayis1
**License:** MIT
**Version:** 1.0.0

---

## 1. Purpose and Overview

**HiveVox** is an intelligent beehive health monitor that combines broadband acoustic
analysis, multi-zone thermometry, humidity sensing, and a precision load cell to give
beekeepers a continuous, non-invasive picture of colony vitality. Instead of opening the
hive (which stresses bees and disrupts brood temperature), HiveVox listens to the colony
and weighs it.

Honeybee colonies produce characteristic sounds whose spectra shift with colony state.
A queenright colony with healthy brood maintains a steady "fanning" hum around
250–300 Hz. A queenless colony develops a distinctive "queenless roar" — a broadband
shift upward above 400 Hz. Swarming preparation ("queen cells capped") produces a
specific pulsing tone around 110 Hz. Winter cluster formation changes the dominant
frequency as bees tighten together, and a silent hive is a dead or dying hive.

HiveVox samples these sounds with a low-noise MEMS microphone, computes a 512-point FFT
on an ARM Cortex-M4 every 2 seconds, classifies the colony state with a lightweight
on-device classifier, and reports via LoRaWAN to a gateway up to 5 km away — no cell
service or Wi-Fi required in the apiary. A 20 kg-capacity load cell under the hive
tracks honey weight gain through the season; sudden weight drops signal robbing,
swarming departure, or hive falling off its stand.

The companion mobile app shows per-hive state history, alerts on anomalies (queen
loss detected, swarming imminent, weight anomaly), and aggregates an apiary-wide
health dashboard. HiveVox is designed for commercial pollinators and serious
hobbyists managing 2–200 hives.

### Why this doesn't exist yet

Existing hive monitors (Broodminder, Arnia, Hivetool) measure temperature and weight.
Some measure sound but only as a waveform recording sent to the cloud for offline ML.
None run an on-device acoustic classifier over LoRaWAN with a multi-zone thermal array
and load cell in a single solar-powered, weatherproof (IP65) package under $60 BOM.
HiveVox closes that gap.

---

## 2. Hardware Specifications

### MCU
- **STMicroelectronics STM32L432KC** — ARM Cortex-M4F, 80 MHz, 256 KB flash, 64 KB SRAM
- Chosen for low-power (1.2 V core, 1.65 µA STOP2 mode), hardware FPU for FFT math,
  AES-128 accelerator for LoRaWAN, and UFQFPN28 package (7×7 mm) for compact layout.

### Sensors
| Function | Part | Interface | Notes |
|---|---|---|---|
| Acoustic | **Knowles SPH0645LU4H-1** MEMS mic | I²S (PDM) | 65 dB SNR, flat to 80 kHz |
| Temp (brood zone) | **DS18B20** ×3 (top/mid/bottom) | 1-Wire | ±0.5 °C, waterproof stainless probe |
| Temp/Humidity (entrance) | **SHT45** | I²C | ±0.1 °C, ±1.5 %RH |
| Weight | **HX711** + 20 kg load cell | GPIO bit-bang | 24-bit ADC, 80 Hz |
| Light (entrance activity) | **VEML7700** | I²C | For entrance flight detection |
| Battery | Internal divider | ADC | LiFePO4 18650, 3.2 V nominal |

### Connectivity
- **RFM95W** (SX1276) LoRa transceiver, 915 MHz (US) / 868 MHz (EU) via 0Ω straps
- Spread factor SF7–SF12 adaptive, +20 dBm PA boost
- AES-128 encrypted LoRaWAN 1.0.4 join (OTAA)

### Power
- **LiFePO4 18650** (1500 mAh, 3.2 V) — safe chemistry, no BMS fire risk in sun
- **6 V 1 W monocrystalline solar panel** + **MCP73871** MPPT solar charger
- Average consumption: 180 µA (LoRa tx 40 mA × 50 ms / 10 min + MCU RUN 8 mA × 200 ms / 10 min)
- 14-day dark runtime from full battery (no sun)

### Form Factor
- **114 × 70 × 22 mm** sealed ABS enclosure (IP65)
- Mounts under outer cover, centered on the hive top
- Three DS18B20 probes on 30 cm leads drop into the brood box via notched inner cover
- Load cell mounts between hive bottom board and stand (external, wired)

### Block Diagram

```
                       ┌────────────────────────────────────────────┐
                       │              STM32L432KC MCU                 │
                       │  Cortex-M4F 80MHz · 256KB Flash · 64KB RAM  │
                       │  AES-128 · RTC · DMA · HW FPU               │
                       └─────┬───────┬───────┬───────┬───────┬────────┘
                             │       │       │       │       │
            ┌────────────────┤  I²S  │  I²C  │ 1-Wire│ GPIO  │
            │                │       │       │       │       │
   ┌────────▼─────┐  ┌───────▼──┐ ┌──▼──┐ ┌──▼──┐ ┌──▼──┐ ┌──▼─────┐
   │SPH0645 MEMS  │  │ SHT45    │ │VEML │ │DS18B│ │HX711│ │ RFM95W │
   │  Mic (PDM)   │  │T/H entry │ │7700 │ │  ×3 │ │Load │ │ LoRa   │
   └──────────────┘  └──────────┘ └─────┘ └─────┘ └─────┘ └────────┘
                                                                  │
                                                         915/868 MHz │
                                                              Antenna │
   ┌──────────┐    ┌────────────┐         ┌──────────────────┐    │
   │6V Solar  │────│MCP73871    │─────────│ LiFePO4 18650    │    ▼
   │  Panel   │    │  MPPT      │   VBAT  │  1500mAh Battery │  [Gateway]
   └──────────┘    └────────────┘         └──────────────────┘
```

---

## 3. Firmware Architecture

The firmware is a bare-metal super-loop design (no RTOS — 64 KB SRAM is tight and the
task graph is simple). It uses the CMSIS DSP library for the 512-point radix-2 FFT.

### Main loop
1. **RTC wakes MCU** every 10 minutes (configurable 2–60 min).
2. **Acoustic sample**: enable MEMS mic, DMA-capture 512 samples at 16 kHz (32 ms).
3. **FFT + classify**: compute magnitude spectrum, extract features (dominant freq,
   spectral centroid, band energies in 5 bands), run `colony_classify()`.
4. **Environmental read**: DS18B20 ×3, SHT45, VEML7700, HX711 weight.
5. **Packet build & LoRa TX**: assemble binary LoRaWAN uplink, transmit, then sleep.
6. **Deep sleep**: all peripherals off, RTC alarm set, STOP2 mode (~2 µA).

### Acoustic classifier
`colony_classify()` is a hand-tuned decision tree (not ML — too costly to train and
deploy over the air, and a hand classifier is auditable and fits in <2 KB flash).
It uses the dominant frequency `f0`, the ratio of energy above 400 Hz to total
energy `r_hi`, and the coefficient of variation of frame-level loudness `cv_loud`:

- **Healthy queenright**: 240 ≤ f0 ≤ 320 Hz, r_hi < 0.20, cv_loud < 0.15 → `STATE_QUEENRIGHT`
- **Queenless**: f0 > 380 Hz, r_hi > 0.35 → `STATE_QUEENLESS` (alert)
- **Swarming prep**: 95 ≤ f0 ≤ 125 Hz, pulsing (cv_loud > 0.25) → `STATE_SWARM_PREP` (alert)
- **Winter cluster**: f0 60–90 Hz, low total energy, temp < 10 °C → `STATE_WINTER`
- **Dead/empty**: total energy < threshold for >30 min → `STATE_DEAD` (alert)
- **Unknown**: fallback → `STATE_UNKNOWN`

### Files
- `firmware/main.c` — super-loop, RTC wake, state machine
- `firmware/registers.h` — peripheral register map (bare-metal, no HAL)
- `firmware/board.h` — pin map and board constants
- `firmware/drivers/` — I²C, 1-Wire, HX711, I²S/PDM, LoRa SX1276, FFT (CMSIS-DSP glue)
- `firmware/Makefile` — arm-none-eabi-gcc toolchain build

### Design decisions
- **Bare metal over HAL**: ST's HAL is ~40 KB; we have 256 KB but prefer the lean path
  for auditability and to leave room for the FFT buffer. Direct register access also
  makes power transitions explicit.
- **PDM over I²S not ADC**: an ADC microphone front-end would need an op-amp and would
  pick up the HX711 and LoRa switching noise. The digital MEMS mic is immune.
- **LiFePO4 not Li-ion**: beehives sit in full sun and get hot. Li-ion is a fire risk;
  LiFePO4 is thermally stable to 270 °C and has a flat discharge curve.
- **Decision tree not NN**: on-device neural nets are hard to verify, train, and
  update over LoRa. A decision tree on hand-crafted features is auditable, tiny, and
  deterministic — critical when the alert says "your queen is dead."

---

## 4. Application / Software Interface

### LoRaWAN uplink payload (12 bytes, little-endian)
| Offset | Size | Field |
|---|---|---|
| 0 | 1 | Colony state enum (0–6) |
| 1 | 2 | Dominant frequency (Hz / 2, uint16) |
| 3 | 1 | r_hi × 255 (uint8) |
| 4 | 1 | cv_loud × 255 (uint8) |
| 5 | 2 | Brood temp mid (°C × 100, int16) |
| 7 | 1 | Entrance humidity (%RH, uint8) |
| 8 | 2 | Weight (kg × 100, uint16) |
| 10 | 1 | Battery (mV / 20, uint8) |
| 11 | 1 | Flags (bit0: alert, bit1: solar good, bit2: probe fault) |

### Downlink (4 bytes, optional)
| Offset | Size | Field |
|---|---|---|
| 0 | 1 | New sample interval (minutes) |
| 1 | 1 | New LoRa SF (7–12) |
| 2 | 2 | Reserved |

### Companion app
React Native (Expo) app. Screens:
- **ApiaryDashboard** — grid of hive cards, color-coded state, last-seen, weight trend
- **HiveDetail** — 7-day charts of state, temp, weight; acoustic event log
- **Alerts** — list of anomalies (queenless, swarm prep, weight drop, low battery)
- **Settings** — add hive (DevEUI scan QR), rename, set sample interval, SF override

The app decodes the LoRaWAN payload via TTI/TTN webhook (HTTP integration) and stores
to a local SQLite + cloud sync. No proprietary backend required — works with The Things
Network community servers.

---

## 5. Use Cases and Target Audience

### Commercial pollinators (50–500 hives)
Pollination contracts pay per healthy hive. HiveVox detects queen loss within
hours instead of the next monthly inspection visit, letting the operator re-queen
before the colony collapses and the contract payment is lost. One saved colony
($180 queen + labor) pays for two HiveVox units.

### Serious hobbyists / small apiaries (2–20 hives)
Weekend beekeepers cannot inspect midweek. HiveVox's "swarm prep" alert gives
~48 hours notice to act (add supers, split, or catch the swarm) before half the
colony leaves. Preventing one swarm saves a season's honey crop from that hive.

### Research / citizen science
University entomology labs use HiveVox's timestamped acoustic + thermal data to
study colony phenology, Varroa-related frequency shifts, and regional overwintering
survival — all from a standardized, open-format sensor.

### Disaster / post-event assessment
After a pesticide spray event or wildfire smoke, HiveVox's "dead/empty" or
"queenless" alerts pinpoint which hives survived before the beekeeper returns to
the apiary, saving triage time.

### Who it is NOT for
Single-hive casual owners who inspect weekly and don't want alerts. The value is
in scale and absence — managing hives you can't see every day.

---

## 6. Bill of Materials (estimated, 1k qty)

| Part | Qty | Unit $ | Subtotal |
|---|---|---|---|
| STM32L432KC | 1 | 4.20 | 4.20 |
| SPH0645LU4H-1 | 1 | 1.80 | 1.80 |
| RFM95W | 1 | 6.50 | 6.50 |
| DS18B20 | 3 | 1.10 | 3.30 |
| SHT45 | 1 | 2.40 | 2.40 |
| VEML7700 | 1 | 1.20 | 1.20 |
| HX711 | 1 | 0.90 | 0.90 |
| Load cell 20kg | 1 | 3.00 | 3.00 |
| MCP73871 | 1 | 1.60 | 1.60 |
| LiFePO4 18650 | 1 | 3.50 | 3.50 |
| Solar panel 6V 1W | 1 | 1.40 | 1.40 |
| PCB + enclosure + misc | 1 | 12.00 | 12.00 |
| **Total** | | | **$41.80** |

Target retail $79 (assembled) or $59 (kit with separate load cell).

---

## 7. Mechanical and Environmental

- **Enclosure**: UV-stabilized ABS, IP65 (gasketed screw lid), white to reflect sun.
- **Mounting**: stainless strap to hive, 3 DS18B20 probes drop through inner cover.
- **Load cell**: separate IP67 aluminum load cell between bottom board and stand,
  4-wire to HiveVox via weatherproof RJ9 connector.
- **Operating temp**: −20 °C to +55 °C (matches beekeeping climate range).
- **Solar**: panel faces south on hive lid at apiary latitude tilt.

---

## 8. Calibration and Deployment

1. **Load cell**: zero with empty hive bottom board (tare command via app downlink).
2. **Mic**: factory-calibrated; no field calibration needed (frequency features only).
3. **Temp probes**: DS18B20 factory ±0.5 °C; re-verify annually against ice bath.
4. **Join**: scan QR on enclosure with app → DevEUI/AppKey auto-provisioned to TTN.
5. **First 24 h**: app shows "baseline" while classifier collects reference frames.

---

## 9. Roadmap

- **v1.0** (this release): acoustic + thermal + weight, LoRaWAN, decision-tree classifier.
- **v1.1**: OTA firmware update via LoRa FUOTA (fragmented, ECC-256 signed).
- **v2.0**: second mic for entrance flight-count (wingbeat discrimination, bee vs wasp).
- **v2.1**: Varroa mite drop detection via accelerometer on the bottom board.

---

## 10. License and Credit

Firmware, schematics, PCB, and app © 2026 **jayis1**, released under the MIT License.
See `LICENSE` in each subdirectory. The CMSIS-DSP library (ARM) retains its Apache-2.0
license; LoRaWAN stack is Semtech's reference, BSD-3.

**Author:** jayis1