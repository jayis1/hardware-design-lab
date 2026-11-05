# Toggle Hold revision A verification

Author and creator: jayis1

## Executed checks

This report records software and file-level CAD verification, not fabricated-hardware measurements. Review was performed in standalone-owner mode with advisory review rather than a full collective hardware review.

- Repository discovery identified `jayis1/hardware-design-lab`, with remote default branch `main`. The starting tree was clean and `git pull --ff-only` reported already up to date. Device directories and all concept titles were inventoried; Catch Witness and Detent Loop were read in detail before selecting the unloaded bistable-retention contract. No worldwide novelty claim is made.
- `make -C firmware clean all test fixture` passed. GNU Arm Embedded GCC 14.2.1 linked the real Cortex-M4 target with `-Wall -Wextra -Werror`, supplied startup, register drivers and linker script. The final ELF reports 2596 bytes text, zero initialized data and 788 bytes BSS. The binary was generated with objcopy.
- Native engine, clock and UART test executables passed with address and undefined-behavior sanitizers. Tests cover both origin directions, 499/500 interval retention, raw rebound exactly at completion, pressure recontact, early release, wrong-direction/overlap conflict, deadline precedence, sampling loss, independent qualification, held-paddle startup, time wrap, sequence exhaustion, CRC check vector and bounded encoding.
- All sixteen final raw input masks were compared against an independent outcome oracle in both origin directions at the final HOLD interval. A completion tied to the rejecting ten-second deadline is explicitly tested as timeout, not retention. Invalid phase milestones and an inconsistent retained hold interval are rejected by the C validator.
- `xvfb-run -a python3 tests/test_app.py` passed all eleven test methods. Tests use actual production C-encoder frames for both directions, the real Tk event loop, Linux pseudoterminals, strict replay, CRC-valid impossible snapshots, disconnect handling, failed replacement connection, duplicate connection rejection, terminal evidence freezing, freshness expiry, private CSV and overwrite refusal. Replay rejects nonregular files, empty files, oversized files and partial tails. GUI coverage includes all three tabs and all retained rows, not just imports.
- Advisory review identified two host-side issues that were reproduced with failing regressions and corrected: adjacent snapshots could claim qualification too quickly or acquired time without matching samples, and ordinary intervening acquisition samples were mislabeled as frame losses. The final model bounds age and hold growth by sample-sequence growth and exports `unreported_samples`, not packet loss. These checks remain necessary bounds rather than complete hidden-history reconstruction.
- `/usr/bin/python3 tools/check_hardware.py` passed with KiCad 9.0.2: zero ERC violations, zero DRC violations, zero unconnected items. A freshly exported schematic netlist, PCB pads and manifest matched on fourteen components, thirty pins and ten nets, including values and full footprint identities. No rule exclusions were added.
- Schematic SVG export succeeded and its rasterized rendering was visually inspected. Four endpoint/plunger rows, supply and MCU harness labels, resistor values and author title were readable without observed label collisions. This is not a physical routing or assembly inspection.
- README contains 3469 whitespace-delimited words. Production firmware contains 558 nonblank comment-free `.c` lines plus 80 `.h` lines, excluding tests, generated build products and comments. `tools/verify.py` checks those thresholds, required files, every delivered file's jayis1 attribution, and the shared endpoint/plunger hardware pin contract.

## Reproduce

From this device directory:

    make -C firmware clean all test fixture
    xvfb-run -a python3 tests/test_app.py
    /usr/bin/python3 tools/check_hardware.py
    python3 tools/verify.py

KiCad regeneration, if needed, uses `/usr/bin/python3 tools/generate_hardware.py`. The import script documents provenance of the reused jayis1 platform and deliberately refuses overwriting existing files; it is not a build prerequisite. Generated firmware binaries, host executables, synthetic recordings, Python caches and local KiCad preferences are ignored rather than committed.

## Not verified and remaining limitations

No board has been fabricated or electrically energized, no target has been programmed, and no specimen or physical pressure-witness fixture has been assembled. MCU timing, supply behavior, input thresholds, contact reliability, sensor force, plunger clearance, mechanical disturbance, ESD, EMC, enclosure fit and user benefit are unverified. Passive ERC does not validate the purchased Nucleo or its jumper harness. Automated rule checks do not authorize manufacturing or safety-critical deployment.

Pressure contacts are displacement witnesses, not calibrated force transducers. Retained means a finite acquired unloaded-indication window, not permanent stability or measured holding force. Short pulses can be filtered or missed. A raw glitch during HOLD intentionally rejects retention. Broken or shorted leads can imitate legitimate contacts because there is no far-end supervision. One trial is retained per reset; reset loses volatile evidence. CRC provides no authentication, and the protocol has no persistent boot identity. Some resets and initial transport delays cannot be distinguished from valid traffic.

The fixture dimensions in README are design recommendations, not released machining or enclosure files. Before useful measurement, perform the documented electrical bring-up and guarded mechanical comparison against independent observation, including with/without-sensor checks. This package is an inspectable engineering prototype, not a validated commercial instrument.
