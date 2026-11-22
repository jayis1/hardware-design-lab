# Ghost Grid — independent-contact phantom-key topology notebook

Author and creator: jayis1

Copyright (c) 2026 jayis1. Original firmware, application, documentation and design sources are MIT licensed; see LICENSE. The board support, startup, UART, passive carrier generator and serial transport reuse jayis1's Sleeve Order platform at commit 97a09650408803eb8b24ab908ba9ded367c65041. The topology model, exposure accounting, GG1 protocol, desktop screens and experiments are specific to Ghost Grid. Installed KiCad footprint geometry retains its upstream licensing. The overwrite-refusing import script records exactly which platform files were reused and is not an ordinary build step.

Revision A is an unbuilt engineering prototype. Its custom PCB is a passive interface for a separately purchased NUCLEO-F401RE, not a standalone MCU motherboard. Target linking, host tests, desktop exercises and native CAD validation are reproducible. Fabrication, flashing, electrical performance, mechanical behavior and usefulness have not been physically demonstrated. This is not a keyboard controller, accessibility product, safety monitor or medical instrument.

## Purpose and practical use

A small handmade button panel can behave perfectly when each button is tried individually and still produce unexpected keys when three buttons are held. Testing through an operating system often obscures the cause: a keyboard controller may suppress ambiguous combinations, apply debounce, discard events or synthesize its own output policy. Ghost Grid records four independent contact closures before those policies exist and shows what an ideal diode-less two-row, two-column scanner would infer from the same physical combination.

The proposed device is a local workbench for a maker developing an interlocked macro pad, a teacher explaining matrix ambiguity, or a designer deciding whether mechanical exclusion of certain combinations actually holds during slow operation. Four removable switches are arranged as R0C0, R0C1, R1C0 and R1C1. Each is electrically independent and has its own input. The displayed matrix is a model computed from those observations; the fixture must never be connected to an already wired or powered keyboard matrix.

The useful question is not whether matrix ghosting exists. That is established circuit behavior. The question is which otherwise invisible three-corner combinations a particular detached mechanism actually visits, for how many clean sampled intervals, and which missing corner the ideal scanner would falsely report. A sliding cover that nominally blocks one button may still allow a different triple while moving between positions. The instrument retains separate evidence for every physical combination, including all-four closure, instead of collapsing everything into a single generic error count.

An example experiment compares two removable exclusion plates on the same low-energy button jig. Begin with the switches released, reset the observer, connect the application and exercise the specified slow motions. Export the first session before changing the plate. Repeat the same sequence with the other plate and preserve unknown-time and missing-sample counts alongside the exposure histogram. Do not equate fewer predicted-phantom intervals with a certified improvement. Check with video or a reference logic analyzer whether changing the plate also changed contact force, bounce or achievable motion speed.

The intended result is an inspectable early design record before selecting a keyboard controller. A camera, logic analyzer or an actual diode-equipped controller may be a better tool for fast typing, scan-order effects or real host behavior. This instrument does not inject keys, observe typing text, identify a person, score dexterity or establish that a particular assistive interface is suitable for anyone.

## Originality, prior art and scope

All existing device directories and README titles were inventoried before selecting this concept. The closest purpose documents, Contact Atlas and Chord Window, were read. Contact Atlas discovers detached rotary-switch transition graphs. Chord Window measures four-switch press, overlap and release timing for retrospective chord policies. Neither computes a row/column reachability model, retains missing-corner exposure by physical topology, or distinguishes a truly closed fourth corner from a predicted extra corner. Reusing their general electrical approach is deliberate; four pullups are not presented as a new invention.

The bounded external comparison found Microchip AN3407, “Using Matrix Keypad with AVR Devices,” QMK's matrix explanation, and Dave Dribin's “Three Simultaneous Key Presses and Ghosting.” The last two explicitly illustrate three pressed corners creating an apparent fourth corner. Accordingly, no claim is made to have discovered ghosting, to be globally first, to have patentable circuitry, or to have established freedom to operate. The proposed contribution is the independent-contact hardware notebook and its conservative coverage semantics for a detached mechanism.

