# Key Escrow — key-retention sequence notebook for detached teaching mechanisms

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, companion software, documentation and locally authored electrical sources are MIT licensed; see LICENSE. Installed KiCad footprint geometry retains its upstream licensing.

Revision A is an unbuilt engineering prototype. The custom circuit is a routed passive contact carrier for an external NUCLEO-F401RE, not a complete MCU board. Firmware target linking, synthetic contact experiments, companion workflows and electrical CAD checks are recorded in VALIDATION.md. Fabrication, physical programming, switch adjustment, electrical bring-up, durability and actual workshop utility remain unverified. This device must never control, bypass, validate or certify a protective interlock. It has no actuator output and is intended only for a disconnected, hand-operated tabletop model without hazardous energy.

## Purpose and practical value

Key Escrow records the custody sequence of a removable key in a small mechanical access-lock demonstration. Four contacts report whether a sliding bolt is retracted, whether it is extended, whether the key is seated, and whether a lightweight flap is closed. The interesting result is not simply that a flap opened. It is whether a complete insertion, retraction, opening, closing, extension and key-removal sequence was observed, whether the operator returned the key without opening, or whether observation became inconsistent with the declared experiment.

An educational mechanism often looks correct when operated carefully but permits a different sequence when handled unexpectedly. A maker may adjust a cam so the key can be withdrawn slightly too early. A student may move a mock bolt before seating the key. A flap may be pulled open while the bolt endpoint witness is still traveling. The proposed instrument turns these observations into explicitly classified bench evidence, making it easier to compare successive low-energy model revisions without narrating each event into a camera.

The target audience is mechanical-design educators, exhibit builders and workshop instructors constructing transparent demonstrations of mechanical sequencing. A reasonable initial use case is a laser-cut acrylic enclosure containing a plastic bolt, a lightweight hinged flap and a captive-key mock cam. The observer records a lesson or prototype experiment while the mechanism remains entirely detached from any machine. It is not an industrial diagnostic attachment and should not be wired across existing protective switches. A certified safety device, professional risk assessment and applicable standards are separate topics outside this project.

The practical hypothesis is improved experimental bookkeeping. Contact data can distinguish returning an inserted key without opening from completing an actual open-and-close sequence. That distinction matters when comparing how intuitive a teaching model is. A simple cycle counter merges both actions. Video remains a useful complementary reference because this notebook cannot measure forces, identify a person, prove key identity or determine whether a bolt actually restrains the flap. The readout intentionally says “access cycle,” never “safe cycle.”

## Differentiation and prior art

The repository's top-level directories and device titles were inventoried before selection. Catch Witness studies settled-latch reopening and associates it with a release handle or declared tug. Clamp Relay studies whether material-holding witnesses overlap during a transfer between clamps. Vacancy Walk observes an empty pocket moving around a four-station ring. Key Escrow instead records a key-custody transaction spanning two bolt endpoints and an access flap, with prior-observation requirements at key release, flap opening and bolt extension. It does not reuse those devices' experiment logic or present their measurements under a new name.

The electrical carrier and Nucleo board support are intentionally reused from the inspected jayis1 platform in Vacancy Walk. A passive four-contact circuit is appropriate here; changing resistor networks only to make the hardware look different would add risk without improving the product. The original contribution is the specific bench workflow, independent qualification policy, transaction accounting, fault precedence, protocol and companion interpretation. This is a new repository concept, not a substantiated claim of worldwide novelty or patentability.

Trapped-key interlocking itself is established prior art. Fortress Safety's explanation describes key transfers enforcing a predetermined sequence, and IDEM's single-key bolt interlock description specifically requires a closed guard and extended bolt before key release. Those products are real protective components with application requirements that this prototype does not meet. Key Escrow is proposed as a noncontrolling experimental notebook for detached teaching models, not an inexpensive replacement for them. Before any commercialization, compare it against a four-channel logic analyzer and annotated video, interview instructors, and perform a professional prior-art review.

