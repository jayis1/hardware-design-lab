# Pantry Warden

**Author:** jayis1  
**Copyright:** (C) 2026 jayis1. All rights reserved.

## Device purpose and overview

Pantry Warden is a shelf-edge food protection instrument for homes, food banks, school kitchens, apartment pantries, sailboats, RV galleys, farm stands, and preparedness lockers. It solves a problem that most people only notice after food is already wasted: storage spaces fail slowly and invisibly. A pantry can look normal from the outside while one cereal box is taking on humidity, rice is absorbing odors, flour moths are starting to move, a protein bar case is overheating against a poorly vented wall, or a sealed pouch is slowly swelling from hidden spoilage. Existing smart-home sensors mostly watch rooms, not shelves. Pantry Warden watches the shelf itself.

The device mounts as a slim bar under the front lip of a pantry shelf or on the side wall of a cabinet. It uses a small controllable airflow path, gas sensing, weight trend sensing, distance measurement, optical freshness observation, and acoustic pest detection to build a live model of what stored food is experiencing. Instead of only reporting temperature and humidity, Pantry Warden interprets shelf behavior. It can tell the difference between a restock event, a stale-air pocket, a likely pest nibble, a package bulge, condensation forming behind canned goods, and an odor plume from a leaking onion, fermenting produce basket, or ruptured dry-goods bag. It exists for the period before spoilage is obvious but after the environment has already started drifting into failure.

What makes Pantry Warden novel is its focus on the microclimate and physical state of stored goods. Pantry safety today is pieced together from separate, limited tools: passive traps for pests, room hygrometers, visual inspection, and inventory apps that know what you bought but not whether the shelf is still healthy. Pantry Warden combines those layers into one shelf intelligence node. It does not require the user to put electronics inside every jar or package. It instruments the shelf boundary and infers the condition of the goods from signatures that are practical to measure at low cost.

A single household might install one unit in a baking goods shelf, one near pet food, and one in a root-cellar cabinet. A community kitchen could deploy a mesh of units to watch donated dry goods and bulk bins. A boat owner could place it in a humid galley locker where corrosion, mildew, and food degradation happen faster. The device is especially useful for users who store grains, legumes, powders, teas, dehydrated foods, emergency rations, spices, nuts, and sealed pouches that are vulnerable to moisture ingress, odor contamination, or infestation.

Pantry Warden turns invisible shelf drift into actionable maintenance. The app does not merely say that humidity is high. It can say: rear-left shelf zone is retaining moisture behind stacked cans; rotate items and increase airflow. It can say: acoustic chew activity plus night motion suggests pantry moth larvae near flour. It can say: rapid package bulge with VOC rise suggests one suspect item, likely near the front right cluster. That actionability is the reason the product should exist.

## Why this device is original

The new idea is not any one sensor. The novelty is **shelf-state inference**. Pantry Warden treats a pantry shelf as a living storage system with airflow, contamination gradients, weight transitions, shape changes, and bioactivity signatures. By observing the shelf from the edge and sampling over time, it can determine whether the storage environment is stable or deteriorating.

Four design decisions make the concept distinct:

1. **Under-shelf perspective instead of package instrumentation.** The device looks across the shelf cavity, under package faces, and into stagnant rear pockets. That allows one sensor bar to monitor many goods at once.
2. **Multi-modal spoilage interpretation.** Gas data alone can be ambiguous. Pantry Warden fuses VOC rise, humidity drift, package bulge distance, mass change, and optical color shift to separate harmless restocking from emerging spoilage.
3. **Pest detection without traps as the primary sensor.** A contact microphone and MEMS microphone listen for chew and wingbeat patterns while the ToF sensor watches for nighttime micro-motion near the shelf edge. The system can warn before the owner finds visible damage.
4. **Storage-health scoring, not just telemetry.** The firmware produces shelf health, spoilage confidence, pest confidence, and condensation risk scores. The app speaks in operational advice instead of raw charts alone.

Existing kitchen scales, fridge thermometers, and pantry inventory apps each solve one slice of the problem. Pantry Warden creates a category that bridges environmental monitoring, inventory integrity, and biological contamination warning for dry storage.

## Practical problem it solves

Pantry losses are expensive and frustrating because the failure modes are gradual. Flour and cereal can absorb humidity before users feel clumping. Nuts and seeds can go rancid in warm stagnant pockets. Powdered drink mixes can wick moisture from tiny package failures. Pet food and bird seed attract pests long before users notice winged adults. Emergency food kits can sit for years while seals weaken and temperature exposure shortens shelf life. People also increasingly live in small apartments where food storage is close to dishwashers, laundry closets, poorly sealed exterior walls, or hot appliances. Those conditions create tiny shelf climates that are worse than the room average.

Pantry Warden reduces waste by detecting leading indicators:

- rising VOC signatures that match stale or fermenting food
- shelf-local humidity pockets behind dense storage clusters
- package bulge or settling inconsistent with normal use
- nighttime wingbeat or chew patterns consistent with infestation
- repeated condensation cycles on cold exterior walls
- unexplained mass loss or gain from moisture absorption or leakage
- light-exposed discoloration in clear containers or labels

The result is fewer discarded goods, earlier cleaning, better inventory rotation, and less guesswork about which shelf is becoming risky.

## Hardware specifications

### Compute and control

- **Primary MCU:** Espressif ESP32-S3-WROOM-1-N8R8
- Dual-core Xtensa LX7 at up to 240 MHz
- 8 MB flash and 8 MB PSRAM for local event history, OTA staging, and richer inference rules
- Native USB for provisioning, calibration export, and service console
- BLE for first-time setup and Wi-Fi for sync, OTA, and fleet monitoring

The ESP32-S3 is appropriate because Pantry Warden needs local signal processing, buffering, and optional app connectivity without forcing the user into cloud-only operation. On-device interpretation keeps alerts available even if the pantry is in a basement or off-grid cabin with intermittent network access.

### Sensing stack

- **SCD41** CO2, humidity, and temperature sensor for stale-air and moisture tracking
- **SGP41** VOC and odor trend sensor for volatile spoilage signatures
- **AS7341** multispectral sensor with controlled white/amber side illumination for package-face reflectance and discoloration trend analysis
- **VL53L4CD** time-of-flight sensor to estimate package front distance, bulge, and shelf occlusion geometry
- **NAU7802 + four foil load cells** integrated into clip feet for shelf mass trend sensing
- **MEMS microphone + piezo contact strip** for airborne wingbeat and structure-borne chew detection
- **Capacitive moisture strip** on the rear underside for condensation and hidden spill detection
- **Hall sensor on service door** for tamper and maintenance access logging

The sensor mix is deliberately balanced. Gas sensing finds chemistry changes, weight sensing captures depletion or absorption, optics catches visible drift, ToF sees geometric changes, and audio detects pest activity that humans rarely hear.

### Outputs and local UI

- RGB status light pipe visible at shelf edge
- 1.54-inch e-paper tag showing zone health and battery level
- Piezo beeper for critical spoilage or pest alerts
- Quiet mode button for kitchens and night use
- Micro-fan for scheduled air pulls across the gas chamber

### Connectivity

- Bluetooth Low Energy for commissioning and direct local control
- Wi-Fi 2.4 GHz for alerts, OTA updates, and multi-shelf dashboarding
- USB-C for charging and engineering console
- Optional ESP-NOW peer mesh for larger food-bank deployments without full Wi-Fi coverage

### Power system

- 2200 mAh Li-ion pouch cell
- BQ25895 USB-C charger with power-path management
- TPS63070 buck-boost 3.3 V rail for stable sensor operation across battery range
- Typical endurance: 3 to 4 weeks with 10-minute sampling and burst fan scans
- Low-power watch mode: up to 10 weeks when only condensation and pest sentinels remain active overnight

### Mechanical and form factor

- Main bar: 280 mm × 32 mm × 14 mm
- Clip feet span common 16–20 mm shelf thicknesses
- Replaceable airflow cartridge with dust screen and odor-neutral baffle
- Food-area-safe ABS/PC enclosure with silicone isolation pads
- Cable-free front install; charging from a recessed side USB-C port

## Architecture and system block diagram

Pantry Warden is organized as five cooperating subsystems:

1. **Shelf sensing plane** — load cells, capacitive moisture strip, ToF sensor, and optical module observe the shelf geometry and package state.
2. **Air chemistry plane** — SCD41 and SGP41 sample a micro-fan-drawn air path across the shelf face and into a small sensor chamber.
3. **Bioacoustic plane** — a MEMS mic handles airborne wingbeat noise while a piezo strip bonded to the enclosure picks up structure-borne nibbling and tap activity.
4. **Compute and inference plane** — the ESP32-S3 time-aligns samples, computes trend features, runs rule-based inference, and manages event logging.
5. **User and network plane** — the e-paper display, RGB light, BLE, Wi-Fi, and app protocol present health and recommendations.

```text
+---------------------------------------------------------------+
|                        Pantry Warden                          |
|                                                               |
|  Load Cells ----+                                              |
|  Moisture Strip +--> Shelf Sensor Front End -->                |
|  ToF Distance --+                           |                  |
|  Spectral Optics ---------------------------+                  |
|                                               --> ESP32-S3 --> BLE/Wi-Fi/App
|  SCD41 Temp/RH/CO2 --------------------------+                  |
|  SGP41 VOC + Fan Path -----------------------+                  |
|                                                                  
|  MEMS Mic ----------------------------------+                  |
|  Piezo Contact Strip -----------------------+--> Feature DSP -->|
|                                                               |
|  Battery + Charger + Fuel Gauge -----------> Power Manager    |
|  E-Paper + RGB LED + Buzzer <--------------- Alert Engine     |
+---------------------------------------------------------------+
```

