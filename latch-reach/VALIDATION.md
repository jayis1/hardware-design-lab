# Latch Reach revision A validation

Author: jayis1

This report records software and CAD checks actually rerun during recovery of the interrupted device-design task. This is a standalone-owner release with advisory review, not collective approval or physical hardware validation.

## Reproducible checks

From the repository root:

```sh
make -C latch-reach/firmware -B all test fixture
xvfb-run -a python3 -m unittest discover -s latch-reach/tests -p 'test_*.py' -v
/usr/bin/python3 latch-reach/tools/check_hardware.py
```

All commands completed successfully. Tool versions were arm-none-eabi GCC 14.2.1, Python 3.13.5 and KiCad CLI 9.0.2.

- The complete Cortex-M4 firmware linked with warnings treated as errors. GNU size reported 2780 bytes text, zero initialized data and 788 bytes BSS. The binary was generated from that fresh ELF, not reused from the interrupted run.
- Three host executables passed with AddressSanitizer and UndefinedBehaviorSanitizer enabled: engine/wire, clock/register model and UART/register model.
- Engine checks cover the checkpoint matrix, disturbances, independent qualification, rearm, deadline precedence, gap termination, timestamp wrap, overflow and serialization. An independent set oracle covers all 729 six-symbol checkpoint words. Exact-deadline rejection is tested separately from the preceding successful completion.
- Eleven Python tests passed, including real Tk event-loop callbacks under Xvfb and Linux pseudo-terminal transport. The application consumed newly generated C-encoded synthetic observations, displayed eight completed trials, exported private CSV evidence, refused a second connection, retained evidence on failed connection setup, and detected PTY hangup. These are simulated transport tests, not USB-board measurements.
- KiCad schematic ERC reported zero violations. PCB DRC reported zero violations and zero unconnected items.
- Fresh schematic XML connectivity matched actual PCB pads and the authored component manifest: 14 components, 30 pins and 10 nets. Reference values and footprint identifiers also matched.
- The native schematic was exported to SVG, rendered and visually inspected. Component symbols, labeled electrical connections and jayis1 title-block attribution were visible without apparent label overlap.
- The README contains 3576 whitespace-delimited words. Production firmware contains 589 physical C-source lines, including 533 nonblank lines after comments are removed; these counts exclude headers and tests. C plus header files contain 682 physical lines and 612 nonblank noncomment lines. These volume checks are not substitutes for the functional checks above.
- Every deliverable checked contains jayis1 attribution in a format-valid comment, field or text. Build products, Python caches and local KiCad preferences are ignored rather than published.

A separate read-only code review found no concrete correctness blockers. The owner remains responsible for the publication decision; advisory findings are not hardware certification.

## Scope and remaining release gates

The custom board is a passive dry-contact carrier for the separately purchased NUCLEO-F401RE, not a standalone MCU board. No hardware was fabricated, flashed, powered or measured. The MCU board supplies regulation, decoupling, programming and USB-to-serial support; the carrier does not duplicate those functions. Its inputs are for disconnected voltage-free contacts only.

The test fixture is synthetic evidence. Mechanical positioning, contact wetting and bounce, follower loading, actual qualification timing, electrical levels, oscillator drift, watchdog operation on silicon, reset behavior, ESD and EMC remain unverified. The next engineering stage must compare physical checkpoint/latch observations against an independent reference on a detached, unloaded, guarded mechanism. No safety, retention-force, reliable-engagement or worldwide-novelty claim is made.

Git publication and remote-branch verification are reported separately after the commit is created, so this document does not predict a commit hash or claim an unperformed push.