References used for the design:

- Fortress Safety, Understanding Trapped Key Systems: https://fortress-safety.com/news/understanding-trapped-key-systems/
- IDEM M-BS single-key bolt interlock: https://idemsafetyusa.com/product/m-bs-bolt-interlock-single-key-in-mirror-polished-die-cast-metal/
- ST Nucleo MB1136 manual UM1724: https://www.st.com/resource/en/user_manual/um1724-stm32-nucleo64-boards-mb1136-stmicroelectronics.pdf
- Distributor-hosted UM1724 copy, whose indexed connector table was available when the ST download failed: https://www.farnell.com/datasheets/1948962.pdf
- STM32F401 reference manual RM0368: https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf

## Hardware specifications

| Subsystem | Revision A choice |
|---|---|
| Processor assembly | NUCLEO-F401RE, MB1136, STM32F401RET6 |
| CPU and storage | Cortex-M4, nominal 16 MHz HSI, 512 KiB flash, 96 KiB SRAM |
| External sensors | Four normally-open, low-level, voltage-free contacts |
| Suggested sensor construction | Four protected reed contacts with mechanically captured magnets |
| Candidate reed | Littelfuse MDSR-7-10-15; verify current availability and sensitivity before procurement |
| Sampling | One acquired contact vector per nominal millisecond |
| Qualification | Five consecutive identical samples independently per channel |
| Baseline | Fifty clean observed intervals, key absent, bolt extended, flap closed |
| Trial bound | 60000 acquired milliseconds; deadline equality rejects completion |
| Communications | USART2 TX, PA2 AF7, 115200 baud, 8N1 through ST-LINK VCP |
| Carrier PCB | 80 × 70 mm, two copper layers, nominal 1.6 mm FR-4 |
| Power | USB to Nucleo; regulated 3.3 V and ground from Nucleo to carrier |
| Companion | Python 3 with Tk on Linux; standard library only |
| Actuation, RF, batteries | None |

The external development board supplies the MCU, regulation, reset, processor decoupling, debugger, USB bridge and programming connection. The KiCad schematic is a complete representation of the passive carrier, not a misleadingly incomplete processor board. The carrier uses a short harness and is not a mechanically mating Arduino shield. Maintain the Nucleo's default ST-LINK virtual-COM routing. Check the actual board revision, connector orientation and solder bridges against UM1724 before applying power.

Each channel has a 10 kΩ pull-up between VCC and SENSE and a 1 kΩ series resistor between SENSE and the external contact signal. The contact closes its signal to ground. At nominal 3.3 V and nominal resistor values, a closed contact draws 0.30 mA and produces 0.30 V on SENSE. Four closed contacts draw 1.20 mA, approximately 3.96 mW in the carrier network. These are ideal calculations, not measurement results or guaranteed thresholds. Select contacts suitable for such low excitation; a large power-switch current rating does not establish dependable low-current operation.

Allow a conventional 5 V, 500 mA USB source budget for the complete development-board assembly. Actual current is not measured. The carrier accepts only the Nucleo's regulated 3.3 V; do not attach a separate supply. All contacts share USB host ground. There is no isolation, surge qualification, reverse-polarity protection or field-wiring interface. The series resistors do not make powered signals safe. Keep the first prototype's leads shorter than half a meter, pair each signal with its return, and provide strain relief at both ends.

### Wiring and bill of materials

| Carrier connection | Nucleo or external meaning |
|---|---|
| J1.1 VCC | CN6.4 3V3 |
| J1.2 GND | CN6.6 GND |
| J2.1 SENSE0 | CN8.1 A0, PA0, retracted bolt witness |
| J2.2 SENSE1 | CN8.2 A1, PA1, extended bolt witness |
| J2.3 SENSE2 | CN8.3 A2, PA4, key-seated witness |
| J2.4 SENSE3 | CN8.4 A3, PB0, flap-closed witness |
| J3.1 / J3.2 | RETRACTED contact signal / ground |
| J4.1 / J4.2 | EXTENDED contact signal / ground |
| J5.1 / J5.2 | KEY_SEATED contact signal / ground |
| J6.1 / J6.2 | FLAP_CLOSED contact signal / ground |

