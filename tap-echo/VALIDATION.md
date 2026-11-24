# Tap Echo validation record

Author: jayis1

Revision A was validated as source code and CAD, not as fabricated hardware. This run used degraded standalone-owner review; advisory peer reports informed the checks, but no full collective review or independent hardware sign-off occurred.

## Executed checks

- Clean temporary copy of only the intended device files: ARM build, native tests, Python/Tk tests, KiCad checks and CLI replay all passed. No generated binary from a neighboring device was used.
- Cortex-M4 target ELF linked with `arm-none-eabi-gcc`, `-Wall -Wextra -Werror`, freestanding startup and linker script. GNU size: text 2632 bytes, initialized data 0 bytes, BSS 1044 bytes. The engine and frame buffer also use stack; BSS is not total runtime RAM. The linker reserves stack headroom.
- Four native sanitizer-enabled C test executables passed: engine, independent phrase oracle, clock/configuration and nonblocking UART.
- The independent oracle checked 192 combinations of three-tap reference identity, attempt identity and tolerance-boundary timing. Separate tests checked capture completion immediately before and exactly at the rejecting deadline.
- Six Python unittest cases passed under Xvfb, including actual POSIX pseudo-terminal transport, Tk screens, duplicate-counter staleness, non-TTY connection failure, replay and private JSON export.
- Real C-engine synthetic replay produced five snapshots and ended with one matched comparison. This is a simulated trace, not a physical experiment.
- KiCad 9.0.2 ERC: zero violations. DRC: zero violations and zero unconnected items. Fresh schematic XML, PCB pads and manifest agree for fourteen components, thirty pins and ten nets, including values and footprint identities.
- Schematic SVG was rendered to PNG and visually inspected: the four resistor/contact branches, external supply/input headers and author/title block are legible, with no observed label collision.
- Release checker counted 3029 README words and 593 nonblank, noncomment lines in production firmware `.c` files, excluding headers and tests. Attribution and JSON checks passed across intended deliverables.
- Calculated passive network values: 0.300 mA per closed branch, 0.300 V ideal sense level, 1.200 mA and 3.96 mW with all four contacts closed.

## Review findings resolved

Tests initially demonstrated that a short raw A-to-B exchange could evade simultaneous-overlap detection. The engine now retains the raw identity union until qualified release, and the regression passes. A GUI regression demonstrated that duplicate acquisition counters could renew freshness; these no longer refresh its progress timer. Replay now clears live-link diagnostics before export, and exports retain source and freshness qualifications. An existing non-TTY serial path is reported as a connection error rather than escaping the GUI callback.

## Limits and deferred physical checks

No fabrication, actual ST-LINK programming, scope measurements, power consumption measurement, mechanical assembly, human study, ESD test or regulatory assessment was performed. ERC of this passive carrier does not validate the external Nucleo supply or harness. The ST manual retrieval failed over HTTP/2 and timed out over HTTP/1.1; the inherited connector/bridge mapping must be checked against the purchased board before powering. Worldwide novelty and product utility are not established.

The serial protocol is unauthenticated. A recent advancing acquisition counter does not prove absence of buffered historical traffic. Reset epochs are heuristic without a persistent boot identifier. The GUI and exported results are supervised workshop evidence, never permission signals or medical assessments.

## Repository scope

Only `tap-echo/` belongs to this change. The pre-existing untracked `quorum-comb/` is excluded and retained. Publication uses the configured `jayis1/hardware-design-lab` default branch, `main`; no task branch is required. Scoped `git add -A -- tap-echo` replaces unrestricted staging so unrelated files cannot enter this commit. Remote commit verification is reported by the owner after publication rather than predicted in this document.
