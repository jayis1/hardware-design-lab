# Domino Ledger — validation record

Author: jayis1

## Scope

Revision A is a build-tested, unbuilt prototype. All input sequences below are synthetic. The tests do not establish magnetic sensing accuracy, real-time behavior on a flashed MCU, mechanical noninterference, manufacturability beyond the reported CAD rules, or worldwide product novelty.

## Reproduction

From the repository root:

    python3 domino-ledger/tools/verify.py

This entry point cleans and builds the target, executes native sanitizer tests, generates a DL1 recording using the production C encoder, exercises the Python/Tk companion, and checks the committed CAD files. It does not regenerate CAD automatically, flash a board, modify another device, install packages, open a network service or push Git changes.

Tested environment:

| Tool | Observed version |
| --- | --- |
| ARM GNU GCC | 14.2.1 20241119, package 15:14.2.rel1-1 |
| Native GCC | 14.2.0 |
| Python | 3.13.5 |
| Tk | 8.6 |
| KiCad CLI and pcbnew | 9.0.2 |

The app has no pip/npm dependencies. Linux termios, Tk and an X server are required; Xvfb supplies a display during tests. CAD scripts require system pcbnew and KiCad's installed footprint libraries. Compiler and KiCad major-version changes require rerunning these checks rather than assuming equivalent output.

## Executed results

- `make -C firmware clean all test`: passed. Cortex-M4 target ELF and BIN generated with freestanding C11, warnings as errors and no vendor SDK. Size output: text 2812 bytes, data 0 bytes, BSS 788 bytes. `arm-none-eabi-readelf -h` independently identified an executable ELF32 ARM image, entry 0x08000737.
- Engine sanitizer suite: passed all 24 first-fall permutations, simultaneous ties, re-erection, immutable completed evidence, rearming, exact timeout, all-down quiet boundary, raw glitch interruption, unrelated-channel chatter, missed sample, modular clock wrap, counter exhaustion and bounded serialization assertions.
- Independent station-history oracle: all 65,536 four-plateau words passed. Each word consists of four arbitrary four-bit contact masks held for five acquired samples after an upright baseline. A separate per-station array model checks immutable first departure offsets and retained returns. The production serializer must accept every intermediate engine state. This covers repeated and simultaneous operations, not only distinct fall permutations.
- Clock-driver tests: passed bounded startup-failure paths, masks and GPIO/UART configuration readback.
- Serial-driver tests: passed buffer bounds, busy rejection, nonblocking drain, timer wrap and timeout recovery.
- Python and actual Tk event-loop tests: 18 passed under Xvfb. Covered the real C-generated full/disturbed/partial recording; CRC and framing failure; truncated-frame resynchronization; partial EOF; invalid records; frozen offsets; contradictory duplicates; completion-bound disturbed-count growth; private export and overwrite refusal; serial attribute restoration; invalid TTY; FIFO replay rejection; live reception pause; counter rollback; all three GUI tabs; PTY capture and hangup; failed replacement preserving evidence; and reconnect clearing old labels/history.
- Python `compileall`: passed for app, tools and tests.
- KiCad schematic ERC: zero violations.
- KiCad PCB DRC: zero violations and zero unconnected items.
- Fresh exported schematic XML netlist versus manifest and actual PCB pads: matched all 14 components, 30 pins and 10 nets, including values and full library footprint identities.
- Schematic and combined PCB layer SVG exports were rendered and visually inspected for readable labels, component placement, connector routing and outline. No schematic text collision was observed. Rendered inspection images are temporary review artifacts, not committed fabrication outputs.
- README size: 3326 whitespace-delimited words. Production `.c` sources: 552 nonblank/noncomment lines, excluding headers, tests and generated/vendor code. This includes deliberately reused board support as disclosed in the README.
- Every deliverable contains format-valid jayis1 attribution; upstream footprint licensing remains acknowledged. Build products, Python caches and local KiCad preferences are ignored.

## Review correction

Read-only advisory review identified a sparse-telemetry weakness: a completed clean result could gain a disturbed count without another completion. A regression first demonstrated the acceptance error. The companion now bounds disturbed-count growth by completed-count growth, rejecting that transition while preserving the actual encoder fixture. The advertised verifier and this record were completed before publication. Review was advisory; the owner performed all repository writes and verification.

## Outstanding physical gates

No physical board was available, so `make flash` was not run. A target build is not evidence of successful programming, watchdog reset, clock accuracy or electrical input margins. Before use, inspect the purchased Nucleo revision and supply configuration, test the carrier without sensors, measure logic levels, verify header orientation, flash only the intended board, check sampling and UART timing, and test watchdog and real cable removal.

The reed/magnet fixture is a mechanical design hypothesis. Validate operation and release angles, contact dwell, cross-coupling between magnets, bounce, hinge disturbance and real first-fall/return association against video. Compare sensor-free and instrumented cascades. The instrument cannot identify a disconnected sensor uniquely, reconstruct hidden intermediate trials, authenticate an imported recording, or prove causality from timing. ESD, EMC, regulatory compliance, production tolerances and durability remain untested.