The electrical BOM is one external NUCLEO-F401RE, four 10 kΩ resistors, four 1 kΩ resistors, five two-position 2.54 mm headers, one four-position 2.54 mm header, four external contacts, harness wire and the PCB. Use axial resistors with 7.62 mm lead spacing and appropriate through-hole headers matching the assigned footprints. The manifest kicad/connectivity.json enumerates every carrier component, pin and net. Magnets, reeds, Nucleo and the mechanical model are external assemblies rather than hidden unconnected PCB components. A USD 50–100 materials allowance is a planning estimate, not a supplier quote or availability guarantee.

## Architecture and mechanical fixture

    Detached, hand-operated key-and-flap teaching model
        bolt retracted ---- J3 ---- series/pull-up ---- PA0
        bolt extended ----- J4 ---- series/pull-up ---- PA1
        key seated -------- J5 ---- series/pull-up ---- PA4
        flap closed ------- J6 ---- series/pull-up ---- PB0
                                                       |
    USB supply -> Nucleo 3V3 -> carrier                 |
                                             independent qualifiers
                                                       |
                                           key transaction observer
                                                       |
                                             bounded KE1 encoder
                                                       |
                                      USART2 TX -> ST-LINK USB VCP
                                                       |
                                           Linux local workbench
                                  Sequence / Trace / Wiring / CSV

A starting fixture can use a 200 × 150 mm nonconductive base, an approximately 70 × 70 mm lightweight flap, and a hand-slid plastic bolt with roughly 20 mm travel. These are prototype layout assumptions, not production drawings or validated tolerances. Position the two bolt witnesses so they cannot both close at any legitimate endpoint. The between-endpoints region intentionally closes neither witness. A magnetic key token can actuate the key-seated contact in a recess, but its presence is not an authenticated key code. Fit mechanical retainers so magnets cannot detach or become a swallowing hazard.

Place the flap sensor near its closing stop and keep the key sensor sufficiently far from bolt magnets to avoid cross-talk. Before relying on the software, move each component separately through its complete range and inspect all four raw bits. A seated key should change only bit 2; an open flap should clear only bit 3. Tune magnet spacing experimentally. Reed sensitivity is specified in magnetic terms, not as a guaranteed distance for this enclosure. Protect glass reed bodies from bending loads and use insulated mounts that do not let wires become mechanical stops.

The model may deliberately allow invalid actions so students can observe why a sequencing mechanism needs those constraints. Do not modify a commercial protective device to create such demonstrations. Use inert components, rounded edges and a lightweight flap that cannot trap a finger with significant force. This PCB has no mounting holes; retain its edges in an insulating enclosure or use nonconductive clips outside copper clearances. Do not drill holes through routed areas. Keep the USB cable outside the hand-motion area and label the complete fixture “BENCH MODEL — NO PROTECTIVE FUNCTION.”

## Firmware behavior and design decisions

Firmware is freestanding C11 with explicit startup, vector table and linker memory map. It restores the internal 16 MHz HSI clock using bounded readiness waits, configures GPIO as externally biased inputs, and enables only USART2 transmission. The production drivers contain nonblocking buffered UART draining, a transmission timeout, a millisecond SysTick counter and an independent watchdog. Register definitions use the STM32F401 peripheral map rather than invented addresses. The compiler links an actual Cortex-M4 ELF and binary; host tests separately substitute register storage to exercise configuration and timeout branches.