References:

- https://ww1.microchip.com/downloads/en/Appnotes/00003407A.pdf
- https://github.com/qmk/qmk_firmware/blob/master/docs/how_a_matrix_works.md
- https://www.dribin.org/dave/keyboard/html/ghosting.html
- https://www.st.com/resource/en/user_manual/um1724-.pdf
- https://www.st.com/resource/en/reference_manual/rm0368-stm32f401xbc-and-stm32f401xde-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
- https://estore.st.com/en/nucleo-f401re-cpn.html

The ST manual search result provided UM1724 Rev 17 table 16 and the USART solder-bridge description, confirming CN6/CN8 mapping and the default virtual-COM arrangement. The ST store result listed NUCLEO-F401RE in stock during research; this is not a reservation or a lasting availability guarantee. Full-page extraction was unavailable with the configured extractor. The register implementation reuses the existing RM0368-based platform rather than claiming a fresh exhaustive register audit. Check the actual purchased board revision before assembly.

## Hardware specifications

| Function | Revision A implementation |
|---|---|
| Processor | STM32F401RET6 on NUCLEO-F401RE, MB1136 family |
| Core and memory | Cortex-M4; 512 KiB flash; 96 KiB SRAM |
| System clock | Internal HSI, nominal 16 MHz |
| Acquisition | Four low-active digital inputs, nominal one observation per millisecond |
| Sensor type | Detached normally-open, voltage-free signal contacts |
| Qualification | Whole-vector stability for five unchanged one-millisecond intervals |
| Communications | USART2 PA2 AF7, 115200 baud, 8N1; ST-LINK virtual COM |
| Power | USB to Nucleo; Nucleo regulated 3.3 V to carrier |
| Custom carrier | 80 × 70 mm, two copper layers, 1.6 mm FR-4 |
| Assembly | Through-hole axial resistors and 2.54 mm headers |
| Companion | Linux/POSIX Python 3 with Tk, standard library only |
| Outputs | Receive-only host telemetry, no actuator or HID function |

The purchased board supplies the regulator, MCU decoupling, debugger, USB bridge, reset and processor. None of these is silently omitted from an allegedly complete custom processor board: they are explicitly outside the carrier boundary. The passive board contains fourteen components, thirty pads and ten named nets, with actual routed copper and footprint identities. The schematic's all-passive ERC cannot validate the external development board, wiring harness or power source.

### Connector contract

| Bit weight | Physical key | Field connector pin 1 | J2 pin | Nucleo destination |
|---|---|---|---|---|
| 1 | R0C0 | J3 / KEY_00 | 1 | CN8.1, A0, PA0 |
| 2 | R0C1 | J4 / KEY_01 | 2 | CN8.2, A1, PA1 |
| 4 | R1C0 | J5 / KEY_10 | 3 | CN8.3, A2, PA4 |
| 8 | R1C1 | J6 / KEY_11 | 4 | CN8.4, A3, PB0 |

Each field connector pin 2 is ground. J1 pin 1 connects to CN6.4, regulated 3V3, and J1 pin 2 connects to CN6.6, ground. Do not substitute the adjacent 5 V header position. J2 carries only sensing nodes, not an alternative powered bus. The header numbering must be established by continuity measurement and the actual board drawing, not guessed from a photograph of the opposite side.

ST-LINK virtual COM requires SB13 and SB14 fitted and SB62 and SB63 open, as described in UM1724. USART receive is not enabled. PA2 is the only actively driven application interface. Internal input pulls are disabled because the external resistor network defines the bias. The four analog-header pins are used as digital inputs, not ADC measurements.

### Electrical limits and BOM

Each path is 3.3 V through 10 kohm to SENSE, through 1 kohm to the contact, then ground. Nominal closed current is 0.300 mA; nominal MCU-side low voltage is 0.300 V. Four closed channels draw 1.20 mA and consume 3.96 mW in the resistor networks. These are calculations, not measured limits. Development-board consumption dominates; reserve a conventional protected 5 V USB supply capable of 500 mA rather than claiming battery life or measured operating current.

