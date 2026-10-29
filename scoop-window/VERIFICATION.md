# Scoop Window verification

Author: jayis1

Validation date: 2026-10-29 UTC. Scope: source package and simulated inputs, not physical hardware. Execution and final review used standalone-owner mode; advisory reports were not treated as test evidence.

## Baseline and scope

Repository: jayis1/hardware-design-lab. Configured remote default branch: main. Starting commit: 7df9ac77431cd9854d6b513f7794218a3652c2cb. This publication run recovered the untracked scoop-window/ package after confirming in the originating task transcript that it was interrupted output of this same authorized cron task. No unrelated work was adopted or removed. `git pull --ff-only` reported already up to date before recovery validation. Device directories and README titles were inventoried again; the purpose sections of Clamp Relay, Pick Witness and Pantry Warden were inspected. Changes are confined to scoop-window/. No task branch was created and no daily-ideas repository was used.

## Executed gates

- `make -C scoop-window/firmware clean all test`: PASS. Real Cortex-M4 compilation and linking with arm-none-eabi-gcc 14.2.1, strict warnings, freestanding startup/linker script and no substitute host target. ELF size: text 2,524 bytes; initialized data 0; static BSS 532. Stack and automatic engine snapshots are additional RAM, not included in BSS. ELF and binary are ignored build products.
- Host C tests: three executables, built with AddressSanitizer and UndefinedBehaviorSanitizer, all PASS. Engine tests cover symmetric excursions, both scoops absent, startup without a dock baseline, gap abandonment and redocking, independent qualification under unrelated chatter, four-versus-five observation boundaries, repeated foreign openings, return/open coincidence, timer wrap, transactional overflow, impossible public snapshots, watchdog startup fault serialization, all 256 pairs of input masks, bounded encoding and the standard CRC check vector. Clock/register masks and UART busy/timeout/wrap paths are exercised against simulated registers.
- `xvfb-run -a /usr/bin/python3 -m unittest discover -s scoop-window/tests -p 'test_app.py' -v`: PASS, nine tests. Uses Python 3.13.5, real Tk event loops and Linux pseudo-terminals. Verifies three screen tabs, actual C-encoder records entering the GUI, serial fragmentation, 2,000 deterministic C-engine snapshots accepted by the companion, terminal fault labelling, private CSV creation and overwrite refusal, serial hangup, connection ownership, termios restoration, replay bounds and incomplete tails, CRC-valid impossible snapshots, duplicate/rollback/backlog handling, and terminal evidence immutability.
- `/usr/bin/python3 -m compileall -q scoop-window/app scoop-window/tools scoop-window/tests`: PASS.
- `/usr/bin/python3 scoop-window/tools/check_hardware.py`: PASS under KiCad 9.0.2. ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML netlist, PCB pads and manifest agree on 14 components, 30 pins and 10 nets, including values and full footprint library identifiers.
- `kicad-cli sch export svg`: PASS. The rendered schematic was visually inspected for legible net labels, component references, title attribution and connector layout. It is a passive carrier drawing; the external development board is not silently represented as a custom MCU design.
- `/usr/bin/python3 scoop-window/tools/check_quality.py`: PASS before publication. README has 3,618 whitespace-delimited words. Production firmware C/header sources have 692 total lines and 619 nonblank/noncomment lines, excluding tests. Required files, format-valid author credit and channel pin contract were checked. The checker excludes its own literal forbidden-word vocabulary from that vocabulary scan, not from authorship checking.
- `git diff --check`: PASS. Final staging is restricted to this device; no generated ELF, binary, map, cache, local KiCad preferences, credentials or unrelated paths belong in the commit.

Recovery reran the clean target build, all three sanitized C executables, all nine companion tests, Python compilation, CAD checks, schematic export and source-package quality gate. Existing build products were not accepted as evidence. The schematic SVG was rasterized using rsvg-convert and visually inspected; the optional CairoSVG module was unavailable and was not needed. Final review was performed by the standalone owner, not an independent implementation reviewer.

## Research and verification boundaries

The direct ST user-manual PDF download failed with an HTTP/2 error and subsequent timeouts. An alternative indexed search result from the same ST manual supplied table 16 and section 7.10, confirming PA0/PA1/PA4/PB0, CN6 3V3/ground and the USART2 solder-bridge configuration. The PDF download failure is not described as a successful local document extraction. The limited product search found existing dedicated scoop holders; it establishes adjacent work, not worldwide novelty or freedom to operate.

No physical board was fabricated or powered, no target was flashed, and no actual reed switch, USB cable, scoop, lid or bin was exercised. GPIO timing, oscillator accuracy, brownout behaviour, watchdog timing, electromagnetic susceptibility, wetting-current suitability, mounting robustness and any training benefit remain physical-validation tasks. The 0.3 mA closed-path current, 0.3 V closed SENSE level and 1.2 mA four-channel current are calculated nominal values. Nucleo consumption and purchase costs in the README are planning allowances, not measured values or supplier quotes.

The application/firmware contract preserves counts of observed dock excursions, not utensil identity or proof of contamination. Gaps abort current excursions and require renewed dock baselines. A reset can erase volatile evidence, and some resets cannot be detected without a persistent boot identity. The source package is appropriate for an explicitly bounded prototype experiment, not safety certification.

## Reproduction

Run the commands above from the repository root. For physical programming, consult the README and use the explicit `make -C firmware flash` target only after verifying wiring and the selected ST-LINK target. Automated checks do not invoke that target. The Git commit containing this report is the reproducible source revision; remote publication and branch read-back are reported separately after the push, rather than embedding an unverifiable future commit identifier here.
