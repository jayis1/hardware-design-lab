# Rebound Rungs — first-return height bands for a captive magnetic slider

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, tests and locally authored hardware sources are MIT licensed; see LICENSE. Installed KiCad footprint-library material retains its upstream licensing.

Revision A is an engineering prototype. The firmware is target-linked, the companion runs locally, and the custom carrier is routed and electrically checked. No assembly has been fabricated, flashed, mechanically commissioned, calibrated or physically tested. The processor, regulator, decoupling, programmer and USB interface are on a separately purchased NUCLEO-F401RE, not on the passive carrier. Passing software and CAD checks is not evidence of rebound accuracy or safe mechanical construction.

## Purpose, person and practical job

Rebound Rungs is a small comparative experiment for a model maker choosing a soft end-stop for a lightweight guided mechanism. A captive slider descends onto a replaceable elastomer coupon and returns upward. Four adjustable magnetic gate zones record the ordered descent, the first upward visits, and the repeated highest visited gate on the way back down. Instead of offering a suspiciously precise resilience percentage, the instrument reports an observed band: the slider returned through rung one without visiting rung two, for example. If it reaches the uppermost rung, the result is explicitly upper-censored. If a coherent first rebound cannot be observed, the result is invalid rather than silently zero.

The intended first user builds miniature sliding displays, teaching mechanisms or small hand-operated fixtures. Their question is not whether a material passes a published foam standard. It is whether a particular bumper, with a particular slider and guide, makes an inconveniently large first return. A workshop can compare two thicknesses without buying a full displacement acquisition system. The captured descent timestamps also expose grossly inconsistent releases; a run that approaches the bumper differently should not be pooled uncritically with the others.

The proposed contribution is the combination of a removable rung fixture, strict descending-order prerequisite, first-return bracket, and inspectable local evidence. Four closures on the way down are not themselves a result. The recorder requires a separately qualified bottom recrossing before accepting any upward evidence. A repeated highest gate closes the bracket; a skipped rung, a raw overlapping contact, an acquisition gap or an expired run produces a latched invalid trial. This observation contract is more useful than counting arbitrary magnet pulses.

A phone can film a bounce and may provide better spatial information. It cannot remain inside every guarded workshop fixture, and extracting repeated trajectories requires lighting, scale references, frame-rate knowledge and interpretation. This device instead leaves simple stationary switches in place and publishes a compact trace. The tradeoff is substantial: it only observes gates, it can miss unqualified pulses, and it relies on the geometry and single-slider assumption. Hardware earns its place only if that simpler repeated experiment proves more convenient than video after physical evaluation.

## Originality and prior art

Existing repository directories and README concepts were inventoried before implementation. Swing Envelope pairs pendulum periods with bilateral threshold visits. Tilt Break brackets a sled's departure while raising an incline. Rail Skew compares paired drawer stages. Rebound Rungs has a different experiment contract: one vertical descent, one bottom recrossing, and one first-return rung bracket, with terminal evidence frozen until hardware reset. It reuses jayis1's F401 clock, GPIO, UART, serial framing and passive-carrier infrastructure from pattern-sled at revision 8452cdb. Reuse of that electrical platform is explicit; the measurement engine and application model are new.

Rebound testers already exist. Public descriptions of the Bareiss Ball Rebound Tester and Weinmann's DIN EN ISO 8307 / ASTM D3574 equipment describe electronic rebound-height measurement, standardized specimens, and reporting of resilience values. Rebound Rungs does not invent rebound testing, replace that equipment, or implement those standards. Its captive slider introduces guide friction and a different impact geometry, and its output is deliberately a sequence-qualified band rather than a free-ball resilience percentage. The proposed original combination is an open, low-voltage, fixture-inclusive first-return notebook for small mechanisms. Worldwide uniqueness, patentability and absence of equivalent maker projects have not been established.

Prior-art references consulted during the design:

- https://www.bareiss-testing.com/product/ball-rebound-tester/
- https://www.weinmann-online.de/en/produkt/ball-rebound-tester-according-to-din-en-iso-8307-and-astm-d-3574/

Platform references for assembly review:

- ST UM1724, Nucleo-64 MB1136 board manual: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- ST RM0368, STM32F401 reference manual: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

The live ST manual download failed with an HTTP error and then a timeout during this run. Register definitions and connector assignments reuse the existing repository platform, rather than claiming a fresh manufacturer-document review. Check the exact purchased board revision and its virtual-COM solder bridges against the manufacturer's manual before assembly. Stock availability, switch sensitivity and component prices were not verified with suppliers.

## Hardware specifications and architecture

| Subsystem | Revision A contract |
| --- | --- |
| MCU | STM32F401RET6, Cortex-M4, on NUCLEO-F401RE MB1136 |
| Clock | Nominal 16 MHz internal HSI; no external oscillator required |
| Memory | 512 KiB flash, 96 KiB RAM on the purchased MCU |
| Inputs | Four normally-open, voltage-free reed contacts |
| Sensor example | Littelfuse MDSR-7 family, sensitivity selected by fixture experiment |
| Acquisition | One observation per 1 ms SysTick advance |
| Qualification | Raw vector unchanged for 3 ms; four observations minimum |
| Communication | USART2 TX, 115200 baud, 8N1, through ST-LINK USB VCP |
| Power | USB-powered Nucleo; carrier supplied from regulated 3.3 V |
| Carrier | 80 by 70 mm, two copper layers, 1.6 mm FR-4 |
| Assembly | Through-hole DIN0207 resistors and 2.54 mm headers |
| Application | Linux Python 3 with Tk, standard library only |
| Outputs | Serial evidence only; no actuator, brake, radio or charger |

The custom PCB contains fourteen components: eight resistors and six headers. Each input has a 10 kilohm pullup to VCC and a 1 kilohm series resistor between SENSE and the external contact. Contacts close to ground. At nominal 3.3 V a closed contact carries approximately 0.30 mA and the MCU sense voltage is approximately 0.30 V. Four closed channels draw approximately 1.20 mA through the resistor network, although overlapping gate zones are not a valid experiment. These are circuit calculations, not electrical measurements. Reserve a 5 V, 500 mA USB source for the complete development-board assembly; actual consumption was not measured.

    captive slider + retained magnet
             |
        rung 3 reed ------ J6 RUNG3 ----- 1k ----- SENSE3 / PB0
        rung 2 reed ------ J5 RUNG2 ----- 1k ----- SENSE2 / PA4
        rung 1 reed ------ J4 RUNG1 ----- 1k ----- SENSE1 / PA1
        rung 0 reed ------ J3 RUNG0 ----- 1k ----- SENSE0 / PA0
             |                                      |
        replaceable bumper                         10k each
             |                                      |
        insulating base                         3.3 V supply

    all reed returns -> GND -> Nucleo common ground
    USB -> NUCLEO-F401RE -> GPIO / SysTick -> first-return state machine
                        -> CRC snapshots -> PA2 USART2 -> ST-LINK VCP
                        -> Linux Connection / Crossings / Evidence screens

J1 pin 1 connects to Nucleo CN6 pin 4, 3V3; J1 pin 2 connects to CN6 pin 6, ground. J2 pins 1 through 4 connect respectively to CN8 pins 1 through 4: A0/PA0, A1/PA1, A2/PA4 and A3/PB0. Every external gate connector J3 through J6 has signal on pin 1 and ground on pin 2. Rung numbering increases upward. USART2 TX is PA2 alternate function AF7. There is no host command receiver. The carrier is wired alongside the development board; it is not mechanically an Arduino shield and must not be pressed onto unmatched headers.

## Mechanical design and measurement assumptions

