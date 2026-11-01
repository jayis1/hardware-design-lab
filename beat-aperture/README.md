# Beat Aperture — paired pendulum return-window notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software, documentation and locally authored design sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom PCB is a routed passive carrier for a separately purchased NUCLEO-F401RE, not a standalone MCU board. Firmware target linking, synthetic acquisition tests, desktop operation and electrical CAD checks are reproducible. No board has been fabricated, target flashed, pendulum fixture assembled, contact calibration performed or actual synchronization measured. No patent novelty, precision timing, safety certification or commercial readiness is claimed.

## Purpose, user and moment of use

Beat Aperture helps a physics workshop instructor compare two slow demonstration pendulums without collapsing their relationship into two average periods. The specific question is whether each complete reference return contains no comparison return, exactly one, or more than one. When there is exactly one comparison return, the instrument retains its position within that reference window. When that assumption fails, it retains the failure of one-to-one pairing instead of selecting whichever comparison event happens to be nearest and presenting a misleading phase trace.

Imagine two small guarded pendulums suspended from a demonstration frame. An instructor adjusts one length slightly, releases both, and asks students to watch their relative timing drift. A stopwatch can establish average periods but cannot reliably record which individual reference cycles contained extra or absent comparison returns. Beat Aperture uses stationary magnetic contact zones on each pendulum. Its desktop notebook shows zero, single and multiple-return window totals, the most recently completed window, and any incomplete pending window. The instructor can repeat with another length or a removable weak coupling and compare exported electrical observations with synchronized video.

This is useful precisely because the output is limited. A window with one return does not establish entrainment, an energy-transfer mechanism or continuous phase locking. A window with zero returns can arise from slower motion, insufficient amplitude to reach a gate, a missed electrical pulse, or sensor disconnection. Multiple returns can be legitimate faster motion or a fixture problem. Those distinctions remain experiments for the teacher, not diagnoses asserted by software. The instrument records a reproducible event definition and makes missing pairing assumptions visible.

## Why hardware and what is different

A phone lacks four independently sampled dry-contact inputs and cannot remain fixed at four hidden gate positions while also serving as the classroom display. Video remains a valuable independent reference, but requires lighting, geometry and later annotation. A general-purpose logic analyzer could implement this experiment with custom analysis; the proposed contribution is the dedicated two-endpoint return contract, bounded reference apertures, explicit tie rule, retained multi-return evidence and local workflow. The passive electrical interface itself is intentionally conventional.

Before design, 127 existing top-level device README files were inventoried. Swing Envelope is the closest pendulum project: it measures one oscillator's directional crossings and outer amplitude thresholds. Beat Aperture instead uses two endpoint zones per oscillator and partitions completed B returns into A-return windows. Tip Cadence compares two tipping mechanisms and alternating interval cohorts; it does not establish cross-stream reference apertures with left-right-left return evidence. WebSlack Ledger compares signed strip travel, not periodic endpoint returns. These are functional differences, not just new names for an existing counter.

Other candidates were rejected: a single pendulum period display would be too close to Swing Envelope and ordinary laboratory timers; a generic two-pulse frequency counter would lose return direction; an automatic synchronization controller would introduce actuation without improving evidence quality. Worldwide absence of an equivalent instrument has not been established. This is an original proposed combination within this repository, not a claim that paired oscillators, reed switches or timing instruments were newly discovered.

Deliberately omitted features include motors, release solenoids, wireless pairing, cloud dashboards, automatic length adjustment, angle reconstruction, statistical significance claims and storage on the MCU. There is no safety output. A useful future acceptance experiment would compare every qualified window against manually annotated video over several deliberately different period ratios and report disagreements, including skipped gate visits. That physical experiment has not been performed.

## Hardware specification and architecture

The processor is the STM32F401RET6 on an ST NUCLEO-F401RE MB1136 board, with a Cortex-M4, 512 KiB flash and 96 KiB SRAM. This implementation runs from the internal 16 MHz HSI clock and uses GPIOA, GPIOB, a 1 ms SysTick, USART2 transmission and the independent watchdog. The purchased board supplies regulation, reset, decoupling, SWD programming and the ST-LINK USB virtual COM bridge. Those are not falsely depicted as components on the passive carrier.

The custom carrier is 80 by 70 mm, two copper layers, nominal 1.6 mm FR-4, with fourteen through-hole components. Each of four channels has a 10 kilohm pull-up from SENSE to 3V3 and a 1 kilohm series path from SENSE to a normally open dry contact. Closing the contact pulls its signal to ground. Headers use 2.54 mm pitch; resistors use DIN0207 axial footprints with 7.62 mm lead spacing. The board connects by short insulated wires and is not a mechanically compatible plug-on Arduino shield.