Data moves over I2C for the environmental and optical devices, analog front ends for the load cells and piezo path, and a timer-driven sampling engine for synchronous feature extraction. The firmware keeps low-speed drift sensors on a slower cadence than the acoustic path so the battery budget stays practical.

## Firmware design details and rationale

The firmware is built as a compile-ready simulation-oriented C project so the repo carries executable logic, not just prose. The structure mirrors how a real embedded codebase would be organized on the target MCU:

- `main.c` coordinates sampling, inference, telemetry, and logging.
- `drivers/gas.*` models gas, humidity, CO2, and airflow behavior.
- `drivers/shelf.*` handles weight distribution, package distance, moisture strip, and optical freshness proxies.
- `drivers/acoustic.*` computes wingbeat, chew, and disturbance scores.
- `drivers/power.*` simulates battery drain, charging state, and power budgets.
- `drivers/inference.*` fuses all sensing into user-facing storage states.
- `drivers/comms.*` formats compact status frames for the companion app.
- `drivers/logger.*` stores recent shelf-health events and summaries.

The code uses a deterministic scenario generator rather than random noise so regression checks stay stable in CI or on another machine. That matters for a design repo: a future maintainer can compile the firmware, run it, and compare the output against expected shelf events. The simulation walks through normal operation, restocking, condensation buildup, a suspect spoilage event, and pest-like nighttime activity. This gives the project a working behavioral skeleton that can later be mapped to FreeRTOS tasks and actual peripheral drivers.

The firmware favors explainability over opaque machine learning. Pantry monitoring has relatively sparse but interpretable signatures, so rule-based confidence scoring is a good first-generation architecture. The device computes:

- stale-air index from CO2 trend, humidity bias, and fan response
- spoilage confidence from VOC lift, bulge growth, optical decline, and moisture retention
- pest confidence from chew score, wingbeat score, motion bursts, and timing
- restock confidence from abrupt weight gain, distance reduction, and low hazard chemistry
- condensation risk from humidity saturation and rear-strip moisture persistence

Those scores map into operational states such as `STABLE`, `RESTOCKED`, `CONDENSATION_WATCH`, `SPOILAGE_SUSPECT`, `PEST_WATCH`, and `CRITICAL_INTERVENE`. The design decision here is intentional: users need recommendations they can trust. A shelf monitor that says “anomaly score 0.82” is less useful than one that says “check the right-front package for bulging; VOC and distance changed together.”

The code is also structured to support future hardware growth. A second optical channel, barcode scanner, or E-Ink shelf label network could be added without redesigning the whole stack. The register map in `registers.h` reserves fields for zone arrays, event codes, and OTA status. The board definitions in `board.h` capture timing and threshold constants separate from logic.

## Companion application and software interface

Pantry Warden includes a browser-based companion app in `app/` because a pantry monitor benefits from simple, low-friction access on phones, tablets, and wall dashboards. The app is designed as a local-first single-page dashboard that could later be wrapped in Capacitor, React Native WebView, or another shell if the product moved toward mobile deployment.

The app contains four major screens:

1. **Overview** — shows shelf health, battery, dominant risk, and next actions.
2. **Shelf Map** — visualizes left/center/right shelf zones, weight balance, optical freshness, and package protrusion.
3. **Event History** — lists recent transitions such as restock, condensation watch, or pest suspicion.
4. **Calibration Lab** — lets the user simulate inventory changes, adjust thresholds, and export a JSON frame.

The UI logic parses compact JSON telemetry frames emitted by the firmware protocol. The protocol keeps field names short enough for BLE transport but still readable: mass, VOC, moisture, spoilage confidence, pest confidence, action code, and state label. The app includes a built-in sample frame generator so it remains useful as a standalone artifact in this repo even without live hardware.

The software interface design choices are practical:

- **Local meaning over cloud dependence.** A pantry device should still show useful state when the internet is down.
- **Recommendation-first UI.** Most users want to know what to inspect now, not study a dozen raw traces.
- **Calibration visibility.** Pantry geometry and stored goods vary, so users can tune shelf depth, sensitivity, and quiet hours.
- **Event transparency.** The app keeps a readable history to build trust in why the device raised an alert.

## Use cases and target audience

Pantry Warden has clear value for several groups:

### 1. Apartment households
Small apartments often have pantry shelves exposed to steam from adjacent cooking spaces, warm appliance walls, and limited airflow. Pantry Warden helps residents catch humidity damage and stale-air buildup before dry goods clump or spoil.

