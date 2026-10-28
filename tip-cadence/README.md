# Tip Cadence — paired emitter timing with alternating-bucket evidence

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original software and locally authored design sources are MIT licensed; see LICENSE.

Revision A is an engineering prototype. Its STM32 firmware is target-buildable and its custom carrier is routed, but no physical assembly, target programming, hydraulic calibration or irrigation trial has been performed. The carrier is a passive input board for a separately purchased NUCLEO-F401RE, not a standalone microcontroller board. Automated checks establish properties of source files and simulated signals, not metrological performance.

## Purpose and why this exists

Tip Cadence is a bench instrument for a nursery technician comparing two low-flow irrigation emitters. Each emitter drains into its own magnetic tipping-bucket assembly. Instead of reporting only a total number of tips, the instrument records completed tip pulses, intervals between completed pulses, alternating interval cohorts, maximum adjacent-interval change, and interruptions caused by handling. The companion application puts both channels beside one another and refuses to display an alternating-cohort contrast after continuity has been lost.

The trigger moment is a confusing catch-cup test: two emitters deliver approximately equal total mass over a long interval, yet one produces visibly uneven short-term delivery. The technician needs to distinguish three explanations before replacing parts: intermittent emitter output, unequal left/right bucket volumes, and interference from moving the collection fixture. A stopwatch and notebook can record total time, but cannot reliably retain short alternating intervals across two independent fixtures while also recording handling interruptions. A generic pulse counter hides the timing pattern entirely.

The proposed differentiator is a paired, handling-aware cadence workbench, not a new rain gauge or an assertion that tipping buckets are new. It keeps relative even and odd intervals separate within an uninterrupted observation sequence, making a regular alternation visible alongside irregular changes. It never claims that an alternating pattern proves a specific hydraulic defect. A controlled fixture swap and independent mass measurement remain necessary. Relative parity is anchored to the first observed complete tip, not to a physical left bucket.

The value gate is deliberately testable. Before treating this as a useful instrument, a technician should run a blinded comparison with equal-total-mass streams, one deliberately interrupted and the other steady, and identify the intermittent stream in at least nine of ten trials while flagging every deliberate handling interruption. This is an acceptance target, not an achieved result. If ordinary tipping mechanics create indistinguishable patterns, the concept should be abandoned or redesigned rather than marketed with a confident label.

Non-goals are important. This is not a medical infusion counter, a pump controller, a rainfall standard, a leak alarm, or a safety interlock. It does not measure flow volume without independent calibration. It cannot detect a broken reed cable, distinguish a missed mechanical tip from a quiet emitter, or certify that a plant receives adequate water. No output controls a valve. The instrument is intended for supervised clean-water bench work only.

## Novelty boundaries and repository comparison

The repository inventory was checked before implementation. RainForge measures raindrop impact and energy; Siphon Witness records threshold transitions between vessels; Wick Escape compares barrier breakthrough and bypass; PipeWhisper investigates plumbing acoustically. None of those documented purposes is this dual, relative-parity tipping-bucket comparison. The nearest reusable electrical platform is the passive four-contact carrier already used in this repository. Reusing its tested clock, serial, startup and carrier generation code does not make its previous measurement algorithm part of this device: the cadence engine, wire format, workbench and tests are specific to Tip Cadence.

Bounded prior-art searches found substantial related work. Dual tipping-bucket rainfall gauges exist, and automated irrigation testing has used many tipping gauges. Optical drip counters also exist. Therefore no claim of worldwide originality, patentability, freedom to operate, or absence of commercial equivalents is made. The design contribution here is the narrow workflow combining two independent interval streams, relative alternation evidence, conservative handling invalidation, and a local evidence export. Relevant sources are listed at the end so this boundary remains auditable.

Other candidate concepts were rejected: an optical liquid-drop comparator depends heavily on chamber alignment and refraction; a generic cumulative irrigation counter duplicates established instruments; a closed-loop emitter controller adds hazardous operational assumptions without solving the diagnostic ambiguity. Tip Cadence instead measures a well-defined electrical proxy and exposes its limitations.

## Hardware specification and system architecture

The processor is the STM32F401RET6, an ARM Cortex-M4 in LQFP64, on a NUCLEO-F401RE MB1136 development board. Firmware uses the internal 16 MHz HSI oscillator, 512 KiB flash and 96 KiB SRAM memory map, a 1 kHz SysTick, GPIO inputs and USART2 transmission. No FPGA, wireless radio, operating system, external crystal, cloud account or removable storage is required. The Nucleo supplies its own regulator, decoupling, reset circuit, SWD/debugger and USB virtual serial bridge. Those components are intentionally not duplicated or falsely represented on the passive carrier schematic.