Populate R1, R3, R5 and R7 with 10 kohm, one-percent resistors, and R2, R4, R6 and R8 with 1 kohm, one-percent resistors. The footprints are DIN0207 axial parts with 7.62 mm lead spacing; use parts rated at least 0.125 W. J1 and J3 through J6 are two-pin headers; J2 is four-pin. The routing uses 0.30 mm tracks, nominal 0.20 mm clearance, and through vias of 0.80 mm diameter with 0.40 mm drills. Obtain fabrication tolerances from the selected board house rather than assuming every supplier supports the same stackup.

External items are one Nucleo, one USB data cable, four signal-rated contacts, insulated wiring, a captive switch fixture and an insulating case. A low-current gold-contact pushbutton is appropriate; an oxidized power switch that requires substantial wetting current is not automatically suitable. Confirm the chosen component's low-level switching rating, actuation force and mounting geometry. No particular switch inventory, enclosure CAD or mechanical tolerance stack has been qualified by this release.

The carrier has no isolation, reverse-polarity protection, TVS network, battery or charger. Series resistance is not protection against external powered wiring. Use dry indoor conditions and short paired signal/ground wires, initially below half a meter. Disconnect every switch from all other circuits before attaching it. USB ground becomes common with the entire fixture. An open cable looks like a released key, and a short looks like a pressed key; no software algorithm can reliably distinguish them here. The PCB has no mounting holes; retain it with insulating edge rails and do not drill through existing copper.

## Architecture and block diagram

    Four independent detached switches
      R0C0    R0C1    R1C0    R1C1
        |       |       |       |
       J3      J4      J5      J6       pin 2 on each -> GND
        |       |       |       |
       1k      1k      1k      1k
        |       |       |       |
       SENSE0..3 -------------------- J2 -> PA0/PA1/PA4/PB0
        |       |       |       |
       10k     10k     10k     10k
        +-------+-------+-------+----- J1 -> Nucleo 3V3

    USB -> Nucleo -> 1 ms observation -> whole-vector qualifier
                                          |
                              clean-interval topology histogram
                                          |
                               GG1 snapshot plus CRC-16
                                          |
                             USART2 -> ST-LINK virtual COM
                                          |
                         local desktop grids / histogram / CSV

The circuit does not physically scan rows and columns. That distinction is essential: connecting the four inputs into a shared row/column network would destroy the independent ground-truth observation. The software creates a hypothetical matrix from the four independently observed closures. Its ideal scanner activates one row while leaving the other row high impedance and reads columns through an ideal conducting contact network. Push-pull inactive rows, diode polarity, contact resistance, capacitive charging, leakage, finite settling and real keyboard firmware are not modeled.

For zero, one or two physical closures, the predicted mask equals the physical mask. Any three closures join the four row/column vertices, so the predicted mask is fifteen and the missing corner is a phantom. With all four closures the predicted mask is also fifteen, but there is no additional phantom because every inferred key is actually closed. All-four observations remain scanner-ambiguous relative to three-key alternatives; “no extra phantom” must not be mislabeled “uniquely decodable.” This small truth table is independently tested against graph reachability rather than merely repeating the implementation's population-count rule.

## Firmware behavior and evidence contract

Firmware is freestanding C11. Startup installs the Cortex-M vector table, initializes data and BSS, and uses the linker script's flash origin 0x08000000 and RAM origin 0x20000000. Clock restoration explicitly selects HSI and clears prescalers with bounded pre-tick iteration waits. GPIO configuration is checked before observations. The watchdog uses the independent low-speed source; UART transmission is nonblocking and aborts a pending transfer after 100 nominal milliseconds. No vendor SDK or network fetch is required for the build.