| Connector | Signal and target | Function |
|---|---|---|
| J1.1 | VCC to CN6.4 | Nucleo regulated 3.3 V only |
| J1.2 | GND to CN6.6 | Common return |
| J2.1 | SENSE0 to CN8.1 / A0 / PA0 | A_LEFT, bit 0 |
| J2.2 | SENSE1 to CN8.2 / A1 / PA1 | A_RIGHT, bit 1 |
| J2.3 | SENSE2 to CN8.3 / A2 / PA4 | B_LEFT, bit 2 |
| J2.4 | SENSE3 to CN8.4 / A3 / PB0 | B_RIGHT, bit 3 |
| J3, J4 | A_LEFT, A_RIGHT | Pendulum A gates |
| J5, J6 | B_LEFT, B_RIGHT | Pendulum B gates |
| Every J3–J6 pin 2 | GND | Contact return; pin 1 is signal |
| Nucleo PA2 | USART2 TX, AF7 | Onboard ST-LINK VCP |

```text
Pendulum A magnet -> left reed ---- J3 -> PA0
                  -> right reed --- J4 -> PA1
Pendulum B magnet -> left reed ---- J5 -> PA4
                  -> right reed --- J6 -> PB0

Per channel: 3V3 -- 10k -- SENSE -- 1k -- contact -- GND
                           |
                         GPIO

NUCLEO-F401RE
  reset HSI -> 1 ms sample -> independent five-sample qualifiers
                             -> two left/right return machines
                             -> B returns in (previous A, current A]
                             -> BA1 status + CRC -> USART2 TX
                                                       |
                                                ST-LINK USB VCP
                                                       |
Linux desktop: Connection | Return apertures | History / private CSV
```

The supplied KiCad project contains locally authored embedded resistor and connector symbols, real pin connectivity, footprint identities, routed copper, vias, silkscreen and a closed outline. `connectivity.json` is the component/pin manifest. `tools/check_hardware.py` exports a fresh schematic netlist and compares every pin, value and footprint identity with actual PCB pads, then runs ERC and DRC. Passive-only ERC does not validate the external Nucleo supply or your wiring harness. There are no mounting holes; use insulating edge supports rather than drilling through copper.

## Sensor mechanics and commissioning

Use two lightweight captive pendulums, each with a securely retained small magnet, and four normally open reed contacts in insulated stationary mounts. A suitable candidate family is the Littelfuse MDSR-7; sensitivity, magnetic geometry and reliable operation at this circuit's low wetting current must be checked before committing to a particular part. The glass envelope must not bear fixture loads. No universal angular dimensions are specified because switch operate/release fields depend on magnet orientation and distance.

On each pendulum place one gate left of center and one right of center, well inside the initial motion envelope. Their magnetic activation zones must not overlap. The apparatus should start with both bobs parked outside all four zones. Reset, verify RUN and zero contact mask, then release the bobs. The first left visit anchors that pendulum; a later right visit followed by another left visit completes a return. Starting by visiting right is allowed, but does not establish an anchor until left is seen. This deliberately discards a partial initial cycle.

A bob may encounter the same gate twice around an extreme, or linger near a zone. Repeated left visits without an intervening right do not complete a return and do not reset the age timer. Repeated right visits simply retain the right-seen state. Consequently the counted period is a left-right-left qualified visit sequence, not a promise that the bob moved monotonically between gates. If amplitude drops enough to miss an opposite gate, it eventually produces a timeout rather than an invented period. Record gate positions and initial release amplitude with every experiment.

Use slow demonstrations with contact dwell and open intervals comfortably longer than five sampled observations. A starting design target is a period above one second and contact plateaus above ten milliseconds; those are commissioning guidelines, not tested limits. Check each channel with an oscilloscope or logic analyzer and compare it with video. Magnetic attraction can alter motion, and reed hysteresis shifts the apparent crossing location. Swap sensors and repeat with the magnets removed to investigate disturbance before interpreting small differences.

Guard the swept area, keep leads outside it, and use low-mass bobs. Do not attach this prototype to valuable clocks, industrial machinery or public unguarded exhibits. Disconnect USB before wiring changes. Only isolated, voltage-free contacts are permitted: no powered Hall outputs, external logic voltages, existing clock drive circuits or mains connections. The carrier has no galvanic isolation, certified ESD protection, surge protection or battery charger. An open cable resembles an absent gate visit; a short resembles continuous occupancy. Firmware cannot uniquely distinguish either from physical behavior.

## Power, BOM and cost target