The custom carrier measures 80 by 70 mm, uses two copper layers and 1.6 mm nominal FR-4, and has through-hole 2.54 mm headers and axial resistors. It connects four voltage-free switches to four MCU GPIOs. Two switches belong to purchased tipping-bucket mechanisms with reed pulse outputs. Two are maintained handling switches, one per fixture, which the operator closes before touching, emptying, levelling or swapping that fixture. They are not automatic overflow sensors and do not establish that the operator remembered to use them.

```text
Emitter A -> tipping bucket A -> reed contact -> J3 --10k/1k network-- PA0
Emitter B -> tipping bucket B -> reed contact -> J4 --10k/1k network-- PA1
Handling switch A ----------------------------> J5 --10k/1k network-- PA4
Handling switch B ----------------------------> J6 --10k/1k network-- PB0
                                                       |
USB host <-> Nucleo ST-LINK VCP <- USART2 TX PA2 <- STM32F401
                                                       |
                                             two independent engines
                                                       |
                                           CRC-protected TC1 snapshots
                                                       |
                                       local Linux Tk evidence workbench
```

J1 pin 1 connects only to Nucleo CN6 pin 4, the 3.3 V output. J1 pin 2 connects to CN6 pin 6 ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. J3 is TIP_A, J4 TIP_B, J5 HANDLE_A and J6 HANDLE_B. Every field connector uses pin 1 for its signal and pin 2 for ground. Each SENSE net has a 10 kilohm pull-up to VCC and a 1 kilohm series path to its external switch. The generated manifest and fresh schematic netlist are checked against the actual PCB pad assignments.

Only dry, disconnected contacts are permitted. Do not connect a commercial sensor's powered voltage output, a 12 V rain-sensor bus, an industrial PLC contact circuit, or mains wiring. The board provides neither galvanic isolation nor certified surge or ESD protection. USB ground connects the entire assembly to the host computer. Use short leads, keep wet collectors physically below and away from electronics, and unplug USB before changing wiring. The series resistor is not a substitute for protection. Switch compatibility at low wetting current must be checked using the selected sensor's specification and bench measurements.

## Sensor mechanics, mounting and calibration

Choose two identical tipping-bucket assemblies exposing a simple normally open reed switch. They must produce one closed pulse for each mechanical tip; a sensor that merely toggles state on alternate tips does not meet this interface. No particular supplier is required, because cadence compatibility is an acceptance property that must be checked, not inferred from a generic connector. Before purchase, obtain pulse width, bounce, contact-rating and maximum-tip-rate information. This reference design is deliberately not a new unvalidated precision bucket mechanism.

Mount both mechanisms on a rigid, independently levelled tray with separate catch vessels. Keep outlet tubes unrestrained and above the receiving funnels without permitting a siphon back into an emitter. Use clean water at ambient conditions. Mark fixture identity, emitter identity, height, pressure, temperature and catch mass in the laboratory notebook. The electrical export contains no automatically measured pressure, mass or temperature; these contextual observations must not be invented by the app.

Characterize each mechanism with a steady supply and video. Compare the tip pulse count with visible mechanical tips and capture the pulse waveform using a scope or logic analyzer. Closed pulses should be comfortably inside the 10 to 2000 ms accepted envelope after qualification, and completed tips should be separated by 100 to 60000 ms. Those limits define the supported experiment, not the unrestricted flow range of the sensor. A sensor outside them requires a reviewed firmware and protocol revision.

For a useful comparison, collect enough tips for both relative cohorts to contain several intervals, stop the streams, export, then repeat with emitters exchanged between fixtures. If an alternating pattern follows a fixture, investigate bucket levelling and unequal side volumes. If irregular timing follows an emitter, investigate pressure, clogging and feed stability. These are hypotheses rather than automatic diagnoses. Weigh accumulated output with an independent balance; do not multiply counts by an assumed universal tip volume. Residual liquid, surface tension, splash loss and tipping dead time can all change the relationship between mechanical events and mass.

## Power, bill of materials and assembly budget

The Nucleo is powered through its supported USB connection and power-jumper configuration. The carrier has no battery or charger. Do not feed its VCC from a second supply while the Nucleo is attached. The design budgets 100 mA at 5 V for the whole USB-powered development-board assembly; this is a conservative planning allowance, not a measured operating current or a statement about every Nucleo jumper configuration. Measure actual current during bring-up before selecting a supply or enclosure.