Main observes each changed millisecond tick once. It never fills missing observations by repeating the newest switch state. The first acquired vector seeds a candidate. Five subsequent unchanged observations qualify that vector. Thus the sixth observation is the first qualified endpoint; the seventh can credit the first clean interval. A histogram interval is credited only when both its endpoints are qualified, raw-equal and describe the same mask. Every other acquired observation increments unknown. Startup observations therefore belong to unknown rather than being retroactively assigned to idle.

A raw change immediately invalidates current qualification, seeds a new candidate, and breaks the current segment. Even a one-sample glitch that never qualifies splits the segment. Changing back to the earlier raw mask still requires the whole qualification window. The qualifier treats all four contacts as one topology; this deliberately avoids constructing a combination from independently delayed keys. Conversely, chatter on one contact makes the entire topology temporarily unknown. That is a coverage tradeoff, not a claim that the other contacts stopped existing.

For every mask, the engine retains total clean intervals, number of clean segments and longest segment. Current run length is retained separately. These are observed segments, not distinct physical key presses or ghosting incidents: contact chatter and acquisition gaps can split one continuous physical episode. Histograms use nominal millisecond intervals, not calibrated time. The sum of all sixteen duration bins plus unknown equals acquired sample count. Missing intervals after the first acquisition are counted separately and never contribute to either histogram or unknown. Time before the first acquired sample is outside coverage.

On a sampling gap, the current observation becomes unknown, missed grows by elapsed ticks minus one, and qualification restarts. Previously accumulated exposure stays intact. Counter updates are transactional: if any new increment would overflow uint32, no part of that observation is committed and the engine enters terminal overflow fault. Configuration failure, watchdog startup failure, invalid input and impossible time progression also halt accounting. Fault bits are respectively 1, 2, 16 and 4; overflow is 8. Faults are sticky. In HALT only status time and saturating transport-error count may advance; old measurements remain historical, not live acquisition.

A reset starts a new volatile history. There is no boot identifier, persistent log or signed update mechanism. The host detects many resets from counter or timestamp rollback but cannot prove that every apparently forward stream belongs to the same uninterrupted boot. Reset the fixture deliberately before a new experiment and reconnect explicitly. Flashing is local through ST-LINK/SWD under the operator's physical control. There is no remote update endpoint or unauthenticated command protocol to claim secure.

## GG1 application interface

Every periodic frame is ASCII: the marker GG1, comma-separated unsigned decimal fields, an asterisk, four uppercase CRC hexadecimal digits, and LF. CRC is CCITT-FALSE, polynomial 0x1021, initial value 0xFFFF, computed over everything before the asterisk. The known check string 123456789 yields 0x29B1. CRC detects corruption, not a malicious sender or authentic hardware identity.

Field order is now, samples, missed, unknown, tx_errors, fault, raw, qualified, valid, run, then bins0 through bins15, segments0 through segments15, and peaks0 through peaks15. All are uint32 values. Raw and qualified are four-bit masks; valid is zero or one. The theoretical maximum record is 647 bytes, within the 768-byte firmware buffer. Encoding returns an explicit byte count and does not promise a terminating NUL. Transmission is attempted at approximately 100 ms intervals; snapshots are cumulative, not one event per packet. Unreported acquisition samples are not necessarily lost packets.

The application rejects malformed framing, bad CRCs, range violations, broken histogram conservation, impossible segment bounds, backward counters and contradictory same-time snapshots. Across sparse snapshots it bounds acquisition by device-time growth and limits histogram and peak growth to newly acquired samples. Adjacent acquired endpoints get exact interval-attribution checks. Necessary sparse checks do not reconstruct every hidden intermediate topology; a malicious CRC-valid sequence may still describe an unobserved plausible history. Do not treat the file format as authenticated evidence.

The desktop has a connection bar, physical/predicted contact grid, a sixteen-row histogram screen and a protocol/limits screen. Green denotes a retained physical closure; orange denotes a predicted missing corner. Unqualified and halted vectors are gray. The summary explicitly calls the grid the latest retained snapshot, not proof of present position. Disconnection does not erase captured history or its provenance. Replay is always offline, and failed replay or connection setup preserves prior evidence.