The four bits are independent qualifiers. A different channel's chatter cannot postpone a key removal indefinitely. Each channel needs five consecutive observations with the same value before becoming valid or changing its qualified value. Pulses shorter than that may disappear entirely from qualified evidence. Raw guards at authorization boundaries improve conservatism but do not turn sampled contacts into continuous mechanical truth. Timestamp resolution and accuracy are separate: milliseconds are nominal HSI-derived units, and physical clock accuracy has not been calibrated.

The observer starts in WAIT. It ignores arbitrary startup mechanism positions until all channels are valid and both raw and qualified masks equal 10: bolt extended, flap closed, key absent, bolt not retracted. Fifty intervals with that condition at both endpoints establish READY. The first eligible endpoint receives no preceding-time credit. A raw disturbance resets baseline credit even if it never qualifies. This startup rule establishes an observed experimental baseline, not proof that a real machine was isolated.

In READY, qualified key insertion starts a trial only while both current and previous qualified observations, and the current raw observation, show the release position. A bolt or flap movement without the key instead halts as unkeyed movement. In ACTIVE, the bolt may pass through neither endpoint while the flap remains closed. Opening requires a previously qualified retracted bolt and seated key, together with corresponding current raw and qualified witnesses. Retraction and opening on the same qualification sample do not establish the required order and are rejected.

A trial can contain more than one opening and closing while the key remains inserted. The opened flag records whether any eligible opening occurred, not the number of opening episodes. Extending the bolt requires the flap to have been closed in the preceding qualified observation; simultaneous flap closing and bolt extension is rejected. Removing the key completes only with the release position already established, still qualified, and present in the raw vector. A trial that included opening increments completed; one that never opened increments canceled. These are mutually exclusive classifications, not judgments about human behavior.

Every started trial has exactly one eventual outcome or is currently active: starts equals completed plus canceled plus aborted plus the ACTIVE indicator. Invalid sequences abort the active transaction and enter terminal HALT. Contradictory bolt endpoints have priority over key-removal interpretation. Key removal then precedes opening checks; opening eligibility precedes extension-while-open classification. The 60000 ms deadline is checked before all same-sample completions, so completion at equality is rejected. A trial completing at 59999 ms remains eligible. The last duration freezes at the latest outcome, while the opened flag remains associated with the latest or current trial.

Any acquisition gap, including one before arming, enters HALT instead of pretending that unseen samples were stable. Arithmetic supports ordinary 32-bit millisecond rollover; sequence exhaustion and bounded counter exhaustion are terminal. Once halted, evidence fields and acquisition sequence freeze, while wall-clock timestamps and transport-error diagnostics may continue. No automatic baseline recovery can erase the reason for stopping. Restore the model to its baseline, export retained evidence, and reset the Nucleo to begin a new volatile experiment. Resetting erases the previous experiment; there is no flash log or persistent boot identity.

## Application and wire interface

Run `python3 app/workbench.py` from this directory on a Linux desktop with Tk installed. Enter the serial device path, normally /dev/ttyACM0, and choose Connect. Grant device access through the operating system rather than running the application as root. Only one connection is owned at a time. Opening a replacement while a port is already open is rejected, and a failed initial open preserves existing evidence. The serial adapter restores saved terminal settings when closed, handles disconnect notifications, and never transmits a device command.

The Sequence screen presents phase, retained result, cycle totals, trial age, last duration, contact vectors and opening evidence. Trace shows the most recent 512 accepted snapshots with acquisition sequence and device time. Wiring and limits provides connector meanings and the experiment contract. Replay KE1 loads a bounded regular file through exactly the same CRC and semantic checks used for live input. The generated fixture recording is explicitly synthetic. Export CSV creates a new private file containing author, provenance, connection/freshness state, parser errors, warning and retained rows; it refuses to overwrite an existing path.

