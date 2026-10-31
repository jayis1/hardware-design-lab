# Bellows Phase validation record

Author: jayis1

## Scope and results

Revision A is a software/CAD-verified engineering prototype. No physical board, bellows fixture, calibrated sensor or flashed MCU was used. All test inputs are synthetic. The owner performed the writes and verification; advisory review did not constitute collective hardware approval.

Executed checks:

- `make -C firmware clean all test`: passed. Cortex-M4 target ELF and binary linked with GCC Arm 14.2.1, `-Wall -Wextra -Werror`, freestanding C11, supplied startup and linker script. ELF size: text 2620 bytes, initialized data 0 bytes, BSS 788 bytes. Build products are ignored, not committed.
- Three AddressSanitizer/UndefinedBehaviorSanitizer host suites passed: stroke engine/encoder, clock/configuration and nonblocking serial driver. Engine tests cover both directions and all valve categories, same-end return, one-sample raw glitch, continuity loss, endpoint conflict, timeout/rearm, transactional exhaustion, timestamp wrap, CRC reference vector, every next raw mask from both active origins, and a 20000-observation deterministic mixed trace.
- `xvfb-run -a python3 -m unittest discover -s tests -p 'test_*.py' -v`: all seven tests passed on Python 3.13.5. Coverage includes actual C-encoder fixtures, CRC-valid impossible snapshots, partial-frame resynchronization, truncated replay rejection, sparse exposure bounds, retained-result immutability, freshness, rollback, exclusive mode-0600 CSV exports, FIFO rejection, termios restoration, actual pseudo-terminal hangup, three real Tk tabs, live receive, replay, history and export workflows.
- A read-only code review identified that `termios.error` is not an `OSError` on this runtime. A failing GUI regression using `/dev/null` reproduced the uncaught constructor error; the connection handler now catches it and preserves existing replay evidence. The entire Python suite then passed. No accounting blocker was found in that review.
- `/usr/bin/python3 tools/generate_hardware.py`: generated the carrier with KiCad 9.0.2 bindings and installed standard footprints.
- `/usr/bin/python3 tools/check_hardware.py`: real KiCad schematic ERC reported zero violations; PCB DRC reported zero violations and zero unconnected items. A newly exported schematic XML netlist agreed with the PCB and manifest on 14 references, 30 pins and 10 nets, including values and full footprint identities.
- A fresh schematic SVG was rendered to PNG using `rsvg-convert` and visually inspected. Four separated contact networks and supply/signal headers are legible without label/pin overlap. An initial optional CairoSVG conversion was unavailable; librsvg successfully provided the render instead.
- `python3 tools/check_quality.py`: README 3172 whitespace-delimited words; authored firmware C/header sources 686 lines, including 614 nonempty lines after block-comment removal. This count excludes tests and generated build output. Attribution is checked on every deliverable, along with JSON parsing, connector-net assignments and selected credential patterns.
- `git diff --check`: passed before publication. Final staging is restricted by inspection to the new device and one root catalog entry. Commit author and committer must both be jayis1.

## Interpretation and outstanding physical work

Target linking is not on-target execution. Simulated register tests check software masks and bounded failure handling, not oscillator accuracy or actual peripheral behaviour. Pseudo-terminals validate host transport paths but are not USB interoperability measurements. CRC and consistency checks cannot establish authenticated identity or detect every reset without a persistent boot identifier.

ERC/DRC and netlist equality do not prove correct external wiring, current board-revision jumper settings, supply stability, contact reliability, ESD immunity or manufacturability. Confirm these on the purchased Nucleo and first assembly. The carrier is passive and does not contain the processor power network.

Before an experiment, verify switch wetting-current suitability, magnet retention, endpoint exclusion, actual open thresholds, flap disturbance, contact bounce, cable noise, timing and watchdog behaviour. Compare the directional bins with synchronized video. Do not interpret lever-position indications as measured airflow, pressure, valve sealing, efficiency or safety. Worldwide novelty is unproven. No flashing, fabrication, purchasing, pneumatic testing or regulatory certification is claimed.