Live status requires advancing acquisition, not merely recently received bytes. Two advancing snapshots are required before LIVE is shown; a half-second without sample advance becomes stale. A host reception pause over one second or more than one second of cumulative disagreement between host and device progress blocks the session and requires reconnect. This fixed allowance may reject long sessions with oscillator drift; it is intentionally not an indefinitely growing backlog allowance. Queued serial data and reset ambiguity remain reasons not to use this device for consequential real-time decisions.

Replay reads only regular files up to 2 MiB, rejects empty recordings and partial final frames, and never blocks waiting on a FIFO. Live framing can discard an initial truncated suffix and counts the discard. History is bounded to the latest one thousand snapshots, while the final cumulative histogram still covers the device's retained session. CSV export includes author, live/replay provenance, connection and freshness state, parser errors and warning text before the data rows. Exports are private mode 0600 and refuse overwrite. Export is not a raw GG1 recording and cannot be loaded as one. No data is transmitted over a network; deleting exported files is the user's retention policy.

## Build, run and reproduce validation

Install an ARM embedded GCC toolchain, GNU Make, host GCC, Python 3 with Tk, KiCad 9 with Python pcbnew and footprint libraries, and Xvfb for headless GUI tests. Runtime dependencies are operating-system packages; there is no pip or JavaScript dependency graph and no package lockfile is required. Tested versions are recorded in VALIDATION.md.

    make -C firmware clean all test fixture
    python3 -m unittest discover -s tests -p test_app.py
    xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py'
    /usr/bin/python3 tools/check_hardware.py
    python3 app/workbench.py

Run those commands from this device directory. The fixture target generates synthetic GG1 records with the real C engine and encoder. For an offline demonstration use `python3 app/workbench.py firmware/build/demo.gg1`. The target binary is firmware/build/ghost-grid.bin. After inspecting the wiring and confirming the exact Nucleo, `make -C firmware flash` invokes OpenOCD with the ST-LINK and STM32F4 configurations. No physical flashing was performed during development.

The CAD generator is available for review and regeneration with `/usr/bin/python3 tools/generate_hardware.py`. It emits native schematic, PCB, project, symbol library and connectivity manifest files. The checker runs ERC and DRC, exports a fresh schematic netlist, and compares every pin/net, value and footprint identity against the PCB and manifest. Zero rule violations establish only those automated checks; they do not establish signal integrity, enclosure safety or fabrication readiness. A rendered schematic was also visually inspected for readable net labels and connector mapping.

## Physical commissioning and remaining work

Before applying USB power, inspect resistor values, shorts, solder bridges and every connector orientation. With the carrier disconnected, measure ground continuity and confirm that each field switch is isolated from other equipment. Power the Nucleo alone, verify its 3.3 V rail, then attach the carrier with power removed. Reapply power through a protected source and measure all four open and closed input voltages. Confirm actual input thresholds and check that exercising one switch changes only its intended bit.

Perform all sixteen static contact combinations with independent observation. Each of the four triples should show precisely its missing corner as predicted phantom. The full set should show four physical closures and no extra phantom. Then introduce bounce, long cable motion and deliberate disconnection to measure how much coverage becomes unknown. Compare time intervals against an independent reference before interpreting them quantitatively. A test that only looks at the final app colors does not validate sampling, electrical isolation or serial continuity.

Finally build the actual mechanical exclusion fixture and compare its contacts against video during slow operation. Establish repeatability and document any observer-induced loading or altered ergonomics. Fast transitions below the qualification window can be missed entirely; the application is intentionally not a high-speed switch-bounce analyzer. An unexpected result may come from wiring, contact wetting, fixture mechanics or the ideal-model assumptions rather than a flawed keyboard. Resolve these causes before making design decisions. Production enclosure work, EMC/ESD assessment, manufacturing test coverage and human-factors validation remain future engineering tasks, not completed deliverables disguised by successful compilation.