For a closed contact the nominal carrier current is 3.3 V divided by 11 kilohms, or 0.30 mA. Four simultaneous closures therefore draw approximately 1.20 mA from the 3.3 V rail. The MCU-side low level is nominally 0.30 V because the 1 kilohm series resistor and 10 kilohm pull-up form a divider when grounded. Open contacts ideally draw no DC current apart from leakage. The carrier does not determine the development board's dominant debugger and MCU consumption. No battery-life claim is made.

| Item | Quantity | Planning allowance, USD | Notes |
|---|---:|---:|---|
| NUCLEO-F401RE | 1 | 20–35 | MCU, regulator, debugger and USB bridge |
| Bare passive carrier | 1 | 3–10 | Small-batch PCB estimate |
| 10k and 1k axial resistors | 4 each | 1–3 total | 1 percent, quarter-watt is sufficient |
| 2-pin headers | 5 | 1–3 total | J1 and J3 through J6 |
| 4-pin header | 1 | 1 | J2 |
| Reed-output tipping assemblies | 2 | 20–60 each | Must pass pulse compatibility tests |
| Maintained dry-contact switches | 2 | 2–6 total | Clearly label handling inhibits |
| Tray, wiring and splash enclosure | 1 set | 10–25 | Keep electronics dry |

These are rough budget ranges, not current supplier quotes or verified stock. Use standard axial 7.62 mm resistor footprints and inspect pin-1 marks before assembly. Check resistance from VCC to ground before connecting the Nucleo. With switches disconnected, verify all four sense nodes rise to 3.3 V. Close one contact at a time and confirm only its intended GPIO changes. Then test simultaneous closures. The absence of an active regulator on this carrier is intentional; it is not evidence of a missing schematic part.

## Firmware measurement contract

The firmware is freestanding C11. GPIO polling is separated from the pure cadence engine, and serialization is separated from both. The main loop polls the nonblocking USART transmitter, samples on each newly observed millisecond tick, feeds the independent watchdog after an acquisition step, and sends a complete snapshot every 100 ms when the transmitter is idle. Sampling is not deferred until a transmission completes. A missed tick is explicitly detected rather than extrapolated as observed stability.

Each tip input has an independent five-sample qualifier. Chatter on B cannot postpone A's accepted transition. A newly started lane waits for a qualified open baseline, then enters READY. A qualified closed input starts PULSE. A qualified reopening completes the envelope; a duration from 10 through 2000 ms is accepted as one tip. Short qualified envelopes are rejected, and an overlong closure is rejected once and must clear before reuse. Sub-five-sample raw pulses never become a qualified envelope and are not counted as rejected complete envelopes. This distinction matters when comparing scope traces with counters.

Intervals are measured between qualified pulse completions, not electrical first edges or mechanical bucket endpoints. The first valid pulse establishes an anchor. Subsequent completion intervals from 100 through 60000 ms contribute to total intervals, minimum, maximum and one relative-parity cohort. The first interval after an anchor belongs to cohort zero, then parity alternates. Maximum adjacent change compares neighboring accepted periods within the same uninterrupted chain. It is not a standard deviation or a statistical confidence bound.

A raw handling closure immediately invalidates that lane, even if shorter than the debounce threshold. A scheduler gap invalidates both lanes. A rejected envelope or an expired interval also breaks the affected chain. Break counters count invalidated existing anchors, not a known number of missed tips. An incomplete first pulse lost during a gap has no anchor and does not increment a rejected-envelope count. Totals already established are retained. After invalidation a newly qualified clear baseline is required before a fresh chain can begin.

Cohort counts and sums are cumulative across the boot session, but relative phase restarts after every broken chain. Therefore pooled cohort means after a break must not be interpreted as physical bucket asymmetry. The application suppresses the contrast after any break, rejection, acquisition gap or terminal fault. To obtain a new clean comparison, intentionally reset the target and explicitly reconnect the app after the apparatus is stationary. Merely reconnecting does not clear target counters.

All multi-counter sample changes are staged transactionally. If a 32-bit aggregate would overflow, neither lane receives a partial update; a sticky terminal fault freezes acquisition evidence while timestamps continue so the fault remains observable. Ages are bounded instead of allowed to alias after a full timer wrap. Ordinary millisecond wrap is handled by unsigned subtraction. Configuration and watchdog-start failures also enter terminal fault. These flags are diagnostics, not safe-control outputs.

