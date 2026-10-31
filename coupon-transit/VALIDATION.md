# Coupon Transit validation record

Author: jayis1

## Executed checks

This is a standalone-owner engineering run with advisory review, not a full collective approval. All original files and the repository commit credit jayis1. No physical device was connected, flashed or measured.

Commands run from the repository root:

    make -C coupon-transit/firmware clean all test
    xvfb-run -a /usr/bin/python3 -m unittest discover -s coupon-transit/tests -p 'test_*.py' -v
    /usr/bin/python3 coupon-transit/tools/check_quality.py
    /usr/bin/python3 coupon-transit/tools/check_hardware.py
    kicad-cli sch export svg -o <scratch-render-directory>/ coupon-transit/kicad/device.kicad_sch
    git diff --check

Tool versions: arm-none-eabi-gcc 14.2.1; native GCC 14.2.0; Python 3.13.5; KiCad CLI 9.0.2. Builds use -Wall -Wextra -Werror. Host C tests use AddressSanitizer and UndefinedBehaviorSanitizer.

The clean target build linked coupon-transit.elf and produced coupon-transit.bin. GNU size reported text 2552 bytes, initialized data 0, and BSS 788. The linker reserves at least 4096 bytes between static storage and stack end. Stack high-water usage and physical execution remain unmeasured.

All three C test executables passed: engine/encoder, clock/configuration, and serial transport. Engine tests cover every combination of both holders' origin and destination; return versus delivery; simultaneous independent moves; timeout at 5000 ms versus completion at 4999; direct handover; raw contradictory arrival; unrelated-channel chatter; sampling gaps; startup absent/contradictory; terminal freeze; uptime wrap; CRC known vector; buffer bounds; and transactional overflow rollback across both holders. In addition, all 4096 six-step words over a holder's four dock masks are compared against an independent dock/away ordering oracle, with conservation checked on every acquisition sample. This enumeration uses settled endpoints; it is not an exhaustive analog bounce model.

Nine Python unittest cases passed, including actual Tk event-loop operation under Xvfb with a Linux pseudoterminal. The tests consume five synthetic frames emitted by the real C encoder, exercise all three tabs, live progress, duplicate-connect rejection, unplug detection, terminal restoration, failed connection setup, replay, CSV creation with mode 0600, overwrite refusal, malformed and CRC-valid impossible data, empty/partial/FIFO replay rejection, duplicate freshness, cumulative backlog, and sparse accounting contradictions. These are simulated serial inputs, not physical USB delivery tests.

Read-only code review exposed three missing sparse-stream guards: impossible return/start throughput, elapsed rollback during a continuing transit, and latest-outcome labels inconsistent with new completion counters. Regression tests reproduced failure before correction. The owner added start/rearm bounds, exact continuing-transit progression, latest-kind constraints and single-delivery sum/maximum checks, then reran the full suite successfully. Sparse validation remains a set of necessary bounds, not a complete reconstruction or authenticity proof.

Native schematic ERC: zero violations. Native PCB DRC: zero violations and zero unconnected items. Fresh schematic XML netlist, actual board pads and connectivity manifest agree on all 14 references, 30 pins and 10 nets, including component values and full footprint-library identities. The exported schematic was rasterized and visually inspected: contact labels, resistor values, pin numbers, connector labels and jayis1 title-block credit are readable without the earlier left-label overlap pattern. Passive ERC does not check the external Nucleo, supply, jumpers or wiring harness.

Source measurements: 711 combined firmware C/header lines; 654 nonblank lines after stripping C comments; README 3380 whitespace-delimited words. Counts exclude tests and vendor code. The quality checker requires jayis1 in every delivered text file and checks required artifacts and the four connector contracts. Generated binaries, map files, Python caches and KiCad local preferences are ignored and not published.

## Scope and provenance

The configured workspace remote was verified as https://github.com/jayis1/hardware-design-lab.git, with remote default main. The starting tree was clean and git pull --ff-only reported up to date at fcfd4d11a8f545d4692127d4f441fd91381e6a99. Device directories and purpose summaries were inspected, followed by complete contracts for the nearest rack and movement designs. Only coupon-transit/ and the root device index are changed. No task branch was created. Publication verification is recorded in the run's final report rather than claiming an unperformed push here.

The board-support, transport and carrier topology are explicitly reused from jayis1's shutter-quartet platform. Native CAD is generated from that topology with this design's connector names and title. Upstream KiCad footprint licensing is retained. An external search located elapsed sample-removal prior art; it does not establish global novelty. A direct extraction attempt was unavailable through the configured search-only backend, so the prior-art comparison is limited to the retrieved search material rather than a full patent analysis.

## Physical gates still open

Fabrication, assembly inspection, target flashing, oscillator calibration, actual current consumption, switch wetting reliability, cable interference, ESD, environmental performance and enclosure fit are unverified. The suggested switch variant and estimated BOM are not availability guarantees or current supplier quotations. Holder keying and sensor force require local mechanical trials. No coupon treatment, drying, cure, identity or chain-of-custody outcome has been validated. The release is a software-build-tested and CAD-rule-checked prototype design, not a finished or certified instrument.
