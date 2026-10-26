# Wick Escape — breakthrough-versus-bypass notebook for paper barriers

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is an engineering prototype. The custom PCB is a routed passive conductance interface for a separately purchased NUCLEO-F401RE. It is not a standalone microcontroller board. Target compilation, software tests, schematic connectivity and PCB design-rule checks can be reproduced from these files. No board has been fabricated, no target has been flashed, and no liquid coupon, electrode mounting, conductivity range or physical timing accuracy has been validated.

## Purpose and overview

Wick Escape helps a teaching laboratory or paper-microfluidics workshop answer a deceptively narrow question: did liquid appear beyond a paper barrier, or did it first escape around one of the barrier's ends? A downstream wet mark alone cannot distinguish those paths. A stopwatch also cannot remember which edge became wet while an instructor was helping another student. This device records four separate electrical wetting observations and refuses to describe a contaminated observation as an uncomplicated downstream arrival.

The primary user is an instructor comparing hand-drawn wax barriers on inexpensive cellulose coupons. Each coupon has one upstream sensing zone, one downstream zone, and two flank zones outside the barrier endpoints. Four isolated graphite electrode pairs contact those zones. A low-current resistance measurement reports qualified wetting. After a dry baseline, upstream wetting starts a bounded trial. Downstream wetting records a latency; flank wetting invalidates that interpretation. Late flank wetting can revoke an already recorded downstream result, which is deliberately labelled provisional in the application.

This is not a medical strip reader, a composition analyzer, a quantitative flow meter or a guaranteed leak detector. It observes resistance thresholds at four places. It cannot establish the path taken by every molecule, detect a dry channel disconnected from its electrode, or prove that an unwetted downstream zone is protected indefinitely. The useful output is a qualified notebook entry that helps decide which coupons deserve microscopic inspection or a better controlled experiment.

## Originality and alternatives considered

The repository was inventoried by directory and README purpose before selecting this concept. Its closest infrastructure relatives are ContactAtlas, which maps detached switch continuity; Rinse Ledger, which records basket immersion; and Siphon Witness, which records vessel threshold transfer. None performs four-zone resistive paper-barrier breakthrough and flank-contamination classification. The MCU platform and passive carrier routing pattern are intentionally reused, rather than presenting another register initialization sequence as an invention.

Prior art makes broad novelty claims inappropriate. Electrical wicking measurement already exists. Three relevant comparisons are:

- [Capacitive platform for real-time wireless monitoring of liquid wicking in a paper strip](https://doi.org/10.1039/d3lc00368j), which measures changing capacitance beneath a strip and supports flow monitoring. Wick Escape instead sacrifices continuous flow estimation for independently observed upstream, downstream and flank qualifications.
- [Electronic flow monitoring of paper/textile-based microfluidic devices](https://doi.org/10.70477/mqzk2765), which describes an interdigitated capacitive sensing platform. This proposal uses replaceable contact electrodes and a small diagnostic state machine, not a distributed capacitive position estimate.
- [Capillary rise in woven fabrics by electrical principle](http://nopr.niscair.res.in/bitstream/123456789/11227/1/IJFTR%2036%281%29%2099-102.pdf), which discusses electrical observations at multiple levels. The specific proposed contribution here is treating flank arrival and late escape as disqualifying evidence in a wax-barrier coupon workflow.

These references were found by an external prior-art search; they are not evidence of patentability, exhaustive market novelty or experimental performance of this design. Five candidates were considered: a generic capillary-rise logger, a four-zone barrier notebook, a drawer closure logger, an encoder backlash meter and a refrigerated wetness badge. The generic logger overlaps published work directly, the drawer logger offers weak differentiation, backlash overlaps Takeup Atlas, and refrigeration overlaps existing refrigerator projects. The barrier notebook offers a specific experimental decision rather than a general sensor dashboard.

## Hardware specifications and boundary

The processor is the STM32F401RET6 on ST's NUCLEO-F401RE development board: Cortex-M4, 512 KiB flash and 96 KiB SRAM. Firmware runs on its internal 16 MHz HSI clock. The Nucleo supplies regulation, MCU decoupling, reset circuitry, SWD programming, ST-LINK and USB virtual serial connectivity. None of those components is silently implied to be on the passive carrier schematic.

The carrier is an 80 by 70 mm, two-layer, 1.6 mm FR-4 board. It contains fourteen through-hole components: six headers and eight resistors. Four 100 kilohm pull-ups connect ADC inputs to the Nucleo's 3.3 V rail. Each ADC input reaches its electrode connector through a separate 1 kilohm resistor. The other electrode terminal connects to common ground. A pair of graphite contacts bridged by a wet coupon therefore forms a divider with the pull-up. No battery, charger, radio, motor or hazardous actuator is included.

| Carrier connection | External connection | Meaning |
| --- | --- | --- |
| J1 pin 1 | Nucleo CN6 pin 4 | Regulated 3.3 V, never 5 V |
| J1 pin 2 | Nucleo CN6 pin 6 | Ground |
| J2 pin 1 | CN8 pin 1, A0, PA0 | ADC1 channel 0, upstream |
| J2 pin 2 | CN8 pin 2, A1, PA1 | ADC1 channel 1, downstream |
| J2 pin 3 | CN8 pin 3, A2, PA4 | ADC1 channel 4, left flank |
| J2 pin 4 | CN8 pin 4, A3, PB0 | ADC1 channel 8, right flank |
| J3 through J6 pin 1 | Individual graphite signal contact | Upstream, downstream, left, right |
| J3 through J6 pin 2 | Matching graphite ground contact | Return for that local pair |

The Nucleo must retain its normal USART2-to-ST-LINK virtual-COM arrangement. PA2 uses alternate function 7 for transmit at 115200 baud, eight data bits, no parity, one stop bit. There is no firmware receive channel and no application command path. Consult ST's UM1724 Nucleo-64 manual and the actual board revision before connecting headers; a visually similar Nucleo model is not an interchangeable processor target.

## Architecture and measurement model

```text
Cellulose coupon: upstream / downstream / left flank / right flank
        | four local graphite pairs, no external powered source
        v
100k pull-ups + 1k series resistors on passive carrier
        | four analog voltages, common ground
        v
NUCLEO-F401RE ADC1: channels 0, 1, 4, 8
        | mux discard + eight-sample average per channel
        v
Independent hysteresis and five-sample qualification
        | dry baseline / ready / active / complete / expired / invalid
        v
CRC-protected W1 snapshots -> USART2 -> ST-LINK USB
        v
Linux desktop: Connection / Live coupon / History / Method
        | local evidence export, no cloud
        v
Author-attributed JSON notebook with validity and provenance
```

Ignoring leakage and electrode polarization, the ADC fraction is approximately `(Rprobe + 1000) / (100000 + Rprobe + 1000)`. Firmware considers counts at or below 1200 wet and counts at or above 3000 dry. At a 4095 full scale these correspond to approximately 40.45 kilohms and 272.97 kilohms of probe resistance. These are thresholds, not calibrated material properties. The intervening band preserves the last qualified level but cancels a pending transition.

The maximum nominal shorted-probe current is about 32.67 microamps per channel at 3.3 V. Four shorted probes dissipate approximately 0.431 milliwatts collectively. Resistor tolerances, analog leakage, surface contamination and cable capacitance change the observed boundary. The 100 kilohm source impedance requires the long ADC sampling interval used here. Validate it by resistor substitution; do not assume ideal-divider algebra establishes target ADC accuracy.

## Coupon fixture and assembly

A starting mechanical arrangement is a 60 by 40 mm cellulose rectangle supported horizontally on a clean insulating tray. Let x run along the intended flow direction and y run across the coupon. Draw a wax line at x = 30 mm from y = 4 to 36 mm. Suggested electrode-pair centers are upstream at (20,20), downstream at (40,20), left at (30,2), and right at (30,38), all in millimeters. Each pair has two small graphite contact tips separated by approximately 1 mm. These dimensions define an initial experiment, not a validated standard coupon geometry.

Use a removable nonconductive bridge to hold electrode tips gently against the paper. The tips must not scratch through the paper, crush the local pores, or create a capillary shortcut along a supporting thread. Keep the two flank pairs distinct and outside the wax endpoints. Photograph the assembled coupon and retain its dimensions separately with the exported notebook. If the wax spreads or the paper curls, adjust and document the geometry rather than comparing the resulting numbers as though nothing changed.

Use short paired leads, preferably under 150 mm, and keep the carrier and Nucleo outside the tray. The ground contacts are electrically common but mechanically separate. A continuous wet grounded wire crossing the coupon would itself distort the experiment. Secure cables to the dry fixture, not to the coupon. The delivered board has no mounting holes; use insulated edge supports or an enclosure tray, never a metal clamp contacting copper. No enclosure CAD or industrial electrode jig is claimed.

Before introducing liquid, inspect orientation, measure resistor values, check continuity and confirm no 3.3 V-to-ground short. Connect one pair at a time to known resistors. An open should read near full scale, a 10 kilohm test resistor should be comfortably wet, and a 1 megohm resistor should be dry. Swap channels to distinguish a cable problem from a channel problem. This detects many assembly errors, but an open electrode remains indistinguishable from genuinely dry paper during normal use.

## Firmware details and decisions

Firmware is freestanding C11 with explicit startup vectors, data/BSS initialization, a flash/RAM linker script and direct documented peripheral registers. `registers.h` references STM32F401 RM0368 Rev 6, particularly ADC chapter 11. The clock driver selects HSI, verifies the switch, clears bus dividers, disables PLL and only then reduces flash latency. Clock failure stops before acquisition. This is a reset-entry firmware image, not an arbitrary bootloader handoff ABI.

ADC1 uses PCLK2 divided by two, giving an 8 MHz ADC clock. Each selected input uses the 480-cycle sampling setting. The first conversion after selecting a channel is discarded, then eight conversions are averaged with rounding. A complete four-channel vector has 36 conversions, an ideal conversion time near 2.214 ms before software overhead. Input vectors are scheduled every 20 ms. Conversion polling has a bounded iteration budget; timeout disables the converter and invalidates the trial rather than synthesizing a dry reading.

Each input qualifies independently after five consecutive threshold samples. This avoids an unrelated flank fluctuation delaying the upstream timestamp. A successful dry startup requires fifty fully dry vectors. The first dry vector establishes an observation, so this is approximately a second of samples, not a certified one-second continuous interval. The five-sample rule similarly spans four sample intervals from the first qualifying observation to the fifth. Signal changes between samples are unobserved.

The state machine has six states. BASELINE waits for dry qualification. READY waits for upstream without downstream or flank activity. ACTIVE measures from qualified upstream arrival. COMPLETE means downstream subsequently qualified with no observed flank contamination. EXPIRED means the active trial reached 600000 ms without an earlier accepted downstream arrival. INVALID preserves sticky reasons. A simultaneous first upstream/downstream vector is an ordering fault. Flank activity takes priority over completion. Qualified upstream drying during ACTIVE invalidates the trial because the intended source condition was lost.

An interval over 25 ms between successful input vectors is an acquisition-gap fault; the five-millisecond allowance tolerates small scheduling jitter but not an entire missed scheduled vector. Gap, ADC, serial-transmit and watchdog-startup faults are terminal for that coupon. A watchdog is started using LSI and fed only after scheduled sampling work. All busy waits before SysTick have iteration budgets. Actual watchdog period and HSI timing tolerance must be checked on the assembled board; no crystal-grade latency accuracy is claimed.

After COMPLETE, latency and first-arrival times freeze. Later qualified flank wetting changes the state to INVALID with the bypass flag. No precise timestamp is invented for that late flank event: its first-arrival entry remains unseen, and the fault plus snapshot uptime establish only the later observation. After EXPIRED, classification is closed; subsequent wet levels update, but cannot rescue the expired trial. Reset requires a new dry baseline and should be performed with a fresh coupon. There is no persistent storage or automatic multi-coupon sequencing.

## Application and software interface

The companion is a real Python/Tk desktop application appropriate for a USB-connected bench instrument. Python 3 with Tk and Linux termios support is required; there are no pip dependencies, package downloads or native mobile build requirements. Run `python3 app/wick.py` from this directory. The Connection tab selects the serial device, starts a fresh session, disconnects while retaining evidence, opens a raw replay, and exports a new JSON file. Live coupon shows state, first arrivals, wet mask, fault mask and four ADC bars. History shows the latest 200 snapshots; the model retains up to 5000. Method explains the boundaries while the user is at the bench.

Every transmitted line is `W1,seq,uptime,state,mask,faults,elapsed,a0,a1,a2,a3,t0,t1,t2,t3,CRC` followed by LF. All numeric fields except CRC are unsigned decimal. CRC is four uppercase hexadecimal digits of CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xffff, over bytes from W through the final time digit, excluding the comma before CRC. Sequence and uptime wrap modulo 32 bits. First-arrival sentinel 4294967295 means no timed event; upstream arrival is zero. ADC counts are 0 through 4095. Masks use upstream bit zero, downstream bit one, left bit two and right bit three.

State numbers are BASELINE 0, READY 1, ACTIVE 2, COMPLETE 3, EXPIRED 4 and INVALID 5. Fault bits are order 1, bypass 2, source drying 4, acquisition gap 8, ADC 16, transmission 32 and watchdog startup 64. Protocol W1 fixes thresholds and qualification policy; a changed policy must use a new protocol version rather than silently reusing W1. Snapshots repeat approximately five times per second, so late attachment does not require a one-shot boot announcement.

The parser bounds frames, validates CRC and field ranges, and checks state-specific relationships. It rejects contradictory duplicates, backward sequence/time progress, disappearing sticky faults, changed arrival times and impossible terminal transitions. Duplicate snapshots do not refresh freshness. Live receipt ages past 1.5 seconds are stale; a reception pause beyond two seconds blocks subsequent queued data until reconnect. Cumulative host/device progress is checked with a one-second buffering allowance plus two percent drift tolerance. This is a heuristic, not authenticated continuity: certain resets with forward-looking fields remain indistinguishable because there is no persistent boot identity.

Freshness describes received telemetry, not a guarantee that every ADC number is current. An ADC fault explicitly marks exported rows `adc_valid: false`, suppresses the bars and labels counts last-known. Serial disconnect preserves source provenance. Exports include author, protocol, connection state, freshness, warning text, parser errors, discarded-row count and record age even when there are no observations. Files are created privately with mode 0600 and never overwrite existing files. Raw replays must be regular files no larger than one MiB, contain at least one accepted frame, and end without a partial frame. JSON evidence exports are not raw replay files; the application does not currently record a raw UART archive.

## Use cases, operating procedure and power

Use cases include comparing wax application techniques, demonstrating why an edge leak can confound a downstream endpoint, and identifying coupons needing visual investigation. Begin with a dry assembled coupon, connect the USB device, reset the target, and wait for READY. Apply a small repeatable quantity of benign conductive water upstream without bridging an electrode directly with a droplet. Observe both the paper and the application. Export after the relevant observation, noting that a later bypass can revoke a provisional result. Save a final export before removing the coupon.

Use only small volumes of nonhazardous aqueous solution. Deionized water may remain too resistive to qualify even when visibly wet; salt concentration changes both electrical response and the experiment. Hold solution composition, temperature, paper lot, electrode pressure and geometry fixed during comparisons. Never infer concentration from the ADC counts. Electrode polarization and DC electrochemistry limit repeatability, which is why contacts are replaceable and trials are short. Disconnect power after use, clean the dry fixture and dispose of coupons under local laboratory practice.

Power comes entirely from the Nucleo USB connection. Budget 100 mA at 5 V for the complete bench assembly as a conservative planning allowance, not a measured consumption figure. The passive sensing network adds at most roughly 0.131 mA on the 3.3 V rail with four shorted probes. A host USB supply and cable must satisfy the Nucleo's documented limits. There is no battery-life claim because no battery system is designed here.

Indicative quantity-one budgeting, not supplier quotations: NUCLEO-F401RE $20–35; small two-layer carrier $3–10 per board in a prototype batch; eight 1% axial resistors $2 allowance; six headers $2–4; graphite contacts and paired leads $5–15; insulating tray and removable bridge $5–20; USB cable $2–5. That gives an approximate $39–91 assembly envelope excluding tools, labor and shipping. Quantity-one hundred pricing has not been sourced and should not be extrapolated from these ranges.

## Safety, privacy and known limitations

The circuit has no galvanic isolation, input clamps, dedicated ESD protection or independent input overvoltage limiter. The 1 kilohm resistors are not a safety barrier. Connect only passive electrode pairs on an isolated coupon. Never connect powered sensors, live equipment, mains wiring, human subjects, food-contact processes or medical samples. USB ground is connected to the host computer. Keep liquids physically separated from electronics; the enclosure is not waterproof and the board has no ingress rating. Supervise all wet experiments.

There is no radio or cloud service, no user account, and no network permission. Measurements remain in volatile device RAM and local application memory unless explicitly exported. A local process able to replace a serial device or replay file can forge evidence; CRC detects corruption, not malicious authenticity. There is no signed update system. Development firmware is programmed through ST-LINK/SWD with local physical access.

A zero-violation CAD report does not qualify electrodes, wetting physics, EMC, regulatory compliance or production manufacturability. Further work includes electrode impedance characterization, repeatable fixture pressure, AC excitation to reduce polarization, a persistent session identifier, and physical comparison against synchronized video. Those are future tasks, not delivered capabilities.

## Reproduce builds and verification

From this device directory run `make -C firmware all test`, `python3 tests/test_app.py`, and `xvfb-run -a python3 tests/test_gui.py`. The first command builds and links Cortex-M4 firmware and runs an address/undefined-behavior-sanitized C test executable. The GUI tests drive a real Tk event loop and Linux pseudoterminal; that is simulated transport, not a flashed board. The C encoder generates the synthetic raw fixture consumed by app tests.

With KiCad 9 and its Python bindings installed, run `/usr/bin/python3 tools/generate_hardware.py` to regenerate native CAD files, then `/usr/bin/python3 tools/check_hardware.py`. The checker invokes real schematic ERC, PCB DRC and a fresh XML netlist export, comparing every reference, pin, net, value and footprint identity against the board and manifest. The symbol library is embedded and also supplied locally for editing. See VERIFICATION.md for measured results, tool versions and remaining bench gates. Author attribution is checked in every delivered text file; build outputs and local preferences are intentionally ignored.