Use an insulating vertical frame with a transparent guard and a permanently captive lightweight slider. A starting fixture envelope is a 300 mm high rail on a 150 by 150 mm base. Limit the initial moving mass to approximately 10 grams and the intended release travel to 150 mm or less. These are conservative commissioning dimensions, not a validated safety rating. Secure the frame against tipping, cover pinch locations, and provide an upper stop that prevents the slider from leaving the guide. The lower replaceable coupon sits on a rigid backing; do not hold it with fingers during a release.

Mount the four switches on slotted stationary carriers so their zones can be adjusted independently. A starting layout puts the switch centers approximately 20, 45, 70 and 95 mm above the uncompressed coupon surface. Place the magnet at a known fixed offset from the slider's impact face. Record that offset: switch-center height is not automatically slider clearance. The actual magnetic operate and release positions, rather than these nominal center dimensions, determine the observation bands. The lowest gate must be fully open before impact, and must close again only after the returning slider reaches its field.

The slider must move monotonically downward through all four zones before impact, then monotonically upward until its first apex, then downward again. No operator may touch it during the trial. The guide must prevent rotation from making the same magnet leave and reenter one field without crossing the intended height. A single sufficiently small retained magnet is required; multiple magnets, broad overlapping fields or loose components invalidate the sequence interpretation. A magnetic switch is an indirect proxy for position and cannot establish direction on its own.

At each gate, the closure and the open interval before the next visit must both survive qualification. Four samples spanning three milliseconds are the theoretical minimum; commission with measured pulses comfortably above ten milliseconds. Widening a magnetic field can improve pulse duration but risks overlap and increases spatial uncertainty. With no physical trajectory yet measured, there is no guaranteed operating speed. Use a scope or logic analyzer on SENSE nets and independent video to establish the usable range before comparing coupons.

A repeated gate means only that the contact opened, qualified clear, and closed again. A vibration-induced recrossing, guide wobble, missed higher gate or pulse too short to qualify can imitate a first apex. The firmware cannot disprove those explanations. Report the result as an observed rung bracket under the stated motion assumptions, not as a calibrated maximum height. Reaching rung three upward is lower-bound evidence only. Failing to return through rung zero within the deadline is invalid or unobserved rebound; it is not proof of zero bounce.

## Firmware behavior and implementation

The firmware is freestanding C11. It supplies a vector table, reset data/BSS initialization, linker script, direct STM32 register definitions, clock restoration with bounded waits, GPIO initialization, nonblocking UART transmission and independent watchdog service. There is no heap, vendor SDK, floating point, dynamic configuration or filesystem. The interrupt handler only advances the millisecond tick; the foreground loop owns the engine and transmitter. A failed clock setup stops before establishing a misleading serial interface. GPIO configuration and watchdog startup failures produce explicit fault states.

WAIT observes all-clear. Once raw and qualified vectors are both clear across 250 consecutive intervals, the engine becomes READY. Occupied gates at startup do not start a trial. The operator must position the slider above every gate before resetting and wait for READY. From READY the only accepted first closure is rung three. That event sets the relative time origin to zero. DOWN accepts two, one and zero in order. TURN accepts a second zero only after an intervening qualified clear; that establishes the beginning of the upward sequence.

UP accepts either the next higher rung or a repeat of the current highest rung. A higher visit extends the upward mask. Repeating the highest rung completes a bounded result, and visiting rung three completes an upper-censored result immediately. A direct nonzero-to-different-nonzero qualified transition has no clear separation and is invalid. Raw overlap is rejected immediately after arming, rather than debounced into an apparently valid order. Wrong-order events do not inflate the accepted crossing count.

Any sample interval other than one millisecond terminates correlation with a gap fault. Duplicate calls at the same timestamp are ignored. A trial reaches timeout when its age is at least 5000 ms; timeout takes precedence even if a completing contact qualifies on that exact observation. The duration is capped at 5000. DONE and FAULT latch measurement evidence permanently until reset; subsequent raw input and device uptime remain visible, but accepted timestamps, masks, result and trial age freeze. Transport-loss diagnostics can still increase and saturate at the unsigned maximum as a lower bound.

