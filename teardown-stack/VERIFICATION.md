# Teardown Stack verification record

Author: jayis1

This is a standalone-owner engineering verification record, not collective approval or physical certification. The supplied peer reports were advisory; they did not test these artifacts.

## Executed checks

- Repository identity: origin is https://github.com/jayis1/hardware-design-lab.git; remote HEAD resolves to main. Initial working tree was clean. git pull --ff-only completed before design selection. No task branch was created. daily-ideas was not modified.
- Inventory: listed top-level device directories and README purpose excerpts; inspected the nearest sequence-oriented designs and searched for teardown, disassembly, LIFO and reverse-order concepts. The selected concept is dynamic reverse retrieval of removable parts cups, not fixed fold sequencing or independent pocket residence.
- Target build: make -C teardown-stack/firmware all test, using arm-none-eabi-gcc 14.2.1. Cortex-M4 ELF and binary linked successfully with warnings as errors. ELF size: text 2176 bytes, initialized data 0 bytes, static BSS 788 bytes. BSS excludes the runtime stack. The supplied linker script maps 512 KiB flash and 96 KiB RAM.
- C engine checks: all 576 combinations of four-cup deposit and retrieval permutations; 100,000 randomized sampled masks with continuity gaps and invariant checks; independent-channel chatter; glitches; startup occupied; valid dock reuse; wrong retrieval; simultaneous transitions; correlation-loss recovery; timer wrap; transactional exhaustion; explicit faults; serializer rejection of invalid states and insufficient output capacity. Host builds use AddressSanitizer and UndefinedBehaviorSanitizer.
- Shared driver checks: bounded clock failure paths, input-mode validation, UART bounds, busy rejection, nonblocking drain, timeout and wrap behavior. No external hardware was accessed by these host tests.
- App checks: xvfb-run -a python3 -m unittest discover -s teardown-stack/tests -p 'test_*.py': 10 tests passed using Python 3.13.5. Tests consume real C-encoder synthetic fixtures, validate CRC and impossible snapshots, exercise framing recovery, duplicate and reset suspicion, freshness/backlog, FIFO rejection and bounded buffering.
- Real GUI event-loop checks: three tabs, Linux pseudoterminal acquisition, five accepted history rows, replay, evidence retention, disconnect/HUP handling, double-connect refusal, terminal attribute restoration including constructor failure, private mode-0600 CSV creation and overwrite refusal. Transport is a PTY, not a physical Nucleo.
- KiCad 9.0.2: hardware generation produced 14 components and 10 nets. Schematic ERC: zero violations. PCB DRC: zero violations and zero unconnected items. A newly exported XML schematic netlist matched all 30 PCB and manifest pins, component values and full library footprint identities.
- Visual inspection: exported and rendered the schematic and combined copper/silkscreen board views. Four cup channels, supply and signal headers, reference labels, routed traces and board outline were visible without obvious label overlap. Rendering used rsvg-convert; Python cairosvg was unavailable and was not needed.
- Source quantity: README has 3220 whitespace-delimited words. Production firmware has 620 C/header lines, of which 578 are nonblank and noncomment. Tests, binaries, linker scripts and generated CAD are excluded from the firmware count.
- Attribution is checked in every tracked or nonignored project file by tools/check_quality.py. Build products, bytecode and KiCad preference files are ignored. Shared platform sources retain jayis1 attribution; upstream installed footprint material is not relicensed.

## Reproduce

Run from the repository root:

    make -C teardown-stack/firmware clean all test
    xvfb-run -a python3 -m unittest discover -s teardown-stack/tests -p 'test_*.py'
    python3 teardown-stack/tools/check_hardware.py
    python3 teardown-stack/tools/check_quality.py
    git diff --check

Generate the synthetic recording for app replay, outside the repository:

    teardown-stack/firmware/build/test-engine --fixture > "$TMPDIR/teardown-stack-synthetic.txt"

The test executable runs its assertions before emitting the five protocol frames. This recording represents software stimulus, not measured cup motion. The app can open it from Connect / fixture after disconnecting a live port.

## Limits and outstanding physical work

No PCB was fabricated, no MCU was programmed, and no switch, cup, power rail, timing signal or repaired mechanism was physically measured. Prototype cup/dock dimensions are suggestions rather than fitted CAD. Electrical ESD behavior, contact wetting reliability, switch overtravel, USB current, oscillator timing, watchdog recovery and mechanical repeatability require bench validation. The board has shared ground and no isolation or surge-protection claim. A qualified hardware review is required before fabrication or use around sensitive electronics.

The system observes dock identity, not cup contents, unique cup identity, tool use or assembly correctness. It supports at most four simultaneously docked cups and no durable storage. CRC is not authentication. Reset inference can miss plausible forward-looking resets because no persistent boot identifier exists. Long host sessions may conservatively block on HSI drift. Logs retain only the latest 1000 accepted snapshots, although cumulative MCU totals survive serial disconnection while power remains present. None of these limitations is removed by passing software tests.

Global novelty and product usefulness are not established. The linked organizer products are adjacent prior art, and a small repair-class comparison with a numbered mat remains the proposed product-validation experiment.
