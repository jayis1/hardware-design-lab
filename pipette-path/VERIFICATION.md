# Pipette Path verification record

Author: jayis1

## Scope and baseline

The original implementation used the existing `jayis1/hardware-design-lab` checkout on remote default branch `main`, starting at `dde48d3231b2bc0ba99c2e8bc7c2dfa36b86fa32`, with an initially clean working tree. Execution was interrupted before publication. The recovery run found the untracked device directory and its root README index row, confirmed their provenance in the originating task transcript, and preserved them rather than starting a duplicate device. A fresh `git pull --ff-only` reported already up to date. No temporary branch was created. Only the new device directory and one root README index row belong to this change. This was standalone-owner execution with advisory read-only review, not a collective hardware qualification.

The device directory inventory and README descriptions were inspected for overlap. A targeted pipette/plunger search found no existing pipetting device; Creep Pair's plungers measure foam compression rather than this sequence. FoldTrace's complete purpose and contract were read as the closest sequencing neighbour. Public technique sources support the pipetting use case but do not establish global novelty.

## Reproduced checks

From the repository root, recovery repeated a clean target rebuild, all firmware and application tests, the quality checker, and fresh KiCad ERC/DRC/netlist comparison. The results below were reproduced except the explicitly historical visual inspection:

- `make -C pipette-path/firmware all test`: real ARM target link and binary generation, with `-Wall -Wextra -Werror`; production-engine tests, simulated register clock/configuration tests and UART bounds/timeout tests pass under address and undefined-behaviour sanitizers.
- Target toolchain: `arm-none-eabi-gcc 14.2.1 20241119`. Final ELF reports 2748 bytes text, zero initialized data, and 532 bytes BSS. Stack is separately reserved by the linker layout. This is a target build, not a host-only syntax check.
- Engine tests cover valid repeated cycles, missing blowout, early second stop, source loss, pre-baseline activity, contradictory contacts, gaps, timeout, timestamp wrap, independent qualification, rejected short pulses, transactional exhaustion and CRC/buffer bounds. All sixteen input masks are tested in pairs from baseline and from twelve points in the valid cycle path. Every tested state must satisfy invariants and serialize.
- `xvfb-run -a python3 -m unittest discover -s pipette-path/tests -p 'test_app.py' -v`: six tests pass. Synthetic C-encoder records span all eight non-WAIT phases. They exercise Python validation, framing/resynchronization, freshness and pause rejection, bounded replay and FIFO refusal. A real Tk event loop and Linux PTY exercise connection, duplicate-connect rejection, every screen, multi-frame display, disconnect, private CSV export, overwrite refusal, replay, reconnect clearing and port hangup.
- `python3 pipette-path/tools/check_quality.py`: README 3587 words; 597 nonblank noncomment production C lines, or 685 including headers. Tests are excluded from those source counts. Every deliverable has format-valid jayis1 attribution, required files exist, and the carrier signal contract matches the board header and manifest.
- `/usr/bin/python3 pipette-path/tools/check_hardware.py`: KiCad 9.0.2 reports zero ERC violations, zero DRC violations and zero unconnected items. Exported schematic XML, PCB pads and connectivity manifest agree on fourteen components, thirty pins and ten nets, including values and library-qualified footprints. No rule exclusions were added.
- The original implementation recorded visual inspection of schematic and combined copper/silkscreen/outline SVG exports. Recovery independently reran the CAD checks but does not claim a new visual inspection. These images are inspection aids, not fabricated board photographs.
- `git diff --check`: no whitespace errors. Build products, caches and KiCad local preferences are ignored rather than published.

The initial TDD checks failed because the engine and app modules did not yet exist. Later review regressions demonstrated two real failures: source reentry at home after removal was accepted, and contradictory phase/contact telemetry was accepted. An explicit REMOVED phase and full allowed-mask validation corrected both; the new regressions and all existing tests then passed. Follow-up read-only review found no remaining concrete blockers within those files and contracts.

## Limits and deferred physical work

No PCB was fabricated, no target was programmed, and no actual USB device, switch fixture, pipette or learner was tested. PTY transport testing is not USB electrical validation. Register simulations do not establish oscillator accuracy or watchdog behaviour on silicon. Mechanical fit, follower friction, switch force, threshold separation, ESD tolerance, liquid exclusion, current consumption and educational value require physical validation. The BOM prices are planning allocations, not quotes.

Direct retrieval of ST's UM1724 PDF failed. An alternate indexed extraction of the manufacturer's document supplied the connector and solder-bridge table; the README records that limitation. The unrelated GitHub CLI has no authenticated session, so publication uses the repository's existing HTTPS Git authentication rather than changing credentials.

The carrier is intentionally passive and relies on an external Nucleo module. The source input is a human declaration, not a liquid sensor. Successful cycles are not volumetric, contamination, safety or competency certificates. There is no persistent boot identifier and no guaranteed per-trial log. See README for the exact conservative protocol and sampling contract.

## Publication gate

The owner must inspect staged paths, commit as jayis1 with the requested device-specific subject, push main without force, and read the exact remote main ref back. The final run report supplies the resulting commit identifier and remote verification; this file does not predict a commit hash before publication.