The startup code initializes RAM and vectors; the linker script declares the exact flash and RAM regions and reserves stack headroom. Clock initialization returns to HSI with bounded waits before changing flash latency. GPIO configuration is read back. USART2 is TX-only at 115200 baud, eight data bits, no parity and one stop bit. There is no command parser, remote reset or firmware update service. Programming is through the development board's normal ST-LINK interface. Reset causes are not persisted, so the app cannot prove session continuity across every possible reboot.

## Protocol, application and evidence handling

Every snapshot begins with `TC1,`, contains 33 unsigned decimal fields, and ends with `*` followed by four uppercase hexadecimal CRC digits and LF. CRC is CRC-16/CCITT-FALSE, polynomial 0x1021, initial value 0xffff, covering the bytes before the asterisk. The five common fields are uptime, raw closed-bit mask, scheduler-gap count, transmitter-error count and fault mask. Each lane then contributes tips, interval count, even count, odd count, even sum, odd sum, minimum, maximum, maximum adjacent change, rejected envelopes, broken anchors, phase, anchor-valid flag and age since anchor. Times and sums are milliseconds. Phase values are zero WAIT_CLEAR, one READY and two PULSE. Fault bits are configuration 1, watchdog 2, overflow 4 and invalid input 8.

The Linux companion is a Python standard-library Tk application. Its Connect screen opens a selected local serial path; the Cadence screen shows both lane counts and timing summaries; the Evidence screen loads raw serial recordings and exports private JSON. Run `python3 app/cadence.py` from this directory on a desktop with Tk installed. A fresh session requires valid TC1 identity rather than accepting arbitrary serial text. A second connection is refused until the first is closed, and original terminal settings are restored on close or setup failure.

The parser validates CRC, field count, unsigned bounds, interval conservation, cohort sums and extrema. It resynchronizes on the complete TC1 marker after partial transmission loss. Duplicate timestamps do not renew freshness. Contradictory duplicates, counter rollback and implausible clock progression block further acceptance while preserving existing rows. Live device progression is compared with host monotonic time, allowing one second of buffering and drift before requiring explicit reconnect. This is a conservative local heuristic, not authenticated telemetry; a malicious serial device can fabricate internally plausible observations.

The app retains at most 600 snapshots. Export creates a new mode-0600 JSON file and refuses to overwrite an existing path. It includes author, provenance, freshness, parser errors, discarded bytes, warning state, field names and retained observations. Disconnect preserves provenance and data. Replay is explicitly labelled not live, requires a regular file no larger than 2 MiB, and rejects empty, corrupt or partially terminated recordings. No simulated data is presented as physical evidence. There is no backend, account, network telemetry or dependency installer hidden in the application.

## Reproduction and acceptance procedure

Run `make -C firmware all test` to build the Cortex-M4 image and exercise the real engine, clock and UART logic with host sanitizers. Run `xvfb-run -a python3 tests/test_app.py` for parser, export and graphical workflows using synthetic frames produced by the actual C encoder and Linux pseudo-terminals. Run `/usr/bin/python3 tools/check_hardware.py` for KiCad ERC, DRC and exact net connectivity comparison. Regeneration uses `/usr/bin/python3 tools/generate_hardware.py` with KiCad 9 pcbnew bindings and standard footprint libraries. Generated CAD files are already supplied; rebuilding them is not required just to inspect the project.

For physical bring-up, first inspect the net map and externally powered board settings against ST's manual. Test with isolated switches before installing water fixtures. Use a logic analyzer to compare qualified timing with the declared limits, deliberately stall acquisition under a debugger, unplug serial, restart the target, and verify every integrity warning. Then perform bucket-volume calibration and the blinded usefulness test described above. Do not fabricate or deploy without this independent electrical and mechanical review. Detailed executed software and CAD results belong in VERIFICATION.md; no automated result substitutes for these unperformed physical steps.

## References

- ST UM1724, Nucleo-64 MB1136 user manual, table 16 and USART solder-bridge configuration: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual, RCC/GPIO/USART/IWDG register definitions: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- Dual tipping-bucket rainfall prior art: https://link.springer.com/article/10.1007/s13143-022-00295-0
- Tipping-bucket uncertainty and calibration review: https://www.mdpi.com/1424-8220/23/12/5385
- Automated sprinkler performance testing using tipping gauges: https://doi.org/10.13031/2013.26410
- Optical drip-counter comparator, not this device's sensing principle: https://pmc.ncbi.nlm.nih.gov/articles/PMC9418544/