Power comes through the Nucleo's supported USB configuration. Connect no second supply to carrier VCC. Nominal current through a closed contact is 3.3 V / 11 kilohms = 0.30 mA. Its MCU sense level is 0.30 V. All four electrically closed channels draw 1.20 mA and dissipate 3.96 mW total in their resistor networks, although overlapping gates on one pendulum are rejected by the experiment. These are ideal resistor calculations, not measured tolerances or EMC results. Reserve 150 mA at USB 5 V for development-board planning and measure real consumption during bring-up. No battery-runtime assertion is made.

| Item | Quantity | Planning allowance in USD |
|---|---:|---:|
| NUCLEO-F401RE | 1 | 20–35 |
| Two-layer carrier PCB | 1 | 3–10 |
| 10k, 1 percent axial resistors | 4 | 1 total allowance |
| 1k, 1 percent axial resistors | 4 | 1 total allowance |
| 2-pin headers | 5 | 2 total allowance |
| 4-pin header | 1 | 1 |
| Reed switches with protective mounts | 4 | 8–20 total |
| Magnets, wiring, insulating supports | 1 set | 10–20 |
| Guarded two-pendulum teaching fixture | 1 | Reuse laboratory fixture; budget separately |

These allowances are not supplier quotations or stock checks. The electronics-and-sensors target is under USD 100 excluding the computer, fixture and assembly labor. This is a design budget, not a retail offer. Before connecting the MCU, inspect solder joints, resistor values, connector orientation and VCC-to-ground resistance. With power applied, verify every disconnected SENSE node is near 3.3 V and each deliberately grounded contact drives only its corresponding node low. Do not treat CAD rule checks as a substitute for those measurements.

## Firmware contract and failure behavior

`firmware/main.c` separates target acquisition from the pure C engine and wire encoder. `board.h` documents the pin contract; `registers.h` uses STM32F401 peripheral addresses. Clock, GPIO and UART drivers reuse jayis1's existing repository platform, while the return-aperture engine, protocol model and experiment tests are specific to this device. The freestanding build includes its own startup vectors, RAM initialization and linker script. No vendor SDK, heap allocation, floating-point runtime or network dependency is needed.

Each contact independently qualifies after five identical samples. A different raw value restarts its candidate count. This prevents chatter on B from indefinitely delaying a legitimate A event. The initial WAIT state requires all contacts qualified open before RUN. Thereafter a phase value of zero means unanchored, one means left visited, and two means a subsequent right visit. A left rising edge from phase two completes a return. Independent qualification describes electrical observations and cannot establish sub-millisecond physical order.

A's first left anchor opens an aperture. Every later A return closes it and opens the next. A completed B return belongs to the interval `(previous A anchor/return, current A return]`. Thus a B return qualifying simultaneously with an A return belongs to the closing window, with offset equal to its period. A B return simultaneous with the first A anchor is excluded because no prior aperture existed. B's first left anchor is never a completed B return. The ordering is implemented explicitly rather than depending accidentally on channel iteration order.

Each closed aperture increments exactly one of zero, one or many. The retained result includes period, hit count, first hit offset and last hit offset. A single-return window has equal first and last offsets; multiple returns retain their span without pretending to know intermediate positions from a summary. Pending hits and offsets remain separate from the last completed window. Cumulative assigned hits equal completed hits plus pending hits, and cannot exceed completed B returns. These identities are checked by both encoder and companion.

A return cycle must complete before 30,000 acquired milliseconds since its anchor or previous return. The exact deadline rejects even a return qualifying on that sample. Qualified simultaneous left and right occupancy on either pendulum terminates correlation. A missed sampling tick also halts; the code does not interpolate continuity across scheduling gaps. Configuration or watchdog startup failure is terminal. Diagnostic counters use checked increments and a staged engine copy, so overflow rolls back the entire sample rather than publishing half of a coincident completion.

HALT preserves historical and pending evidence. Device timestamps may continue advancing to announce the fault, but that is not new acquisition. Recovery requires an intentional MCU reset and a new all-open baseline; reconnecting the desktop alone does not clear firmware history. The independent watchdog is fed after acquisition; its restart erases volatile state. There is no flash log, nonvolatile session identity or remote reset command. Ordinary millisecond counter wrap is supported, but a reset can remain undetectable if its new observations happen to look monotonically plausible.

## Wire protocol and desktop software

USART2 transmits 115200 baud, eight data bits, no parity, one stop bit. Keep the MB1136 default VCP bridges: SB13 and SB14 on, SB62 and SB63 off. The firmware enables TX only. USB possession controls observation; CRC is corruption detection, not authentication. No security-sensitive actuator exists and there is no command channel, credential storage, pairing service or OTA updater. Programming uses physical ST-LINK/SWD.