The protocol is ASCII: marker KE1 followed by comma-separated unsigned decimal fields, an asterisk, four uppercase hexadecimal CRC digits, and LF. CRC is CRC-16/CCITT-FALSE with polynomial 0x1021 and initial value 0xffff, covering the marker and decimal body but not the asterisk. It detects ordinary corruption, not intentional modification. Field order is sequence, now, state, result, fault, raw, qualified, valid, opened, starts, completed, canceled, aborted, age, last_ms, baseline, tx_errors, gaps. State codes are WAIT=0, READY=1, ACTIVE=2, HALT=3. Exact result names and codes are shared by firmware/engine.h and app/model.py.

Periodic snapshots are emitted at nominal 100 ms intervals, not on every contact edge. The sequence advances per acquired sample, so sequence jumps normally represent unreported samples rather than lost serial packets. The companion checks field bounds, conservation, plausible time progress, monotonic counters, latest-outcome relationships and frozen halted evidence. Those are necessary sparse-history bounds, not a reconstruction or cryptographic proof of the unseen path. Device firmware performs the complete local sample-by-sample classification. The marker identifies this fixed policy revision; it is not a boot notification and never clears session evidence.

Freshness requires a live connected port, at least one advancing acquisition after the initial frame, no latched session warning, and progress within one host second. Duplicate frames do not renew freshness. A fixed two-second cumulative host/device timing tolerance rejects large stalls and gradually accumulating backlog once the tolerance is exceeded. It also means oscillator drift during a long session can conservatively require reconnection. Reconnection flushes queued input but cannot authenticate a boot or establish absolute sample freshness. Some resets with forward-looking values remain indistinguishable without a persistent boot identifier; do not merge unrelated experiments on that basis.

Replay rejects FIFOs, devices, empty input, oversized files and incomplete final frames. Live framing can discard a partial attachment prefix and recover at a complete KE1 marker, recording the parser error. All evidence remains local, with no telemetry service, network listener, pairing secret or account requirement. A malicious local process can still forge a recording or serial stream. The application is an engineering convenience, not a trusted audit system. CSV retains metadata after disconnection so an offline export is not accidentally represented as fresh live acquisition.

## Build, assembly and verification

From the device directory run `make -C firmware all test fixture`. This links the target image, runs sanitizer-backed host tests and creates firmware/build/demo.ke1 from the real C encoder. Run `xvfb-run -a /usr/bin/python3 -m unittest discover -s tests -p 'test_*.py'` for parser, model, serial and real Tk event-loop tests. The GUI tests use Linux pseudoterminals, not a physical Nucleo. No third-party Python package download is required. The host needs GCC, GNU Arm Embedded tools, Tk, Xvfb and KiCad 9 with its Python bindings for the complete development checks.

To regenerate the carrier, run `/usr/bin/python3 tools/generate_hardware.py`; to validate it, run `/usr/bin/python3 tools/check_hardware.py`. The checker runs schematic ERC and PCB DRC, exports a fresh XML netlist, and compares every reference, pin, net, component value and footprint identifier against the board and manifest. Real symbols and routed tracks are present. Passing these checks does not verify the external power source, solder quality, harness assignment, enclosure fit, contact physics or regulatory compliance. Render and inspect the schematic after any placement change.

For first assembly, inspect the unpowered carrier for shorts, resistor values, connector orientation and continuity. Verify the harness against the table rather than relying on wire colors. Attach only the Nucleo and measure carrier supply voltage before connecting the model. Close each sensor individually and check the corresponding raw bit and measured input voltage. Confirm that no physical position produces both endpoint witnesses. Only after those checks should the flashed observer be used for scripted nominal and intentionally invalid low-energy trials.

The provided flash target uses OpenOCD with ST-LINK and STM32F4 configuration files, programs the ELF, verifies it and resets the target. Physical flashing was not performed in this design run. If startup produces no serial output, verify power, board type, USB enumeration, default VCP routing and baud rate before changing firmware. WAIT that never arms usually means a baseline wiring or qualification problem. HALT must be investigated using its retained reason, not bypassed by weakening guards. Prototype testing should include contact bounce, unplugged leads, deliberate sampling overload, repeated resets and independent video correlation before anyone relies on the notebook for experiments.