### 2. Families buying bulk staples
People who buy large amounts of flour, oats, rice, pasta, beans, cereal, pet food, and baking ingredients benefit from weight tracking, shelf-zone visibility, and early pest warning.

### 3. Food banks and mutual aid pantries
Community food spaces handle mixed packaging quality, irregular turnover, and varying storage conditions. A network of Pantry Warden bars could identify risky shelves before donors or recipients are affected.

### 4. RV, van, and marine users
Mobile living spaces experience large thermal swings and condensation. Pantry Warden can watch enclosed lockers where dry storage quietly degrades.

### 5. Preparedness and emergency storage users
Long-horizon food stores need verification that shelf conditions remain dry, stable, and pest-free. Pantry Warden adds trend awareness without opening containers constantly.

### 6. Small cafés and maker kitchens
Not every small food business has a sophisticated dry-storage monitoring system. Pantry Warden offers a low-cost, retrofittable way to track shelf health where powders, spices, and packaged ingredients matter.

## Example operating scenarios

### Scenario A: Flour shelf in a humid apartment
Steam from a nearby kettle condenses on an exterior-wall pantry shelf. The rear moisture strip stays elevated overnight while humidity rises faster behind stacked canisters than in room air. Pantry Warden flags a condensation watch and recommends spacing containers and adding airflow.

### Scenario B: Hidden spoiled pouch
A vacuum-sealed snack pouch begins to swell. The front distance to the package changes by several millimeters, VOC index trends upward, and optical reflectance shifts from normal matte to slightly glossy due to bulge angle. The device marks a spoilage suspect event at the right-front zone.

### Scenario C: Pantry moth early detection
The shelf is quiet during the day, but at night the piezo path records repeated faint chew bursts while the MEMS mic sees intermittent wingbeat-like energy near 95–120 Hz. Weight remains stable, but pest confidence rises. The app advises inspecting flour and cereal, notifies the user, and suggests enabling a deeper night sweep.

### Scenario D: Normal restock
The user loads several cans and dry-goods boxes. Shelf mass jumps quickly, package distance drops, and VOC remains flat. The firmware classifies the event as a restock, updates the baseline, and avoids a false alert.

## Design tradeoffs

Pantry Warden intentionally avoids trying to identify exact food species or provide medical-grade contamination certification. It is a screening and maintenance device, not a lab instrument. The sensor suite was selected for strong directionality and cost practicality rather than maximum analytical chemistry precision. That keeps the concept feasible for a consumer or prosumer product.

The design also balances battery life with insight depth. Continuous audio streaming would detect more subtle events but would consume too much power and require more privacy handling. Instead, Pantry Warden uses short listening bursts, event-triggered escalation, and a contact sensor that is more power-efficient than constant airborne recording.

A final tradeoff is mechanical simplicity. Integrating load cells into clip feet gives useful trend data without requiring the entire shelf to sit on an instrumented platform. Accuracy is lower than a laboratory scale, but the installation burden is much smaller, which is the right compromise for adoption.

## Manufacturing and prototyping notes

A first prototype can be built around an ESP32-S3 dev module, breakout boards for SCD41, SGP41, AS7341, and VL53L4CD, a NAU7802 load-cell board, and a simple analog front end for piezo sensing. The enclosure should isolate the fan airflow chamber from the electronics volume so warm MCU operation does not bias gas readings. The optical window needs a matte shroud to reject ambient kitchen lighting as much as possible. The load-cell clip feet should preload gently to avoid excessive creep while still capturing relative shelf mass changes.

The production version would likely move to a long narrow two-board stack: a compute/power board in the center and sensor daughterboards placed toward the left and right ends. That arrangement improves spatial awareness across wider shelves and supports modular length variants.

## Validation plan

The device should be validated against realistic pantry failure modes rather than only synthetic bench metrics. Useful tests include:

- humidity soak and recovery with different package stacks
- controlled VOC sources for onions, spices, rancid nuts, and fermenting samples
- bulge detection with staged pouch inflation increments
- mealworm or pantry moth proxy acoustic captures in enclosed bins
- false-positive suppression during ordinary restock and door-open disturbances
- battery profiling with different night-sweep duty cycles

A strong validation goal is trustworthy intervention: the user should believe that when Pantry Warden says to inspect one zone, it has a concrete reason.

## Repository contents

- `README.md` — full system overview and design rationale
- `firmware/` — compile-ready C simulation firmware and drivers
- `kicad/` — KiCad project, schematic, and PCB outline with real symbols, footprints, and named nets
- `app/` — local-first companion dashboard with multiple screens and telemetry parsing

Pantry Warden is a new category of storage-health hardware: a device that understands the shelf, not just the room. It should exist because food waste, pest damage, and hidden moisture are common, expensive, and usually discovered too late.
