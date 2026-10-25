# Nest Settle verification record

Author: jayis1

## Executed checks

The single command `python3 nest-settle/tools/verify.py` passed from the repository root. This invokes the actual target link, sanitizer-backed C tests, Python application tests, headless Tk event loop and KiCad validation; it does not substitute simulated output for these commands.

- README: 3,131 whitespace-delimited words.
- Firmware: 514 nonblank C-source lines after stripping comments, plus 106 header lines. Tests and generated build artifacts are excluded. Shared board-support code is intentionally reused from the repository's Chord Window platform; the seating engine is new.
- ARM GNU compiler: 14.2.1 (20241119). `-Wall -Wextra -Werror`, Cortex-M4/Thumb, freestanding link with the supplied startup and linker script succeeded.
- Target image: 2,216 bytes text, zero initialized data, 276 bytes static BSS. The engine and frame are automatic variables on the stack, so BSS is not total runtime RAM consumption. The linker reserves at least 4 KiB stack headroom.
- Native engine tests with AddressSanitizer and UndefinedBehaviorSanitizer: passed seating, corner dropout, independent debounce, short raw glitches, empty-baseline recovery, timeout, acquisition gaps, ordinary timestamp wrap, event-counter exhaustion and CRC vector cases.
- Native clock tests with sanitizers: passed bounded failed-clock paths, restored masks, and input/peripheral configuration checks.
- Python 3.13.5 byte compilation: passed.
- Seven Python protocol/model tests: passed the real C encoder fixture, truncated-frame resynchronization, replay EOF handling, malformed result rejection, duplicate freshness, rollback blocking, host pause/backlog blocking, metadata export and FIFO rejection.
- Actual Tk event-loop test under Xvfb: passed Linux PTY connection, live display, trial table, all three tabs, comparison, CSV creation, disconnect/hangup, replay, reconnect clearing and stale/backlogged input blocking. All test data is synthetic; no physical USB capture is implied.
- KiCad 9.0.2 ERC: zero violations.
- KiCad 9.0.2 PCB DRC: zero violations and zero unconnected items.
- Fresh exported XML connectivity comparison: 14 components, 30 pins, 10 named nets; every PCB reference, value, footprint identity and pin/net agrees with the schematic and manifest.
- Schematic SVG export succeeded and its rendered image was visually inspected. Four separate resistor/contact channels and power/MCU connectors are legible; labels distinguish SEAT_A through SEAT_D from SENSE0 through SENSE3.
- Authorship gate: every deliverable contains jayis1 in format-valid source comments, metadata, title blocks or text. Build products and KiCad per-user preferences are ignored.

## Review resolutions

Review found that timeout omitted the final observed millisecond of full-contact dwell. The timeout boundary now integrates that interval before finishing, and a continuously seated timeout test requires full_ms equal to 30,000.

Review also found that a delayed host could present queued old snapshots as live. After a live reception pause exceeding one second, acceptance now freezes until an explicit reconnect. A regression advances host time ten seconds while device time advances only 50 ms and verifies blocked/stale status. This conservative policy sacrifices seamless recovery rather than implying freshness from receipt alone.

A contact opening on the exact 250 ms settling milestone deliberately does not qualify as settled. The README documents this ordering rule and a regression verifies that longest/full_ms can equal 250 while reason remains UNSETTLED. Timeout similarly takes precedence over a removal or new milestone at its boundary.

## Boundaries of the evidence

No PCB has been fabricated, no firmware has been flashed to a physical board, and no switch fixture has been assembled or measured during this run. Rail tolerance, oscillator error, electrical noise, mechanical hysteresis, spring loading, input threshold margins, real-device unplug behavior and electromagnetic compatibility remain unverified. ERC on passive symbols cannot establish external power correctness. The PCB is a passive carrier, not a complete MCU design.

The app's receive-only protocol cannot prove session identity after all possible reset/loss combinations. It has no persistent boot identifier and no durable event log. A missing event is reported as missing, not reconstructed. Complete withdrawal is excluded from dropout counts, while sequential withdrawal may create partial dropouts; interpretation requires a controlled handling procedure. These limitations prohibit machine authorization or calibrated dimensional claims.

## Reproduction

Run `python3 nest-settle/tools/verify.py`. To regenerate the synthetic replay fixture alone, run `make -C nest-settle/firmware test`; the fixture is written under the ignored build directory. To inspect the custom electronics, open kicad/device.kicad_pro with KiCad 9. Regeneration through tools/generate_hardware.py is deliberate and may update generated UUIDs in library footprints; validation does not regenerate or silently rewrite the design.