One trial per reset is intentional. Automatic rearming during manual lifting can mistake the return to the starting position for another release. Export first, lift the captive slider, reset the Nucleo and reconnect the companion. Reboot does not itself establish that the slider is correctly positioned; all-clear cannot distinguish above the top from between gates. The operator and the first upper-gate prerequisite provide the required setup contract. Firmware has no persistent run identity or storage, so loss of power erases evidence.

The encoder validates structural invariants before serializing: legal descending and ascending masks, accepted-event conservation, ordered timestamps, at least eight milliseconds between accepted closures, absence of timestamps for unvisited gates, and consistency between terminal state and final time. Eight milliseconds is the minimum closure-to-closure spacing when both a release and a new assertion require qualification. Sparse host snapshots are checked again for impossible time progression; these checks reject contradictions but are not a proof of every electrical sample that occurred between packets.

## Protocol and companion application

Every approximately 100 ms, firmware emits an ASCII RR1 record. There is no command channel or remote reset. A record begins with `RR1,`, contains canonical unsigned decimal fields separated by commas, and ends with `*HHHH` followed by LF. HHHH is uppercase CRC-16/CCITT-FALSE over every byte before the asterisk: polynomial 0x1021, initial 0xFFFF, no reflection and no final XOR. The CRC detects accidental corruption; it is not authentication. Any local process able to replace the serial stream can fabricate evidence.

Field order is fixed:

    now,state,raw,stable,qualified,faults,age,high,downmask,upmask,
    events,gaps,tx_errors,clear_ms,down0,down1,down2,down3,
    up0,up1,up2,up3,end_ms

State values are WAIT 0, READY 1, DOWN 2, TURN 3, UP 4, DONE 5 and FAULT 6. Fault bits are ordering 1, raw overlap 2, acquisition gap 4, timeout 8, input configuration 16 and watchdog initialization 32. Times are milliseconds; event times are relative to the first accepted upper-gate closure, not to impact. Masks distinguish an absent timestamp from legitimate zero. The transmitted `high` field is meaningful only with a nonzero upward mask, and a result requires DONE.

Launch `python3 rebound-rungs/app/workbench.py` from the repository root. The Connection screen opens a selected Linux serial device at 115200 8N1, preserves its original terminal attributes, and restores them on close. A second connection is refused while a port is owned. Failed connection or replay replacement leaves existing evidence intact. Use an operating-system serial-access group or narrowly scoped device permission instead of running the whole desktop application as root.

The Crossings screen displays descent and ascent times for every rung, clearly marks absent visits, and exposes state, raw mask, gaps, transmit losses and fault bits. Live status requires two advancing device timestamps, a connected port, no blocked session and recent device progress. Repeated identical timestamps do not renew freshness. A reception pause over 1.5 seconds, clock rollback, impossible phase transition or host/device accumulated progress difference over half a second blocks continued live interpretation until explicit reconnect. Long uncalibrated oscillator drift can trigger this conservative rule even with an otherwise healthy stream.

The Evidence screen exports a new private CSV file with author, fixture notes, source provenance, connection and freshness flags, parser-error count, dropped-row count, warnings and all retained snapshots. Existing files are never overwritten. A bounded queue retains the most recent 2048 snapshots; older rows are counted as dropped. Evidence remains available after disconnect. User-entered fixture notes are prefixed as text to avoid spreadsheet-formula interpretation. The export is not a raw RR1 recording and cannot be loaded as one.

Replay accepts regular files at most two MiB, rejects devices and FIFOs without blocking, and requires complete CRC-valid frames through end of file. It is always marked replay, never live. `python3 rebound-rungs/app/workbench.py --check capture.txt` validates such a recording without a display. Framing resynchronizes on the full RR1 marker after corruption or a truncated transmission, and diagnostics survive disconnect. No Wi-Fi credentials, cloud service, network listener, update channel or hidden telemetry are present.