Every approximately 100 ms the main loop sends `BA1,<fields>*<CRC>\n`. Decimal fields are unsigned and ordered as follows: now, state, levels, valid, faults, gaps, tx_errors, a_phase, a_age, a_cycles, b_phase, b_age, b_cycles, windows, zero, one, many, assigned, completed_hits, pending, first, last, period, last_hits, last_first, last_last. State values are WAIT=0, RUN=1 and HALT=2. Fault bits are gap=1, overflow=2, configuration=4, watchdog=8, overlapping gates=16 and timeout=32. The CRC is CRC-16/CCITT-FALSE with polynomial 0x1021 and initial 0xFFFF, covering bytes before the asterisk; four uppercase hexadecimal digits follow. There is no wire NUL terminator.

Run `python3 app/workbench.py` on Linux with Tk installed. Connection selects a serial path, opens it without sending commands, disconnects while retaining evidence, or loads a bounded recording. Return apertures shows classifications, latest period/offsets, pending hits and faults. History retains up to 4096 status rows, not a guaranteed complete event log: several windows can finish between transmitted snapshots. Cumulative totals retain those counts, while intermediate per-window details are unavailable. The GUI displays a single-return event fraction only when the latest hit count is one; it does not turn multiple hits into a fictitious phase angle.

Replay is strictly non-live and the provided fixture is explicitly synthetic. Files must be regular and at most 2 MiB; truncated tails, wrong versions, malformed decimals, bad checksums and impossible conservation are rejected. Live framing can recover from a partial line at the next complete BA1 marker and counts corruption. Duplicate timestamps do not renew progress; contradictory duplicates or regressing counters block a session. Sparse transition bounds detect some impossible histories but do not reconstruct all hidden edges or prove continuity across resets.

A one-second reception pause blocks further acquisition until reconnect. A fixed 1.5-second host/device elapsed-time mismatch also blocks accumulating backlog. The desktop deliberately says that acquisition age is unverified: receive-only telemetry cannot prove absolute sample age. Long sessions with oscillator drift may therefore require a conservative reconnect. Exports use exclusive creation with mode 0600 and preserve provenance, connection state, parser errors, warnings, receive age and retained rows. There is no upload or network listener. A failed replay or failed serial open preserves previous evidence.

## Build, validation and physical release gates

From this directory run `python3 tools/verify.py`. Required tools are GNU Make, host GCC, arm-none-eabi-gcc/binutils, Python 3 with Tk, Xvfb, KiCad 9 CLI and the distribution's pcbnew Python bindings. `make -C firmware all test` creates the target ELF/binary and runs sanitizer-backed engine and driver tests. `make -C firmware flash` is a separate intentional hardware programming action; it has not been run. Build output and KiCad preferences are ignored by Git.

The engine suite enumerates every seven-event word over four endpoint symbols against an independent alternating-endpoint/window oracle, and separately checks coincident completion, zero windows, qualification chatter, gaps, deadlines, wrap and transactional overflow. The app suite consumes the actual C encoder fixture and exercises CRC rejection, invalid snapshots, replay boundaries, private export, three-screen rendering and serial operation through a real Linux pseudo-terminal. These tests simulate contacts and serial traffic; they do not simulate magnetic physics or certify MCU timing.

Regenerate CAD only with `/usr/bin/python3 tools/generate_hardware.py` on a KiCad-equipped machine. Run `/usr/bin/python3 tools/check_hardware.py` afterward, export the schematic and inspect it visually. Rule checks must show zero violations and zero unrouted items; the netlist comparison must agree for all components and pads. Source attribution and substantive C-line/README-word counts are also automated. See VALIDATION.md for the executed results and remaining limitations.

Before fabrication, have a competent reviewer inspect the carrier and the actual Nucleo revision. Before claims about measurement, perform continuity, powered voltage, watchdog, serial loss, gate dwell, magnetic perturbation and video-correlation tests. Deliberately disconnect one contact and reduce amplitude to demonstrate the ambiguity between missing sensing and missing motion. Validate the classroom workflow with real users rather than assuming that a successful host test establishes practical usefulness.

## Technical references

ST UM1724, STM32 Nucleo-64 boards (MB1136), supplies the connector and VCP bridge contract: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf . The accessible Rev 13 copy at https://pages.hmc.edu/brake/class/e155/fa20/assets/doc/Nucleo-64_User_Manual_UM1724.pdf was checked for Table 16's F401RE connector assignments. The manufacturer PDF download timed out in this run; retrieved search excerpts also confirmed the VCP bridge settings.

ST RM0368 defines the STM32F401 peripheral register map: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf . ST AN4616 Table 8 corroborates the F401 GPIO, RCC, USART2 and watchdog peripheral bases. Direct RM0368 download also timed out; the implementation reuses the already present F401 platform and builds against its explicit definitions. Before physical bring-up, compare the complete register configuration and device datasheet with the actual purchased board; compilation alone cannot verify silicon behavior.
