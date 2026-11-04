# Grip Lattice validation record

Author: jayis1

## Reproduction

Run `/usr/bin/python3 tools/verify.py` from this directory. It performs clean target and host builds, sanitizer tests, C-generated fixture replay, real Tk event-loop tests under Xvfb, Python compilation, KiCad ERC/DRC, schematic/PCB/manifest comparison and author/size checks. It does not install dependencies, program hardware or publish changes.

## Executed checks

- GNU Arm Embedded GCC 14.2.1 linked the Cortex-M4 target successfully: 2712 bytes text, zero initialized data, 788 bytes BSS. These exclude runtime stack usage; the linker reserves stack headroom.
- Production firmware contains 546 nonblank, noncomment C lines, excluding headers and tests. README contains 3381 whitespace-delimited words.
- Host engine tests passed with AddressSanitizer and UndefinedBehaviorSanitizer: held pedal at startup, exact baseline threshold, independent contact qualification under chatter, active/result conservation, raw glitch uncertainty, segment splitting, retained results, repeated trials, sampling loss, rejecting deadline, cumulative counter exhaustion and unsigned timestamp wrap.
- All 4096 four-word walks over the eight contact combinations passed an independently accumulated duration/run/maximum comparison. That comparator consumes the engine's qualified endpoints; qualification is separately boundary-tested, not independently reimplemented by this oracle.
- Production clock-driver register simulations passed bounded failure and pin/configuration masks. UART-driver tests passed invalid/busy submission rejection, nonblocking drain, timeout, unsigned wrap and recovery.
- Eleven Python tests passed, including the real C encoder's synthetic fixture through parser and model, corrupted and truncated framing, impossible CRC-valid durations, frozen-result mutation, freshness/backlog rejection, sparse growth rejection, private non-overwriting export, FIFO replay rejection, and actual POSIX PTY restoration/hangup.
- GUI tests used a real Tk event loop under Xvfb, exercised all three screens, retained replay after a failed non-TTY connection, received the C-encoded fixture via a paced serial PTY, rejected a second connection and detected hangup while preserving source provenance. PTY transport is simulated electrical input, not a real USB target.
- KiCad 9.0.2 ERC: zero violations. DRC: zero violations and zero unconnected items.
- A freshly exported schematic XML netlist matched the PCB and manifest for all 14 components, 30 pins and 10 named nets, including values and footprint identifiers.
- Schematic SVG was exported, rendered and visually inspected: four independently labelled resistor/input networks, six connectors and attribution were legible with no observed label overlap. This is a drawing review, not physical wiring validation.
- Authorship is present in every delivered source/document/CAD file using comments or supported metadata. Generated build products and local KiCad preference files are ignored.

## Owner review and limitations

This was a standalone-owner execution, not an independent collective approval. Advisory reports were used for repository boundaries and acceptance gates. Repository identity was established from the actual checkout and origin remote; conflicting advisory claims naming other repositories were not adopted.

Review traced the GPIO bit mapping, carrier nets, qualified input polarity, trial gate, duration conservation, target encoder order and companion FIELDS order. Both target and app preserve eight mutually exclusive combinations rather than summing simultaneous contacts as elapsed time. Uncertain acquired intervals break segments and remain in the denominator. HALT wall-clock progress is never labelled valid acquisition. Counter and result rollback checks are necessary endpoint constraints, not complete proof of hidden history.

Physical PCB fabrication, assembled electrical measurements, target flashing, switch/magnet cross-actuation, paddle travel and return force, clock calibration, enclosure safety, ESD, EMI, regulatory compliance and participant usefulness remain unverified. The PCB is an external-development-board carrier, not a standalone MCU product. The target maintains only volatile current/last trial arrays; completed trials may be missed if telemetry is lost and another trial starts. No globally novel or patent-clearance claim is made. No pressure, force, injury-risk or ergonomic recommendation is produced.