## Build, verification and practical acceptance

From the repository root:

    make -C rebound-rungs/firmware all test
    xvfb-run -a python3 -m unittest discover -s rebound-rungs/tests -p 'test_*.py'
    /usr/bin/python3 rebound-rungs/tools/check_hardware.py
    python3 rebound-rungs/tools/check_quality.py

The target build requires GNU Arm Embedded tools and produces an ELF, binary and map in the ignored firmware/build directory. Host C tests use GCC with address and undefined-behavior sanitizers. The GUI tests use real Tk widgets under Xvfb and a Linux pseudo-terminal, not physical USB. The Python suite compiles the actual C engine and encoder to generate a synthetic RR1 fixture, then feeds that fixture through the real parser and model. Generated fixtures are not experimental measurements.

KiCad 9 and its system Python pcbnew binding are used for native CAD validation. The hardware checker runs schematic ERC, PCB DRC and exports a fresh XML netlist; it compares every reference, value, footprint and pin/net connection against the PCB and connectivity manifest. All carrier connections must be routed, with no unexplained errors or unconnected items. Passive-symbol ERC cannot validate the external development board, connector orientation, cable assembly, real contact behavior or supply voltage. The schematic is also rendered for visual inspection. The generator can recreate the design, but should not be run over manual edits without first reviewing its overwrite behavior.

Flashing is available with `make -C rebound-rungs/firmware flash` when a real Nucleo and OpenOCD are present. That command has not been exercised on hardware during this design run. Before flashing or connecting a carrier, inspect solder joints and polarity, check resistance from VCC to GND with power removed, and verify each input with an isolated jumper. Start without the slider. Then hand-pass the retained magnet slowly through each zone and verify the contact mapping independently. Only after electrical and mechanical checks should a guarded low-energy release be attempted.

## Estimated bill of materials and limitations

| Item | Quantity | Planning allowance, USD |
| --- | --- | --- |
| NUCLEO-F401RE development board | 1 | 18–30 |
| Two-layer carrier PCB | 1 from small batch | 4–12 |
| 10 kohm axial resistors | 4 | 1 |
| 1 kohm axial resistors | 4 | 1 |
| Two-pin 2.54 mm headers | 5 | 2 |
| Four-pin 2.54 mm header | 1 | 1 |
| Signal reed switches and insulating mounts | 4 | 4–12 |
| Retained small magnet | 1 | 1–3 |
| Short insulated leads and strain relief | set | 3–7 |
| USB cable | 1 | 3–6 |
| Guard, rail, slider and base materials | set | 15–40 |
| Replaceable coupon holders | set | 2–6 |

These figures are rough planning allowances rather than quotations; tools, labor, calibration references and shipping are excluded. A prototype may cost more than the materials suggest because a low-friction guided slider with well-behaved magnetic zones takes iteration. A purchased laboratory rebound tester is preferable when standardized material properties, traceable accuracy or documented compliance are needed.

The design has no isolation, ESD network, surge protection or protected power input. Connect only disconnected voltage-free contacts; never powered machine wiring, mains, industrial proximity sensors or wet electrodes. The 1 kilohm resistor is not a safety barrier. USB ground is shared with the host. An open sensor cable can look like a missed gate, and a short can look like an occupied gate; neither is uniquely diagnosed. The chosen low wetting current must be checked with the actual switches.

Open physical questions include guide friction, magnet/reed attraction, switch hysteresis, field overlap, repeatable release position, coupon movement, impact-face alignment and whether deliberate material changes produce distinguishable bands. Benchmark against independent video and a displacement reference before trusting any inferred geometry. Record invalid trials as well as completed ones. Do not compute a coefficient of restitution from these bands, do not equate absence of a visit with absence of motion, and do not use this prototype for safety acceptance or certification.
